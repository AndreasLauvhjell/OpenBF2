// Routes projectile impacts to game logic after special-target handling and animation bookkeeping.
// Default stick operations are inert; getStickTransformation reports a refusal.
// Preserve the ProjectileComp and collision-interface bases. Header path is attested by __FILE__.

#ifndef GAME_OBJECTS_PROJECTILES_COLLISIONCOMPONENTS_DEFAULTCOLLISIONCOMP_H
#define GAME_OBJECTS_PROJECTILES_COLLISIONCOMPONENTS_DEFAULTCOLLISIONCOMP_H

#include "../GenericProjectile.h"      // IProjectileCollisionComp, CollisionFaceInfo
#include "../../WeaponComp.h"          // ProjectileComp - the shared 16-byte base

namespace dice {
namespace hfe {
namespace world {

class DefaultCollisionCompTemplate;
class IObject;

class DefaultCollisionComp : public ProjectileComp, public IProjectileCollisionComp
{
public:
    explicit DefaultCollisionComp(DefaultCollisionCompTemplate* objectTemplate);
    virtual ~DefaultCollisionComp();

    // The quartet. This leaf owns its reference count; ProjectileComp's is set once and abandoned.
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual bool hasUpdate() const;
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual void activate();

    virtual void resetProjectile(bool value);
    virtual void preResetProjectile(bool value);
    virtual bool handleCollision(IObject* target, const Vec3& velocity, const Vec3& normal,
                                 const Vec3& collisionOffset, int a, int b, int c,
                                 CollisionFaceInfo* face1, CollisionFaceInfo* face2);
    virtual int  getDefaultCollisionType(int collisionType) const;
    virtual void stick(IObject* host, const Mat4& transform);
    virtual void stickAbsolute(const Mat4& transform);
    virtual IObject* getHostObject() const;
    virtual const Mat4& getStickTransformation() const;
    virtual bool hasCollisionEffect() const;
    virtual bool getIsBouncing() const;
    virtual float getHealEffect(Vec3& out) const;

private:
    /* +0x14 */ int m_refCount;
    /* +0x18 */ DefaultCollisionCompTemplate* m_template;
    /* +0x1C */ int m_dword1C;          // ctor 0, read by nothing recovered. [UNRESOLVED]
    // Prevents replaying the damage animation after a collision reaches GameLogic.
    // resetProjectile and activate leave this latch set, including for pooled projectiles. [CHOSEN] Name.
    /* +0x20 */ bool m_collisionHandled;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_PROJECTILES_COLLISIONCOMPONENTS_DEFAULTCOLLISIONCOMP_H
