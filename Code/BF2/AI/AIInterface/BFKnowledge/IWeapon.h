// Object-component interface exposing an engine firearm to the AI.
// [INFERRED] Return types are derived from implementations. Preserve overload declaration order;
// MSVC reverses the createInformation pair when laying out the virtual table.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_IWEAPON_H
#define AI_AIINTERFACE_BFKNOWLEDGE_IWEAPON_H

#include <string>

#include "AIAdapterTypes.h"
#include "SoldierPose.h"
#include "../../../Game/Objects/IObjectComponent.h"

namespace dice {
namespace hfe {
namespace ai {

class IAIObject;

class IWeapon : public world::IObjectComponent
{
public:
    virtual ~IWeapon() {}                                                       // +0x38/+0x3C

    virtual bool update() = 0;                                                  // +0x40 (WeaponObject: true)
    virtual unsigned int getInformation(int side, int ownSide) = 0;             // +0x44
    virtual unsigned int createInformation(int side, int ownSide) = 0;          // +0x48
    virtual unsigned int createInformation(double time, int side, int ownSide) = 0;   // +0x4C
    virtual void deleteInformation(int side) = 0;                               // +0x50
    virtual void deleteAllInformation() = 0;                                    // +0x54
    virtual const Mat4& getAbsoluteTransformation() const = 0;                  // +0x58
    virtual const Vec3& getPosition() const = 0;                                // +0x5C
    virtual const Mat4& getProjectileOrigin() = 0;                              // +0x60
    virtual int getWeaponActivate() const = 0;                                  // +0x64
    virtual int getWeaponFire() const = 0;                                      // +0x68
    virtual int getWeaponZoom() const = 0;                                      // +0x6C
    virtual int getWeaponAltFire() const = 0;                                   // +0x70
    virtual bool isZoomed() const = 0;                                          // +0x74
    virtual int getWeaponState() const = 0;                                     // +0x78
    virtual float getDrag() const = 0;                                          // +0x7C
    virtual float getGravityModifier() const = 0;                               // +0x80
    virtual float getExitVelocity() const = 0;                                  // +0x84
    virtual float getMinRange() const = 0;                                      // +0x88
    virtual float getMaxRange() const = 0;                                      // +0x8C
    virtual float getOptimalRangePercentage() const = 0;                        // +0x90
    virtual SoldierPose getFiringPose() const = 0;                              // +0x94
    virtual float getAllowedDeviation() const = 0;                              // +0x98
    virtual bool getFiresThroughTransparent() const = 0;                        // +0x9C
    virtual const int* getStrengths() const = 0;                                // +0xA0
    virtual int getStrength(int index) const = 0;                               // +0xA4
    virtual float getExplosionRadius() const = 0;                               // +0xA8
    virtual bool getIsThrown() const = 0;                                       // +0xAC
    virtual bool useAimerOnly() const = 0;                                      // +0xB0
    virtual int getFireRate() const = 0;                                        // +0xB4
    virtual int getAmmo() const = 0;                                            // +0xB8
    virtual int getCurrentAmmo() const = 0;                                     // +0xBC
    virtual int getAmmoInMag(int mag) const = 0;                                // +0xC0
    virtual int getMaxAmmo() const = 0;                                         // +0xC4
    virtual int getMaxAmmoInMag() const = 0;                                    // +0xC8
    virtual int getNumberOfMags() const = 0;                                    // +0xCC
    virtual bool hasAutoReload() const = 0;                                     // +0xD0
    virtual float getReloadTime() const = 0;                                    // +0xD4
    virtual bool refillAmmo(float fraction) = 0;                                // +0xD8
    virtual int getWeaponType() const = 0;                                      // +0xDC
    virtual int getWeaponGuidingSystem() const = 0;                             // +0xE0
    virtual void setTimeOfFiring(double time) = 0;                              // +0xE4
    virtual double getTimeOfFiring() const = 0;                                 // +0xE8
    virtual bool getBurst() const = 0;                                          // +0xEC
    virtual bool getIndirect() const = 0;                                       // +0xF0
    virtual float getHeat() const = 0;                                          // +0xF4
    virtual bool hasHeat() const = 0;                                           // +0xF8
    virtual bool usesHeat() const = 0;                                          // +0xFC
    virtual bool isReadyToFire() const = 0;                                     // +0x100
    virtual bool setFakeFire(bool fake) = 0;                                    // +0x104
    virtual bool getFakeFire() const = 0;                                       // +0x108
    virtual float getSoundSphereRadius() const = 0;                             // +0x10C
    virtual int getProjectileMaterial() const = 0;                              // +0x110
    virtual float getReplenishRadius() const = 0;                               // +0x114
    virtual void setBotSkill(float skill, double since, float scale) = 0;       // +0x118
    virtual world::IObject* getProjectile() const = 0;                          // +0x11C
    virtual bool setTarget(unsigned int targetId) = 0;                          // +0x120
    virtual bool hasTargetLocked() const = 0;                                   // +0x124
    virtual IAIObject* getLockedTarget() const = 0;                             // +0x128
    virtual const std::string& getName() const = 0;                             // +0x12C
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_IWEAPON_H
