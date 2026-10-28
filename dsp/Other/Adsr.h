#pragma once

#include <libraries/ADSR/ADSR.h>

namespace dsp
{
/** ADSR envelope: a volume (or modulation) shape with four stages -
 *  Attack, Decay, Sustain, Release. Useful on its own for driving a
 *  filter cutoff, amplitude, or any other parameter over time.
 *  Usage:
 *      Adsr env;
 *      env.Init(sample_rate);
 *      env.SetAttack(0.01f);
 *      env.SetDecay(0.2f);
 *      env.SetSustain(0.7f);
 *      env.SetRelease(0.3f);
 *      env.NoteOn();
 *      float level = env.Process();
 *      ...
 *      env.NoteOff();
 */
class Adsr
{
  public:
    void Init(float sample_rate)
    {
        sample_rate_ = sample_rate;
        env_.reset();
        SetAttack(0.01f);
        SetDecay(0.2f);
        SetSustain(0.7f);
        SetRelease(0.3f);
    }

    /** Starts the envelope from its attack stage. If it's already
     *  running, the attack starts again from the current level. */
    void NoteOn() { env_.gate(1); }

    /** Moves the envelope to its release stage. */
    void NoteOff() { env_.gate(0); }

    /** How long the envelope takes to reach full level, in seconds. */
    void SetAttack(float time_s) { env_.setAttackRate(ToSamples(time_s)); }

    /** How long the envelope takes to fall from full level to the
     *  sustain level, in seconds. */
    void SetDecay(float time_s) { env_.setDecayRate(ToSamples(time_s)); }

    /** Level the envelope holds at while the note is held. Range: 0 - 1. */
    void SetSustain(float level) { env_.setSustainLevel(level); }

    /** How long the envelope takes to fall to 0 after the note is
     *  released, in seconds. */
    void SetRelease(float time_s) { env_.setReleaseRate(ToSamples(time_s)); }

    /** True while the envelope is anywhere but fully idle at 0. */
    bool IsRunning() { return env_.getState() != ::env_idle; }

    /** Returns the next envelope value, 0 - 1. */
    float Process() { return env_.process(); }

  private:
    // Bela's ADSR takes stage lengths in samples, not seconds. At
    // least 1 sample, so a time of 0 still works (an instant jump).
    float ToSamples(float time_s)
    {
        float samples = time_s * sample_rate_;
        return samples < 1.f ? 1.f : samples;
    }

    ADSR  env_;
    float sample_rate_ = 44100.f;
};

} // namespace dsp
