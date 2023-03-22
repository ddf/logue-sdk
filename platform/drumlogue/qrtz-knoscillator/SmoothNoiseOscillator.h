#ifndef SMOOTH_NOISE_OSCILLATOR_HPP
#define SMOOTH_NOISE_OSCILLATOR_HPP

#include "Oscillator.h"

/**
 * The SmoothNoiseOscillator generates random values in the range [-1, 1] at
 * a given frequency with smoothing between them.
 * It behaves like a white noise generator going into a sample and hold.
 * Based on Daisy SmoothRandomGenerated, which was ported from 
 * pichenettes/eurorack/plaits/dsp/noise/smooth_random_generator.h
 * Original code written by Emilie Gillet in 2016.
 */
// #TODO move this to either OwlPatches or OwlProgram (probably the latter and make a pull request)
class SmoothNoiseOscillator : public OscillatorTemplate<SmoothNoiseOscillator> {
protected:
  float from = 0;
  float interval = 0;
  float sample = 0;
public:
  static constexpr float begin_phase = 0;
  static constexpr float end_phase = 1;
  SmoothNoiseOscillator() {}
  SmoothNoiseOscillator(float sr) {
    setSampleRate(sr);
  }
  float getSample() {
    return sample;
  }
  float generate() {
    phase += incr;
    if (phase >= 1) {
      from += interval;
      interval = randf() * 2 - 1 - from;
      phase -= 1;
    }
    float t = phase * phase * (3.0f - 2.0f * phase);
    sample = from + interval * t;
    return sample;
  }
  using OscillatorTemplate<SmoothNoiseOscillator>::generate;
  void reset() {
    from += interval;
    interval = randf() * 2 - 1 - from;
    phase = 0;
  }
  using SignalGenerator::generate;
};

#endif /* SMOOTH_NOISE_OSCILLATOR_HPP */

