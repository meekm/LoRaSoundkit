/*******************************************************************************
* file lora.h
* LoraWAN wrapper, interface using LMIC mcci catena library
* https://github.com/mcci-catena/arduino-lmic
* don't forget to set '#define CFG_eu868 1' in the file 'lmic_project_config.h'
* and add the line: '#define hal_init LMICHAL_init'
* author Marcel Meek
*********************************************************************************/

#ifndef __LORA_H_
#define __LORA_H_

#include <stdint.h> // uint8_t type

// external functions 
extern void loraBegin(const char* appeui, const char* deveui, const char* appkey);
extern void loraSetRxHandler( void (*callback)(unsigned int, uint8_t*, unsigned int));
extern void loraJoin();
extern bool loraSend( int port, uint8_t* mydata, int len);
extern bool loraConnected();
extern bool loraTxReady();
extern void loraSetWorker( void (*worker)( void));
//extern void loraSetTxComplete( void (*txComplete)(bool ok));
extern void loraSleep( int seconds);
extern void loraLoop( void);

#endif // __LORA_H_
