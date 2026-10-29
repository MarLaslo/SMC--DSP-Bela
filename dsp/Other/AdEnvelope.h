#pragma once

#include <libraries/ADSR/ADSR.h>

namespace dsp
{
/** AD envelope: a one-shot envelope with just Attack and Decay (no
 *  sustain), triggered on each hit. Handy for percussive sounds or
 *  "zap" style modulation (e.g. sweeping a filter cutoff on each
 *  note). Built on Bela's ADSR library, with the sustain at 0 and the
 *  release done for you as soon as the decay ends.
 *  Usage:
 *      AdEnvelope env;
 *      env.Init(sample_rate);
 *      env.SetAttack(0.01f);
 *      env.SetDecay(0.3f);
 *      env.Hit();
 *      float level = env.Process();
 */
class AdEnvelope
{
  public:
    void Init(float sample_rate)
    {
        sample_rate_ = sample_rate;
        env_.reset();
        env_.setSustainLevel(0.f);
        env_.setReleaseRate(1.f);
        SetAttack(0.01f);
        SetDecay(0.3f);
        SetMin(0.f);
        SetMax(1.f);
    }

    /** Starts (or restarts) the envelope from its attack stage. A hit
     *  while it is still running starts again from the current level. */
    void Hit() { env_.gate(1); }

    /** How long the envelope takes to reach its max value, in seconds. */
    void SetAttack(float time_s) { env_.setAttackRate(ToSamples(time_s)); }

    /** How long the envelope takes to fall back to its min value, in
     *  seconds. */
    void SetDecay(float time_s) { env_.setDecayRate(ToSamples(time_s)); }

    /** Value the envelope sits at when idle / at the start of attack. */
    void SetMin(float min) { min_ = min; }

    /** Value the envelope reaches at the peak of attack. */
    void SetMax(float max) { max_ = max; }

    /** True while the envelope is running (anywhere but idle). */
    bool IsRunning() { return env_.getState() != ::env_idle; }

    /** Returns the next envelope value. */
    float Process()
    {
        float level = env_.process();
        // The decay has reached 0: let go, so the envelope goes idle
        // and the next Hit() starts a fresh attack
        if(env_.getState() == ::env_sustain)
            env_.gate(0);
        return min_ + (max_ - min_) * level;
    }

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
    float min_         = 0.f;
    float max_         = 1.f;
};

} // namespace dsp
