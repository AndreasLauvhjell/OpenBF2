// Descriptor-attested dice::meme::ShowEffectNode, literal94AC50; path [INFERRED]
// beside attested BFMeme sources. Only the existing Node33 prefix is declared.
#ifndef BFMEME_SHOWEFFECTNODE_H
#define BFMEME_SHOWEFFECTNODE_H
#include "EffectNode.h"
#if !DEDICATED
namespace dice { namespace meme {
class ShowEffectNode : public EffectNode
{
public:
    explicit ShowEffectNode(Effect* effect = nullptr);     // 82CD80, factory82CDA0
    TypeDescriptor* getType() override;
    bool onCreate(System*) override;
    bool visit0x34(System*, Stream*) override;
    void paint(System*, const NodeState*, const Rect*, const Rect*, RenderContext*) override;
    static TypeDescriptor* getClassType();                // 82CFE0, metadata94A308
};
} }
#endif
#endif
