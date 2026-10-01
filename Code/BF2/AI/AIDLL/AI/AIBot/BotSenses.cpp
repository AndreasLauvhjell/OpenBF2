// [FRONTIER] Remaining methods stop by name until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining BotSenses operations.

#include "BotSenses.h"

#include "../AIUtils/checkPointer.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// Creates the sight frustum and initializes sensing collections for the supplied bot.
BotSenses::BotSenses(BotMain* owner)
    : m_bot(owner)
    // the camera. See the block above.
    , m_frustum(checkPointer(new Frustum()))
    , m_double12(-1000000.0)
    , m_int32(0)
    , m_int36(0)
    , m_int40(0)
    , m_int88(0)
    , m_int92(0)
{
    // the constructor'S last statement - see the block above. The binary copies the list's head
    // pointer; in source that is the list's own end. The cursor at +80 is deliberately not here.
    m_cursor60 = m_list56.end();
}

// Clears sensed-object records and resets per-life sensing state.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
void BotSenses::reset()
{
    for (SensedObjectMap::iterator it = m_sensedObjects.begin(); it != m_sensedObjects.end(); )
    {
        // Retail here releases the object's `Information` through `informationManager` and then
        // deletes `it->second`. See the block above for why this tree cannot write it yet.
        BF2_AI_FRONTIER("dice::hfe::ai::BotSenses::reset (sensed objects present)");
        m_sensedObjects.erase(it++);
    }
    m_sensedObjects.clear();

    m_map44.clear();
    m_list56.clear();
    m_cursor60 = m_list56.end();
    m_sortBuffer.clear();

    m_int40 = 0;
    m_int36 = 0;
    m_int32 = 0;
    m_double12 = -1000000.0;

    m_set64.clear();
    m_list76.clear();
    // The constructor never did this; the cursor at +80 becomes valid here for the first time.
    m_cursor80 = m_list76.end();

    for (std::list<FireEntry>::iterator it = m_list84.begin(); it != m_list84.end(); ++it)
    {
        // Retail here is `delete it->first;`. See the block above.
        BF2_AI_FRONTIER("dice::hfe::ai::BotSenses::reset (fire objects present)");
    }
    m_list84.clear();

    m_int88 = 0;
    m_int92 = 0;

    m_map100.clear();
}

// Resets senses and deletes the frustum; member containers release their own storage.
BotSenses::~BotSenses()
{
    reset();
    delete m_frustum;
}

// [FRONTIER] Remaining sight, hearing and communication operations require implementation.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

} // namespace ai
} // namespace hfe
} // namespace dice
