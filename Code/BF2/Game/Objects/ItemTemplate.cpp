// Selectable-item template defaults and class lookup.
#include "ItemTemplate.h"
#include "../../Core/Ids.h"

namespace dice {
namespace hfe {
namespace world {

// ELF C1 0x8182C30 / C2 0x8182BB0 (121 B each), PE 0x58F440 (103 B), read end to end.
// BundleTemplate constructs first. The three -1 sentinels and zero rank are explicit stores;
// neither image writes the trailing teamChange byte or its padding. The unmodelled secondary
// interface pointer remains layout residue; do not copy a retail vtable into the rebuilt object.
ItemTemplate::ItemTemplate()
    : m_refCount3(1)
    , m_itemIndex(-1)
    , m_itemType(-1)
    , m_team(-1)
    , m_rank(0)
{
}

// . Vtable +0x10. by the getClassID sweep - see
// SoldierTemplate::getClassID in Soldier.cpp for what a missing one of these actually costs.
unsigned int ItemTemplate::getClassID() const
{
    return CID_ItemTemplate;
}

// Recognizes ItemTemplate, delegating other class IDs to BundleTemplate.
void* ItemTemplate::queryImplementation(unsigned int cid)
{
    if (cid == CID_ItemTemplate)
        return this;
    return BundleTemplate::queryImplementation(cid);
}

} // namespace world
} // namespace hfe
} // namespace dice
