// Path:     [INFERRED] beside the attested LPTargetComp.cpp.
// Contents: shared target-component runtime recovered from the BF2 1.50 Linux ELF and cross-read
// against the Win32 dedicated-server PE.  The concrete leaves own a second refcount.

#ifndef GAME_OBJECTS_WEAPONS_TARGETCOMPONENTS_TARGETCOMP_H
#define GAME_OBJECTS_WEAPONS_TARGETCOMPONENTS_TARGETCOMP_H

#include "../FireComponents/FireComp.h"
#include "../../CompTemplateBases.h"

namespace dice {
namespace hfe {
namespace world {

class GenericFireArm;
class IWeaponObject;

class TargetComp : public WeaponComp, public IWeaponTargetComp
{
public:
    explicit TargetComp(TargetCompTemplate* objectTemplate);
    virtual ~TargetComp();

    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual void initComponent(WeaponInitEvent& event);
    virtual bool preUpdate();
    virtual void updateComponent(float dt, unsigned int tick);
    virtual bool isSelectable() const;
    virtual void fireStop(bool primary);
    virtual bool requestFire(bool released, bool alternate);
    virtual void postFire(WeaponFireEvent& event);
    virtual void fireComplete(unsigned int tick);
    virtual void postSwitchFireRate(FireRate rate);
    virtual bool requestReload(bool primary);
    virtual void reload(ReloadState state);
    virtual void postReload();
    virtual void outOfAmmo(bool primary);
    virtual void setZoom(int camera);
    virtual void stoppedTracking();
    virtual void postChangeTarget(IObject* target);
    virtual void projectileWasReset(IObject* projectile);
    virtual void setWeaponState(unsigned int state);
    virtual unsigned int getWeaponState() const;
    virtual bool hasMissileConnection() const;
    virtual void enable();
    virtual void disable();
    virtual void stop();
    virtual bool busy();
    virtual void exit();
    virtual void destroy();
    virtual void pickup();
    virtual void reset();
    virtual void setCurrentProjectile(IObject* projectile);
    virtual void setTargetObject(IObject* target, bool locked, IObject* lockVehicle);
    virtual IObject* getTargetObject() const;
    virtual bool hasTargetLock() const;
    virtual float getDynamicVelocity() const;
    virtual void setAllowTargetSeek(bool allow);
    virtual void setTargetDirection(Vec3 start, Vec3 end);
    virtual io::PlayerInputMap getTargetInput() const;
    virtual bool getSignalChangeTarget();
    virtual int getTargetSystem() const;
    virtual float getMaxTargetDistance() const;
    virtual IObject* getProjectile() const;
    virtual int getTargetObjectNetworkId() const;

    // This leaf-independent slot follows the IWeaponTargetComp surface in the shipped primary
    // table.  LPTargetComp overrides it; the other two leaves inherit false.
    virtual bool getLaunched();

    // Used when composing MissileLockMessage.  This helper is non-virtual in both shipped
    // binaries; the target-system delegates above are the polymorphic surface.
    int getTargetType() const;

protected:
    GenericFireArm* firearm() const
    {
        return reinterpret_cast<GenericFireArm*>(m_initObject);
    }

    void lockVehicle(IObject* vehicle);

    /* +0x14 */ int m_targetRefCount;
    /* +0x18 */ Vec3 m_targetPoint;
    /* +0x24 */ IWeaponObject* m_onLockWeapon;
    /* +0x28 */ unsigned int m_onLockWeaponLastTick;
    /* +0x2C */ WeakPtr<IObject> m_targetObject;
    /* +0x30 */ bool m_targetLock;
                 // +0x31..+0x33 padding.
    /* +0x34 */ WeakPtr<IObject> m_lockVehicle;
    /* +0x38 */ bool m_reloadLatch;
    /* +0x39 */ bool m_signalChangeTarget;
                 // +0x3A..+0x3B padding.
    /* +0x3C */ int m_dword3C;
    /* +0x40 */ IObject* m_connectionEffect;
    /* +0x44 */ bool m_zoomed;
    /* +0x45 */ bool m_allowTargetSeek;
                 // +0x46..+0x47 padding.
    /* +0x48 */ TargetCompTemplate* m_targetTemplate;
};

#if defined(_MSC_VER) && defined(_M_IX86)
static_assert(sizeof(TargetComp) == 0x4C, "TargetComp x86 ABI drift");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_TARGETCOMPONENTS_TARGETCOMP_H
