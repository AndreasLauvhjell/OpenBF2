// Default projectile target-following behavior.

#ifndef GAME_OBJECTS_PROJECTILES_FOLLOWCOMPONENTS_DEFAULTFOLLOWCOMP_H
#define GAME_OBJECTS_PROJECTILES_FOLLOWCOMPONENTS_DEFAULTFOLLOWCOMP_H

#include "../GenericProjectile.h"      // IProjectileFollowComp
#include "../../WeaponComp.h"          // ProjectileComp
#include "../../../../Core/WeakPtr.h"

namespace dice {
namespace hfe {
namespace world {

class DefaultFollowCompTemplate;
class IObject;

// Both retail x86 ports allocate exactly 0x44 bytes and place the follow face at +0x10.
class DefaultFollowComp : public ProjectileComp, public IProjectileFollowComp
{
public:
    explicit DefaultFollowComp(DefaultFollowCompTemplate* objectTemplate);

    virtual ~DefaultFollowComp();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual bool hasUpdate() const;                     // false
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual void activate();

    virtual void setTargetObject(IObject* target);
    virtual void setDelayTick(unsigned int tick);

private:
    bool followTarget(IObject* target);

    /* +0x14 */ int m_refCount;
    /* +0x18 */ DefaultFollowCompTemplate* m_template;
    /* +0x1C */ float m_yawTurn;
    /* +0x20 */ float m_pitchTurn;
    /* +0x24 */ WeakPtr<IObject> m_targetObject;
    /* +0x28 */ unsigned int m_delayTick;
    /* +0x2C */ float m_initialSpeedMagnitude;
    /* +0x30 */ bool m_firstFollow;
                 // +0x31..+0x33 padding.
    /* +0x34 */ Vec3 m_previousSpeed;
    /* +0x40 */ bool m_terminal;
    /* +0x41 */ bool m_killDistanceReached;
                 // +0x42..+0x43 padding.
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_PROJECTILES_FOLLOWCOMPONENTS_DEFAULTFOLLOWCOMP_H
