// Configures supply, healing and repair abilities for a vehicle or kit. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_ABILITYGLCOMPTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_ABILITYGLCOMPTEMPLATE_H

#include <string>

#include "CompTemplateInterfaces.h"
#include "GameLogicComp/IAbilityGLComp.h"
#include "../../IO/Console/IConsoleSaveable.h"

namespace dice {
namespace hfe {
namespace world {

class AbilityGLComp;
class IObjectComponent;

// Ability configuration; preserve the interface and serialization vtable order.
class AbilityGLCompTemplate : public IAbilityGLCompTemplate, public io::IConsoleSaveable
{
public:
    AbilityGLCompTemplate();                     // see the .cpp

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int getClassID() const;
    virtual unsigned int getInterfaceID();
    virtual IObjectComponent* createComponent();
    virtual void  objectCreated(IObject*);
    virtual void  objectDestroyed(IObject*);
    virtual void  setTemplate(IObjectTemplate*);
    virtual void* queryImplementation(unsigned int);
    virtual void  setHasHealingAbility(bool);       virtual bool  getHasHealingAbility();
    virtual void  setHasRepairingAbility(bool);     virtual bool  getHasRepairingAbility();
    virtual void  setHasAmmoAbility(bool);          virtual bool  getHasAmmoAbility();
    virtual void  setRadarRadius(int);              virtual int   getRadarRadius();
    virtual ~AbilityGLCompTemplate();
    virtual bool  makeScript(io::IStream*);

    // getGLInfo (AbilityGLComp) hands out the address of the payload block below (+0x10).
    friend class AbilityGLComp;

protected:
    /*  8 */ int   m_refCount;

    /* 12 */ bool  m_needsBuild;                  // ctor true, createComponent test-and-clears -
                                                  // the Armor m_needsBuild slot
    /* 13 */ bool  _byteD;                        // [UNRESOLVED] - never written by any read body
    /* 14 */ bool  _byteE;
    /* 15 */ bool  _byteF;

    /* 16 */ AbilityGLInfo m_info;
    /* 24 */ std::string _string18;               // [UNRESOLVED] meaning - ctor default-constructs,
    /* 28 */ std::string _string1C;               // dtor drops; no read accessor in the vtable
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_ABILITYGLCOMPTEMPLATE_H
