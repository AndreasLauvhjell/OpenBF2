// Shared game-logic interface for client/server lifecycle, content and gameplay operations.
// Use release() for lifetime management; no destructor may be inserted into the early interface slots.
// [CHOSEN] Return types without caller evidence remain provisional; preserve the declared ABI.

#ifndef GAME_IGAMELOGIC_H       // [INFERRED] guard style
#define GAME_IGAMELOGIC_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "../Core/WeakPtr.h"            // WeakPtr<world::IPlayer> - giveDamage's list parameter

namespace dice {
namespace hfe {

// Opaque type declarations; enums specify their underlying type for by-value parameters.
class  EventNode;
struct TicketHistory;
enum   EventCategory : int;             // Defined in Game/IEventListener.h.
                                        // Value 1 triggers a player's hit indicator when enabled. Other enumerators remain [UNRESOLVED].
enum   SpecialGameEventType : int { SGE_HitIndicator = 1 };
enum   BeginRoundState      : int {};   // [UNRESOLVED] enumerators

// Game-state values; names are [CHOSEN] from status messages. Interfaces expose the state as an integer.
enum   GameStatus
{
    GameStatus_Playing      = 1,
    GameStatus_EndGame      = 2,
    GameStatus_PreGame      = 3,
    GameStatus_Paused       = 4,
    GameStatus_Restart      = 5,
    GameStatus_Disconnected = 6
};

namespace io { class PlayerInput; }

namespace world {
class  IArmorGLComp;
class  IObject;
class  IObjectTemplate;
class  IPlayer;
class  IProjectileObject;
class  StrategicObjectHolder;
class  CollisionFaceInfo;
enum   MaterialCollisionType : int {};  // [UNRESOLVED] enumerators
enum   TargetType            : int {};  // [UNRESOLVED] enumerators
}

namespace Math { struct Vec3; struct Vec4; }    // the typedef reality - see Math/Vec3.h
typedef Math::Vec3 Vec3;
typedef Math::Vec4 Vec4;

class IGameLogic
{
public:
    virtual void  addRef() = 0;                             // +0x00
    virtual int   getRef() = 0;                             // +0x04
    virtual int   release() = 0;                            // +0x08  <- the destruction path
    virtual void* queryInterface(unsigned int) const = 0;   // +0x0C

    // NO destructor here. See the header block; adding one shifts every slot below.

    virtual bool  init() = 0;                               // +0x10 [S] bool attested by use
    virtual bool  load(const std::string& level) = 0;       // +0x14  [CHOSEN] return
    virtual bool  unload() = 0;                             // +0x18 [S] bool attested by GameServer
    virtual bool  close() = 0;                              // +0x1C [S] PE close bodies explicitly clear AL
    virtual void  update(float deltaTime) = 0;              // +0x20 [S]
    virtual void  reset() = 0;                              // +0x24 [S]
    virtual void  restartDemoPlayback() = 0;                // +0x28
    virtual void  runNextLevel() = 0;                       // +0x2C [S]

    // Opaque void* results stand in for unidentified types; replace them only when the concrete type is known.
    // Return types remain [CHOSEN] unless documented otherwise.
    virtual void  setNextLevel(int) = 0;                                        // +0x30 [S]
    virtual void  addPreStoredTemplate(unsigned int, unsigned int,
                                       unsigned int) = 0;                       // +0x34
    virtual void  createPreStoredObjects() = 0;                                 // +0x38
    virtual unsigned int getStartPreStoredNetID() const = 0;                    // +0x3C
    virtual unsigned int getEndPreStoredNetID() const = 0;                      // +0x40
    virtual void  setTeamName(int team, const std::string&) = 0;                // +0x44
    // Returns a borrowed team name, or a static empty string when absent.
    virtual const std::string& getTeamName(int team) = 0;                       // +0x48
    virtual int   getTeamFromName(const std::string&) = 0;                      // +0x4C
    // Returns a borrowed team drop-vehicle template name.
    virtual const std::string& getTeamDropVehicle(int team) const = 0;           // +0x50
    virtual void  setNumDropVehiclesForTeam(int team, int n) = 0;               // +0x54
    virtual int   getNumDropVehiclesForTeam(int team) = 0;                      // +0x58
    virtual void  setDropVehicleDeviationRadius(int, int) = 0;                  // +0x5C
    virtual int   getDropVehicleDeviationRadius() = 0;                          // +0x60
    virtual int   getDropVehicleDeviationMinRadius() = 0;                       // +0x64
    virtual void  suicide() = 0;                                                // +0x68 [S]
    // Spawn actions return whether they succeeded; respawn forwards the kill result and rejects a null player.
    virtual bool  killPlayer(world::IPlayer*, bool, bool) = 0;                  // +0x6C [S]
    virtual bool  reSpawnPlayer(world::IPlayer*) = 0;                           // +0x70 [S]
    virtual bool  selectSpawnGroup(world::IPlayer*, int) = 0;                   // +0x74 [S]
    virtual bool  selectTeam(world::IPlayer*, int) = 0;                         // +0x78 [S]
    virtual bool  selectKitAndUnlockLevel(world::IPlayer*, int, int) = 0;       // +0x7C [S] bool, not void - ServerGameLogic 0x80FBDA0 materialises eax on both paths (0/1);
    virtual bool  isLevelLoadedXP() = 0;                                        // +0x80
    virtual bool  getNoVehiclesMode() = 0;                                      // +0x84
    virtual void  setNoVehiclesMode(bool) = 0;                                  // +0x88
    virtual void  setTicketState(unsigned int team, int) = 0;                   // +0x8C [S]
    virtual int   getTicketState(unsigned int team) = 0;                        // +0x90 [S]
    virtual void  setDefaultNumberOfTickets(unsigned int, unsigned int) = 0;    // +0x94 [S]
    virtual unsigned int getDefaultNumberOfTickets(unsigned int) = 0;           // +0x98 [S]
    virtual void  setDefaultNumberOfTicketsEx(unsigned int, unsigned int,
                                              unsigned int) = 0;                // +0x9C [S]
    virtual unsigned int getDefaultNumberOfTicketsEx(unsigned int,
                                                     unsigned int) = 0;         // +0xA0 [S]
    virtual void  setDefaultTimeToNextAIWave(float) = 0;                        // +0xA4 [S]
    virtual float getDefaultTimeToNextAIWave() = 0;                             // +0xA8 [S]
    virtual void  setTicketLossAtEndPerMin(float) = 0;                          // +0xAC [S]
    virtual float getTicketLossAtEndPerMin() = 0;                               // +0xB0 [S]
    virtual void  setTicketLossPerMin(unsigned int team, float) = 0;            // +0xB4 [S]
    virtual float getTicketLossPerMin(unsigned int team) = 0;                   // +0xB8 [S]
    virtual void  setMaximumLevelViewDistance(float) = 0;                       // +0xBC
    virtual float getMaximumLevelViewDistance() const = 0;                      // +0xC0
    virtual void  setExtraUAVVehicleFlightHeight(float) = 0;                    // +0xC4
    virtual float getExtraUAVVehicleFlightHeight() const = 0;                   // +0xC8
    virtual void  setFogStartEndAndBase(const Vec4&) = 0;                       // +0xCC
    virtual const Vec4& getFogStartEndAndBase() const = 0;                      // +0xD0
    virtual void  setDefaultLevel(const std::string&) = 0;                      // +0xD4
    virtual const std::string& getDefaultLevel() const = 0;                     // +0xD8
    virtual void  refreshTriggerables() = 0;                                    // +0xDC
    virtual void  playEffect(Vec3, const world::IObjectTemplate*, Vec3) = 0;    // +0xE0 [S]
    // Returns the team-hit flag; the base implementation returns false.
    virtual bool  giveDamage(world::IObject*, float, world::IPlayer*,
                             std::list<WeakPtr<world::IPlayer> >*,
                             world::IObject*, int, world::IObject*,
                             world::IObject*, Vec3, Vec3, int,
                             bool, bool, bool) = 0;                             // +0xE4 [S]
    virtual void  handleExplosion(Vec3, float, int, float, world::IPlayer*,
                                  world::IArmorGLComp*, int, world::IObject*,
                                  world::IObject*, float, world::IObject*,
                                  world::IObject*, float, bool, bool) = 0;      // +0xE8 [S]
    virtual void  handlePickup(world::IObject*, world::IPlayer*,
                               world::IObject*, bool) = 0;                      // +0xEC [S]
    virtual void  handleDrop(world::IObject*, world::IPlayer*,
                             world::IObject*, bool) = 0;                        // +0xF0 [S]
    // bool, NOT void (): ServerGameLogic's 0x81018C0 leaves a value in
    // eax on every path - a constant 1 - and StickyCollisionComp branches on it.
    virtual bool  handleCollisionForProjectile(world::IObject*, world::IObject*,
                             const Vec3&, const Vec3&, const Vec3&, int, int,
                             world::CollisionFaceInfo*,
                             world::CollisionFaceInfo*) = 0;                    // +0xF4 [S]
    virtual bool  handleCollisionForReplenisher(bool, world::IObject*,
                             world::IObject*, const Vec3&, const Vec3&,
                             const Vec3&, int, int) = 0;                        // +0xF8 [S]
    // Both specialised handlers materialise and consume AL in their collision-component callers.
    virtual bool  handleCollisionForResurrecter(world::IObject*, world::IObject*,
                             float, float, const Vec3&, const Vec3&,
                             const Vec3&, int, int) = 0;                        // +0xFC [S]
    virtual void  handleCollision(world::IObject*, world::IObject*,
                             const Vec3&, const Vec3&, const Vec3&, int, int,
                             world::CollisionFaceInfo*,
                             world::CollisionFaceInfo*) = 0;                    // +0x100 [S]
    // Returns whether the target was served, allowing Player to record the ability use.
    virtual bool  heal(world::IObject*, float, world::IPlayer*, int, int,
                       float*, bool, bool) = 0;                                 // +0x104 [S]
    virtual bool  replenishAmmo(world::IObject*, float, world::IPlayer*,
                                int, int, float*, bool) = 0;                    // +0x108 [S]
    virtual void  doImpact(int, int, world::MaterialCollisionType,
                           const Vec3&, const Vec3&, const Vec3&,
                           world::IObject*, world::IObject*,
                           world::IProjectileObject*,
                           world::CollisionFaceInfo*, Vec3, bool) = 0;          // +0x10C
    // [CHOSEN, not recovered] void return: original callers discard the value and current fixtures use void (RW-254).
    virtual void  playCollisionEffect(Vec3, const world::IObjectTemplate*,
                                      Vec3, Vec3) = 0;                          // +0x110
    virtual void  spawnMultipleObjects(const world::IObjectTemplate*, int) = 0; // +0x114 [S]
    virtual world::IObject* spawnObject(const std::string&, Vec3, Vec3,
                                        int) = 0;                               // +0x118 [S]
    virtual world::IObject* spawnObject(const world::IObjectTemplate*, Vec3,
                                        Vec3, int) = 0;                         // +0x11C [S]
    virtual void  triggerSpecialGameEvent(SpecialGameEventType, void*) = 0;     // +0x120 [S]
    virtual bool  remoteCommand(int, int, int, int, int) = 0;                   // +0x124  / PE 0x465464 return true
    virtual void  setUseActiveCombatArea(bool) = 0;                             // +0x128
    virtual bool  getUseActiveCombatArea() = 0;                                 // +0x12C
    virtual void  setActiveCombatArea(float, float, float, float) = 0;          // +0x130
    virtual void  getActiveCombatArea(float*, float*, float*, float*) = 0;      // +0x134
    virtual float getTimeUntilRestartMap() = 0;                                 // +0x138

    // +0x13C - bool attested BY USE: BF2Engine::recordDemo and stopRecording both `test al` it.
    virtual bool  isServer() const = 0;

    virtual void  setKit(int, int, const std::string&, const std::string&) = 0; // +0x140
    virtual world::IObjectTemplate* getKitTemplate(int, int) = 0;               // +0x144
    virtual std::string getKitTemplateString(int, int) = 0;                     // +0x148
    virtual std::string getSoldierSkin(int, int) = 0;                           // +0x14C
    virtual void* getKits() const = 0;                                          // +0x150 opaque
    virtual void  checkPlayerTriggers(world::IPlayer*, world::IObject*,
                                      io::PlayerInput, bool) = 0;               // +0x154 [S]

    // State 1 is ordinary play; other states allow watchdog polling during long-running operations.
    virtual int   getGameStatus() = 0;

    virtual BeginRoundState getBeginRoundState() = 0;                           // +0x15C
    virtual void  setBeginRoundState(BeginRoundState) = 0;                      // +0x160
    virtual unsigned int getNextStateChangeAtTick() = 0;                        // +0x164
    virtual void  setNextStateChangeAtTick(unsigned int) = 0;                   // +0x168
    virtual float getTimeToNextStateChange() = 0;                               // +0x16C
    virtual void  addControlPoint(world::IObject*, int) = 0;                    // +0x170
    // Both shipped bodies return the address of the object-id -> control-point registry, which
    // several callers iterate directly.
    virtual std::map<unsigned int, world::IObject*>* getControlPointMap() const = 0; // +0x174
    virtual world::IObject* getControlPoint(unsigned int) = 0;                  // +0x178
    virtual void  removeControlPoint(world::IObject*) = 0;                      // +0x17C
    // bool is observed by the client map painter immediately after the +0x180
    // dispatch. Both shipped bodies return false only for a stale group entry.
    virtual bool  getControlPointsInGroup(std::vector<world::IObject*>&,
                                          int group) = 0;                       // +0x180
    virtual int   getNumberOfControlPointGroups() = 0;                          // +0x184
    virtual void  addFlag(world::IObject*) = 0;                                 // +0x188
    virtual void* getFlagMap() const = 0;                                       // +0x18C opaque
    virtual void  removeFlag(world::IObject*) = 0;                              // +0x190
    virtual void  addPco(world::IObject*) = 0;                                  // +0x194
    // The client map painter iterates this registry directly; both shipped
    // bodies return the address of the object-id -> PCO map.
    virtual std::map<unsigned int, world::IObject*>* getPcoMap() const = 0;      // +0x198
    virtual void  removePco(world::IObject*) = 0;                               // +0x19C
    virtual void  addStrategicObject(world::IObject*) = 0;                      // +0x1A0
    // Unlike the other registry getters this is non-const: shipped callers mutate holder state.
    virtual std::map<std::string, world::StrategicObjectHolder*>*
                  getStrategicObjectMap() = 0;                                  // +0x1A4
    virtual void  removeStrategicObject(world::IObject*) = 0;                   // +0x1A8
    virtual void  addTargetObject(world::IObject*) = 0;                         // +0x1AC
    virtual void* getTargetObjectMap() const = 0;                               // +0x1B0 opaque
    virtual void  removeTargetObject(world::IObject*) = 0;                      // +0x1B4
    virtual world::IObject* getTargetObject(unsigned int) const = 0;            // +0x1B8
    virtual world::IObject* findBestTargetObject(const Vec3&, const Vec3&,
                             world::TargetType, float, float, world::IObject*,
                             world::IObject*, float, const world::IObject*,
                             bool, bool, bool) = 0;                             // +0x1BC
    virtual void  addRemoteControlledObject(world::IObject*) = 0;               // +0x1C0
    virtual void* getRemoteControlledObjectMap() const = 0;                     // +0x1C4 opaque
    virtual void  removeRemoteControlledObject(world::IObject*) = 0;            // +0x1C8
    virtual world::IObject* getRemoteControlledObject(unsigned int) const = 0;  // +0x1CC
    virtual void  addProjectile(world::IObject*) = 0;                           // +0x1D0
    virtual void* getProjectileMap() const = 0;                                 // +0x1D4 opaque
    virtual void  removeProjectile(world::IObject*) = 0;                        // +0x1D8
    virtual void  addObjectSpawner(world::IObject*) = 0;                        // +0x1DC
    virtual void* getObjectSpawnerMap() const = 0;                              // +0x1E0 opaque
    virtual void  removeObjectSpawner(world::IObject*) = 0;                     // +0x1E4
    virtual world::IObject* getObjectSpawner(unsigned int) const = 0;           // +0x1E8
    virtual void  addSupplyDepot(world::IObject*) = 0;                          // +0x1EC
    virtual void* getSupplyDepotMap() const = 0;                                // +0x1F0 opaque
    virtual void  removeSupplyDepot(world::IObject*) = 0;                       // +0x1F4
    virtual void  setTeamLanguage(int, const std::string&) = 0;                 // +0x1F8
    // Returns a borrowed reference to the team's language string.
    virtual const std::string& getTeamLanguage(int) const = 0;                  // +0x1FC
    virtual void  setTeamFlag(int, const std::string&) = 0;                     // +0x200
    // Returns the team's flag string by reference.
    virtual const std::string& getTeamFlag(int) const = 0;                      // +0x204
    virtual void  toggleLocalFreeCamera() = 0;                                  // +0x208 [S]
    // Returns a copy of the configured pre-spawn camera transform component.
    virtual Vec3  getBeforeSpawnCameraPosition() const = 0;                     // +0x20C
    virtual Vec3  getBeforeSpawnCameraDirection() const = 0;                    // +0x210
    virtual void  resetPlayerFreeCameraPosition(world::IPlayer*) = 0;           // +0x214
    // Default client pause/statistics operations are empty or false; ServerGameLogic overrides them.
    virtual void  togglePause() {}                                              // +0x218 [S]
    virtual bool  isLevelLoaded() const = 0;                                    // +0x21C
    virtual void  setPause(bool) {}                                             // +0x220 [S]
    virtual bool  getPause() const { return false; }                            // +0x224 [S]
    virtual bool  isPrecaching() const = 0;                                     // +0x228
    virtual bool  getEnableVoHelp() = 0;                                        // +0x22C
    virtual void  stopPlayingVoMessages() = 0;                                  // +0x230
    virtual void  setSupplyDropHeight(float) = 0;                               // +0x234
    virtual float getSupplyDropHeight() = 0;                                    // +0x238
    virtual void  setVehicleDropHeight(float) = 0;                              // +0x23C
    virtual float getVehicleDropHeight() = 0;                                   // +0x240
    virtual void  setSupplyDropNumSecsToLive(float) = 0;                        // +0x244
    virtual float getSupplyDropNumSecsToLive() = 0;                             // +0x248
    virtual void  setFeslEnvironment(int) = 0;                                  // +0x24C
    virtual int   getFeslEnvironment() = 0;                                     // +0x250
    virtual void  storeVoipServerInfo(bool, unsigned long, unsigned short,
                                      unsigned short, int) = 0;                 // +0x254
    virtual void  getVoipServerInfo(bool&, unsigned int&, unsigned short&,
                                    unsigned short&, int&) = 0;                 // +0x258
    virtual void  startupVoipClient() = 0;                                      // +0x25C
    virtual bool  isVoipClientRunning() = 0;                                    // +0x260
    virtual bool  getOutputPlayerStats() { return false; }                      // +0x264 [S]
    virtual bool  getAirMouseInverted() = 0;                                    // +0x268
    virtual void  setAirMouseSensitivity(float) = 0;                            // +0x26C
    virtual float getAirMouseSensitivity() = 0;                                 // +0x270
    virtual void  becomeSquadLeader(bool) = 0;                                  // +0x274
    // Return constness resolved by python::gl_sendEndOfRoundData: the shipped caller appends its
    // newly assembled Ticket directly through this reference. It is mutable, not const.
    virtual TicketHistory& getTicketHistory() = 0;                              // +0x278 [S]
    virtual void  setTicketHistory(const TicketHistory&) = 0;                   // +0x27C [S]
    virtual void  setHitIndicationThreshold(float) = 0;                         // +0x280 [S]
    virtual float getHitIndicationThreshold() = 0;                              // +0x284 [S]
    virtual float getUAVActiveTime() const = 0;                                 // +0x288
    virtual float getSatelliteScanActiveTime() const = 0;                       // +0x28C
    virtual float getUAVReloadTime() const = 0;                                 // +0x290
    virtual float getUAVRadius() const = 0;                                     // +0x294
    virtual float getSatelliteScanReloadTime() const = 0;                       // +0x298
    virtual float getSatelliteScanVisibleTime() const = 0;                      // +0x29C
    // Returns RendDX9's opaque ground DecalObject, despite the [INFERRED] IObject* signature.
    // Pass it only to decal helpers; never dereference it as IObject.
    virtual world::IObject* getGroundDecalObject() = 0;                         // +0x2A0
    // these four take std::string BY value - the manglings say Ss, not RKSs - and two carry
    // dice's own inconsistent capitalisation, reproduced exactly.
    virtual void  noVehiclesOmit(std::string) = 0;                              // +0x2A4
    virtual bool  isOmitted(std::string) = 0;                                   // +0x2A8
    virtual void  AddInvalidDropVehicleObject(std::string, bool) = 0;           // +0x2AC
    virtual std::list<std::pair<std::string, bool> >*
                  GetInvalidVehicleDropObjectList() = 0;                        // +0x2B0
    virtual void  setDebugMissiles(int) = 0;                                    // +0x2B4
    virtual int   getDebugMissiles() = 0;                                       // +0x2B8
    virtual void  setDebugReplenish(int) = 0;                                   // +0x2BC
    virtual int   getDebugReplenish() = 0;                                      // +0x2C0

    // +0x2C4 / +0x2C8: ServerGameLogic's OWN virtual destructor pair - the first class in the
    // chain to declare one, so its slots land here, not at the top. Modelled as the point the
    // interface gains a virtual dtor; nothing dispatches these slots in the recovered engine.
    virtual ~IGameLogic() {}

    virtual void  setTeamDropVehicle(int, const std::string&) = 0;              // +0x2CC
    virtual void  ClearDropVehicles() = 0;                                      // +0x2D0 (dice caps)
    virtual void  handleEvent(EventCategory, unsigned int, EventNode*,
                              void*) = 0;                                       // +0x2D4 [S]
    virtual void  killAllPlayers() = 0;                                         // +0x2D8
    // Returns success; load aborts the remaining precache steps on failure.
    virtual bool  preCacheSpawnedTemplates() = 0;                               // +0x2DC
    virtual void  clearConsole() = 0;                                           // +0x2E0
    virtual bool  loadUpdateComponents() = 0;                                   // +0x2E4 bool - see +0x2DC

    // THE content-load pair - what a started mission drives through the mounted archives.
    virtual bool  loadInitFile(const std::string&) = 0;                         // +0x2E8 [CHOSEN] ret
    virtual bool  loadAllConFiles(const std::string&) = 0;                      // +0x2EC [CHOSEN] ret

    virtual void  loadVoiceMessages() = 0;                                      // +0x2F0
    virtual bool  setCreatedOnLoadFlag() = 0;                                   // +0x2F4 bool - see +0x2DC
    virtual void  createSunTemplate() = 0;                                      // +0x2F8
    virtual void  resetAllPlayers() = 0;                                        // +0x2FC
    virtual void  resetAllObjects() = 0;                                        // +0x300
    // setOutputPlayerStats belongs to ServerGameLogic, not this interface.
    // ClientGameLogic has different subclass virtuals following the shared slots.
};

// ds:0x8ACBFA0. Written only by dice::hfe::setGameLogic(unsigned int).
extern IGameLogic* gameLogic;

// A free function, NOT a member: releases the current gameLogic and createInstance2(classId,
// IID_IGameLogic) into the global. Body: ELF 0x85257E0 (82 B), defined in Core/ModuleRegistry.cpp.
IGameLogic* setGameLogic(unsigned int classId);

} // namespace hfe
} // namespace dice

#endif // GAME_IGAMELOGIC_H
