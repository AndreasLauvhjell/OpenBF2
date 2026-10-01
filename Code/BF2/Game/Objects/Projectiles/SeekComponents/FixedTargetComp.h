// Keeps a weak target until the tracking delay elapses, then drops inactive targets.
// The x86 component is 0x20 bytes.

#ifndef GAME_OBJECTS_PROJECTILES_SEEKCOMPONENTS_FIXEDTARGETCOMP_H
#define GAME_OBJECTS_PROJECTILES_SEEKCOMPONENTS_FIXEDTARGETCOMP_H

#include "../GenericProjectile.h"     // IProjectileSeekComp, TargetType
#include "../../WeaponComp.h"         // ProjectileComp
#include "../../../../Core/WeakPtr.h"

namespace dice {
namespace hfe {
namespace world {

class FixedTargetCompTemplate;
class IObject;

class FixedTargetComp : public ProjectileComp, public IProjectileSeekComp
{
public:
    explicit FixedTargetComp(FixedTargetCompTemplate* objectTemplate);
    virtual ~FixedTargetComp();

    // The leaf owns this second reference count; ProjectileComp's +0x04 count is initialised but
    // otherwise abandoned, matching every recovered projectile-component leaf.
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual bool hasUpdate() const;
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual void activate();

    // Complete IProjectileSeekComp tail, ordered by the ELF 0x88BEB50 and PE 0x7DD0A8 vtables.
    virtual void setTargetObject(IObject* target, bool value);
    virtual IObject* getTargetObject() const;
    virtual int getTrackingDelayTick() const;
    virtual float getMaxTargetDistance() const;
    virtual float getMaxTargetAngle() const;
    virtual TargetType getTargetType() const;

private:
    void checkTarget();

    /* +0x14 */ int m_refCount;
    /* +0x18 */ WeakPtr<IObject> m_targetObject;
    /* +0x1C */ FixedTargetCompTemplate* m_template;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_PROJECTILES_SEEKCOMPONENTS_FIXEDTARGETCOMP_H
