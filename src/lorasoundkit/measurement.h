/** 
 * Measurement Header file
 * Converts energy spectrum to a weighted DB level
 * Marcel Meek april 2020
 */

#define OCTAVES 9

// A-weighting and C-weighting curve from 31.5 Hz ... 8kHz in steps of whole octaves
// spectrum           31.5Hz 63Hz  125Hz 250Hz  500Hz 1kHz 2kHz 4kHz  8kHz
#define A_WEIGHTING { -39.4, -26.2, -16.1, -8.6, -3.2, 0.0, 1.2, 1.0, -1.1 };
#define C_WEIGHTING {  -3.0,  -0.8,  -0.2,  0.0,  0.0, 0.0, 0.2, 0.3, -3.0 };
#define Z_WEIGHTING {   0.0,  -0.0,  -0.0,  0.0,  0.0, 0.0, 0.0, 0.0,  0.0 };


class Measurement {
  public:
    Measurement( float* weighting);
    void reset();
    void update( float* energies);     
    void calculate();
    float decibel(float v);
    void print();

    float spectrum[OCTAVES]; 
    float peak;
    float avg;

  private:
    float* _weighting;
    int _count;
}; 
