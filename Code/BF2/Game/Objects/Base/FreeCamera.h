// Spectator and pre-spawn camera with entry-point and player-control interfaces.
// Player-control inheritance is nonpublic; access it through queryInterface.
// Vehicle-style defaults are intentional, including isOccupied returning true.

#ifndef GAME_OBJECTS_GAME_FREECAMERA_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_FREECAMERA_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include "../Base/Bundle.h"
#include "../FreeCameraTemplate.h"
#include "../ICameraObject.h"
#include "../IEntryPointObject.h"
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "../../../Core/WeakPtr.h"
#include "../../../Math/Mat4.h"

namespace dice {
namespace hfe {

class FreeCameraNetworkable;      // reads m_followedPlayerId raw - see the friend below
class GameClient;                 // enterVehicle sets the camera's trace object

namespace world {

class FreeCamera : public Bundle,
                   public ICameraObject,
                   public IEntryPointObject,
                   protected IPlayerControlObject     // non-public in the RTTI - see the header note
{
public:
    explicit FreeCamera(const FreeCameraTemplate* objectTemplate);
    virtual ~FreeCamera();

    // its own refcount quartet, the OverheadCamera arrangement, counter at +0x148
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;// five answers, see the .cpp

    // the primary chain's overrides
    virtual void init();
    virtual void destroy(bool force);
    virtual void handleUpdate(float deltaTime, unsigned int flags);
    virtual void handlePlayerInput(IPlayer*, const io::PlayerInput&, float, unsigned int);
    virtual void initNetworkableComp();

    // [UNRESOLVED] declaring base; these occupy primary slots 137 and 138.
    virtual int getSubPos();                         // 0
    virtual int getSonarPos();                       // 0

    // ICameraObject: 23 pure
    virtual const Mat4& getTransformation(float);    // mode-6 pair
                                                     // vs absolute; member-copy tail (+0x210)
    virtual void setInterpolatedMode(bool);          // empty
    virtual bool setViewMode(CameraViewMode, IObject*, bool);   // , 521 B
    virtual CameraViewMode getViewMode() const;      // +0x190
    virtual bool isActive() const;                   // +0x168
    virtual void setIsActive(bool);
    virtual void setCameraId(int);                   // empty
    virtual int  getCameraId();                      // 0
    virtual void prevViewMode();                     // empty
    virtual void setTraceObjectId(unsigned int);     // +0x1EC
    virtual unsigned int getTraceObjectId();
    virtual void setExternCameraTrans(Mat4);         // empty (BY value)
    virtual Mat4* getExternCameraTrans();            // 0
    virtual bool getToggleMouseLook();               // false
    virtual int  getPcoId();                         // 0  (ICameraObject's +0x48)
    virtual void handleExplosion(float);             // empty
    virtual void resetCameraView(bool);
    virtual void updateCameraView(bool);             // vcalls resetCameraView
    virtual float getActiveFOV() const;              // the template's two FOVs
    virtual const Vec3& getStaticSunDirection() const;   // Vec3::getIdentity
    virtual const Vec3& getStaticSunColor() const;       // ditto
    virtual const Vec3& getStaticSunDamageColor() const; // ditto
    virtual const Vec3& getStaticSunAmbientColor() const;// ditto
    virtual bool allowCameraModeCycle();             // false (the base says true)

    // IEntryPointObject: 4 pure
    virtual bool enter(IPlayer* player, int& seat);
    virtual bool exit(IPlayer* player);
    virtual IPlayer* getPlayer() const;
    virtual IPlayerControlObject* getLinkedPlayerControlObject();

    // IPlayerControlObject: 48 pure
    virtual IPlayer* getOccupingPlayer() const;      // (dice's spelling)
    virtual bool isOccupied() const;                 // hard true
    virtual bool enter(IPlayer* player);
    virtual void exit();
    virtual void exitPlayer();
    virtual void onEnter(IObject*);                  // empty
    virtual void onExit();                           // empty
    virtual int  getPcoId() const;                   // 0
    virtual void setPcoId(int id);                   // empty
    virtual std::map<int, ICameraObject*>* getCameras();     // &m_ownCameras
    virtual std::map<int, ICameraObject*>* getOwnCameras();  // THE same MAP
    virtual void nextCamera();                       // empty
    virtual ICameraObject* getActiveCamera();        // the +0x13C sub-object
    virtual void setActiveCamera(int);               // empty
    virtual std::map<int, IPlayerControlObject*>* getPcos();
    virtual const std::vector<int>* getPcoPosId();   // &m_pcoPosIds
    virtual void addWeapon(IObject*);                // empty
    virtual void* getWeapons();                      // &m_weapons
    virtual IObject* getActiveWeapon(int) const;     // 0
    virtual void* getWeapons(int);                   // the same vector
    virtual void setActiveWeapon(IObject*, int);     // empty
    virtual void toggleOnWeaponLock(IObject*);       // empty
    virtual int  getArtPos();                        // 0
    virtual void setTeam(int);                       // empty
    virtual void setDefaultTeam(int);                // empty
    virtual int  getTeam();                          // 0
    virtual void setTimeToLiveUnused(float);         // empty
    virtual void addPlayerInputObject(IObject*);     // empty
    virtual void removePlayerInputObject(IObject*);  // empty
    virtual void setObjectSpawnerId(int);            // empty
    virtual int  getObjectSpawnerId() const;         // -1
    virtual void setObjectSpawnerHolding(bool);      // empty
    virtual bool getObjectSpawnerHolding() const;    // false
    virtual const Vec3& getNameTagOffset();          // +0x19C
    virtual const Vec3& getRidingWithOffset();       // THE same Vec3
    virtual float getRidingWithDistMod();            // AND ITS X component
    virtual void addVehicleMod(VehicleMod);          // empty
    virtual void removeVehicleMod(VehicleMod);       // empty
    virtual int  getVehicleMod(VehicleMod);          // 0
    virtual const std::string& getSoundFilter() const;   // +0x20C
    virtual float getShakeFactor() const;            // 0.0f
    virtual bool getIsSprinting() const;             // false
    virtual bool getIsSprintingPossible() const;     // false (base is not pure? see .cpp)
    virtual float getSprintStamina() const;          // 0.0f
    virtual void setSprintStamina(float);            // empty
    virtual void setSprintStaminaDissipationFactor(float);   // empty
    virtual float getSprintFactor();                 // 0.0f
    virtual bool getIsUnderwater() const;            // false
    virtual float getNormalizedUnderwaterTime() const;   // 1.0f, not 0
    virtual float getListenerObstruction() const;    // 0.0f
    virtual void setListenerObstruction(float);      // empty
    virtual void setMenuVisibleForTeam(unsigned char);   // empty
    virtual unsigned char getMenuVisibleForTeam();   // 0
    virtual std::list<std::string>* getPrimWeaponIcons();   // +0x158
    virtual std::list<std::string>* getSecWeaponIcons();    // +0x15C
    virtual std::string getVehicleIcon();            // a copy of +0x160

    // Input handlers preserve all four caller arguments, including the final unsigned value.
    void handlePlayerInputDefault(IPlayer*, const io::PlayerInput&, float,
                                  unsigned int);
    void handlePlayerInputCVMTrace(IPlayer*, const io::PlayerInput&, float,
                                   unsigned int);
    const Mat4& getTransformationFollowPlayer(float);
    const Mat4& getTransformationDeath(float);
    IObject* getTraceObject();
    void resetTraceCamera();
    // Cycles living human teammates, excluding the occupant; keeps the old ID if none qualify.
    void followNextPlayer(bool);                     // each

private:
    // Replication reads the followed-player ID directly.
    friend class dice::hfe::FreeCameraNetworkable;
    // Parachute::detachSoldier writes the released Soldier's WeakPtr straight into +0x28C after
    // retaining this camera during exitPlayer (ELF 0x81955D0 / PE 0x599DA0).
    friend class Parachute;
    // The same field, the same way, from the client: GameClient::enterVehicle tells the camera
    // what the player just got into. Named rather than opened, exactly as Parachute is.
    friend class dice::hfe::GameClient;

    // layout +0x13C..+0x293, from the 1,028-byte C1 constructor. The three base vptrs at +0x13C /
    // +0x140 +0x144 are the compiler's - they are listed so the numbering reads straight.
    // [CHOSEN] names unless noted; the offsets are the ELF's and are recorded, not enforced.
    /* 0x13C */ // ICameraObject vptr
    /* 0x140 */ // IEntryPointObject vptr
    /* 0x144 */ // IPlayerControlObject vptr
    /* 0x148 */ int   m_refCount;                    // ctor 1
    /* 0x14C */ std::vector<IObject*> m_weapons;     // ctor empty; getWeapons hands out its address
    /* 0x158 */ std::list<std::string> m_primWeaponIcons;   // ctor empty
    /* 0x15C */ std::list<std::string> m_secWeaponIcons;    // ctor empty
    /* 0x160 */ std::string m_vehicleIcon;           // ctor ""
    /* 0x164 */ WeakPtr<IPlayer> m_occupingPlayer;   // ctor null - the ONE piece of real state
    /* 0x168 */ bool  m_isActive;                    // ctor false
    /* 0x16C */ std::map<int, ICameraObject*> m_ownCameras;         // init() inserts {0, this}
    /* 0x178 */ std::map<int, IPlayerControlObject*> m_playerControlObjs;   // init() inserts {0, this}
    /* 0x184 */ std::vector<int> m_pcoPosIds;        // ctor empty
    /* 0x190 */ int   m_viewMode;                    // ctor 1 (CameraViewMode); exit() zeroes it
    /* 0x194 */ float _float194;                     // ctor -1.0f - handleUpdate ages it while
                                                     // Trace age uses -1 as disabled, starts at zero, and becomes ten seconds on a manual target hop.
                                                     // [UNRESOLVED] Its downstream timer consumer. The adjacent latch controls view-mode input.
    /* 0x198 */ bool  m_viewModeToggleArmed;         // ctor true  [CHOSEN name]
    /* 0x19C */ float m_nameTagOffset[3];            // ctor 0,0,0 - a Vec3 by its accessors, and
                                                     // getRidingWithDistMod reads element 0
    /* 0x1A8 */ int   m_followedPlayerId;            // ctor -1 - handleUpdate's spectate target
    /* 0x1AC */ float _mat1AC[16];                   // ctor identity [UNRESOLVED role]
    /* 0x1EC */ unsigned int m_traceObjectId;        // ctor -1
    /* 0x1F0 */ bool  _bool1F0;                      // ctor true [UNRESOLVED]
    /* 0x1F4 */ float _vec1F4[3];                    // ctor 0,0,0
    // Current movement multiplier and the threshold used while retaining fast throttle.
    /* 0x200 */ float m_cameraSpeedThreshold;        // ctor 1.0f  [CHOSEN name]
    /* 0x204 */ bool  _bool204;                      // ctor false
    /* 0x208 */ float m_cameraSpeed;                 // ctor 1.0f  [CHOSEN name]
    /* 0x20C */ std::string m_soundFilter;           // ctor ""
    /* 0x210 */ float m_transformation[16];          // ctor identity - getTransformation's output
    /* 0x250 */ float _dword250;                     // death direction x; twice initialised
    /* 0x254 */ float _dword254;                     // death direction y; twice initialised
    /* 0x258 */ float _float258;                     // death direction z; ctor 0, then 1
    /* 0x25C */ float _dword25C;                     // current death-camera radius; ctor 0
    /* 0x260 */ float _vec260[3];                    // death orientation xyz; twice initialised
    /* 0x26C */ float _float26C;                     // death orientation w; ctor 1
    /* 0x270 */ float _vec270[3];                    // tracked death-camera target
    /* 0x27C */ float _float27C;                     // desired death-camera radius; ctor 2
    /* 0x280 */ float _vec280[3];                    // interpolated attachment target
    // [PLATFORM] These reference offsets describe the ELF layout; client PE members have different base offsets.
    // Use fields and typed interfaces instead of copying raw offsets across images.
    /* 0x28C */ WeakPtr<IObject> m_traceObject;       // object followed when entering trace mode;
    // Clamps spectator height unless developer mode disables the ground restriction.
    /* 0x290 */ bool  m_clampToGround;               // ctor true  [CHOSEN name]
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_FREECAMERA_H
