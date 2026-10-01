// [INFERRED names] Internal numeric formatting helpers; float callers request seven or nine fractional digits.
#ifndef BFMEME_DATAFORMATTING_H
#define BFMEME_DATAFORMATTING_H
#include "../../Core/BuildConfig.h"
#if !DEDICATED
namespace dice { namespace meme {
void formatStringDataFloat(float value, char* output, int digits = 7);
// sprintf(output, "%i", value) with no width bound. IntData82D96F
// calls the shared helper; the remaining Int/IntRef formatting sites inline the
// same sprintf. Every caller reserves exactly twelve bytes.
void formatStringDataInt(int value, char* output);
} }
#endif
#endif
