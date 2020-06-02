/*--------------------------------------------------------------------
  This file is part of the TTN-Apeldoorn Sound Sensor.

  This code is free software:
  you can redistribute it and/or modify it under the terms of a Creative
  Commons Attribution-NonCommercial 4.0 International License
  (http://creativecommons.org/licenses/by-nc/4.0/) by
  TTN-Apeldoorn (https://www.thethingsnetwork.org/community/apeldoorn/) 

  The program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  --------------------------------------------------------------------*/

/*!
 * \file lora.cpp
 * \author Marcel Meek
 * \date See revision table in header file
 * \version see revision table in header file
 */

#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>

#include "lora.h"
#include "config.h"

// RFM95 pin mappings
#if defined(_SPARKFUN) || defined(_ESP32)
const lmic_pinmap lmic_pins {
  .nss = 16,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 5,
  .dio = {26, 33, 32},
};
#endif

#if defined(_TTGO)
const lmic_pinmap lmic_pins {
  .nss = 18,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 14,
  .dio = {26, 33, 32},
};
#endif

const int      TxTIMEOUT = 10000;                   /// wait max TxTIMEOUT msec. after a join or send
const uint8_t  DOWNLINK_DATA_SIZE = 52;             ///< Maximum payload size (not enforced)

static bool    txBusy {false};

static bool    downlink {false};                    ///< flag to indicatie that downlink dat is available.
static uint8_t downlinkPort {0};                   ///< used uplink port
static uint8_t downlinkData[DOWNLINK_DATA_SIZE] {};  ///< uplink data
static uint8_t downlinkDataSize {0};

static void (*_callback)(unsigned int, uint8_t*, unsigned int) = NULL;     // TTN receive handler

LoRa::LoRa() {
  
#ifdef VCC_ENABLE
  // For Pinoccio Scout boards
  pinMode(VCC_ENABLE, OUTPUT);
  digitalWrite(VCC_ENABLE, HIGH);
  delay(1000);
#endif
  
  // LMIC init
  os_init();
  
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
}

LoRa::~LoRa() {
  // Reset the MAC state. Session and pending data transfers will be discarded.
  LMIC_reset();
}

void LoRa::sendMsg(int port, uint8_t* buf, int len){
  os_runloop_once();

  // Check if there is not a current TX/RX job running
  if (LMIC.opmode & OP_TXRXPEND) {
    printf("OP_TXRXPEND, not sending\n");
  }
  else {
    // Prepare upstream data transmission at the next possible time.
    txBusy = true;
    printf("queue ttn message len=%d\n", len);
    LMIC_setTxData2( port, buf, len, 0);
    uint32_t start = millis();
    while ( txBusy && (millis() - start < TxTIMEOUT) ) {         //MM,  wait max TxTIMEOUT ms
      os_runloop_once();
    }
    if ( txBusy) {
      LMIC_reset();
      txBusy = false;
      printf( "ttn send failed\n");
    }
  }
}

void LoRa::process() {
  os_runloop_once();
  if(downlink){
    captureDownlinkData();
  }
}

void LoRa::receiveHandler( void (*callback)(unsigned int, uint8_t*, unsigned int)) {
  _callback = callback;
}


// handle TTN keys
void convertStringToByteArray(unsigned char* byteBuffer, const char* str);
void os_getArtEui (u1_t* buf) { convertStringToByteArray( buf, APPEUI);}
void os_getDevEui (u1_t* buf) { convertStringToByteArray( buf, DEVEUI);}
void os_getDevKey (u1_t* buf) { convertStringToByteArray( buf, APPKEY);}

void onEvent (ev_t ev) {
  printf("%d: ", os_getTime());
  switch(ev){
    case EV_SCAN_TIMEOUT:
      printf("EV_SCAN_TIMEOUT\n");
      break;
    case EV_BEACON_FOUND:
      printf("EV_BEACON_FOUND\n");
      break;
    case EV_BEACON_MISSED:
      printf("EV_BEACON_MISSED\n");
      break;
    case EV_BEACON_TRACKED:
      printf("EV_BEACON_TRACKED\n");
      break;
    case EV_JOINING:
      printf("EV_JOINING\n");
      break;
    case EV_JOINED:
      txBusy = false;           // MM added, a succesful join must also reset this flag
      printf("EV_JOINED\n");
      // Disable link check validation (automatically enabled
      // during join, but not supported by TTN at this time).
      LMIC_setLinkCheckMode(0);
      break;
    case EV_RFU1:
      printf("EV_RFU1\n");
      break;
    case EV_JOIN_FAILED:
      printf("EV_JOIN_FAILED\n");
      break;
    case EV_REJOIN_FAILED:
      printf("EV_REJOIN_FAILED\n");
      break;
    case EV_TXCOMPLETE:
      txBusy = false;
      printf("EV_TXCOMPLETE (includes waiting for RX windows)\n");
      if (LMIC.txrxFlags & TXRX_ACK){
        printf("Received ack\n");
      }
      if (LMIC.dataLen != 0){
        printf("Received %d bytes of payload\n", LMIC.dataLen);
        downlinkDataSize = LMIC.dataLen;
        downlink = true;
        downlinkPort = LMIC.frame[LMIC.dataBeg-1];
        for( int i = 0; i < downlinkDataSize; i++){
          downlinkData[i] = LMIC.frame[LMIC.dataBeg+i];
        }
        downlinkData[downlinkDataSize] = '\0';    // MM append zero char
        // MM callback added
        if( _callback != NULL) {
          _callback( downlinkPort, downlinkData, downlinkDataSize);
        }
      }
      break;
    case EV_LOST_TSYNC:
      printf("EV_LOST_TSYNC\n");
      break;
    case EV_RESET:
      printf("EV_RESET\n");
      break;
    case EV_RXCOMPLETE:
      // data received in ping slot
      printf("EV_RXCOMPLETE\n");
      break;
    case EV_LINK_DEAD:
      printf("EV_LINK_DEAD\n");
      break;
    case EV_LINK_ALIVE:
      printf("EV_LINK_ALIVE\n");
      break;
    default:
      printf("Unknown event\n");
      break;
  }
}

// convert TTN keys strings to byte array
void convertStringToByteArray(unsigned char* byteBuffer, const char* str){
  int len = strlen(str);
  for (int i = 0; i < len; i+=2){
    char substr[3];
    const char* strSel = &(str[i]);
    strncpy(substr, strSel, 2);
    substr[2] = '\0';
    if( len == 16){
      byteBuffer[((len-1)-i)/2] = (unsigned char)(strtol(substr, NULL, 16));   // reverse the TTN KEY string !!
    }
    else if( len == 32) {
      byteBuffer[i/2] = (unsigned char)(strtol(substr, NULL, 16));             // don't reverse the TTN KEY
    }
    else {
      printf( "Incorrect TTN key length: %s\n", str);
      break;
    }  
  }
}
