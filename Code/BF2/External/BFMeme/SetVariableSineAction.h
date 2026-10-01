// Descriptor-attested class at833090/literal94CFDC; path [INFERRED].
// Metadata94CD6C proves SetVariableSoftAction parent; complete17-slot family.
#ifndef BFMEME_SETVARIABLESINEACTION_H
#define BFMEME_SETVARIABLESINEACTION_H
#include "SetVariableSoftAction.h"
#if !DEDICATED
namespace dice { namespace meme {
class SetVariableSineAction : public SetVariableSoftAction
{
public:
    SetVariableSineAction();                            // ctor8330D0/factory8331C0
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    bool execute(System*, const Rect*, const NodeState*, const NodeInputEvent*) override;
    static TypeDescriptor* getClassType();
protected:
    float m_brakingDistance;                            // +10, initially0
};
} }
#endif
#endif
