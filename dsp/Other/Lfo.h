#pragma once

#include <libraries/Oscillator/Oscillator.h>

namespace dsp
{
/** A slow, free-running oscillator for modulating a parameter instead
 *  of making sound - vibrato on a pitch, a wobbling filter cutoff, a
 *  tremolo on amplitude. Unlike dsp::Oscillator it has no amp envelope
 *  or NoteOn/NoteOff - an LFO just keeps cycling on its own for as
 *  long as the patch runs.
 *
 *  Process() returns a bipolar value in -1..1; multiply it by how far
 *  you want the parameter to swing (the modulation "depth") and add it
 *  to the parameter's base value.
 *
 *  Usage:
 *      Lfo vibrato;
 *      vibrato.Init(sample_rate);
 *      vibrato.SetFreq(5.f);              // 5 wobbles per second
 *      vibrato.SetWaveform(Lfo::WAVE_SINE);
 *      ...
 *      float depth = 10.f;                // +/- 10 Hz of wobble
 *      voice.SetFreq(440.f + depth * vibrato.Process());
 */
class Lfo
{
  public:
    enum Waveform
    {
        WAVE_SINE,
        WAVE_TRIANGLE,
        WAVE_SQUARE,
        WAVE_SAW,  // falls from 1 to -1, then jumps back up
        WAVE_RAMP, // rises from -1 to 1, then jumps back down
    };

    void Init(float sample_rate)
    {
        osc_.setup(sample_rate, ::Oscillator::sine);
        SetWaveform(WAVE_SINE);
        SetFreq(1.f);
        Reset();
    }

    /** LFO rate, in Hz. Typical range is well under 20 Hz - above
     *  that it starts to sound more like an audio tone than a wobble. */
    void SetFreq(float freq_hz) { osc_.setFrequency(freq_hz); }

    void SetWaveform(Waveform wave)
    {
        // Bela's Oscillator only has a rising saw, so WAVE_SAW is
        // that one turned upside down in Process()
        invert_ = wave == WAVE_SAW;
        switch(wave)
        {
            case WAVE_TRIANGLE: osc_.setType(::Oscillator::triangle); break;
            case WAVE_SQUARE: osc_.setType(::Oscillator::square); break;
            case WAVE_SAW:
            case WAVE_RAMP: osc_.setType(::Oscillator::sawtooth); break;
            case WAVE_SINE:
            default: osc_.setType(::Oscillator::sine); break;
        }
    }

    /** Jumps back to the start of its cycle - call on a NoteOn so the
     *  wobble starts from the same place every note instead of
     *  wherever it happened to be free-running. */
    void Reset() { osc_.setPhase(0.f); }

    /** Returns the next LFO value, in -1..1. */
    float Process()
    {
        float out = osc_.process();
        return invert_ ? -out : out;
    }

  private:
    // Bela's Oscillator - "::" because dsp::Oscillator is the synth voice
    ::Oscillator osc_;
    bool         invert_ = false;
};

} // namespace dsp
