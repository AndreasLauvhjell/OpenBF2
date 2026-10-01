// Creates the default ammo component and adds reference counting to its shared template state.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_DEFAULTAMMOCOMPTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_DEFAULTAMMOCOMPTEMPLATE_H

#include "AmmoCompTemplate.h"

namespace dice {
namespace hfe {
namespace world {

class IObjectComponent;

class DefaultAmmoCompTemplate : public AmmoCompTemplate
{
public:
    DefaultAmmoCompTemplate();

    // IBase quartet. This leaf owns its reference count; AmmoCompTemplate is abstract and has none.
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    virtual unsigned int getClassID() const;
    virtual IObjectComponent* createComponent();
    virtual void* queryImplementation(unsigned int classId);
    virtual bool  makeScript(io::IStream* stream);

private:
    /* +0x48 */ int m_refCount;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_DEFAULTAMMOCOMPTEMPLATE_H
