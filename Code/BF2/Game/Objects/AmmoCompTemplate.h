// Magazine and reload settings. Durations retain separate seconds and simulation-tick values.
// [INFERRED] source placement.

#ifndef GAME_OBJECTS_AMMOCOMPTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_AMMOCOMPTEMPLATE_H

#include <string>

#include "CompTemplateInterfaces.h"
#include "../../IO/Console/IConsoleSaveable.h"

namespace dice {
namespace hfe {
namespace world {

class IObjectComponent;

// Abstract ammo template; concrete subclasses implement reference counting and interface lookup.
// Keep makeScript after createComponent; this base has no virtual destructor.
class AmmoCompTemplate : public IWeaponCompTemplate, public io::IConsoleSaveable
{
public:
    AmmoCompTemplate();

    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent();      // real body in the ELF, not pure
    virtual void  objectCreated(IObject*);
    virtual void  objectDestroyed(IObject*);
    virtual void  setTemplate(IObjectTemplate*);
    virtual void* queryImplementation(unsigned int);
    virtual bool  makeScript(io::IStream*);           // +0x30

    // Typed field access for the runtime ammo component; these add no virtual slots.
    int   getAmmoType() const                  { return m_ammoType; }
    void  setAmmoType(int value)               { m_ammoType = value; }
    float getReloadTime() const                { return m_reloadTime; }
    int   getReloadTimeTimes30() const         { return m_reloadTimeTimes30; }
    float getFirstShotExtraTime() const        { return m_firstShotExtraTime; }
    int   getExtraReloadTimeFirstTimes30() const { return m_extraReloadTimeFirstTimes30; }
    float getLastShotExtraTime() const         { return m_lastShotExtraTime; }
    int   getExtraReloadTimeLastTimes30() const  { return m_lastShotExtraTimeTimes30; }
    float getChangeMagAt() const               { return m_changeMagAt; }
    bool  getAutoReload() const                { return m_autoReload; }
    void  setAutoReload(bool value)            { m_autoReload = value; }
    bool  getInstantReloadOnEnable() const     { return m_instantReloadOnEnable; }
    void  setInstantReloadOnEnable(bool value) { m_instantReloadOnEnable = value; }
    float getMinimumTimeUntilReload() const    { return m_minimumTimeUntilReload; }
    int   getMinimumTimeToReloadTimes30() const { return m_minimumTimeToReloadTimes30; }
    int   getNrOfMags() const                  { return m_nrOfMags; }
    void  setNrOfMags(int value)               { m_nrOfMags = value; }
    int   getMagSize() const                   { return m_magSize; }
    void  setMagSize(int value)                { m_magSize = value; }
    int   getReloadAmount() const              { return m_reloadAmount; }
    void  setReloadAmount(int value)           { m_reloadAmount = value; }
    bool  getToggleWhenNoAmmo() const          { return m_toggleWhenNoAmmo; }
    void  setToggleWhenNoAmmo(bool value)      { m_toggleWhenNoAmmo = value; }
    bool  getReloadWithoutPlayer() const       { return m_reloadWithoutPlayer; }
    void  setReloadWithoutPlayer(bool value)   { m_reloadWithoutPlayer = value; }
    const std::string& getMagLinkWeapon() const { return m_magLinkWeapon; }
    void  setMagLinkWeapon(const std::string& value) { m_magLinkWeapon = value; }

    // The four console setters that recompute a cached tick count beside their seconds field.
    void setChangeMagAt(float value);
    void setReloadTime(float seconds);
    void setExtraReloadTimeFirst(float seconds);
    void setExtraReloadTimeLast(float seconds);
    void setMinimumTimeToReload(float seconds);

protected:
    /*  8 */ int   m_ammoType;
    /* 12 */ float m_reloadTime;
    /* 16 */ int   m_reloadTimeTimes30;               // cached: m_reloadTime * 30
    /* 20 */ float m_firstShotExtraTime;
    /* 24 */ int   m_extraReloadTimeFirstTimes30;     // cached
    /* 28 */ float m_lastShotExtraTime;
    /* 32 */ int   m_lastShotExtraTimeTimes30;        // cached
    /* 36 */ float m_changeMagAt;

    /* 40 */ bool  m_autoReload;
    /* 41 */ bool  m_instantReloadOnEnable;
    /* 42 */ bool  _byte2A;                           // [UNRESOLVED]
    /* 43 */ bool  _byte2B;                           // [UNRESOLVED]

    /* 44 */ float m_minimumTimeUntilReload;
    /* 48 */ int   m_minimumTimeToReloadTimes30;      // cached

    /* 52 */ int   m_nrOfMags;
    /* 56 */ int   m_magSize;
    /* 60 */ int   m_reloadAmount;

    /* 64 */ bool  m_toggleWhenNoAmmo;
    /* 65 */ bool  m_reloadWithoutPlayer;
    /* 66 */ bool  _byte42;                           // [UNRESOLVED]
    /* 67 */ bool  _byte43;                           // [UNRESOLVED]

    /* 68 */ std::string m_magLinkWeapon;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_AMMOCOMPTEMPLATE_H
