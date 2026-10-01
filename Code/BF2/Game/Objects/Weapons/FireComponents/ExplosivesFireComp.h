// Path:     [INFERRED] beside the attested FireComp.h; this leaf filename is not in either
// canonical shipped source-path map.
// Contents: complete runtime surface and source member order recovered from both retail images.

#ifndef GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_EXPLOSIVESFIRECOMP_H
#define GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_EXPLOSIVESFIRECOMP_H

#include <set>

#include "FireComp.h"

namespace dice {
namespace hfe {
namespace world {

class ExplosivesFireCompTemplate;

class ExplosivesFireComp : public FireComp
{
public:
    explicit ExplosivesFireComp(ExplosivesFireCompTemplate* objectTemplate);
    virtual ~ExplosivesFireComp();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    virtual void initComponent(WeaponInitEvent& event);
    virtual bool preUpdate();
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual bool requestFire(bool first, bool second);
    virtual bool launchesProjectiles(int index) const;
    virtual int fire(bool value, int index);
    virtual void fireComplete(unsigned int tick);
    virtual bool requestReload(bool value);
    virtual void reload(ReloadState state);
    virtual void postReload();
    virtual void projectileWasReset(IObject* projectile);
    virtual void enable();
    virtual void disable();
    virtual bool busy();
    virtual void destroy();
    virtual void pickup();
    virtual void reset();

    virtual IObject* getAlternateObject() const;
    virtual void switchFireRate();
    virtual void setWeaponState(unsigned int state);
    virtual unsigned int getWeaponState() const;
    virtual void addProjectileReference(IObject* projectile);
    virtual bool ownsProjectile(IObject* projectile) const;
    virtual bool releaseProjectile(IObject* projectile);

protected:
    // This is the final primary-table override after FireComp::getProjectileOrigin.  Both ports
    // dispatch it virtually from FireComp's launch path (ELF 0x82615C0; PE 0x5B0500).
    virtual IObject* createProjectile(const IObjectTemplate* objectTemplate,
                                      const Mat4& transform);

private:
    enum WeaponState : int;

    void showDetonator(bool show);
    void fireCompleteUpdateStates();
    bool launchProjectile(int index);
    void toggleWeaponState(WeaponState state);
    void destroyPlantedCharges(bool trigger);
    void triggerPlantedCharges();

    /* +0x128 */ int          m_refCount;
    /* +0x12C */ unsigned int m_fireCompleteAtTick;
    /* +0x130 */ unsigned int m_fireLaunchDelayStartedAtTick;
    /* +0x134 */ bool         m_waitingForFireLaunchDelay;
    /* +0x138 */ WeaponState  m_weaponState;
    /* +0x13C */ WeaponState  m_pendingWeaponState;
    /* +0x140 */ std::set<IObject*> m_plantedProjectiles;
    /* +0x14C */ unsigned int m_toggleCompleteAtTick;
    /* +0x150 */ bool         m_switchToDetonatorAfterFire;
    /* +0x154 */ unsigned int m_lastBusyAtTick;
    /* +0x158 */ unsigned int m_triggerChargesAtTick;
    /* +0x15C */ bool         m_triggerChargesPending;
    /* +0x160 */ unsigned int m_explosivesReloadAtTick;
    /* +0x164 */ bool         m_reloading;
    /* +0x168 */ IObject*     m_alternateObject;
    /* +0x16C */ bool         m_destroyingPlantedCharges;
    /* +0x170 */ ExplosivesFireCompTemplate* m_explosivesTemplate;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_EXPLOSIVESFIRECOMP_H
