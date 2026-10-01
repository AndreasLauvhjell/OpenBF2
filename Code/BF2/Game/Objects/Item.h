// Carryable world object with owner, team and enabled state. [INFERRED] Source placement.
// Its reference-count overrides use Item's own counter, independently of Object's counter.

#ifndef GAME_OBJECTS_ITEM_H     // [INFERRED] guard style
#define GAME_OBJECTS_ITEM_H

#include "Base/Bundle.h"
#include "IItemObject.h"

namespace dice {
namespace hfe {
namespace world {

class ItemTemplate;
class IPlayer;

class Item : public Bundle, public IItemObject
{
public:
    // (C1) / 0x81828E0 (C2), 121 B. Bundle(objectTemplate), the three vptrs, then
    // refCount 1, ownerId -1, itemType and team copied OFF THE template, isEnabled true.
    explicit Item(const ItemTemplate* objectTemplate);
    // (D1) 49 B / 0x8182E60 (D0) 62 B - empty apart from the vptr resets the compiler
    // emits itself.
    virtual ~Item();

    // the IBase quartet, on item'S OWN count (+0x140)
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;    // adds IID_IItemObject

    // the one Object slot Item overrides
    virtual void  initNetworkableComp();                     // (Object +0x178)

    // Item's own fourteen, in primary-vtable order (see the map above)
    virtual void handlePickup(IPlayer* player, IObject* obj);
    virtual void handleDrop(IPlayer* player, IObject* obj);
    virtual void setOwnerId(int ownerId);
    virtual int  getOwnerId();
    virtual void setItemType(int type);
    virtual int  getItemType() const;
    // enableItem returns bool. IItemObject.h declared it void as a [CHOSEN] return; the ELF
    // materialises the value explicitly on both paths (`xor eax,eax` when already enabled,
    // `mov eax,1` after the work), which is not what a void body leaves behind. Corrected there.
    virtual bool enableItem();
    virtual void disableItem();                                 // void: the
                                                                // already-disabled path writes no eax
    virtual bool isEnabled();
    virtual void setTeam(int team);
    virtual int  getTeam();
    virtual int  getItemIndex();                                // off the template
    virtual int  getRank() const;                               // off the template
    virtual bool isReadyToUse();                                // constant true

protected:
    // The template, already typed. Object holds it at +0x14; Item reads it inline everywhere, as
    // the ELF does (`mov eax,[this+14h]` then a field off it - no accessor exists on either side).
    const ItemTemplate* itemTemplate() const;

    // layout, offsets 320..339. Bundle occupies 0..315, the IItemObject vptr 316..319.
    // NOT Object::m_refCount. See the header note: this is a second, independent count, and it is
    // the one the IBase quartet on this class operates on.
    /* 320 */ int  m_itemRefCount;               // ctor 1
    /* 324 */ int  m_ownerId;                    // ctor -1, i.e. "nobody"
    /* 328 */ int  m_itemType;                   // ctor = ItemTemplate::m_itemType   (+0x188)
    /* 332 */ bool m_isEnabled;                  // ctor true
    /* 336 */ int  m_team;                       // ctor = ItemTemplate::m_team       (+0x18C)
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_ITEM_H
