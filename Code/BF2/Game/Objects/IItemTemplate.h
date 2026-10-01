// Path:     [INFERRED] shared interface beside the object-template hierarchy.
// Contents: complete eight-slot item-template interface recovered from three implementors.

#ifndef GAME_OBJECTS_IITEMTEMPLATE_H
#define GAME_OBJECTS_IITEMTEMPLATE_H

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace world {

// ItemTemplate, ItemContainerTemplate and GenericFireArmTemplate all expose this exact secondary
// table: the IBase quartet followed by four item selection accessors.  It has no virtual dtor.
class IItemTemplate : public IBase
{
public:
    virtual void setItemIndex(int) = 0;
    virtual int getItemIndex() const = 0;
    virtual void setItemType(int) = 0;
    virtual int getItemType() const = 0;

protected:
    ~IItemTemplate() {}
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_IITEMTEMPLATE_H
