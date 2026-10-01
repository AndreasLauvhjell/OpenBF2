// Action that invokes a borrowed Function. Its destructor does not release that pointer.
#ifndef BFMEME_CALLFUNCTIONACTION_H
#define BFMEME_CALLFUNCTIONACTION_H
#include "Action.h"
#if !DEDICATED
namespace dice { namespace meme {
class Function;
class CallFunctionAction : public Action
{
public:
    CallFunctionAction();
    ~CallFunctionAction() override;                      // 835660, the shared empty body
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    bool execute(System*, const Rect*, const NodeState*, const NodeInputEvent*) override;
    static TypeDescriptor* getClassType();               // metadata94D954
protected:
    Function* m_function;                                // +04, borrowed
};
} }
#endif
#endif
