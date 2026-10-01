// Explosives-fire template configuration.

#ifndef GAME_OBJECTS_EXPLOSIVESFIRECOMPTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_EXPLOSIVESFIRECOMPTEMPLATE_H

#include <string>

#include "FireCompTemplate.h"

namespace dice {
namespace hfe {
namespace world {

class ExplosivesFireCompTemplate : public FireCompTemplate
{
public:
    ExplosivesFireCompTemplate();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void* queryImplementation(unsigned int);
    virtual ~ExplosivesFireCompTemplate();
    virtual bool  makeScript(io::IStream*);

    void setToggleWeaponTime(float value);
    void setTriggerChargesTime(float value);
    void setShowWeaponAfterReloadDelay(float value);

    // Retail's detonatorObject executor performs a direct std::string assignment; no separate
    // method symbol exists.  This typed seam preserves that recovered source operation.
    void setDetonatorObject(const std::string& value) { m_detonatorObject = value; }

    float getToggleWeaponTime() const { return m_toggleWeaponTime; }
    int getToggleWeaponTimeInTicks() const { return m_toggleWeaponTimeTimes30; }
    float getTriggerChargesTime() const { return m_triggerChargesTime; }
    int getTriggerChargesTimeInTicks() const { return m_triggerChargesTimeTimes30; }
    float getShowWeaponAfterReloadDelay() const { return m_showWeaponAfterReloadDelay; }
    int getShowWeaponAfterReloadDelayInTicks() const
    {
        return m_showWeaponAfterReloadDelayTimes30;
    }
    const std::string& getDetonatorObject() const { return m_detonatorObject; }

protected:
    // Documentary ELF offsets.  FireCompTemplate occupies 0..115 and this leaf is 0x94.  PE STL
    // layouts differ; source member order is the portable recovered contract.
    /* 116 */ int   m_refCount;

    /* 120 */ float m_toggleWeaponTime;
    /* 124 */ int   m_toggleWeaponTimeTimes30;             // cached

    /* 128 */ float m_triggerChargesTime;
    /* 132 */ int   m_triggerChargesTimeTimes30;           // cached

    /* 136 */ float m_showWeaponAfterReloadDelay;
    /* 140 */ int   m_showWeaponAfterReloadDelayTimes30;   // cached

    /* 144 */ std::string m_detonatorObject;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_EXPLOSIVESFIRECOMPTEMPLATE_H
