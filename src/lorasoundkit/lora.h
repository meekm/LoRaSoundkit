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
 * \file lora.h
 * \brief LoRa LMIC wrapper class for TTN-Apeldoorn Sound Sensor.
 * 
 * \author Marcel Meek
 * \date See revision table
 * \version see revision table
 * 
 * ## version
 * 
 * version | date       | who            | Comment
 * --------|------------|----------------|-------------------------------------
 * 0.1     | 22-4-2020  | Marcel Meek    | Initial release within community for review and testing within dev-team
 * 0.2     | 24-4-2020  | Remko Welling  | Added headers, Sanitize code, add Doxygen compatible comments, add include guards
 * 0.3     | 3-5-2020   | Remko Welling  | Added downlink handling.
 * 0.4     | 5-5-2020   | Remko Welling  | downlink handling completed
 * 0.5     | 31-5-2020  | Marcel Meek    | timout added when a join or send fails
 * 0.6     | 1-6-2020   | Marcel Meek    | callback for downlink added, static buffers moved from .h file to .cpp file
 * 
 * # References
 *
 * -
 *
 * # Dependencies
 * 
 */

#ifndef __LORA_H_
#define __LORA_H_

#include <stdint.h> // uint8_t type

class LoRa{
  public:
    
    LoRa();
    ~LoRa();
    
    /// \brief send data to LoRaWAN
    /// \param [in] port application port in LoRaWAN application (1 to 99)
    /// \param [in] buf pointer to character array that contains payload to be sent.
    /// \param [in] len Lenght of payload to be sent.
    void sendMsg( int port, uint8_t* buf, int len);
    
    /// \brief function to be called to allow LMIC OS to operated.
    /// Shall be called periodically to let LMIC operate.
    void process();

    /// \brief specify callback funftion, this function is called when downlink data has been receieved
    /// \param port
    /// \param buffer
    /// \param length of buffer
    void receiveHandler( void (*callback)(unsigned int, uint8_t*, unsigned int));
};

#endif // __LORA_H_
