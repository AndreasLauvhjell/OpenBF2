// Shared persistence interface for objects and templates. [INFERRED] Header placement.

#ifndef CORE_IPERSISTANT_H
#define CORE_IPERSISTANT_H

#include "IBase.h"

namespace dice {
namespace hfe {

// dice's original spelling is "Persistant".  RTTI proves this is a zero-data, zero-slot layer
// between IBase and both IObjectTemplate and world::IObject.  In particular its destructor is not
// virtual: adding one here would move every destructor and method below it in both hierarchies.
class IPersistant : public IBase
{
protected:
    ~IPersistant() {}
};

} // namespace hfe
} // namespace dice

#endif // CORE_IPERSISTANT_H
