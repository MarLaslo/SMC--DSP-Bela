#pragma once

/** Every building block in one include: utilities, synthesis,
 *  effects, filters and the rest, plus printing (Board.h).
 *  render.cpp only needs:
 *      #include "dsp/dsp.h"
 */

#include "Utility.h"
#include "Board.h"
#include "Synthesis/Synthesis.h"
#include "Effects/Effects.h"
#include "Filters/Filters.h"
#include "Other/Other.h"
