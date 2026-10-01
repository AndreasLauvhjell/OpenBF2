// Split node using the known Node prefix; unsupported suffix calls retain diagnostic stops.
#ifndef BFMEME_SPLITNODE_H
#define BFMEME_SPLITNODE_H
#include "Node.h"
#if !DEDICATED
namespace dice { namespace meme {
class SplitNode : public Node
{
public:
    SplitNode();
    ~SplitNode() override;                                // 835660; does not own links
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    int getChildCount() override;
    Node*& getChild(int) override;
    void setChild(int, Node*) override;
    void visitChildren(System*, NodeState*, const Rect*, NodeVisitor*) override;
    void paint(System*, const NodeState*, const Rect*, const Rect*, RenderContext*) override;
    bool update(System*, const NodeState*, const Rect*, float) override;
    void reset() override;                               // 82D560; Node+C4 remains frontier
    static TypeDescriptor* getClassType();
protected:
    Node* m_child;                                       // retail+08; name [INFERRED]
};
} }
#endif
#endif
