// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining AIEventCourier operations.

#include "AIEventCourier.h"

#include <string>

#include "../../../../Core/ClassManager.h"
#include "../../../../Core/Debug.h"                            // the map-part events' fatal Debugs
#include "CommonSingletonServer.h"
#include "AIDllFrontier.h"
#include "IAIPathfinding.h"                                  // IAIPathfindingSetup::initMaps
#include "../AIStrategicEntity/AIStrategicObjectManager.h"   // postLoadInitAreas
#include "AIMain.h"                                            // IAIMain::spawnBots
#include "../AIBot/SquadManager.h"                              // squad notification forwarders

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// IAIEventCourier
IAIEventCourier::IAIEventCourier()
{
    if (!classManager->getSimpleSingleton("IAIEventCourier"))
        classManager->registerSimpleSingleton("IAIEventCourier", this);
}

IAIEventCourier::~IAIEventCourier()
{
    if (classManager->getSimpleSingleton("IAIEventCourier") == this)
        classManager->unregisterSimpleSingleton("IAIEventCourier");
}

// Stateless event courier; construction requires only the base-class initialization.
AIEventCourier::AIEventCourier()
{
}

// The vptr reset and the base call, which again is what C++ emits.
AIEventCourier::~AIEventCourier()
{
}

void AIEventCourier::eventMissileLock(IAIObject *, world::MissileLockMessage *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::eventMissileLock");
}

int AIEventCourier::eventPlayerSpawned(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::eventPlayerSpawned");
}

void AIEventCourier::eventPlayerKilled(int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::eventPlayerKilled");
}

// Notifies the active AI runtime that a player was created, if the runtime exists.
void AIEventCourier::eventPlayerCreated(bool isAI)
{
    if (IAIMain::m_instance)
        IAIMain::m_instance->newPlayerCreated(isAI);      // IAIMain vtable +0x20
}

void AIEventCourier::eventChangedKit(KitType)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::eventChangedKit");
}

void AIEventCourier::eventPlayerDisconnect(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::eventPlayerDisconnect");
}

int AIEventCourier::eventPlayerEnterVehicle(int, IAIObject *, IAIObject *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::eventPlayerEnterVehicle");
}

int AIEventCourier::eventPlayerExitVehicle(IAIObject *, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::eventPlayerExitVehicle");
}

void AIEventCourier::eventDrawDebug()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::eventDrawDebug");
}

// Starts bot spawning when the game-start event arrives.
// [CHOSEN, not recovered] Returns zero; the original scalar return contract is unverified.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
int AIEventCourier::eventGameStart()
{
    IAIMain::m_instance->spawnBots();       // IAIMain vtable +0x54

    return 0;   // [CHOSEN, not recovered] - see above
}

void AIEventCourier::event_roundEnd()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_roundEnd");
}

void AIEventCourier::event_worldReset()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_worldReset");
}

int AIEventCourier::eventManDown(int, IAIObject *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::eventManDown");
}

int AIEventCourier::eventRevived(int, IAIObject *)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::eventRevived");
}

int AIEventCourier::eventRadioMessage(const AIRadioMessageSettings&, int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::eventRadioMessage");
}

int AIEventCourier::event_mineRemoved(unsigned int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_mineRemoved");
}

int AIEventCourier::event_enemySpotted(IAIObject *, unsigned char)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_enemySpotted");
}

int AIEventCourier::event_isRepaired(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_isRepaired");
}

void AIEventCourier::event_requestNewOrders(int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_requestNewOrders");
}

void AIEventCourier::event_recievedAttackCommanderOrder(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_recievedAttackCommanderOrder");
}

void AIEventCourier::event_recievedDefendCommanderOrder(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_recievedDefendCommanderOrder");
}

void AIEventCourier::event_recievedMoveCommanderOrder(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_recievedMoveCommanderOrder");
}

void AIEventCourier::event_incomingArtillery(int, Vec2)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_incomingArtillery");
}

int AIEventCourier::event_receivedArtilleryRequest(int, int, Vec3)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_receivedArtilleryRequest");
}

int AIEventCourier::event_receivedUavRequest(int, int, Vec3)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_receivedUavRequest");
}

int AIEventCourier::event_receivedSupplyRequest(int, int, Vec3)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_receivedSupplyRequest");
}

int AIEventCourier::event_receivedVehicleDropRequest(int, int, Vec3)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_receivedVehicleDropRequest");
}

// Both guard the singleton and forward the
// unchanged squad id through IAIMain +0x68 / +0x64. The concrete AIMain callback is empty
// in both images ; this courier still performs the virtual call.
void AIEventCourier::event_commanderOrderAccepted(int squadId)
{
    if (IAIMain::m_instance)
        IAIMain::m_instance->commanderOrderAccepted(squadId);
}

void AIEventCourier::event_commanderOrderRejected(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_commanderOrderRejected");
}

int AIEventCourier::event_appliedForCommander(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_appliedForCommander");
}

int AIEventCourier::event_acceptedCommander(int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_acceptedCommander");
}

void AIEventCourier::event_resignAsCommander(int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_resignAsCommander");
}

// Only the SquadManager singleton is guarded;
// the manager receives the original one-based side and squad id without validation here.
void AIEventCourier::eventNewSquadLeader(int side, int squadId)
{
    if (SquadManager::m_instance)
        SquadManager::m_instance->newSquadLeader(side, squadId);
}

// The order is player id, squad id, side.
// Both images pass all three integers unchanged after testing the manager singleton.
void AIEventCourier::event_joinedSquad(int playerId, int squadId, int side)
{
    if (SquadManager::m_instance)
        SquadManager::m_instance->playerJoinsSquad(playerId, squadId, side);
}

void AIEventCourier::event_removedFromSquad(int, int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_removedFromSquad");
}

void AIEventCourier::event_recievedAttackSquadOrder(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_recievedAttackSquadOrder");
}

void AIEventCourier::event_recievedDefendSquadOrder(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_recievedDefendSquadOrder");
}

void AIEventCourier::event_recievedMoveSquadOrder(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_recievedMoveSquadOrder");
}

void AIEventCourier::event_receivedRepairSquadOrder(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_receivedRepairSquadOrder");
}

void AIEventCourier::event_squadOrderAccepted(int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_squadOrderAccepted");
}

void AIEventCourier::event_squadOrderRejected(int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_squadOrderRejected");
}

int AIEventCourier::event_botTeamKilled(int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_botTeamKilled");
}

int AIEventCourier::event_freeCameraFire(bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_freeCameraFire");
}

void AIEventCourier::event_removeAllSquadMembers()
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_removeAllSquadMembers");
}

// Forwards destroyable-map and bridge events to pathfinding and strategic owners.
// Requires initialized owners; negative map-part IDs emit an error diagnostic.
int AIEventCourier::event_retrievePathfindingMapId(int& mapPartId, bool walkable, const Vec3& minCorner,
                                                   const Vec3& maxCorner, const Mat4& transformation)
{
    IAIPathfinding::instance->initializeDestroyableMapPart(mapPartId, walkable, minCorner, maxCorner,
                                                           transformation);   // +0xE0 / PE +0xDC
    return 0;   // [CHOSEN, not recovered] - see above
}

int AIEventCourier::event_pathfindingMapIdDestroyed(int mapPartId, IAIObject* object)
{
    if (mapPartId < 0)
    {
        Debug(DebugTypeError, "AI/AIDLL/AI/Main/AIEventCourier.cpp", 508, "")
            << "Object destroyed that has unknown connection to the the pathfinding.";
    }
    else
    {
        IAIPathfinding::instance->destroyMapPart(mapPartId);                    // +0xE4 / PE +0xE0
        if (object)
            AIStrategicObjectManager::getInstance()->removeBridgeSegment(object);
    }
    return 0;   // [CHOSEN, not recovered] - see above
}

int AIEventCourier::event_pathfindingMapIdRestored(int mapPartId)
{
    IAIPathfinding::instance->restoreMapPart(mapPartId);                        // +0xE8 / PE +0xE4
    return 0;   // [CHOSEN, not recovered] - see above
}

int AIEventCourier::event_pathfindingMapIdRestoreOriginalMaterials(int mapPartId, IAIObject* object)
{
    if (mapPartId < 0)
    {
        Debug(DebugTypeError, "AI/AIDLL/AI/Main/AIEventCourier.cpp", 532, "")
            << "Object destroyed that has unknown connection to the the pathfinding.";
    }
    else
    {
        IAIPathfinding::instance->resetOriginalMaterialOfDestroyableMapPart(mapPartId);   // +0xEC / PE +0xE8
        if (object)
            AIStrategicObjectManager::getInstance()->removeBridgeSegment(object);
    }
    return 0;   // [CHOSEN, not recovered] - see above
}

// The int lands in the segment's +4, [UNRESOLVED] (see AIStrategicObjectManager::BridgeSegment).
int AIEventCourier::event_bridgeDestroyed(IAIObject* object, int value)
{
    AIStrategicObjectManager::getInstance()->bridgeSegmentDestroyed(object, value);
    return 0;   // [CHOSEN, not recovered] - see above
}

int AIEventCourier::event_bridgeRepaired(IAIObject* object)
{
    AIStrategicObjectManager::getInstance()->bridgeSegmentRepaired(object);
    return 0;   // [CHOSEN, not recovered] - see above
}

int AIEventCourier::event_bridgeCreated(IAIObject* object)
{
    AIStrategicObjectManager::getInstance()->createBridgeSegment(object);
    return 0;   // [CHOSEN, not recovered] - see above
}

int AIEventCourier::event_bridgeRemoved(IAIObject* object)
{
    AIStrategicObjectManager::getInstance()->removeBridgeSegment(object);
    return 0;   // [CHOSEN, not recovered] - see above
}

// Initializes strategic areas, then pathfinding maps after level loading.
int AIEventCourier::event_levelLoaded()
{
    AIStrategicObjectManager::getInstance()->postLoadInitAreas();
    IAIPathfindingSetup::instance->initMaps();

    return 0;   // [CHOSEN, not recovered] - see above
}

int AIEventCourier::event_botSwitchSides(int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AIEventCourier::event_botSwitchSides");
}

// Forwards a side switch when the IAIMain singleton exists.
void AIEventCourier::event_playerSwitchSides(int side, int playerId)
{
    if (IAIMain::m_instance)
        IAIMain::m_instance->switchSides(side, playerId);      // IAIMain vtable +0x2C
}

} // namespace ai
} // namespace hfe
} // namespace dice
