// Serializes an object as console script to the supplied stream. Header path [INFERRED].
// makeScript returns whether the complete script was written; callers may stop traversal on false.
// The interface appends that non-const method to IBase's four slots and adds no virtual destructor.
// It is commonly a secondary base, so callers must use the adjusted interface pointer.

#ifndef IO_CONSOLE_ICONSOLESAVEABLE_H     // [INFERRED] guard style
#define IO_CONSOLE_ICONSOLESAVEABLE_H

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace io {

class IStream;

class IConsoleSaveable : public IBase
{
public:
    // Emit this object as console script (.con text) through the stream. The one method this
    // interface adds; slot 5 of its vtable, directly after the inherited quartet.
    // Returns whether the script was written in full; a false result stops a containing traversal.
    virtual bool makeScript(IStream*) = 0;
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_CONSOLE_ICONSOLESAVEABLE_H
