// Item-container interface implemented independently of ISoldier.
// Their shared item methods do not make one interface a base of the other. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_IITEMCONTAINEROBJECT_H     // [INFERRED] guard style
#define GAME_OBJECTS_IITEMCONTAINEROBJECT_H

#include <string>

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace world {

class IObject;
class IPlayer;

class IItemContainerObject : public IBase
{
public:
    virtual bool addItem(IObject* item, int index) = 0;              // +0x10 [CHOSEN ret]
    virtual IObject* getItem(int index) = 0;                         // +0x14
    // Returns the detached item.
    virtual IObject* removeItem(int index) = 0;                      // +0x18
    virtual IObject* removeItem(IObject* item) = 0;                  // +0x1C
    // Soldier answers this from its template (+0x1C0 there), not from its own state.
    virtual int  getItemContainerType() const = 0;                   // +0x20
    virtual void handlePickupOnItem(IPlayer* player, IObject* item) = 0;   // +0x24
    virtual void handleDropOnItem(IPlayer* player, IObject* item) = 0;     // +0x28
protected:
    ~IItemContainerObject() {}
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_IITEMCONTAINEROBJECT_H
