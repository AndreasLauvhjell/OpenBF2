// Effect node using the shared Node interface and its existing suffix frontiers.
#ifndef BFMEME_EFFECTNODE_H
#define BFMEME_EFFECTNODE_H
#include "Node.h"
#if !DEDICATED
namespace dice { namespace meme {
class EffectNode : public Node
{
public:
    explicit EffectNode(Effect* effect = nullptr);          // 82C220, factory82CB10
    ~EffectNode() override;
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    void paint(System*, const NodeState*, const Rect* bounds,
               const Rect* clip, RenderContext*) override;
    bool updateSelf(System*, const NodeState*, const Rect*, float delta) override;
    static TypeDescriptor* getClassType();                 // metadata94A2C8
protected:
    Effect* m_effect; // +08, borrowed; name [INFERRED] from property and callers
};
} }
#endif
#endif
