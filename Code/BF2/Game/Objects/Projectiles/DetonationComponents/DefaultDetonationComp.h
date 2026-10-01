// Projectile explosion and proximity-trigger component. Only proximity/radius/arming settings enable updates;
// ordinary projectiles detonate from collisions. Explosion material -1 disables the explosion.
// Component construction lazily resolves effect names. Flashbang settings are client presentation data.
// Source placement is [INFERRED].

#ifndef GAME_OBJECTS_PROJECTILES_DETONATIONCOMPONENTS_DEFAULTDETONATIONCOMP_H
#define GAME_OBJECTS_PROJECTILES_DETONATIONCOMPONENTS_DEFAULTDETONATIONCOMP_H

#include "../GenericProjectile.h"      // IProjectileDetonationComp
#include "../../WeaponComp.h"          // ProjectileComp
#include "../../../../Core/WeakPtr.h"

namespace dice {
namespace hfe {
namespace world {

class DefaultDetonationCompTemplate;
class IObject;
class IObjectTemplate;

class DefaultDetonationComp : public ProjectileComp, public IProjectileDetonationComp
{
public:
    explicit DefaultDetonationComp(DefaultDetonationCompTemplate* objectTemplate);
    virtual ~DefaultDetonationComp();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual bool hasUpdate() const;
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual void activate();

    virtual void detonate(IObject* source, int collisionType, Vec3 position,
                          unsigned int tick, Vec3 normal,
                          CollisionFaceInfo* face);
    virtual void setTargetObject(IObject* target);
    virtual bool getDetonate() const;
    virtual void setDetonate(bool value);
    virtual bool getPlayMaterialEffectAtSurface() const;
    virtual bool hasExplosion() const;
    virtual int  getExplosionMaterial() const;
    virtual float getExplosionRadius() const;
    virtual float getExplosionInnerConeRadius() const;

    // the read surface beyond the interface
    // ALL pass-throughs to the template. Every one is 14-17 bytes in the ELF: load m_template,
    // load one field, return. The offsets in the .cpp are the recovered ones.
    float getFlashbangRadiusWithNightVision() const;
    float getFlashbangBillboardMinAlpha() const;
    float getFlashbangBillboardMaxAlpha() const;
    float getFlashbangBillboardMaxRampTime() const;
    float getFlashbangBillboardMinHoldTime() const;
    float getFlashbangBillboardMaxHoldTime() const;
    float getFlashbangBillboardMinHealTime() const;
    float getFlashbangBillboardMaxHealTime() const;
    float getFlashbangAdditiveGlowMinAlpha() const;
    float getFlashbangAdditiveGlowMaxAlpha() const;
    float getFlashbangAdditiveGlowMaxRampTime() const;
    float getFlashbangAdditiveGlowMinHoldTime() const;
    float getFlashbangAdditiveGlowMaxHoldTime() const;
    float getFlashbangAdditiveGlowMinHealTime() const;
    float getFlashbangAdditiveGlowMaxHealTime() const;
    float getFlashbangMotionBlurMinAlpha() const;
    float getFlashbangMotionBlurMaxAlpha() const;
    float getFlashbangMotionBlurMaxRampTime() const;
    float getFlashbangMotionBlurMinHoldTime() const;
    float getFlashbangMotionBlurMaxHoldTime() const;
    float getFlashbangMotionBlurMinHealTime() const;
    float getFlashbangMotionBlurMaxHealTime() const;

    float getExplosionSoldierLineOfSight() const;
    float getExplosionSoldierLineOfSightTimer() const;
    float getExplosionSoldierLineOfSightHealRate() const;
    float getExplosionLineOfSightMinimumDamage() const;
    float getExplosionConeAngle() const;
    float getExplosionYMod() const;
    float getExplosionForce() const;
    float getExplosionDamage() const;
    float getDetectionRadius() const;
    bool  getShowMineIndicationIcon() const;
    int   getRadiusTriggerType() const;
    IObjectTemplate* getEndEffect() const;
    // THE ONE getter that reads this component, not the template: its own countdown at +0x30,
    // seeded from the template's +0xE0 by the constructor and decremented by updateComponent.
    float getTimeUntilCanDetonate() const;

protected:
    // . The complete mine/claymore trigger predicate.
    // ReplenishDetonationComp overrides the final primary-table slot (ELF vtable 0x88BE620;
    // PE vtable 0x7DCB5C), so this is protected virtual rather than a private helper.
    virtual bool checkIfObjectCanTrigger(IObject* candidate);

private:
    // remains empty in the dedicated image. Client PE 0x617AE0 creates,
    // orients, initializes and starts the authored effect.
    void playDetonationEffect(IObjectTemplate* effect, Vec3 position, Vec3 rotation,
                              IPlayer* player, int material);

    /* +0x14 */ int m_refCount;
    /* +0x18 */ DefaultDetonationCompTemplate* m_template;
    /* +0x1C */ bool m_detonate;            // ctor false; get/setDetonate, and detonate() raises it
    // setTargetObject writes the increment/decrement/delete dance out inline - which is what
    // WeakPtr's copy-assign compiles to, so the source form is a WeakPtr and an assignment.
    /* +0x20 */ WeakPtr<IObject> m_targetHandle;
    // THE poll clock. Seeded from a global at ds:0x8AB0278 and counted down by updateComponent;
    // when it goes negative the proximity and radius arms run and it is reseeded. That global is the
    // trigger poll interval - mines do NOT test every tick.
    /* +0x24 */ float m_pollTimer;
    /* +0x28 */ float m_float28;            // ctor from template +0x84. [UNRESOLVED] role
    /* +0x2C */ bool m_byte2C;              // ctor false; updateComponent's radius arm raises it
    /* +0x30 */ float m_timeUntilCanDetonate;   // ctor from template +0xE0
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_PROJECTILES_DETONATIONCOMPONENTS_DEFAULTDETONATIONCOMP_H
