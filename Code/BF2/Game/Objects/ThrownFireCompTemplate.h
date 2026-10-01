// Configuration for thrown-projectile firing.

#ifndef GAME_OBJECTS_THROWNFIRECOMPTEMPLATE_H
#define GAME_OBJECTS_THROWNFIRECOMPTEMPLATE_H

#include "FireCompTemplate.h"

namespace dice {
namespace hfe {
namespace world {

class ThrownFireCompTemplate : public FireCompTemplate
{
public:
    ThrownFireCompTemplate();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void* queryImplementation(unsigned int classId);
    virtual ~ThrownFireCompTemplate();
    virtual bool  makeScript(io::IStream* stream);

    void setFireLaunchDelaySoft(float value);
    void setShowWeaponAfterReloadDelay(float value);
    void setPullbackTime(float value);
    void setTimeToReachMaxStrengthSlow(float value);

    // These two properties are generated direct-field console bindings in retail, so no named
    // out-of-line accessor survives.  The source spellings are inferred from their exact console
    // names; the stores and runtime consumers are recovered in both ports.
    void setRandomRotationSpeed(float value) { m_randomRotationSpeed = value; }
    float getRandomRotationSpeed() const { return m_randomRotationSpeed; }
    void setProjectileStartRotation(const Vec3& value) { m_projectileStartRotation = value; }
    const Vec3& getProjectileStartRotation() const { return m_projectileStartRotation; }

    float getTimeToReachMaxStrengthSlow() const { return m_timeToReachMaxStrengthSlow; }
    float getInverseStrengthPerTick() const { return m_inverseStrengthPerTick; }
    float getPullbackTime() const { return m_pullbackTime; }
    int getPullbackTimeInTicks() const { return m_pullbackTimeTimes30; }
    float getFireLaunchDelaySoft() const { return m_fireLaunchDelaySoft; }
    int getFireLaunchDelaySoftInTicks() const { return m_fireLaunchDelaySoftTimes30; }
    float getShowWeaponAfterReloadDelay() const { return m_showWeaponAfterReloadDelay; }
    int getShowWeaponAfterReloadDelayInTicks() const
    {
        return m_showWeaponAfterReloadDelayTimes30;
    }

protected:
    // Documentary ELF offsets.  FireCompTemplate is 0x74 and this leaf is 0xA8.  The PE base is
    // four bytes larger because its std::vector representation differs, but the source order is
    // identical (PE makeScript reads the corresponding fields at +0x78..+0xA4).
    /* +0x74 */ int   m_refCount;
    /* +0x78 */ float m_timeToReachMaxStrengthSlow;
    /* +0x7C */ float m_inverseStrengthPerTick;
    /* +0x80 */ float m_pullbackTime;
    /* +0x84 */ int   m_pullbackTimeTimes30;
    /* +0x88 */ float m_fireLaunchDelaySoft;
    /* +0x8C */ int   m_fireLaunchDelaySoftTimes30;
    /* +0x90 */ float m_showWeaponAfterReloadDelay;
    /* +0x94 */ int   m_showWeaponAfterReloadDelayTimes30;
    /* +0x98 */ float m_randomRotationSpeed;
    /* +0x9C */ Vec3  m_projectileStartRotation;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_THROWNFIRECOMPTEMPLATE_H
