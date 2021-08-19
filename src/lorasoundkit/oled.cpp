/*******************************************************************************
* file oled.h
* Oled wrapper, interface to display messages
* author Marcel Meek
*********************************************************************************/

#include "oled.h"

//OLED pins
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128  // OLED display width, in pixels
#define SCREEN_HEIGHT 64  // OLED display height, in pixels

Oled::Oled() {
  display = new Adafruit_SSD1306( SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
};

Oled::~Oled() {
  delete display;
}

void Oled::begin( char *deveui) {

 //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    printf("SSD1306 allocation failed\n");
    for (;;); // Don't proceed, loop forever
  }
   
  display->clearDisplay();
  //display->setRotation( 2);  // rotate 180 degrees
  display->setTextColor(WHITE);
  display->setTextSize(1);
  display->setCursor(0, 10);  display->printf("Soundkit Starting");
  display->setCursor(0, 30);  display->printf("DEVEUI:");
  display->setCursor(0, 40);  display->printf("%s", deveui);
  display->display();
}

void Oled::showValues( Measurement& la, Measurement& lc, Measurement& lz, bool ttnOk) {
  display->clearDisplay();
  //display->setRotation( 2);  // rotate 180 degrees
  display->setTextColor(WHITE);
  display->setTextSize(1);
  display->setCursor(0, 0);  display->printf( "      avg  min  max");
  display->setCursor(0, 10);  display->printf("dB(A) %.1f %.1f %.1f", la.avg, la.min, la.max), 
  display->setCursor(0, 20);  display->printf("dB(C) %.1f %.1f %.1f", lc.avg, lc.min, lc.max);
  display->setCursor(0, 30);  display->printf("dB(Z) %.1f %.1f %.1f", lz.avg, lz.min, lz.max);
  display->setCursor(0, 50);  display->printf("TTN %s", (ttnOk) ? "ok" : "fail");
  display->display();
}
