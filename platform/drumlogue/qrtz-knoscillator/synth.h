#pragma once
/*
 *  File: synth.h
 *
 *  Dummy Synth Class.
 *
 *
 *  2021-2022 (c) Korg
 *
 */

#include <atomic>
#include <cstddef>
#include <cstdint>

#include <arm_neon.h>

#include "runtime.h"
#include "attributes.h"
#include "noise.h"
#include "notes.h"

#include "KnotOscillator.h"
#include "CartesianFloat.h"
#include "CartesianTransform.h"
#include "Frequency.h"
#include "SmoothValue.h"
#include "SmoothValue.cpp"
#include "SineOscillator.h"
#include "TriangleOscillator.h"
#include "RampOscillator.h"
#include "SquareWaveOscillator.h"
#include "NoiseOscillator.h"
#include "SmoothNoiseOscillator.h"
#include "AdsrEnvelope.h"

enum class Param : uint8_t
{
  Note,
  KnotP,
  KnotQ,
  KnotS,
  Morph,
  FmIndex,
  FmRatio,
  Noise,
  RotateX,
  RotateY,
  RotateZ,
  Empty1,
  EGAttack,
  EGDecay,
  EGToMorph,
  EGToIndex,
  AmpAttack,
  AmpDecay,
  AmpSustain,
  AmpRelease,
  LFOType,
  LFOFreq,
  LFOToPitch,
  LFOToIndex,

  Count
};

static const char* FM_RATIO_STR[] = {
  "1/4", "1/2", "3/4", "1x", "2x", "3x", "4x",
  "5x", "6x", "7x", "8x", "9x", "10x", "11x",
  "12x", "13x", "14x", "15x", "16x"
};

static const float FM_RATIO_VAL[] = {
  0.25f, 0.5f, 0.75f, 1.0f, 2.0f, 3.0f, 4.0f, 
  5.0f, 6.0f, 7.0f, 9.0f, 10.f, 11.f, 
  12.f, 13.f, 14.f, 15.f, 16.f
};

static const char* LFO_TYPE_STR[] = {
  "SINE", "TRI", "SAW", "SQR", "S&H", "RANDOM"
};

static const Noise2D<128> noise;

class Synth {
/*===========================================================================*/
/* Private Member Variables. */
/*===========================================================================*/
  static constexpr float TWO_PI = M_PI * 2;
  static constexpr float STEP_RATE = TWO_PI / 48000;
  static constexpr float PCT = 0.01f;
  static constexpr float EG_SEC_MIN = 0.001f;
  static constexpr float EG_SEC_MAX = 3.0f;
  static constexpr float ADSR_SEC_MIN = 0.001f;
  static constexpr float ADSR_SEC_MAX = 5.0f;
  static constexpr float LFO_FREQ_MIN = 0.0625f;
  static constexpr float LFO_FREQ_MAX = 20.0f;

  KnotOscillator knosc;
  SineOscillator kpm;
  Rotation3D rotator;
  LinearAdsrEnvelope adsrMod;
  ExponentialAdsrEnvelope adsrAmp;

  // our lfo types
  SineOscillator lfoSin;
  TriangleOscillator lfoTri;
  RampOscillator lfoSaw;
  SquareWaveOscillator lfoSqr;
  NoiseOscillator lfoStep;
  SmoothNoiseOscillator lfoSmooth;
  // this points to one of the lfo types based on the LFO TYPE param
  Oscillator* lfoOsc;

  int32_t params[static_cast<uint8_t>(Param::Count)];
  Notes notes;
  float rotateX;
  float rotateY;
  float rotateZ;
  SmoothFloat morph;
  SmoothFloat fmIndex;
  float phaseS;
  float freq;
  float vol;

 public:
  /*===========================================================================*/
  /* Public Data Structures/Types. */
  /*===========================================================================*/

  /*===========================================================================*/
  /* Lifecycle Methods. */
  /*===========================================================================*/

  Synth(void) : knosc(48000), kpm(48000), adsrMod(48000), adsrAmp(48000)
    , lfoSin(48000), lfoTri(48000), lfoSaw(48000), lfoSqr(48000)
    , lfoStep(48000), lfoSmooth(48000), lfoOsc(&lfoSin)
    , rotateX(0), rotateY(0), rotateZ(0)
    , morph(), fmIndex(), freq(0), vol(0)
  {
    memset(params, 0, sizeof(params));
    adsrMod.setSustain(0);
    adsrMod.setRelease(0);
  }

  ~Synth(void) 
  {
  }

  inline int8_t Init(const unit_runtime_desc_t * desc) {
    // Check compatibility of samplerate with unit, for drumlogue should be 48000
    if (desc->samplerate != 48000)
      return k_unit_err_samplerate;

    // Check compatibility of frame geometry
    if (desc->output_channels != 2)  // should be stereo output
      return k_unit_err_geometry;

    return k_unit_err_none;
  }

  inline void Teardown() {
    // Note: cleanup and release resources if any
  }

  inline void Reset() 
  {
    rotateX = 0;
    rotateY = 0;
    rotateZ = 0;
    vol = 0;
  }

  inline void Resume() {
    // Note: Synth will resume and exit suspend state. Usually means the synth
    // was selected and the render callback will be called again
  }

  inline void Suspend() {
    // Note: Synth will enter suspend state. Usually means another synth was
    // selected and thus the render callback will not be called
  }

  /*===========================================================================*/
  /* Other Public Methods. */
  /*===========================================================================*/

  fast_inline void Render(float * out, size_t frames) 
  {
    float * __restrict out_p = out;
    const float * out_e = out_p + (frames << 1);  // assuming stereo output

    int knotP = getParameterValue(Param::KnotP);
    int knotQ = getParameterValue(Param::KnotQ);

    adsrMod.setAttack(lerp(EG_SEC_MIN, EG_SEC_MAX, getParameterValue(Param::EGAttack) * PCT));
    adsrMod.setDecay(lerp(EG_SEC_MIN, EG_SEC_MAX, getParameterValue(Param::EGDecay) * PCT));

    adsrAmp.setAttack(lerp(ADSR_SEC_MIN, ADSR_SEC_MAX, getParameterValue(Param::AmpAttack) * PCT));
    adsrAmp.setDecay(lerp(ADSR_SEC_MIN, ADSR_SEC_MAX, getParameterValue(Param::AmpDecay) * PCT));
    adsrAmp.setSustain(getParameterValue(Param::AmpSustain) * PCT);
    adsrAmp.setRelease(lerp(ADSR_SEC_MIN, ADSR_SEC_MAX, getParameterValue(Param::AmpRelease) * PCT));

    lfoOsc->setFrequency(lerp(LFO_FREQ_MIN, LFO_FREQ_MAX, getParameterValue(Param::LFOFreq) * PCT));

    morph = getParameterValue(Param::Morph)*PCT;
    fmIndex = TWO_PI * (getParameterValue(Param::FmIndex)*PCT);

    //kpm.setFrequency(freq * FM_RATIO_VAL[getParameterValue(Param::FmRatio)]);

    //knosc.setFrequency(freq);
    knosc.setPQ(knotP, knotQ);

    // #TODO: use zoom parameter?
    const float zoom = 6.0f;
    const float rotateBaseFreq = 1.0f / 16.0f;
    const float rotateStep = rotateBaseFreq * STEP_RATE;
    const float rfx = rotateStep * (getParameterValue(Param::RotateX)*PCT) * 16;
    const float rfy = rotateStep * (getParameterValue(Param::RotateY)*PCT) * 16;
    const float rfz = rotateStep * (getParameterValue(Param::RotateZ)*PCT) * 16;
    const float squigVol = getParameterValue(Param::KnotS) * PCT * 0.25f;
    const float squigStep = freq * STEP_RATE * 4 * (knotP + knotQ);
    const float noiseVol = getParameterValue(Param::Noise) * PCT * 0.5f;
    const float egToMorph = getParameterValue(Param::EGToMorph) * PCT;
    const float egToIndex = TWO_PI * getParameterValue(Param::EGToIndex) * PCT;
    const float lfoToPitch = getParameterValue(Param::LFOToPitch) * PCT;
    const float lfoToIndex = TWO_PI * getParameterValue(Param::LFOToIndex) * PCT;
    const float fmRatio = FM_RATIO_VAL[getParameterValue(Param::FmRatio)];
    for (; out_p != out_e; out_p += 2) 
    {
      const float mod = adsrMod.generate();
      const float lfo = lfoOsc->generate();
      const float frq = freq * lfoToFreqMult(lfo * lfoToPitch);
      kpm.setFrequency(frq * fmRatio);
      knosc.setFrequency(frq);
      knosc.setMorph(clamp(morph + mod*egToMorph, 0, 1));
      const float fm = kpm.generate() * clamp(fmIndex + egToIndex*mod + lfoToIndex*lfo, 0, TWO_PI);

      // #TODO: should take advantage of NEON ArmV7 instructions?
      //vst1_f32(out_p, vdup_n_f32(0.f));
      CartesianFloat coord = knosc.generate<false>(fm, 0, 0);

      rotator.setEuler(rotateX, rotateY, rotateZ);
      coord = rotator.process(coord);

      const float st = phaseS + fm;
      const float nz = noiseVol * noise.sample(coord.x, coord.y);
      coord.x += cosf(st) * squigVol + coord.x * nz;
      coord.y += sinf(st) * squigVol + coord.y * nz;
      coord.z += coord.z * nz;
      
      float projection = (1.0f / (coord.z + zoom)) * vol * adsrAmp.generate();
      out_p[0] = coord.x * projection;
      out_p[1] = coord.y * projection;

      phaseS = stepPhase(phaseS, squigStep);
      rotateX = stepPhase(rotateX, rfx);
      rotateY = stepPhase(rotateY, rfy);
      rotateZ = stepPhase(rotateZ, rfz);
    }
  }

  inline void setParameter(uint8_t index, int32_t value) 
  {
    switch (Param(index)) 
    {
      case Param::Note:
        if (notes.size() > 0 && notes.last() == Notes::GATE)
        {
          freq = Frequency::ofMidiNote(value).asHz();
        }
        break;

      case Param::LFOType:
      {
        Oscillator* selected = lfoOsc;
        switch (value)
        {
          case 0: selected = &lfoSin; break;
          case 1: selected = &lfoTri; break;
          case 2: selected = &lfoSaw; break;
          case 3: selected = &lfoSqr; break;
          case 4: selected = &lfoStep; break;
          case 5: selected = &lfoSmooth; break;
        }
        if (lfoOsc != selected)
        {
          selected->setFrequency(lfoOsc->getFrequency());
          selected->setPhase(lfoOsc->getPhase());
          lfoOsc = selected;
        }
      }
      break;

      // prevents warnings about missing cases
      default: break;
    }

    if (index < static_cast<uint8_t>(Param::Count))
    {
      params[index] = value;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const
  {
    return index < static_cast<uint8_t>(Param::Count) ? params[index] : 0;
  }

  inline int32_t getParameterValue(Param index) const
  {
    return getParameterValue(static_cast<uint8_t>(index));
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
    switch (Param(index)) 
    {
      case Param::FmRatio: return FM_RATIO_STR[value];
      case Param::LFOType: return LFO_TYPE_STR[value];
      default:
        break;
    }
    return nullptr;
  }

  inline const uint8_t * getParameterBmpValue(uint8_t index,
                                              int32_t value) const {
    (void)value;
    switch (index) {
      // Note: Bitmap memory must be accessible even after function returned.
      //       It can be assumed that caller will have copied or used the bitmap
      //       before the next call to getParameterBmpValue
      // Note: Not yet implemented upstream
      default:
        break;
    }
    return nullptr;
  }

  inline void NoteOn(uint8_t note, uint8_t velocity) 
  {
    adsrMod.trigger(true, 0);
    if (notes.size() == 0)
    {
      adsrAmp.gate(true);
    }
    notes.noteOn(note);
    freq = Frequency::ofMidiNote(note).asHz();
    vol = (float)velocity / 127.0f;
  }

  inline void NoteOff(uint8_t note) 
  { 
    notes.noteOff(note);
    if (notes.size() == 0)
    {
      adsrAmp.gate(false);
    }
    else
    {
      uint8_t note = notes.last();
      if (note == Notes::GATE)
      {
        freq = Frequency::ofMidiNote(getParameterValue(Param::Note)).asHz();
      }
      else if (note <= 127)
      {
        freq = Frequency::ofMidiNote(note).asHz();
      }
    }
  }

  inline void GateOn(uint8_t velocity) 
  {
    adsrMod.trigger(true, 0);
    if (notes.size() == 0)
    {
      adsrAmp.gate(true);
    }
    notes.gateOn();
    freq = Frequency::ofMidiNote(getParameterValue(Param::Note)).asHz();
    vol = (float)velocity / 127.0f;
  }

  inline void GateOff() 
  {
    notes.gateOff();
    if (notes.size() == 0)
    {
      adsrAmp.gate(false);
    }
    else
    {
      freq = Frequency::ofMidiNote(notes.last()).asHz();
    }
  }

  inline void AllNoteOff() 
  {
    adsrAmp.gate(false);
  }

  inline void PitchBend(uint16_t bend) { (void)bend; }

  inline void ChannelPressure(uint8_t pressure) { (void)pressure; }

  inline void Aftertouch(uint8_t note, uint8_t aftertouch) {
    (void)note;
    (void)aftertouch;
  }

  inline void LoadPreset(uint8_t idx) { (void)idx; }

  inline uint8_t getPresetIndex() const { return 0; }

  /*===========================================================================*/
  /* Static Members. */
  /*===========================================================================*/

  static inline const char * getPresetName(uint8_t idx) {
    (void)idx;
    // Note: String memory must be accessible even after function returned.
    //       It can be assumed that caller will have copied or used the string
    //       before the next call to getPresetName
    return nullptr;
  }

private:
  static inline float lfoToFreqMult(float lfo)
  {
    return lfo < 0 ? lerp(1.0f, 0.5f, -1 * lfo) : lerp(1.0f, 2.0f, lfo);
  }

  static inline float lerp(const float from, const float to, const float t)
  {
    return from + (to - from) * t;
  }

  static inline float stepPhase(const float phase, const float step)
  {
    return phase > TWO_PI ? phase - TWO_PI + step : phase + step;
  }
};
