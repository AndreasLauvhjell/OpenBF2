// HUD action list with an additional typed action slot. [INFERRED] Header placement.
#ifndef MENU_BF2ACTIONLISTACTION_H
#define MENU_BF2ACTIONLISTACTION_H
#include "../External/BFMeme/ActionListAction.h"
#if !DEDICATED
namespace dice { namespace meme {
class Bf2ActionListAction : public ActionListAction
{
public:
    Bf2ActionListAction();
    TypeDescriptor* getType() override;
    // +40, 7CB810. Unlike ActionListAction::execute (834CA0), which returns on
    // the first action that answers true, this one runs every action and reports
    // only the last one's result.
    bool execute(System*, const Rect*, const NodeState*, const NodeInputEvent*) override;
    // +44, 7CB7D0. Appends after the last action, or at the first position when
    // the list is empty. first() is called unconditionally and its result is
    // discarded whenever the list is not empty.
    virtual bool addAction(System* system, Action* action);
    static TypeDescriptor* getClassType();               // metadata937C84
};
} }
#endif
#endif
