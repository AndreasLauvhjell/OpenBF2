// Base state for runtime AI objects. Owns six plug-ins and borrows its object template.
// Pair borrowArmament with resetArmament before destruction to avoid deleting the lender's plug-in.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECT_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECT_H

#include "IAIObject.h"

namespace dice {
namespace hfe {
namespace ai {

class AITemplate;

class AIObject : public IAIObject
{
public:
    explicit AIObject(AITemplate* aiTemplate);
    virtual ~AIObject();

    // IBase - AIObject's own counter at +0x04
    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;

    // IObjectComponent
    virtual world::IObject* getObject() const;                  // +0x20

    // IAIObject
    virtual void lockInit();                                    // +0x40
    virtual void unlockInit();                                  // +0x44
    virtual bool isArmament() const;                            // +0x64
    virtual bool isCover() const;                               // +0x68
    virtual bool isPhysical() const;                            // +0x6C
    virtual bool isMobile() const;                              // +0x70
    virtual bool isUnit() const;                                // +0x74
    virtual IAIObjectPlugIn* getPlugIn(IAIObjectFlags which) const;   // +0x78
    virtual void borrowArmament(IAIObject* from);               // +0x7C
    virtual void resetArmament();                               // +0x80
    virtual IAIObjectArmament* getArmament() const;             // +0x84
    virtual IAIObjectControlInfo* getControlInfo() const;       // +0x88
    virtual IAIObjectCover* getCover() const;                   // +0x8C
    virtual IAIObjectMobile* getMobile() const;                 // +0x90
    virtual IAIObjectPhysical* getPhysical() const;             // +0x94
    virtual IAIObjectUnit* getUnit() const;                     // +0x98
    virtual void setIsAllowedByBots(bool allowed);              // +0x9C
    virtual bool getIsAllowedByBots();                          // +0xA0
    virtual bool isSame(int flags) const;                       // +0xA8
    virtual unsigned int getObjectId() const;                   // +0xE4
    virtual SoldierPose getSoldierPose() const;                 // +0xEC
    virtual const Vec3& getSoldierPoseCameraPosition(SoldierPose pose) const;   // +0xF0
    virtual int getKitType() const;                             // +0xF4
    virtual IAIObject* getPrimaryObject();                      // +0xFC
    virtual void setPrimaryObject(IAIObject* primary);          // +0x100
    virtual void positionChanged();                             // +0x104, - empty
    virtual bool hasSecondaryPosition() const;                  // +0x108
    virtual void registrateSecondaryPosition(IAIObject* secondary);   // +0x10C, - empty
    virtual void unregisterSecondaryPosition(IAIObject* secondary);   // +0x110, - empty
    virtual IAIObject* getSecondaryObject(int& iterator);       // +0x114
    virtual const IAIObject* getSecondaryObject(int& iterator) const;   // +0x118
    virtual IAIObject* getFirstSecondaryObject(int& iterator);  // +0x11C
    virtual const IAIObject* getFirstSecondaryObject(int& iterator) const;   // +0x120
    virtual int getFirstSecondaryObjectIterator() const;        // +0x124
    virtual int getSide() const;                                // +0x12C
    virtual bool hasSameParentObject(const IAIObject* other) const;   // +0x130
    virtual bool isRootObject() const;                          // +0x140
    virtual int getPlayerId() const;                            // +0x148
    virtual IAIObject* getWreckObject(world::IObject* wreck) const;   // +0x14C

    // AIObject's own two slots
    virtual void createInfoPlugins(unsigned int handle, bool real, int side);   // +0x150
    // The template's plug-in bit mask (AITemplate::m_plugInMask). Pure here; AIObjectReal and
    // AIObjectShared each supply the same 14-byte body.
    virtual int getFlags() const = 0;                           // +0x154

    // A plug-in kind a secondary object may take from its primary instead of
    // having its own (the Secondary createInfoPlugins bodies ask the primary for cover and physical).
    static bool isLinked(IAIObjectFlags which);
    static bool m_plugInLinkVec[6];                             // ds: = {0,0,1,0,1,0}

protected:
    void IAIObjectInit();
    void aiObjectUpdate();
    void aiObjectPositionChanged();
    void aiObjectAddPlugIn(IAIObjectPlugIn* plugIn);
    bool checkIfInfoExists(unsigned int handle);
    unsigned int createInformationInternal(bool real, double time, int side, bool addToGrid);
    void addInfoToGrid(unsigned int handle, int side);
    bool isLocked();

    /* +0x04 */ int  m_refCount;
    /* +0x08 */ bool m_isAllowedByBots;
    /* +0x0C */ int  m_initLock;
    /* +0x10 */ int  m_side;
    /* +0x14 */ bool m_ownSideInGrid;
    /* +0x18 */ world::IObject* m_object;
    /* +0x1C */ AITemplate* m_template;
    /* +0x20 */ IAIObjectPlugIn* m_plugIns[6];
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECT_H
