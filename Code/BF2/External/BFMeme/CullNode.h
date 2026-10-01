// Descriptor-attested dice::meme::CullNode at PE8360A0; path [INFERRED]
// beside the attested BFMeme cluster. Only Node's existing33-slot prefix.
#ifndef BFMEME_CULLNODE_H
#define BFMEME_CULLNODE_H
#include "Node.h"
#if !DEDICATED
namespace dice { namespace meme {
class Data;
class CullNode : public Node
{
public:
    CullNode();                                           // inlined in836060
    ~CullNode() override;                                 // 835660; borrowed links/data
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    void visitChildren(System*, NodeState*, const Rect*, NodeVisitor*) override;
    void paint(System*, const NodeState*, const Rect*, const Rect*, RenderContext*) override;
    bool update(System*, const NodeState*, const Rect*, float) override;
    static TypeDescriptor* getClassType();                // 8360A0; metadata94E134
protected:
    // Member names [INFERRED]; property labels attest data and timing roles.
    Data* m_data;                                        // +08, borrowed
    float m_inTime;                                      // +0C
    float m_outTime;                                     // +10
    float m_fade;                                        // +14, not serialized
};
} }
#endif
#endif
