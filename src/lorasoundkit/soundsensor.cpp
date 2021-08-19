/*--------------------------------------------------------------------
  This file is part of the TTN-Apeldoorn Sound Sensor.

  This code is free software:
  you can redistribute it and/or modify it under the terms of a Creative
  Commons Attribution-NonCommercial 4.0 International License
  (http://creativecommons.org/licenses/by-nc/4.0/) by
  TTN-Apeldoorn (https://www.thethingsnetwork.org/community/apeldoorn/) 

  The program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  --------------------------------------------------------------------*/

/*!
 * \file SoundSensor.cpp
 * \author Marcel Meek, Remko Welling (remko@rfsee.nl)
 */

#include "soundsensor.h"
#include "arduinoFFT.h"


const i2s_port_t I2S_PORT = I2S_NUM_0;

// The I2S config as per the example
const i2s_config_t i2s_config = {
  .mode                 = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),  // Receive, not transfer
  .sample_rate          = SAMPLE_FREQ,
  .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,                  // only 24 bits are used
  .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,                  // although the SEL config should be left, it seems to transmit on right, or not
  .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
  .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,                       // Interrupt level 1
  .dma_buf_count        = 8,                                          // number of buffers
  .dma_buf_len          = 1024,                                       //BLOCK_SIZE, samples per buffer
  .use_apll             = true
};

// pin config MEMS microphone
#if defined(ARDUINO_TTGO_LoRa32_V1)
// define IO pins TTGO LoRa32 V1 for I2S for MEMS microphone
const i2s_pin_config_t pin_config = {
  .bck_io_num   = 13,
  .ws_io_num    = 12,
  .data_out_num = -1,
  .data_in_num  = 35 //changed to 35, (21 is used by i2c)
};
#elif defined(ARDUINO_ESP32_DEV)
// define IO pins Sparkfun for I2S for MEMS microphone
const i2s_pin_config_t pin_config = {
  .bck_io_num   = 18,
  .ws_io_num    = 23,
  .data_out_num = -1,
  .data_in_num  = 19
};
#else
  #error Unsupported board selection.
#endif

SoundSensor::SoundSensor() {
  _fft = new arduinoFFT(_real, _imag, SAMPLES, SAMPLES);
  _runningDC = 0.0;
  _runningN = 0;
  offset( 0.0);
}

SoundSensor::~SoundSensor(){
  delete _fft;
}

void SoundSensor::begin(){
  
  // Configuring the I2S driver and pins.
  // This function must be called before any I2S driver read/write operations.
  _err = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (_err != ESP_OK) {
    printf("Failed installing I2S driver: %d\n", _err);
    while (true);
  }
  
  _err = i2s_set_pin(I2S_PORT, &pin_config);
  if (_err != ESP_OK) {
    printf("Failed setting pin: %d\n", _err);
    while (true);
  }
  printf("I2S driver installed.\n");
}

float* SoundSensor::readSamples(){
  // Read multiple samples at once and calculate the sound pressure
   
  size_t num_bytes_read;
  _err = i2s_read(
    I2S_PORT,
    (char *) _samples,
    BLOCK_SIZE * 4,        // 4 bytes per sample
    &num_bytes_read,
    portMAX_DELAY
  );    // no timeout
  
  //printf("bytes read %d\n", num_bytes_read);
  
  if(_err != ESP_OK){
    printf("%d err\n",_err);
  }

  integerToFloat(_samples, _real, _imag, SAMPLES);

  // apply HANN window, optimal for energy calculations
  _fft->Windowing(FFT_WIN_TYP_HANN, FFT_FORWARD);     // changed was FFT_WIN_TYP_FLT_TOP
 
  // do FFT processing
  _fft->Compute(FFT_FORWARD);

  // calculate energy in each bin
  calculateEnergy(_real, _imag, SAMPLES);

  // sum up energy in bin for each octave
  sumEnergy(_real, _energy);

  return _energy;
}

// convert WAV integers to float
// convert 24 High bits from I2S buffer to float and divide * 256 
// remove DC offset, necessary for some MEMS microphones 
/*void SoundSensor::integerToFloat(int32_t * samples, float *vReal, float *vImag, uint16_t size) {
  float sum = 0.0;
  for (uint16_t i = 0; i < size; i++) {
    int32_t val = (samples[i] >> 8);            // move 24 value bits on the correct place in a long
    sum += (float)val;
    samples[i] = (val - _offset ) << 8;         // DC component removed, and move back to original buffer
    vReal[i] = (float)val / (256.0 * FACTOR);   // adjustment
    vImag[i] = 0.0;
  }
  _offset = sum / size;   //dc component
  //printf("DC offset %d\n", offset);
}*/

void SoundSensor::integerToFloat(int32_t * samples, float *vReal, float *vImag, uint16_t size) {
  float sum = 0.0;
  // calculate offset
  for (uint16_t i = 0; i < size; i++) {
    int32_t val = (samples[i] >> 8);            // move 24 value bits on the correct place in a long
    vReal[i] = (float)val;
    sum += vReal[i];
  }
  float offs = sum / (float)size;   //dc component
  if( _runningN < 100)
    _runningN++;
  float newDC = _runningDC + (offs - _runningDC)/_runningN;
  _runningDC = newDC;

  for (uint16_t i = 0; i < size; i++) {
    vReal[i] = (vReal[i] - newDC) / (256.0 * FACTOR / _factor);   // 30.0 adjustment
    vImag[i] = 0.0;
  }
  //printf("DC offset %f\n", newDC);
}


// calculates energy from Re and Im parts and places it back in the Re part (Im part is zeroed)
void SoundSensor::calculateEnergy(float *vReal, float *vImag, uint16_t samples)
{
  for (uint16_t i = 0; i < samples; i++) {
    vReal[i] = sq(vReal[i]) + sq(vImag[i]);
    vImag[i] = 0.0;
  }
}

// convert dB offset to factor
void SoundSensor::offset( float dB) {
   _factor = pow(10, dB / 20.0);    // convert dB to factor 
}

// sums up energy in whole octave bins
void SoundSensor::sumEnergy(const float *samples, float *energies) {

  // skip the first two bins
  int bin_size = 2;
  int bin = bin_size;
  for (int octave = 0; octave < OCTAVES; octave++){
    float sum = 0.0;
    for (int i = 0; i < bin_size; i++){
      sum += samples[bin++];
    }
    energies[octave] = sum;
    bin_size *= 2;
    //printf("octaaf=%d, bin=%d, sum=%f\n", octave, bin-1, sum);
  }
}

/*
// generate test sinus
static void generateSineWave( int32_t* samples, float amplitude, float freq) {
  float c = round( freq / (SAMPLE_FREQ / (float)SAMPLES)) / SAMPLES;     // put a multipe of complete sinewaves in buffer
  for ( int i = 0; i < BLOCK_SIZE; i++) {
    int32_t temp = 256 * amplitude * sin((float)i * c * twoPi );        // sine wave
    samples[i] = temp & 0xFFFFFF00;  // convert to WAV integers
  }
}
*/
