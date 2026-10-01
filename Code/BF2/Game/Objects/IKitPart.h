// Interface for an item stored in a kit. GenericFireArm uses the same template index for kit and item access.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_IKITPART_H     // [INFERRED] guard style, matching IItemObject.h
#define GAME_OBJECTS_IKITPART_H

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace world {

class IKitPart : public IBase
{
public:
    virtual int getRank() const = 0;          // +0x10 shared body with IItemObject::getRank
    virtual int getKitPartIndex() = 0;        // +0x14 == m_template->m_itemIndex
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_IKITPART_H
