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

void Oled::begin() {

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
  display->setTextSize(2);
  display->setCursor(0, 0);  display->printf("Soundkit");
  display->setCursor(0, 17);  display->printf("Starting");
  display->display();
}

void Oled::showValues( float la, float lc, float lz, bool ttnOk) {
  display->clearDisplay();
  //display->setRotation( 2);  // rotate 180 degrees
  display->setTextColor(WHITE);
  display->setTextSize(2);
  display->setCursor(0, 0);  display->printf("dB(A) %.1f", la);
  display->setCursor(0, 17);  display->printf("dB(C) %.1f", lc);
  display->setCursor(0, 34);  display->printf("dB(Z) %.1f", lz);
  display->setCursor(0, 51);  display->printf("TTN %s", (ttnOk) ? "ok" : "fail");
  display->display();
}
