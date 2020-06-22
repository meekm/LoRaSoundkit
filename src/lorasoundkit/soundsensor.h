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
 
#ifndef __SOUND_SENSOR_H_
#define __SOUND_SENSOR_H_

#include <Arduino.h>
#include <driver/i2s.h>
#include "arduinoFFT.h"

#define FACTOR 30.0        /// \todo to be cheked why this 10.0 ?

// size of noise sample
#define SAMPLES 2048  //1024       ///< at sample frequency of 22,627 kHz with 2048 samples, duration is 90 ms.
#define SAMPLE_FREQ 22627          ///< this makes a bin bandwith of 22627 / 2048 = 11 Hz
#define OCTAVES 9

const int BLOCK_SIZE = SAMPLES;

class SoundSensor {
  public:
  
    /// \brief constructor
    SoundSensor();
    ~SoundSensor();

    /// \brief Initialize Sound sensor class and start.
    void begin();

    // Read multiple samples at once and calculate the sound pressure
    // returns energy in octave bands
    float* readSamples();

  private:
    arduinoFFT   *_fft;               ///< FFT class
    // FFT buffers
    float         _real[SAMPLES];
    float         _imag[SAMPLES];
    float         _energy[OCTAVES];
    int32_t       _samples[BLOCK_SIZE];
    int32_t       _offset;            ///< variable to compensate for DC-offset

    esp_err_t _err;                   ///< Variable to store errors from ESP32

    /// \brief Convert integer to float
    void integerToFloat(int32_t *samples, float *vReal, float *vImag, uint16_t size);
    
    // calculates energy from Re and Im parts and places it back in the Re part (Im part is zeroed)
    void calculateEnergy(float *vReal, float *vImag, uint16_t samples);
    
    // sums up energy in whole octave bins
    void sumEnergy(const float *samples, float *energies);
    // sums up energy in terts bins
    void sumEnergy3(const float *samples, float *energies);
}; 

#endif // __SOUND_SENSOR_H_
