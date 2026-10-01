// Path:     [INFERRED] beside the attested FireComp.h; this leaf filename is not in either
// canonical shipped source-path map.
// Contents: complete runtime surface and source member order recovered from both retail images.

#ifndef GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_THROWNFIRECOMP_H
#define GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_THROWNFIRECOMP_H

#include "FireComp.h"

namespace dice {
namespace hfe {
namespace world {

class ThrownFireCompTemplate;

class ThrownFireComp : public FireComp
{
public:
    explicit ThrownFireComp(ThrownFireCompTemplate* objectTemplate);
    virtual ~ThrownFireComp();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual bool preUpdate();
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual void reload(ReloadState state);
    virtual bool requestFire(bool first, bool second);
    virtual float getAcceptDelay(bool value) const;
    virtual bool launchesProjectiles(int index) const;
    virtual void outOfAmmo(bool value);
    virtual int fire(bool value, int index);
    virtual void exit();
    virtual void enable();
    virtual void disable();
    virtual void fireStop(bool value);
    virtual void fireComplete(unsigned int tick);
    virtual void reset();
    virtual const Mat4& getProjectileOrigin() const;
    virtual bool busy();

private:
    void fireUpdateStrength();
    bool setThrowState(FireState state, bool simulate);
    bool launchProjectile();

    // ELF documentary offsets (FireComp is 0x128; ThrownFireComp is 0x18C).  Do not assert these
    // across STL implementations; the member order is shared by the PE source build.
    /* +0x128 */ int          m_refCount;
    /* +0x12C */ float        m_fireStrength;
    /* +0x130 */ unsigned int m_pullbackStartedAtTick;
    /* +0x134 */ unsigned int m_launchDelayStartedAtTick;
    /* +0x138 */ bool         m_requestingFire;
    /* +0x139 */ bool         m_reloading;
    /* +0x13C */ mutable Mat4 m_thrownProjectileOrigin;
    /* +0x17C */ unsigned int m_fireCompleteAtTick;
    /* +0x180 */ bool         m_pendingFireStop;
    /* +0x184 */ ThrownFireCompTemplate* m_thrownTemplate;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_THROWNFIRECOMP_H
