// Action that invokes a named menu function.
#ifndef MENU_BF2CALLFUNCTIONACTION_H
#define MENU_BF2CALLFUNCTIONACTION_H
#include "../External/BFMeme/CallFunctionAction.h"
#if !DEDICATED
namespace dice { namespace meme {
class Bf2CallFunctionAction : public CallFunctionAction
{
public:
    Bf2CallFunctionAction();
    TypeDescriptor* getType() override;
    // +44, 7CFFA0. Stores the pointer and answers a hard true - there is no
    // null test and no diagnostic, so a null Function is accepted here and only
    // refused later, by CallFunctionAction::execute.
    virtual bool setFunction(Function* function);
    static TypeDescriptor* getClassType();               // metadata938A60
};
} }
#endif
#endif
