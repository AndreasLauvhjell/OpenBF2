// Carried-item ownership, pickup, selection and readiness interface.
// Unverified return types remain marked on individual declarations. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_IITEMOBJECT_H     // [INFERRED] guard style
#define GAME_OBJECTS_IITEMOBJECT_H

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace world {

class IObject;
class IPlayer;

class IItemObject : public IBase
{
public:
    virtual void handlePickup(IPlayer* player, IObject* item) = 0;    // +0x10
    virtual void handleDrop(IPlayer* player, IObject* item) = 0;      // +0x14
    virtual void setOwnerId(int ownerId) = 0;                         // +0x18
    virtual int  getOwnerId() = 0;                                    // +0x1C
    virtual void setItemType(int type) = 0;                           // +0x20
    virtual int  getItemType() const = 0;                             // +0x24
    // Returns false if already enabled; true when the item is drawn.
    virtual bool enableItem() = 0;                                    // +0x28 draw / take out
    virtual void disableItem() = 0;                                   // +0x2C stow / put away
    virtual int  getItemIndex() = 0;                                  // +0x30
    virtual bool isReadyToUse() = 0;                                  // +0x34 [CHOSEN ret]
    virtual int  getRank() const = 0;                                 // +0x38 [CHOSEN ret]
protected:
    ~IItemObject() {}
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_IITEMOBJECT_H
