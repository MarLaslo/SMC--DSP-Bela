/** SMC26 DSP course starter project.
 *
 *  This is the only file you need to edit. It has four parts, top to
 *  bottom:
 *    1. Sensors  - declare your buttons/knobs/...
 *    2. Sound    - declare your dsp:: objects (oscillators, effects, ...)
 *    3. render() - called over and over: reads the sensors and makes
 *                  the sound, one sample at a time
 *    4. setup()  - runs once at the start: sets up sensors and sound
 *
 *  Free pins on the board:
 *    Analog  (knobs, flex, piezo, ...): A0 A1 A2 A3 A4  (channels 0-4)
 *    Digital (buttons):                 D0 D1           (pins 0 and 1)
 *  Buttons: pin to 3.3V when pressed, 10k from the pin to GND (pull-down).
 *  Knobs: wrap Read() in dsp::FullRange() to get the whole 0 - 1.
 *
 *  For finished instruments to copy from, see examples/.
 *  To run it: "make run" in the terminal. Ctrl-C stops it.
 */

// The building blocks: the board (Bela.h), the sensors, and the
// synthesis, effects, filters and utilities. You do not need to
// change these.
#include <Bela.h>
#include "sensors/sensors.h"
#include "dsp/dsp.h"

// ---------------------------------------------------------------------
// 1. Sensors
// ---------------------------------------------------------------------

// Declare your sensors here (see sensors/sensors.h for every kind),
// e.g.
//   Button button1;
//   Potentiometer knob1;

// Change false to true to print the sensor values in the terminal
bool gPrintValues = false;

// Prints every sensor's value on one line. Add a line here for each
// sensor you add. Only runs when gPrintValues is true.
void PrintValues(BelaContext *context)
{
	// PrintValue("button1", button1.IsPressed());
	// PrintValue("knob1", dsp::FullRange(knob1.Read(context)));
	PrintLineEnd();
}

// ---------------------------------------------------------------------
// 2. Sound
// ---------------------------------------------------------------------

// Declare your dsp:: objects here, e.g.
//   dsp::Oscillator osc;

// ---------------------------------------------------------------------
// 3. render()
// ---------------------------------------------------------------------

// Called by the board over and over, very fast, each time for the
// next few samples of sound. Keep it quick: no printf() and no waiting
// in here, or the sound will click and drop out.
void render(BelaContext *context, void *userData)
{
	// Read your sensors here and pass their values on to your dsp::
	// objects. Call Update() on each button first, so JustPressed()
	// and IsPressed() see the latest press, e.g.
	//   button1.Update(context);
	//   osc.SetFreq(dsp::MapLinear(dsp::FullRange(knob1.Read(context)), 100.f, 1000.f));

	// About 10 times a second, print the sensor values - only if
	// gPrintValues is true
	if(gPrintValues && TimeToPrint(context))
		PrintValues(context);

	// Runs once per sample - context->audioFrames samples each time
	for(unsigned int n = 0; n < context->audioFrames; n++)
	{
		// Work out this sample (a number from -1 to 1) here,
		// e.g. float sample = osc.Process();
		float sample = 0.f;

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

	// Tell each sensor which pin it is plugged into, e.g.
	//   button1.Init(context, 0); // digital pin D0
	//   knob1.Init(0);            // analog input A0

	// Init() your dsp:: objects here, e.g.
	//   osc.Init(context->audioSampleRate);
	//   osc.NoteOn(); // start the note so you can hear it

	// From here on, the board keeps calling render()
	return true;
}

// Runs once when the program stops
void cleanup(BelaContext *context, void *userData)
{
	// Nothing to do here
}
