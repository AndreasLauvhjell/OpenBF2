// Strategic simulation and supporting statistics.
// [FRONTIER] Remaining strategic phases and object-query paths stop by name.
// [RETIRE-WHEN: aidll-02-skeleton] Implement and validate the remaining strategic operations.

#include "SAI.h"

#include <string>

#include "../Main/AIDllFrontier.h"
#include "../Main/AISettingsInterfaces.h"                  // IAISettings::getMaxNBots
#include "../Main/BotBehaviourManager.h"                   // getBehaviourId, by name
#include "../Main/CommonSingletonServer.h"
#include "../AIKnowledge/BFInformationEnvironment.h"
#include "../AIKnowledge/Information.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/IPIPhysical.h"
#include "../AIKnowledge/IPIUnit.h"
#include "../AIKnowledge/UnitPredicate.h"
#include "../AIBot/BotCallBack.h"
#include "../AIBot/BotMain.h"
#include "../AIBot/BotSquad.h"
#include "../AIBot/SquadManager.h"
#include "../AIStrategicEntity/AIStrategicObject.h"
#include "../AIStrategicEntity/AIStrategicObjectManager.h"
#include "../AIStrategicMap/BotSpawner.h"
#include "../AIUtils/checkPointer.h"
#include "../../../AIInterface/BFKnowledge/BFCommander.h"  // request-denial callbacks
#include "../../../AIInterface/BFKnowledge/BFGame.h"       // IAIGame counters
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"     // IAIPlayer::setSpawnGroup
#include "../../../AIInterface/BFKnowledge/BFTimer.h"      // IBFTimer::getGameTime
#include "../../../../Core/Debug.h"                      // update's shipped invalid-state diagnostic

namespace dice {
namespace hfe {
namespace ai {

// The concrete callback names and signatures are ELF-attested. Their source-file placement here
// is [INFERRED]; only SAI.cpp is attested for this part of the strategic subsystem.
class SAIBotDCB : public IBotDeathCallBack
{
public:
    // The SAI pointer is non-owning, at +8 on both ports.
    SAIBotDCB(Bot* bot, ISAI* sai) : IBotDeathCallBack(bot), m_sai(sai) {}
    // PE deleting destructor : only base destruction.
    virtual ~SAIBotDCB() {}

    // Neither pointer is guarded.
    virtual void reportDeath()
    {
        m_sai->setBotIsDead(m_bot->getBotId());
    }

private:
    ISAI* m_sai;
};

class SAIBotSCB : public IBotSpawnCallBack
{
public:
    SAIBotSCB(Bot* bot, ISAI* sai) : IBotSpawnCallBack(bot), m_sai(sai) {}
    // PE deleting destructor : only base destruction.
    virtual ~SAIBotSCB() {}

    // Preserve the repeated virtual squad lookups and priority.
    // setBotIsAlive and the three recieve* callees retain their own named frontiers.
    virtual void reportSpawn()
    {
        m_sai->setBotIsAlive(m_bot->getBotId());
        if (m_bot->getBotSquad()->hasAttackOrder())
            m_bot->getBotSquad()->recieveAttackSquadOrder();
        else if (m_bot->getBotSquad()->hasDefendOrder())
            m_bot->getBotSquad()->recieveDefendSquadOrder();
        else if (m_bot->getBotSquad()->hasMoveOrderOrder())
            m_bot->getBotSquad()->recieveMoveSquadOrder();
    }

private:
    ISAI* m_sai;
};

static_assert(sizeof(void*) != 4 || sizeof(SAIBotDCB) == 12, "SAIBotDCB x86 layout");
static_assert(sizeof(void*) != 4 || sizeof(SAIBotSCB) == 12, "SAIBotSCB x86 layout");

// Running weighted statistics across size buckets. Its two array allocations are unchecked.
EnemyStatistics::EnemyStatistics(unsigned int size)
    : m_size(size)
    , m_weight(1.0f)
    , m_samples(0)
{
    m_strength = new float[size];
    m_count    = new float[m_size];

    for (unsigned int i = 0; i < m_size; ++i)
    {
        m_strength[i] = 0.0f;
        m_count[i]    = 0.0f;
    }
}

EnemyStatistics::EnemyStatistics(unsigned int size, float weight)
    : m_size(size)
    , m_weight(weight)
    , m_samples(0)
{
    m_strength = new float[size];
    m_count    = new float[m_size];

    for (unsigned int i = 0; i < m_size; ++i)
    {
        m_strength[i] = 0.0f;
        m_count[i]    = 0.0f;
    }
}

EnemyStatistics::~EnemyStatistics()
{
    if (m_strength)
        delete[] m_strength;

    if (m_count)
        delete[] m_count;
}

// Updates each bucket with (n * old + weight * observation) / (n + weight), using the
// entry sample count for every bucket. Integer counts are accumulated as floats.
void EnemyStatistics::sample(float* strength, int* count)
{
    const float samples = static_cast<float>(m_samples);
    const float total   = samples + m_weight;

    for (unsigned int i = 0; i < m_size; ++i)
    {
        m_strength[i] = samples / total * m_strength[i];
        m_strength[i] = m_weight / total * strength[i] + m_strength[i];

        m_count[i] = static_cast<float>(m_samples) / total * m_count[i];
        m_count[i] = m_weight / total * static_cast<float>(count[i]) + m_count[i];
    }

    ++m_samples;
}

// both of these walk this object'S size, not the other's. Copying from a differently-sized
// statistics object reads past its arrays. Shipped; no check here either.
void EnemyStatistics::copy(EnemyStatistics& other)
{
    for (unsigned int i = 0; i < m_size; ++i)
    {
        m_strength[i] = other.m_strength[i];
        m_count[i]    = other.m_count[i];
    }

    m_samples = other.m_samples;
}

void EnemyStatistics::add(EnemyStatistics& other)
{
    for (unsigned int i = 0; i < m_size; ++i)
    {
        m_strength[i] += other.m_strength[i];
        m_count[i]    += other.m_count[i];
    }

    m_samples += other.m_samples;
}

void EnemyStatistics::clear()
{
    for (unsigned int i = 0; i < m_size; ++i)
    {
        m_strength[i] = 0.0f;
        m_count[i]    = 0.0f;
    }

    m_samples = 0;
}

// CommanderInterface - one byte, no state, six orders. The constructor and destructor are five
// bytes each: a return.
CommanderInterface::CommanderInterface()
{
}

CommanderInterface::~CommanderInterface()
{
}


void CommanderInterface::giveAttackOrder(const Vec3&, SquadCoordinator*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommanderInterface::giveAttackOrder (SquadCoordinator)");
}

void CommanderInterface::giveMoveOrder(const Vec3&, SquadCoordinator*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommanderInterface::giveMoveOrder (SquadCoordinator)");
}

void CommanderInterface::giveDefendOrder(const Vec3&, SquadCoordinator*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommanderInterface::giveDefendOrder (SquadCoordinator)");
}

void CommanderInterface::giveAttackOrder(const Vec3&, Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommanderInterface::giveAttackOrder (Bot)");
}

void CommanderInterface::giveMoveOrder(const Vec3&, Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommanderInterface::giveMoveOrder (Bot)");
}

void CommanderInterface::giveDefendOrder(const Vec3&, Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::CommanderInterface::giveDefendOrder (Bot)");
}


// ISAI - the interface. Its destructor is : a vptr store and a return, like every other
// bare interface destructor in this subsystem. No registration and no roster - `ISAI` is not a
// singleton, it is a per-side object `AIMain` owns by pointer.
ISAI::~ISAI()
{
}

// the unwind path the
// constructor's possible throw needs is the other hundred bytes.
SAI* ISAI::create(int side, int gridWidth, int gridHeight, Vec2 min, Vec2 max)
{
    return new SAI(side, gridWidth, gridHeight, min, max);
}

// Starts the three integer states at -1 and the float accumulator at zero.
SAI::BotInfo::BotInfo(Bot* bot)
    : m_bot(bot)
    , m_int04(-1)
    , m_int08(-1)
    , m_int0C(-1)
    , m_float10(0.0f)
    , m_double14(0.0)
{
}

// Clears both 44-element state arrays, then sets each final normalization slot to one.
void SAI::clearStates(float* a, float* b)
{
    for (int i = 0; i <= 43; ++i)
    {
        a[i] = 0.0f;
        b[i] = 0.0f;
    }

    a[43] = 1.0f;
    b[43] = 1.0f;
}

// Creates strategic state for side. Grid dimensions and bounds are retained in the signature but unused.
SAI::SAI(int side, int, int, Vec2, Vec2)
    : m_flagAC(true)
    , m_behaviourIdle(0)
    , m_behaviourMoveTo(0)
    , m_behaviourAvoid(0)
    , m_behaviourFire(0)
    , m_intD8(0)
    , m_intDC(0)
    , m_intE0(0)
    // The strategy cursor starts at `m_strategies.end`, which on a freshly built empty vector is
    // the null `finish` pointer. The binary copies the vector's finish word into it.
    , m_activeStrategy(0)
    , m_enemyStats(0)
    , m_friendlyStats(0)
    , m_int18C(19)
    , m_float190(-992.0f)
    , m_side(side)
    , m_sideMask(1 << side)
    // the other side'S BIT, and the expression only has two answers - this is a two-sided game.
    , m_enemyMask((side == 1) ? 4 : 2)
    , m_flag1C0(false)
    , m_flag1C1(false)
    , m_flag1CC(false)
    , m_flag1CD(false)
    , m_flag1CE(true)
    , m_flag1CF(false)
    , m_flag1D0(true)
    , m_flag1D1(false)
    , m_strategicObjectIterator()
    , m_stateA(0)
    , m_stateB(0)
    , m_stateC(0)
    , m_stateD(0)
    , m_int1F4(0)
    , m_commander(0)
{
    m_commander = checkPointer(new CommanderInterface());

    m_behaviourIdle   = BotBehaviourManager::m_instance->getBehaviourId(std::string("Idle"));
    m_behaviourMoveTo = BotBehaviourManager::m_instance->getBehaviourId(std::string("MoveTo"));
    m_behaviourAvoid  = BotBehaviourManager::m_instance->getBehaviourId(std::string("Avoid"));
    m_behaviourFire   = BotBehaviourManager::m_instance->getBehaviourId(std::string("Fire"));

    // four deadlines off one clock read. Three at a minute out and one at ten seconds, with the
    // fifth left at zero - so three slow reviews and one fast one, all stamped at construction.
    const double now =
        static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();

    m_time80 = now + 10.0;
    m_time68 = now + 60.0;
    m_time70 = now + 60.0;
    m_time78 = now + 60.0;
    m_time88 = 0.0;

    m_stateA = checkPointer(new float[44]);
    m_stateB = checkPointer(new float[44]);
    m_stateC = checkPointer(new float[44]);
    m_stateD = checkPointer(new float[44]);

    // the pairing is (A, C) and (B, D), not (A) and (C, D).
    clearStates(m_stateA, m_stateC);
    clearStates(m_stateB, m_stateD);

    // Not checkPointer'ed, unlike everything above.
    m_enemyStats    = new EnemyStatistics(6);
    m_friendlyStats = new EnemyStatistics(6);

    for (int i = 0; i <= 5; ++i)
    {
        m_enemyStrengths[i] = 0.0f;
        m_enemyCounts[i] = 0.0f;
        m_friendlyStrengths[i] = 0.0f;
        m_friendlyCounts[i] = 0.0f;
    }

    const IAISettings* const settings =
        static_cast<const IAISettings*>(CommonSingletonServer::m_iaiSettings);

    m_botInfo.resize(settings->getMaxNBots());

    for (int i = 0; i < settings->getMaxNBots(); ++i)
        m_botInfo[i] = 0;
}

// [FRONTIER] Strategy and distribution operations remain incomplete.
// [FRONTIER] Destruction stops until ordered teardown is implemented.
// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// Replace each stop when its corresponding behavior is recovered.
SAI::~SAI()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::~SAI");
}


// Map-dimension setters intentionally ignore their arguments, matching the shipped bodies.
void SAI::setMapXDimension(int)
{
}

void SAI::setMapYDimension(int)
{
}


void SAI::addStrategy(Strategy*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::addStrategy");
}

// One resumable pass, with a budget check after each
// advancing stage. Equality and NaN pass the budget check; state 4's frequency gate instead
// requires an ordered strict greater-than. The prefix always runs, even for exhausted budgets.
// State 5 and 16..18 are holes. The scheduled helpers retain their own named frontiers.
void SAI::update(double budget)
{
    IBFTimer* const timer = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer);
    const double start = timer->getUpdateTime();
    updateRequestDenyTimers();

    for (std::vector<BotInfo*>::iterator it = m_botInfo.begin(); it != m_botInfo.end(); ++it)
    {
        if (*it)
        {
#if defined(_MSC_VER)
            // [PLATFORM] PE reloads the entry after the timer callback and does not round
            // the interval to float before adding it to the stored float.
            const double increase = timer->getTimeIncrease();
            BotInfo* const info = *it;
            info->m_float10 = static_cast<float>(info->m_float10 + increase);
#else
            BotInfo* const info = *it;
            const float increase = static_cast<float>(timer->getTimeIncrease());
            info->m_float10 += increase;
#endif
        }
    }

    switch (m_int18C)
    {
    case 19:
        m_int18C = 0;
        // fall through
    case 0:
        m_int18C = 1;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 1:
        updateStrengths();
        m_int18C = 2;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 2:
    {
        const bool next = !m_int1F4;
        clearStates(next ? m_stateB : m_stateA, next ? m_stateD : m_stateC);
        m_int18C = 3;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    }
    case 3:
    {
        const bool next = !m_int1F4;
        updateStates(next ? m_stateB : m_stateA, next ? m_stateD : m_stateC);
        m_int18C = 4;
        m_int1F4 = next;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    }
    case 4:
    {
#if defined(_MSC_VER)
        // [PLATFORM] PE retains settings across getGameTime; ELF reloads it afterward.
        const IAISettings* const settings =
            static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
        const double gameTime = timer->getGameTime();
        const double elapsed = gameTime - m_float190;
#else
        const double gameTime = timer->getGameTime();
        const double elapsed = gameTime - m_float190;
        const IAISettings* const settings =
            static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
#endif
        if (!(elapsed > settings->getSAIUpdateFrequency()))
            return;
        prepareUpdate();
        m_int18C = 6;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    }
    case 6:
        if (!m_flag1D1)
        {
            if (!updateStrategicObjects())
                return;
            checkCommanderUseArtillery();
        }
        m_int18C = 7;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 7:
        if (!m_flag1D1)
            chooseStrategy();
        m_int18C = 8;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 8:
        modifyStrategicObjectsTemperatures();
        m_int18C = 9;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 9:
        updateBotPositions();
        m_int18C = 10;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 10:
        if (!m_flag1D1)
            updateDistribution();
        m_int18C = 11;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 11:
        if (!m_flag1D1)
            distributeResources();
        if (!haveAnyOwnedCps(m_side))
        {
            m_int18C = 15;
            return;
        }
        m_int18C = 12;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 12:
        if (!m_flag1D1)
            distributeDefence();
        m_int18C = 13;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 13:
        if (!m_flag1D1)
            distributeAttack();
        m_int18C = 14;
        if (timer->getUpdateTime() - start > budget * 0.95)
            return;
        // fall through
    case 14:
        // The ELF calls this empty direct helper; PE eliminates both the test and call.
        if (!m_flag1D1)
            distributeArtillery();
        m_int18C = 19;
        timer->getUpdateTime(); // Shipped final sample, discarded without another budget check.
        return;
    case 15:
        if (!m_flag1D1)
            distributeWithNoAreas();
        m_int18C = 19;
        timer->getUpdateTime();
        return;
    default:
        Debug(DebugTypeError, "AI/AIDLL/AI/AIStrategic/SAI.cpp", 1044, "")
            << "Unhadled case in SAI::Update()."; // Preserve the shipped spelling and source line.
        return;
    }
}

// Null bots do nothing; dead bots first receive (-1,-1,13).
// Squad registration is now a typed call; its narrower relocation/formation frontiers remain
// visible in the owning bodies. Only a completed registration reaches the publication tail.
void SAI::addBot(Bot* bot)
{
    if (!bot)
        return;

    if (bot->isDead())
        bot->setSpawnTarget(-1, -1, 13);

    SquadManager::m_instance->addSquadMember(bot->getPlayerId(), m_side);

    // Recovered tail, reached only after squad registration returns.
    // The vector is ELF +0x94 / PE +0xAC; port-specific STL layouts must not be copied as offsets.
    m_vector94.push_back(bot);
    bot->setDeathCallBack(checkPointer(new SAIBotDCB(bot, this)));
    bot->setSpawnCallBack(checkPointer(new SAIBotSCB(bot, this)));
}

void SAI::setBotIsDead(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::setBotIsDead");
}

void SAI::setBotIsAlive(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::setBotIsAlive");
}

void SAI::removeBot(Bot*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::removeBot");
}

// Both test the requested bit; x86 masks shift count.
bool SAI::isEnemy(int side) const
{
    return (static_cast<unsigned int>(m_enemyMask) & (1u << (side & 31))) != 0;
}

// the same bit test on the friendly mask.
bool SAI::isFriend(int side) const
{
    return (static_cast<unsigned int>(m_sideMask) & (1u << (side & 31))) != 0;
}

float* SAI::getEnemyStrengths(int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getEnemyStrengths");
}

int* SAI::getEnemyTypes(int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getEnemyTypes");
}

float* SAI::getFriendlyStrengths(int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getFriendlyStrengths");
}

int* SAI::getFriendlyTypes(int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getFriendlyTypes");
}

void SAI::setBotHasChangedEquipment(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::setBotHasChangedEquipment");
}

// find, update the second member of the existing pair, or do nothing.
// Missing bot IDs are not inserted. The first member of the pair is left untouched.
void SAI::setNewBotKit(int botId, int kit)
{
    std::map<int, std::pair<int, int> >::iterator it = m_map5C.find(botId);
    if (it != m_map5C.end())
        it->second.second = kit;
}

// return the member map's address
// at +0x5C / +0x68. The caller copies the map; this getter neither copies nor mutates it.
// [INFERRED] const-reference spelling, as documented on the shared declaration.
const std::map<int, std::pair<int, int> >& SAI::getDeadBots() const
{
    return m_map5C;
}

void SAI::setPlayerIsCommander(bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::setPlayerIsCommander");
}

// byte getter at +0x1D1 / +0x221.
bool SAI::isPlayerCommander()
{
    return m_flag1D1;
}

bool SAI::playerRequestsArtillery(int, Vec3)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::playerRequestsArtillery");
}

bool SAI::playerRequestsUav(int, Vec3)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::playerRequestsUav");
}

bool SAI::playerRequestsSupply(int, Vec3)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::playerRequestsSupply");
}

bool SAI::playerRequestsVehicleDrop(int, Vec3)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::playerRequestsVehicleDrop");
}

void SAI::updateRequestDenyTimers()
{
    // Cancellation deliberately retains the shipped
    // invalid-iterator defect: erases, then reads the erased
    // node; frees it, then reads [edi]. A nonempty queue
    // therefore makes the following ++it unsafe. Do not turn this into erase(it++).
    if (m_flag1D1)
    {
        for (auto it = m_denyRequests.begin(); it != m_denyRequests.end(); ++it)
        {
            ::operator delete(*it);
            m_denyRequests.erase(it);
        }
        return;
    }

    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    for (auto it = m_denyRequests.begin(); it != m_denyRequests.end(); )
    {
        // The comparison is ordered and strict: equality and NaN retain the request.
        if (now > (*it)->m_deadline)
        {
            switch ((*it)->m_type)
            {
            case 0:
                static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)->
                    denySupplyRequest(m_side, (*it)->m_playerId);
                break;
            case 1:
                static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)->
                    denyArtilleryRequest(m_side, (*it)->m_playerId);
                break;
            case 2:
                static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)->
                    ArtilleryUnavailable(m_side, (*it)->m_playerId);
                break;
            case 3:
                static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)->
                    denyUavRequest(m_side, (*it)->m_playerId);
                break;
            case 4:
                static_cast<IBFCommander*>(CommonSingletonServer::m_iBFCommander)->
                    denyVehicleDropRequest(m_side, (*it)->m_playerId);
                break;
            }
            // Both ports reload the payload after the callback, then advance before erase.
            ::operator delete(*it);
            m_denyRequests.erase(it++);
        }
        else
            ++it;
    }
}

void SAI::checkCommanderUseArtillery()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::checkCommanderUseArtillery");
}

bool SAI::hostileTakeoveInProgress(AIStrategicObject*&, Vec2&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::hostileTakeoveInProgress");
}

bool SAI::isPosInsideOwnedControlPoint(AIStrategicObject*&, Vec3, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::isPosInsideOwnedControlPoint");
}

bool SAI::haveAnyOwnedCps(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::haveAnyOwnedCps");
}

bool SAI::haveAnyOwnedAndTakeableCps(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::haveAnyOwnedAndTakeableCps");
}

bool SAI::ownAllTakeableCps(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::ownAllTakeableCps");
}

// preserve unAssignSquads' full integer return.
// The caller's full-EAX additions establish the count contract; the body remains stopped.
int SAI::releaseSquadsFromObject(int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::releaseSquadsFromObject");
}

void SAI::releaseUnitFromObject(int, std::map<int, int>::iterator&, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::releaseUnitFromObject");
}

void SAI::foundDeadBot(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::foundDeadBot");
}

bool SAI::hasChangedEquipment(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::hasChangedEquipment");
}

void SAI::equipmentChangedFinished(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::equipmentChangedFinished");
}

void SAI::assignBot(std::map<int, int>::iterator&, int, int, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::assignBot");
}

bool SAI::strategicObjectsHasChanged()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::strategicObjectsHasChanged");
}

void SAI::registerTeleporter(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::registerTeleporter");
}

float SAI::getStrategyHeat(unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getStrategyHeat");
}

void SAI::listStrategies()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::listStrategies");
}

int SAI::getActiveStrategy()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getActiveStrategy");
}

// insert (point,1), increment only on duplicate.
// The native map lives at retail ELF +0x1D8 / PE +0x228; no key validation.
void SAI::addSpawnToPoint(int point)
{
    const std::pair<std::map<int, int>::iterator, bool> inserted =
        m_map1D8.insert(std::make_pair(point, 1));
    if (!inserted.second)
        ++inserted.first->second;
}

int SAI::getSpawnsToPoint(int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getSpawnsToPoint");
}

Vec3 SAI::getBotCurrentPos(int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getBotCurrentPos");
}

Vec3 SAI::getBotOrderedPos(int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getBotOrderedPos");
}

Vec3 SAI::getBotLastOrderedPos(int) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getBotLastOrderedPos");
}

// Accumulate into the previous totals, then decay; no initial clear.
// Nonempty information can still reach the named Information/IPIUnit forwarding frontiers.
void SAI::updateStrengths()
{
    std::vector<unsigned int> handles;
    UnitPredicate* predicate = checkPointer(new UnitPredicate);
    static_cast<IAIInformationEnvironment*>(AISingletonServer::m_iaiInformationEnvironment)->
        getAllInformation(handles, m_side, true, predicate, nullptr);

#if defined(_MSC_VER)
    // [PLATFORM] PE retains the last secondary lookup's manager for the next root lookup.
    // ELF reloads the global at the start of each root, and before each secondary callback.
    InformationManager* manager = handles.empty() ? nullptr : informationManager;
#endif
    for (auto it = handles.begin(); it != handles.end(); ++it)
    {
#if !defined(_MSC_VER)
        InformationManager* manager = informationManager;
#endif
        Information* root = manager->getInformation(*it);
        int secondary = root->getFirstSecondaryObjectIterator();
        const int rootSide = root->getSide();
        const bool friendly = rootSide == m_side;
        float* strengths = friendly ? m_friendlyStrengths : m_enemyStrengths;
        float* counts = friendly ? m_friendlyCounts : m_enemyCounts;
        Information* current = root;
        do
        {
            // Keep the selected unit across isOccupied/getSecurity, even if a callback replaces it.
            IPIUnit* unit = current->getUnit();
            if (unit->isOccupied())
            {
                const float security = current->getSecurity();
                for (int category = 0; category != 6; ++category)
                {
                    // Both shipped x87 paths retain the product through the add before storing
                    // float. Widen explicitly so modern SSE does not round the product early.
                    strengths[category] = static_cast<float>(
                        static_cast<double>(security) * unit->getBattleStrengths()[category]
                        + strengths[category]);
                }
                const int category = current->getPhysical()->getStrengthType();
                counts[category] += security;
            }

#if !defined(_MSC_VER)
            manager = informationManager;
#endif
            // Iteration always belongs to the original record, with the current SAI side.
            const unsigned int handle = root->getSecondaryObject(secondary, m_side);
#if defined(_MSC_VER)
            manager = informationManager;
#endif
            current = manager->getInformation(handle);
        } while (current);
    }

    for (int category = 0; category != 6; ++category)
    {
        if (m_friendlyStrengths[category] > 0.001f)
            m_friendlyStrengths[category] *= 0.5f;
        if (m_friendlyCounts[category] > 0.001f)
            m_friendlyCounts[category] *= 0.5f;
        if (m_enemyStrengths[category] > 0.001f)
            m_enemyStrengths[category] *= 0.5f;
        if (m_enemyCounts[category] > 0.001f)
            m_enemyCounts[category] *= 0.5f;
    }
    delete predicate;
}

// Updates strategic state in callback order with live vector iteration and repeated
// Route/Bridge tests. Non-area objects reuse the preceding area's output and border flag.
void SAI::updateStates(float* a, float* b)
{
    float* selected = 0;
    bool neighbour = false;
    const int enemySide = m_side == 1 ? 2 : 1;
    IAIGame* const game = static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame);
    a[13] = static_cast<float>(game->getTickets(m_side));
    b[13] = static_cast<float>(game->getTickets(enemySide));
    a[14] = static_cast<float>(game->getNumberOfControlPoints(m_side));
    b[14] = static_cast<float>(game->getNumberOfControlPoints(enemySide));

    const double now = static_cast<IBFTimer*>(CommonSingletonServer::m_iBFTimer)->getGameTime();
    a[15] = static_cast<float>(now);
    b[15] = static_cast<float>(now);
    StrategyContainer* const end = m_strategies.empty()
        ? m_strategies.data() : m_strategies.data() + m_strategies.size();
    a[16] = m_activeStrategy == end ? 0.0f : static_cast<float>(now - m_activeStrategy->m_startTime);
    b[16] = 0.0f;
    a[17] = static_cast<float>(m_list1BC.size());
    a[18] = static_cast<float>(m_list1B8.size());
    a[19] = 0.0f; b[19] = 0.0f;
    a[20] = 0.0f; b[20] = 0.0f;
    a[21] = 0.0f; b[21] = 0.0f;
    a[39] = 0.0f; b[39] = 0.0f;
    a[38] = 0.0f; b[38] = 0.0f;
    b[41] = 0.0f; a[41] = 0.0f;
    a[40] = 0.0f; b[40] = 0.0f;
    a[42] = 0.0f; b[42] = 0.0f;

    std::set<int> attack;
    std::set<int> defence;
    const std::vector<AIStrategicObject*>& objects =
        AIStrategicObjectManager::getInstance()->getStrategicObjects();
    for (std::vector<AIStrategicObject*>::const_iterator it = objects.begin(); it != objects.end(); ++it)
    {
        if ((*it)->getType() == 0)
        {
            switch ((*it)->getStatus(m_side))
            {
            case StatusMap::Owned:
                a[19] += 1.0f;
                b[21] += 1.0f;
                selected = a;
                neighbour = (*it)->hasNeighbourWithStatus(StatusMap::Hostile, m_side);
                if (neighbour) { a[39] += 1.0f; b[41] += 1.0f; }
                else           { a[40] += 1.0f; b[42] += 1.0f; }
                break;
            case StatusMap::Hostile:
                a[21] += 1.0f;
                b[19] += 1.0f;
                selected = b;
                neighbour = (*it)->hasNeighbourWithStatus(StatusMap::Owned, m_side);
                if (neighbour) { b[39] += 1.0f; a[41] += 1.0f; }
                else           { b[40] += 1.0f; a[42] += 1.0f; }
                break;
            case StatusMap::Neutral:
                a[20] += 1.0f;
                b[20] += 1.0f;
                if ((*it)->hasNeighbouringObjectWithStatus(StatusMap::Owned, m_side))
                    a[38] += 1.0f;
                if ((*it)->hasNeighbouringObjectWithStatus(StatusMap::Hostile, m_side))
                    b[38] += 1.0f;
                selected = 0;
                break;
            default:
                selected = 0;
                neighbour = false;
                break;
            }
        }

        if ((*it)->getNumberOfAssignedUnits(m_side) > 0)
        {
            if ((*it)->isAttack(m_side))
                attack.insert((*it)->getIndex());
            else
                defence.insert((*it)->getIndex());
        }

        if (selected)
        {
            // Each test reloads *it, including after a preceding virtual callback replaced it.
            static const AIStrategicObject::StrategicObjectType areaFlags[] = {
                AIStrategicObject::Flank, AIStrategicObject::Base, AIStrategicObject::Close,
                AIStrategicObject::Centre, AIStrategicObject::Remote
            };
            for (int i = 0; i != 5; ++i)
            {
                if ((*it)->hasObjectTypeFlag(areaFlags[i]))
                {
                    selected[22 + i] += 1.0f;
                    if (neighbour)
                        selected[33 + i] += 1.0f;
                }
            }
            static const AIStrategicObject::StrategicObjectType otherFlags[] = {
                AIStrategicObject::Route, AIStrategicObject::Bridge,
                AIStrategicObject::Route, AIStrategicObject::Bridge,
                AIStrategicObject::North, AIStrategicObject::West,
                AIStrategicObject::South, AIStrategicObject::East
            };
            static const int indices[] = { 27, 28, 27, 28, 29, 30, 31, 32 };
            for (int i = 0; i != 8; ++i)
                if ((*it)->hasObjectTypeFlag(otherFlags[i]))
                    selected[indices[i]] += 1.0f;
        }
    }
    b[17] = static_cast<float>(attack.size());
    b[18] = static_cast<float>(defence.size());
}

// Prepares admitted bots and resource strengths. Admission retains its Bot pointer
// across callbacks; strength/removal traversals reload the element. Preserve all four
// id queries during removal.
void SAI::prepareUpdate()
{
    for (std::vector<Bot*>::iterator it = m_vector94.begin(); it != m_vector94.end(); ++it)
    {
        Bot* const bot = *it;
        if (bot)
        {
            BotInfo* const info = checkPointer(new BotInfo(bot));
            m_botInfo[bot->getBotId()] = info;
            if (bot->isDead())
            {
                m_map5C.insert(std::make_pair(static_cast<int>(bot->getBotId()), std::make_pair(-1, 13)));
                bot->getPlayer()->setSpawnGroup(0);
                bot->setIsInTransitionPhase(true);
            }
            else
                setBotIsAlive(bot->getBotId());
            (void)m_setFC.find(bot->getBotId()); // shipped discarded lookup
        }
    }

    // [PLATFORM] PE frees/nulls all three vector pointers; ELF erase retains capacity.
#if defined(_MSC_VER)
    std::vector<Bot*>().swap(m_vector94);
#else
    m_vector94.erase(m_vector94.begin(), m_vector94.end());
#endif
    clearSpawnsToPoints();
    for (std::list<int>::iterator it = m_list110.begin(); it != m_list110.end(); ++it)
        handleTeleportingBot(*it);
    m_list110.clear();

    for (int i = 0; i != 6; ++i)
    {
        m_friendlyStrengths[i] = 0.0f;
        m_friendlyCounts[i] = 0.0f;
    }
    for (std::vector<BotInfo*>::iterator it = m_botInfo.begin(); it != m_botInfo.end(); ++it)
    {
        if (*it && !(*it)->m_bot->isDead())
        {
            // [PLATFORM] ELF captures the manager before the equipment callback. PE does so
            // only afterward, and does not read it at all when the handle's low half is zero.
#if !defined(_MSC_VER)
            InformationManager* const manager = informationManager;
#endif
            const unsigned int handle = (*it)->m_bot->getCurrentEquipment();
#if defined(_MSC_VER)
            if ((handle & 0xffffu) == 0)
                continue;
            InformationManager* const manager = informationManager;
#endif
            Information* const information = manager->getInformation(handle);
            if (information && information->getUnit())
            {
                IPIUnit* const unit = information->getUnit();
                // Assignment, not summation: the last live unit supplies all six strengths.
                for (int i = 0; i != 6; ++i)
                    m_friendlyStrengths[i] = unit->getBattleStrengths()[i];
                const int category = information->getPhysical()->getStrengthType();
                m_friendlyCounts[category] += 1.0f;
            }
        }
    }

    for (std::map<int, std::pair<int, int> >::iterator it = m_map5C.begin(); it != m_map5C.end();)
    {
        IAISettings* const settings = static_cast<IAISettings*>(CommonSingletonServer::m_iaiSettings);
        BotInfo* const info = m_botInfo[it->first];
        if (!info)
        {
            std::map<int, std::pair<int, int> >::iterator removed = it++;
            m_map5C.erase(removed);
        }
        else
        {
            Bot* const bot = info->m_bot;
            if (settings->isRespawnAllowed())
                BotSpawner::findSuitableSpawnGroup(m_enemyStrengths, m_enemyCounts,
                    m_friendlyStrengths, m_friendlyCounts, it->second.first, bot);
            ++it;
        }
    }

    for (std::vector<Bot*>::iterator it = m_vectorA0.begin(); it != m_vectorA0.end(); ++it)
    {
        if (m_botInfo[(*it)->getBotId()])
        {
            m_botInfo[(*it)->getBotId()]->remove(this);
            delete m_botInfo[(*it)->getBotId()];
            m_botInfo[(*it)->getBotId()] = 0;
        }
    }
#if defined(_MSC_VER)
    std::vector<Bot*>().swap(m_vectorA0);
#else
    m_vectorA0.erase(m_vectorA0.begin(), m_vectorA0.end());
#endif
}

// Reset mapped values
// retaining keys and nodes. The live native end is checked after each iterator advance.
void SAI::clearSpawnsToPoints()
{
    for (std::map<int, int>::iterator it = m_map1D8.begin(); it != m_map1D8.end(); ++it)
        it->second = 0;
}

// Remaining direct SAI::update dependencies. Paired callers and helper return paths establish
// these signatures; the strategic work behind them remains unrecovered.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
// BotInfo::remove: / , paired callers discard
// the result and neither body materializes a return. Assigned/free/present-unit removal is absent.
void SAI::BotInfo::remove(SAI*)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::BotInfo::remove");
}

// Void is inferred from complete
// bodies and their callers. The spawn-group selector and dead-bot/respawn chain remain absent.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
void SAI::handleTeleportingBot(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::handleTeleportingBot");
}

bool SAI::updateStrategicObjects()
{
    const std::vector<AIStrategicObject*>& objects =
        AIStrategicObjectManager::getInstance()->getStrategicObjects();
    bool changed = false;
    if (m_flag1D0)
    {
        m_flag1CF = false;
        m_flag1D0 = false;
        m_strategicObjectIterator = objects.begin();
        m_enemyStats->clear();
        m_friendlyStats->clear();
    }

    // Both ports dereference unconditionally and process exactly one object. The concrete
    // update returns void. There is no empty-vector guard or inner budget check.
    (*m_strategicObjectIterator)->update(m_side, this, changed);
    m_flag1CF = m_flag1CF || changed;

    // The callback may change the cursor/entry/side. Capture status before the getter
    // then load the selected destination; the shipped numeric split is zero/nonzero.
    AIStrategicObject* const object = *m_strategicObjectIterator;
    const int side = m_side;
    const StatusMap::Status status = object->getStatus(side);
    EnemyStatistics& statistics = object->getBattleStatistics(side);
    if (status == 0)
        m_enemyStats->add(statistics);
    else
        m_friendlyStats->add(statistics);

    ++m_strategicObjectIterator;
    if (m_strategicObjectIterator != objects.end())
        return false;

    if (m_flag1CF)
    {
        // Retain this invocation's vector, but reload its live end, entry and side.
        for (std::vector<AIStrategicObject*>::const_iterator it = objects.begin();
             it != objects.end(); ++it)
        {
            (*it)->setAsDirty(m_side);
            (*it)->releaseAssignedResources(m_side);
        }
    }
    m_flag1D0 = true;
    return true;
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
void SAI::chooseStrategy()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::chooseStrategy");
}

void SAI::modifyStrategicObjectsTemperatures()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::modifyStrategicObjectsTemperatures");
}

void SAI::updateBotPositions()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::updateBotPositions");
}

void SAI::updateDistribution()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::updateDistribution");
}

void SAI::distributeResources()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::distributeResources");
}

void SAI::distributeDefence()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::distributeDefence");
}

void SAI::distributeAttack()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::distributeAttack");
}

void SAI::distributeWithNoAreas()
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::distributeWithNoAreas");
}

// Recovered empty helper: , (frame prologue/epilogue and return only).
// PE SAI::update omits the empty call at state 14.
void SAI::distributeArtillery()
{
}

// SampleHelper: complete paired bodies, unsigned live bound and no index/size guards.
SampleHelper::SampleHelper(unsigned int size) : m_size(size)
{
}

void SampleHelper::addSample(float* input, int index, float* strength, int* count)
{
    for (unsigned int i = 0; i < m_size; ++i)
        strength[i] = input[i] + strength[i];
    ++count[index];
}

void SampleHelper::clear(float* strength, int* count)
{
    for (unsigned int i = 0; i < m_size; ++i)
    {
        strength[i] = 0.0f;
        count[i] = 0;
    }
}

CommanderInterface* SAI::getCommanderInterface() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::SAI::getCommanderInterface");
}

} // namespace ai
} // namespace hfe
} // namespace dice
