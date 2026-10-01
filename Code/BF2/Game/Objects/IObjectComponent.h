// Base interface for object components, with inert default updates and messages.
// Derived components override only the work they provide; preserve virtual order.
// No virtual destructor: release through IBase. PythonComp uses queryInterface
// rather than overriding getInterfaceID.
// [UNRESOLVED] The update-channel scheduling and trailing message flag still
// need caller-side confirmation; do not assume both update hooks run on servers.

#ifndef GAME_OBJECTS_IOBJECTCOMPONENT_H     // [INFERRED] guard style
#define GAME_OBJECTS_IOBJECTCOMPONENT_H

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace world {

class IObject;
class IPlayer;

// [UNRESOLVED] enumerators. Named from handleMessage's mangled parameter type.
enum TemplateMessage { /* [UNRESOLVED] */ };

// No data members of its own - IBase's vptr only.
class IObjectComponent : public IBase
{
public:
    virtual unsigned int getInterfaceID() const;   // +0x10 returns 0xC37B in the base
    virtual void init(IObject*);                   // +0x14 empty default
    virtual void destroy();                        // +0x18 empty default

    virtual void setObject(IObject*) = 0;          // +0x1C pure
    virtual IObject* getObject() const = 0;        // +0x20 pure

    virtual bool hasUpdate() const;                // +0x24 false
    virtual void handleUpdate(float);              // +0x28 empty
    virtual bool hasFrameUpdate() const;           // +0x2C false
    virtual void handleFrameUpdate(float);         // +0x30 empty

    virtual void handleMessage(TemplateMessage, IPlayer*, bool);   // +0x34 empty
};

// Interface IDs are declared centrally in Core/Ids.h.

} // namespace world
} // namespace hfe
} // namespace dice

#endif
