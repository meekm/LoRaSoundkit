/**
   TTGO / Sparkfun I2S Noise FFT with LoRa

   This example calculates noise in octave bands, in a,c and z weighting
   Using Arduino FFT  https://www.arduinolibraries.info/libraries/arduino-fft
   Using LoRa LMIC https://github.com/matthijskooijman/arduino-lmic
   example: https://bitbucket.org/edboel/edboel/src/master/noise/

   Marcel Meek, May 2020
*/

#include <Arduino.h>
#include "config.h"
#include "lora.h"
#include "soundsensor.h"
#include "measurement.h"


#if defined(ARDUINO_TTGO_LoRa32_V1)
#include "oled.h"
static Oled oled;
#endif

// create soundsensor
static SoundSensor soundSensor;

// Weighting lists
static float aweighting[] = A_WEIGHTING;
static float cweighting[] = C_WEIGHTING;
static float zweighting[] = Z_WEIGHTING;

// measurement buffers
static Measurement aMeasurement( aweighting);
static Measurement cMeasurement( cweighting);
static Measurement zMeasurement( zweighting);

long milliCount = -1;
long cycleCount = CYCLECOUNT;
bool ttnOk = false;

// LoRa receive handler (downnlink)
void loracallback( unsigned int port, unsigned char* msg, unsigned int len) {
  printf("lora download message received port=%d len=%d\n", port, len);

  // change cycle count in seconds with a remote TTN download command 
  // port is 20, byte 0 is low byte 1 is high byte
  if( port == 20 && len >=2) {
    int value = msg[0] + 256 * msg[1];
    if( value >= 10 && value <= 600) {
       cycleCount = 1000 * value;
       printf( "cycleCount changed to %d sec.\n" , value);
    }
  }
}

// Arduino set up
void setup(void) {

  Serial.begin(115200);
  delay(100);
  printf("Setup..\n");
  pinMode(LED_BUILTIN, OUTPUT);       // lit if sending data
  digitalWrite( LED_BUILTIN, HIGH);

#if defined(ARDUINO_TTGO_LoRa32_V1)
  oled.begin();
#endif
  soundSensor.begin();
  loraBegin();
  loraSetRxHandler( loracallback);        // set LoRa receive handler (downnlink
  loraSend( 0, NULL, 0);                  // send LoRA Join message
  digitalWrite( LED_BUILTIN, LOW);

  printf("End setup\n");
}

// add 12 bits value to payloadbuffer
int add12bitsToBuf( unsigned char* buf, int nibbleCount, short val) {
  if ( nibbleCount % 2 == 0) {
    buf[ nibbleCount / 2] = val >> 4;
    buf[ nibbleCount / 2 + 1] = (val << 4) & 0xF0;
  }
  else {
    buf[ nibbleCount / 2] |= ((val >> 8) & 0x0F);
    buf[ nibbleCount / 2 + 1] = val;
  }
  return nibbleCount + 3;
}

// compose message, and send it to TTN
// convert shorts to 12 bit integers, to save 25% space in th TTN message
static void sendToTTN( Measurement& la, Measurement& lc, Measurement& lz) {
  digitalWrite( LED_BUILTIN, HIGH);
  unsigned char payload[80];
  int i = 0; // nibble count, a nibble is 4 bits

  // convert floats to 12 bits integers

  i = add12bitsToBuf( payload, i, la.min * 10.0);
  i = add12bitsToBuf( payload, i, la.max * 10.0);
  i = add12bitsToBuf( payload, i, la.avg * 10.0);
  
  i = add12bitsToBuf( payload, i, lc.min * 10.0);
  i = add12bitsToBuf( payload, i, lc.max * 10.0);
  i = add12bitsToBuf( payload, i, lc.avg * 10.0);

  i = add12bitsToBuf( payload, i, lz.min * 10.0);
  i = add12bitsToBuf( payload, i, lz.max * 10.0);
  i = add12bitsToBuf( payload, i, lz.avg * 10.0);
  
  // send only LZ spectrum to Lora, LA and LC is generated at the TTN server side from LZ
  for ( int j = 0; j < OCTAVES; j++) {
    i = add12bitsToBuf( payload, i, lz.spectrum[j] * 10.0);
  }
 
  int len = i / 2 + (i % 2);
  printf( "messagelength=%d\n", len);

  if ( len > 51)   // max TTN message length
    printf( "message to big length=%d\n", len);
  //lora.sendMsg( 21, payload, len );    // use port 21, protocol V2
  ttnOk = loraSend( 21, payload, len );
  digitalWrite( LED_BUILTIN, LOW);
}

// Arduino Main Loop
void loop(void) {

  // read chunk form MEMS and perform FFT, and sum energy in octave bins
  float* energy = soundSensor.readSamples();

  // update
  aMeasurement.update( energy);
  cMeasurement.update( energy);
  zMeasurement.update( energy);

  // calculate average and send
  if ( millis() - milliCount > cycleCount) {
    //printf("\n");
    milliCount = millis();

    aMeasurement.calculate();
    cMeasurement.calculate();
    zMeasurement.calculate();

    // debug info, should be comment out
    //aMeasurement.print();
    //cMeasurement.print();
    //zMeasurement.print();

    sendToTTN( aMeasurement, cMeasurement, zMeasurement);

#if defined(ARDUINO_TTGO_LoRa32_V1)
   oled.showValues( aMeasurement.avg, cMeasurement.avg, zMeasurement.avg, ttnOk);
#endif

    // reset counters etc.
    aMeasurement.reset();
    cMeasurement.reset();
    zMeasurement.reset();
  }
}
