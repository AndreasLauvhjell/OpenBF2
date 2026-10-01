// [INFERRED] Source placement.

#include "AITemplatePlugIn.h"

namespace dice {
namespace hfe {
namespace ai {

// is the vptr store the compiler emits for any virtual destructor, and
// adds the tail jump to operator delete: nothing is left for source to have written.
AITemplatePlugIn::~AITemplatePlugIn()
{
}

// `xor eax, eax / ret`. No subclass overrides it (all six tables inherit it).
int AITemplatePlugIn::getTemplateKind()
{
    return 0;
}

} // namespace ai
} // namespace hfe
} // namespace dice
