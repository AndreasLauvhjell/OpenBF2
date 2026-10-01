// Weapon control interface. Preserve the complete virtual declaration order.
// Return types without caller evidence remain marked [CHOSEN]. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_IWEAPONOBJECT_H     // [INFERRED] guard style, matching IItemObject.h
#define GAME_OBJECTS_IWEAPONOBJECT_H

#include "../../Core/IBase.h"
#include "../../Core/Random.h"
#include "../../IO/Input/Input.h"        // io::PlayerInputMap - the real enum
#include "../../Math/Vec3.h"

#include <vector>

namespace dice {
namespace hfe {

namespace io { class PlayerInput; }
struct Mat4;

namespace world {

class IObject;
class IPlayer;
class IObjectTemplate;

// [INFERRED] underlying type. setFireRate(world::FireRate) proves the type exists and is passed by
// value; its enumerators are NOT read, so none are invented here (agents.md).
enum FireRate : int;

// 0x18-byte state packet copied by GenericFireArm::setStateSync (ELF 0x82529A0, PE 0x581E40).
// The field roles come from the setters each word feeds; the source names remain [CHOSEN].
struct WeaponState
{
    int          ammoInMag;
    FireRate     fireRate;
    int          currentNrOfMags;
    int          camera;
    unsigned int flags;
    float        heat;
};

class IWeaponObject : public IBase
{
public:
    // fire timing / state
    // Returns through the floating-point ABI (`fld1`) in both shipped x86 images.
    virtual float getTimeToNotAllowChange() = 0;                     // +0x010 (E)
    virtual FireRate getFireRate() = 0;                              // +0x014 (E) fireComp +0xDC
    virtual int   getFireAtTick() const = 0;                         // +0x018
    virtual bool  isFiring() const = 0;                              // +0x01C
    virtual bool  isReadyToUseFire() = 0;                            // +0x020
    virtual float getHeat() const = 0;                               // +0x024 (E) fld
    virtual void  setHeat(float) = 0;                                // +0x028
    virtual bool  getHasHeat() const = 0;                            // +0x02C
    virtual bool  getUsesHeat() const = 0;                           // +0x030
    virtual bool  getShowHudIcon() const = 0;                        // +0x034
    virtual io::PlayerInputMap getFireInput() const = 0;             // +0x038 (E) default 0x40
    virtual io::PlayerInputMap getAltFireInput() const = 0;          // +0x03C
    virtual bool  launchesProjectile(int) const = 0;                 // +0x040

    // kit ability
    virtual void* getAbilityObjectList() const = 0;                  // +0x044
    virtual bool  getAbilityWorking() const = 0;                     // +0x048
    virtual float getAbility() const = 0;                            // +0x04C

    // setters
    virtual void  setAmmoInMag(unsigned int, int) = 0;               // +0x050
    virtual void  setFireRate(FireRate) = 0;                         // +0x054
    virtual void  setZoom(int) = 0;                                  // +0x058
    virtual void  setWeaponState(unsigned int) = 0;                  // +0x05C
    virtual void  setStateSync(const WeaponState&) = 0;              // +0x060

    virtual IPlayer* getPlayer() = 0;                                // +0x064
    virtual void  setPlayer(IPlayer*) = 0;                           // +0x068 (E) void
    // Both shipped x86 images return an address in EAX; the null-component arm allocates and
    // returns a leaked identity matrix, proving this is a reference rather than a Vec3 value.
    virtual const Mat4& getProjectileOrigin() = 0;                   // +0x06C (E)
    virtual void  detonateProjectiles() = 0;                         // +0x070

    // ammo / reload
    virtual bool  canBeRefilled() = 0;                               // +0x074 (E) movzx al
    virtual bool  shouldReload() const = 0;                          // +0x078
    // SupplyDepot consumes the unsigned leftover in EAX (ELF 0x81B9D00 / PE 0x5721E0).
    virtual unsigned int reloadAmmo(unsigned int, bool) = 0;        // +0x07C
    virtual float reloadAmmoPercent(float, bool) = 0;                // +0x080 (E) x87 return
    virtual int   getTotalAmmo() const = 0;                          // +0x084
    virtual bool  isReadyToUse() = 0;                                // +0x088
    virtual float getReloadTimeLeftInPercent() const = 0;            // +0x08C
    virtual float getTotalReloadTimeLeftInPercent() const = 0;       // +0x090
    virtual int   getAmmoInMag(unsigned int) const = 0;              // +0x094 (E) -1 / 0 defaults
    virtual int   getMaxNrOfMags() const = 0;                        // +0x098 (E)
    virtual void  setCurrentNrOfMags(int) = 0;                       // +0x09C
    virtual int   getCurrentNrOfMags() const = 0;                    // +0x0A0 (E)
    virtual int   getAmmoType() = 0;                                 // +0x0A4
    virtual int   getMagSize() = 0;                                  // +0x0A8 (E)
    virtual int   getMag0Size() = 0;                                 // +0x0AC
    virtual float getReloadTime() const = 0;                         // +0x0B0
    virtual bool  isReloading() const = 0;                           // +0x0B4
    virtual bool  getAutoReload() const = 0;                         // +0x0B8
    virtual bool  needsReload() const = 0;                           // +0x0BC

    // deviation (accuracy)
    virtual void  updateDeviation(io::PlayerInput, bool) = 0;        // +0x0C0
    virtual float getDeviation() = 0;                                // +0x0C4
    virtual float getMinDeviation() = 0;                             // +0x0C8
    virtual float getMaxDeviation() = 0;                             // +0x0CC
    virtual void  setAIDeviation(float) = 0;                         // +0x0D0
    virtual Vec3  deviatePosition(Vec3&, int) = 0;                   // +0x0D4
    virtual void  setProjectileDeviationFixed(Vec3&, int) = 0;       // +0x0D8

    // recoil
    virtual bool  getHasRecoilForce() = 0;                           // +0x0DC
    virtual const RandomDistribution& getRecoilForceLeftRight() = 0;// +0x0E0 (E)
    virtual const RandomDistribution& getRecoilForceUp() = 0;       // +0x0E4 (E)
    virtual bool  getGoBackOnRecoil() = 0;                           // +0x0E8
    virtual float getDevMod() = 0;                                   // +0x0EC
    virtual float getRecoilForceModifier() const = 0;                // +0x0F0
    // Despite the retail name, this is the per-tick scalar recoil graph.  Both ports' consumers
    // read the returned object as std::vector<float> begin/end/capacity pointers.
    virtual const std::vector<float>& getRecoilVector() const = 0;   // +0x0F4 (E)
    virtual float getPitchRecoil() = 0;                              // +0x0F8
    virtual float getYawRecoil() = 0;                                // +0x0FC

    // optics
    virtual int   getPrevCameraId() const = 0;                       // +0x100
    virtual int   getChangeCameraId() const = 0;                     // +0x104
    virtual int   getZoom() = 0;                                     // +0x108 (E) Networkables int
    virtual float getMouseSpeedMod() = 0;                            // +0x10C (E) default 1.0f
    virtual float getZoomFactor(unsigned int) const = 0;             // +0x110
    virtual bool  getNoisy() const = 0;                              // +0x114

    // flags
    virtual void  setFlags(unsigned int) = 0;                        // +0x118
    virtual int   getFlags() const = 0;                              // +0x11C (E) bit0 | comp<<1

    // targeting
    virtual IObject* getTargetObject() const = 0;                    // +0x120
    virtual bool  hasTargetLock() const = 0;                         // +0x124
    virtual int   getLastFiredProjectileId() const = 0;              // +0x128
    virtual void  setTargetDirection(Vec3, Vec3) = 0;                // +0x12C
    virtual int   getTargetSystemType() const = 0;                   // +0x130
    virtual float getMaxTargetDistance() const = 0;                  // +0x134
    virtual float getMaxTargetAngle() const = 0;                     // +0x138

    virtual bool  isAnimationReadyToUse() = 0;                       // +0x13C
    virtual bool  getUseScope() = 0;                                 // +0x140

    // projectile bookkeeping
    virtual void  projectileWasReset(IObject*) = 0;                  // +0x144
    virtual void  addProjectileReference(IObject*) = 0;              // +0x148

    // selection / identity
    virtual bool  isSelectable() const = 0;                          // +0x14C (E)
    virtual bool  isUnselectable() const = 0;                        // +0x150 (E)
    virtual bool  getWorksInWater() const = 0;                       // +0x154 (E) default true
    virtual int   getItemIndex() = 0;                                // +0x158 (E) template field
    virtual IObject* getProjectile() const = 0;                      // +0x15C
    virtual void* getActiveWeaponGeometry() const = 0;               // +0x160
    virtual int   getWeaponState() const = 0;                        // +0x164 (E) Networkables int

    virtual bool  hasMissileConnection() const = 0;                  // +0x168
    virtual bool  isWaitingForMissileConfirm() const = 0;            // +0x16C
    virtual bool  ownsProjectile(IObject*) const = 0;                // +0x170
    virtual void  releaseProjectile(IObject*) = 0;                   // +0x174

    virtual int   getNetworkSafeItemIndex() const = 0;               // +0x178
    virtual void  setNetworkSafeItemIndex(int) = 0;                  // +0x17C

    virtual void  enableWeapon() = 0;                                // +0x180 (E) void
    virtual void  disableWeapon() = 0;                               // +0x184
    virtual bool  getDisableOnFire() = 0;                            // +0x188
    virtual float getVelocity() const = 0;                           // +0x18C
    virtual IObjectTemplate* getProjectileTemplate() const = 0;      // +0x190

    virtual void  addNetworkableParts() = 0;                         // +0x194
    virtual void  removeNetworkableParts() = 0;                      // +0x198
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_IWEAPONOBJECT_H
