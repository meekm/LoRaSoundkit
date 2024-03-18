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
#define CYCLETIME   120  // use 150 for RIVM and sensor.community project

// set microphone dependent correction in dB
// for SPH0645 define -1.8
// for ICS43434 define 1.5
// otherwise define 0.0
#define MIC_OFFSET 0.0

// specify here TTN keys

#define APPEUI "70B3D57ED003ED46"
#define APPKEY "53A4419ACE4EABF7BE9E5FB0B8A16F27"
// DEVEU is obtained from ESP board id

// Some aestethical options
//#define SHOWCYCLEPROGRESS   true

#endif

//api key  (MQTT password)
//NNSXS.6X7AUQOG4C7URKQ43OLIMZREFTITPGX7ZZ5UNVY.OYACRXFUVBUPIGCS7YN3TY6BOHYEEER5Z6BM3OLZ5PVRIG2TT4DQ
//NNSXS.4722K7VVPPBYWCC7Q7BXRLVC6SEYVKTHL2SSTOY.GMIEYPDIMZYHE2W5NY7ZW3W2UVAQDNGS4DY7E6IW6X65SAFMGUQQ

