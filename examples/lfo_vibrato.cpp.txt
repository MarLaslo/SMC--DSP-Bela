/** SMC26 DSP course example: LFO vibrato.
 *
 *  An LFO (Low Frequency Oscillator) is a wave too slow to hear as a
 *  sound. Instead, it moves a setting up and down for you. Here it
 *  moves the pitch of a steady saw tone, around A3: a little and slow
 *  is a singer's vibrato, a lot and fast is a siren or a laser.
 *
 *  The LFO's waveform decides HOW the pitch moves. Press waveButton to
 *  step through them (the name is printed in the terminal):
 *    1. sine     - glides smoothly up and down (classic vibrato)
 *    2. triangle - goes up and down at an even speed
 *    3. square   - jumps between two notes (a trill, or a police siren)
 *    4. saw      - jumps up, then slides down
 *    5. ramp     - slides up, then jumps down
 *
 *  rateKnob   how fast the pitch moves: half a wobble to 15 a second
 *  depthKnob  how far it moves: none at all, up to an octave either way
 *
 *  To try it, copy this whole file into render.cpp and type
 *  "make run" in the terminal. Ctrl-C stops it.
 *
 *  Wiring:
 *    waveButton - D0     rateKnob  - A0
 *                        depthKnob - A1
 *  The button connects its pin to 3.3V when pressed, with a 10k
 *  resistor from the pin to GND (a pull-down).
 */

// The building blocks: the board (Bela.h), the sensors, and the
// synthesis, effects, filters and utilities. You do not need to
// change these.
#include <Bela.h>
#include <cmath>
#include "sensors/sensors.h"
#include "dsp/dsp.h"

// ---------------------------------------------------------------------
// 1. Sensors
// ---------------------------------------------------------------------

Button waveButton;

Potentiometer rateKnob;
Potentiometer depthKnob;

// Change false to true to print the sensor values in the terminal
bool gPrintValues = false;

// Prints every sensor's value on one line. Add a line here for each
// sensor you add. Only runs when gPrintValues is true.
void PrintValues(BelaContext *context)
{
	PrintValue("wave", waveButton.Count());
	PrintValue("rate", dsp::FullRange(rateKnob.Read(context)));
	PrintValue("depth", dsp::FullRange(depthKnob.Read(context)));
	PrintLineEnd();
}

// ---------------------------------------------------------------------
// 2. Sound
// ---------------------------------------------------------------------

// The tone you hear: a saw wave that never stops
dsp::Oscillator tone;

// The LFO that moves its pitch. It makes no sound itself - it just
// moves between -1 and 1, over and over.
dsp::Lfo vibrato;

// The five LFO waveforms waveButton steps through, and their names
const int kNumWaves = 5;
dsp::Lfo::Waveform gWaves[kNumWaves] = {
	dsp::Lfo::WAVE_SINE,
	dsp::Lfo::WAVE_TRIANGLE,
	dsp::Lfo::WAVE_SQUARE,
	dsp::Lfo::WAVE_SAW,
	dsp::Lfo::WAVE_RAMP,
};
const char* gWaveNames[kNumWaves] = {"sine", "triangle", "square", "saw", "ramp"};

// The note the vibrato moves around
float gCenterNote = dsp::NoteToFreq("A3");

// How far the vibrato moves, in semitones either way. Set by depthKnob
// at the top of render(), used in the loop below.
float gDepthSemitones = 0.f;

// ---------------------------------------------------------------------
// 3. render()
// ---------------------------------------------------------------------

// Called by the board over and over, very fast, each time for the
// next few samples of sound. Keep it quick: no printf() and no waiting
// in here, or the sound will click and drop out.
void render(BelaContext *context, void *userData)
{
	// Read the button first, so JustPressed() below sees the latest
	// press
	waveButton.Update(context);

	// Each press: the next waveform. Count() goes up by one on every
	// press, and "% kNumWaves" wraps it back to 0 after the last one.
	if(waveButton.JustPressed())
	{
		int wave = waveButton.Count() % kNumWaves;
		vibrato.SetWaveform(gWaves[wave]);

		// rt_printf is Bela's printf for render(). Use it only now and
		// then - here just once per press.
		rt_printf("LFO: %s\n", gWaveNames[wave]);
	}

	// Rate: from one wobble every 2 seconds (0.5 Hz) to 15 a second.
	// MapExponential so the knob feels even across its whole travel
	// (see dsp/Utility.h).
	vibrato.SetFreq(dsp::MapExponential(dsp::FullRange(rateKnob.Read(context)), 0.5f, 15.f));

	// Depth: from 0 (no vibrato) to 12 semitones (an octave) either way
	gDepthSemitones = dsp::MapLinear(dsp::FullRange(depthKnob.Read(context)), 0.f, 12.f);

	// About 10 times a second, print the sensor values - only if
	// gPrintValues is true
	if(gPrintValues && TimeToPrint(context))
		PrintValues(context);

	// Runs once per sample - context->audioFrames samples each time
	for(unsigned int n = 0; n < context->audioFrames; n++)
	{
		// The LFO runs here, once per sample like the sound, so the
		// pitch moves perfectly smoothly. Its value goes from -1 to 1,
		// so this moves the pitch from -depth to +depth semitones.
		float semitones = gDepthSemitones * vibrato.Process();

		// Every semitone up multiplies the pitch by 2^(1/12), so:
		// center * 2^(semitones / 12)
		tone.SetFreq(gCenterNote * powf(2.f, semitones / 12.f));

		// The next sample of the tone, turned down by 12 dB - a saw
		// that never stops is loud
		float sample = dsp::ApplyGain(tone.Process(), -12.f);

		// Send it to the left (0) and right (1) outputs
		audioWrite(context, n, 0, sample);
		audioWrite(context, n, 1, sample);
	}
}

// ---------------------------------------------------------------------
// 4. setup()
// ---------------------------------------------------------------------

// Runs once, before the sound starts
bool setup(BelaContext *context, void *userData)
{
	// Keeps fading sounds from overloading the CPU - leave this here
	dsp::FlushDenormals();

	// Tell each sensor which pin it is plugged into
	waveButton.Init(context, 0); // digital pin D0
	rateKnob.Init(0);            // analog input A0
	depthKnob.Init(1);           // analog input A1

	float sampleRate = context->audioSampleRate;

	// A note that never ends: full sustain, started once here
	tone.Init(sampleRate);
	tone.SetWaveform(dsp::Oscillator::WAVE_SAW);
	tone.SetAmpSustain(1.f);
	tone.NoteOn();

	// Starts as a sine
	vibrato.Init(sampleRate);
	vibrato.SetWaveform(dsp::Lfo::WAVE_SINE);

	// From here on, the board keeps calling render()
	return true;
}

// Runs once when the program stops
void cleanup(BelaContext *context, void *userData)
{
	// Nothing to do here
}
