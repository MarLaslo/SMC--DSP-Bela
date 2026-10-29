#pragma once

#include <cmath>
#include <libraries/DelayLine/DelayLine.h>
#include "../Utility.h"
#include "../../libs/DaisySP/Source/Utility/dsp.h"

namespace dsp
{
/** Delay: a classic echo. Repeats the input after a set time, with each
 *  repeat quieter than the last. Built on Bela's DelayLine library.
 *
 *  Usage:
 *      Delay delay;
 *      delay.Init(sample_rate);
 *      delay.SetTimeMs(350.f);
 *      delay.SetFeedback(0.4f);
 *      delay.SetMix(0.5f);
 *      float sample = delay.Process(input);
 */
class Delay
{
  public:
    /** Longest delay time this can hold, in milliseconds. */
    static constexpr float kMaxDelayMs = 1000.f;

    /** Call in setup(): this is where the delay memory is reserved. */
    void Init(float sample_rate)
    {
        sample_rate_ = sample_rate;
        // Time constant for smoothing the delay time (see SetTimeMs).
        smooth_coeff_ = 1.f - expf(-1.f / (0.12f * sample_rate_));
        line_.setup(kMaxDelayMs, sample_rate_, 1);
        SetTimeMs(350.f);
        current_ms_ = target_ms_;
        SetFeedback(0.7f);
        SetMix(0.5f);
    }

    /** Time between repeats, in milliseconds. Range: 0 - 1000 ms.
     *
     *  Expected to be called continuously (e.g. once per render() block
     *  from a knob), so the actual delay time glides smoothly toward
     *  this target rather than snapping to it - snapping would jump the
     *  read position by a sample or more on every call, which clicks
     *  audibly even from the small jitter a knob reading has while
     *  sitting still. The glide is also what gives a "tape echo" its
     *  pitch-bend character when you sweep the time by hand. */
    void SetTimeMs(float ms)
    {
        // At least one sample: Bela's DelayLine treats 0 as the full length
        float min_ms = 1000.f / sample_rate_;
        target_ms_   = Clamp(ms, min_ms, kMaxDelayMs - min_ms);
    }

    /** How much of the repeated signal feeds back into itself, for more
     *  repeats. Range: 0 (single repeat) - 1 (repeats forever).
     *
     *  Capped just under 1: at exactly 1 the loop's gain (roughly
     *  1 / (1 - feedback) for a held, continuous input) diverges, so
     *  a sustained note builds up without bound the longer it's held,
     *  eventually overwhelming SoftClip's headroom and distorting. */
    void SetFeedback(float feedback) { line_.setFeedbackGain(Clamp(feedback, 0.f, 0.97f)); }

    /** Balance between the dry input and the delayed signal. Range: 0
     *  (dry only) - 1 (delayed signal only). */
    void SetMix(float mix) { line_.setWetDryMix(mix); }

    /** Returns the next audio sample. */
    float Process(float in)
    {
        current_ms_ += (target_ms_ - current_ms_) * smooth_coeff_;
        line_.setDelayTime(current_ms_);
        return daisysp::SoftClip(line_.process(in));
    }

  private:
    // "::" because Bela's DelayLine is outside the dsp namespace
    ::DelayLine line_;
    float       sample_rate_  = 44100.f;
    float       target_ms_    = 350.f;
    float       current_ms_   = 350.f;
    float       smooth_coeff_ = 1.f;
};

} // namespace dsp
