// Selectable item identity and team/rank availability settings.
// [INFERRED] source placement.

#ifndef GAME_OBJECTS_ITEMTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_ITEMTEMPLATE_H

#include "Base/BundleTemplate.h"

namespace dice {
namespace hfe {
namespace world {

class ItemTemplate : public BundleTemplate /* , public IItemTemplate @ +380 */
{
public:
    ItemTemplate();

    // Identifies ItemTemplate independently of its BundleTemplate base.
    virtual unsigned int getClassID() const;               // +0x10
    virtual void* queryImplementation(unsigned int cid);   // +0x50

    // Item reads identity and availability fields directly.
    friend class Item;

protected:
    // layout, offsets 380..407. BundleTemplate occupies 0..379.
    /* 380 */ void* pVMT_IItemTemplate;              // second base, RTTI-proven @ +380
    /* 384 */ int   m_refCount3;

    /* 388 */ int   m_itemIndex;
    /* 392 */ int   m_itemType;
    /* 396 */ int   m_team;
    /* 400 */ int   m_rank;                          // rank gate - the unlock mechanism

    /* 404 */ bool  m_teamChange;
    /* 405 */ bool  _byte195;                        // [UNRESOLVED]
    /* 406 */ bool  _byte196;
    /* 407 */ bool  _byte197;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_ITEMTEMPLATE_H
