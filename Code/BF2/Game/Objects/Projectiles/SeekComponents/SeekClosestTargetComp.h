// Acquires the nearest eligible projectile target.

#ifndef GAME_OBJECTS_PROJECTILES_SEEKCOMPONENTS_SEEKCLOSESTTARGETCOMP_H
#define GAME_OBJECTS_PROJECTILES_SEEKCOMPONENTS_SEEKCLOSESTTARGETCOMP_H

#include "../GenericProjectile.h"     // IProjectileSeekComp, TargetType
#include "../../WeaponComp.h"         // ProjectileComp
#include "../../../../Core/WeakPtr.h"

namespace dice {
namespace hfe {
namespace world {

class IObject;
class SeekClosestTargetCompTemplate;

// Both retail x86 ports allocate exactly 0x34 bytes. The secondary seek face starts at +0x10;
// the three WeakPtr members are one-word control-block handles in both shipped runtimes.
class SeekClosestTargetComp : public ProjectileComp, public IProjectileSeekComp
{
public:
    explicit SeekClosestTargetComp(SeekClosestTargetCompTemplate* objectTemplate);

    virtual ~SeekClosestTargetComp();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual bool hasUpdate() const;                      // true
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual void activate();

    // Complete IProjectileSeekComp tail. PE's secondary table at 0x7DCFB8 and the symbolized ELF
    // thunks agree on this exact order.
    virtual void setTargetObject(IObject* target, bool forced);
    virtual IObject* getTargetObject() const;
    virtual int getTrackingDelayTick() const;
    virtual float getMaxTargetDistance() const;
    virtual float getMaxTargetAngle() const;
    virtual TargetType getTargetType() const;

private:
    void lockVehicle(IObject* vehicle);
    int getTargetObjectNetworkId() const;
    void seekForBestTarget();
    void checkTarget();

    /* +0x14 */ int m_refCount;
    /* +0x18 */ WeakPtr<IObject> m_targetObject;
    /* +0x1C */ WeakPtr<IObject> m_lockedVehicle;
    /* +0x20 */ bool m_reLockPending;
    /* +0x21 */ bool m_forcedTarget;
                 // +0x22..+0x23 padding.
    /* +0x24 */ WeakPtr<IObject> m_pendingVehicle;
    /* +0x28 */ unsigned int m_reLockStartTick;
    /* +0x2C */ SeekClosestTargetCompTemplate* m_template;
    /* +0x30 */ float m_ignoreFriendlyElapsed;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_PROJECTILES_SEEKCOMPONENTS_SEEKCLOSESTTARGETCOMP_H
