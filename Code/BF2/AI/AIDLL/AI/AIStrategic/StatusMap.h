// Strategic status values used by scripts. Unknown input maps to Unknown.
// Writing an out-of-range value also produces Unknown, so such values do not round-trip.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_AISTRATEGIC_STATUSMAP_H      // [INFERRED] guard style
#define AI_AIDLL_AI_AISTRATEGIC_STATUSMAP_H

namespace dice {
namespace hfe {
namespace ai {

class StatusMap
{
public:
    // Unknown is the parser fallback for unrecognized input, including tokens never spelled "Unknown".
    enum Status
    {
        Owned   = 0,
        Hostile = 1,
        Neutral = 2,
        Unknown = 3
    };
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AISTRATEGIC_STATUSMAP_H
