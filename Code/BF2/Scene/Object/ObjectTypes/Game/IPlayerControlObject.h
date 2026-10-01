// Player-control interface with fixed virtual declaration order; header path is attested.
// The concrete PlayerControlObject supplies the secondary interface, retiring RW-1's raw-vptr seam.
// Default methods retain their observed values or Debug stops; other slots remain pure virtual.
// getPcoPosId's pointer/const spelling is [INFERRED]. Return types without direct evidence retain
// the existing [CHOSEN] declarations; VehicleMod is part of the shared signature contract.

#ifndef SCENE_OBJECT_OBJECTTYPES_GAME_IPLAYERCONTROLOBJECT_H     // [INFERRED] guard style
#define SCENE_OBJECT_OBJECTTYPES_GAME_IPLAYERCONTROLOBJECT_H

#include <map>
#include <string>       // getSoundFilter
#include <vector>

#include "../../../../Core/IBase.h"
#include "../../../../Math/Vec3.h"
#include "../../../../Math/Mat4.h"
#include "../../../../IO/Input/PlayerInput.h"      // the PlayerInputMap / GameInputMap enums, and io::Input<T,N>

namespace dice {
namespace hfe {


namespace world {

class IObject;
class IPlayer;
class ICameraObject;

// Attested by name in the manglings (addVehicleMod / removeVehicleMod / getVehicleMod take it);
// values [UNRESOLVED].
enum VehicleMod { /* [UNRESOLVED] */ };

class IPlayerControlObject : public IBase
{
public:
    virtual IPlayer* getOccupingPlayer() const = 0;                              // +0x10  dice's spelling
    virtual IObject* getOccupingObject() const;                                  // +0x14  (default) 0x81B1050: 0
    virtual bool isOccupied() const = 0;                                         // +0x18
    virtual bool enter(IPlayer* player) = 0;                                     // +0x1C
    virtual void exit() = 0;                                                     // +0x20
    virtual void exitPlayer() = 0;                                               // +0x24
    virtual void onEnter(IObject*) = 0;                                          // +0x28
    virtual void onExit() = 0;                                                   // +0x2C
    virtual int  getPcoId() const = 0;                                           // +0x30
    virtual void setPcoId(int id) = 0;                                           // +0x34
    virtual std::map<int, ICameraObject*>* getCameras() = 0;                     // +0x38
    virtual std::map<int, ICameraObject*>* getOwnCameras() = 0;                  // +0x3C
    virtual void nextCamera() = 0;                                               // +0x40
    virtual ICameraObject* getActiveCamera() = 0;                                // +0x44
    virtual void setActiveCamera(int) = 0;                                       // +0x48
    virtual bool getExitPosition(Vec3& out);                                     // +0x4C  (default) 0x81B1060: true
    virtual bool getAltExitPosition(Vec3& out);                                  // +0x50  (default) 0x81B1070: true
    virtual void addExternalCamera(ICameraObject* camera);                       // +0x54  (default) 0x81B1080: Debug 6
    virtual void removeExternalCamera(ICameraObject* camera);                    // +0x58  (default) 0x81B1170: Debug 6
    virtual bool canSpawn(const Mat4& transform);                                // +0x5C  (default) 0x8180C40: true -
                                                                                 //        interface-only, no override
    virtual std::map<int, IPlayerControlObject*>* getPcos() = 0;                 // +0x60
    virtual const std::vector<int>* getPcoPosId() = 0;                          // +0x64  borrowed vector
    virtual void addWeapon(IObject* weapon) = 0;                                 // +0x68
    // [PLATFORM] Preserve overload order. ELF slots +0x6C/+0x70/+0x74 are getWeapons(),
    // getActiveWeapon(int), getWeapons(int). PE groups the overloads as getWeapons(int),
    // getWeapons() [INFERRED], getActiveWeapon(int). Resolve offsets per image, not by index alone.
    virtual void* getWeapons() = 0;                                              // +0x6C  [CHOSEN ret]
    virtual IObject* getActiveWeapon(int) const = 0;                             // +0x70 ELF / +0x74 PE
    // Returns storage for a const vector<IObject*> reference through the current void* signature.
    // [RETIRE-WHEN: pco-getweapons-vector] Retype the interface, every override and caller atomically,
    // then validate all interface dispatch and replication consumers.
    virtual void* getWeapons(int) = 0;                                            // +0x74 ELF / +0x70 PE
    virtual void setActiveWeapon(IObject* weapon, int) = 0;                      // +0x78
    virtual void toggleOnWeaponLock(IObject*) = 0;                               // +0x7C
    virtual int  getArtPos() = 0;                                                // +0x80  the template's +0x280 byte
    virtual bool checkSpecialToggle(io::PlayerInput input);                      // +0x84  (default) 0x81B1260: Debug 6 - by value
    virtual bool checkSpecialToggleLastUpdate(io::PlayerInputMap map);           // +0x88  (default) 0x81B1350: Debug 6 - by value.
                                                                                 //        declared only: io::PlayerInputMap has no
                                                                                 //        layout in the tree (IO/Input/Input.h), and a
                                                                                 //        by-value parameter needs one to define the body -
                                                                                 //        RVA 0 under /force; PlayerControlObject overrides
    virtual void setTeam(int team) = 0;                                          // +0x8C
    virtual void setDefaultTeam(int team) = 0;                                   // +0x90
    virtual int  getDefaultTeam();                                               // +0x94  (default) 0x81B1440: 0
    virtual int  getTeam() = 0;                                                  // +0x98
    virtual float getTimeToLiveUnused();                                         // +0x9C  (default) 0x81B1450: -1.0f
    virtual void setTimeToLiveUnused(float t) = 0;                               // +0xA0
    virtual bool isAllowedToExit();                                              // +0xA4  (default) 0x81B1460: true
    virtual void addPlayerInputObject(IObject*) = 0;                             // +0xA8
    virtual void removePlayerInputObject(IObject*) = 0;                          // +0xAC
    virtual void setTimeSpawned(float t);                                        // +0xB0  (default) 0x81B1470: Debug 6
    virtual float getTimeSpawned() const;                                        // +0xB4  (default) 0x81B1560: Debug 6
    virtual void setObjectSpawnerId(int id) = 0;                                 // +0xB8
    virtual int  getObjectSpawnerId() const = 0;                                 // +0xBC
    virtual void setObjectSpawnerHolding(bool holding) = 0;                      // +0xC0
    virtual bool getObjectSpawnerHolding() const = 0;                            // +0xC4
    virtual const Vec3& getNameTagOffset() = 0;                                  // +0xC8
    virtual const Vec3& getRidingWithOffset() = 0;                               // +0xCC
    virtual float getRidingWithDistMod() = 0;                                    // +0xD0
    virtual int  getVehicleCameraShake();                                        // +0xD4  (default) 0x81B1660: -1
    virtual float getVehicleFov();                                               // +0xD8  (default) 0x81B1670: 0.0f
    virtual void addVehicleMod(VehicleMod mod) = 0;                              // +0xDC
    virtual void removeVehicleMod(VehicleMod mod) = 0;                           // +0xE0
    virtual int  getVehicleMod(VehicleMod mod) = 0;                              // +0xE4
    virtual const std::string& getSoundFilter() const = 0; // Returns the template's sound-filter string.
    virtual int  getCockpitLod();                                                // +0xEC  (default) 0x81B1680: 0
    virtual int  getCockpitSubGeom();                                            // +0xF0  (default) 0x81B1690: 0
    virtual float getShakeFactor() const = 0;                                    // +0xF4
    virtual bool getIsSprinting() const = 0;                                     // +0xF8
    virtual void setIsSprinting(bool);                                           // +0xFC  (default) 0x81E56B0: empty
    virtual float getSprintStamina() const = 0;                                  // +0x100
    virtual void setSprintStamina(float) = 0;                                    // +0x104
    virtual void setSprintStaminaDissipationFactor(float) = 0;                   // +0x108
    virtual float getSprintFactor() = 0;                                         // +0x10C
    virtual bool getIsUnderwater() const = 0;                                    // +0x110
    virtual float getNormalizedUnderwaterTime() const = 0;                       // +0x114
    virtual void setDriverDecalObject(IObject*);                                 // +0x118 (default) 0x81B16A0: empty
    virtual IObject* getDriverDecalObject() const;                               // +0x11C (default) 0x81B16B0: 0
    virtual void addDriverDecalTransformation(const Mat4&);                      // +0x120 (default) 0x81B16C0: empty
    virtual std::vector<Mat4>* getDriverDecalTransformations() const;            // +0x124 (default) 0x8180C50: 0 -
                                                                                 //        interface-only [CHOSEN ret]
    virtual int  getSpecialToggleWeaponInput() const;                            // +0x128 (default) 0x81B16D0: 0x40
    virtual void setIsDucking(bool);                                             // +0x12C (default) 0x81B16E0: empty
    virtual bool getIsDucking();                                                 // +0x130 (default) 0x81B16F0: false
    virtual bool getAutoUseAbility() const;                                      // +0x134 (default) 0x81B1700: false
    virtual bool applyCameraAnimation(const Mat4& in, Mat4& out);                // +0x138 (default) 0x8195D60: false -
                                                                                 //        interface-only
    virtual bool allowEnterOtherVehicle();                                       // +0x13C (default) 0x81A17C0: false -
                                                                                 //        interface-only
    virtual bool isChildSoldierActive() const;                                   // +0x140 (default) 0x81B1710: false
    virtual bool getNeverDrawAs1p() const;                                       // +0x144 (default) 0x81B1720: false
    virtual void removeChildVehicle(IObject*);                                   // +0x148 (default) 0x81B1730: empty
    virtual int  getAmmoStatus();                                                // +0x14C (default) 0x81E56C0: 15
    virtual void setAmmoStatus(int);                                             // +0x150 (default) 0x81E56D0: empty
    virtual void onFirstGhost();                                                 // +0x154 (default) 0x81B1740: empty
    virtual void setGiveUnmannedInput(bool);                                     // +0x158 (default) 0x81B1750: empty
    virtual void preHandlePlayerInput(io::PlayerInput& input, float dt) const;   // +0x15C (default) 0x81B1760: empty

protected:
    ~IPlayerControlObject() {}                       // no virtual dtor in the table (IBase's rule)
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // SCENE_OBJECT_OBJECTTYPES_GAME_IPLAYERCONTROLOBJECT_H
