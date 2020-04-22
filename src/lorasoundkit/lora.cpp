/** 
 * LoRa LMIC wrapper class implentation
 * Contains convenient mtehods
 * Marcel Meek april 2020
 * 
 * TO BE DONE, time-out, and handle download messages
 */


#include <Arduino.h>
#include <lmic.h>
#include <hal/hal.h>
//#include <SPI.h>
#include "lora.h"

// pin mappings
const lmic_pinmap lmic_pins {
    .nss = NSS,
    .rxtx = RXTX,
    .rst = RST, 
    .dio = {DIO1, DIO2, DIO3},
};

// handle TTN keys
void convertStringToByteArray(unsigned char* byteBuffer, const char* str);
void os_getArtEui (u1_t* buf) { convertStringToByteArray( buf, APPEUI);}
void os_getDevEui (u1_t* buf) { convertStringToByteArray( buf, DEVEUI);}
void os_getDevKey (u1_t* buf) { convertStringToByteArray( buf, APPKEY);}

static bool txBusy= false;

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

void LoRa::sendMsg( int port, uint8_t* buf, int len) {
    os_runloop_once();
    digitalWrite( LED_BUILTIN, HIGH);
    
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        printf("OP_TXRXPEND, not sending\n");
    } 
    else {
        // Prepare upstream data transmission at the next possible time.
        txBusy= true;
        printf("queue ttn message\n");
        LMIC_setTxData2( port, buf, len, 0);
        while( txBusy) {                             // TO BE DONE handle time out
            os_runloop_once();
        }
    }
    digitalWrite( LED_BUILTIN, LOW);
}

void LoRa::loop() {
    os_runloop_once();
}


void onEvent (ev_t ev) {
    printf("%d: ", os_getTime());
    switch(ev) {
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
            if (LMIC.txrxFlags & TXRX_ACK)
              printf("Received ack\n");
            if (LMIC.dataLen) {
              printf("Received %d bytes of payload\n", LMIC.dataLen);
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
void convertStringToByteArray(unsigned char* byteBuffer, const char* str) {
  int len = strlen(str);
  for (int i = 0; i < len; i+=2) {
    char substr[3];
    const char* strSel = &(str[i]);
    strncpy(substr, strSel, 2);
    substr[2] = '\0';
    if( len == 16)
       byteBuffer[((len-1)-i)/2] = (unsigned char)(strtol(substr, NULL, 16));   // reverse the TTN KEY string !!
    else if( len == 32)
       byteBuffer[i/2] = (unsigned char)(strtol(substr, NULL, 16));             // don't reverse the TTN KEY
    else {
      printf( "Incorrect TTN key length: %s\n", str);
      break;
    }  
  }
}
