// [FRONTIER] Remaining methods stop by name until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining BotNavigation operations.

#include "BotNavigation.h"

#include "BotMain.h"                       // getBotId / getCurrentEquipment - vtable +0xE8, +0xD8
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"   // IBFTimer::getGameTime
#include "../AIKnowledge/InformationManager.h"
#include "../AIUtils/checkPointer.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {
namespace ai {

// Creates navigation paths and initializes movement state for the supplied bot.
BotNavigation::BotNavigation(BotMain* owner)
    : m_bot(owner)
    , m_float4(2.0f)
    , m_float8(2.0f)
    , m_int16(-1)
    , m_int20(-1)
    , m_int36(-1)
    , m_flag40(false)
    // the two paths, in the order the binary builds them: the fine one first.
    , m_polygonPath(checkPointer(new QTPath()))
    , m_int76(-1)
    , m_int116(0)
    , m_clusterPath(checkPointer(new ClusterNodePath()))
    , m_int128(-1)
    , m_double144(0.0)
    , m_double152(0.0)
    , m_double160(0.0)
    , m_double168(0.0)
    // 9,999,999.0f - a distance sentinel, not a time and not a magnitude. See above.
    , m_sentinel188(9999999.0f)
    , m_flag192(false)
    , m_flag193(false)
    , m_flag195(false)
    , m_int200(-1)
    , m_int204(-1)
    , m_int208(-1)
    , m_int212(-1)
    , m_int240(0)
    , m_double268(0.0)
    , m_flag276(false)
    // Independent sentinel, initialized to -1.0.
    , m_double300(-1.0)
{
    for (int i = 0; i < 3; ++i)
    {
        m_zero24[i]  = 0;
        m_zero44[i]  = 0;
        m_zero88[i]  = 0;
        m_zero176[i] = 0;
        m_zero216[i] = 0;
        m_zero256[i] = 0;
    }
    m_pathOwner = 0;                   // +60 - a pointer, see BotNavigation.h
    m_zero64 = 0;
    m_zero308[0] = 0;
    m_zero308[1] = 0;
    for (int i = 0; i < 5; ++i)
        m_zero280[i] = 0;
    for (int i = 0; i < 6; ++i)
        m_zero320[i] = 0;

    // the dead resize - see the block above. Written as what it is rather than dropped.
    m_points.resize(0);
}

// [FRONTIER] Remaining navigation operations stop until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.

BotNavigation::~BotNavigation()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::~BotNavigation");
}

// Resets navigation, movement flags and progress timing.
void BotNavigation::resetAll()
{
    reset();

    m_int252 = 0;
    m_flag276 = false;
    // Not the constructor's 0.0 - see the block above.
    m_double268 = -9999.0;
    m_zero308[0] = 0;
    m_double300 = -1.0;

    clearProgress();
}

// Resets navigation state and returns path resources to their owners.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] Incomplete resource paths stop by name.
// [RETIRE-WHEN: aidll-02-skeleton] Implement and validate path release and progress reset.
// [RETIRE-WHEN: botmain-bases] Remove when the corresponding implementation is complete and validated.
void BotNavigation::reset()
{
    m_flag316 = false;

    // giving the polygon path back. See the block above.
    if (m_int76 != -1)
    {
        // Retail: `m_pathOwner->release(&m_int76, m_bot->getBotId)`. The bot half is typed now;
        // the object at +60 still has no class, so the slot cannot be.
        BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::reset (a polygon path is reserved)");
        m_int76 = -1;
    }

    // and the cluster path, through a different slot on the same object.
    if (m_int128 != -1)
    {
        BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::reset (a cluster path is reserved)");
        m_int128 = -1;
    }

    // Reaching through both path objects - see the block above.
    m_polygonPath->m_nodes.resize(0);
    m_polygonPath->m_i0 = 0;
    m_polygonPath->m_i20 = 0;

    m_clusterPath->m_nodes.resize(0);
    m_clusterPath->m_i0 = 0;

    m_int56 = 0;

    m_points.resize(0);

    m_int240 = 0;
    m_int116 = 0;
    m_double160 = 0.0;
    m_flag195 = false;
    m_flag193 = false;
    m_flag194 = false;
    m_double244 = 0.0;

    // Resolves the bot's current equipment handle; skips equipment work when it has no object.
    const unsigned int equipment = m_bot->getCurrentEquipment();
    Information* const info = informationManager->getPool()->get(equipment);
    if (info)
    {
        // Retail: `IPIMobile* m = info->m_mobile56; if (m) { m_int80 = m->getVehicleNumber;
        // const Vec* p = m->getLastValidPosition(m_int80); m_int200 = p[0]; ... }`
        // `Information` and `IPIMobile` are both unrecovered - `Information` is a forward
        // declaration behind the handle pool and `IPIMobile` is not in this tree at all.
        BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::reset (the bot has equipment)");
    }

    // The sentinel is the interesting one - see the block above.
    m_sentinel188 = 9999999.0f;
    m_zero280[3] = 0;
    m_zero280[4] = 0;
}

// Clears progress counters and restarts the progress timer at the current game time.
// Paused game time does not accumulate lack of progress.
void BotNavigation::clearProgress()
{
    m_zero176[0] = 0;                      // +176

    // the one non-zero store. See the block above.
    m_double144 =
        static_cast<const IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();

    m_double168 = 0.0;
    m_zero176[1] = 0;                      // +180
    m_zero176[2] = 0;                      // +184
    m_double152 = 0.0;
    m_flag192 = false;
    m_zero280[0] = 0;                      // +280
    m_zero280[1] = 0;                      // +284
    m_zero280[2] = 0;                      // +288
}

// [FRONTIER] Obstacle updates for living, standing bots are not implemented.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Replace the stop when the corresponding navigation behavior is recovered.
void BotNavigation::updateObstacles()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::updateObstacles");
}

bool BotNavigation::isActive()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::isActive");
}

void BotNavigation::updateOccationally()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotNavigation::updateOccationally");
}

// Always returns true, leaving BotManager's final pathfinding sweep gated by isSearchActive.
bool BotNavigation::isRunning()
{
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
