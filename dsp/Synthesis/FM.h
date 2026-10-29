#pragma once

#include "FMOperator.h"
#include "../Other/Adsr.h"

namespace dsp
{
/** FM synthesis: carrier oscillator frequency-modulated by a modulator
 *  oscillator (see FMOperator.h), with two envelopes:
 *  one shapes the volume, the other shapes the modulation depth, so
 *  the timbre can open up on the attack and mellow out over the note
 *  (classic FM/DX7-style behavior).
 *  Usage:
 *      FM voice;
 *      voice.Init(sample_rate);
 *      voice.SetFreq(440.f);
 *      voice.NoteOn();
 *      float sample = voice.Process();
 *      ...
 *      voice.NoteOff();
 */
class FM
{
  public:
    void Init(float sample_rate)
    {
        fm_.Init(sample_rate);
        SetFreq(440.f);
        SetModRatio(2.f);
        SetModDepth(1.f);

        amp_env_.Init(sample_rate);
        amp_env_.SetAttack(0.01f);
        amp_env_.SetDecay(0.3f);
        amp_env_.SetSustain(0.8f);
        amp_env_.SetRelease(0.3f);

        mod_env_.Init(sample_rate);
        mod_env_.SetAttack(0.01f);
        mod_env_.SetDecay(0.15f);
        mod_env_.SetSustain(0.3f);
        mod_env_.SetRelease(0.2f);
    }

    void SetFreq(float freq_hz) { fm_.SetFrequency(freq_hz); }

    /** Modulator frequency relative to the carrier (1 = same pitch). */
    void SetModRatio(float ratio) { fm_.SetRatio(ratio); }

    /** Peak amount the modulator pushes the carrier's pitch around,
     *  reached at the top of the modulation envelope. */
    void SetModDepth(float depth) { mod_depth_ = depth; }

    /** Starts a note: triggers both envelopes from the attack stage.
     *  On a note that's already playing, both start again from their
     *  current level, so every NoteOn() sounds like a new note. */
    void NoteOn()
    {
        amp_env_.NoteOn();
        mod_env_.NoteOn();
    }

    /** Releases the note: both envelopes move to their release stage. */
    void NoteOff()
    {
        amp_env_.NoteOff();
        mod_env_.NoteOff();
    }

    void SetAmpAttack(float time_s) { amp_env_.SetAttack(time_s); }
    void SetAmpDecay(float time_s) { amp_env_.SetDecay(time_s); }
    void SetAmpSustain(float level) { amp_env_.SetSustain(level); }
    void SetAmpRelease(float time_s) { amp_env_.SetRelease(time_s); }

    void SetModAttack(float time_s) { mod_env_.SetAttack(time_s); }
    void SetModDecay(float time_s) { mod_env_.SetDecay(time_s); }
    void SetModSustain(float level) { mod_env_.SetSustain(level); }
    void SetModRelease(float time_s) { mod_env_.SetRelease(time_s); }

    /** Returns the next audio sample. */
    float Process()
    {
        float amp_level = amp_env_.Process();
        float mod_level = mod_env_.Process();

        fm_.SetIndex(mod_depth_ * mod_level);
        return fm_.Process() * amp_level;
    }

  private:
    FMOperator fm_;
    Adsr       amp_env_;
    Adsr       mod_env_;
    float      mod_depth_ = 1.f;
};

} // namespace dsp
