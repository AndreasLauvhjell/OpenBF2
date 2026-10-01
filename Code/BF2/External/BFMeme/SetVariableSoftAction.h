// Descriptor-attested class at833050/literal94CF70; path [INFERRED].
// Complete17-slot Action family, metadata94CD4C proves SetVariableAction parent.
#ifndef BFMEME_SETVARIABLESOFTACTION_H
#define BFMEME_SETVARIABLESOFTACTION_H
#include "SetVariableAction.h"
#if !DEDICATED
namespace dice { namespace meme {
class SetVariableSoftAction : public SetVariableAction
{
public:
    SetVariableSoftAction();                            // factory833190 inlines construction
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    bool execute(System*, const Rect*, const NodeState*, const NodeInputEvent*) override;
    static TypeDescriptor* getClassType();
protected:
    float m_speed;                                      // +0C, Speed; initially100
};
} }
#endif
#endif
