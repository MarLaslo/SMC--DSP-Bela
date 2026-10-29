#pragma once

#include "../Utility.h"
#include "../../libs/DaisySP/DaisySP-LGPL/Source/Effects/reverbsc.h"

namespace dsp
{
/** Reverb: the sound of a room or hall. Many echoes blur together into
 *  a smooth tail that fades away after the sound stops. Built on
 *  DaisySP's ReverbSc (a stereo reverb, from DaisySP-LGPL).
 *  It holds about 400 KB of memory, so declare it with the other
 *  globals at the top of render.cpp, never inside a function.
 *  Usage:
 *      Reverb reverb;
 *      reverb.Init(sample_rate);
 *      reverb.SetTime(0.85f);
 *      reverb.SetMix(0.3f);
 *      float sample = reverb.Process(input);
 *  or in stereo:
 *      reverb.Process(inLeft, inRight, outLeft, outRight);
 */
class Reverb
{
  public:
    void Init(float sample_rate)
    {
        reverb_.Init(sample_rate);
        SetTime(0.85f);
        SetToneFreq(10000.f);
        SetMix(0.3f);
    }

    /** Length of the tail. Range: 0 (short, small room) - 1 (endless).
     *  Around 0.6 is a room, 0.9 a large hall. */
    void SetTime(float time) { reverb_.SetFeedback(Clamp(time, 0.f, 1.f)); }

    /** How bright the tail is, in Hz: lower values make it darker and
     *  softer, like a room full of curtains. Range: 100 - 20000 Hz. */
    void SetToneFreq(float freq_hz) { reverb_.SetLpFreq(Clamp(freq_hz, 100.f, 20000.f)); }

    /** Balance between the dry input and the reverb. Range: 0 (dry
     *  only) - 1 (reverb only). */
    void SetMix(float mix) { mix_ = Clamp(mix, 0.f, 1.f); }

    /** Mono: returns the next audio sample. */
    float Process(float in)
    {
        float wet_l, wet_r;
        reverb_.Process(in, in, &wet_l, &wet_r);
        float wet = 0.5f * (wet_l + wet_r);
        return (1.f - mix_) * in + mix_ * wet;
    }

    /** Stereo: reads both inputs, writes both outputs. */
    void Process(float in_l, float in_r, float& out_l, float& out_r)
    {
        float wet_l, wet_r;
        reverb_.Process(in_l, in_r, &wet_l, &wet_r);
        out_l = (1.f - mix_) * in_l + mix_ * wet_l;
        out_r = (1.f - mix_) * in_r + mix_ * wet_r;
    }

  private:
    daisysp::ReverbSc reverb_;
    float             mix_ = 0.3f;
};

} // namespace dsp
