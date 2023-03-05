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

#include "KnotOscillator.h"
#include "CartesianFloat.h"
#include "CartesianTransform.h"
#include "Frequency.h"
#include "SmoothValue.h"
#include "SmoothValue.cpp"
#include "SineOscillator.h"

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

class Synth {
/*===========================================================================*/
/* Private Member Variables. */
/*===========================================================================*/
  static constexpr float TWO_PI = M_PI * 2;
  static constexpr float STEP_RATE = TWO_PI / 48000;

  KnotOscillator knosc;
  SineOscillator kpm;
  Rotation3D rotator;
  int   noteParam; // parameter
  int   knotP;
  int   knotQ;
  int   knotS;
  int   morphParam;
  int   fmIndexParam;
  int   fmRatioParam;
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

  Synth(void) : knosc(48000), kpm(48000)
    , noteParam(0), knotP(0), knotQ(0), knotS(0), morphParam(0)
    , fmIndexParam(0), fmRatioParam(0)
    , rotateX(0), rotateY(0), rotateZ(0)
    , morph(), fmIndex(), freq(0), vol(0)
  {
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

    morph = (float)morphParam / 100.0f;
    fmIndex = TWO_PI * ((float)fmIndexParam / 100.0f);

    kpm.setFrequency(freq * FM_RATIO_VAL[fmRatioParam]);

    knosc.setFrequency(freq);
    knosc.setPQ(knotP, knotQ);
    knosc.setMorph(morph);

    // #TODO: use zoom parameter?
    const float zoom = 6.0f;
    const float rotateBaseFreq = 1.0f / 16.0f;
    const float rotateStep = rotateBaseFreq * STEP_RATE;
    const float squigVol = (float)knotS * 0.25f / 100.0f;
    const float squigStep = freq * STEP_RATE * 4 * (knotP + knotQ);
    for (; out_p != out_e; out_p += 2) 
    {
      const float fm = kpm.generate() * fmIndex;

      // #TODO: should take advantage of NEON ArmV7 instructions?
      //vst1_f32(out_p, vdup_n_f32(0.f));
      CartesianFloat coord = knosc.generate<false>(fm, 0, 0);

      rotator.setEuler(rotateX, rotateY, rotateZ);
      coord = rotator.process(coord);

      const float st = phaseS + fm;
      coord.x += cosf(st) * squigVol;
      coord.y += cosf(st) * squigVol;
      
      float projection = (1.0f / (coord.z + zoom)) * vol;
      out_p[0] = coord.x * projection;
      out_p[1] = coord.y * projection;

      phaseS = stepPhase(phaseS, squigStep);
      rotateX = stepPhase(rotateX, rotateStep);
      rotateY = stepPhase(rotateY, rotateStep);
      rotateZ = stepPhase(rotateZ, rotateStep);
    }
  }

  inline void setParameter(uint8_t index, int32_t value) 
  {
    switch (Param(index)) 
    {
      case Param::Note: noteParam = value; freq = Frequency::ofMidiNote(noteParam).asHz(); break;
      case Param::KnotP: knotP = value; break;
      case Param::KnotQ: knotQ = value; break;
      case Param::KnotS: knotS = value; break;
      case Param::Morph: morphParam = value; break;
      case Param::FmIndex: fmIndexParam = value; break;
      case Param::FmRatio: fmRatioParam = value; break;
      default:
        break;
    }
  }

  inline int32_t getParameterValue(uint8_t index) const {
    switch (Param(index))
    {
      case Param::Note: return noteParam;
      case Param::KnotP: return knotP;
      case Param::KnotQ: return knotQ;
      case Param::KnotS: return knotS;
      case Param::Morph: return morphParam;
      case Param::FmIndex: return fmIndexParam;
      case Param::FmRatio: return fmRatioParam;

      default:
        break;
    }
    return 0;
  }

  inline const char * getParameterStrValue(uint8_t index, int32_t value) const {
    switch (Param(index)) 
    {
      case Param::FmRatio: return FM_RATIO_STR[value];
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
    freq = Frequency::ofMidiNote(note).asHz();
    vol = (float)velocity / 127.0f;
  }

  inline void NoteOff(uint8_t note) 
  { 
    (void)note;
    vol = 0;
  }

  inline void GateOn(uint8_t velocity) 
  {
    freq = Frequency::ofMidiNote(noteParam).asHz();
    vol = (float)velocity / 127.0f;
  }

  inline void GateOff() 
  {
    vol = 0;
  }

  inline void AllNoteOff() 
  {
    vol = 0;
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
  static inline float stepPhase(const float phase, const float step)
  {
    return phase > TWO_PI ? phase - TWO_PI + step : phase + step;
  }
};
