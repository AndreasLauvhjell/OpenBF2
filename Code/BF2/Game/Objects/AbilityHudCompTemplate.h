// Ability sound settings paired with AbilityGLCompTemplate flags.
// [INFERRED] source placement; string layout depends on the compiler.

#ifndef GAME_OBJECTS_ABILITYHUDCOMPTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_ABILITYHUDCOMPTEMPLATE_H

#include <string>

#include "CompTemplateInterfaces.h"
#include "../../IO/Console/IConsoleSaveable.h"

namespace dice {
namespace hfe {
namespace world {

class IObjectComponent;
class AbilityHudComp;

// Ability sounds with by-value string getters and setters.
// Preserve virtual declaration order across both base interfaces.
class AbilityHudCompTemplate : public IAbilityHudCompTemplate, public io::IConsoleSaveable
{
public:
    AbilityHudCompTemplate();

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
    virtual void setHealingSound(std::string);      virtual std::string getHealingSound();
    virtual void setRepairingSound(std::string);    virtual std::string getRepairingSound();
    virtual void setAmmoSound(std::string);         virtual std::string getAmmoSound();
    virtual ~AbilityHudCompTemplate();
    virtual bool makeScript(io::IStream*);                           // +0x4C last

protected:
    friend class AbilityHudComp;

    /*  8 */ int   m_refCount;

    /* 12 */ bool  m_needsBuild;                   // cleared by createComponent
    /* 13 */ char  _padD[3];                       // not initialized by either shipped ctor

    /* 16 */ std::string m_healingSound;
    /* 20 */ std::string m_repairingSound;
    /* 24 */ std::string m_ammoSound;

    // Both are owned std::strings, not dwords. ELF ctor 0x8225230 constructs all five strings and
    // dtor 0x8225410 destroys all five; PE ctor 0x65CE10 and dtor 0x65CC90 independently prove
    // the corresponding +0x64/+0x80 fields. No non-lifecycle use has yet named either string.
    /* 28 */ std::string m_unknownString1C;        // [UNRESOLVED] name/purpose
    /* 32 */ std::string m_unknownString20;        // [UNRESOLVED] name/purpose
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_ABILITYHUDCOMPTEMPLATE_H
