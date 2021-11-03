/*--------------------------------------------------------------------
  LoRa Soundkit
  Measures environtmentalsound and send data to LoRa network

  Author Marcel Meek
  Date 12/7/2020
  changed 15-8-2021
  - MCCI Catena LoRa stack
  - Worker loop changed (hang situation solved with TTN V3)
  - OLED display added
  - DEVEUI obtained from BoardID, (same SW for multiple sensors)
  - use the TWO processor cores of ESP (one core for audio and one core for LoRa)
  - DC offset MEMS compensated by moving average window
  - payload compressed from 27 to 19 bytes
  --------------------------------------------------------------------*/
#include <Arduino.h>
#include "lora.h"
#include "soundsensor.h"
#include "measurement.h"
#include "config.h"

#if defined(ARDUINO_TTGO_LoRa32_V1)
#include "oled.h"
static Oled oled;
#endif

static char deveui[32];
static int cycleTime = CYCLETIME;
static bool ttnOk = false;

// Task 1 is the default ESP core 1, this one handles the LoRa TTN messages
// Task 0 is the added ESP core 0, this one handles the audio, (read MEMS, FFT process and compose message)
TaskHandle_t Task0;

// task semaphores
bool audioRequest = false;
bool audioReady = false;

// payloadbuffer, filled by core 0, read by core 1
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
  sprintf(deveui, "%08X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  printf("deveui=%s\n", deveui);

  //create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    Task0code,   /* Task function. */
                    "Task0",     /* name of task. */
                    40000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task0,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */                  

//initialize LoRa
  loraBegin( APPEUI, deveui, APPKEY);
  loraSetRxHandler( loracallback);    // set LoRa receive handler (downnlink)
  loraSetWorker( loraWorker);         // set Worker handler
  //loraSetTxComplete( loraTxComplete); // set Tx complete handler
  loraSend( 0, 0, 0);  // do join
  loraSleep( 15);                

  Serial.println("end setup");
} 

//Task0code: handle sound measurements
void Task0code( void * pvParameters ){
  Serial.print("Task0 running on core ");
  Serial.println(xPortGetCoreID());

#if defined(ARDUINO_TTGO_LoRa32_V1)
  oled.begin(deveui);
#endif

  // Weighting lists
  static float aweighting[] = A_WEIGHTING;
  static float cweighting[] = C_WEIGHTING;
  static float zweighting[] = Z_WEIGHTING;

  // measurement buffers
  static Measurement aMeasurement( aweighting);
  static Measurement cMeasurement( cweighting);
  static Measurement zMeasurement( zweighting);

  soundSensor.offset( MIC_OFFSET);   // set microphone dependent correction in dB
  soundSensor.begin();

  // main loop task 0
  while( true){
    // read chunk form MEMS and perform FFT, and sum energy in octave bins
    float* energy = soundSensor.readSamples();

    // update
    aMeasurement.update( energy);
    cMeasurement.update( energy);
    zMeasurement.update( energy);
 
    // calculate average and compose message
    if( audioRequest) {
       audioRequest = false;

      aMeasurement.calculate();
      cMeasurement.calculate();
      zMeasurement.calculate();

      // debug info, should be comment out
      //aMeasurement.print();
      //cMeasurement.print();
      //zMeasurement.print();

#if defined(ARDUINO_TTGO_LoRa32_V1)
    oled.showValues( aMeasurement, cMeasurement, zMeasurement, ttnOk);
#endif

      composeMessage( aMeasurement, cMeasurement, zMeasurement);

      // reset counters etc.
      aMeasurement.reset();
      cMeasurement.reset();
      zMeasurement.reset();
      printf("end compose message core=%d\n", xPortGetCoreID());
      audioReady = true;    // signal LoRa worker task that audio report is ready
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
static bool composeMessage( Measurement& la, Measurement& lc, Measurement& lz) {
  
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
  //printf( "messagelength=%d\n", payloadLength);

  if ( payloadLength > 51)   // max TTN message length
    printf( "message to big length=%d\n", payloadLength);  
}

// called from LoRa Task, each cycle time
void loraWorker( ) {
  printf("Worker\n");
  digitalWrite( LED_BUILTIN, HIGH);
  audioRequest = true;  // signal audiotask to compose an audio report
 
  // wait for Task 0 to be ready
  while( !audioReady)
    loraLoop();    
  audioReady = false;
  
  ttnOk = loraConnected();
  if( ttnOk) {
    printf("send message len=%d core=%d\n", payloadLength, xPortGetCoreID());
    loraSend( 22, (unsigned char*)payload, payloadLength);  // use port 22
  }
  else
    printf("skip send, lora not connected\n");
  digitalWrite( LED_BUILTIN, LOW);

  loraSleep( cycleTime);
}

// main loop task 1 (esp default)
void loop() {
   loraLoop();
}
