// Path:     [INFERRED] beside ItemContainerTemplate.h.
// Contents: exact distinct factory path recovered from both server ports.

#include "ItemContainerTemplate.h"
#include "Base/Bundle.h"
#include "../../Core/Ids.h"

namespace dice {
namespace hfe {
namespace world {

ItemContainerTemplate::ItemContainerTemplate()
    : m_refCount3(1),
      m_index(0),
      m_itemIndex(-1),
      m_itemType(-1),
      m_unlockLevel(0),
      m_items()
{
}

ItemContainerTemplate::~ItemContainerTemplate()
{
}

void ItemContainerTemplate::addRef()
{
    ++m_refCount3;
}

int ItemContainerTemplate::getRef()
{
    return m_refCount3;
}

int ItemContainerTemplate::release()
{
    const int refs = --m_refCount3;
    if (refs == 0)
        delete this;
    return refs;
}

void* ItemContainerTemplate::queryInterface(unsigned int iid) const
{
    ItemContainerTemplate* self = const_cast<ItemContainerTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplate)
        return static_cast<IObjectTemplate*>(self);
    if (iid == IID_ISimpleObjectTemplate)
        return static_cast<ISimpleObjectTemplate*>(self);
    if (iid == IID_IBundleTemplate)
        return static_cast<IBundleTemplate*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    if (iid == IID_IItemTemplate)
        return static_cast<IItemTemplate*>(self);
    return 0;
}

unsigned int ItemContainerTemplate::getClassID() const
{
    return CID_ItemContainerTemplate;
}

void* ItemContainerTemplate::queryImplementation(unsigned int classId)
{
    return classId == CID_ItemContainerTemplate
        ? this : BundleTemplate::queryImplementation(classId);
}

IObject* ItemContainerTemplate::createObject() const
{
    // Intentionally constructs the same runtime Bundle, passing this through its BundleTemplate
    // primary base.  It does not have a separate ItemContainer runtime class.
    return new Bundle(this);
}

void ItemContainerTemplate::setIndex(int index) { m_index = index; }
void ItemContainerTemplate::setItemIndex(int itemIndex) { m_itemIndex = itemIndex; }
int ItemContainerTemplate::getItemIndex() const { return m_itemIndex; }
void ItemContainerTemplate::setItemType(int itemType) { m_itemType = itemType; }
int ItemContainerTemplate::getItemType() const { return m_itemType; }

void ItemContainerTemplate::replaceItem(const std::string& item)
{
    m_items.insert(m_items.begin(), item);
}

} // namespace world
} // namespace hfe
} // namespace dice
