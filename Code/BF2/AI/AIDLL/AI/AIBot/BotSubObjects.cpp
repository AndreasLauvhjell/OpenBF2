// Aim-correction state initialization.

#include "BotSubObjects.h"

namespace dice {
namespace hfe {
namespace ai {

// Initializes aim-correction state while leaving byte ranges +0..+3 and +68..+83
// untouched. [UNRESOLVED] The contents of those ranges remain unknown.
FireCorrectionData::FireCorrectionData()
    : m_flag4(true)
    , m_flag5(true)
    , m_flag84(false)
{
    // +0..+3 and +68..+83 are deliberately untouched - see above.
    for (int i = 0; i < 15; ++i)
        m_zero8[i] = 0;
    for (int i = 0; i < 9; ++i)
        m_zero88[i] = 0;
}


} // namespace ai
} // namespace hfe
} // namespace dice
