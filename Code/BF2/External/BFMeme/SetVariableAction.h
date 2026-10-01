// Descriptor-attested dice::meme::SetVariableAction, getter832F50/literal94CEA4.
// Path [INFERRED] beside BFMeme; complete inherited17-slot Action interface.
#ifndef BFMEME_SETVARIABLEACTION_H
#define BFMEME_SETVARIABLEACTION_H
#include "Action.h"
#if !DEDICATED
namespace dice { namespace meme {
class Data;
class SetVariableAction : public Action
{
public:
    SetVariableAction();                                // factory833170 inlines construction
    ~SetVariableAction() override;                       // 835660, borrowed Data links
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    bool execute(System*, const Rect*, const NodeState*, const NodeInputEvent*) override;
    static TypeDescriptor* getClassType();               // descriptor94CCCC
protected:
    Data* m_variable;                                   // +04 destination, borrowed
    Data* m_data;                                       // +08 source, borrowed
};
} }
#endif
#endif
