// Connected participant state: identity, team, inventory selection, input and network state.
// Player derives from this interface independently of world objects and keeps weak references to its vehicle.
// Return types marked [UNRESOLVED] require caller evidence before their values are used. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_IPLAYER_H     // [INFERRED] guard style
#define GAME_OBJECTS_IPLAYER_H

#include <map>
#include <string>
#include <vector>

#include "../../Core/IBase.h"
#include "../../Core/WeakPtr.h"
#include "../../Core/Flag32Bits.h"
#include "../../Math/Vec3.h"
#include "../../IO/Input/PlayerInput.h"      // the PlayerInputMap / GameInputMap enums, and io::Input<T,N>

namespace dice {
namespace hfe {

class ActionBuffer;                                  // dice::hfe::ActionBuffer - unrecovered
namespace io {
    class PlayerInput;                               // IO/Input/PlayerInput.h
    class BitStream;
    class INetworkable;
}

namespace world {

class IObject;
class IObjectTemplate;
class ICameraObject;
class PlayerScoreData;                               // unrecovered
enum UpdateFreqencyType : int;                       // Game/Objects/IObject.h's opaque enum
enum PlayerVotingStatus : int;                       // [UNRESOLVED enumerators] - the vote states

class IPlayer : public IBase
{
public:
    IPlayer();                                       // vptr + null control block
    // (the quartet, slots 0..3, stays IBase's pure virtuals)
    virtual unsigned int getClassID() const = 0;                                    // +0x10
    virtual ~IPlayer();                                                             // +0x14 +0x18 - detaches the weak block
    virtual WeakPtr<IPlayer> getWeakPtr();                                          // +0x1C , (lazy block)
    virtual void  setName(const std::string& name) = 0;                             // +0x20
    virtual const std::string& getName() const = 0;                                 // +0x24 (Player +0x5C)
    virtual void  setFlags(const Flag32Bits<IPlayer>& flags) = 0;                   // +0x28 updateFlags on Player +0x58
    virtual bool  testFlags(const Flag32Bits<IPlayer>& flags) const = 0;            // +0x2C
    virtual void  handleUpdate(float dt, unsigned int) = 0;                         // +0x30 (Player: empty)
    virtual void  handleFrameUpdate(float dt) = 0;                                  // +0x34
    virtual UpdateFreqencyType getUpdateFrequencyType() const = 0;                  // +0x38 (Player +0xA0)
    virtual void  setUpdateFrequencyType(UpdateFreqencyType type) = 0;              // +0x3C
    virtual void  handleInput(io::PlayerInput& input, float dt, unsigned int) = 0;  // +0x40 [UNRESOLVED return]
    virtual void  getAutoInput(io::PlayerInput& out, bool) = 0;                     // +0x44
    virtual IObject* getVehicle() const = 0;                                        // +0x48 the WeakPtr'd vehicle,
    // Assigns the vehicle and seat; the Player implementation returns true.
    virtual bool  setVehicle(IObject* vehicle, int seat) = 0;                       // +0x4C
    virtual int   getInputId() const = 0;                                           // +0x50 (Player +0x98)
    virtual int   getId() const = 0;                                                // +0x54 (Player +0x9C)
    virtual void  setIsRemote(bool remote);                                         // +0x58 IPlayer default: empty (Player: byte +0x165)
    virtual bool  getIsRemote() const;                                              // +0x5C IPlayer default: false
    virtual void  setIsAIPlayer(bool ai);                                           // +0x60 IPlayer default: empty (Player: 220 B)
    virtual bool  getIsAIPlayer() const;                                            // +0x64 IPlayer default: false (Player: byte +0xB8)
    virtual void  setIsAlive(bool alive) = 0;                                       // +0x68 (also stores -1 at +0x314)
    virtual bool  getIsAlive() const = 0;                                           // +0x6C (byte +0xB9)
    virtual void  setSuicide(bool) = 0;                                             // +0x70 (byte +0xBA)
    virtual bool  getSuicide() const = 0;                                           // +0x74
    virtual bool  getIsConnected() const = 0;                                       // +0x78 (byte +0x17C)
    virtual void  setIsConnected(bool) = 0;                                         // +0x7C
    virtual float getCrossHairFadeValue() const = 0;                                // +0x80 (float +0x25C)
    virtual void  setViewDistance(float) = 0;                                       // +0x84
    virtual float getViewDistance() const = 0;                                      // +0x88
    virtual void  setFov(float fov, float time) = 0;                                // +0x8C
    virtual float getFov() const = 0;                                               // +0x90 (float +0xA8)
    virtual float getEndFov() const = 0;                                            // +0x94 (float +0xA4)
    virtual ActionBuffer* getActionBuffer() = 0;                                    // +0x98 (Player +0x158)
    // Returns io::PlayerInput storage through the existing untyped ABI slot.
    virtual void* getAIPlayerInput() const = 0;                                     // +0x9C (Player +0x15C) - an io::PlayerInput*
    virtual void  setDefaultVehicle(IObject*) = 0;                                  // +0xA0
    virtual IObject* getDefaultVehicle() = 0;                                       // +0xA4
    virtual IObject* getFreeCamera() const = 0;                                     // +0xA8 the +0x6C weak block's target
    virtual void  setFreeCameraEnabled(bool) = 0;                                   // +0xAC
    virtual bool  getFreeCameraEnabled() const = 0;                                 // +0xB0 (byte +0x70)
    virtual void  toggleFreeCameraEnabled() = 0;                                    // +0xB4 > virtual setter(!stored flag)
    virtual void  setOverheadCameraEnabled(bool) = 0;                               // +0xB8
    virtual bool  getOverheadCameraEnabled() const = 0;                             // +0xBC (byte +0x71)
    virtual void  setOverheadCameraTopDownView(bool) = 0;                           // +0xC0
    virtual bool  getOverheadCameraTopDownView() const = 0;                         // +0xC4 (commander && overhead && ...)
    virtual void  setCommanderCamOverhead(bool) = 0;                                // +0xC8 (byte +0x73)
    virtual bool  getCommanderCamOverhead() const = 0;                              // +0xCC
    virtual int   getTickCount() const = 0;                                         // +0xD0 (int +0x248)
    virtual void  addTick() = 0;                                                    // +0xD4 ++
    virtual void  subTick() = 0;                                                    // +0xD8
    virtual void  resetTickCounter() = 0;                                           // +0xDC = 0
    virtual float getTimeOutSideWorld() const = 0;                                  // +0xE0 (float +0x24C)
    virtual void  setTeam(int team, bool) = 0;                                      // +0xE4
    virtual int   getTeam() const = 0;                                              // +0xE8 (int +0xBC)
    virtual unsigned int getSpawnAtTick() const = 0;                                // +0xEC (uint +0xC8)
    virtual void  setSpawnAtTick(unsigned int tick) = 0;                            // +0xF0
    virtual float getTimeToSpawn() const = 0;                                       // +0xF4 (spawnAtTick - WorldPref::getTickCount()) as seconds
    virtual float getManDownTimeLeft() const = 0;                                   // +0xF8
    virtual void  setIsManDown(bool) = 0;                                           // +0xFC
    virtual bool  getIsManDown() = 0;                                               // +0x100 (byte +0xCC)
    virtual unsigned int getManDownAtTick() = 0;                                    // +0x104 +0xD0 while man-down, else 0
    virtual void  setSquadId(unsigned int id) = 0;                                  // +0x108 narrows to a word, tail-calls setSquadId(int)
    virtual int   getSquadId() const = 0;                                           // +0x10C (int +0xF0)
    virtual void  setIsCommander(bool) = 0;                                         // +0x110 > setIsCommander(int)
    virtual bool  getIsCommander() const = 0;                                       // +0x114 (byte +0xF4)
    virtual void  setUAVIsReady(bool) = 0;                                          // +0x118 (byte +0xF6)
    virtual bool  getUAVIsReady() const = 0;                                        // +0x11C
    virtual void  setSatelliteIsReady(bool) = 0;                                    // +0x120 (byte +0xF7)
    virtual bool  getSatelliteIsReady() const = 0;                                  // +0x124
    virtual void  setLastTimeCommander(double t) = 0;                               // +0x128 (double +0x10C)
    virtual double getLastTimeCommander() const = 0;                                // +0x12C
    virtual void  setCommanderReapplyWaitTime(double t) = 0;                        // +0x130 (double +0x114)
    virtual double getCommanderReapplyWaitTime() const = 0;                         // +0x134
    virtual void  setIsSquadLeader(bool) = 0;                                       // +0x138 > setIsSquadLeader(int)
    virtual bool  getIsSquadLeader() const = 0;                                     // +0x13C (byte +0xF5)
    virtual bool  getIsGassed() = 0;                                                // +0x140 float +0x2F4 > 0
    virtual void  setIsInsideCP(bool) = 0;                                          // +0x144 (byte +0x108)
    virtual bool  getIsInsideCP() = 0;                                              // +0x148
    virtual void  setHasSimulatedFrame(bool) = 0;                                   // +0x14C (byte +0x166)
    virtual bool  getHasSimulatedFrame() const = 0;                                 // +0x150
    virtual void  setSpawnGroup(int group) = 0;                                     // +0x154 (int +0xD4)
    virtual int   getSpawnGroup() const = 0;                                        // +0x158
    virtual void  setInterpolationTime(unsigned int ms) = 0;                        // +0x15C clamped to 0x200
    virtual unsigned int getInterpolationTime() const = 0;                          // +0x160 (uint +0xE0)
    virtual void  setPing(unsigned int ping) = 0;                                   // +0x164 clamped to 0x3FF
    virtual unsigned int getPing() const = 0;                                       // +0x168 (uint +0xDC)
    virtual void  setKit(int kit) = 0;                                              // +0x16C (int +0x54)
    virtual int   getKit() const = 0;                                               // +0x170
    virtual PlayerScoreData* getScoreData() = 0;                                    // +0x174 (Player +0xEC)
    virtual void  setFlagHolder(bool) = 0;                                          // +0x178 (byte +0x124)
    virtual bool  getFlagHolder() const = 0;                                        // +0x17C
    virtual ICameraObject* getCamera() const = 0;                                   // +0x180 commander cam / free cam / vehicle's
    virtual void  setCamera(ICameraObject* camera) = 0;                             // +0x184 (Player +0x68)
    virtual bool  hasFirstPersonView() const = 0;                                   // ELF +0x188 0x80F32A0 / PE +0x184 0x472460; Player body
    virtual double getTimeConnected() const = 0;                                    // +0x18C (double +0x180)
    virtual io::INetworkable* getNetworkable() = 0;                                 // +0x190 (Player +0x270)
    virtual bool  getStateSync(io::BitStream* stream, IObject* object) = 0;         // +0x194 ; Player body recovered
    virtual bool  setStateSync(io::BitStream* stream, IObject* object, double) = 0; // +0x198 ; Player body
    virtual void  reset() = 0;                                                      // +0x19C
    virtual void  setFiringState(bool) = 0;                                         // +0x1A0
    virtual bool  getFiringState() const = 0;                                       // +0x1A4
    virtual void  setAltFiringState(bool) = 0;                                      // +0x1A8 (byte +0x161)
    virtual bool  getAltFiringState() const = 0;                                    // +0x1AC
    virtual void  setFlareFiringState(bool) = 0;                                    // +0x1B0 (byte +0x162)
    virtual bool  getFlareFiringState() const = 0;                                  // +0x1B4
    virtual void  setSprintState(bool) = 0;                                         // +0x1B8 (byte +0x163)
    virtual bool  getSprintState() const = 0;                                       // +0x1BC
    virtual void  setSprintStateLastTick(bool) = 0;                                 // +0x1C0 (byte +0x164)
    virtual bool  getSprintStateLastTick() const = 0;                               // +0x1C4
    virtual void  setReloadState() = 0;                                             // +0x1C8 (byte +0x244 = 1)
    virtual bool  getAndResetReloadState() = 0;                                     // +0x1CC read, then clear
    virtual void  updateFiringState(unsigned int, IObject*) = 0;                    // +0x1D0 [UNRESOLVED return]
    virtual void  updateGhostFiringState() = 0;                                     // +0x1D4 [UNRESOLVED return]
    virtual void  setHitIndicator(bool) = 0;                                        // +0x1D8 also fades +0x25C to 1.0
    virtual bool  getHitIndicator() const = 0;                                      // +0x1DC read, then clear (byte +0x260)
    // [PLATFORM] ELF +0x1E0 / 0x80F7F10; PE +0x1DC / 0x474BB0. Both return a float in ST0.
    virtual float updateHitFromDirection(const Vec3& direction) = 0;
    virtual float getHitFromDirection() const = 0;                                  // +0x1E4 (float +0x188)
    virtual void  updateCameraShake(float amount) = 0;                              // [PLATFORM] +0x1E8 ELF / +0x1E4 PE - Player.cpp
    virtual float getCameraShake() const = 0;                                       // +0x1EC (float +0x190)
    virtual unsigned int getSquadInviteMask() const = 0;                            // +0x1F0 (uint +0x11C)
    virtual void  setSquadInviteMask(unsigned int mask) = 0;                        // +0x1F4
    virtual bool  getIsInvitedToSquad(int squad) const = 0;                         // +0x1F8 mask bit test
    virtual void  setKitTimeStamp(double t) = 0;                                    // +0x1FC (double +0x4C)
    virtual double getKitTimeStamp() const = 0;                                     // +0x200
    // [PLATFORM] ELF +0x204/+0x208; PE +0x200/+0x204 (one destructor slot fewer).
    virtual void  setCurrentFiringButton(io::PlayerInputMap button) = 0;            // exact dword store
    virtual io::PlayerInputMap getCurrentFiringButton() const = 0;                  // exact dword load
    virtual void  setPersonalDecalTemplate(const IObjectTemplate* decal) = 0;       // +0x20C (Player +0x200)
    virtual const IObjectTemplate* getPersonalDecalTemplate() const = 0;            // +0x210
    virtual void  enableVOHelp(bool) = 0;                                           // +0x214 (Player: empty - server build)
    virtual void  enableVOHelpMessage(int) = 0;                                     // +0x218 (empty)
    virtual void  disableVOHelpMessage(int) = 0;                                    // +0x21C (empty)
    virtual bool  getVOHelpMessagePlayed(int) = 0;                                  // +0x220 (Player: true)
    virtual void  doAddTouchItem(IPlayer* other) = 0;                               // +0x224 [UNRESOLVED return]
    virtual void  setHealRepairedAtTick(unsigned int tick, bool giving, bool repairing) = 0;   // +0x228
    virtual unsigned int getHealRepairedAtTick() const = 0;                         // +0x22C (uint +0x128)
    virtual void  setGaveAmmoAtTick(unsigned int tick, bool giving) = 0;            // +0x230
    virtual unsigned int getGaveAmmoAtTick() const = 0;                             // +0x234 (uint +0x12C)
    virtual void  setFailedExitAtTick(unsigned int tick) = 0;                       // +0x238 (uint +0x130)
    virtual unsigned int getFailedExitAtTick() const = 0;                           // +0x23C
    virtual bool  getIsGivingAbility() const = 0;                                   // +0x240 (byte +0x134)
    virtual bool  getIsRepairing() const = 0;                                       // +0x244 (byte +0x135)
    virtual bool  getIsGivingReplenish() const = 0;                                 // +0x248 (byte +0x136)
    virtual int   getChosenUnlockLevel(int kit, int slot) const = 0;                // +0x24C
    virtual void  setChosenUnlockLevel(int kit, int slot, int level) = 0;           // +0x250 void
    virtual bool  isUnlocked(int kit, int slot, int level) = 0;                     // +0x254
    virtual int   getUnlockLevel(int kit, int slot) = 0;                            // +0x258
    virtual void  setUnlocks(std::map<int, int>* unlocks) = 0;                      // +0x25C copies into +0x278
    virtual std::map<int, int>* getUnlocks() = 0;                                   // +0x260 (Player +0x278)
    virtual void  setPeerAddress(const std::string& address) = 0;                   // +0x264 > std::string::assign (+0x2AC)
    virtual const std::string& getPeerAddress() const = 0;                          // +0x268
    virtual bool  getVoipEnabled() = 0;                                             // +0x26C (byte +0x2B0)
    virtual unsigned short getVoipSessionId() = 0;                                  // +0x270 (word +0x2B2)
    virtual void  setVoipSessionId(unsigned short id) = 0;                          // +0x274
    virtual bool  getIsMuted() = 0;                                                 // +0x278 (byte +0x2B4)
    virtual bool  getIsTalking() = 0;                                               // +0x27C against the wall clock
    virtual double getLastTalkTime() = 0;                                           // +0x280 (double +0x2B8)
    virtual double getStopTalkingThreshold() = 0;                                   // +0x284 (double +0x2C0)
    virtual void  setIsMuted(bool) = 0;                                             // +0x288
    virtual void  setIsTalking(bool) = 0;                                           // +0x28C
    virtual void  setVoipEnabled(bool) = 0;                                         // +0x290
    virtual void  setVotingStatus(PlayerVotingStatus status) = 0;                   // +0x294 (dword +0x2C8)
    virtual PlayerVotingStatus getVotingStatus() = 0;                               // +0x298
    virtual void  setParticipateInVoting(bool) = 0;                                 // +0x29C (byte +0x2CC)
    virtual bool  getParticipateInVoting() = 0;                                     // +0x2A0
    virtual void  setTargetOfVote(bool) = 0;                                        // +0x2A4 (byte +0x2CD)
    virtual bool  getTargetOfVote() = 0;                                            // +0x2A8
    virtual int   getProfileId() = 0;                                               // +0x2AC (int +0x2D0)
    virtual void  setProfileId(int id) = 0;                                         // +0x2B0
    virtual void  setBlurryVision(float, float, float) = 0;                         // +0x2B4
    virtual float getBlurValue() const = 0;                                         // +0x2B8 (float +0x300)
    virtual bool  getIsNearGasCloud() const = 0;                                    // +0x2BC (byte +0x305)
    virtual bool  getIsGasMaskOn() const = 0;                                       // +0x2C0 (byte +0x304)
    virtual void  setIsGasMaskOn(bool, bool) = 0;                                   // +0x2C4
    virtual void  setGasMaskHudItem(bool) = 0;                                      // +0x2C8 (Player: empty)
    virtual float getPlayerGasMaskChangeTimer() const = 0;                          // +0x2CC (float +0x308)
    virtual float getPlayerNightVisionChangeTimer() const = 0;                      // +0x2D0 (float +0x30C)
    virtual bool  getIsUnderwater() const = 0;                                      // +0x2D4 (Player: false)
    virtual float getNormalizedNightVisionPower() = 0;                              // +0x2D8 (Player: 1.0f)
    virtual void  setNightVision(bool, bool) = 0;                                   // +0x2DC
    virtual void  rechargeNightVision() = 0;                                        // +0x2E0 (Player: empty)
    virtual void  setVisualNightVision(bool) = 0;                                   // +0x2E4
    virtual bool  getNightVision() const = 0;                                       // +0x2E8 (byte +0x320)
    virtual void  disableFlashBangEffect() = 0;                                     // +0x2EC
    virtual void  disableBlurryVision() = 0;                                        // +0x2F0
    virtual io::PlayerInput getPlayerInput() = 0;                                   // +0x2F4 BY value (sret, the 273-byte copy)
    virtual void  resetOnDeath() = 0;                                               // +0x2F8
    virtual bool  isPlayerInNightMap() const = 0;                                   // +0x2FC (Player: false)
    virtual void  setTracePlayerId(int id) = 0;                                     // +0x300 (int +0x314)
    virtual int   getTracePlayerId() = 0;                                           // +0x304
    virtual void  allocateFreeCamera() = 0;                                         // +0x308
    virtual bool  getIsThisASpam(bool) = 0;                                         // +0x30C
    virtual void  setRadioSpamFlagCount(int) = 0;                                   // +0x310 (int +0x148)
    virtual int   getRadioSpamFlagCount() = 0;                                      // +0x314
    virtual void  setRadioSpamBlockedFlagCount(int) = 0;                            // +0x318 (int +0x14C)
    virtual int   getRadioSpamBlockedFlagCount() = 0;                               // +0x31C
    virtual void  setRadioSpamFilterFlagTimer(float) = 0;                           // +0x320 (float +0x150)
    virtual float getRadioSpamFilterFlagTimer() = 0;                                // +0x324
    virtual void  resetSpamFilter() = 0;                                            // +0x328 counts 0, timer -1.0
    virtual void  getCloseMines(std::vector<IObject*>& out) const = 0;              // +0x32C [UNRESOLVED return]
    virtual bool  getAutoReload() const = 0;                                        // +0x330 (byte +0x318)
    virtual void  setAutoReload(bool) = 0;                                          // +0x334
    virtual void  updateClientSettings() = 0;                                       // +0x338 (Player: empty on the ELF; the client copies GeneralSettings.autoReload)
    // Player adds seven virtuals of its own after these (+0x33C makeScript(io::IStream*) ..
    // +0x354 moveTo(const Vec3&)) - the concrete class's, not the interface's.

protected:
    /* +0x04 */ WeakPtr<IPlayer>::Ctrl* m_weakControl;   // ctor 0; getWeakPtr allocates {1, this}
};

} // namespace world

// Player flags allocated in initialization order.
extern Flag32Bits<world::IPlayer> g_playerFlagBit0;      // ds:0x8AB0A28  makeScript: Player.isAi
extern Flag32Bits<world::IPlayer> g_playerFlagBit1;      // ds:0x8AB0A30  makeScript: Player.isNetwork
extern Flag32Bits<world::IPlayer> g_playerFlagBit2;      // ds:0x8AB0A38  [UNRESOLVED meaning]
extern Flag32Bits<world::IPlayer> g_playerFlagBit3;      // ds:0x8AB0A40  the item-switch latch
                                                         // (SoldierNetworkable::setNetUpdate,
                                                         // DefaultSoundComp::isBackstepping)

} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_IPLAYER_H
