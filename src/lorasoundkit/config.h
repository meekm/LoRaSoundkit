/** 
 * Define here your configuration
 * - boardtype 
 * - LoRa keys
 * Marcel Meek, May 2020
 * 
 */

#ifndef _CONFIG_h /* Prevent loading library twice */
#define _CONFIG_h

// define here your LoRa board, used for MEMS and RFM95 pinnning!
//#define _SPARKFUN         // uncomment if SParkfun board
#define _TTGO               // uncomment if TTGO board

// define im milleseconds how often a message will be sent
#define CYCLECOUNT   20000  //60000

// specify here TTN keys
#define APPEUI "0000000000000000"
#define DEVEUI "0000000000000000"
#define APPKEY "00000000000000000000000000000000"

#endif
