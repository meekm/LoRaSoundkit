/*******************************************************************************
* file oled.h
* Oled wrapper, interface to display messages
* author Marcel Meek
*********************************************************************************/

#ifndef __OLED_H_
#define __OLED_H_

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SSD1306.h>
#include "measurement.h"

class Oled {
  public:
    Oled();
    ~Oled();
    void begin(); 
    void values( Measurement* la, Measurement* lc, Measurement* lz);
    void update();

    const char* status;
    const char* deveui;

  private:
    Measurement *_la, *_lc, *_lz;
    Adafruit_SSD1306 *display;
}; 

#endif //__MEASUREMENT_H_
