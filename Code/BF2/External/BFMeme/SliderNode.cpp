// dice::meme::SliderNode. Constructor, metadata, both child accessors and
// visitChildren are read in full from BF2.exe; the six per-frame/event bodies
// below are named frontiers with their addresses and byte counts recorded.
#include "SliderNode.h"
#if !DEDICATED
#include "TypeDescriptor.h"
#include "Style.h"   // dice::meme::Rect is defined here
#include <cstdio>
#include <cstdlib>
namespace dice { namespace meme {

// Node() first, then nine own words in the shipped order: the vptr
// store sits between the zeroed group and the two 1.0 pairs, which is why the
// member initializers below are not in declaration order in the binary.
SliderNode::SliderNode()
    : Node(), m_grabOffset(0.0f), m_thumbOffset(0.0f), m_minimum(0.0f),
      m_maximum(1.0f), m_thumbSpan(1.0f), m_data(nullptr), m_child1(nullptr)
{
    m_state.target = 1.0f;   // +24
    m_state.current = 1.0f;  // +28
}

TypeDescriptor* SliderNode::getClassType()                // 833AB0, singletonA39E34
{
    static ClassTypeDescriptor<SliderNode, Node> type("dice::meme::SliderNode");
    return &type;
}
TypeDescriptor* SliderNode::getType() { return getClassType(); }
unsigned int SliderNode::getObjectSize() { return sizeof(SliderNode); }
int SliderNode::getChildCount() { return 2; }             // 828A70, shared with SplitNode

// not an array and not a range check: index 1 is the thumb and
// everything else - zero, negative, out of range - is Node's own +04 slot.
Node*& SliderNode::getChild(int index)
{
    if (index == 1) return m_child1;
    return m_next;
}

// The setter's fallback differs from the getter's: only 0 and 1 are
// accepted here, and any other index writes nothing and still returns.
void SliderNode::setChild(int index, Node* child)
{
    if (index == 0) { m_next = child; return; }
    if (index == 1) m_child1 = child;
}

// The thumb is visited first, through the visitor's +08 hook, with a
// rect shifted along x by the thumb offset and narrowed to the thumb's share of
// the range; the tracked child then gets the caller's own bounds and state
// through the +04 hook.
// The span is the ordered difference, chosen by a single `fcomp`: only a
// maximum strictly below the minimum takes the reversed subtraction, so an
// unordered (NaN) compare falls with the normal case exactly as `<` does here.
void SliderNode::visitChildren(System* system, NodeState* state, const Rect* bounds,
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


// [FRONTIER] [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: slider-node-bodies]
// Unfinished drawing and input methods diagnose and stop on use; BfFixedSliderNode shares these boundaries.

namespace {
void sliderFrontier(const char* method, const char* site)
{
    std::fprintf(stderr, "[FRONTIER] dice::meme::SliderNode::%s reached (BF2.exe %s)\n",
                 method, site);
    std::fflush(stderr);
    std::abort();
}
}

bool SliderNode::streamProperties(System*, Stream*)
{ sliderFrontier("streamProperties", "833660, 151 B"); return false; }

void SliderNode::paint(System*, const NodeState*, const Rect*, const Rect*, RenderContext*)
{ sliderFrontier("paint", "833B80, 269 B"); }

bool SliderNode::update(System*, const NodeState*, const Rect*, float)
{ sliderFrontier("update", "833C90, 325 B"); return false; }

bool SliderNode::updateSelf(System*, const NodeState*, const Rect*, float)
{ sliderFrontier("updateSelf", "833810, 233 B"); return false; }

bool SliderNode::onEvent1(System*, const Rect*, const NodeState*, int, int, int)
{ sliderFrontier("onEvent1", "833900, 292 B"); return false; }

// One shipped body serves both slots +78 and +7C; it is not duplicated code.
bool SliderNode::onEvent2(System*, const Rect*, const NodeState*, int, int)
{ sliderFrontier("onEvent2", "833A30, 46 B, shared with onEvent3"); return false; }

bool SliderNode::onEvent3(System*, const Rect*, const NodeState*, int, int)
{ sliderFrontier("onEvent3", "833A30, 46 B, shared with onEvent2"); return false; }

} }
#endif
