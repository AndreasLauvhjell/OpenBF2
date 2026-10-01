// Descriptor-attested dice::meme::ActionNode at PE831210; path [INFERRED]
// beside the BFMeme cluster. Only Node's existing33-slot prefix is declared.
#ifndef BFMEME_ACTIONNODE_H
#define BFMEME_ACTIONNODE_H
#include "Node.h"
#if !DEDICATED
namespace dice { namespace meme {
class Action;
class ActionNode : public Node
{
public:
    ActionNode();
    ~ActionNode() override;                               // 835660, no owned links
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    bool updateSelf(System*, const NodeState*, const Rect*, float) override;
    bool dispatchEvent(System*, const Rect*, const NodeState*, const NodeInputEvent*) override;
    static TypeDescriptor* getClassType();                // 831210, metadata94BEB4
protected:
    Action* m_action;                                    // +08, borrowed; name [INFERRED]
};
} }
#endif
#endif
