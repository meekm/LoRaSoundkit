/*******************************************************************************
* file oled.h
* Oled wrapper, interface to display messages
* author Marcel Meek
*********************************************************************************/

#include "oled.h"
#define VERSION "4.0"

//OLED pins
#define OLED_SDA 21    // v1=4, v2=21
#define OLED_SCL 22   //  v1=15, v2=22
//#define OLED_RST -1    // v1=16, v2=-1
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels


Oled::Oled() {
  display = new Adafruit_SSD1306( SCREEN_WIDTH, SCREEN_HEIGHT, &Wire); //OLED_RST);
};

Oled::~Oled() {
  delete display;
}

void Oled::begin() {
//initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    printf("SSD1306 allocation failed\n");
    for (;;); // Don't proceed, loop forever
  }
}

void Oled::values( Measurement* la, Measurement* lc, Measurement* lz) {
  _la = la;
  _lc = lc;
  _lz = lz;
}


void Oled::update( ) {
  //printf( "showValues status=%s\n", status);
  display->clearDisplay();
  //display->setRotation( 2);  // rotate 180 degrees
  display->setTextColor(WHITE);
  display->setTextSize(1); 
  if( _la != NULL && _la->avg > 0.0) {
    display->setCursor(0, 0);  display->printf( "      avg  min  max");
    display->setCursor(0, 10);  display->printf("dB(A) %.1f %.1f %.1f", _la->avg, _la->min, _la->max), 
    display->setCursor(0, 20);  display->printf("dB(C) %.1f %.1f %.1f", _lc->avg, _lc->min, _lc->max);
    display->setCursor(0, 30);  display->printf("dB(Z) %.1f %.1f %.1f", _lz->avg, _lz->min, _lz->max);
  }
  display->setCursor(0, 40);  display->print( deveui);
  display->setCursor(0, 50);  display->print( status);
  display->display();
}

