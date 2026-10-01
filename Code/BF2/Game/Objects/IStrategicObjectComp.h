// Strategic landmark component interface. getHolderPosition returns shared storage by reference.
// The separate interface class and source placement are [INFERRED]; preserve the declared virtual order.

#ifndef GAME_OBJECTS_ISTRATEGICOBJECTCOMP_H     // [INFERRED] guard style
#define GAME_OBJECTS_ISTRATEGICOBJECTCOMP_H

#include <string>

#include "IObjectComponent.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace world {

class IStrategicObjectComp : public IObjectComponent
{
public:
    // Returns the template's strategic-object name by reference.
    virtual const std::string& getStrategicObjectName() = 0;      // +0x38
    virtual const std::string& getIntactIconName() = 0;           // +0x3C
    virtual const std::string& getDestroyedIconName() = 0;        // +0x40
    // 77E500 signed-compares both results with zero and stores the same dword directly into
    // MapIconRecord::texture. They are integer texture handles, despite the historical Ptr names.
    virtual int   getIntactIconPtr() = 0;                         // +0x44
    virtual int   getDestroyedIconPtr() = 0;                      // +0x48
    virtual int   getPart() = 0;                                  // +0x4C
    // Index of the holder assigned by GameLogic.
    virtual int   getIndex() = 0;                                 // +0x50
    virtual void  setIndex(int) = 0;                              // +0x54
    virtual void* getHudInfo() = 0;                               // +0x58 [CHOSEN]
    virtual void* getHudIcons() = 0;                              // +0x5C [CHOSEN]
    virtual bool  has3dMapIcon() const = 0;                       // +0x60
    virtual int   get3dMapIcon() const = 0;                       // +0x64
    virtual bool  isOnePiece() const = 0;                         // +0x68
    // True when the holder exists and has at least one destroyed object.
    virtual bool  isPartOfDestroyedHolder() const = 0;            // +0x6C
    virtual const Vec3& getHolderPosition() const = 0;            // +0x70
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_ISTRATEGICOBJECTCOMP_H
