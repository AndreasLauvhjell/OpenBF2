// After the delay, runs a debug-only ballistic trace when a target exists.
// The target gates this trace; it does not steer the projectile.

#ifndef GAME_OBJECTS_PROJECTILES_FOLLOWCOMPONENTS_PREDESTINEDFOLLOWCOMP_H
#define GAME_OBJECTS_PROJECTILES_FOLLOWCOMPONENTS_PREDESTINEDFOLLOWCOMP_H

#include <list>

#include "../GenericProjectile.h"      // IProjectileFollowComp
#include "../../WeaponComp.h"          // ProjectileComp
#include "../../../../Core/WeakPtr.h"

namespace dice {
namespace hfe {
namespace world {

class IObject;
class PreDestinedFollowCompTemplate;

class PreDestinedFollowComp : public ProjectileComp, public IProjectileFollowComp
{
public:
    explicit PreDestinedFollowComp(PreDestinedFollowCompTemplate* objectTemplate);

    virtual ~PreDestinedFollowComp();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual bool hasUpdate() const;                      // false
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual void activate();

    virtual void setTargetObject(IObject* target);
    virtual void setDelayTick(unsigned int tick);

private:
    bool followTarget(IObject* target, float seconds);

    /* +0x14 */ int m_refCount;
    /* +0x18 */ PreDestinedFollowCompTemplate* m_template;
    /* +0x1C */ WeakPtr<IObject> m_targetObject;
    /* +0x20 */ unsigned int m_delayTick;
    /* +0x24 */ bool m_firstFollow;
    /* +0x28 */ Vec3 m_launchSpeed;
    /* +0x34 */ float m_launchWorldTime;
    /* +0x38 */ std::list<Vec3> m_debugPoints;
    // [PLATFORM] Retail ELF's old libstdc++ list is one word and puts this at +0x3C. Retail PE's
    // MSVC list is twelve bytes and puts it at +0x44. Keep source-level STL, never force offsets.
    int m_debugPointCounter;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_PROJECTILES_FOLLOWCOMPONENTS_PREDESTINEDFOLLOWCOMP_H
