// Authored component state and factory for the corresponding runtime component.
// objectCreated attaches a created component and releases the temporary reference; null means no runtime component.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_IOBJECTTEMPLATECOMPONENT_H     // [INFERRED] guard style
#define GAME_OBJECTS_IOBJECTTEMPLATECOMPONENT_H

#include "../../Core/IBase.h"
#include "IObject.h"

namespace dice {
namespace hfe {
namespace world {

class ObjectTemplate;
class IObjectTemplate;

// The shared interface ends at queryImplementation. preCache belongs to IWeaponCompTemplate,
// and makeScript belongs to IConsoleSaveable; neither may be inserted here.
class IObjectTemplateComponent /* : public IPersistant */ : public IBase
{
public:
    virtual unsigned int getClassID() const                = 0;   // +0x10
    virtual unsigned int getInterfaceID()                  = 0;   // +0x14 (was reserved14)

    // Builds the runtime counterpart of this template component. May return NULL, in which case
    // nothing is attached to the object. The result is release()d by the caller after attachment,
    // so the object takes the reference.
    virtual IObjectComponent* createComponent()            = 0;   // +0x18

    // Callbacks receive interface pointers so implementations share the same ABI.
    virtual void         objectCreated(IObject*)           = 0;   // +0x1C
    virtual void         objectDestroyed(IObject*)         = 0;   // +0x20

    // Back-pointer to the owning template, set by ObjectTemplate::setComponent at attach time.
    virtual void         setTemplate(IObjectTemplate*)     = 0;   // +0x24

    // Returns this implementation or an ancestor for a matching class id, otherwise null.
    // Class ids differ from the interface ids accepted by queryInterface.
    // [UNRESOLVED] Original pointer return type; void* preserves its current representation.
    virtual void*        queryImplementation(unsigned int) = 0;   // +0x28
};

// The IID this interface is registered under. ObjectTemplate::setComponent queries it on whatever
// it is handed, so anything can be attached as long as it answers this.
extern const unsigned int IID_IObjectTemplateComponent;

// The runtime-side counterpart, queried by objectDestroyed. Declared here because that is the only
// place it has been observed so far. [UNRESOLVED] its own interface.
extern const unsigned int IID_IObjectComponent;

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_IOBJECTTEMPLATECOMPONENT_H
