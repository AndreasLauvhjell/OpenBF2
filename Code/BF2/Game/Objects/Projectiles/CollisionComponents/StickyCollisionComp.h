// Schedules projectile attachment on collision and applies/broadcasts it on the next component tick.
// Networked non-control-point hosts use the nearest rotational bundle so attachments follow moving parts.
// Stick angle is a cosine of clamped degrees; the -1 default accepts every normal.
// Retains a weak host and deferred transformation. Header placement is [INFERRED].

#ifndef GAME_OBJECTS_PROJECTILES_COLLISIONCOMPONENTS_STICKYCOLLISIONCOMP_H
#define GAME_OBJECTS_PROJECTILES_COLLISIONCOMPONENTS_STICKYCOLLISIONCOMP_H

#include "../GenericProjectile.h"      // IProjectileCollisionComp, CollisionFaceInfo
#include "../../WeaponComp.h"          // ProjectileComp
#include "../../../../Math/Mat4.h"
#include "../../../../Core/WeakPtr.h"

namespace dice {
namespace hfe {
namespace world {

class StickyCollisionCompTemplate;
class IObject;

class StickyCollisionComp : public ProjectileComp, public IProjectileCollisionComp
{
public:
    explicit StickyCollisionComp(StickyCollisionCompTemplate* objectTemplate);
    virtual ~StickyCollisionComp();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual bool hasUpdate() const;
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual void activate();

    virtual void resetProjectile(bool keepParent);
    virtual void preResetProjectile(bool keepParent);
    virtual bool handleCollision(IObject* target, const Vec3& velocity, const Vec3& normal,
                                 const Vec3& collisionOffset, int collisionType, int b,
                                 int collisionTypeOverride,
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
    // . IT reads m_object (+0x08), NOT m_initObject (+0x0C) - the distinction
    // WeaponComp.h warns about, and the only place in this class that uses the other pointer.
    void restoreProjectileState(bool keepParent);

    /* +0x14 */ int m_refCount;
    /* +0x18 */ StickyCollisionCompTemplate* m_template;
    /* +0x1C */ int m_dword1C;          // ctor 0, read by nothing recovered. [UNRESOLVED]
    // THE host handle. The ELF shows the increment / decrement / delete dance written out inline
    // at both assignment sites - which is exactly what WeakPtr's copy-assign compiles to, so the
    // source-level form is a plain WeakPtr member and an assignment.
    /* +0x20 */ WeakPtr<IObject> m_hostHandle;
    // THE deferral latch. handleCollision sets it, updateComponent consumes it. See the file note.
    /* +0x24 */ bool m_pendingStick;
    /* +0x28 */ Mat4 m_stickTransform;  // ctor: identity
    // Remembers that the physics node was turned off, so resetProjectile knows to turn it back on.
    /* +0x68 */ bool m_physicsDisabled;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_PROJECTILES_COLLISIONCOMPONENTS_STICKYCOLLISIONCOMP_H
