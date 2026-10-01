// Transform node using the known Node prefix; unknown virtual suffix declarations remain absent.
#ifndef BFMEME_TRANSFORMNODE_H
#define BFMEME_TRANSFORMNODE_H
#include "Node.h"
#include "../../Math/Vec3.h"
#if !DEDICATED
namespace dice { namespace meme {
class MovePointer;
class TransformNode : public Node {
public:
    TransformNode();
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    const char* getName() override;                        // 80D430, real null default
    bool streamProperties(System*, Stream*) override;
    int getChildCount() override;
    Node*& getChild(int) override;
    void setChild(int, Node*) override;
    void visitChildren(System*, NodeState*, const Rect*, NodeVisitor*) override;
    void paint(System*, const NodeState*, const Rect*, const Rect*, RenderContext*) override;
    bool update(System*, const NodeState*, const Rect*, float) override;
    void reset() override;                               // 82D560; base+C4 remains frontier
    NodeState* getState() override { return &m_state; }    // 54B140, complete LEA EAX,[ECX+1C];RET
    static TypeDescriptor* getClassType();
    hfe::Vec2& getPosition() { return m_position; }        // 65DDD0, complete LEA EAX,[ECX+C];RET
    hfe::Vec2& getSize() { return m_size; }                // 659990, complete LEA EAX,[ECX+14];RET
protected:
    friend class MovePointer;
    Node* m_child;                                       // +08
    hfe::Vec2 m_position;                                // +0C,0
    hfe::Vec2 m_size;                                    // +14,20
    NodeState m_state;                                   // +1C,{1,1}
};
} }
#endif
#endif
