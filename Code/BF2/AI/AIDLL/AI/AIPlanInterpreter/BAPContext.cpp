// [INFERRED] Source placement.

#include "BAPContext.h"

namespace dice {
namespace hfe {
namespace ai {

// one instruction in the binary - the vptr store C++ emits. The class owns nothing and declares
// nothing beyond this; see the note in BAPContext.h.
BotScriptVariable::~BotScriptVariable()
{
}

// `allocate(12)` self-linked is an empty `std::list` with a 4-byte element - a member declaration
// not a statement. The whole constructor is the list coming into existence.
BAPContext::BAPContext()
{
}

// Destroys list nodes without deleting their BotScriptVariable pointers. Call clear
// before destruction to release variables; the empty destructor preserves that requirement.
BAPContext::~BAPContext()
{
}

// Deletes all owned script variables and empties the context.
void BAPContext::clear()
{
    for (std::list<BotScriptVariable*>::iterator it = m_variables.begin();
         it != m_variables.end(); ++it)
    {
        if (*it)
            delete *it;
    }

    m_variables.clear();
}

} // namespace ai
} // namespace hfe
} // namespace dice
