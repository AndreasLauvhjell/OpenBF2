// Path:     attested - the ELF Debug literal names
// Game/Objects/Projectiles/CollisionComponents/ResurrectCollisionComp.h.
// Contents: the resurrecting projectile collision component recovered from the symbolized
// BF2 1.50 ELF and cross-checked against bf2_w32ded.exe.

#ifndef GAME_OBJECTS_PROJECTILES_COLLISIONCOMPONENTS_RESURRECTCOLLISIONCOMP_H
#define GAME_OBJECTS_PROJECTILES_COLLISIONCOMPONENTS_RESURRECTCOLLISIONCOMP_H

#include "../GenericProjectile.h"
#include "../../WeaponComp.h"

namespace dice {
namespace hfe {
namespace world {

class ResurrectCollisionCompTemplate;

class ResurrectCollisionComp : public ProjectileComp, public IProjectileCollisionComp
{
public:
    explicit ResurrectCollisionComp(ResurrectCollisionCompTemplate* objectTemplate);
    virtual ~ResurrectCollisionComp();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual bool hasUpdate() const;
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual void activate();

    virtual void resetProjectile(bool keepParent);
    virtual void preResetProjectile(bool keepParent);
    virtual bool handleCollision(IObject* target, const Vec3& velocity, const Vec3& normal,
                                 const Vec3& collisionOffset, int collisionType, int b,
                                 int collisionTypeOverride, CollisionFaceInfo* face1,
                                 CollisionFaceInfo* face2);
    virtual int getDefaultCollisionType(int collisionType) const;
    virtual void stick(IObject* host, const Mat4& transform);
    virtual void stickAbsolute(const Mat4& transform);
    virtual IObject* getHostObject() const;
    virtual const Mat4& getStickTransformation() const;
    virtual bool hasCollisionEffect() const;
    virtual bool getIsBouncing() const;
    virtual float getHealEffect(Vec3& out) const;

    // A Resurrect-only primary-table slot (ELF 0x828B6E0); it is not part of the shared
    // IProjectileCollisionComp face and returns false unconditionally in retail.
    virtual bool getDieAfterColl();

private:
    /* +0x14 */ int m_refCount;
    /* +0x18 */ ResurrectCollisionCompTemplate* m_template;
    /* +0x1C */ int m_dword1C; // Constructor writes zero; no recovered body reads it.
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_PROJECTILES_COLLISIONCOMPONENTS_RESURRECTCOLLISIONCOMP_H
