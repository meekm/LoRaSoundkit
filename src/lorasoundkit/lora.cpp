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
#include "config.h"

// pin mappings
#if defined(ARDUINO_TTGO_LoRa32_V1)
// define IO pins TTGO LoRa32 V1
const lmic_pinmap lmic_pins {
  .nss = 18,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 14,
  .dio = {26, 33, 32},
};
#elif defined(ARDUINO_ESP32_DEV)
// define IO pins Sparkfun
const lmic_pinmap lmic_pins {
  .nss = 16,
  .rxtx = LMIC_UNUSED_PIN,
  .rst = 5,
  .dio = {26, 33, 32},
};
#else
  #error Unsupported board selection.
#endif


// handle TTN keys
static void printHex( u1_t *buf, size_t len);
static void parseHex( u1_t *byteBuffer, const char* str);
static void parseHexReverse( u1_t *byteBuffer, const char* str);

extern void os_getArtEui (u1_t* buf) { parseHexReverse( buf, APPEUI);}
extern void os_getDevEui (u1_t* buf) { parseHexReverse( buf, DEVEUI);}
extern void os_getDevKey (u1_t* buf) { parseHex( buf, APPKEY);}

static bool busy = false;
static bool ok = false;
//static osjob_t sendjob;
static void (*rxCallback)(unsigned int, uint8_t*, unsigned int) = NULL;     // TTN receive handler

// handle events
extern void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
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
              Serial.print("netid: ");   Serial.println(netid, DEC);
              Serial.print("devaddr: ");  Serial.println(devaddr, HEX); 
              Serial.print("AppSKey: "); printHex( artKey, sizeof(artKey));
              Serial.print("NwkSKey: "); printHex( nwkKey, sizeof(nwkKey));
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
            if (LMIC.dataLen) {
              Serial.printf("Received %d bytes of payload\n", LMIC.dataLen);
              
              // call application callback, to handle TTN received message
              if( rxCallback != NULL) 
                rxCallback( LMIC.frame[LMIC.dataBeg-1], &LMIC.frame[LMIC.dataBeg], LMIC.dataLen);
            }
            // Schedule next transmission
            //os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            busy = false;
            ok = true;
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
            break;

        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

//void do_send(osjob_t* j){
extern bool loraSend( int port, uint8_t* mydata, int len) {
    ok = false;
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(port, mydata, len, 0);
        Serial.println(F("Packet queued"));
    }
    busy = true;
    while( busy)
      os_runloop_once();
    return ok;
    // Next TX is scheduled after TX_COMPLETE event.
}

extern void loraBegin() {

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

    LMIC_setClockError(MAX_CLOCK_ERROR * 1 / 100);
    // Set up the channels used by the Things Network, which corresponds
    // to the defaults of most gateways. Without this, only three base
    // channels from the LoRaWAN specification are used, which certainly
    // works, so it is good for debugging, but can overload those
    // frequencies, so be sure to configure the full frequency range of
    // your network here (unless your network autoconfigures them).
    // Setting up channels should happen after LMIC_setSession, as that
    // configures the minimal channel set.

    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
    // TTN defines an additional channel at 869.525Mhz using SF9 for class B
    // devices' ping slots. LMIC does not have an easy way to define set this
    // frequency and support for class B is spotty and untested, so this
    // frequency is not configured here.

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    //LMIC_setDrTxpow(DR_SF11,14);
    LMIC_setDrTxpow(DR_SF9,14);

    // Start job (sending automatically starts OTAA too)
    //do_send(&sendjob);
}

extern void loraSetRxHandler( void (*callback)(unsigned int, uint8_t*, unsigned int)) {
    rxCallback = callback;
}

// somme TTN key convenient functions
// print byte arrray in hex
static void printHex( u1_t *buf, size_t len) {
    for (int i = 0; i < len; i++) 
        Serial.printf("%02X", buf[i]);
    Serial.printf("\n");
}

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
