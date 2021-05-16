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
   \file measurement.cpp
   \author Marcel Meek
   \date See revision table in header file
   \version see revision table in header file

   # ToDo
   \todo RW Add documentation on hardware connections
*/
#include <float.h>
#include <Arduino.h>
#include "measurement.h"

Measurement::Measurement( float* weighting) {
  _weighting = weighting;
  for ( int i = 0; i < OCTAVES; i++)
    _weighting[i] = pow(10, _weighting[i] / 10.0);  // convert dB constants to energy level constants
  reset();
}

void Measurement::reset() {
  avg = 0.0;
  _n = 0;
  min = FLT_MAX;
  max = FLT_MIN;

  for ( int i = 0; i < OCTAVES; i++)
    spectrum[i] = 0.0;
}

void Measurement::update( float* energies ) {
  _n++;
  float sum = 0.0;                             // sum in energy for this measurement
  for (int i = 0; i < OCTAVES; i++) {
    float v = energies[i] * _weighting[i];
    spectrum[i] += v;                          // sum energy per band for all measurements
    sum += v;
  }
  avg += sum;

  if ( max < sum) max = sum;
  if ( min > sum) min = sum;
}

void Measurement::calculate() {
  avg = decibel( avg / (float)_n);            // calculate average and convert to dB
  min = decibel( min);                       // convert to dB
  max = decibel( max);                       // convert to dB

  // calculate average for each band and convert to dB
  for ( int i = 0; i < OCTAVES; i++) {
    float val = spectrum[i] / (float)_n;      // energy average
    spectrum[i] = decibel( val);              // convert to dB
  }
}

float Measurement::decibel(float v) {
  return 10.0 * log10(v);                    // for energy this should be 20.0 * log...  to be checked!
}

void Measurement::print() {
  printf("count=%d min=%.1f max=%.1f avg=%.1f  =>", _n, min, max, avg);
  for (int i = 0; i < OCTAVES; i++)
    printf(" %.1f", spectrum[i]);
  printf("\n");
}
