/*--------------------------------------------------------------------
  LoRa Soundkit
  Measures environtmentalsound and send data to LoRa network

  Author Marcel Meek
  Date 12/7/2020
  changed 18-3-2024
  - Sources adapted for LilyGO TTGO T3 LoRa32 board (10dB better radio performance)
  - Sources adapted for VC PlatformIO (arduino IDE support not tested)
  - During a TTN connect phase, the I2S MEMS driver is stopped, because it gives Rx radio interference
  - The update of the OLED display is moved from the sound thread to the main thread.
  changed 15-8-2021
  - MCCI Catena LoRa stack
  - Worker loop changed (hang situation solved with TTN V3)
  - OLED display added
  - DEVEUI obtained from BoardID, (same SW for multiple sensors)
  - use the TWO processor cores of ESP (one core for audio and one core for LoRa)
  - DC offset MEMS compensated by moving average window
  - payload compressed from 27 to 19 bytes
  Changed 1/11/2023
  - Joining TTN problems solved, join was disturbed by I2S driver, it is stopped during join
  - Working loop improved, sleep is postponed after send, and a shorter sleep during joining phase 
  - TTN SF9 is the default and ADR is enabled
  --------------------------------------------------------------------*/

#include <Arduino.h>
#include "lora.h"
#include "soundsensor.h"
#include "measurement.h"
#include "config.h"
#include "oled.h"
static Oled oled;

// forward declarations
void Task0code( void * pvParameters );
void loracallback( unsigned int port, unsigned char* msg, unsigned int len);
void loraWorker( );
static void composeMessage( Measurement& la, Measurement& lc, Measurement& lz);

static int cycleTime = CYCLETIME;
static char deveui[40];

// Weighting lists
  static float aweighting[] = A_WEIGHTING;
  static float cweighting[] = C_WEIGHTING;
  static float zweighting[] = Z_WEIGHTING;

// measurement buffers, filled by core 0, read by core 1
  static Measurement aMeasurement( aweighting);
  static Measurement cMeasurement( cweighting);
  static Measurement zMeasurement( zweighting);
 
// Task 1 is the default ESP core 1, this one handles the LoRa TTN messages
// Task 0 is the added ESP core 0, this one handles the audio, (read MEMS, FFT process and compose message)
TaskHandle_t Task0;

// task semaphores
static bool audioRequest = false;
static bool audioReady = false;
static bool sound = false;

// payloadbuffer
unsigned char payload[80];
int payloadLength = 0;

// create soundsensor
static SoundSensor soundSensor;

 void setup() {
  Serial.begin(115200); 
  delay(100);
  // LoRa send LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite( LED_BUILTIN, LOW);   // off

// get chip id, to be used for DEVEUI LoRa
  uint64_t chipid = ESP.getEfuseMac();   //The chip ID is essentially its MAC address(length: 6 bytes).
  sprintf( deveui, "%08X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  printf("deveui=%s\n", deveui);

  oled.begin();
  oled.deveui = deveui; 
  oled.values( &aMeasurement, &cMeasurement, &zMeasurement);
  oled.status = "Starting";
  oled.update( );

  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    Task0code,   // Task function.
                    "Task0",     // name of task.
                    40000,       // Stack size of task
                    NULL,        // parameter of the task
                    1,           // priority of the task 1
                    &Task0,      // Task handle to keep track of created task
                    0);          // pin task to core 0                 


// do testread for 2 seconds
  sound = true;
  delay(2000);
  audioRequest = true;
  while( !audioReady) 
    delay(10);
  audioReady = false;
  sound = false;
  oled.update();
 
//initialize LoRa
  loraBegin( APPEUI, deveui, APPKEY);
  loraSetRxHandler( loracallback);    // set LoRa receive handler (downnlink)
  loraSetWorker( loraWorker);         // set Worker handler
  loraSleep(1);                      // start worker 
  Serial.println("end setup");
} 

// Handle sound measurements
// Note this is anoher theread running in another Core!!!!
void Task0code( void * pvParameters ){
  Serial.print("Task0 running on core ");
  Serial.println(xPortGetCoreID());
  soundSensor.begin();

  // main loop task 0
  while( true){

    if( sound ) { 
      if( !soundSensor.running())
        soundSensor.start();
      // read chunk form MEMS and perform FFT, and sum energy in octave bins
      float* energy = soundSensor.readSamples();

      // update
      aMeasurement.update( energy);
      cMeasurement.update( energy);
      zMeasurement.update( energy);

      // calculate audio result on request
      if( audioRequest) {
        audioRequest = false;
        aMeasurement.calculate();
        cMeasurement.calculate();
        zMeasurement.calculate();
        audioReady = true;    // signal worker task that audio result is ready
      }
    }
    else {
      if( soundSensor.running())
        soundSensor.stop();
      delay(100);  // do nothing
    }
  }
}

// LoRa receive handler (downnlink)
void loracallback( unsigned int port, unsigned char* msg, unsigned int len) {
  printf("lora download message received port=%d len=%d\n", port, len);

  // change cycle count in seconds with a remote TTN download command 
  // byte 0 is low byte 1 is high byte
  if( len >=2) {
    int value = msg[0] + 256 * msg[1];
    if( port == 20) {  // change cycle time (value must ne between 10 and 600 seconds)
      if( value >= 10 && value <= 600) {
         cycleTime = value;
         printf( "cycleTime changed to %d sec.\n" , value);
      }
    }
    if( port == 21) {  // change dB offset  (value/10 must be between -40dB and +40dB)
      if( value >= -400 && value <= 400) {
        float dB = value / 10.0;
        soundSensor.offset( dB); 
        Serial.print( "Mic offset changed to "); Serial.println( dB);
      }
    }
  }
}

// compose payload message
static void composeMessage( Measurement& la, Measurement& lc, Measurement& lz) {
  // find max value to compress values [0 .. max] in an unsigned byte from [0 .. 255]
  float max = ( la.max > lc.max) ? la.max : lc.max;
  max = ( lz.max > max) ? lz.max : max;

  float c = 255.0 / max;
  int i=0;
  payload[ i++] = round(max);   // save this constant as first byte in the message
  
  payload[ i++] = round(c * la.min);
  payload[ i++] = round(c * la.max);
  payload[ i++] = round(c * la.avg);

  payload[ i++] = round(c * lc.min);
  payload[ i++] = round(c * lc.max);
  payload[ i++] = round(c * lc.avg);
 
  payload[ i++] = round(c * lz.min);
  payload[ i++] = round(c * lz.max);
  payload[ i++] = round(c * lz.avg);

  for ( int j = 0; j < OCTAVES; j++) {
    payload[ i++] = round(c * lz.spectrum[j]);
  }

  payloadLength = i;
  if( payloadLength > 51)   // max TTN message length
    printf( "message to big length=%d\n", payloadLength);
}

// called from LoRa Task (task1), each cycle time
void loraWorker( ) {
  printf("Worker\n");

  if( loraConnected()) { 
    sound = true;
    oled.status = "TTN Connected";
    audioRequest = true;  // signal audiotask to compose an audio report
   
    while( !audioReady)  // wait for Task 0 to be ready
      loraLoop();    
    audioReady = false;
    digitalWrite( LED_BUILTIN, HIGH);
   
          // save values for oled display
    oled.update();
    // debug info, should be comment out
    //aMeasurement.print();
    //cMeasurement.print();
    //zMeasurement.print();
    composeMessage( aMeasurement, cMeasurement, zMeasurement);
    printf("send message len=%d core=%d\n", payloadLength, xPortGetCoreID());
    loraSend( 22, (unsigned char*)payload, payloadLength);  // use port 22
    // wait until lora request is ready within timeout
    //long start = millis();
    while ( !loraTxReady() /*&& millis() - start < 100000 */ )
      loraLoop(); 
    digitalWrite( LED_BUILTIN, LOW);
    loraSleep( cycleTime);
  }
  else {   // lora not connected so do a (re)join
    sound = false;
    digitalWrite( LED_BUILTIN, HIGH);
    printf("loraJoin\n");
    oled.status = "TTN Joining..";
    oled.update();
    loraJoin(); 
   // wait until lora request is ready within timeout
    //long start = millis();
    while ( !loraTxReady() /*&& millis() - start < 1000*/  )
      loraLoop(); 
    digitalWrite( LED_BUILTIN, LOW);
    if( loraConnected()) { 
      oled.status = "TTN Connected"; 
      sound = true;
      oled.update();
      loraSleep( cycleTime);
    }
    else {
      oled.status = "TTN Join Failed";
      oled.update();
      loraSleep( 10);  // sleep a short time to retry a join again
    }
  }
}

// main loop task 1 (esp default)
void loop() {
   loraLoop();
}
