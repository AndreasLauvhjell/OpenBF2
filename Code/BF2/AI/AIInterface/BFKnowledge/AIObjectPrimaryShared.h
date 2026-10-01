// Primary AI object whose information is shared by every side.
// Collects secondary objects and represents the group with one neutral-side information handle.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTPRIMARYSHARED_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTPRIMARYSHARED_H

#include <vector>

#include "AIObjectShared.h"

namespace dice {
namespace hfe {
namespace ai {

class AIObjectPrimaryShared : public AIObjectShared
{
public:
    AIObjectPrimaryShared(AITemplate* aiTemplate, int side, world::IObject* object);
    virtual ~AIObjectPrimaryShared();

    virtual void addRef();                                      // over +0x40
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int iid) const;

    virtual void init();                                        // +0x48
    virtual void addInformationToGrid(int side);                // +0xC4
    virtual void removeFromGrid();                              // +0xC8
    virtual void reInsertInGrid();                              // +0xCC
    virtual bool isPrimary() const;                             // +0xF8
    virtual void positionChanged();                             // +0x104
    virtual bool hasSecondaryPosition() const;                  // +0x108
    virtual void registrateSecondaryPosition(IAIObject* secondary);   // +0x10C
    virtual void unregisterSecondaryPosition(IAIObject* secondary);   // +0x110
    virtual IAIObject* getSecondaryObject(int& iterator);       // +0x114
    virtual const IAIObject* getSecondaryObject(int& iterator) const;   // +0x118
    virtual IAIObject* getFirstSecondaryObject(int& iterator);  // +0x11C
    virtual const IAIObject* getFirstSecondaryObject(int& iterator) const;   // +0x120
    virtual int getFirstSecondaryObjectIterator() const;        // +0x124
    virtual void setSide(int side);                             // +0x128

protected:
    void traverseObject(world::IObject* object);

    /* +0x40 */ int m_refCount;
    /* +0x44 */ std::vector<IAIObject*> m_secondaryObjects;
    /* +0x50 */ bool m_isDestroying;
    /* +0x54 */ Vec3 m_lastGridPosition;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTPRIMARYSHARED_H
