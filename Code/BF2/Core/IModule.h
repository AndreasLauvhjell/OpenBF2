// Module registration and initialization interface. Header path [INFERRED].
// Eight virtual slots: IBase, class list, singleton list, init and close. Lifetime uses release;
// there is no virtual destructor. AIInterfaceModule's reset is a concrete extension, not an IModule slot.

#ifndef CORE_IMODULE_H          // [INFERRED] guard style
#define CORE_IMODULE_H

#include <vector>

#include "ClassManager.h"       // IClassServer - the base, and ClassRegInfo / SingletonRegInfo

namespace dice {
namespace hfe {

// Extends class registration with module initialization and shutdown.
class IModule : public IClassServer
{
public:

    virtual bool  init() = 0;                                       // +0x18
    // Closes the module and returns its completion status.
    virtual bool  close() = 0;                                      // +0x1C  <- the last slot

protected:
    // Not deleted through this interface. Non-virtual and protected so the vtable stays exactly as
    // recovered while `delete module` is still rejected.
    ~IModule() {}
};

} // namespace hfe
} // namespace dice

#endif // CORE_IMODULE_H
