// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining BotMemory operations.

#include "BotMemory.h"

#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// Initializes behavior-memory sentinels. [UNRESOLVED] The fields' precise meanings remain unknown.
BotMemory::BotMemory()
    : m_int0(-1)
    , m_int4(-1)
    , m_float8(1.0f)
    , m_int12(1)
{
    // m_double16 is deliberately not initialised here. `BotMain`'s constructor writes it from
    // outside, immediately after allocating this object. Adding it would be an invented guard.
}

// Intentionally empty destructor.
BotMemory::~BotMemory()
{
}

void BotMemory::reset()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMemory::reset");
}

void BotMemory::modifyBehaviourUrgency(BotBehaviour*, double)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMemory::modifyBehaviourUrgency");
}

} // namespace ai
} // namespace hfe
} // namespace dice
