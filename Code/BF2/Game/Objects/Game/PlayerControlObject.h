// Rideable object managing occupants, weapons, cameras, input and vehicle lifecycle.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_PLAYERCONTROLOBJECT_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_PLAYERCONTROLOBJECT_H

#include <list>
#include <map>
#include <vector>

#include "../Base/Bundle.h"
#include "../PlayerControlObjectTemplate.h"
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"   // the real second base
#include "../IAnimatedBundle.h" // Third base at +0x140.
#include "../../../IO/Input/PlayerInput.h"      // the PlayerInputMap / GameInputMap enums, and io::Input<T,N>

namespace dice {
namespace anim { class IAnimationSystem; }
namespace hfe {

// Tracks held fire input and emits start, continued-fire and stop events.
// The counter may become negative after release; the no-input sentinel emits no event.
struct FireButtonState
{
    FireButtonState() { reset(); }

    void reset() { m_count = 0; }

    // . Only message 6 does anything; everything else is passed straight through.
    int translateMessage(int message)
    {
        if (message != 6)
            return message;
        const bool wasFiring = m_count > 0;
        m_count = 2;
        return wasFiring ? 8 : 7;
    }

    // . 9 on the 1 -> 0 transition, 71 otherwise.
    int handleUpdate()
    {
        return (m_count-- == 1) ? 9 : 71;
    }

    /* +0x00 */ int m_count;                         // ctor 0. [CHOSEN] name
};

struct SprintState
{
    SprintState();
    void reset();
    void setStamina(float stamina);
    void setConstants(float a, float b, float c);
    void handleTMSprint(bool arg);
    int  handleUpdate(bool disable, float sprintFactor, float dt);   // Returns
                                                     // 71 (nothing), 42 (started sprinting), 43 (stopped) -
                                                     // the caller posts 42/43 as messages

    /* +0x00 */ float m_constant0;                   // setConstants' 1st; handleUpdate divides by it
    /* +0x04 */ float m_constant1;                   // setConstants' 2nd
    /* +0x08 */ float m_constant2;                   // setConstants' 3rd (the sprint threshold)
    /* +0x0C */ float m_staminaDissipationFactor;    // ctor 1.0f; PCO::setSprintStaminaDissipationFactor
    /* +0x10 */ float m_stamina;                     // ctor 1.0f; setStamina
    /* +0x14 */ bool  m_sprintRequested;             // [CHOSEN name] handleTMSprint / handleUpdate
    /* +0x15 */ bool  _byte15;                       // reset clears; [UNRESOLVED]
    /* +0x16 */ bool  m_isSprinting;                 // get/setIsSprinting
    /* +0x17 */ bool  _pad17;
};

namespace world {

class ICameraObject;
class GroundContactSound;
class Ladder;                                        // LadderContainer's machinery, unrecovered

// (VehicleMod moved to Scene/Object/ObjectTypes/Game/IPlayerControlObject.h - the interface's slots
// take it.)

// Controllable vehicle with occupancy, camera, input, and animation interfaces.
class PlayerControlObject : public Bundle, public IPlayerControlObject, public IAnimatedBundle
{
public:
    // Attested nested type (the _getExitPosition mangling).
    enum ExitPositionType { /* [UNRESOLVED] */ };

    explicit PlayerControlObject(const PlayerControlObjectTemplate* objectTemplate);
                                                     // C2 0x81962E0, 1,207 B - read in full
    virtual ~PlayerControlObject();                  // (D2) - the release cascade

    // own quartet, refcount +0x144
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;// the five-case table above

    // family-slot overrides (0..95)
    virtual void init();                             // slot 8 - transcribed: the vehicle
                                                     // assembly line (see the .cpp); its helper
                                                     // family is the open frontier
    virtual void disableCachedObject();              // slot 9 , the cache park
    virtual void enableCachedObject();               // slot 10 , the cache revive
    virtual void destroy(bool force);                // slot 13 - : reset, drop the
                                                     // dynamic spawn group, skip-bundle
    virtual float getBoundingRadiusModifier() const; // slot 39 (template +0x304)
    virtual void handleUpdate(float, unsigned int);  // slot 44 -
    virtual bool handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                                 int, int, CollisionFaceInfo*, CollisionFaceInfo*);
                                                     // slot 45 ,
    virtual void handlePlayerInput(IPlayer*, const io::PlayerInput&, float, unsigned int);
                                                     // slot 64 ELF 700 B / PE 355 B
    virtual void handleMessage(TemplateMessage, IPlayer*, bool);   // slot 65
    virtual void initNetworkableComp();              // slot 94 (child: PCO networkable / root: SimpleObject's)

    // the vehicle surface: primary slots 96..180, order walked from the ZTV
    // Parameter types are the manglings'; return types [CHOSEN] unless a read pinned them.
    // Sizes annotated; everything not marked is declared-unwritten and faults by name.
    virtual void setActiveAnimationSystem(int);              // s96  empty (IAnimatedBundle's slot too)
    virtual void* getActiveAnimationSystem();                // s97  the +0x218 system (IAnimatedBundle's slot too; was `int`)
    virtual IPlayer* getOccupingPlayer() const;              // s98  dice's spelling
    virtual IObject* getOccupingObject() const;              // s99
    virtual bool isOccupied() const;                         // s100
    virtual bool enter(IPlayer*);                            // s101
    virtual void exit();                                     // s102
    virtual void exitPlayer();                               // s103
    virtual void onEnter(IObject*);                          // s104 empty
    virtual void onExit();                                   // s105 empty
    // exitPlayer (s103) is a one-line forwarder into the shared exit transaction.
    void internalExit(bool byPlayer);
    virtual int  getPcoId() const;                           // s106
    virtual void setPcoId(int);                              // s107
    virtual std::map<int, ICameraObject*>* getCameras();     // s108
    virtual std::map<int, ICameraObject*>* getOwnCameras();  // s109
    virtual void nextCamera();                               // ELF 155 B / PE 64 B s110
    virtual ICameraObject* getActiveCamera();                // s111
    ICameraObject* getNonCommanderCamera();                  // ELF 819B190 / PE 594150, nonvirtual
    virtual void setActiveCamera(int);                       // s112
    virtual void addExternalCamera(ICameraObject*);          // ELF 79 B / PE 50 B s113
    virtual void removeExternalCamera(ICameraObject*);       // ELF 155 B / PE 65 B s114
    virtual std::map<int, IPlayerControlObject*>* getPcos(); // s115
    virtual const std::vector<int>* getPcoPosId();           // s116 ELF 0x819CF90 / PE 0x592F00; template-owned
    virtual void addPlayerInputObject(IObject*);             // ELF 116 B / PE 154 B s117
    virtual void removePlayerInputObject(IObject*);          // ELF 121 B / PE 72 B s118
    virtual void addWeapon(IObject*);                        // s119
    virtual void* getWeapons(); // s120 [CHOSEN ret]
    virtual IObject* getActiveWeapon(int) const;             // s121 the join
                                                             // crash (updateStateMask's caller)
    virtual void* getWeapons(int); // s122 [CHOSEN ret]
    virtual void setActiveWeapon(IObject*, int);             // s123
    virtual void toggleOnWeaponLock(IObject*);               // ELF 135 B / PE 89 B s124
    virtual bool checkSpecialToggle(io::PlayerInput);        // s125 BY value, per mangling
    virtual bool checkSpecialToggleLastUpdate(io::PlayerInputMap);   // s126 BY value
    virtual bool getExitPosition(Vec3&);                     // s127 returns _getExitPosition's bool (movzx al)
    virtual bool _getExitPosition(Vec3&, ExitPositionType);  // ELF 581 B / PE 336 B s128 - dice's underscore
    virtual bool getAltExitPosition(Vec3&);                  // s129 copies +0x200, returns 1
    virtual int  getArtPos();                                // s130 the template's +0x280 byte (movzx),
                                                             // not a Vec3* as first [CHOSEN]
    virtual void setTeam(int);                               // s131
    virtual int  getTeam();                                  // s132
    virtual void setDefaultTeam(int);                        // s133 (store + setTeam vcall)
    virtual int  getDefaultTeam();                           // s134
    virtual void setTimeToLiveUnused(float);                 // s135
    virtual float getTimeToLiveUnused();                     // s136
    virtual void setObjectSpawnerId(int);                    // s137
    virtual int  getObjectSpawnerId() const;                 // s138
    virtual void setTimeSpawned(float);                      // s139
    virtual float getTimeSpawned() const;                    // s140
    virtual const Vec3& getNameTagOffset();                  // s141
    virtual const Vec3& getRidingWithOffset();               // s142
    virtual float getRidingWithDistMod();                    // s143
    virtual void setObjectSpawnerHolding(bool);              // s144
    virtual bool getObjectSpawnerHolding() const;            // s145
    virtual bool isAllowedToExit();                          // ELF 1,582 B / PE 1,027 B s146
    virtual int  getVehicleCameraShake();                    // s147 the template's +0x2B0 dword
                                                             // in eax (the interface default is -1)
    virtual float getVehicleFov();                           // s148
    virtual void addVehicleMod(VehicleMod);                  // s149 0x819DB70 -
    virtual void removeVehicleMod(VehicleMod);               // s150 0x819DBE0 -
    virtual int  getVehicleMod(VehicleMod);                  // s151 0x819DC50 -
    virtual const std::string& getSoundFilter() const;       // s152 (the template's string)
    virtual int  getCockpitLod();                            // s153
    virtual int  getCockpitSubGeom();                        // s154
    virtual float getShakeFactor() const;                    // s155
    virtual bool getIsSprinting() const;                     // s156
    virtual void setIsSprinting(bool);                       // s157
    virtual float getSprintStamina() const;                  // s158
    virtual void setSprintStamina(float);                    // s159
    virtual void setSprintStaminaDissipationFactor(float);   // s160
    virtual float getSprintFactor();                         // s161
    virtual bool getIsUnderwater() const;                    // s162 false
    virtual float getNormalizedUnderwaterTime() const;       // s163 0
    virtual void setDriverDecalObject(IObject*);             // s164
    virtual IObject* getDriverDecalObject() const;           // s165
    virtual void addDriverDecalTransformation(const Mat4&);  // ELF 247 B / PE 118 B s166
    virtual std::vector<Mat4>* getDriverDecalTransformation() const;   // s167 [CHOSEN ret]
    virtual int  getSpecialToggleWeaponInput() const;        // s168
    virtual void setIsDucking(bool);                         // s169
    virtual bool getIsDucking();                             // s170
    virtual bool getAutoUseAbility() const;                  // s171
    virtual bool isChildSoldierActive() const;               // s172
    virtual bool getNeverDrawAs1p() const;                   // s173
    virtual void removeChildVehicle(IObject*);               // s174
    virtual int  getAmmoStatus();                            // s175
    virtual void setAmmoStatus(int);                         // s176
    virtual void setGiveUnmannedInput(bool);                 // s177
    virtual void onFirstGhost();                             // s178 the two
                                                             // message-70 wheel snaps (1/3 s, 5 s)
    virtual void preHandlePlayerInput(io::PlayerInput&, float) const;   // ELF 115 B / PE 83 B s179
    virtual Vec3* getExitRotation();                         // s180 [CHOSEN ret]

    // non-virtual, sizes on record
    void reset();                                    // (destroy's first act)
    void disableWeapons();                           // reset calls it
    void regulateInput(io::PlayerInput& input, float deltaTime) const;
    Vec3 getExitPosRayTestStart(const Vec3& exitPosition);
    // The vehicle sitting - non-virtual (no vtable references them):
    int  calcAmmoStatus();                           // 0x819DE80 - floor(clamp(15 x
                                                     // the running ammo ratio over the weapon list, 0, 15));
                                                     // 15 with no weapons
    void getUnmannedInput(io::PlayerInput& input);   // ; dormant stock gate
    void setProperKitGeometry();
    void updateAmbientSound();
    bool vehicleIsUsed() const;                      // 0x819C6B0 - any pco in m_pcos isOccupied
    void enableHandleUpdate(bool enable);            // 0x8196BC0 - the 0x800 update flag / the 3 s run-out
    void updateTeam();                               // 0x819D390 - the occupants' team onto every pco
    void snapWheel(IObject* object);                 // handleMessage's
                                                     // message-70 arm; recurses over the object
                                                     // tree looking for CID_SpringTemplate children
    void updateTimeToLiveUnused();
    // Occupant-entry helpers.
    void setPlayerOnWeapons(IPlayer* player);        // 0x819A5E0 - IWeaponObject::setPlayer
                                                     // on every weapon. enter() also does this
                                                     // loop inline, so retail runs it twice.
    void enableSpawnPoints(bool enable);             // 0x819AB20 - setActive(enable) on
                                                     // every m_spawnPoints entry, but only when
                                                     // the template says disableSpawnPointsOnEnter
    bool setListenerEnvironment(bool enable);
    // All three are written; enter() calls them on its straight-line path.
    // enableTargetNametags IS A shipped NO-OP - and the suspicion this note used to carry was
    // right on both counts: the bool really is never read, and the iteration over its copy of
    // GameLogic::getTargetObjectMap really has no body. All 368 bytes are the inlined map clone and
    // its teardown. The copy is reproduced, not optimised away: it is per-vehicle-entry allocator
    // traffic, so removing it would change the server's allocation profile. See the .cpp.
    void enableTargetNametags(bool enable);          // 0x819AB90 - gated on the template's
                                                     // m_targetObjectTypesToWatch being non-empty
                                                     // (+0x2CC is that SGI list's count word).
    void enablePlayerDecal(IPlayer* player, bool enable);   // 0x819A650 - the driver's

protected:
    // Object state follows the IPlayerControlObject and IAnimatedBundle base subobjects.
    /* 0x144 */ int   m_refCount;                    // ctor 1
    /* 0x148 */ const PlayerControlObjectTemplate* m_template;
    /* 0x14C */ float m_handleUpdateTimeout; // Seconds until removal from the update map; -1 disables the countdown. [CHOSEN name]
    // enableHandleUpdate(false) sets 3 seconds; a sleeping physics node can end the countdown early.
    /* 0x150 */ bool  m_entered; // Entered state controls camera updates.
    // Occupant references use weak control blocks. Reset clears the object; destruction clears object and player.
    /* 0x154 */ int   m_pcoId;                       // ctor <- template (the per-vehicle position
                                                     // id; 0 = the root/driver position)
    /* 0x158 */ int*  m_occupyingPlayer;             // WeakPtr<IPlayer> control block (IDB: occupyingPlayer)
    /* 0x15C */ int*  m_occupyingObject;             // WeakPtr<IObject> control block (IDB: occupyingObject)
    // Camera maps use native STL storage; original map offsets are layout references only.
    // The own-camera count determines whether camera lookup delegates to an occupant.
    /* 0x160 */ std::map<int, ICameraObject*> m_ownCameras;   // +0x160 header ptr, +0x164 node count
    /* 0x168 */ int   _dword168;                     // [UNRESOLVED] - init reads it (0x8197E58)
    /* 0x16C */ int   _dword16C;                     // [UNRESOLVED] - ctor 0; nextCamera reads it
    // Fallback camera cache; commander and parent cameras take priority.
    /* 0x170 */ ICameraObject* m_activeCamera;       // ctor 0. [CHOSEN] name
    // Controlled positions indexed by ID.
    /* 0x174 */ std::map<int, IPlayerControlObject*> m_pcos;  // (IDB: PCOs) - the interface ptr,
                                                              // per findPlayerControlObjects'
                                                              // attested signature
    /* 0x180 */ std::map<int, IPlayerControlObject*> _map180;  // value type
                                                              // the dtor's third _M_erase is the
                                                              // IPlayerControlObject tree, not
                                                              // <int,int>
    /* 0x18C */ std::list<IObject*> m_playerInputObjects;     // (findPlayerInputObjects fills)
    /* 0x190 */ std::list<unsigned int> m_playerInputIds;     // init: the +0x10 id of each
    /* 0x194 */ std::vector<IObject*> m_weapons;     // (IDB: weapons) - pinned by disableWeapons'
    // Weapons available in each of the three input slots.
    /* 0x1A0 */ std::vector<IObject*> m_slotWeapons[3];
    /* 0x1C4 */ IObject* m_activeWeapons[3];         // pinned: disableWeapons zeroes exactly
                                                     // these three; the ctor's tail zero loop
                                                     // ("gap1A4 + 32") is the same three words
    /* 0x1D0 */ std::vector<IObject*> m_spawnPoints; // (IDB: spawnPoints) - findSpawnPoints'
                                                     // attested out-vector; init feeds each
                                                     // through hfe::getSpawnPoint(IObject*)
    /* 0x1DC */ int   m_spawnGroupId;                // ctor -1
    /* 0x1E0 */ int   _dword1E0;                     // [UNRESOLVED] ctor 0
    /* 0x1E4 */ int   m_team;                        // (IDB: dword1E4 - init's setTeam source;
                                                     // [CHOSEN name])
    /* 0x1E8 */ int   m_defaultTeam;                 // ctor 0
    /* 0x1EC */ float m_timeToLiveUnused;            // ctor -1.0f
    /* 0x1F0 */ float m_timeToLiveUnusedReset; // Full unused-life countdown restored when the vehicle is near its spawner.
    /* 0x1F4 */ float m_timeSpawned;                 // ctor -1.0f
    /* 0x1F8 */ int   m_objectSpawnerId;             // ctor -1
    /* 0x1FC */ bool  m_objectSpawnerHolding;        // ctor 0
    /* 0x200 */ float m_altExitPosition[3];          // ctor <- template's alternate exit
    /* 0x20C */ bool  _byte20C;                      // [UNRESOLVED] ctor 0
    /* 0x210 */ IObject* m_driverDecalObject;        // ctor 0
    /* 0x214 */ IObject* m_seatParentObject;         // (IDB: dword214; init resolves the
                                                     // seatParent template) [CHOSEN name]
    /* 0x218 */ anim::IAnimationSystem* m_activeAnimationSystem; // ctor 0; init resolves it; reset / dtor release it.
    /* 0x21C */ int   m_vehicleMods[9];              // (IDB: VehicleMod1..8 + dword21C) ctor 0
    /* 0x240 */ int   _dword240;                     // [UNRESOLVED]
    /* 0x244 */ std::vector<Mat4> m_driverDecalTransformations;  // (64-byte elements per the dtor)
    // One 32-bit decal ID per driver-decal transformation.
    /* 0x250 */ std::vector<unsigned int> m_driverDecalIds;
    // Held-state latch used to detect a rising special-toggle input.
    /* 0x25C */ bool  m_specialTogglePressed;        // ctor 0
    /* 0x260 */ SprintState m_sprintState;           // to +0x277 (the "+0x274 listener3DObstruction"
    /* 0x278 */ int   _dword278;                     // ctor -1
    /* 0x27C */ void* m_ambientSoundFromTemplate2;   // = m_ambientSoundFromTemplate after init
    /* 0x280 */ void* m_ambientSoundFromTemplate;    // init: createSoundFromTemplate("Ambient")
    /* 0x284 */ void* m_ambientDamagedSoundFromTemplate;   // ...("AmbientDamaged")
    /* 0x288 */ float m_listener3DObstruction;       // (IDB name) - handleUpdate: clamp((speed - 5) / 20, 0, 1) - RE-
    /* 0x28C */ GroundContactSound* m_groundContactSound; // owns sound/controller/node carrier
    /* 0x290 */ float m_inputTime;                   // ctor 0 (IDB name)
    /* 0x294 */ int   m_ammoStatus; // Defaults to all four bits set.
    // Weak references to the two drive wheels.
    /* 0x298 */ WeakPtr<world::IObject> m_driveWheelFirst;   // (was _dword298 - dtor's release-loop bound)
    /* 0x29C */ WeakPtr<world::IObject> m_driveWheelSecond;  // (was _dword29C - ctor 0)
    /* 0x2A0 */ unsigned int m_unmannedInputTick;    // [INFERRED] name; unsigned gate in ELF
                                                     // (+0x2E4), ctor/reset 0
    /* 0x2A1 */ bool  m_isDucking;                   // ctor 0 - offsets in this run are IDB-name
                                                     // approximations; the raw pass pins them
                                                     // when a reader needs them
    /* 0x2A4 */ int   _inputState[64];               // (IDB: gap2A4, 0x100 bytes zeroed) - the
                                                     // per-frame input block [UNRESOLVED shape]
    /* 0x3A4 */ bool  m_giveUnmannedInput;           // ctor 0
    /* 0x3A8 */ int   _dword3A8;                     // ctor 0 [UNRESOLVED] (was mis-named the animation system)
    /* 0x3AC */ int   _dword3AC;                     // [UNRESOLVED]
    /* 0x3B0 */ int   _dword3B0;                     // [UNRESOLVED]
    /* 0x3B4 */ int   _dword3B4;                     // [UNRESOLVED]
    /* 0x3B8 */ bool  _byte3B8;                      // [UNRESOLVED]
    /* 0x3B9 */ bool  _byte3B9;                      // ctor 0; reset clears
    /* 0x3BA */ bool  m_firstGhost;                  // ctor 1
    /* 0x3BB */ char  _pad3BB;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_PLAYERCONTROLOBJECT_H
