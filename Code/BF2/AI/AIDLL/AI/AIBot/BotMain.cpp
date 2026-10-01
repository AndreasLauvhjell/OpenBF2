// Bot implementation: owns its behavior, movement, sensing and communication state.
// The manager schedules work through the Bot interface; unimplemented operations stop by name.

#include "BotMain.h"
#include "BotOrder.h"
#include "BotCallBack.h"

#include "../Main/AIDllFrontier.h"
#include "../Main/AIMain.h"
#include "../AIStrategic/SAI.h"
#include "BotMemory.h"
#include "BotNavigation.h"
#include "BotSquad.h"
#include "../AIPlanInterpreter/BAPContext.h"
#include "SensingAgent.h"
#include "CommunicationAgent.h"
#include "BotSubObjects.h"
#include "../AIBotBehaviour/UrgencyMerger.h"
#include "../AIUtils/checkPointer.h"
#include "../AIKnowledge/InformationManager.h"
#include "../AIKnowledge/Information.h"      // updatePlayerAction: the equipment's mobile plug-in
#include "../AIKnowledge/IPIMobile.h"
#include "../../../AIInterface/BFKnowledge/AIPlayer.h"
#include "../../../../IO/Input/PlayerInput.h"
#include <cfloat>       // DBL_MAX - the two 'never happened' timestamps

namespace dice {
namespace hfe {
namespace ai {

// The base destructor leaves member lifetime to the derived class.
Bot::~Bot()
{
}

// Returns base bot type 1; BotMain overrides this with type 0.
int Bot::getBotType() const
{
    return 1;
}

// Returns the context stored in Bot. BotMain inherits this accessor.
BAPContext* Bot::getScriptContext()
{
    return m_bapContext;
}

// Constructs the per-bot state and helper objects. The supplied player is borrowed.
BotMain::BotMain(IAIPlayer* player,
                 unsigned int id,
                 float* weights,
                 float f,
                 int i,
                 BotOrder* order,
                 BotMemory* memory,
                 PlanInterpreter* interpreter,
                 int n,
                 UrgencyGenerator* merger,
                 UrgencyGenerator** generators)
{
    m_debugger  = 0;
    m_lodLevel  = BotLodFull;           // = 0
    m_planInvalid = true;
    m_hasDefaultPlan = false;
    m_botId        = id;
    m_basicWeights = weights;
    m_float32      = f;
    m_obedience      = 1.0f;
    m_int40        = i;
    m_ptr52        = 0;
    m_spawnToStrategicAreaId        = -1;
    m_side        = n;
    m_ptr72        = 0;
    m_float48      = 0.01f;
    m_gap88[1]     = 0;                 // +89
    m_memory       = memory;
    m_gap88[2]     = 0;                 // +90
    m_inTransitionPhase = false;        // +91
    m_pad92        = 0;                 // +92
    m_deactivated  = false;             // +93
    m_ptr100 = 0;
    m_ptr104 = 0;
    m_ptr108 = 0;
    m_activePlan = 0;
    m_planLock = false;                 // +120

    // List members construct themselves. [UNRESOLVED] Some element types remain unknown.

    m_currentEquipment            = -1;
    m_planInterpreter   = interpreter;
    m_urgencyMerger     = merger;
    m_urgencyGenerators = generators;
    m_ptr160 = 0;
    m_ptr164 = 0;
    m_ptr168 = 0;

    // sub-object 1 of 6
    m_fireCorrection = checkPointer(new FireCorrectionData());

    for (int k = 0; k < 5; ++k)
        (&m_changeTarget)[k] = static_cast<unsigned int>(-1);   // +204..+220
    m_mineTarget = -1;
    m_moveInclination = 0;
    m_fireInclination = 0;
    m_moveInclinationDelta = 1.0f;
    m_fireInclinationDelta = 1.0f;
    m_ptr232 = 0;
    m_ptr256 = 0;
    m_ptr260 = 0;
    m_firingTarget = -1;
    m_requestedSoldierPose = 1;
    m_int304 = -1;
    m_unresolvedCollision = -1;
    m_ptr252 = 0;

    // Use -DBL_MAX for uninitialized timestamps; magnitude sentinels use separate values.
    m_time276 = -DBL_MAX;
    m_time288 = -DBL_MAX;
    m_botStuck = false;
    m_flag285 = false;
    for (int k = 0; k < 4; ++k)
        m_flag300[k] = false;           // +300..+303
    m_airBorn = false;                  // +312
    for (int k = 0; k < 3; ++k)
        m_ptr316[k] = 0;                // +316..+324
    m_deathCallBack = 0;                // +328
    m_spawnCallBack = 0;                // +332
    m_squadMember = 0;
    m_assignedAreas[0] = -1;
    m_assignedAreas[3] = -1;                   // +396
    m_assignedAreas[2] = -1;                   // +392
    m_ptr400 = 0;
    m_assignedAreas[1] = -1;                   // +388
    m_manDown = false;                  // +360
    m_isFiring = false;                 // +372
    m_fireAllowed = true;
    m_ptr404 = 0;
    m_ptr408 = 0;
    m_int412 = -1;
    m_stalledPlanTicks = 0;
    for (int k = 0; k < 3; ++k)
        m_ptr428[k] = 0;                // +428..+436
    m_moveToStatus = 0;
    m_timeOfLastFailedStaticAttack = 0.0;
    m_timeOfLastFailedStaticSkyAttack = 0.0;
    m_avoidSide = 0;
    for (int k = 0; k < 3; ++k)
        (&m_wantsToSimulateArtillery)[k] = false;   // +500..+502
    m_gap512[0] = 0;                    // +512
    m_collidedObject = -1;
    m_double516 = 0.0;
    for (int k = 0; k < 3; ++k)
        m_collidedObjectPosition[k] = 0;                // +528..+536

    // A different sentinel from the two above - -9999.0 is a magnitude, not a time.
    m_lastCheckMoveToFinishedTime = -9999.0;
    m_timeOfLastFailedSplashDamageAttack = -9999.0;
    m_slowDownToTarget    = -1;
    m_friendsInLineOfFireTime = 0.0;
    m_ignoreObstacles = false;          // +540
    m_isGoingForFlag = false;
    m_useStaticAttack = false;

    // copied component by component out of Vec3::getZero, which is what the binary does.
    const Vec3& zero = Vec3::getZero();
    m_takeOffPosition[0] = zero.x;
    m_takeOffPosition[1] = zero.y;
    m_takeOffPosition[2] = zero.z;

    m_grenadeThrownTime[0] = 0.0;
    m_grenadeThrownTime[1] = 0.0;
    m_grenadeThrownTime[2] = 0.0;

    // sub-objects 2 to 5
    m_navigation = checkPointer(new BotNavigation(this));
    m_squad      = checkPointer(new BotSquad(this));
    m_sensingAgent = checkPointer(new SensingAgent(this, mu8NBehaviours));

    // this byte is set in the middle of the allocations, between SensingAgent and
    // CommunicationAgent, and it is -1 rather than 0. Reproduced in position.
    m_gap64[0] = static_cast<char>(-1);  // +64
    m_communication = checkPointer(new CommunicationAgent(this));

    // Allocates matching urgency and behavior arrays, but clears only behavior pointers.
    // Urgency values remain uninitialized until written.
    m_behaviourUrgencies = checkPointer(new float[mu8NBehaviours]);
    m_behaviours = checkPointer(new BotBehaviour*[mu8NBehaviours]);
    for (unsigned k = 0; k < mu8NBehaviours; ++k)
        m_behaviours[k] = 0;

    // finishing someone else'S object - see the block above.
    *reinterpret_cast<int*>(m_memory) = -1;
    *reinterpret_cast<double*>(reinterpret_cast<char*>(m_memory) + 16) = 0.0;

    if (order)
        recieveOrder(order);            // dice's spelling

    m_waypoints = 0;
    m_ptr84  = 0;
    m_player = player;
    if (player)
        setPlayerInput();

    // zeroed A second time - see the block above.
    m_ptr160 = 0;
    m_ptr164 = 0;
    m_ptr168 = 0;

    // sub-object 6 of 6, and it is the smallest thing here at four bytes
    m_bapContext = checkPointer(new BAPContext());

    // 256 is BF2'S player cap - the same number BotManager's player-id table uses. Every bot can
    // sense every player, and the storage for that is committed at construction.
    m_sensedObjects.reserve(256);

    m_c4Placed = false;                  // the constructor's last field store
    reset();
}


// [CHOSEN, not recovered] Initial behavior count is zero until configured by the module.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding BotMain operation.
unsigned char BotMain::mu8NBehaviours = 0;
// ELF .bss / PE .data . Set by prepareBehaviourPool, not per bot.
float* BotMain::maBasicWeights = 0;

// [FRONTIER] Remaining bot lifecycle operations stop by name.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding BotMain operation.
BotMain::~BotMain()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::~BotMain");
}

// Releases each behavior's initialized state without deleting the behavior itself.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding BotMain operation.
void BotMain::uninitAllBehaviours()
{
    for (int i = 0; i < mu8NBehaviours; ++i)
    {
        if (m_behaviours[i])
        {
            // Retail here is `m_behaviours[i]->something->uninit(this)` - see the block above.
            BF2_AI_FRONTIER("dice::hfe::ai::BotMain::uninitAllBehaviours (a behaviour is present)");
        }
    }
}

// Resets per-life bot state for reuse. Dependent operations may reach named frontier stops.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
void BotMain::reset()
{
    m_double172 = 0.0;

    if (m_player)
    {
        // Retail calls IAIPlayer byte-slot +8 with no arguments. See the block above.
        BF2_AI_FRONTIER("dice::hfe::ai::BotMain::reset (IAIPlayer slot +8)");
    }

    if (m_activePlan)
    {
        // Retail destroys the plan through byte-slot +8. See the block above.
        BF2_AI_FRONTIER("dice::hfe::ai::BotMain::reset (active plan present)");
        m_activePlan = 0;
        m_planInvalid = true;
        m_sensingAgent->setOverride(false);
    }

    m_list128.clear();
    m_orderedBehaviour = -1;

    // the five. See the block above.
    uninitAllBehaviours();
    m_squad->reset();
    m_navigation->resetAll();
    m_sensingAgent->reset();
    m_communication->reset();

    m_stalledPlanTicks = 0;
    m_int200 = -1;
    m_int304 = -1;
    m_avoidSide = 0;
    m_moveToStatus = 0;
    m_collidedObject = -1;
    m_double364 = 0.0;
    m_unresolvedCollision = -1;
    m_grenadeThrownTime[0] = 0.0;
    m_grenadeThrownTime[1] = 0.0;
    m_grenadeThrownTime[2] = 0.0;
    m_timeOfLastFailedStaticAttack = 0.0;
    m_timeOfLastFailedStaticSkyAttack = 0.0;
    m_double516 = 0.0;
    m_wantsToSimulateArtillery = false;
    m_isSimulatingArtillery = false;
    m_flag502 = false;
    m_gap512[0] = 0;
    m_collidedObjectPosition[0] = 0;
    m_collidedObjectPosition[1] = 0;
    m_collidedObjectPosition[2] = 0;
    m_friendsInLineOfFireTime = 0.0;
    // three of the five, and not the first three: the change and revive targets survive a
    // reset while the three repair targets do not.
    m_wantedRepairTarget = static_cast<unsigned int>(-1);
    m_repairTarget       = static_cast<unsigned int>(-1);
    m_bridgeRepairTarget = static_cast<unsigned int>(-1);
    m_ignoreObstacles = false;
    m_slowDownToTarget = -1;
    m_lastCheckMoveToFinishedTime = -9999.0;
    m_isRepaired           = false;
    m_isEngineerTarget     = false;
    m_shouldWaitForRepairs = false;
    m_isGoingForFlag = false;
    m_useStaticAttack = false;
    // 9999.0, not the ctor's -DBL_MAX. See the block above.
    m_time288 = -9999.0;
    m_timeOfLastFailedSplashDamageAttack = -9999.0;
    m_takeOffPosition[0] = 0.0f;
    m_takeOffPosition[1] = 0.0f;
    m_takeOffPosition[2] = 0.0f;
}

// Both materialise a Boolean result. The pointer at +68 is the
// order's weights, not the BotOrder object. There is no copy, deletion or null-order guard.
bool BotMain::recieveOrder(BotOrder* order)
{
    if (!m_takingOrders)
        return false;
    m_currentOrders = order->m_weights;
    m_orderedBehaviour = static_cast<signed char>(order->m_behaviour);
    m_list128.clear();
    if (m_ptr72)
        calculateMods();
    return true;
}

// Binding a player immediately prepares its input; no null guard.
void BotMain::setPlayer(IAIPlayer* player)
{
    m_player = player;
    setPlayerInput();
}

// Clear only the 64 values; existing mapping/state bits survive.
// The engine's current getPlayerInput declaration is const void*, but both images write through
// the returned pointer. This cast preserves that existing ABI pending its wider type recovery.
void BotMain::setPlayerInput()
{
    m_playerInput = const_cast<io::PlayerInput*>(
        static_cast<const io::PlayerInput*>(m_player->getPlayerInput()));
    for (unsigned int i = 0; i != 64; ++i)
        m_playerInput->inputs[i] = 0.0f;
    m_playerInput->mappingFlags |= 1ULL;
    m_playerInput->setInputIsMapped(io::PIPitch, true);
    m_playerInput->setInputIsMapped(io::PIRoll, true);
    m_playerInput->setInputIsMapped(io::PIThrottle, true);
    m_playerInput->setInputIsMapped(io::PISprint, true);
    m_playerInput->setInputIsMapped(io::PIUse, true);
    m_playerInput->setInputIsMapped(io::PIMouseLook, true);
    m_playerInput->setInputIsMapped(io::PIMouseLookX, true);
    m_playerInput->setInputIsMapped(io::PIMouseLookY, true);
    m_playerInput->setInputIsMapped(io::PIFire, true);
    m_playerInput->setInputIsMapped(io::PIAltFire, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect1, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect2, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect3, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect4, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect5, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect6, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect7, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect8, true);
    m_playerInput->setInputIsMapped(io::PIWeaponSelect9, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect1, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect2, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect3, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect4, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect5, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect6, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect7, true);
    m_playerInput->setInputIsMapped(io::PIPositionSelect8, true);
    m_playerInput->setInputIsMapped(io::PILie, true);
    m_playerInput->setInputIsMapped(io::PICrouch, true);
    m_playerInput->setInputIsMapped(io::PIUse, true); // repeated in both images
    m_playerInput->setInputIsMapped(io::PIReload, true);
    m_playerInput->setInputIsMapped(io::PIFlareFire, true);
    m_playerInput->setInputIsMapped(io::PIAction, true);
}

// ELF 0x18A2C0 / PE 0x1017EB20. [PLATFORM] Both products use the same factors, but the
// instructions multiply in different orders. Keep each order instead of reassociating it.
void BotMain::calculateMods()
{
    for (int i = 0; i < mu8NBehaviours; ++i)
    {
#if defined(_WIN32)
        if (m_takingOrders)
            m_behaviourUrgencies[i] = m_currentOrders[i] * m_ptr72[i] * maBasicWeights[i] * m_basicWeights[i];
        else
            m_behaviourUrgencies[i] = m_ptr72[i] * maBasicWeights[i] * m_basicWeights[i];
#else
        if (m_takingOrders)
            m_behaviourUrgencies[i] = m_currentOrders[i] * m_basicWeights[i] * maBasicWeights[i] * m_ptr72[i];
        else
            m_behaviourUrgencies[i] = maBasicWeights[i] * m_basicWeights[i] * m_ptr72[i];
#endif
    }
}

// 30/50/70 / /50/60/70: class statics, no this argument.
void BotMain::setNBehaviours(unsigned char n) { mu8NBehaviours = n; }
unsigned char BotMain::getNBehaviours() { return mu8NBehaviours; }
void BotMain::setBasicWeights(float* weights) { maBasicWeights = weights; }
float* BotMain::getBasicWeights() { return maBasicWeights; }

// per-bot accessors, unlike getBasicWeights above.
float* BotMain::getBasicBehaviourMods() { return m_basicWeights; }
float* BotMain::getCurrentBehaviourMods() { return m_behaviourUrgencies; }
void BotMain::setTakingOrders(bool taking) { m_takingOrders = taking; }

// Decision making and plan execution below remain frontiers; player binding, input preparation
// and initial order receipt above are recovered without claiming that the bot can execute them.


// The two plan-execution brackets. not vtable slots - `planExecution` at +0x010 is, but these
// two are ordinary members, so whatever calls them does so on a `BotMain*` and not through `Bot`.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
void BotMain::prePlanExecution()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::prePlanExecution");
}

void BotMain::postPlanExecution()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::postPlanExecution");
}

// Bot interface implementations, in virtual declaration order.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the corresponding BotMain operation.

// +0x008
void BotMain::setLodLevel(BotLodLevel level)
{
    m_lodLevel = level;
}

// +0x00C
BotLodLevel BotMain::getLodLevel() const
{
    return m_lodLevel;
}

// +0x010
void BotMain::planExecution(const double& slice)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::planExecution");
}

// +0x014. Returns the count BotManager::actionDecisionMaking banks in BotInfo +32 (see BotMain.h).
int BotMain::decisionMaking(const double& slice)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::decisionMaking");
}

// +0x018
void BotMain::cheatExecution()
{
    // five bytes - A bare `ret`. The hook exists and does nothing. Reproduced.
}

// +0x01C
void BotMain::see(const double& dt)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::see");
}

// +0x020
void BotMain::updateSensing(const double& dt)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::updateSensing");
}

// +0x024
void BotMain::updatePathfinding(bool& a, bool* b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::updatePathfinding");
}

// +0x02C
void BotMain::updateBotVehicle(bool* out)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::updateBotVehicle");
}

// +0x030
void BotMain::updateBot()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::updateBot");
}

// +0x034
BotSquad* BotMain::getBotSquad() const
{
    return m_squad;
}

// +0x038
float BotMain::getObedience()
{
    return m_obedience;
}

// +0x03C
float* BotMain::getCurrentOrders()
{
    return m_currentOrders;
}

// +0x040
WaypointList* BotMain::getWaypoints()
{
    return m_waypoints;
}

// +0x044
void BotMain::setWaypoints(WaypointList* w)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setWaypoints");
}

// +0x04C
void BotMain::recieveOrder(BotOrder* order, std::list<unsigned int>& targets)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::recieveOrder");
}

// +0x050
std::list<unsigned int>* BotMain::getOrderList(unsigned char behaviour)
{
    return (behaviour == m_orderedBehaviour) ? &m_list128 : &m_list132;
}

// +0x054
bool BotMain::hasPlan()
{
    return !m_planInvalid;
}

// +0x058
bool BotMain::hasDefaultPlan() const
{
    return m_hasDefaultPlan;
}

// +0x05C
void BotMain::invalidatePlan()
{
    m_planInvalid = true;
}

// +0x060
BotScript* BotMain::getPlan()
{
    return m_activePlan;
}

// +0x064
void BotMain::setPlan(BotScript* plan)
{
    m_activePlan = plan;
}

// +0x068
bool BotMain::getPlanLock() const
{
    return m_planLock;
}

// +0x06C
void BotMain::setPlanLock(bool b)
{
    m_planLock = b;
}

// +0x070
bool BotMain::isTakingOrders()
{
    return m_takingOrders;
}

// +0x074
bool BotMain::isOrderedBehaviour(unsigned char behaviour)
{
    return behaviour == m_orderedBehaviour;
}

// +0x078
bool BotMain::isOrderedObject(unsigned int handle, BotBehaviour* b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::isOrderedObject");
}

// +0x07C
void BotMain::setFiringTarget(unsigned int handle)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setFiringTarget");
}

// +0x080
unsigned int BotMain::getFiringTarget() const
{
    return m_firingTarget;
}

// +0x084
double BotMain::getFiringTargetTime() const
{
    return m_firingTargetTime;
}

// +0x088
SensingAgent* BotMain::getSensingAgent()
{
    return m_sensingAgent;
}

// +0x08C
void BotMain::insertBehaviourInformation(float f, const Vec3& v, unsigned char c)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::insertBehaviourInformation");
}

// +0x090
const std::vector<IAIObject*>& BotMain::getSensedObjects() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getSensedObjects");
}

// +0x094
void* BotMain::getSensedObjectFromHandle(unsigned int handle) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getSensedObjectFromHandle");
}

// +0x098
void BotMain::setUnresolvedCollisionObject(unsigned int handle)
{
    m_unresolvedCollision = handle;
}

// +0x09C
unsigned int BotMain::getUnresolvedCollisionHandle()
{
    return m_unresolvedCollision;
}

// +0x0A0
void* BotMain::getIncommingFire()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getIncommingFire");
}

// +0x0A4
float BotMain::getIncommingFireTotalStrength()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getIncommingFireTotalStrength");
}

// +0x0A8
float BotMain::getIncommingFireTotalStrengthAgainstSelf()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getIncommingFireTotalStrengthAgainstSelf");
}

// +0x0AC
float BotMain::getAttackerStrength() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getAttackerStrength");
}

// +0x0B0
CommunicationAgent* BotMain::getCommunicationAgent()
{
    return m_communication;
}

// forward to the bound player's getId, without a null guard.
int BotMain::getPlayerId() const
{
    return m_player->getId();
}

// +0x0B8
int BotMain::getInputId()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getInputId");
}

// +0x0BC
IAIPlayer* BotMain::getPlayer() const
{
    return m_player;
}

// The squared speed both images test, grouped (x*x + y*y) + z*z as the PE's x87 sequence is and
// summed in double, which is exactly the PE's 53-bit precision control (products of floats are
// exact at 53 bits). [PLATFORM] the ELF keeps the sum at 64-bit precision. A NaN sum is never
// "at most", which matches both images' unordered branch.
static bool speedSquaredAtMost(const Vec3& speed, float limit)
{
    const double x = speed.x, y = speed.y, z = speed.z;
    return (x * x + y * y) + z * z <= limit;
}

// Updates player actions, retaining stalled movement when a plan update was skipped.
void BotMain::updatePlayerAction(bool stalled)
{
    if (!stalled)
    {
        m_stalledPlanTicks = 0;
        return;
    }
    ++m_stalledPlanTicks;
    Information* const equipment = informationManager->getInformation(m_currentEquipment);
    if (m_stalledPlanTicks <= 15)
    {
        IPIMobile* const mobile = equipment->getMobile();
        if (mobile && !speedSquaredAtMost(mobile->getSpeed(), 0.01f))
        {
            m_playerInput->setInput(io::PILie, 0.0f);
            m_playerInput->setInput(io::PIFire, 0.0f);
            m_playerInput->setInput(io::PIAltFire, 0.0f);
            m_playerInput->setInput(io::PIReload, 0.0f);
            return;
        }
    }
    resetAllControls(true, true, true);
}

// read in outline only. Its effect depends on the
// equipment's flag word, Information::getFaceing (slot 16), the mobile speed and a function-static
// `minThrottle`. The first gate releases sprint, yaw, pitch and roll and sets the throttle against
// the speed along the facing (clamped by minThrottle) or to zero; the second releases both look
// axes; the third releases fire, alt-fire, the nine weapon and eight position selects, flares
// crouch and lie (conditionally), use and reload.
// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
void BotMain::resetAllControls(bool, bool, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::resetAllControls");
}

// deactivated bots are dead without consulting the player.
bool BotMain::isDead() const
{
    return m_deactivated || !m_player->getIsAlive();
}

// +0x0C8 (+0xC4). The equipment is updated before either
// the bot or its player; an absent/stale information handle skips only that equipment arm.
bool BotMain::setSide(int side)
{
    Information* const equipment = informationManager->getInformation(m_currentEquipment);
    if (equipment)
    {
        // [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: aidll-02-skeleton]
        // IAIObject has no recovered typed interface yet. Retail calls its setSide at ELF
        // +0x128 / PE +0x124 through Information's object (+0x20 ELF / +0x28 PE).
        BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setSide (equipment IAIObject::setSide)");
    }
    m_side = side;
    m_player->setSide(side);
    return true;
}

// +0x0CC
void BotMain::setName(const std::string& name)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setName");
}

// +0x0D0
bool BotMain::isDeactivated() const
{
    return m_deactivated;
}

// +0x0D4
void BotMain::setAsDeactivated(bool b)
{
    m_deactivated = b;
}

// +0x0D8
unsigned int BotMain::getCurrentEquipment() const
{
    return m_currentEquipment;
}

// +0x0DC
void BotMain::setCurrentEquipment(unsigned int handle)
{
    m_currentEquipment = handle;
}

// +0x0E0
int BotMain::getSide() const
{
    return m_side;
}

// +0x0E4
int BotMain::getBotType() const
{
    return 0;
}

// +0x0E8
unsigned int BotMain::getBotId() const
{
    return m_botId;
}

// +0x0EC
BotMemory* BotMain::getMemory()
{
    return m_memory;
}

// +0x0F0
const std::string& BotMain::getName() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getName");
}

// +0x0F4
void BotMain::setTimeOfLastGrenadeThrown(double t)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setTimeOfLastGrenadeThrown");
}

// +0x0F8
double BotMain::getTimeOfGrenadeThrown(int i) const
{
    return m_grenadeThrownTime[i];
}

// +0x0FC
BotNavigation* BotMain::getBotNavigation()
{
    return m_navigation;
}

// no player null guard.
int BotMain::getSpawnPoint() const
{
    return m_player->getSpawnGroup();
}

// +0x104
int BotMain::getSpawnKit() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getSpawnKit");
}

// +0x108
void BotMain::getSpawnTarget(int& a, int& b) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getSpawnTarget");
}

// Keep fresh player/side/id reads
// after each callback, and clear the strategic area only after the SAI notification.
void BotMain::setSpawnTarget(int spawnGroup, int kit)
{
    m_player->setSpawnGroup(spawnGroup);
    m_player->setSpawnKit(kit);
    ISAI* const sai = IAIMain::m_instance->getSAI(m_side);
    sai->setNewBotKit(m_botId, kit);
    m_spawnToStrategicAreaId = -1;
}

// Set the engine spawn metadata, notify the side's SAI
// then store the strategic area. No player/SAI null guard or value validation is present.
void BotMain::setSpawnTarget(int strategicArea, int spawnGroup, int kit)
{
    m_player->setSpawnGroup(spawnGroup);
    m_player->setSpawnKit(kit);
    ISAI* const sai = IAIMain::m_instance->getSAI(m_side);
    sai->setNewBotKit(m_botId, kit);
    m_spawnToStrategicAreaId = strategicArea;
}

// +0x114
int BotMain::getSpawnToStrategicAreaId()
{
    return m_spawnToStrategicAreaId;
}

// +0x118
bool BotMain::isJustSpawned()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::isJustSpawned");
}

// +0x11C
IBotDeathCallBack* BotMain::getDeathCallBack() const
{
    return m_deathCallBack;
}

// destroy the previous callback before storing its replacement.
// No same-pointer guard: passing the current callback leaves the shipped dangling pointer.
void BotMain::setDeathCallBack(IBotDeathCallBack* cb)
{
    if (m_deathCallBack)
        delete m_deathCallBack;
    m_deathCallBack = cb;
}

// +0x124
IBotSpawnCallBack* BotMain::getSpawnCallBack() const
{
    return m_spawnCallBack;
}

// same ownership ordering as the death callback.
void BotMain::setSpawnCallBack(IBotSpawnCallBack* cb)
{
    if (m_spawnCallBack)
        delete m_spawnCallBack;
    m_spawnCallBack = cb;
}

// +0x12C
void BotMain::setIsFiring(bool b)
{
    m_isFiring = b;
}

// +0x130
bool BotMain::isFiring()
{
    return m_isFiring;
}

// +0x134
void BotMain::setAllowFire(bool b)
{
    m_fireAllowed = b;
}

// +0x138
bool BotMain::isFireAllowed()
{
    return m_fireAllowed;
}

// +0x13C
void BotMain::setSquadMember(SquadMember* m)
{
    m_squadMember = m;
}

// +0x140
SquadMember* BotMain::getSquadMember()
{
    return m_squadMember;
}

// +0x144
bool BotMain::isBotStuck()
{
    return m_botStuck;
}

// +0x148
void BotMain::setBotStuck(bool b)
{
    m_botStuck = b;
}

// +0x14C
void BotMain::placeC4(int placementType, Vec3 v, AIStrategicObject* o)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::placeC4");
}

// +0x150
bool BotMain::isC4Placed()
{
    return m_c4Placed;
}

// +0x154
void BotMain::removeC4()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::removeC4");
}

// +0x158
void* BotMain::getC4()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getC4");
}

// +0x15C
void BotMain::updateC4()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::updateC4");
}

// +0x160
void BotMain::enableBotScriptDebugger(bool /*enable*/)
{
    // defect, reproduced: the argument is discarded and the debugger is always cleared.
    m_debugger = 0;
}

// +0x164
void* BotMain::getDebugger()
{
    return m_debugger;
}

// +0x168 is `Bot::getScriptContext` and is defined above, beside `Bot::getBotType`. BotMain does not
// override it - see the note in BotMain.h.

// +0x16C
void BotMain::event_projectile(FireObject* f, unsigned int h)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::event_projectile");
}

// +0x170
void BotMain::event_shotHit(int i, world::IObject* o, const Vec3& v)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::event_shotHit");
}

// +0x174
void BotMain::event_shotMissed(int i, const Vec3& v)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::event_shotMissed");
}

// +0x178
void BotMain::event_heardObject(unsigned int h)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::event_heardObject");
}

// +0x17C
bool BotMain::isInTransitionPhase() const
{
    return m_inTransitionPhase;
}

// +0x180
void BotMain::setIsInTransitionPhase(bool b)
{
    m_inTransitionPhase = b;
}

// +0x184
void BotMain::addToMoveInclination(float f)
{
    m_moveInclinationDelta += f;
}

// +0x188
void BotMain::addToFireInclination(float f)
{
    m_fireInclinationDelta += f;
}

// +0x18C
float BotMain::getMoveInclination()
{
    return m_moveInclination;
}

// +0x190
float BotMain::getFireInclination()
{
    return m_fireInclination;
}

// +0x194
void BotMain::requestSoldierPose(int pose, bool b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::requestSoldierPose");
}

// +0x198
int BotMain::getRequestedSolderPose()
{
    return m_requestedSoldierPose;
}

// +0x19C
void BotMain::setPoseStatus(int pose, unsigned int h, bool a, bool b)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setPoseStatus");
}

// +0x1A0
void BotMain::event_airborn()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::event_airborn");
}

// +0x1A4
bool BotMain::isAirBorn() const
{
    return m_airBorn;
}

// +0x1A8
void BotMain::setLastChangeTime(double t)
{
    m_lastChangeTime = t;
}

// +0x1AC
void BotMain::setManDown(bool b)
{
    m_manDown = b;
}

// +0x1B0
bool BotMain::getManDown() const
{
    return m_manDown;
}

// +0x1B4
void BotMain::rearm(Information* i)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::rearm");
}

// +0x1B8
void BotMain::setChangeTarget(unsigned int h)
{
    m_changeTarget = h;
}

// +0x1BC
unsigned int BotMain::getChangeTarget() const
{
    return m_changeTarget;
}

// +0x1C0
void BotMain::setRepairTarget(unsigned int h)
{
    m_repairTarget = h;
}

// +0x1C4
unsigned int BotMain::getRepairTarget() const
{
    return m_repairTarget;
}

// +0x1C8
void BotMain::setWantedRepairTarget(unsigned int h)
{
    m_wantedRepairTarget = h;
}

// +0x1CC
unsigned int BotMain::getWantedRepairTarget() const
{
    return m_wantedRepairTarget;
}

// +0x1D0
void BotMain::setBridgeRepairTarget(unsigned int h)
{
    m_bridgeRepairTarget = h;
}

// +0x1D4
unsigned int BotMain::getBridgeRepairTarget() const
{
    return m_bridgeRepairTarget;
}

// +0x1D8
void BotMain::setReviveTarget(unsigned int h)
{
    m_reviveTarget = h;
}

// +0x1DC
unsigned int BotMain::getReviveTarget() const
{
    return m_reviveTarget;
}

// +0x1E0
void BotMain::setIsRepaired(bool b)
{
    m_isRepaired = b;
}

// +0x1E4
bool BotMain::getIsRepaired() const
{
    return m_isRepaired;
}

// +0x1E8
void BotMain::setIsEngineerTarget(bool b)
{
    m_isEngineerTarget = b;
}

// +0x1EC
bool BotMain::getIsEngineerTarget() const
{
    return m_isEngineerTarget;
}

// +0x1F0
void BotMain::setShouldWaitForRepairs(bool b)
{
    m_shouldWaitForRepairs = b;
}

// +0x1F4
bool BotMain::getShouldWaitForRepairs() const
{
    return m_shouldWaitForRepairs;
}

// +0x1F8
void BotMain::setMineTarget(unsigned int h)
{
    m_mineTarget = h;
}

// +0x1FC
unsigned int BotMain::getMineTarget() const
{
    return m_mineTarget;
}

// +0x200
void BotMain::setNewAssignedAreas(int a, int b)
{
    m_assignedAreas[0] = a;
    m_assignedAreas[1] = b;
}

// +0x204
bool BotMain::hasSameAssignedAreas(int a)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::hasSameAssignedAreas");
}

// +0x208
void BotMain::copyAssignedAreas()
{
    m_assignedAreas[2] = m_assignedAreas[0];
    m_assignedAreas[3] = m_assignedAreas[1];
}

// +0x20C
void BotMain::setCurrentWaypoint(Vec3 v, int i)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setCurrentWaypoint");
}

// +0x210
const Vec3& BotMain::getCurrentWaypoint()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::getCurrentWaypoint");
}

// +0x214
void BotMain::resetAssignedAreas()
{
    // only two of the four. The copies at [1] and [3] are left alone; reproduced.
    m_assignedAreas[0] = -1;
    m_assignedAreas[2] = -1;
}

// +0x218
void BotMain::setTimeOfLastFailedStaticSkyAttack(double t)
{
    m_timeOfLastFailedStaticSkyAttack = t;
}

// +0x21C
double BotMain::getTimeOfLastFailedStaticSkyAttack() const
{
    return m_timeOfLastFailedStaticSkyAttack;
}

// +0x220
void BotMain::setTimeOfLastFailedStaticAttack(double t)
{
    m_timeOfLastFailedStaticAttack = t;
}

// +0x224
double BotMain::getTimeOfLastFailedStaticAttack() const
{
    return m_timeOfLastFailedStaticAttack;
}

// +0x228
void BotMain::setBotWantsToSimulateArtillery(bool b)
{
    m_wantsToSimulateArtillery = b;
}

// +0x22C
bool BotMain::getBotWantsToSimulateArtillery() const
{
    return m_wantsToSimulateArtillery;
}

// +0x230
void BotMain::setBotIsSimulatingArtillery(bool b)
{
    m_isSimulatingArtillery = b;
}

// +0x234
bool BotMain::getBotIsSimulatingArtillery() const
{
    return m_isSimulatingArtillery;
}

// +0x238
void BotMain::setAvoidSide(int side)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setAvoidSide");
}

// +0x23C
int BotMain::getAvoidSide() const
{
    return m_avoidSide;
}

// +0x240
int BotMain::getMoveToStatus() const
{
    return m_moveToStatus;
}

// +0x244
void BotMain::setMoveToStatus(int status)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setMoveToStatus");
}

// +0x248
void BotMain::setCollidedObject(unsigned int h)
{
    m_collidedObject = h;
}

// +0x24C
unsigned int BotMain::getCollidedObject() const
{
    return m_collidedObject;
}

// +0x250
void BotMain::setCollidedObjectPosition(const Vec3& v)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setCollidedObjectPosition");
}

// +0x254
const Vec3& BotMain::getCollidedObjectPosition() const
{
    return *reinterpret_cast<const Vec3*>(m_collidedObjectPosition);
}

// +0x258
void BotMain::setIgnoreObstacles(bool b)
{
    // it also clears the avoid side - one setter, two fields. Reproduced.
    m_ignoreObstacles = b;
    m_avoidSide = 0;
}

// +0x25C
bool BotMain::getIgnoreObstacles() const
{
    return m_ignoreObstacles;
}

// +0x260
void BotMain::setSlowDownToTarget(unsigned int h)
{
    m_slowDownToTarget = h;
}

// +0x264
unsigned int BotMain::getSlowDownToTarget() const
{
    return m_slowDownToTarget;
}

// +0x268
double BotMain::getFriendsInLineOfFireTime() const
{
    return m_friendsInLineOfFireTime;
}

// +0x26C
void BotMain::setFriendsInLineOfFireTime(double t)
{
    m_friendsInLineOfFireTime = t;
}

// +0x270
void BotMain::setLastCheckMoveToFinishedTime(double t)
{
    m_lastCheckMoveToFinishedTime = t;
}

// +0x274
double BotMain::getLastCheckMoveToFinishedTime() const
{
    return m_lastCheckMoveToFinishedTime;
}

// +0x278
void BotMain::setIsGoingForFlag(bool b)
{
    m_isGoingForFlag = b;
}

// +0x27C
bool BotMain::getIsGoingForFlag() const
{
    return m_isGoingForFlag;
}

// +0x280
bool BotMain::botInPoseTransition() const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::botInPoseTransition");
}

// +0x284
void BotMain::setUseStaticAttack(bool b)
{
    m_useStaticAttack = b;
}

// +0x288
bool BotMain::getUseStaticAttack() const
{
    return m_useStaticAttack;
}

// +0x28C
void BotMain::setTimeOfLastFailedSplashDamageAttack(double t)
{
    m_timeOfLastFailedSplashDamageAttack = t;
}

// +0x290
double BotMain::getTimeOfLastFailedSplashDamageAttack() const
{
    return m_timeOfLastFailedSplashDamageAttack;
}

// +0x294
void BotMain::setTakeOffPosition(const Vec3& v)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setTakeOffPosition");
}

// +0x298
const Vec3& BotMain::getTakeOffPosition() const
{
    return *reinterpret_cast<const Vec3*>(m_takeOffPosition);
}

// +0x29C
void BotMain::setQuadSeen()
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::setQuadSeen");
}

// +0x2A0
bool BotMain::isAttacking(unsigned int handle, double& out) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::isAttacking");
}

// +0x2A4
void BotMain::event_shotTimeOut(int i)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::event_shotTimeOut");
}

// +0x2A8
void BotMain::event_firing(bool& b, int i, const Vec3& v)
{
    BF2_AI_FRONTIER("dice::hfe::ai::BotMain::event_firing");
}

} // namespace ai
} // namespace hfe
} // namespace dice
