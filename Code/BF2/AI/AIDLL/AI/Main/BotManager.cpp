// Owns the bot pool and schedules sensing, decisions and plan execution.
// Work is distributed across ticks according to per-bot update deadlines and detail level.

#include "BotManager.h"


#include "AIDllFrontier.h"
#include "AIMain.h"                  // IAIMain::m_instance - cached at +60
#include "AISettingsInterfaces.h"    // IAISettings::getMaxNBots
#include "../AIBot/BotMain.h"          // the preallocated pool
#include "../AIBot/BotMemory.h"
#include "../AIBot/BotOrder.h"
#include "../AIBotBehaviour/UrgencyMerger.h"
#include "BotBehaviourManager.h"
#include "../AIUtils/checkPointer.h"
#include "CommonSingletonServer.h"
#include "../AIStrategic/SAI.h"                 // ISAI::addBot - reactivateBot hands it over
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"   // IAIPlayer::setPlayer(int)
#include "../../../AIInterface/BFKnowledge/AIPlayerFactory.h"
#include "../../../AIInterface/BFKnowledge/BFTimer.h"    // IBFTimer::getUpdateTime - action's clock
#include "../../../AIInterface/BFKnowledge/BFGameEffector.h"   // IAIGameEffector::giveDamage
#include "../AIBot/BotNavigation.h"          // actionComAgent / actionUpdateNavPoints
#include "../AIBot/CommunicationAgent.h"     // actionComAgent
#include "../AIKnowledge/Information.h"      // updateDamages
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIMobile.h"        // actionExecutePlan's stall test, actionPathfinding
#include "IAIPathfinding.h"                  // actionPathfinding hands the searcher its share
#include "../../../AIInterface/BFKnowledge/BFObjectEnvironment.h"   // actionHearing's object list
#include "../AIKnowledge/IPIPhysical.h"
#include "../../../../IO/Input/PlayerInput.h" // actionExecutePlan presses a climber's inputs

namespace dice {
namespace hfe {
namespace ai {

// Runs the scheduled bot work within the supplied time budget.
void BotManager::action(const double& budget)
{
    IBFTimer* const timer = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer);
    const double deadline = timer->getUpdateTime() + budget;

    updateDamages();
    actionComAgent();

    // Measured after the two unbudgeted steps, so they shrink every share downstream.
    const double share = deadline - timer->getUpdateTime();
    const double planSlice = share * 0.6;
    actionExecutePlan(planSlice);
    actionUpdateNavPoints();

    double remaining = deadline - timer->getUpdateTime();
    std::list<BMUpdate>::iterator update = m_updates.begin();
    for (; update != m_updates.end() && remaining > 0.0; ++update)
    {
        switch (update->type)
        {
        case BMUpdate::Idle:
            break;
        case BMUpdate::DecisionMaking:
        {
            double slice = share * 0.2;
            if (slice > remaining)
                slice = remaining;
            actionDecisionMaking(slice);
            break;
        }
        case BMUpdate::Pathfinding:
        {
            double slice = share * 0.3;
            if (slice > remaining)
                slice = remaining;
            actionPathfinding(slice);
            break;
        }
        case BMUpdate::Hearing:
        {
            double slice = share * 0.025;
            if (slice > remaining)
                slice = remaining;
            actionHearing(slice);
            break;
        }
        case BMUpdate::Sensing:
        {
            double slice = share * 0.15;
            if (slice > remaining)
                slice = remaining;
            actionSensing(slice);
            break;
        }
        default:
            Debug(DebugTypeError, "AI/AIDLL/AI/Main/BotManager.cpp", 1179, "")
                << "Bad update type.";
        }
        remaining = deadline - timer->getUpdateTime();
    }

    // Rotate the work this tick did not reach toward the front: at most the ring's five entries.
    int moves = 5;
    std::list<BMUpdate>::iterator front = m_updates.begin();
    while (front != m_updates.end() && front->type != update->type && moves > 0)
    {
        m_updates.push_back(*front);
        m_updates.erase(front++);
        --moves;
    }
}

// Applies accumulated bot damage once per tick.
void BotManager::updateDamages()
{
    IAIGameEffector* const effector =
        static_cast<IAIGameEffector*>(CommonSingletonServer::m_iaiGameEffector);
    for (std::vector<BotInfo*>::iterator it = m_botInfos.begin(); it != m_botInfos.end(); ++it)
    {
        if (!(*it)->m_bot->isDead() && (*it)->m_damage > 0.0f)
        {
            world::IPlayer* const from =
                m_botInfos[(*it)->m_attackerIndex]->m_bot->getPlayer()->getPlayer();
#if defined(_WIN32)
            Bot* const attacker = m_botInfos[(*it)->m_attackerIndex]->m_bot;
            const unsigned int handle = (*it)->m_bot->getCurrentEquipment();
            Information* const information =
                (handle & 0xffffu) ? informationManager->getInformation(handle) : nullptr;
            IAIObject* const target = information->getAIObject();
            const float damage = (*it)->m_damage;
            effector->giveDamage(target, from, attacker->getSide(), damage);
#else
            BotInfo* const info = *it;
            const int side = m_botInfos[info->m_attackerIndex]->m_bot->getSide();
            InformationManager* const manager = informationManager;
            const unsigned int handle = (*it)->m_bot->getCurrentEquipment();
            Information* const information = manager->getInformation(handle);
            effector->giveDamage(information->getAIObject(), from, side, info->m_damage);
#endif
        }
        (*it)->m_damage = 0.0f;
    }
}

// [INFERRED] Services one eligible bot's radio per tick, resuming from the saved cursor.
// Skips dead, man-down, or navigation-blocked bots and makes at most one roster circuit.
void BotManager::actionComAgent()
{
    const unsigned int start = m_comAgentCursor;
    if (m_botInfos.empty())
        return;
    Bot* bot = m_botInfos[m_comAgentCursor]->m_bot;
    for (;;)
    {
        if (!bot->isDead() && !bot->getManDown() && !bot->getBotNavigation()->isPathSearchPending())
        {
            bot->getCommunicationAgent()->update();
            if (++m_comAgentCursor >= m_botInfos.size())
                m_comAgentCursor = 0;
            return;
        }
        if (++m_comAgentCursor >= m_botInfos.size())
            m_comAgentCursor = 0;
        if (start == m_comAgentCursor)
            return;
        bot = m_botInfos[m_comAgentCursor]->m_bot;
    }
}

// The squared speed the stall test compares, grouped (x*x + y*y) + z*z as the PE's x87 sequence
// is and summed in double, which is exactly the PE's 53-bit precision (float products are exact
// at 53 bits). [PLATFORM] the ELF keeps the sum at 64-bit precision. A NaN sum is never "at most"
// which is both images' unordered branch. BotMain::updatePlayerAction keeps its own copy.
static bool speedSquaredAtMost(const Vec3& speed, float limit)
{
    const double x = speed.x, y = speed.y, z = speed.z;
    return (x * x + y * y) + z * z <= limit;
}

// Executes plans for the scheduled portion of the roster and refills work queues within the budget.
void BotManager::actionExecutePlan(const double& slice)
{
    IBFTimer* const timer = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer);
    const double deadline = timer->getUpdateTime() + slice;

    m_queuePriorityA->clear();
    m_queuePriorityB->clear();
    m_queueSensing->clear();
    m_planExecutionsThisTick = 0;
    m_tickCount180 = 0;
    m_tickCount184 = 0;

    const unsigned int size = static_cast<unsigned int>(m_botInfos.size());
    m_planHalfToggle = !m_planHalfToggle;
    unsigned int count = size / 2 + 1;
    if (count > size)
        count = size;
    if (!m_planHalfToggle && (size & 1))
        --count;

    unsigned int processed = 0;
    while (processed < count)
    {
        BotInfo* info = m_botInfos[m_planCursor];
        Bot* const bot = info->m_bot;
        if (!bot || bot->isDead() || bot->isDeactivated() || !bot->getSquadMember())
        {
            ++processed;
            m_planCursor = (m_planCursor + 1) % size;
            continue;
        }

        bool executed = false;
        // Both images re-read the record's bot for these two questions.
        Information* const information =
            informationManager->getInformation(info->m_bot->getCurrentEquipment());
        const std::string name = info->m_bot->getPlayer()->getName();   // unused in both images
        if (!information)
        {
            ++processed;
            m_planCursor = (m_planCursor + 1) % size;
            continue;
        }

        if (bot->getBotNavigation()->isPathSearchPending())
        {
            if (bot->getPlayer() && bot->getPlayer()->getCurrentVehicle())
            {
                // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
                // Retail: `IAIObject* primary = getPlayer->getCurrentVehicle->getPrimaryObject;
                // if (primary) { bot->setBotStuck(true); primary->setIsAllowedByBots(false); }`
                // IAIObject slots ELF +0xFC / +0x9C (PE +0xF8 / +0x98), named from the engine's
                // AIObjectReal table (bf2 ). IAIObject has no declaration in this tree.
                IAIObject* const vehicle = bot->getPlayer()->getCurrentVehicle();
                (void)vehicle;
                BF2_AI_FRONTIER("dice::hfe::ai::BotManager::actionExecutePlan (IAIObject::getPrimaryObject)");
            }
            bool onFoot = false;
            if (bot->getPlayer())
                onFoot = bot->getPlayer()->getDefaultVehicle() == bot->getPlayer()->getCurrentVehicle();
            IPIMobile* const mobile = information->getMobile();
            if (!mobile
                || (speedSquaredAtMost(mobile->getSpeed(), 0.25f) && onFoot)
                || (!information->getPhysical()->isInAir() && information->getPhysical()->isInWater()))
            {
                static_cast<IAIGameEffector*>(CommonSingletonServer::m_iaiGameEffector)
                    ->respawnBot(bot->getPlayer());
                ++processed;
                m_planCursor = (m_planCursor + 1) % size;
                bot->updatePlayerAction(false);
                continue;
            }
        }

        if (information->hasArmament())
            bot->rearm(information);
        info->m_double24 += timer->getTimeIncrease();
        double time = timer->getUpdateTime();
        const bool inBudget = deadline > time;
        if (inBudget)
        {
            const double noSlice = 0.0;
            bot->planExecution(noSlice);
            // The same local becomes the elapsed time (the ELF stores it back into one slot).
            time = timer->getUpdateTime() - time;
            info->m_double48 += time;
            ++info->m_word36;
            executed = true;
            if (bot->getPlayer()->isClimbingLadder())
            {
                // Both are the float overload; the ELF inlines the first, the PE both.
                bot->m_playerInput->setInput(io::PIThrottle, 1.0f);
                bot->m_playerInput->setInput(io::PIMouseLookY, 1.0f);
            }
        }

        info->m_priority += m_lodLevelFullPriority;
        info->m_sensingPriority += m_lodLevelFullPriority;
        ++m_planExecutionsThisTick;
        if (bot->hasPlan() && !bot->hasDefaultPlan())
        {
            if (info->m_double24 > m_plannedDecisionMakingThresholdFull)
                m_queuePriorityA->push(info);
        }
        else if (info->m_double24 > m_unplannedDecisionMakingThresholdFull)
        {
            m_queuePriorityB->push(info);
        }
        m_queueSensing->push(info);
        bot->updatePlayerAction(!inBudget);
        if (!executed)
            break;
        ++processed;
        m_planCursor = (m_planCursor + 1) % size;
    }

    for (unsigned int i = 0; i < size - processed; ++i)
        m_botInfos[m_planCursor]->m_bot->updatePlayerAction(false);
}

// [INFERRED] Visits at most one roster circuit and refreshes at most size/16 + 1 active
// navigations, skipping dead and man-down bots. Requires nonnull bot records: a null bot
// does not advance the cursor and would loop indefinitely.
void BotManager::actionUpdateNavPoints()
{
    if (m_botInfos.empty())
        return;
    const unsigned int limit = static_cast<unsigned int>(m_botInfos.size() >> 4) + 1;
    unsigned int updated = 0;
    unsigned int visited = 0;
    while (updated < limit && visited < m_botInfos.size())
    {
        Bot* const bot = m_botInfos[m_navPointCursor]->m_bot;
        if (!bot)
            continue;
        m_navPointCursor = (m_navPointCursor + 1) % static_cast<unsigned int>(m_botInfos.size());
        ++visited;
        if (bot->isDead() || bot->getManDown())
            continue;
        bot->getBotNavigation()->updateObstacles();
        if (bot->getBotNavigation()->isActive())
        {
            bot->getBotNavigation()->updateOccationally();
            ++updated;
        }
    }
}

// Schedules decisions using the interleave limits and each bot's detail level.
void BotManager::actionDecisionMaking(const double& slice)
{
    unsigned int light = 0;
    unsigned int full = 0;
    unsigned int medium = 0;
    const double deadline =
        static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getUpdateTime() + slice;

    int spare = static_cast<int>(m_plannedDecisionMakingInterleave)
        - static_cast<int>(m_queuePriorityA->size());
    if (spare < 0)
        spare = 0;
    const unsigned int unplannedLimit = m_unplannedDecisionMakingInterleave + spare;
    m_selectedBots.resize(0);

    while (!m_queuePriorityB->empty() && m_selectedBots.size() < unplannedLimit)
    {
        BotInfo* info = m_queuePriorityB->top();
        m_queuePriorityB->pop();
        switch (info->m_bot->getLodLevel())
        {
        case BotLodFull:
            ++full;
            break;
        case BotLodMedium:
            ++medium;
            break;
        case BotLodLight:
            ++light;
            // no break - see the block above
        default:
            Debug(DebugTypeError, "AI/AIDLL/AI/Main/BotManager.cpp", 1666, "") << "Unhandled lod.";
        }
        m_selectedBots.push_back(info);
    }

    unsigned int planned = 0;
    while (!m_queuePriorityA->empty() && planned++ < m_plannedDecisionMakingInterleave)
    {
        BotInfo* info = m_queuePriorityA->top();
        m_queuePriorityA->pop();
        switch (info->m_bot->getLodLevel())
        {
        case BotLodFull:
            ++full;
            break;
        case BotLodMedium:
            ++medium;
            break;
        case BotLodLight:
            ++light;
            // no break - see the block above
        default:
            Debug(DebugTypeError, "AI/AIDLL/AI/Main/BotManager.cpp", 1690, "") << "Unhandled lod.";
        }
        m_selectedBots.push_back(info);
    }

    if (m_selectedBots.empty())
        return;
    const unsigned int weight =
        full * m_lodLevelFullTicks + medium * m_lodLevelMediumTicks + light * m_lodLevelLightTicks;
    if (!weight)
        return;
    const double perTick = slice / static_cast<double>(weight);
    double botSlice;    // one slot for the whole walk, as in both images
    for (std::vector<BotInfo*>::iterator it = m_selectedBots.begin(); it != m_selectedBots.end(); ++it)
    {
        BotInfo* const info = *it;
        if (!info->m_bot || info->m_bot->isDead())
            continue;
        if (!informationManager->getInformation(info->m_bot->getCurrentEquipment()))
            continue;
        switch (info->m_bot->getLodLevel())
        {
        case BotLodFull:
            botSlice = static_cast<double>(m_lodLevelFullTicks) * perTick;
            break;
        case BotLodMedium:
            botSlice = static_cast<double>(m_lodLevelMediumTicks) * perTick;
            break;
        case BotLodLight:
            botSlice = static_cast<double>(m_lodLevelLightTicks) * perTick;
            break;
        default:
            Debug(DebugTypeError, "AI/AIDLL/AI/Main/BotManager.cpp", 1735, "") << "Unhandled lod.";
        }
        if (botSlice > 0.0)
        {
            const double start = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getUpdateTime();
            info->m_behaviourUpdateCount += info->m_bot->decisionMaking(botSlice);
            const double end = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getUpdateTime();
            ++info->m_word40;
            info->m_double24 = 0.0;
            info->m_double56 += end - start;
        }
        info->m_priority = 0;
        if (static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getUpdateTime() > deadline)
            return;
    }
}

// Uses one tenth of the slice for bot path requests and the remainder for search processing.
void BotManager::actionPathfinding(const double& slice)
{
    IBFTimer* const timer = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer);
    const double now = timer->getUpdateTime();
    const double deadline = now + slice;
    const double botDeadline = now + slice * 0.1;
    const unsigned int size = static_cast<unsigned int>(m_botInfos.size());

    for (unsigned int visited = 0; visited < size; ++visited)
    {
        if (timer->getUpdateTime() > botDeadline)
            break;
        BotInfo* const info = m_botInfos[m_pathfindingCursor];
        Bot* const bot = info->m_bot;
        if (bot && !bot->isDead() && !bot->isDeactivated() && bot->getSquadMember()
            && info->m_bot->getBotNavigation()->isSearchActive()
            && info->m_bot->getBotNavigation()->isSearchNew())
        {
            Information* const equipment =
                informationManager->getInformation(info->m_bot->getCurrentEquipment());
            if (equipment)
            {
                IPIMobile* const mobile = equipment->getMobile();
                if (mobile && mobile->getVehicleNumber() >= 0)
                {
                    bool first, second;    // written by the callee
                    info->m_bot->updatePathfinding(first, &second);
                }
            }
        }
        m_pathfindingCursor = (m_pathfindingCursor + 1) % size;
    }

    IAIPathfinding::instance->updatePathfinding((deadline - botDeadline) * 0.9);

    for (unsigned int i = 0; i < size; ++i)
    {
        BotInfo* const info = m_botInfos[i];
        Bot* const bot = info->m_bot;
        if (bot && !bot->isDead() && !bot->isDeactivated() && bot->getSquadMember()
            && info->m_bot->getBotNavigation()->isSearchActive()
            && info->m_bot->getBotNavigation()->isRunning())
        {
            bool first;            // written by the callee
            info->m_bot->updatePathfinding(first, 0);
        }
    }
}

// Scans occupied world objects for hearing within the supplied slice.
// [FRONTIER] The object-query path remains incomplete.
void BotManager::actionHearing(const double& slice)
{
    const double deadline =
        static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getUpdateTime() + slice;
    IAIObjectEnvironment* const environment =
        static_cast<IAIObjectEnvironment*>(CommonSingletonServer::m_iaiObjectEnvironment);
    m_occupiedObjects.resize(0);
    environment->getWorldPlayerOccupiedObjectsWithIds(m_occupiedObjects, 0);

    std::vector<std::pair<int, IAIObject*> >::iterator next = m_occupiedObjects.begin();
    while (next != m_occupiedObjects.end() && next->first != m_hearingResumeId)
        ++next;
    if (next == m_occupiedObjects.end())
        next = m_occupiedObjects.begin();
    const int count = static_cast<int>(m_occupiedObjects.size());
    const double gameTime = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    for (int visited = 0; visited < count;)
    {
        IAIObject* const object = next->second;
        if (++next == m_occupiedObjects.end())
            next = m_occupiedObjects.begin();
        ++visited;
        // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
        // The body's first call is `object->getSide` (IAIObject ELF +0x12C / PE +0x128), and
        // IAIObject has no declaration in this tree; see the outline above.
        (void)object;
        (void)gameTime;
        (void)deadline;
        BF2_AI_FRONTIER("dice::hfe::ai::BotManager::actionHearing (IAIObject hearing query)");
    }
    if (count > 0)
        m_hearingResumeId = next->first;
}

// Processes at most two sensing updates per tick using the full-detail update rate.
void BotManager::actionSensing(const double& slice)
{
    const double deadline =
        static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getUpdateTime() + slice;
    m_selectedBots.resize(0);
    unsigned int full = 0;
    unsigned int medium = 0;
    unsigned int light = 0;
    unsigned int taken = 0;
    while (!m_queueSensing->empty() && taken <= 1)
    {
        BotInfo* info = m_queueSensing->top();
        m_queueSensing->pop();
        Bot* const bot = info->m_bot;
        if (!informationManager->getInformation(info->m_bot->getCurrentEquipment()))
            continue;
        switch (bot->getLodLevel())
        {
        case BotLodFull:
            ++full;
            break;
        case BotLodMedium:
            ++medium;
            break;
        case BotLodLight:
            ++light;
            break;
        default:
            Debug(DebugTypeError, "AI/AIDLL/AI/Main/BotManager.cpp", 1806, "") << "Unhandled lod.";
        }
        m_selectedBots.push_back(info);
        ++taken;
    }

    const unsigned int weight = m_lodLevelFullTicks * (light + medium + full);
    if (!weight)
        return;
    const double perTick = slice / static_cast<double>(weight);
    for (unsigned int i = 0; i < m_selectedBots.size(); ++i)
    {
        BotInfo* const info = m_selectedBots[i];
        double share;
        switch (info->m_bot->getLodLevel())
        {
        case BotLodFull:
        case BotLodMedium:
        case BotLodLight:
            share = perTick * static_cast<double>(m_lodLevelFullTicks);
            break;
        default:
            share = 0.0;
            Debug(DebugTypeError, "AI/AIDLL/AI/Main/BotManager.cpp", 1840, "") << "Unhandled lod.";
        }
        if (share > 0.0)
        {
            const double start = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getUpdateTime();
            const double seeSlice = share * m_sensingQuotient;
            info->m_bot->see(seeSlice);
            const double senseSlice = share * m_sensingQuotientInverse;
            info->m_bot->updateSensing(senseSlice);
            const double end = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getUpdateTime();
            ++info->m_word44;
            info->m_sensingPriority = 0;
            info->m_double64 += end - start;
        }
        if (static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getUpdateTime() > deadline)
            break;
    }
}


// Creates bot storage and scheduling queues.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding BotManager operation.
BotManager::BotManager()
    // The IBotManager and IBotManagerModifier bases construct their own interface subobjects.
    : m_queuePriorityA(new BotQueue)
    , m_queuePriorityB(new BotQueue)
    , m_queueSensing(new SensingQueue)
    , m_comAgentCursor(0)
    // cached, not looked UP later. Every later use of the AI main goes through this copy, so a
    // BotManager built before an AIMain exists caches a null and never notices.
    , m_aiMain(IAIMain::m_instance)
    , m_hearingResumeId(0)
    , m_ptr192(0)
    , m_ptr196(0)
    , m_ptr200(0)
    // m_selectedBots: the three zero stores at +216..+224 are its default construction.
    , m_lastCreatedBot(0)
    , m_float232(0.3f)
    , m_flag236(false)
    // m_occupiedObjects: the three zero stores at +240..+248 are its default construction.
    , m_navPointCursor(0)
    , m_planHalfToggle(true)
    , m_planCursor(0)
    , m_pathfindingCursor(0)
{
    // Seeds one update of each type 0..4; action rotates through this five-entry schedule.
    for (unsigned i = 0; i <= 4; ++i)
        m_updates.insert(m_updates.end(), BMUpdate(static_cast<int>(i)));

    // Applies defaults through setters, preserving threshold absolute values and normalized
    // budget weights. setSystemQuotient stores only its first two shares.
    setLodLevelTicks(6, 6, 6);
    setLodLevelPriority(3, 3, 3);

    // by const reference, so the literals need somewhere to live. Retail spills six doubles to
    // the stack and passes their addresses; this is the same thing said in source.
    const double plannedFull   = 0.5;
    const double plannedMedium = 0.5;
    const double plannedLight  = 0.5;
    setPlannedDecisionMakingThreshold(plannedFull, plannedMedium, plannedLight);

    // not three equal values, unlike the planned set one line above: 0.3 for full detail and 0.4
    // for both reduced tiers. An unplanned decision is made more readily by a distant bot.
    const double unplannedFull   = 0.3;
    const double unplannedMedium = 0.4;
    const double unplannedLight  = 0.4;
    setUnplannedDecisionMakingThreshold(unplannedFull, unplannedMedium, unplannedLight);

    setDecisionMakingInterleave(2, 2);
    setSensingQuotient(1, 1);
    setSystemQuotient(30, 40, 30);

    // Preallocates the bot pool. A maximum of -1 disables preallocation.
    // [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding BotManager operation.
    IAISettings* const settings = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
    if (settings->getMaxNBots() != -1)
    {
        m_botInfos.reserve(settings->getMaxNBots());

        // Re-queries the maximum bot count on every iteration, independently of the reserve call.
        for (int i = 0; i < settings->getMaxNBots(); ++i)
        {
            // `getNumberOfBehaviours` is also called twice per bot - once to size the array and
            // once as the merger's count - rather than stored. Two calls, same value, reproduced.
            UrgencyGenerator** const generators =
                new UrgencyGenerator*[settings->getNumberOfBehaviours()];

            // Creates one merger per bot with no weight array. Generator-array and merger allocations
            // are unchecked; the following bot objects use checkPointer.
            UrgencyMerger* const merger =
                new UrgencyMerger(settings->getNumberOfBehaviours(), 0, generators);

            BotMemory* const memory = checkPointer(new BotMemory());

            // Preallocates every bot with no player or order; admission later fills these stable slots.
            // [UNRESOLVED] The remaining literal constructor settings have unknown meanings.
            BotMain* const bot = checkPointer(
                new BotMain(0, static_cast<unsigned int>(i), 0, 1.0f, 1, 0, memory, 0, 0,
                            merger, generators));

            m_bots.insert(m_bots.end(), bot);
        }
    }

    // Creates 256 player-to-bot entries with -1 meaning no assigned bot.
    m_botByPlayerId.resize(256, -1);
}

// These configuration methods remain virtual through IBotManagerModifier.

void BotManager::setLodLevelTicks(unsigned full, unsigned medium, unsigned light)
{
    m_lodLevelFullTicks   = full;
    m_lodLevelMediumTicks = medium;
    m_lodLevelLightTicks  = light;
}

void BotManager::setLodLevelPriority(unsigned full, unsigned medium, unsigned light)
{
    m_lodLevelFullPriority   = full;
    m_lodLevelMediumPriority = medium;
    m_lodLevelLightPriority  = light;
}

// Stores the absolute value of each decision-making threshold.
void BotManager::setPlannedDecisionMakingThreshold(const double& full,
                                                   const double& medium,
                                                   const double& light)
{
    m_plannedDecisionMakingThresholdFull   = full   < 0.0 ? -full   : full;
    m_plannedDecisionMakingThresholdMedium = medium < 0.0 ? -medium : medium;
    m_plannedDecisionMakingThresholdLight  = light  < 0.0 ? -light  : light;
}

// The same fabs treatment, and note the third test is written the other way round in the
// binary (`>= 0.0` selecting the positive rather than `< 0.0` selecting the negation). Identical
// for every value including -0.0; recorded, not reproduced as a separate shape.
void BotManager::setUnplannedDecisionMakingThreshold(const double& full,
                                                     const double& medium,
                                                     const double& light)
{
    m_unplannedDecisionMakingThresholdFull   = full   < 0.0 ? -full   : full;
    m_unplannedDecisionMakingThresholdMedium = medium < 0.0 ? -medium : medium;
    m_unplannedDecisionMakingThresholdLight  = light  < 0.0 ? -light  : light;
}

void BotManager::setDecisionMakingInterleave(unsigned planned, unsigned unplanned)
{
    m_plannedDecisionMakingInterleave   = planned;
    m_unplannedDecisionMakingInterleave = unplanned;
}

// Normalizes two integer weights into complementary shares. Leaves both unchanged
// when their unsigned sum is zero.
void BotManager::setSensingQuotient(unsigned a, unsigned b)
{
    if (a + b)
    {
        const double quotient = static_cast<double>(a) / static_cast<double>(a + b);
        m_sensingQuotient        = quotient;
        m_sensingQuotientInverse = 1.0 - quotient;
    }
}

// Normalizes three weights but stores only the first two fractions.
// Leaves existing shares unchanged when the unsigned total is zero.
void BotManager::setSystemQuotient(unsigned a, unsigned b, unsigned c)
{
    const unsigned total = a + b + c;
    if (total)
    {
        m_systemDecisionMakingQuotient = static_cast<double>(a) / static_cast<double>(total);
        m_systemQuotient2              = static_cast<double>(b) / static_cast<double>(total);
    }
}

// [FRONTIER] Active-bot counting requires the remaining state predicates.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding BotManager operation.

// [FRONTIER] Bot-manager destruction remains unimplemented.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Replace the stop when owned bot and scheduler teardown is recovered.
BotManager::~BotManager()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::~BotManager");
}

// [FRONTIER] Remaining bot-manager virtual operations stop visibly when reached.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Replace each stop when its corresponding behavior is recovered.

// - the vector's byte span shifted right by two, i.e. its element count.
// It counts admitted records, including deactivated bots. The constructor only reserves capacity;
// addUnspawnedBot appends a record after consuming or allocating a BotMain.
int BotManager::getBotCount() const
{
    return static_cast<int>(m_botInfos.size());
}

// Reports the bot counts from the managed roster.
int BotManager::getNumberOfActiveBots() const
{
    return static_cast<int>(m_botInfos.size()) - static_cast<int>(m_deactivatedBots.size());
}

int BotManager::getNumberOfDeactivedBots() const
{
    return static_cast<int>(m_deactivatedBots.size());
}

// Returns the bot at index. The caller must supply a valid index and a live record.
Bot* BotManager::getBot(unsigned int index) const
{
    return m_botInfos[index]->m_bot;
}

// Bot-ID lookup checks bounds; player lookup requires a valid association.
Bot* BotManager::getBotFromId(unsigned int id) const
{
    if (id < m_botInfos.size())
    {
        BotInfo* const info = m_botInfos[id];

        if (info)
            return info->m_bot;
    }

    return 0;
}

// no bounds check on `playerId` - see the block above.
Bot* BotManager::getBotFromPlayerId(int playerId) const
{
    if (m_botByPlayerId[playerId] == -1)
        return 0;

    // the same element, read A second time.
    return getBotFromId(static_cast<unsigned int>(m_botByPlayerId[playerId]));
}

Bot* BotManager::getBotFromName(const std::string&) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getBotFromName");
}

// BotManager::BotInfo - inlined in ELF addUnspawnedBot at ;
// PE standalone constructor . Only these fields are initialized. In particular
// m_behaviourUpdateCount and m_attackerIndex retain their allocation contents, as do alignment
// bytes.
BotManager::BotInfo::BotInfo(Bot* bot)
    : m_bot(bot), m_word4(0), m_priority(0), m_word12(0), m_sensingPriority(0),
      m_flag20(false), m_double24(0.0), m_word36(0), m_word40(0), m_word44(0),
      m_double48(0.0), m_double56(0.0), m_double64(0.0), m_flag72(true),
      m_flag73(false), m_double76(0.0), m_damage(0.0f)
{
}

// Admits an unspawned player using the first available preallocated bot.
bool BotManager::addUnspawnedBot(int playerId, int side, float* personality)
{
    m_lastCreatedBot = 0;
    const int count = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings)
                          ->getNumberOfBehaviours();
    BotMain* bot;

    if (m_bots.empty())
    {
        UrgencyGenerator** const generators = checkPointer(new UrgencyGenerator*[count]);
        UrgencyMerger* const merger = checkPointer(new UrgencyMerger(count, personality, generators));
        float* const weights = checkPointer(new float[count]);
        for (int i = 0; i < count; ++i)
            weights[i] = 1.0f;
        BotOrder* const order = checkPointer(new BotOrder(weights, static_cast<unsigned char>(
            BotBehaviourManager::m_instance->getBehaviourId("Idle"))));
        IAIPlayer* const player = static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)
                                     ->getPlayerFromId(playerId);
        // BotMain storage is allocated before this nested BotMemory construction in both images.
        // Unlike the preallocation constructor, this path does not checkPointer the memory.
        bot = checkPointer(new BotMain(player, static_cast<unsigned int>(m_botInfos.size()),
            personality, 1.0f, 1, order, new BotMemory(), 0, side, merger, generators));
    }
    else
    {
        bot = static_cast<BotMain*>(m_bots.front());
        bot->setPlayer(static_cast<IAIPlayerFactory*>(CommonSingletonServer::m_iaiPlayerFactory)
                           ->getPlayerFromId(playerId));
        bot->setSide(side);
        bot->m_basicWeights = personality;
        float* const weights = checkPointer(new float[count]);
        for (int i = 0; i < count; ++i)
            weights[i] = 1.0f;
        BotOrder* const order = checkPointer(new BotOrder(weights, static_cast<unsigned char>(
            BotBehaviourManager::m_instance->getBehaviourId("Idle"))));
        bot->recieveOrder(order);
        // Reload begin after the virtual calls, as retail does.
        m_bots.erase(m_bots.begin());
    }

    m_botInfos.push_back(checkPointer(new BotInfo(bot)));
    if (static_cast<int>(m_botByPlayerId.size()) < playerId)
        m_botByPlayerId.resize(playerId, -1);
    m_botByPlayerId[playerId] = static_cast<int>(bot->getBotId());
    m_lastCreatedBot = bot;
    return true;
}

// Reactivates a retired bot for a player and restores it to the active roster.
bool BotManager::reactivateBot(int playerId, int side, const std::string& name)
{
    if (m_deactivatedBots.empty())
        return false;

    // A second, redundant test. See the block above.
    if (m_deactivatedBots.size() == 0)
        return false;

    const std::list<unsigned int>::iterator oldest = m_deactivatedBots.begin();
    const unsigned int botId = *oldest;

    Bot* const bot = m_botInfos[botId]->m_bot;

    bot->getPlayer()->setPlayer(playerId);
    bot->setSide(side);
    bot->setName(name);
    bot->setAsDeactivated(false);

    // off by one - see the block above.
    if (static_cast<int>(m_botByPlayerId.size()) < playerId)
        m_botByPlayerId.resize(playerId, -1);

    m_botByPlayerId[playerId] = static_cast<int>(botId);

    IAIMain::m_instance->getSAI(bot->getSide())->addBot(bot);

    m_deactivatedBots.erase(oldest);

    return true;
}

Bot* BotManager::getBot(IAIObject*) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getBot");
}

Bot* BotManager::getLastCreatedBot() const
{
    // return [this+228]. PE uses the corresponding member at +276.
    return m_lastCreatedBot;
}

void BotManager::updateBotVehicle(Bot*, IAIObject*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::updateBotVehicle");
}

void BotManager::setBotChangedVehicle(int, IAIObject*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::setBotChangedVehicle");
}

void BotManager::setBotIsDead(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::setBotIsDead");
}

void BotManager::setBotIsAlive(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::setBotIsAlive");
}

void BotManager::setBotIsManDown(int, IAIObject*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::setBotIsManDown");
}

void BotManager::setBotIsRevived(int, IAIObject*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::setBotIsRevived");
}

void BotManager::setBotReceivedRadioMessage(const AIRadioMessageSettings&, int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::setBotReceivedRadioMessage");
}

void BotManager::setBotRecievedAttackCommanderOrder(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::setBotRecievedAttackCommanderOrder");
}

void BotManager::setBotRecievedDefendCommanderOrder(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::setBotRecievedDefendCommanderOrder");
}

void BotManager::setBotRecievedMoveCommanderOrder(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::setBotRecievedMoveCommanderOrder");
}

void BotManager::setBotRecievedAttackSquadOrder(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::setBotRecievedAttackSquadOrder");
}

void BotManager::setBotRecievedDefendSquadOrder(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::setBotRecievedDefendSquadOrder");
}

void BotManager::setBotRecievedMoveSquadOrder(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::setBotRecievedMoveSquadOrder");
}

void BotManager::setBotTeamKilled(int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::setBotTeamKilled");
}

void BotManager::registerDamageOnBot(float, int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::registerDamageOnBot");
}

float BotManager::getVehicleBailValue()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getVehicleBailValue");
}

void BotManager::removeMineTarget(unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::removeMineTarget");
}

bool BotManager::reset()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::reset");
}

void BotManager::setRenderBotSensing(bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::setRenderBotSensing");
}

bool BotManager::getRenderBotSensing() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getRenderBotSensing");
}

void BotManager::writeBotPlans()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::writeBotPlans");
}

void BotManager::increaseTabLevel()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::increaseTabLevel");
}

void BotManager::decreaseTabLevel()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::decreaseTabLevel");
}

void BotManager::tabulate(std::ostream&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::tabulate");
}

unsigned BotManager::getLodLevelFullTicks() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getLodLevelFullTicks");
}

unsigned BotManager::getLodLevelMediumTicks() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getLodLevelMediumTicks");
}

unsigned BotManager::getLodLevelLightTicks() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getLodLevelLightTicks");
}

unsigned BotManager::getLodLevelFullPriority() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getLodLevelFullPriority");
}

unsigned BotManager::getLodLevelMediumPriority() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getLodLevelMediumPriority");
}

unsigned BotManager::getLodLevelLightPriority() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getLodLevelLightPriority");
}

double BotManager::getPlannedDecisionMakingThresholdFull() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getPlannedDecisionMakingThresholdFull");
}

double BotManager::getPlannedDecisionMakingThresholdMedium() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getPlannedDecisionMakingThresholdMedium");
}

double BotManager::getPlannedDecisionMakingThresholdLight() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getPlannedDecisionMakingThresholdLight");
}

double BotManager::getUnplannedDecisionMakingThresholdFull() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getUnplannedDecisionMakingThresholdFull");
}

double BotManager::getUnplannedDecisionMakingThresholdMedium() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getUnplannedDecisionMakingThresholdMedium");
}

double BotManager::getUnplannedDecisionMakingThresholdLight() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getUnplannedDecisionMakingThresholdLight");
}

unsigned BotManager::getPlannedDecisionMakingInterleave() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getPlannedDecisionMakingInterleave");
}

unsigned BotManager::getUnplannedDecisionMakingInterleave() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getUnplannedDecisionMakingInterleave");
}

double BotManager::getSensingQuotient() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getSensingQuotient");
}

double BotManager::getMemoryUpdateQuotient() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getMemoryUpdateQuotient");
}

double BotManager::getSystemDecisionMakingQuotient() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getSystemDecisionMakingQuotient");
}

double BotManager::getSystemPathfindingQuotient() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getSystemPathfindingQuotient");
}

double BotManager::getSystemSensingQuotient() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getSystemSensingQuotient");
}

void BotManager::resetStatistics()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::resetStatistics");
}

void BotManager::resetStatistics(unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::resetStatistics");
}

unsigned BotManager::getPlanExecutionUpdateCount(unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getPlanExecutionUpdateCount");
}

unsigned BotManager::getDecisionMakingUpdateCount(unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getDecisionMakingUpdateCount");
}

unsigned BotManager::getSensingUpdateCount(unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getSensingUpdateCount");
}

unsigned BotManager::getDecisionMakingBehaviourUpdateCount(unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getDecisionMakingBehaviourUpdateCount");
}

double BotManager::getPlanExecutionRunTime(unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getPlanExecutionRunTime");
}

double BotManager::getDecisionMakingRunTime(unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getDecisionMakingRunTime");
}

double BotManager::getSensingRunTime(unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getSensingRunTime");
}

bool BotManager::isPathfinding(unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::isPathfinding");
}

bool BotManager::getLastPathfindingSucceeded(unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getLastPathfindingSucceeded");
}

bool BotManager::getPathfindingStall(unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getPathfindingStall");
}

double BotManager::getLastPathfindingRunTime(unsigned int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::getLastPathfindingRunTime");
}

void BotManager::enableBotScriptDebugger(bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotManager::enableBotScriptDebugger");
}

} // namespace ai
} // namespace hfe
} // namespace dice
