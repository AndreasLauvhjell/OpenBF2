// [INFERRED] Menu placement; descriptor literal9397E0 names BfTransformNode.
// This is the80-byte class at primary939800, distinct from Bf2TransformNode.
#ifndef MENU_BFTRANSFORMNODE_H
#define MENU_BFTRANSFORMNODE_H
#include "../External/BFMeme/TransformNode.h"
#if !DEDICATED
namespace dice { namespace meme {
class Data;
class BfTransformNode : public TransformNode
{
public:
    BfTransformNode(); // initialization in factory7D34E0
    TypeDescriptor* getType() override;
    unsigned int getObjectSize() override;
    bool streamProperties(System*, Stream*) override;
    Node*& getChild(int) override;
    void setChild(int, Node*) override;
    void visitChildren(System*, NodeState*, const Rect*, NodeVisitor*) override;
    void paint(System*, const NodeState*, const Rect*, const Rect*, RenderContext*) override;
    bool update(System*, const NodeState*, const Rect*, float) override;
    NodeState* getState() override;
    bool updateSelf(System*, const NodeState*, const Rect*, float) override;
    static TypeDescriptor* getClassType();

protected:
    // Retail duplicates these fields after the entire36-byte TransformNode.
    // All links are borrowed. Inherited reset still addresses the base's child.
    Node* m_transformedChild; // +24
    hfe::Vec2 m_transformedPosition; // +28, +2C
    hfe::Vec2 m_transformedSize; // +30, +34
    NodeState m_transformedState; // +38, +3C
    Data* m_xData; // +40
    Data* m_yData; // +44
    Data* m_widthData; // +48; sampled but not streamed by7D3050
    Data* m_heightData; // +4C; sampled but not streamed by7D3050

private:
    void sampleData(System*); // [INFERRED] shared spelling for the two inline sequences
};
} }
#endif
#endif
