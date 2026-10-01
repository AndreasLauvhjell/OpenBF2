// Descriptor-attested dice::meme::CullVariableActionNode at PE831350;
// path [INFERRED] beside the BFMeme cluster. Parent descriptor is ActionNode.
#ifndef BFMEME_CULLVARIABLEACTIONNODE_H
#define BFMEME_CULLVARIABLEACTIONNODE_H
#include "ActionNode.h"
#if !DEDICATED
namespace dice { namespace meme {
class Data;
class CullVariableActionNode : public ActionNode
{
public:
    CullVariableActionNode();                             // inlined in831190
    ~CullVariableActionNode() override;                   // 835660, borrowed references
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    bool dispatchEvent(System*, const Rect*, const NodeState*, const NodeInputEvent*) override;
    static TypeDescriptor* getClassType();                // 831350, metadata94BF54
protected:
    Data* m_variable;                                    // +0C, borrowed; name [INFERRED]
};
} }
#endif
#endif
