// Bot implementation and its shared virtual interface.
// Preserve virtual declaration order across the AI module boundary.

#ifndef AI_AIDLL_AI_AIBOT_BOTMAIN_H          // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_BOTMAIN_H

#include "../../../../Math/Vec3.h"

#include <list>
#include <string>
#include <vector>

namespace dice {
namespace hfe {

// Include Vec3's definition; it is a Math alias and cannot be forward-declared as a class.
namespace world { class IObject; }
namespace io { class PlayerInput; }

namespace ai {

class AIStrategicObject;
class BAPContext;
class BotBehaviour;
class IAIObject;
class BotMemory;
class BotNavigation;
class BotOrder;
class BotScript;
class BotSquad;
class CommunicationAgent;
class FireCorrectionData;
class FireObject;
class IAIObject;
class IAIPlayer;
class IBotDeathCallBack;
class IBotSpawnCallBack;
class Information;
class PlanInterpreter;
class SensingAgent;
class SquadMember;
class UrgencyGenerator;
class WaypointList;

enum BotLodLevel { BotLodFull = 0, BotLodMedium = 1, BotLodLight = 2 };  // [CHOSEN] - see BotManager

// Base interface used by the bot manager to schedule and query bots.
// [RETIRE-WHEN: botmain-bases] Remove when the corresponding implementation is complete and validated.
class Bot
{
public:
    virtual ~Bot();     // - one instruction each

    virtual void setLodLevel(BotLodLevel level) = 0;                                              // +0x008
    virtual BotLodLevel getLodLevel() const = 0;                                                  // +0x00C
    virtual void planExecution(const double& slice) = 0;                                          // +0x010 [FRONTIER]
    virtual int decisionMaking(const double& slice) = 0;                                          // +0x014 [FRONTIER]
    virtual void cheatExecution() = 0;                                                            // +0x018
    virtual void see(const double& dt) = 0;                                                       // +0x01C [FRONTIER]
    virtual void updateSensing(const double& dt) = 0;                                             // +0x020 [FRONTIER]
    virtual void updatePathfinding(bool& a, bool* b) = 0;                                         // +0x024 [FRONTIER]
    virtual void reset() = 0;                                                                     // +0x028
    virtual void updateBotVehicle(bool* out) = 0;                                                 // +0x02C [FRONTIER]
    virtual void updateBot() = 0;                                                                 // +0x030 [FRONTIER]
    virtual BotSquad* getBotSquad() const = 0;                                                    // +0x034
    virtual float getObedience() = 0;                                                             // +0x038
    virtual float* getCurrentOrders() = 0;                                                       // +0x03C order weights
    virtual WaypointList* getWaypoints() = 0;                                                     // +0x040
    virtual void setWaypoints(WaypointList* w) = 0;                                               // +0x044 [FRONTIER]
    virtual bool recieveOrder(BotOrder* order) = 0;                                               // +0x048
    virtual void recieveOrder(BotOrder* order, std::list<unsigned int>& targets) = 0;             // +0x04C [FRONTIER]
    virtual std::list<unsigned int>* getOrderList(unsigned char behaviour) = 0;                   // +0x050
    virtual bool hasPlan() = 0;                                                                   // +0x054
    virtual bool hasDefaultPlan() const = 0;                                                      // +0x058
    virtual void invalidatePlan() = 0;                                                            // +0x05C
    virtual BotScript* getPlan() = 0;                                                             // +0x060
    virtual void setPlan(BotScript* plan) = 0;                                                    // +0x064
    virtual bool getPlanLock() const = 0;                                                         // +0x068
    virtual void setPlanLock(bool b) = 0;                                                         // +0x06C
    virtual bool isTakingOrders() = 0;                                                            // +0x070
    virtual bool isOrderedBehaviour(unsigned char behaviour) = 0;                                 // +0x074
    virtual bool isOrderedObject(unsigned int handle, BotBehaviour* b) = 0;                       // +0x078 [FRONTIER]
    virtual void setFiringTarget(unsigned int handle) = 0;                                        // +0x07C [FRONTIER]
    virtual unsigned int getFiringTarget() const = 0;                                             // +0x080
    virtual double getFiringTargetTime() const = 0;                                               // +0x084
    virtual SensingAgent* getSensingAgent() = 0;                                                  // +0x088
    virtual void insertBehaviourInformation(float f, const Vec3& v, unsigned char c) = 0;         // +0x08C [FRONTIER]
    virtual const std::vector<IAIObject*>& getSensedObjects() const = 0;                          // +0x090 [FRONTIER]
    virtual void* getSensedObjectFromHandle(unsigned int handle) const = 0;                       // +0x094 [FRONTIER]
    virtual void setUnresolvedCollisionObject(unsigned int handle) = 0;                           // +0x098
    virtual unsigned int getUnresolvedCollisionHandle() = 0;                                      // +0x09C
    virtual void* getIncommingFire() = 0;                                                         // +0x0A0 [FRONTIER]
    virtual float getIncommingFireTotalStrength() = 0;                                            // +0x0A4 [FRONTIER]
    virtual float getIncommingFireTotalStrengthAgainstSelf() = 0;                                 // +0x0A8 [FRONTIER]
    virtual float getAttackerStrength() const = 0;                                                // +0x0AC [FRONTIER]
    virtual CommunicationAgent* getCommunicationAgent() = 0;                                      // +0x0B0
    virtual int getPlayerId() const = 0;                                                          // +0x0B4
    virtual int getInputId() = 0;                                                                 // +0x0B8 [FRONTIER]
    virtual IAIPlayer* getPlayer() const = 0;                                                     // +0x0BC
    virtual void updatePlayerAction(bool stalled) = 0;                                            // +0x0C0
    virtual bool isDead() const = 0;                                                              // +0x0C4
    virtual bool setSide(int side) = 0;                                                           // +0x0C8 equipment arm [FRONTIER]
    virtual void setName(const std::string& name) = 0;                                            // +0x0CC [FRONTIER]
    virtual bool isDeactivated() const = 0;                                                       // +0x0D0
    virtual void setAsDeactivated(bool b) = 0;                                                    // +0x0D4
    virtual unsigned int getCurrentEquipment() const = 0;                                         // +0x0D8
    virtual void setCurrentEquipment(unsigned int handle) = 0;                                    // +0x0DC
    virtual int getSide() const = 0;                                                              // +0x0E0
    virtual int getBotType() const;                                                               // +0x0E4
    virtual unsigned int getBotId() const = 0;                                                    // +0x0E8
    virtual BotMemory* getMemory() = 0;                                                           // +0x0EC
    virtual const std::string& getName() const = 0;                                               // +0x0F0 [FRONTIER]
    virtual void setTimeOfLastGrenadeThrown(double t) = 0;                                        // +0x0F4 [FRONTIER]
    virtual double getTimeOfGrenadeThrown(int i) const = 0;                                       // +0x0F8
    virtual BotNavigation* getBotNavigation() = 0;                                                // +0x0FC
    virtual int getSpawnPoint() const = 0;                                                        // ELF +0x100 / PE +0xFC
    virtual int getSpawnKit() const = 0;                                                          // +0x104 [FRONTIER]
    virtual void getSpawnTarget(int& a, int& b) const = 0;                                        // +0x108 [FRONTIER]
    virtual void setSpawnTarget(int spawnGroup, int kit) = 0;                                    // ELF +0x10C / PE +0x10C (overload order)
    virtual void setSpawnTarget(int strategicArea, int spawnGroup, int kit) = 0;                 // +0x110
    virtual int getSpawnToStrategicAreaId() = 0;                                                  // +0x114
    virtual bool isJustSpawned() = 0;                                                             // +0x118 [FRONTIER]
    virtual IBotDeathCallBack* getDeathCallBack() const = 0;                                      // +0x11C
    virtual void setDeathCallBack(IBotDeathCallBack* cb) = 0;                                     // +0x120
    virtual IBotSpawnCallBack* getSpawnCallBack() const = 0;                                      // +0x124
    virtual void setSpawnCallBack(IBotSpawnCallBack* cb) = 0;                                     // +0x128
    virtual void setIsFiring(bool b) = 0;                                                         // +0x12C
    virtual bool isFiring() = 0;                                                                  // +0x130
    virtual void setAllowFire(bool b) = 0;                                                        // +0x134
    virtual bool isFireAllowed() = 0;                                                             // +0x138
    virtual void setSquadMember(SquadMember* m) = 0;                                              // +0x13C
    virtual SquadMember* getSquadMember() = 0;                                                    // +0x140
    virtual bool isBotStuck() = 0;                                                                // +0x144
    virtual void setBotStuck(bool b) = 0;                                                         // +0x148
    virtual void placeC4(int placementType, Vec3 v, AIStrategicObject* o) = 0;                    // +0x14C [FRONTIER]
    virtual bool isC4Placed() = 0;                                                                // +0x150
    virtual void removeC4() = 0;                                                                  // +0x154 [FRONTIER]
    virtual void* getC4() = 0;                                                                    // +0x158 [FRONTIER]
    virtual void updateC4() = 0;                                                                  // +0x15C [FRONTIER]
    virtual void enableBotScriptDebugger(bool /*enable*/) = 0;                                    // +0x160
    virtual void* getDebugger() = 0;                                                              // +0x164
    virtual BAPContext* getScriptContext();                                                       // +0x168
    virtual void event_projectile(FireObject* f, unsigned int h) = 0;                             // +0x16C [FRONTIER]
    virtual void event_shotHit(int i, world::IObject* o, const Vec3& v) = 0;                      // +0x170 [FRONTIER]
    virtual void event_shotMissed(int i, const Vec3& v) = 0;                                      // +0x174 [FRONTIER]
    virtual void event_heardObject(unsigned int h) = 0;                                           // +0x178 [FRONTIER]
    virtual bool isInTransitionPhase() const = 0;                                                 // +0x17C
    virtual void setIsInTransitionPhase(bool b) = 0;                                              // +0x180
    virtual void addToMoveInclination(float f) = 0;                                               // +0x184
    virtual void addToFireInclination(float f) = 0;                                               // +0x188
    virtual float getMoveInclination() = 0;                                                       // +0x18C
    virtual float getFireInclination() = 0;                                                       // +0x190
    virtual void requestSoldierPose(int pose, bool b) = 0;                                        // +0x194 [FRONTIER]
    virtual int getRequestedSolderPose() = 0;                                                     // +0x198
    virtual void setPoseStatus(int pose, unsigned int h, bool a, bool b) = 0;                     // +0x19C [FRONTIER]
    virtual void event_airborn() = 0;                                                             // +0x1A0 [FRONTIER]
    virtual bool isAirBorn() const = 0;                                                           // +0x1A4
    virtual void setLastChangeTime(double t) = 0;                                                 // +0x1A8
    virtual void setManDown(bool b) = 0;                                                          // +0x1AC
    virtual bool getManDown() const = 0;                                                          // +0x1B0
    virtual void rearm(Information* i) = 0;                                                       // +0x1B4 [FRONTIER]
    virtual void setChangeTarget(unsigned int h) = 0;                                             // +0x1B8
    virtual unsigned int getChangeTarget() const = 0;                                             // +0x1BC
    virtual void setRepairTarget(unsigned int h) = 0;                                             // +0x1C0
    virtual unsigned int getRepairTarget() const = 0;                                             // +0x1C4
    virtual void setWantedRepairTarget(unsigned int h) = 0;                                       // +0x1C8
    virtual unsigned int getWantedRepairTarget() const = 0;                                       // +0x1CC
    virtual void setBridgeRepairTarget(unsigned int h) = 0;                                       // +0x1D0
    virtual unsigned int getBridgeRepairTarget() const = 0;                                       // +0x1D4
    virtual void setReviveTarget(unsigned int h) = 0;                                             // +0x1D8
    virtual unsigned int getReviveTarget() const = 0;                                             // +0x1DC

    // [INFERRED] friendship, like BotMain's below: BotManager::actionExecutePlan reads +4 inline
    // through a `Bot*` to press a climbing bot's inputs.
    friend class BotManager;

protected:
    // Base construction leaves both pointers uninitialized. Player input is borrowed and
    // bound later; BotMain allocates the script context stored here.
    /*  +4 */ io::PlayerInput* m_playerInput;
    /*  +8 */ BAPContext* m_bapContext;     // BotMain's constructor does `new BAPContext` into it
};

// sizeof == 588 (0x24C), from `operator new(0x24Cu)` in BotManager's constructor. The layout below
// ends with a Vec3 at +576, which lands exactly on 588 - an independent confirmation of the size.
class BotMain : public Bot
{
public:
    // Accepts the player, bot settings, shared memory, interpreter, and urgency generators.
    // The merger is passed through its UrgencyGenerator base interface.
    BotMain(IAIPlayer* player,
            unsigned int id,
            float* weights,
            float f,
            int i,
            BotOrder* order,
            BotMemory* memory,
            PlanInterpreter* interpreter,
            int n,
            UrgencyGenerator* merger,
            UrgencyGenerator** generators);
    ~BotMain();

// Bot virtual interface. Preserve declaration order across the DLL boundary.
// [RETIRE-WHEN: botmain-bases] Remove when the corresponding implementation is complete and validated.
    virtual void setLodLevel(BotLodLevel level);                                                  // +0x008
    virtual BotLodLevel getLodLevel() const;                                                      // +0x00C
    virtual void planExecution(const double& slice);                                              // +0x010 [FRONTIER]
    // [INFERRED] Returns an integer work count accumulated by BotManager.
    virtual int decisionMaking(const double& slice);                                              // +0x014 [FRONTIER]
    virtual void cheatExecution();                                                                // +0x018
    virtual void see(const double& dt);                                                           // +0x01C [FRONTIER]
    virtual void updateSensing(const double& dt);                                                 // +0x020 [FRONTIER]
    virtual void updatePathfinding(bool& a, bool* b);                                             // +0x024 [FRONTIER]
    virtual void reset();                                                                         // +0x028
    virtual void updateBotVehicle(bool* out);                                                     // +0x02C [FRONTIER]
    virtual void updateBot();                                                                     // +0x030 [FRONTIER]
    virtual BotSquad* getBotSquad() const;                                                        // +0x034
    virtual float getObedience();                                                                 // +0x038
    virtual float* getCurrentOrders();                                                           // +0x03C order weights
    virtual WaypointList* getWaypoints();                                                         // +0x040
    virtual void setWaypoints(WaypointList* w);                                                   // +0x044 [FRONTIER]
    virtual bool recieveOrder(BotOrder* order);                                                   // +0x048
    virtual void recieveOrder(BotOrder* order, std::list<unsigned int>& targets);                 // +0x04C [FRONTIER]
    virtual std::list<unsigned int>* getOrderList(unsigned char behaviour);                       // +0x050
    virtual bool hasPlan();                                                                       // +0x054
    virtual bool hasDefaultPlan() const;                                                          // +0x058
    virtual void invalidatePlan();                                                                // +0x05C
    virtual BotScript* getPlan();                                                                 // +0x060
    virtual void setPlan(BotScript* plan);                                                        // +0x064
    virtual bool getPlanLock() const;                                                             // +0x068
    virtual void setPlanLock(bool b);                                                             // +0x06C
    virtual bool isTakingOrders();                                                                // +0x070
    virtual bool isOrderedBehaviour(unsigned char behaviour);                                     // +0x074
    virtual bool isOrderedObject(unsigned int handle, BotBehaviour* b);                           // +0x078 [FRONTIER]
    virtual void setFiringTarget(unsigned int handle);                                            // +0x07C [FRONTIER]
    virtual unsigned int getFiringTarget() const;                                                 // +0x080
    virtual double getFiringTargetTime() const;                                                   // +0x084
    virtual SensingAgent* getSensingAgent();                                                      // +0x088
    virtual void insertBehaviourInformation(float f, const Vec3& v, unsigned char c);             // +0x08C [FRONTIER]
    virtual const std::vector<IAIObject*>& getSensedObjects() const;                              // +0x090 [FRONTIER]
    virtual void* getSensedObjectFromHandle(unsigned int handle) const;                           // +0x094 [FRONTIER]
    virtual void setUnresolvedCollisionObject(unsigned int handle);                               // +0x098
    virtual unsigned int getUnresolvedCollisionHandle();                                          // +0x09C
    virtual void* getIncommingFire();                                                             // +0x0A0 [FRONTIER]
    virtual float getIncommingFireTotalStrength();                                                // +0x0A4 [FRONTIER]
    virtual float getIncommingFireTotalStrengthAgainstSelf();                                     // +0x0A8 [FRONTIER]
    virtual float getAttackerStrength() const;                                                    // +0x0AC [FRONTIER]
    virtual CommunicationAgent* getCommunicationAgent();                                          // +0x0B0
    virtual int getPlayerId() const;                                                              // +0x0B4
    virtual int getInputId();                                                                     // +0x0B8 [FRONTIER]
    virtual IAIPlayer* getPlayer() const;                                                         // +0x0BC
    virtual void updatePlayerAction(bool stalled);                                                // +0x0C0
    virtual bool isDead() const;                                                                  // +0x0C4
    virtual bool setSide(int side);                                                               // +0x0C8 equipment arm [FRONTIER]
    virtual void setName(const std::string& name);                                                // +0x0CC [FRONTIER]
    virtual bool isDeactivated() const;                                                           // +0x0D0
    virtual void setAsDeactivated(bool b);                                                        // +0x0D4
    virtual unsigned int getCurrentEquipment() const;                                             // +0x0D8
    virtual void setCurrentEquipment(unsigned int handle);                                        // +0x0DC
    virtual int getSide() const;                                                                  // +0x0E0
    virtual int getBotType() const;                                                               // +0x0E4
    virtual unsigned int getBotId() const;                                                        // +0x0E8
    virtual BotMemory* getMemory();                                                               // +0x0EC
    virtual const std::string& getName() const;                                                   // +0x0F0 [FRONTIER]
    virtual void setTimeOfLastGrenadeThrown(double t);                                            // +0x0F4 [FRONTIER]
    virtual double getTimeOfGrenadeThrown(int i) const;                                           // +0x0F8
    virtual BotNavigation* getBotNavigation();                                                    // +0x0FC
    virtual int getSpawnPoint() const;
    virtual int getSpawnKit() const;                                                              // +0x104 [FRONTIER]
    virtual void getSpawnTarget(int& a, int& b) const;                                            // +0x108 [FRONTIER]
    virtual void setSpawnTarget(int spawnGroup, int kit);
    virtual void setSpawnTarget(int strategicArea, int spawnGroup, int kit);                     // +0x110
    virtual int getSpawnToStrategicAreaId();                                                      // +0x114
    virtual bool isJustSpawned();                                                                 // +0x118 [FRONTIER]
    virtual IBotDeathCallBack* getDeathCallBack() const;                                          // +0x11C
    virtual void setDeathCallBack(IBotDeathCallBack* cb);                                         // +0x120
    virtual IBotSpawnCallBack* getSpawnCallBack() const;                                          // +0x124
    virtual void setSpawnCallBack(IBotSpawnCallBack* cb);                                         // +0x128
    virtual void setIsFiring(bool b);                                                             // +0x12C
    virtual bool isFiring();                                                                      // +0x130
    virtual void setAllowFire(bool b);                                                            // +0x134
    virtual bool isFireAllowed();                                                                 // +0x138
    virtual void setSquadMember(SquadMember* m);                                                  // +0x13C
    virtual SquadMember* getSquadMember();                                                        // +0x140
    virtual bool isBotStuck();                                                                    // +0x144
    virtual void setBotStuck(bool b);                                                             // +0x148
    virtual void placeC4(int placementType, Vec3 v, AIStrategicObject* o);                        // +0x14C [FRONTIER]
    virtual bool isC4Placed();                                                                    // +0x150
    virtual void removeC4();                                                                      // +0x154 [FRONTIER]
    virtual void* getC4();                                                                        // +0x158 [FRONTIER]
    virtual void updateC4();                                                                      // +0x15C [FRONTIER]
    virtual void enableBotScriptDebugger(bool /*enable*/);                                        // +0x160
    virtual void* getDebugger();                                                                  // +0x164
    // Inherits Bot::getScriptContext; do not add a derived override for that slot.
    virtual void event_projectile(FireObject* f, unsigned int h);                                 // +0x16C [FRONTIER]
    virtual void event_shotHit(int i, world::IObject* o, const Vec3& v);                          // +0x170 [FRONTIER]
    virtual void event_shotMissed(int i, const Vec3& v);                                          // +0x174 [FRONTIER]
    virtual void event_heardObject(unsigned int h);                                               // +0x178 [FRONTIER]
    virtual bool isInTransitionPhase() const;                                                     // +0x17C
    virtual void setIsInTransitionPhase(bool b);                                                  // +0x180
    virtual void addToMoveInclination(float f);                                                   // +0x184
    virtual void addToFireInclination(float f);                                                   // +0x188
    virtual float getMoveInclination();                                                           // +0x18C
    virtual float getFireInclination();                                                           // +0x190
    virtual void requestSoldierPose(int pose, bool b);                                            // +0x194 [FRONTIER]
    virtual int getRequestedSolderPose();                                                         // +0x198
    virtual void setPoseStatus(int pose, unsigned int h, bool a, bool b);                         // +0x19C [FRONTIER]
    virtual void event_airborn();                                                                 // +0x1A0 [FRONTIER]
    virtual bool isAirBorn() const;                                                               // +0x1A4
    virtual void setLastChangeTime(double t);                                                     // +0x1A8
    virtual void setManDown(bool b);                                                              // +0x1AC
    virtual bool getManDown() const;                                                              // +0x1B0
    virtual void rearm(Information* i);                                                           // +0x1B4 [FRONTIER]
    virtual void setChangeTarget(unsigned int h);                                                 // +0x1B8
    virtual unsigned int getChangeTarget() const;                                                 // +0x1BC
    virtual void setRepairTarget(unsigned int h);                                                 // +0x1C0
    virtual unsigned int getRepairTarget() const;                                                 // +0x1C4
    virtual void setWantedRepairTarget(unsigned int h);                                           // +0x1C8
    virtual unsigned int getWantedRepairTarget() const;                                           // +0x1CC
    virtual void setBridgeRepairTarget(unsigned int h);                                           // +0x1D0
    virtual unsigned int getBridgeRepairTarget() const;                                           // +0x1D4
    virtual void setReviveTarget(unsigned int h);                                                 // +0x1D8
    virtual unsigned int getReviveTarget() const;                                                 // +0x1DC
    virtual void setIsRepaired(bool b);                                                           // +0x1E0
    virtual bool getIsRepaired() const;                                                           // +0x1E4
    virtual void setIsEngineerTarget(bool b);                                                     // +0x1E8
    virtual bool getIsEngineerTarget() const;                                                     // +0x1EC
    virtual void setShouldWaitForRepairs(bool b);                                                 // +0x1F0
    virtual bool getShouldWaitForRepairs() const;                                                 // +0x1F4
    virtual void setMineTarget(unsigned int h);                                                   // +0x1F8
    virtual unsigned int getMineTarget() const;                                                   // +0x1FC
    virtual void setNewAssignedAreas(int a, int b);                                               // +0x200
    virtual bool hasSameAssignedAreas(int a);                                                     // +0x204 [FRONTIER]
    virtual void copyAssignedAreas();                                                             // +0x208
    virtual void setCurrentWaypoint(Vec3 v, int i);                                               // +0x20C [FRONTIER]
    virtual const Vec3& getCurrentWaypoint();                                                     // +0x210 [FRONTIER]
    virtual void resetAssignedAreas();                                                            // +0x214
    virtual void setTimeOfLastFailedStaticSkyAttack(double t);                                    // +0x218
    virtual double getTimeOfLastFailedStaticSkyAttack() const;                                    // +0x21C
    virtual void setTimeOfLastFailedStaticAttack(double t);                                       // +0x220
    virtual double getTimeOfLastFailedStaticAttack() const;                                       // +0x224
    virtual void setBotWantsToSimulateArtillery(bool b);                                          // +0x228
    virtual bool getBotWantsToSimulateArtillery() const;                                          // +0x22C
    virtual void setBotIsSimulatingArtillery(bool b);                                             // +0x230
    virtual bool getBotIsSimulatingArtillery() const;                                             // +0x234
    virtual void setAvoidSide(int side);                                                          // +0x238 [FRONTIER]
    virtual int getAvoidSide() const;                                                             // +0x23C
    virtual int getMoveToStatus() const;                                                          // +0x240
    virtual void setMoveToStatus(int status);                                                     // +0x244 [FRONTIER]
    virtual void setCollidedObject(unsigned int h);                                               // +0x248
    virtual unsigned int getCollidedObject() const;                                               // +0x24C
    virtual void setCollidedObjectPosition(const Vec3& v);                                        // +0x250 [FRONTIER]
    virtual const Vec3& getCollidedObjectPosition() const;                                        // +0x254
    virtual void setIgnoreObstacles(bool b);                                                      // +0x258
    virtual bool getIgnoreObstacles() const;                                                      // +0x25C
    virtual void setSlowDownToTarget(unsigned int h);                                             // +0x260
    virtual unsigned int getSlowDownToTarget() const;                                             // +0x264
    virtual double getFriendsInLineOfFireTime() const;                                            // +0x268
    virtual void setFriendsInLineOfFireTime(double t);                                            // +0x26C
    virtual void setLastCheckMoveToFinishedTime(double t);                                        // +0x270
    virtual double getLastCheckMoveToFinishedTime() const;                                        // +0x274
    virtual void setIsGoingForFlag(bool b);                                                       // +0x278
    virtual bool getIsGoingForFlag() const;                                                       // +0x27C
    virtual bool botInPoseTransition() const;                                                     // +0x280 [FRONTIER]
    virtual void setUseStaticAttack(bool b);                                                      // +0x284
    virtual bool getUseStaticAttack() const;                                                      // +0x288
    virtual void setTimeOfLastFailedSplashDamageAttack(double t);                                 // +0x28C
    virtual double getTimeOfLastFailedSplashDamageAttack() const;                                 // +0x290
    virtual void setTakeOffPosition(const Vec3& v);                                               // +0x294 [FRONTIER]
    virtual const Vec3& getTakeOffPosition() const;                                               // +0x298
    virtual void setQuadSeen();                                                                   // +0x29C [FRONTIER]
    virtual bool isAttacking(unsigned int handle, double& out) const;                             // +0x2A0 [FRONTIER]
    virtual void event_shotTimeOut(int i);                                                        // +0x2A4 [FRONTIER]
    virtual void event_firing(bool& b, int i, const Vec3& v);                                     // +0x2A8 [FRONTIER]

    // not in the vtable. Ordinary members; the vtable dump above is the authority on which
    // every name here is absent from all 171 slots.
    void  prePlanExecution();
    void  postPlanExecution();
    void  addBehaviour(BotBehaviour* b, int i);
    void  removeBehaviour(BotBehaviour* b);
    void  uninitAllBehaviours();
    BotBehaviour* getActiveBehaviour();
    UrgencyGenerator** getBehaviourUrgencyGenerators();
    void  calculateMods();
    float* getBasicBehaviourMods();
    float* getCurrentBehaviourMods();
    // these two touch the class static, not A member. See the header.
    static void          setNBehaviours(unsigned char n);   // , no this argument
    static unsigned char getNBehaviours();
    static void   setBasicWeights(float* w);                // , no this argument
    static float* getBasicWeights();
    void  setTakingOrders(bool b);
    PlanInterpreter* getPlanInterpreter();
    void  setPlanInterpreter(PlanInterpreter* p);
    double getTimeOfLastFire() const;
    void   stepFireInclination();                           // thunk
    void*  getAttackers() const;
    // +0x298 and returns it), so this pair is the other Vec3 - the one the constructor never
    // initialises. The old note that one of the two pairs was +576 is settled.
    const Vec3& getSpawnCoord() const;
    void  setSpawnCoord(const Vec3& v);
    void  setObedience(float f);
    float getAttention();
    void  setAttention(float f);
    float getInvAttention();
    void  setInvAttention(float f);
    float getMarksmanship();                                // no setter
    float getHarmlessThrsh();                               // no setter, dice's spelling
    void  setPlayer(IAIPlayer* p);
    void  setPlayerInput();
    void  resetAllControls(bool a, bool b, bool c);         // [FRONTIER]
    void  validateDataStructures();
    // A class static, shared by every bot. See the header - the constructor reads it three times.
    static unsigned char mu8NBehaviours;
    static float* maBasicWeights;                           // ELF .bss

private:
    // [INFERRED] friendship: addUnspawnedBot directly replaces +28 in both images.
    friend class BotManager;
    // [CHOSEN] Names for fields whose original identifiers are unavailable. Offsets describe the retail x86 layout.
    /*  12 */ void*        m_debugger;              // = 0 getDebugger
    /*  16 */ BotLodLevel  m_lodLevel;              // = 0 get/setLodLevel
    /*  20 */ bool         m_planInvalid;           // = true hasPlan / invalidatePlan
    /*  21 */ bool         m_hasDefaultPlan;        // = false hasDefaultPlan
    /*  24 */ unsigned int m_botId;                 // ctor arg `id`; getBotId reads it
    /*  28 */ float*       m_basicWeights;          // ctor arg `weights`
    /*  32 */ float        m_float32;               // ctor arg `f` [UNRESOLVED]
    /*  36 */ float        m_obedience;             // = 1.0f getObedience
    /*  40 */ int          m_int40;                 // ctor arg `i` [UNRESOLVED]
    /*  44 */ int          m_side;                  // ctor arg `n` getSide
    /*  48 */ float        m_float48;               // = 0.01f [UNRESOLVED] a small threshold
    /*  52 */ void*        m_ptr52;                 // = 0
    /*  56 */ BotSquad*    m_squad;                 // new(0xC)
    /*  60 */ int          m_spawnToStrategicAreaId;// = -1 getSpawnToStrategicAreaId
    /*  64 */ char         m_gap64[4];              // +64 byte set to -1 [UNRESOLVED]
    /*  68 */ float*       m_currentOrders;         // borrowed BotOrder weights, not the order object
    /*  72 */ float*       m_ptr72;                 // = 0; per-behaviour modifiers in calculateMods
    /*  76 */ BotMemory*   m_memory;                // Borrowed; construction also initializes its first word and +16 double.
    /*  80 */ bool         m_takingOrders;          // isTakingOrders
    /*  81 */ char         m_pad81[3];
    /*  84 */ void*        m_ptr84;                 // = 0
    /*  88 */ char         m_gap88[3];              // +89, +90 zeroed [UNRESOLVED]
    /*  91 */ bool         m_inTransitionPhase;     // isInTransitionPhase
    /*  92 */ char         m_pad92;
    /*  93 */ bool         m_deactivated;           // isDeactivated
    /*  94 */ char         m_pad94[2];
    // Parallel per-behaviour arrays: scalar urgency values and behaviour pointers.
    /*  96 */ float*       m_behaviourUrgencies;    // new[] sized by mu8NBehaviours - one per behaviour
    /* 100 */ void*        m_ptr100;                // = 0
    /* 104 */ void*        m_ptr104;                // = 0
    /* 108 */ void*        m_ptr108;                // = 0
    /* 112 */ BotBehaviour** m_behaviours;         // new[] sized by mu8NBehaviours, zero-filled.
    // Owns the active BotScript. reset destroys it virtually and clears the pointer.
    /* 116 */ BotScript*   m_activePlan;            // Initially null; get/setPlan access it.
    /* 120 */ bool         m_planLock;              // = false get/setPlanLock
    /* 121 */ char         m_pad121[3];
    /* 124 */ WaypointList* m_waypoints;            // = 0 getWaypoints
    // Handle lists. The first element type is attested; the second is inferred.
    /* 128 */ std::list<unsigned int> m_list128;
    /* 132 */ std::list<unsigned int> m_list132;    // [INFERRED] element type, by symmetry with
                                                    // Signed behaviour selector; reset uses -1 for no selection.
    /* 136 */ signed char  m_orderedBehaviour;      // reset: -1 isOrderedBehaviour
    /* 137 */ char         m_pad137[3];
    /* 140 */ unsigned int m_currentEquipment;      // = -1 get/setCurrentEquipment
    /* 144 */ PlanInterpreter*   m_planInterpreter; // ctor arg
    /* 148 */ UrgencyGenerator*  m_urgencyMerger;   // ctor arg `merger`
    /* 152 */ UrgencyGenerator** m_urgencyGenerators;// ctor arg `generators`
    /* 156 */ IAIPlayer*   m_player;                // ctor arg; getPlayer reads it
    /* 160 */ void*        m_ptr160;                // = 0 (written twice - before and after
    /* 164 */ void*        m_ptr164;                // setPlayerInput; see the .cpp)
    /* 168 */ void*        m_ptr168;                // = 0
    // reset clears this double before the other fields.
    /* 172 */ double       m_double172;             // reset: 0.0
    /* 180 */ FireCorrectionData* m_fireCorrection; // new(0x80)
    /* 184 */ std::list<void*> m_list184;    // allocate(12) - see +128
    /* 188 */ BotNavigation*      m_navigation;     // new(0x164)
    /* 192 */ SensingAgent*       m_sensingAgent;   // new(0x9C), ctor takes mu8NBehaviours
    /* 196 */ CommunicationAgent* m_communication;  // new(0x70)
    /* 200 */ int          m_int200;                // reset: -1 (not a gap)
    // Independent task targets, initially -1. reset preserves change and revive targets.
    /* 204 */ unsigned int m_changeTarget;          // = -1
    /* 208 */ unsigned int m_repairTarget;          // = -1
    /* 212 */ unsigned int m_bridgeRepairTarget;    // = -1
    /* 216 */ unsigned int m_wantedRepairTarget;    // = -1
    /* 220 */ unsigned int m_reviveTarget;          // = -1
    // Three flags, not a gap - `reset` zeroes +224, +225 and +226.
    /* 224 */ bool         m_isRepaired;            // reset: false
    /* 225 */ bool         m_isEngineerTarget;      // reset: false
    /* 226 */ bool         m_shouldWaitForRepairs;  // reset: false
    /* 227 */ char         m_pad227;
    /* 228 */ unsigned int m_mineTarget;            // = -1
    /* 232 */ void*        m_ptr232;                // = 0
    /* 236 */ float        m_moveInclination;       // = 0
    /* 240 */ float        m_fireInclination;       // = 0
    /* 244 */ float        m_moveInclinationDelta;  // = 1.0f addToMoveInclination accumulates here
    /* 248 */ float        m_fireInclinationDelta;  // = 1.0f
    /* 252 */ void*        m_ptr252;                // = 0
    /* 256 */ void*        m_ptr256;                // = 0
    /* 260 */ void*        m_ptr260;                // = 0
    /* 264 */ unsigned int m_firingTarget;          // = -1
    /* 268 */ double       m_firingTargetTime;      // getFiringTargetTime
    /* 276 */ double       m_time276;               // = -DBL_MAX - "never happened"
    /* 284 */ bool         m_botStuck;              // = false
    /* 285 */ bool         m_flag285;               // = false
    /* 286 */ char         m_pad286[2];
    // Construction uses -DBL_MAX as the timestamp sentinel; reset uses -9999.0.
    /* 288 */ double       m_time288;               // ctor: -DBL_MAX, reset: -9999.0
    /* 296 */ int          m_requestedSoldierPose;  // = 1 dice spells the getter "Solder"
    /* 300 */ bool         m_flag300[4];            // +300..+303, all false
    /* 304 */ int          m_int304;                // = -1
    /* 308 */ unsigned int m_unresolvedCollision;   // = -1
    /* 312 */ bool         m_airBorn;               // = false isAirBorn
    /* 313 */ char         m_pad313[3];
    /* 316 */ void*        m_ptr316[3];             // +316..+324, all = 0
    /* 328 */ IBotDeathCallBack*  m_deathCallBack;  // = 0
    /* 332 */ IBotSpawnCallBack*  m_spawnCallBack;  // = 0
    /* 336 */ char         m_gap336[4];             // [UNRESOLVED]
    // Sensed objects returned by reference; construction reserves capacity for 256 players.
    /* 340 */ std::vector<IAIObject*> m_sensedObjects;
    /* 352 */ double       m_lastChangeTime;        // setLastChangeTime
    /* 360 */ bool         m_manDown;               // get/setManDown
    /* 361 */ char         m_pad361[3];
    // A double, found by `reset`. The old 12-byte gap swallowed it.
    /* 364 */ double       m_double364;             // reset: 0.0
    /* 372 */ bool         m_isFiring;              // isFiring / setIsFiring
    /* 373 */ char         m_pad373[3];
    /* 376 */ SquadMember* m_squadMember;           // = 0
    /* 380 */ bool         m_fireAllowed;           // = true
    /* 381 */ char         m_pad381[3];
    // four, and the second pair is A copy of the first - see copyAssignedAreas.
    /* 384 */ int          m_assignedAreas[4];      // +384..+396, all = -1
    /* 400 */ void*        m_ptr400;                // = 0
    /* 404 */ void*        m_ptr404;                // = 0
    /* 408 */ void*        m_ptr408;                // = 0
    /* 412 */ int          m_int412;                // = -1
    // [INFERRED] Consecutive plan-executor skips, cleared when execution resumes.
    // A signed comparison against 15 decides when a stalled bot stops coasting.
    /* 416 */ int          m_stalledPlanTicks;      // = 0

    /* 420 */ bool         m_c4Placed;              // = false - the ctor's last field store
    /* 421 */ char         m_pad421[7];
    /* 428 */ void*        m_ptr428[3];             // +428..+436, all = 0
    /* 440 */ char         m_gap440[20];            // [UNRESOLVED]
    // Three grenade timestamps. getTimeOfGrenadeThrown requires an index in [0, 3);
    // it performs no bounds check and larger indices read adjacent attack timestamps.
    /* 460 */ double       m_grenadeThrownTime[3];  // +460, +468, +476
    /* 484 */ double       m_timeOfLastFailedStaticAttack;     // = 0.0
    /* 492 */ double       m_timeOfLastFailedStaticSkyAttack;  // = 0.0
    /* 500 */ bool         m_wantsToSimulateArtillery;   // = false
    /* 501 */ bool         m_isSimulatingArtillery;      // = false
    /* 502 */ bool         m_flag502;                    // = false [UNRESOLVED]
    /* 503 */ char         m_pad503;
    /* 504 */ int          m_moveToStatus;          // = 0
    /* 508 */ int          m_avoidSide;             // = 0
    /* 512 */ char         m_gap512[4];             // +512 byte = 0
    /* 516 */ double       m_double516;             // = 0.0
    /* 524 */ unsigned int m_collidedObject;        // = -1
    /* 528 */ float        m_collidedObjectPosition[3];  // a Vec3 - getCollidedObjectPosition
    /* 540 */ bool         m_ignoreObstacles;       // = false
    /* 541 */ char         m_pad541[3];
    /* 544 */ unsigned int m_slowDownToTarget;      // = -1
    /* 548 */ double       m_friendsInLineOfFireTime;   // = 0.0
    /* 556 */ double       m_lastCheckMoveToFinishedTime;  // = -9999.0, a magnitude sentinel
    /* 564 */ bool         m_isGoingForFlag;        // = false
    /* 565 */ bool         m_useStaticAttack;       // = false
    /* 566 */ char         m_pad566[2];
    /* 568 */ double       m_timeOfLastFailedSplashDamageAttack;  // = -9999.0
    // The last field. `Vec3::getZero` is copied in component by component. `getLookDir`
    // `setLookDir` and `getSpawnCoord` / `setSpawnCoord` have identical shapes
    // so one of the two pairs is this field and the other is a Vec3 the ctor never initialises.
    // [UNRESOLVED] which; reading either 56-byte setter settles it in one step.
    /* 576 */ float        m_takeOffPosition[3];    // Vec3, = Vec3::getZero
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_BOTMAIN_H
