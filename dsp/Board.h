#pragma once

/** The board: printing sensor values, and one setting the sound needs.
 *
 *  render.cpp calls dsp::FlushDenormals() once in setup(), and can
 *  print values from render() like this:
 *
 *      if (TimeToPrint(context))
 *      {
 *          PrintValue("knob1", knob1.Read(context));
 *          PrintLineEnd();
 *      }
 *
 *  The text shows up in the terminal that ran "make run".
 */
#include <Bela.h>
#include <cstdint>

namespace dsp
{
/** Flushes denormal floats to zero. Without this, a value decaying
 *  toward (but never quite reaching) zero - a delay repeat trailing
 *  off, an envelope's release tail - becomes a "denormal" number that
 *  the processor takes dramatically longer to do arithmetic on, long
 *  enough to overload the CPU and cause clicks and dropouts with no
 *  knob involved at all. Call once at the start of setup(). */
inline void FlushDenormals()
{
#if defined(__aarch64__)
    // ARMv8 (PocketBeagle 2): FZ is bit 24 of FPCR
    uint64_t fpcr;
    asm volatile("mrs %0, fpcr" : "=r"(fpcr));
    asm volatile("msr fpcr, %0" : : "r"(fpcr | (1 << 24)));
#elif defined(__arm__)
    // ARMv7 (BeagleBone): FZ is bit 24 of FPSCR
    uint32_t fpscr;
    asm volatile("vmrs %0, fpscr" : "=r"(fpscr));
    asm volatile("vmsr fpscr, %0" : : "r"(fpscr | (1 << 24)));
#endif
}

/** True when denormals are flushed to zero on the thread that calls
 *  it - for checking that FlushDenormals() reached render(). */
inline bool DenormalsFlushed()
{
#if defined(__aarch64__)
    uint64_t fpcr;
    asm volatile("mrs %0, fpcr" : "=r"(fpcr));
    return fpcr & (1 << 24);
#elif defined(__arm__)
    uint32_t fpscr;
    asm volatile("vmrs %0, fpscr" : "=r"(fpscr));
    return fpscr & (1 << 24);
#else
    return false;
#endif
}

} // namespace dsp

// ---------------------------------------------------------------------
// Printing
// ---------------------------------------------------------------------

// Prints sensor values all on one line, e.g.
//   button1: 1   knob1: 0.512   knob2: 0.000
// Call PrintValue() for each value and PrintLineEnd() to finish the
// line. Uses rt_printf, the printf that is safe inside render().

// For buttons and other on/off values: prints 1 or 0
inline void PrintValue(const char* name, bool value)
{
    rt_printf("%s: %d   ", name, value ? 1 : 0);
}

// For knobs and any other number: prints it with 3 decimals
inline void PrintValue(const char* name, float value)
{
    rt_printf("%s: %.3f   ", name, value);
}

// For whole numbers, e.g. a button's Count()
inline void PrintValue(const char* name, int value)
{
    rt_printf("%s: %d   ", name, value);
}
inline void PrintValue(const char* name, unsigned int value)
{
    rt_printf("%s: %u   ", name, value);
}

inline void PrintLineEnd()
{
    rt_printf("\n");
}

// True about 10 times a second - wrap your printing in it so the
// values scroll by slowly enough to read (and so printing doesn't
// slow down the sound). Call it once per render().
inline bool TimeToPrint(BelaContext* context)
{
    static uint64_t last     = 0;
    uint64_t        now      = context->audioFramesElapsed;
    uint64_t        interval = (uint64_t)(context->audioSampleRate / 10.f);
    if(now - last < interval)
        return false;
    last = now;
    return true;
}
