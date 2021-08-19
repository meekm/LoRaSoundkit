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

// specify here TTN keys

#define APPEUI "70B3D57ED003ED46"
#define APPKEY "53A4419ACE4EABF7BE9E5FB0B8A16F27"
// DEVEU is obtained from ESP board id

//api key  (MQTT username)
//NNSXS.6X7AUQOG4C7URKQ43OLIMZREFTITPGX7ZZ5UNVY.OYACRXFUVBUPIGCS7YN3TY6BOHYEEER5Z6BM3OLZ5PVRIG2TT4DQ

#endif
