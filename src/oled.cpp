/*******************************************************************************
* file oled.h
* Oled wrapper, interface to display messages
* author Marcel Meek
*********************************************************************************/

#include "oled.h"
#define VERSION "3.1"

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

void Oled::begin( Measurement* a, Measurement* c, Measurement* z) {
  _a = a;
  _c = c;
  _z = z;
 //reset OLED display via software
 /* pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);
*/
  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    printf("SSD1306 allocation failed\n");
    for (;;); // Don't proceed, loop forever
  }
}

void Oled::showStatus() {
  //printf( "showStatus status=%s\n", status);
  display->clearDisplay();
  //display->setRotation( 2);  // rotate 180 degrees
  display->setTextColor(WHITE);
  display->setTextSize(1);
  display->setCursor(0, 10);  display->printf("Soundkit %s", VERSION);
  display->setCursor(0, 20);  display->printf("DEVEUI:");
  display->setCursor(0, 30);  display->printf("%s", deveui);
  display->setCursor(0, 50);  display->printf("%s", status);
  display->display();
}

void Oled::showValues( ) {
  //printf( "showValues status=%s\n", status);
  display->clearDisplay();
  //display->setRotation( 2);  // rotate 180 degrees
  display->setTextColor(WHITE);
  display->setTextSize(1); 
  display->setCursor(0, 0);  display->printf( "      avg  min  max");
  display->setCursor(0, 10);  display->printf("dB(A) %.1f %.1f %.1f", _a->avg, _a->min, _a->max), 
  display->setCursor(0, 20);  display->printf("dB(C) %.1f %.1f %.1f", _c->avg, _c->min, _c->max);
  display->setCursor(0, 30);  display->printf("dB(Z) %.1f %.1f %.1f", _z->avg, _z->min, _z->max);
  display->setCursor(0, 40);  display->printf("dev %s", deveui);
  display->setCursor(0, 50);  display->printf("TTN %s", status);
  display->display();
}

