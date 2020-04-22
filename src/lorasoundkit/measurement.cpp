/** 
 * Measurement implementation file
 * Converts energy spectrum to a weighted DB level
 * Marcel Meek april 2020
 */
#include <Arduino.h>
#include "measurement.h"


Measurement::Measurement( float* weighting) {
    _weighting = weighting;
    for( int i=0; i<OCTAVES; i++)
      _weighting[i] = pow(10, _weighting[i] / 10.0);  // convert dB constants to level constatnts
    reset();
}

void Measurement::reset() {
    peak = avg = 0.0;
    _count = 0;
    for( int i=0; i< OCTAVES; i++) 
        spectrum[i] = 0.0;
}

void Measurement::update( float* energies ) {
    float sum = 0.0;
    for (int i = 0; i < OCTAVES; i++) {
        float v = energies[i] * _weighting[i];
        sum += v;
        spectrum[i] += decibel(v);
    }
    sum = decibel(sum);
    avg += sum;
    if( peak < sum)
       peak = sum;
    _count++;
}

void Measurement::calculate() {
    avg /= _count;
    for( int i=0; i< OCTAVES; i++) 
        spectrum[i] /= _count;
}

float Measurement::decibel(float v) {
    return 10.0 * log10(v); // log(10);     // for energy this should be 20.0 * log...  to be checked! 
}

void Measurement::print() {
    printf("count=%d peak=%.1f avg=%.1f =>", _count, peak, avg);
    for (int i = 0; i < OCTAVES; i++) 
        printf(" %.1f", spectrum[i]);
     printf("\n");
} 
