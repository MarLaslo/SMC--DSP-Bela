#pragma once

#include "../../libs/DaisySP/Source/Synthesis/oscillator.h"
#include "../Other/Adsr.h"

namespace dsp
{
/** The simplest synth voice: a single oscillator shaped by an amp
 *  envelope. Start here before reaching for Additive, Subtractive, or
 *  FM - it's the one raw waveform all of those build on top of.
 *  Usage:
 *      Oscillator voice;
 *      voice.Init(sample_rate);
 *      voice.SetWaveform(Oscillator::WAVE_SINE);
 *      voice.SetFreq(440.f);
 *      voice.NoteOn();
 *      float sample = voice.Process();
 *      ...
 *      voice.NoteOff();
 */
class Oscillator
{
  public:
    enum Waveform
    {
        WAVE_SINE,
        WAVE_SAW,
        WAVE_SQUARE,
        WAVE_TRIANGLE,
    };

    void Init(float sample_rate)
    {
        osc_.Init(sample_rate);
        SetWaveform(WAVE_SINE);
        SetFreq(440.f);

        amp_env_.Init(sample_rate);
        amp_env_.SetAttack(0.01f);
        amp_env_.SetDecay(0.3f);
        amp_env_.SetSustain(0.8f);
        amp_env_.SetRelease(0.3f);
    }

    void SetFreq(float freq_hz) { osc_.SetFreq(freq_hz); }

    // The saw, square and triangle are DaisySP's band-limited
    // (polyBLEP) versions. Bela's own Oscillator has these shapes
    // too, but its sharp corners alias into harsh, out-of-tune
    // overtones at audio pitches.
    void SetWaveform(Waveform wave)
    {
        switch(wave)
        {
            case WAVE_SAW:
                osc_.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SAW);
                break;
            case WAVE_SQUARE:
                osc_.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_SQUARE);
                break;
            case WAVE_TRIANGLE:
                osc_.SetWaveform(daisysp::Oscillator::WAVE_POLYBLEP_TRI);
                break;
            case WAVE_SINE:
            default:
                osc_.SetWaveform(daisysp::Oscillator::WAVE_SIN);
                break;
        }
    }

    /** Starts a note: triggers the amp envelope from the attack stage.
     *  On a note that's already playing, the attack starts again from
     *  the current level, so every NoteOn() sounds like a new note. */
    void NoteOn() { amp_env_.NoteOn(); }

    /** Releases the note: the amp envelope moves to its release stage. */
    void NoteOff() { amp_env_.NoteOff(); }

    void SetAmpAttack(float time_s) { amp_env_.SetAttack(time_s); }
    void SetAmpDecay(float time_s) { amp_env_.SetDecay(time_s); }
    void SetAmpSustain(float level) { amp_env_.SetSustain(level); }
    void SetAmpRelease(float time_s) { amp_env_.SetRelease(time_s); }

    /** Returns the next audio sample. */
    float Process() { return osc_.Process() * amp_env_.Process(); }

  private:
    daisysp::Oscillator osc_;
    Adsr                amp_env_;
};

} // namespace dsp
