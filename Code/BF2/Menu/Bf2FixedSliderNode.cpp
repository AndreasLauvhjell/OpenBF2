// Named BF2 fixed-slider node.
#include "Bf2FixedSliderNode.h"
#if !DEDICATED
#include "../External/BFMeme/TypeDescriptor.h"
#include <cstring>
namespace dice { namespace meme {
namespace {
char* cloneSliderName(const char* name)
{
    if (!name) return nullptr;
    const unsigned int bytes = static_cast<unsigned int>(std::strlen(name)) + 1;
    char* copy = static_cast<char*>(::operator new(bytes));
    std::memcpy(copy, name, bytes);
    return copy;
}
void releaseSliderName(char*& owned)
{
    ::operator delete(owned);
    owned = nullptr;
}
}

// The name slot is released before it is ever written - the shipped
// body stores the four floats and zeroes +58, then releases +58 and clones into
// it - so that release always frees a null pointer. Same shape as
// Bf2NewListBoxNode's, Bf2EditNode's and Bf2TransformListNode's.
// The four floats land on the inherited BfFixedSliderNode block, overwriting
// its 0/1.0/1.0/1.0 defaults at +34..+40.
Bf2FixedSliderNode::Bf2FixedSliderNode(const char* name, float minimum, float maximum,
                                       float thumbSpan, float step)
    : BfFixedSliderNode(), m_name(nullptr)
{
    m_minimum = minimum;     // +34
    m_maximum = maximum;     // +38
    m_thumbSpan = thumbSpan; // +3C
    m_step = step;           // +40
    releaseSliderName(m_name);
    m_name = cloneSliderName(name);
}

TypeDescriptor* Bf2FixedSliderNode::getClassType()        // 7CDBD0, singletonA233BC
{
    static ClassTypeDescriptor<Bf2FixedSliderNode, BfFixedSliderNode>
        type("dice::meme::Bf2FixedSliderNode");
    return &type;
}
TypeDescriptor* Bf2FixedSliderNode::getType() { return getClassType(); }
unsigned int Bf2FixedSliderNode::getObjectSize() { return sizeof(Bf2FixedSliderNode); }
const char* Bf2FixedSliderNode::getName() { return m_name; }

bool Bf2FixedSliderNode::setName(const char* name)
{
    releaseSliderName(m_name);
    m_name = cloneSliderName(name);
    return true;
}

bool Bf2FixedSliderNode::slot_0x24() { return true; }

// 7CDC70, slot +C8. Fifteen bytes: store and report success. No null test.
bool Bf2FixedSliderNode::setDataForHud(Data* data)
{
    m_data = data;                                         // the inherited +44
    return true;
}

} }
#endif
