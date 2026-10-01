// Variable scope for a bot action plan. [INFERRED] Source placement.
// Call clear to delete the owned variables before destruction; the destructor frees only list nodes.

#ifndef AI_AIDLL_AI_AIPLANINTERPRETER_BAPCONTEXT_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIPLANINTERPRETER_BAPCONTEXT_H

#include <list>

namespace dice {
namespace hfe {
namespace ai {

// Base for owned script variables; deletion uses the virtual destructor.
class BotScriptVariable
{
public:
    virtual ~BotScriptVariable();       // - the vptr store, nothing more
};

class BAPContext
{
public:
    BAPContext();
    ~BAPContext();                      // - , and it leaks; see above
    void clear();

private:
    /* 0 */ std::list<BotScriptVariable*> m_variables;   // the whole object
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIPLANINTERPRETER_BAPCONTEXT_H
