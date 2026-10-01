// Object-component interface exposing engine objects to the AI DLL.
// [INFERRED] Return types are derived from implementations. Preserve virtual declaration order;
// ELF has two destructor slots and MSVC has one. AIObject adds its own methods after this interface.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_IAIOBJECT_H
#define AI_AIINTERFACE_BFKNOWLEDGE_IAIOBJECT_H

#include <string>

#include "AIAdapterTypes.h"
#include "InformationType.h"
#include "SoldierPose.h"
#include "../../../Game/Objects/IObjectComponent.h"

namespace dice {
namespace hfe {
namespace ai {

class IAIObjectPlugIn;
class IAIObjectArmament;
class IAIObjectControlInfo;
class IAIObjectCover;
class IAIObjectMobile;
class IAIObjectPhysical;
class IAIObjectUnit;

// IAIObjectFlags - the plug-in index. Attested by name (getPlugIn(IAIObjectFlags)
// isLinked(IAIObjectFlags)); the values are the plug-ins' getId results and AIObject's slot order
// (getPlugIn's switch: 0 -> +0x20 .. 5 -> +0x34). [CHOSEN] the enumerator spellings.
// Despite the name these are indices. The bit form (1 << index) is AITemplate::m_plugInMask
// which AIObjectReal/AIObjectShared::getFlags return.
enum IAIObjectFlags
{
    AIOFArmament    = 0,
    AIOFControlInfo = 1,
    AIOFCover       = 2,
    AIOFMobile      = 3,
    AIOFPhysical    = 4,
    AIOFUnit        = 5
};

class IAIObject : public world::IObjectComponent
{
public:
    // - store the vptr, nothing else. Emitted beside AIObject.cpp's text
    // as comdat copies, i.e. defined inline.
    virtual ~IAIObject() {}                                                     // +0x38/+0x3C

    virtual void lockInit() = 0;                                                // +0x40 / PE +0x3C
    virtual void unlockInit() = 0;                                              // +0x44
    virtual void init() = 0;                                                    // +0x48 / PE +0x44
    virtual void update() = 0;                                                  // +0x4C
    virtual const std::string& getName() const = 0;                             // +0x50
    virtual const std::string& getAIName() const = 0;                           // +0x54
    virtual const std::string& getObjectTypeName() const = 0;                   // +0x58
    virtual int getPlayerControlObjectId() = 0;                                 // +0x5C
    virtual void addPlugIn(IAIObjectPlugIn* plugIn) = 0;                        // +0x60 / PE +0x5C
    virtual bool isArmament() const = 0;                                        // +0x64
    virtual bool isCover() const = 0;                                           // +0x68
    virtual bool isPhysical() const = 0;                                        // +0x6C
    virtual bool isMobile() const = 0;                                          // +0x70
    virtual bool isUnit() const = 0;                                            // +0x74
    virtual IAIObjectPlugIn* getPlugIn(IAIObjectFlags which) const = 0;         // +0x78
    virtual void borrowArmament(IAIObject* from) = 0;                           // +0x7C
    virtual void resetArmament() = 0;                                           // +0x80
    virtual IAIObjectArmament* getArmament() const = 0;                         // +0x84
    virtual IAIObjectControlInfo* getControlInfo() const = 0;                   // +0x88
    virtual IAIObjectCover* getCover() const = 0;                               // +0x8C
    virtual IAIObjectMobile* getMobile() const = 0;                             // +0x90
    virtual IAIObjectPhysical* getPhysical() const = 0;                         // +0x94
    virtual IAIObjectUnit* getUnit() const = 0;                                 // +0x98
    virtual void setIsAllowedByBots(bool allowed) = 0;                          // +0x9C
    virtual bool getIsAllowedByBots() = 0;                                      // +0xA0
    virtual bool isOfType(InformationType type) const = 0;                      // +0xA4
    virtual bool isSame(int flags) const = 0;                                   // +0xA8
    virtual unsigned int createInformation(int side, bool addToGrid) = 0;       // +0xAC
    virtual unsigned int createInformation(double time, int side, bool addToGrid) = 0;   // +0xB0
    virtual unsigned int forceGetInformation(int side, bool addToGrid) = 0;     // +0xB4
    virtual unsigned int getInformation(int side) = 0;                          // +0xB8
    virtual void deleteInformation(int side) = 0;                               // +0xBC
    virtual void deleteAllInformation() = 0;                                    // +0xC0
    virtual void addInformationToGrid(int side) = 0;                            // +0xC4
    virtual void removeFromGrid() = 0;                                          // +0xC8
    virtual void reInsertInGrid() = 0;                                          // +0xCC
    virtual unsigned int getHandle(int side) const = 0;                         // +0xD0
    virtual const Vec3& getPosition() const = 0;                                // +0xD4
    virtual bool setPosition(const Vec3& position, int side) = 0;               // +0xD8
    virtual const Mat4& getTransformation() const = 0;                          // +0xDC
    virtual bool setTransformation(const Mat4& transformation, int side) = 0;   // +0xE0
    virtual unsigned int getObjectId() const = 0;                               // +0xE4
    virtual float getBoundingRadius() = 0;                                      // +0xE8
    virtual SoldierPose getSoldierPose() const = 0;                             // +0xEC
    virtual const Vec3& getSoldierPoseCameraPosition(SoldierPose pose) const = 0;   // +0xF0
    virtual int getKitType() const = 0;                                         // +0xF4
    virtual bool isPrimary() const = 0;                                         // +0xF8
    virtual IAIObject* getPrimaryObject() = 0;                                  // +0xFC
    virtual void setPrimaryObject(IAIObject* primary) = 0;                      // +0x100
    virtual void positionChanged() = 0;                                         // +0x104
    virtual bool hasSecondaryPosition() const = 0;                              // +0x108
    virtual void registrateSecondaryPosition(IAIObject* secondary) = 0;         // +0x10C (dice's spelling)
    virtual void unregisterSecondaryPosition(IAIObject* secondary) = 0;         // +0x110
    virtual IAIObject* getSecondaryObject(int& iterator) = 0;                   // +0x114
    virtual const IAIObject* getSecondaryObject(int& iterator) const = 0;       // +0x118
    virtual IAIObject* getFirstSecondaryObject(int& iterator) = 0;              // +0x11C
    virtual const IAIObject* getFirstSecondaryObject(int& iterator) const = 0;  // +0x120
    virtual int getFirstSecondaryObjectIterator() const = 0;                    // +0x124
    virtual void setSide(int side) = 0;                                         // +0x128
    virtual int getSide() const = 0;                                            // +0x12C
    virtual bool hasSameParentObject(const IAIObject* other) const = 0;         // +0x130
    virtual void releaseObjectFromSpawner() = 0;                                // +0x134
    virtual bool getObjectSpawnerHolding() const = 0;                           // +0x138
    virtual IAIObject* getObjectSpawner() const = 0;                            // +0x13C
    virtual bool isRootObject() const = 0;                                      // +0x140
    virtual void striveForRotationalSpeed(int axis, float target, float step) = 0;   // +0x144
    virtual int getPlayerId() const = 0;                                        // +0x148
    virtual IAIObject* getWreckObject(world::IObject* wreck) const = 0;         // +0x14C
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_IAIOBJECT_H
