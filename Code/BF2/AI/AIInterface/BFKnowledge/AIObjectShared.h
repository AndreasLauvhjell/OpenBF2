// AI object with one real information handle shared by all sides through neutral side zero.
// Replacing a plug-in invalidates its information; changing side does not rebuild it.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTSHARED_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTSHARED_H

#include "AIObject.h"

namespace dice {
namespace hfe {
namespace ai {

class AIObjectShared : public AIObject
{
public:
    AIObjectShared(AITemplate* aiTemplate, int side, world::IObject* object);
    virtual ~AIObjectShared();

    virtual void addRef();                                      // over +0x38
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;

    virtual void setObject(world::IObject* object);             // +0x1C

    virtual void init();                                        // +0x48
    virtual void update();                                      // +0x4C
    virtual const std::string& getName() const;                 // +0x50
    virtual const std::string& getAIName() const;               // +0x54
    virtual const std::string& getObjectTypeName() const;       // +0x58
    virtual int getPlayerControlObjectId();                     // +0x5C
    virtual void addPlugIn(IAIObjectPlugIn* plugIn);            // +0x60
    virtual bool isOfType(InformationType type) const;          // +0xA4
    virtual unsigned int createInformation(int side, bool addToGrid);           // +0xAC
    virtual unsigned int createInformation(double time, int side, bool addToGrid);   // +0xB0
    virtual unsigned int forceGetInformation(int side, bool addToGrid);         // +0xB4
    virtual unsigned int getInformation(int side);              // +0xB8
    virtual void deleteInformation(int side);                   // +0xBC
    virtual void deleteAllInformation();                        // +0xC0
    virtual unsigned int getHandle(int side) const;             // +0xD0
    virtual const Vec3& getPosition() const;                    // +0xD4
    virtual bool setPosition(const Vec3& position, int side);   // +0xD8
    virtual const Mat4& getTransformation() const;              // +0xDC
    virtual bool setTransformation(const Mat4& transformation, int side);   // +0xE0
    virtual float getBoundingRadius();                          // +0xE8
    virtual void setSide(int side);                             // +0x128
    virtual void releaseObjectFromSpawner();                    // +0x134
    virtual bool getObjectSpawnerHolding() const;               // +0x138
    virtual IAIObject* getObjectSpawner() const;                // +0x13C
    virtual void striveForRotationalSpeed(int axis, float target, float step);   // +0x144
    virtual int getFlags() const;                               // +0x154

protected:
    /* +0x38 */ int m_refCount;
    /* +0x3C */ unsigned int m_handle;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTSHARED_H
