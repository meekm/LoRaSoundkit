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
#include "measurement.h"

class Oled {
  public:
    Oled();
    ~Oled();
    void begin( char* deveui);
    void showValues( Measurement& la, Measurement& lc, Measurement& lz, bool ttnOk);

  private:
     Adafruit_SSD1306 *display;
}; 

#endif //__MEASUREMENT_H_
