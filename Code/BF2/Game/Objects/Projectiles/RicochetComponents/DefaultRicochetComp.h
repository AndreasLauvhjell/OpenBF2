// Ricochet solver with fixed deviation and no material tuning.
// Damage output changes even on refusal; successful bounces update origin and lifetime.
// Keep its private random stream separate from penetration. Source placement is [INFERRED].

#ifndef GAME_OBJECTS_PROJECTILES_RICOCHETCOMPONENTS_DEFAULTRICOCHETCOMP_H
#define GAME_OBJECTS_PROJECTILES_RICOCHETCOMPONENTS_DEFAULTRICOCHETCOMP_H

#include "../GenericProjectile.h"      // IProjectileRicochetComp
#include "../../WeaponComp.h"          // ProjectileComp

namespace dice {
namespace hfe {
namespace world {

class DefaultRicochetCompTemplate;

// THE secondary vtable IS walked (ELF 0x88BF6DC / PE 0x7DCDEC): the shared IProjectileComp
// face, the ABI-specific destructor slot(s), then exactly one appended operation, ricochet.
class DefaultRicochetComp : public ProjectileComp, public IProjectileRicochetComp
{
public:
    explicit DefaultRicochetComp(DefaultRicochetCompTemplate* objectTemplate);
    virtual ~DefaultRicochetComp();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual bool hasUpdate() const;
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual void activate();

    // . Parameter names confirmed against the
    // body: the reflection formula fixes incomingVelocity/surfaceNormal, the damage arithmetic fixes
    // retainedFraction/baseProjectileDamage/currentDamage, and the TTL recomputation fixes
    // sampledLifetime/lastRicochetTime.
    virtual bool ricochet(const Vec3& impactOffset,
                          const Vec3& incomingVelocity,
                          const Vec3& surfaceNormal,
                          float retainedFraction,
                          float baseProjectileDamage,
                          float sampledLifetime,
                          float& lastRicochetTime,
                          Vec3& ricochetWorldPosition,
                          float& currentDamage);

private:
    /* +0x14 */ int m_refCount;
    /* +0x18 */ DefaultRicochetCompTemplate* m_template;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_PROJECTILES_RICOCHETCOMPONENTS_DEFAULTRICOCHETCOMP_H
