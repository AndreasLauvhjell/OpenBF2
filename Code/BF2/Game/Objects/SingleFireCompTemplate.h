// Owns the single-shot weapon time-event vector. [INFERRED] Header placement.
// [PLATFORM] Original GCC and Visual C++ vector layouts differ; do not share STL-containing layout assertions.

#ifndef GAME_OBJECTS_SINGLEFIRECOMPTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_SINGLEFIRECOMPTEMPLATE_H

#include <vector>

#include "FireCompTemplate.h"
#include "Weapons/FireComponents/FireComp.h"     // WeaponAction/WeaponTrigger/WEventStruct

namespace dice {
namespace hfe {
namespace world {

class SingleFireCompTemplate : public FireCompTemplate
{
public:
    SingleFireCompTemplate();

    // IBase quartet.  This leaf owns its reference count; FireCompTemplate is abstract and has no
    // count of its own.
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void* queryImplementation(unsigned int classId);
    virtual ~SingleFireCompTemplate();
    virtual bool  makeScript(io::IStream* stream);

    // .  `value` is authored in seconds; the event caches its truncated
    // 30 Hz tick delay exactly as the runtime consumes it.
    void addWeaponTimeEvent(WeaponTrigger trigger, WeaponAction action, float value);
    float getWeaponTimeEventValue(WeaponAction action, WeaponTrigger trigger);

    void setDropWeaponAfterFiringDelay(float value);
    void setHideWeaponAfterFiringDelay(float value);
    void setShowWeaponAfterReloadDelay(float value);
    void setHideWeaponAfterReloadDelay(float value);
    void setDropWeaponAfterReloadDelay(float value);

    float getDropWeaponAfterFiringDelay();
    float getHideWeaponAfterFiringDelay();
    float getShowWeaponAfterReloadDelay();
    float getHideWeaponAfterReloadDelay();
    float getDropWeaponAfterReloadDelay();

    // [INFERRED] Accessor names; console properties read and write these fields directly.
    void setBusyUntilButtonRelease(bool value) { m_busyUntilButtonRelease = value; }
    bool getBusyUntilButtonRelease() const { return m_busyUntilButtonRelease; }
    void setOnlyFireWhenProne(bool value) { m_onlyFireWhenProne = value; }
    bool getOnlyFireWhenProne() const { return m_onlyFireWhenProne; }
    void setHideLod(int value) { m_hideLod = value; }
    int getHideLod() const { return m_hideLod; }
    void setDropLod(int value) { m_dropLod = value; }
    int getDropLod() const { return m_dropLod; }

    const std::vector<WEventStruct>& getWeaponTimeEvents() const { return m_weaponTimeEvents; }

protected:
    // ELF documentary offsets: FireCompTemplate is 0x74 and this leaf is 0x90.  Do not assert
    // them on the PE/current STL; the source member order, rather than a foreign vector layout, is
    // the recovered contract.
    /* +0x74 */ int                       m_refCount;
    /* +0x78 */ std::vector<WEventStruct> m_weaponTimeEvents;
    /* +0x84 */ bool                      m_busyUntilButtonRelease;
    /* +0x85 */ bool                      m_onlyFireWhenProne;
                              // natural padding before the two ints in both source builds
    /* +0x88 */ int                       m_hideLod;
    /* +0x8C */ int                       m_dropLod;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_SINGLEFIRECOMPTEMPLATE_H
