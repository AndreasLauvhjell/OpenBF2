// Descriptor-attested dice::meme::ActionListAction at834D30/literal94DB18.
// Path [INFERRED]; complete17-slot Action, embedded concrete ObjectList.
#ifndef BFMEME_ACTIONLISTACTION_H
#define BFMEME_ACTIONLISTACTION_H
#include "Action.h"
#include "ObjectList.h"
#if !DEDICATED
namespace dice { namespace meme {
class ActionListAction : public Action
{
public:
    ActionListAction();                                  // factory834DC0 inlines construction
    ~ActionListAction() override;                        // 834D90, links then Object
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    bool execute(System*, const Rect*, const NodeState*, const NodeInputEvent*) override;
    static TypeDescriptor* getClassType();               // metadata94DAF8
protected:
    ObjectList m_actions;                                // +04, link storage owned
};
} }
#endif
#endif
