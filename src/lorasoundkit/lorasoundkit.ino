/** 
 * ESP32 I2S Noise FFT with LoRa
 * 
 * This example calculates noise in octave bands, in a,c and z weighting
 * Using Arduino FFT  https://www.arduinolibraries.info/libraries/arduino-fft
 * Using LoRa LMIC https://github.com/matthijskooijman/arduino-lmic
 * example: https://bitbucket.org/edboel/edboel/src/master/noise/
 * 
 * Marcel Meek april 2020
 */

#include <Arduino.h>
#include <driver/i2s.h>
#ifdef WIFITEST
  #include <WiFi.h>
  #include <HTTPClient.h>
#endif
#include "arduinoFFT.h"
#include "lora.h"
#include "measurement.h"

// size of noise sample
#define SAMPLES 1024
#define SAMPLE_FREQ  22627  // this makes a bin bandwith of 22627 / 1024 = 22,01 Hz

// define IO pins Sparkfun for I2S for MEMS microphone
#define BCKL 18
#define LRCL 23
#define NOT_USED -1
#define DOUT 19

static LoRa lora;

const i2s_port_t I2S_PORT = I2S_NUM_0;
const int BLOCK_SIZE = SAMPLES;

#ifdef WIFITEST
// wifi used for testing raw audio output. 
const char* ssid = "yourssid";
const char* password = "yourpassword";
const char* host = "192.168.1.17:5000";   
#endif

#define CYCLECOUNT   60000

// FFT buffers
static float real[SAMPLES];
static float imag[SAMPLES]; 

static arduinoFFT fft(real, imag, SAMPLES, SAMPLES);

// Weighting lists
static float aweighting[] = A_WEIGHTING;
static float cweighting[] = C_WEIGHTING;
static float zweighting[] = Z_WEIGHTING;

// measurement buffers
static Measurement aMeasurement( aweighting);
static Measurement cMeasurement( cweighting);
static Measurement zMeasurement( zweighting);

static float energy[OCTAVES];
long milliCount = -1;

// generate test sinus
static void generateSineWave( int32_t* samples, float amplitude, float freq) {
    float c = round( freq / (SAMPLE_FREQ/(float)SAMPLES)) / SAMPLES;       // put a multipe of complete sinewaves in buffer
    for( int i=0; i< BLOCK_SIZE; i++) {
       int32_t temp = 256 * amplitude * sin((float)i * c * twoPi );        // sine wave
       samples[i] = temp & 0xFFFFFF00;  // convert to WAV integers
    }
} 

static int32_t offset = 0;
#define FACTOR 10.0       // to be cheked why this 10.0

// convert WAV integers to float
// convert 24 High bits from I2S buffer to float and divide * 256 
// remove DC offset, necessary for some MEMS microphones 
static void integerToFloat(int32_t * samples, float *vReal, float *vImag, uint16_t size) {
    float sum = 0.0;
    for (uint16_t i = 0; i < size; i++) {
        int32_t val = (samples[i] >> 8);           // move 24 value bits on the correct place in a long
        sum += (float)val;
        samples[i] = (val - offset ) << 8;          // DC component removed, and move back to original buffer
        vReal[i] = (float)val / (256.0 * FACTOR);   // adjustment
        vImag[i] = 0.0;
    }
    offset = sum / size;   //dc component
    //printf("DC offset %d\n", offset);
}

// calculates energy from Re and Im parts and places it back in the Re part (Im part is zeroed)
static void calculateEnergy(float *vReal, float *vImag, uint16_t samples)
{
    for (uint16_t i = 0; i < samples; i++) {
        vReal[i] = sq(vReal[i]) + sq(vImag[i]);
        vImag[i] = 0.0;
    }
}
// sums up energy in whole octave bins
static void sumEnergy(const float *samples, float *energies)
{
    // skip the first bin
    int bin_size = 1;
    int bin = bin_size;
    for (int octave = 0; octave < OCTAVES; octave++) {
        float sum = 0.0;
        for (int i = 0; i < bin_size; i++) {
            sum += samples[bin++];
        }
        energies[octave] = sum;
        bin_size *= 2;
        //printf("octaaf=%d, bin=%d, sum=%f\n", octave, bin-1, sum);
    }
}

void setup(void)
{
    Serial.begin(115200);
    printf("Configuring I2S...\n");
    pinMode(LED_BUILTIN, OUTPUT);       // lit if sending to LoRA
    digitalWrite( LED_BUILTIN, LOW);
    
    esp_err_t err;

    // The I2S config as per the example
    const i2s_config_t i2s_config = {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),      // Receive, not transfer
        .sample_rate = SAMPLE_FREQ,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,   // only 24 bits are used
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,    // although the SEL config should be left, it seems to transmit on right, or not
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,       // Interrupt level 1
        .dma_buf_count = 8,     // number of buffers
        .dma_buf_len = 1024,    //BLOCK_SIZE,           // samples per buffer
        .use_apll = true
    };

    // The pin config
    const i2s_pin_config_t pin_config = {
        .bck_io_num = BCKL, 
        .ws_io_num = LRCL,    
        .data_out_num = NOT_USED,
        .data_in_num = DOUT          
    };

    // Configuring the I2S driver and pins.
    // This function must be called before any I2S driver read/write operations.
    err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        printf("Failed installing driver: %d\n", err);
        while (true);
    }
    err = i2s_set_pin(I2S_PORT, &pin_config);
    if (err != ESP_OK) {
        printf("Failed setting pin: %d\n", err);
        while (true);
    }
    printf("I2S driver installed.\n");

// send LoRA Join message
    lora.sendMsg(0, NULL, 0);

 #ifdef WIFITEST
    // begin WIFI
    WiFi.begin(ssid, password); 
 
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        printf("Connecting to WiFi..\n");
    }
    printf("Connected to the WiFi network\n");
 #endif
}

 #ifdef WIFITEST
// TEST FUNCTIONALITY   (should be upgraded to a WAV streaming audio service)
// send raw MEMS buffer to TCP-IP, in order to check on a laptop if audio is clean and has no gaps.
WiFiClient client;
static void sendToTCP(char *buf, int size){

  if( !client.connected() ) {
  
    if (!client.connect( "192.168.1.17", 5000)) {   // my laptop
      printf("connection failed\n");
      return;
    }
  }
  client.write( buf, size);
}
#endif

// compose message, and send it to TTN
// convert shorts to 12 bit integers, to save 25% space in th TTN message
static void sendToTTN( Measurement& la, Measurement& lc, Measurement& lz) {
    unsigned char payload[80];
    int i= 0;  // nibble count, a nibble is 4 bits
    
    // convert floats to 12 bits integers
    for( int j=0; j<OCTAVES; j++) {
      i = add12bitsToBuf( payload, i, la.spectrum[j] * 10.0);
    }
    i = add12bitsToBuf( payload, i, la.peak * 10.0);
    i = add12bitsToBuf( payload, i, la.avg * 10.0);

    for( int j=0; j<OCTAVES; j++) {
      i = add12bitsToBuf( payload, i, lc.spectrum[j] * 10.0);
    }
    i = add12bitsToBuf( payload, i, lc.peak * 10.0);
    i = add12bitsToBuf( payload, i, lc.avg * 10.0);

    for( int j=0; j<OCTAVES; j++) {
      i = add12bitsToBuf( payload, i, lz.spectrum[j] * 10.0);
    }
    i = add12bitsToBuf( payload, i, lz.peak * 10.0);
    i = add12bitsToBuf( payload, i, lz.avg * 10.0);
    
    int len = i / 2 + 1;
    printf( "messagelength=%d\n", len);

    if( len > 50)    // max TTN message length
        printf( "message to big length=%d\n", len);
    lora.sendMsg( 10, payload, len );    // use port 10
}

// add 12 bits value to payloadbuffer
int add12bitsToBuf( unsigned char* buf, int nibbleCount, short val) {
    if( nibbleCount % 2 == 0) {
      buf[ nibbleCount / 2] = val >> 4;
      buf[ nibbleCount / 2 +1] = (val << 4) & 0xF0;
    }
    else {
      buf[ nibbleCount / 2] |= ((val >> 8) & 0x0F);
      buf[ nibbleCount / 2 +1] = val;
    }
    return nibbleCount + 3;
 }

// Arduino Main Loop
void loop(void)
{
    static int32_t samples[BLOCK_SIZE];

    // Read multiple samples at once and calculate the sound pressure
    size_t num_bytes_read;
    esp_err_t err = i2s_read(I2S_PORT,
                             (char *) samples,
                             BLOCK_SIZE *4,        // 4 bytes per sample
                             &num_bytes_read,
                             portMAX_DELAY);    // no timeout
    // printf("bytes read %d\n", num_bytes_read);

    if(err != ESP_OK)
        printf("%d err\n",err);
    
    // overwite teh MEMS buffer with a test sine wave
    //generateSineWave( samples, 265.27, 1000);       // 420426.32 = 94 dB, 265.27 = 30 dB

    // integer to float and remove DC component
    integerToFloat(samples, real, imag, SAMPLES);

    // printf("TEST sendToTCP\n"); 
    //sendToTCP((char*)samples, SAMPLES*4);   //check quality of raw sound via tcp stream on laptop

    // apply HAN window, optimal for energy calculations
    fft.Windowing(FFT_WIN_TYP_HANN, FFT_FORWARD);     // changed was FFT_WIN_TYP_FLT_TOP
   
    // do FFT processinge
    fft.Compute(FFT_FORWARD);

    // calculate energy in each bin
    calculateEnergy(real, imag, SAMPLES);

    // sum up energy in bin for each octave
    sumEnergy(real, energy);

    // update
    aMeasurement.update( energy);
    cMeasurement.update( energy);
    zMeasurement.update( energy);
  
    // calculate average and send 
    if( millis() - milliCount > CYCLECOUNT) {
        printf("\n");
        milliCount = millis();

        aMeasurement.calculate();
        cMeasurement.calculate();
        zMeasurement.calculate();

        // debug info, should be comment out
        aMeasurement.print(); 
        cMeasurement.print();
        zMeasurement.print();

        // send message
        sendToTTN( aMeasurement, cMeasurement, zMeasurement);
         
        // reset counters etc.
        aMeasurement.reset();
        cMeasurement.reset();
        zMeasurement.reset();
    }
    //printf(".");
    lora.loop();
 }
