/*******************************************************************************
* file lora.cpp
* LoraWAN wrapper interface using LMIC mcci catena library
* copied and adapted from ttn-otaa example https://github.com/mcci-catena/arduino-lmic
* author Marcel Meek
*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 * Copyright (c) 2018 Terry Moore, MCCI
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 *
 * This example sends a valid LoRaWAN packet with payload "Hello,
 * world!", using frequency and encryption settings matching those of
 * the The Things Network.
 *
 * This uses OTAA (Over-the-air activation), where where a DevEUI and
 * application key is configured, which are used in an over-the-air
 * activation procedure where a DevAddr and session keys are
 * assigned/generated for use with all further communication.
 *
 * Note: LoRaWAN per sub-band duty-cycle limitation is enforced (1% in
 * g1, 0.1% in g2), but not the TTN fair usage policy (which is probably
 * violated by this sketch when left running for longer)!

 * To use this sketch, first register your application and device with
 * the things network, to set or generate an AppEUI, DevEUI and AppKey.
 * Multiple devices can use the same AppEUI, but each device has its own
 * DevEUI and AppKey.
 *
 * Do not forget to define the radio type correctly in
 * arduino-lmic/project_config/lmic_project_config.h or from your BOARDS.txt.
 *
 *******************************************************************************/
#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
#include "lora.h"

// forward decl.
static void parseHex( u1_t *byteBuffer, const char* str);
static void parseHexReverse( u1_t *byteBuffer, const char* str);

// handle TTN keys
static const char *appeui, *deveui, *appkey;
extern void os_getArtEui (u1_t* buf) { parseHexReverse( buf, appeui);}
extern void os_getDevEui (u1_t* buf) { parseHexReverse( buf, deveui);}
extern void os_getDevKey (u1_t* buf) { parseHex( buf, appkey);}

static osjob_t sendjob;
static void (*workerCallback)(void) = NULL;     // worker callback
static void (*rxCallback)(unsigned int, uint8_t*, unsigned int) = NULL;     // TTN receive handler
//static void (*txCompleteCallback)(bool ok) = NULL;
static bool txReady= false;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).

// Pin mapping
const lmic_pinmap lmic_pins {
  .nss = 18,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 23,   // v1=14, v2=23
  .dio = {26, 33, 32},
};

void printHex2(unsigned v) {
    v &= 0xff;
    if (v < 16)
        Serial.print('0');
    Serial.print(v, HEX);
}

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    txReady = false;
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            {
              u4_t netid = 0;
              devaddr_t devaddr = 0;
              u1_t nwkKey[16];
              u1_t artKey[16];
              LMIC_getSessionKeys(&netid, &devaddr, nwkKey, artKey);
              Serial.print("netid: ");
              Serial.println(netid, DEC);
              Serial.print("devaddr: ");
              Serial.println(devaddr, HEX);
              Serial.print("AppSKey: ");
              for (size_t i=0; i<sizeof(artKey); ++i) {
                if (i != 0)
                  Serial.print("-");
                printHex2(artKey[i]);
              }
              Serial.println("");
              Serial.print("NwkSKey: ");
              for (size_t i=0; i<sizeof(nwkKey); ++i) {
                      if (i != 0)
                              Serial.print("-");
                      printHex2(nwkKey[i]);
              }
              Serial.println();
              LMIC_setLinkCheckMode(0);
              txReady = true;
            }
            // Disable link check validation (automatically enabled
            // during join, but because slow data rates change max TX
            // size, we don't use it in this example.
            LMIC_setLinkCheckMode(0);
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     Serial.println(F("EV_RFU1"));
        ||     break;
        */
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {    // call application callback, to handle TTN received message
              Serial.print(F("Received "));
              Serial.print(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
              if( rxCallback != NULL) 
                rxCallback( LMIC.frame[LMIC.dataBeg-1], &LMIC.frame[LMIC.dataBeg], LMIC.dataLen);
            }
            txReady = true;
            // Schedule next transmission
            //os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    Serial.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        case EV_TXCANCELED:
            Serial.println(F("EV_TXCANCELED"));
            break;
        case EV_RXSTART:
            /* do not print anything -- it wrecks timing */
            break;
        case EV_JOIN_TXCOMPLETE:
            Serial.println(F("EV_JOIN_TXCOMPLETE: no JoinAccept"));
            txReady = true;
            break;

        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

void do_send(osjob_t* j){
   if( workerCallback)
       workerCallback();
}

extern void loraBegin( const char* app, const char* dev, const char* key) {
    appeui = app;
    deveui = dev;
    appkey = key;

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

// If you are having difficulty sending messages to TTN after the first successful send,
// uncomment the next option and experiment with values (~ 1 - 5)
//#define CLOCK_ERROR  1
#ifdef CLOCK_ERROR
    LMIC_setClockError(MAX_CLOCK_ERROR * CLOCK_ERROR / 100);  // CLOCK_ERROR 1..10
#endif

    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band

    LMIC_setLinkCheckMode(0);

    LMIC_setDrTxpow(DR_SF9, 14);
    LMIC_setAdrMode(1);
}

extern bool loraConnected() {
  return (LMIC.devaddr != 0);
}

extern bool loraTxReady() {
  return txReady;
}

extern void loraSetRxHandler( void (*callback)(unsigned int, uint8_t*, unsigned int)) {
    rxCallback = callback;
}

extern void loraSetWorker( void (*worker)( void)) {
   workerCallback = worker;
}

extern void loraJoin() {
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) 
      Serial.println(F("OP_TXRXPEND, not sending"));
    else {
      LMIC_startJoining();
      LMIC_setDrTxpow(DR_SF9, 14);
      LMIC_setAdrMode(1);
    }
}

extern bool loraSend( int port, uint8_t* mydata, int len) {
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
        return false;
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(port, mydata, len, 0);
        Serial.println(F("Packet queued"));
        return true;
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

extern void loraSleep( int seconds) {
  printf("loraSleep %d sec.\n", seconds);
  txReady = false;
  os_setTimedCallback( &sendjob, os_getTime() + sec2osticks( seconds), do_send);
}

extern void loraLoop( void) {
  os_runloop_once();
}


// ****************************************
// somme convenient functions

// convert TTN key string big endian byte array
static void parseHex( u1_t *byteBuffer, const char* str){
  int len = strlen(str);
  for (int i = 0; i < len; i+=2){
    char substr[3];
    strncpy(substr, &(str[i]), 2);
    byteBuffer[i/2] = (u1_t)(strtol(substr, NULL, 16));
  }
}

// convert TTN key string to little endian byte array
static void parseHexReverse( u1_t *byteBuffer, const char* str){
  int len = strlen(str);
  for (int i = 0; i < len; i+=2){
    char substr[3];
    strncpy(substr, &(str[i]), 2);
    byteBuffer[((len-1)-i)/2] = (u1_t)(strtol(substr, NULL, 16));   // reverse the TTN KEY string !!
  }
}
