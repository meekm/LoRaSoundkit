/** 
 * Define here your configuration
 * - cycletime 
 * - LoRa TTN keys
 * Marcel Meek, May 2020
 * 
 */

#ifndef _CONFIG_h /* Prevent loading library twice */
#define _CONFIG_h

// define in seconds, how often a message will be sent
#define CYCLETIME   60  // use 150 for RIVM and sensor.community project

// set microphone dependent correction in dB
// for SPH0645 define -1.8
// for ICS43434 define 1.5
// otherwise define 0.0
#define MIC_OFFSET 0.0

// specify here TTN keys

#define APPEUI "70B3D57ED003ED46"
#define APPKEY "53A4419ACE4EABF7BE9E5FB0B8A16F27"
// DEVEU is obtained from ESP board id

#endif
