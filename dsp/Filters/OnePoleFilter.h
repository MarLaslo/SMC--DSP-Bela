#pragma once

#include <libraries/OnePole/OnePole.h>

namespace dsp
{
/** One-pole filter: the simplest, gentlest lowpass or highpass
 *  (6 dB/oct). Good for smoothing a signal or control value, or for
 *  a subtle tone-shaping effect.
 *
 *  To smooth a sensor once per render() rather than once per sample,
 *  Init() it with context->audioSampleRate / context->audioFrames
 *  (how many times a second render() runs) instead of the sample
 *  rate, so SetFreq() is still in Hz.
 *  Usage:
 *      OnePoleFilter filter;
 *      filter.Init(sample_rate);
 *      filter.SetFreq(800.f);
 *      float sample = filter.Process(input);
 */
class OnePoleFilter
{
  public:
    enum Mode
    {
        MODE_LOWPASS,
        MODE_HIGHPASS,
    };

    void Init(float sample_rate)
    {
        sample_rate_ = sample_rate;
        filter_.setup(1000.f, sample_rate_, OnePole::LOWPASS);
        SetMode(MODE_LOWPASS);
    }

    /** Cutoff frequency, in Hz. */
    void SetFreq(float freq_hz)
    {
        // Bela's OnePole only works below half the sample rate
        float max = 0.49f * sample_rate_;
        filter_.setFilter(freq_hz > max ? max : freq_hz, OnePole::LOWPASS);
    }

    /** Filter response: lowpass or highpass. */
    void SetMode(Mode mode) { mode_ = mode; }

    /** Returns the next audio sample. */
    float Process(float in)
    {
        // The highpass is what the lowpass takes away. (Bela's OnePole
        // has a HIGHPASS type too, but at low cutoffs it lets nearly
        // everything through.)
        float low = filter_.process(in);
        return mode_ == MODE_HIGHPASS ? in - low : low;
    }

  private:
    OnePole filter_;
    Mode    mode_        = MODE_LOWPASS;
    float   sample_rate_ = 44100.f;
};

} // namespace dsp
