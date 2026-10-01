// Configures the five radio callouts an object can trigger. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_RADIOGLCOMPTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_RADIOGLCOMPTEMPLATE_H

#include <string>

#include "CompTemplateInterfaces.h"
#include "../../IO/Console/IConsoleSaveable.h"

namespace dice {
namespace hfe {
namespace world {

class IObjectComponent;
class RadioGLComp;

// Radio message settings; string getters and setters both pass by value.
// Preserve virtual declaration order.
class RadioGLCompTemplate : public IRadioGLCompTemplate, public io::IConsoleSaveable
{
public:
    RadioGLCompTemplate();                       // see the .cpp

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
    virtual void setFailMessage(std::string);      virtual std::string getFailMessage();       // +0x2C/+0x30
    virtual void setUseMessage(std::string);       virtual std::string getUseMessage();        // +0x34/+0x38
    virtual void setDestroyedMessage(std::string); virtual std::string getDestroyedMessage();  // +0x3C/+0x40
    virtual void setRepairedMessage(std::string);  virtual std::string getRepairedMessage();   // +0x44/+0x48
    virtual void setSpottedMessage(std::string);   virtual std::string getSpottedMessage();    // +0x4C/+0x50
    virtual ~RadioGLCompTemplate();
    virtual bool makeScript(io::IStream*);

    // getGLInfo (RadioGLComp) hands out the address of the payload block below (+0x10).
    friend class RadioGLComp;

protected:
    /*  8 */ int   m_refCount;

    /* 12 */ bool  m_needsBuild;                  // ctor true, createComponent test-and-clears -
                                                  // the Armor m_needsBuild slot
    /* 13 */ bool  _byteD;                        // [UNRESOLVED] - never written by any read body
    /* 14 */ bool  _byteE;
    /* 15 */ bool  _byteF;

    /* 16 */ std::string m_failMessage;
    /* 20 */ std::string m_useMessage;
    /* 24 */ std::string m_destroyedMessage;
    /* 28 */ std::string m_repairedMessage;
    /* 32 */ std::string m_spottedMessage;

    /* 36 */ std::string _string24;               // [UNRESOLVED] meaning - ctor default-constructs,
    /* 40 */ std::string _string28;               // dtor drops; no read accessor in the vtable
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_RADIOGLCOMPTEMPLATE_H
