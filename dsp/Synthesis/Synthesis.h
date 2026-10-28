#pragma once

/** Easy DSP library: include this one header to get every synthesis
 *  technique. Each technique is a small class with the same shape:
 *
 *      Init(sample_rate)    - call once, in setup()
 *      SetFreq(freq_hz)     - set the pitch
 *      NoteOn() / NoteOff() - start and stop a note
 *      Process()            - call once per audio sample, returns it
 */

#include "Oscillator.h"
