// Primary AI object with separate information for each side.
// Collects secondary objects and represents the group in the information grid.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTPRIMARYREAL_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTPRIMARYREAL_H

#include <vector>

#include "AIObjectReal.h"

namespace dice {
namespace hfe {
namespace ai {

class AIObjectPrimaryReal : public AIObjectReal
{
public:
    AIObjectPrimaryReal(AITemplate* aiTemplate, int side, world::IObject* object);
    virtual ~AIObjectPrimaryReal();

    virtual void addRef();                                      // over +0x48
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

    /* +0x48 */ int m_refCount;
    /* +0x4C */ std::vector<IAIObject*> m_secondaryObjects;
    /* +0x58 */ bool m_isDestroying;
    /* +0x5C */ Vec3 m_lastGridPosition;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTPRIMARYREAL_H
