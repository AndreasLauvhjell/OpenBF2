// Single-shot firing component.

#ifndef GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_SINGLEFIRECOMP_H
#define GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_SINGLEFIRECOMP_H

#include "FireComp.h"

namespace dice {
namespace hfe {
namespace world {

class SingleFireCompTemplate;

class SingleFireComp : public FireComp
{
    friend class FireComp;

public:
    explicit SingleFireComp(SingleFireCompTemplate* objectTemplate);
    virtual ~SingleFireComp();

    // This concrete leaf owns a second reference counter.  The exact quartet is present in both
    // its primary and adjusted IWeaponFireComp tables.
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    virtual bool preUpdate();
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual bool requestFire(bool first, bool second);
    virtual bool launchesProjectiles(int index) const;
    virtual void outOfAmmo(bool value);
    virtual void enable();
    virtual int fire(bool value, int index);
    virtual void fireComplete(unsigned int tick);
    virtual void postReload();
    virtual void reload(ReloadState state);
    virtual void fireStop(bool value);
    virtual bool busy();
    virtual bool isFireLaunchDelay() const;

private:
    void handleTimeEventAction(const WEventStruct& event);
    void enableTimeEvents(WeaponTrigger trigger);
    void disableTimeEvents();
    void fireCompleteUpdateStates();
    bool launchProjectile(int index);

    // ELF layout (FireComp occupies +0x000..+0x127; sizeof SingleFireComp is 0x150).  These offsets
    // are documentary only because FireComp contains STL objects whose PE representation differs.
    /* +0x128 */ int          m_refCount;
    /* +0x12C */ unsigned int m_fireCompleteAtTick;

    // Intentionally not initialized by the shipped constructor.  It is guarded by
    // m_waitingForFireLaunchDelay and written before that guard becomes true.
    /* +0x130 */ unsigned int m_fireLaunchDelayStartedAtTick;

    /* +0x134 */ bool         m_waitingForFireLaunchDelay;
    /* +0x135 */ bool         m_enabledTimeEventActions[3];
    /* +0x138 */ bool         m_requestingFire;
    /* +0x139 */ bool         m_dropWeaponFxPending;
    /* +0x13C */ Vec3         m_dropWeaponFxPosition;
    /* +0x148 */ unsigned int m_dropWeaponFxAtTick;
    /* +0x14C */ SingleFireCompTemplate* m_singleFireTemplate;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_SINGLEFIRECOMP_H
