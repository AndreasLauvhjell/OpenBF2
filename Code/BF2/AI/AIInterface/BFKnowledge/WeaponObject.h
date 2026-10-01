// Object component exposing a live firearm and its AI weapon template.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_WEAPONOBJECT_H
#define AI_AIINTERFACE_BFKNOWLEDGE_WEAPONOBJECT_H

#include "IWeapon.h"
#include "WeaponTemplate.h"                     // WeaponGuidingSystem

namespace dice {
namespace hfe {
namespace world {
class IWeaponObject;
enum TargetSystem : int;                        // Game/Objects/CompTemplateBases.h
}
namespace ai {

class WeaponObject : public IWeapon
{
public:
    explicit WeaponObject(WeaponTemplate* weaponTemplate);
    virtual ~WeaponObject();

    // IBase
    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;

    // IObjectComponent
    virtual void init(world::IObject* object);                  // +0x14,
    virtual void setObject(world::IObject* object);             // +0x1C
    virtual world::IObject* getObject() const;                  // +0x20

    // IWeapon
    virtual bool update();                                      // +0x40
    virtual unsigned int getInformation(int side, int ownSide);         // +0x44
    virtual unsigned int createInformation(int side, int ownSide);      // +0x48
    virtual unsigned int createInformation(double time, int side, int ownSide);   // +0x4C
    virtual void deleteInformation(int side);                   // +0x50
    virtual void deleteAllInformation();                        // +0x54
    virtual const Mat4& getAbsoluteTransformation() const;      // +0x58
    virtual const Vec3& getPosition() const;                    // +0x5C
    virtual const Mat4& getProjectileOrigin();                  // +0x60
    virtual int getWeaponActivate() const;                      // +0x64
    virtual int getWeaponFire() const;                          // +0x68
    virtual int getWeaponZoom() const;                          // +0x6C
    virtual int getWeaponAltFire() const;                       // +0x70
    virtual bool isZoomed() const;                              // +0x74
    virtual int getWeaponState() const;                         // +0x78
    virtual float getDrag() const;                              // +0x7C
    virtual float getGravityModifier() const;                   // +0x80
    virtual float getExitVelocity() const;                      // +0x84
    virtual float getMinRange() const;                          // +0x88
    virtual float getMaxRange() const;                          // +0x8C
    virtual float getOptimalRangePercentage() const;            // +0x90
    virtual SoldierPose getFiringPose() const;                  // +0x94
    virtual float getAllowedDeviation() const;                  // +0x98
    virtual bool getFiresThroughTransparent() const;            // +0x9C
    virtual const int* getStrengths() const;                    // +0xA0
    virtual int getStrength(int index) const;                   // +0xA4
    virtual float getExplosionRadius() const;                   // +0xA8
    virtual bool getIsThrown() const;                           // +0xAC
    virtual bool useAimerOnly() const;                          // +0xB0
    virtual int getFireRate() const;                            // +0xB4
    virtual int getAmmo() const;                                // +0xB8
    virtual int getCurrentAmmo() const;                         // +0xBC
    virtual int getAmmoInMag(int mag) const;                    // +0xC0
    virtual int getMaxAmmo() const;                             // +0xC4
    virtual int getMaxAmmoInMag() const;                        // +0xC8
    virtual int getNumberOfMags() const;                        // +0xCC
    virtual bool hasAutoReload() const;                         // +0xD0
    virtual float getReloadTime() const;                        // +0xD4
    virtual bool refillAmmo(float fraction);                    // +0xD8
    virtual int getWeaponType() const;                          // +0xDC
    virtual int getWeaponGuidingSystem() const;                 // +0xE0
    virtual void setTimeOfFiring(double time);                  // +0xE4
    virtual double getTimeOfFiring() const;                     // +0xE8
    virtual bool getBurst() const;                              // +0xEC
    virtual bool getIndirect() const;                           // +0xF0
    virtual float getHeat() const;                              // +0xF4
    virtual bool hasHeat() const;                               // +0xF8
    virtual bool usesHeat() const;                              // +0xFC
    virtual bool isReadyToFire() const;                         // +0x100
    virtual bool setFakeFire(bool fake);                        // +0x104
    virtual bool getFakeFire() const;                           // +0x108
    virtual float getSoundSphereRadius() const;                 // +0x10C
    virtual int getProjectileMaterial() const;                  // +0x110
    virtual float getReplenishRadius() const;                   // +0x114
    virtual void setBotSkill(float skill, double since, float scale);   // +0x118
    virtual world::IObject* getProjectile() const;              // +0x11C
    virtual bool setTarget(unsigned int targetId);              // +0x120
    virtual bool hasTargetLocked() const;                       // +0x124
    virtual IAIObject* getLockedTarget() const;                 // +0x128
    virtual const std::string& getName() const;                 // +0x12C

    // non-virtual
    WeaponGuidingSystem targetSystemToWeaponGuidingSystem(world::TargetSystem system) const;
    int getCurrentMagSize() const;                              // no caller in either image
    float calculateDeviation(float skill, double since);        // no caller; see the .cpp
    bool checkIfInfoExists(unsigned int handle);

private:
    /* +0x04 */ int m_refCount;
    /* +0x08 */ world::IWeaponObject* m_weaponObject;
    /* +0x0C */ WeaponTemplate* m_template;
    /* +0x10 */ unsigned int m_information[3];
    /* +0x1C */ double m_timeOfFiring;
    /* +0x24 */ world::IObject* m_object;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_WEAPONOBJECT_H
