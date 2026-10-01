// [TEMP] [CHOSEN, not recovered] Sound-only fallback that reports a skipped call and continues.
// [RETIRE-WHEN: sound-seam-owner] Remove when all callers use typed g_bfSound playSound/stopSound.
// Each site names its caller and sound, counts every hit, and reports the first three then every
// 256th to stderr. No value is fabricated. Existing audio remains unchanged by the skipped call.
// Use only for an unrecovered audible effect; other frontiers retain their stop. stderr avoids
// early-boot logging dependencies and Debug's abort behavior.
#ifndef CORE_SOUNDSEAMFRONTIER_H
#define CORE_SOUNDSEAMFRONTIER_H
#include <cstdio>

namespace dice {
namespace hfe {

// `hits` is the caller's own counter - give each call site a distinct one so the
// rate limiting and the count stay per-seam. `action` is "start" or "stop".
inline void soundSeamFrontier(unsigned int& hits, const char* qualified,
                              const char* detail, const char* action, const char* sound)
{
    ++hits;
    if (hits > 3u && (hits & 0xFFu) != 0u) return;
    std::fprintf(stderr,
                 "[FRONTIER] %s reached (%s): would %s sound \"%s\" - REPORTED AND CONTINUED, "
                 "hit %u\n",
                 qualified, detail, action, sound, hits);
    std::fflush(stderr);
}

} // namespace hfe
} // namespace dice
#endif // CORE_SOUNDSEAMFRONTIER_H
