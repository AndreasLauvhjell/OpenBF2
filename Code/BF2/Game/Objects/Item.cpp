// Item state, pickup handling, and network-component initialization.
#include "Item.h"

#include "../../Core/Ids.h"                 // IID_IBase / IID_IItemObject / CID_ItemNetworkable
#include "../../IO/Console/IConsoleSaveable.h"
#include "IItemContainerObject.h"           // addItem (+0x10) / removeItem (+0x18)
#include "IObjectTemplate.h"                // getNetworkableInfo (+0x94)
#include "IPlayer.h"                        // getId (+0x54)
#include "ItemTemplate.h"
#include "../../Scene/Object/ObjectUtil.h"  // world::setNetworkableComp

namespace dice {
namespace hfe {
namespace world {

const ItemTemplate* Item::itemTemplate() const
{
    return static_cast<const ItemTemplate*>(getTemplate());          // IObject +0x14
}

// Initializes an unowned, enabled item and copies type and team from its template.
// Requires a valid template.
Item::Item(const ItemTemplate* objectTemplate)
    : Bundle(objectTemplate)
    , m_itemRefCount(1)
    , m_ownerId(-1)
    , m_itemType(objectTemplate->m_itemType)
    , m_isEnabled(true)
    , m_team(objectTemplate->m_team)
{
}

// the vptr resets and Bundle::~Bundle, i.e. an empty body.
Item::~Item()
{
}

// THE IBase quartet - on Item's OWN count at +0x140, not Object's at +0x84.
void Item::addRef()  { ++m_itemRefCount; }
int  Item::getRef()  { return m_itemRefCount; }

// , and the same shape as Object::release: decrement, delete at zero through the primary
// deleting-destructor slot (+0x18), return the NEW count. The `test edx,edx` before the call is
// gcc's own null guard for `delete this`, not a source-level test.
int Item::release()
{
    const int refs = --m_itemRefCount;
    if (refs == 0)
        delete this;
    return refs;
}

// . Object's three answers plus IID_IItemObject. It does NOT delegate to Bundle/Object - the
// whole chain is re-tested here, which is why an unknown id returns 0 rather than walking up.
void* Item::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObject)
        return const_cast<Item*>(this);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(const_cast<Item*>(this));
    if (iid == IID_IItemObject)
        return static_cast<IItemObject*>(const_cast<Item*>(this));
    return 0;
}

// Creates item replication only when the template's networkable-info string is nonempty.
void Item::initNetworkableComp()
{
    const IObjectTemplate* const objectTemplate = getTemplate();      // +0x14
    if (!objectTemplate)
        return;
    if (objectTemplate->getNetworkableInfo().compare("") != 0)        // slot +0x94
        setNetworkableComp(this, CID_ItemNetworkable);
}

// Sets ownership and adds this item to the supplied container at the template's item index.
// Requires a valid player and an object exposing IItemContainerObject.
void Item::handlePickup(IPlayer* player, IObject* obj)
{
    const ItemTemplate* const objectTemplate = itemTemplate();
    setOwnerId(player->getId());                                      // IPlayer +0x54, virtual on us
    IItemContainerObject* container = 0;
    if (obj)
        container = static_cast<IItemContainerObject*>(
            obj->queryInterface(IID_IItemContainerObject));
    container->addItem(this, objectTemplate->m_itemIndex);            // +0x10 unchecked, as shipped
}

// Clears ownership and asks the container to remove this object id as an item index.
// The original id/index mismatch is retained; out-of-range ids may leave the item in the container.
void Item::handleDrop(IPlayer* /*player*/, IObject* obj)
{
    setOwnerId(-1);
    const int index = static_cast<int>(getObjectId());                // +0x10 see above
    IItemContainerObject* container = 0;
    if (obj)
        container = static_cast<IItemContainerObject*>(
            obj->queryInterface(IID_IItemContainerObject));
    container->removeItem(index);                                     // +0x18 unchecked, as shipped
}

// THE accessors AND THE TWO enable arms.
void Item::setOwnerId(int ownerId) { m_ownerId = ownerId; }
int  Item::getOwnerId()            { return m_ownerId; }
void Item::setItemType(int type)   { m_itemType = type; }
int  Item::getItemType() const     { return m_itemType; }
bool Item::isEnabled()             { return m_isEnabled; }
int  Item::getTeam()               { return m_team; }

// . Drawing an item clears flag bit 0 (updateFlags(set=0, clear=1)) and puts the object back on
// the ordinary update frequency. Returns false when it was already enabled, having done nothing.
bool Item::enableItem()
{
    if (m_isEnabled)
        return false;
    m_isEnabled = true;
    updateFlags(0, 1);                                                          // +0x60
    setUpdateFrequencyType(static_cast<UpdateFreqencyType>(0));                  // +0xC4
    return true;
}

// the mirror, minus the frequency reset: stowing sets bit 0 and leaves the frequency alone.
// Not symmetric with enableItem, and that asymmetry is the shipped behaviour.
void Item::disableItem()
{
    if (!m_isEnabled)
        return;
    m_isEnabled = false;
    updateFlags(1, 0);                                                          // +0x60
}

// . THE team IS template-gated: an item only changes team if its template says it may. The
// compiled test is `cmp byte [template+0x194], 1` - ItemTemplate::m_teamChange.
void Item::setTeam(int team)
{
    if (itemTemplate()->m_teamChange)
        m_team = team;
}

// each, and both read the template, not this object: the item index and the rank gate are
// per-template facts. (`Kit::getCreationIndex` reaches its own template field the same way.)
int Item::getItemIndex()   { return itemTemplate()->m_itemIndex; }
int Item::getRank() const  { return itemTemplate()->m_rank; }

// a constant. The base item is always ready; classes with a warm-up override it.
bool Item::isReadyToUse() { return true; }

} // namespace world
} // namespace hfe
} // namespace dice
