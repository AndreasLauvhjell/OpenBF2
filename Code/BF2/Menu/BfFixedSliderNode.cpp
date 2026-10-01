// Fixed-slider node. Per-frame and event handlers remain named frontiers.
#include "BfFixedSliderNode.h"
#if !DEDICATED
#include "../External/BFMeme/TypeDescriptor.h"
#include "../External/BFMeme/Style.h"   // dice::meme::Rect is defined here
#include <cstdio>
#include <cstdlib>
namespace dice { namespace meme {

// SliderNode() first - the base's own nine words are constructed and
// then never read again - followed by eleven own words. The vptr store sits
// between the zeroed group and the 1.0 group exactly as in the base.
BfFixedSliderNode::BfFixedSliderNode()
    : SliderNode(), m_grabOffset(0.0f), m_thumbOffset(0.0f), m_minimum(0.0f),
      m_maximum(1.0f), m_thumbSpan(1.0f), m_step(1.0f), m_data(nullptr),
      m_child1(nullptr), m_field054(0)
{
    m_state.target = 1.0f;   // +4C
    m_state.current = 1.0f;  // +50
}

TypeDescriptor* BfFixedSliderNode::getClassType()         // 7D4850, singletonA23D84
{
    static ClassTypeDescriptor<BfFixedSliderNode, SliderNode>
        type("dice::meme::BfFixedSliderNode");
    return &type;
}
TypeDescriptor* BfFixedSliderNode::getType() { return getClassType(); }
unsigned int BfFixedSliderNode::getObjectSize() { return sizeof(BfFixedSliderNode); }

// Byte-for-byte the base's bodies against +48 instead of +20,
// including the same asymmetry: the getter sends every index but 1 to Node's
// +04 slot, while the setter accepts only 0 and 1 and silently drops the rest.
Node*& BfFixedSliderNode::getChild(int index)
{
    if (index == 1) return m_child1;
    return m_next;
}

void BfFixedSliderNode::setChild(int index, Node* child)
{
    if (index == 0) { m_next = child; return; }
    if (index == 1) m_child1 = child;
}

// The base's body at this class's own offsets; see SliderNode.cpp for
// the ordered-difference reading of the single `fcomp`.
void BfFixedSliderNode::visitChildren(System* system, NodeState* state, const Rect* bounds,
                                      NodeVisitor* visitor)
{
    Rect thumb;
    thumb.x = bounds->x + m_thumbOffset;
    thumb.y = bounds->y;
    thumb.width = bounds->width;
    thumb.height = bounds->height;
    thumb.z = bounds->z;
    const float span = m_maximum < m_minimum ? m_minimum - m_maximum : m_maximum - m_minimum;
    thumb.width = m_thumbSpan / span * thumb.width;
    visitor->traverse0x08(system, m_child1, &thumb, &m_state);
    visitor->visit(system, m_next, bounds, state);
}

// [FRONTIER] [TEMP] [CHOSEN, not recovered] Fixed-slider frame and input operations stop at their missing implementations.
// [RETIRE-WHEN: slider-node-bodies] Implement and validate the six remaining frame and input operations.
namespace {
void fixedSliderFrontier(const char* method, const char* site)
{
    std::fprintf(stderr, "[FRONTIER] dice::meme::BfFixedSliderNode::%s reached (BF2.exe %s)\n",
                 method, site);
    std::fflush(stderr);
    std::abort();
}
}

bool BfFixedSliderNode::streamProperties(System*, Stream*)
{ fixedSliderFrontier("streamProperties", "7D3D50, 171 B"); return false; }

void BfFixedSliderNode::paint(System*, const NodeState*, const Rect*, const Rect*, RenderContext*)
{ fixedSliderFrontier("paint", "7D4920, 269 B"); }

bool BfFixedSliderNode::update(System*, const NodeState*, const Rect*, float)
{ fixedSliderFrontier("update", "7D4A30, 325 B"); return false; }

bool BfFixedSliderNode::updateSelf(System*, const NodeState*, const Rect*, float)
{ fixedSliderFrontier("updateSelf", "7D4250, 465 B, the drag and the fixed step"); return false; }

bool BfFixedSliderNode::onEvent1(System*, const Rect*, const NodeState*, int, int, int)
{ fixedSliderFrontier("onEvent1", "7D40A0, 292 B"); return false; }

// One shipped body serves both slots +78 and +7C, as in the base.
bool BfFixedSliderNode::onEvent2(System*, const Rect*, const NodeState*, int, int)
{ fixedSliderFrontier("onEvent2", "7D4430, 57 B, shared with onEvent3"); return false; }

bool BfFixedSliderNode::onEvent3(System*, const Rect*, const NodeState*, int, int)
{ fixedSliderFrontier("onEvent3", "7D4430, 57 B, shared with onEvent2"); return false; }

} }
#endif
