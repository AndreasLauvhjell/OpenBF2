// Menu edit-control data. [INFERRED] Source placement.
#include "BfEditData.h"
#if !DEDICATED
#include "BfEditNode.h"
#include "../External/BFMeme/TypeDescriptor.h"
namespace dice { namespace meme {
namespace {
class BfEditDataTypeDescriptor : public ClassTypeDescriptor<BfEditData, Data>
{
public:
    BfEditDataTypeDescriptor() : ClassTypeDescriptor<BfEditData, Data>("dice::meme::BfEditData") {}
};
}
BfEditData::BfEditData(const char* name)
    : Data(name), m_activeNode(nullptr), m_focusLocked(false), m_hideText(false) {}
BfEditData::~BfEditData() {} // list storage only, then Data; never deletes nodes
TypeDescriptor* BfEditData::getClassType()
{
    static BfEditDataTypeDescriptor type;
    return &type;
}
TypeDescriptor* BfEditData::getType() { return getClassType(); }
unsigned int BfEditData::getObjectSize() { return sizeof(BfEditData); }
void BfEditData::registerNode(BfEditNode* node) { m_nodes.push_back(node); }
void BfEditData::queueKey(int key)
{
    if (m_activeNode) {
        m_activeNode->m_pendingInput = 1;
        m_activeNode->m_pendingKey = key;
    }
}
void BfEditData::queueCharacter(int character)
{
    if (m_activeNode) {
        m_activeNode->m_pendingInput = 2;
        m_activeNode->m_pendingCharacter = character;
    }
}
void BfEditData::clear(System* system) // 7AAD70; no active-string null guard
{
    BfEditNode* node = m_activeNode; // retained in ESI across both virtual call sequences
    if (node) {
        node->m_string->getStringLength(system); // discarded call is observable
        node->m_string->setString(system, "");  // reload string after character callbacks
        node->m_cursor = 0;
    }
}
void BfEditData::setFocus(bool focus) // 7AADB0; lock gate precedes node gate
{
    if (!m_focusLocked && m_activeNode) m_activeNode->m_focused = focus;
}
void BfEditData::setCursorPosition(int cursor) // [INFERRED] accessor name; HUD7B13F7/FD
{
    m_activeNode->m_cursor = cursor;
}
} }
#endif
