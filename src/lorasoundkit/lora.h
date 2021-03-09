/*******************************************************************************
* file lora.h
* LoraWAN wrapper, interface using LMIC mcci catena library
* https://github.com/mcci-catena/arduino-lmic
* don't forget to set '#define CFG_eu868 1' in the file 'lmic_project_config.h'
* author Marcel Meek
*********************************************************************************/

#ifndef __LORA_H_
#define __LORA_H_

#include <stdint.h> // uint8_t type

// external functions 
extern void loraBegin();
extern void loraSetRxHandler( void (*callback)(unsigned int, uint8_t*, unsigned int));
extern bool loraSend( int port, uint8_t* mydata, int len);


#endif // __LORA_H_
