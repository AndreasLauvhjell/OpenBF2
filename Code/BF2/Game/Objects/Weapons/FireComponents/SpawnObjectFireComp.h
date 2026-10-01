// Firing component that spawns an object.

#ifndef GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_SPAWNOBJECTFIRECOMP_H
#define GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_SPAWNOBJECTFIRECOMP_H

#include "SingleFireComp.h"

namespace dice {
namespace hfe {
namespace world {

class ICollisionMesh;
class SpawnObjectFireCompTemplate;

class SpawnObjectFireComp : public SingleFireComp
{
public:
    explicit SpawnObjectFireComp(SpawnObjectFireCompTemplate* objectTemplate);
    virtual ~SpawnObjectFireComp();

    // The concrete specialization owns another reference count.  Both primary and adjusted
    // IWeaponFireComp vtables carry this exact quartet in the shipped images.
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    virtual unsigned int getInterfaceID() const;
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual bool isSelectable() const;
    virtual const Mat4& getProjectileOrigin() const;
    virtual bool preEnable();

private:
    bool checkHeight();
    bool canSpawn();

    // Documentary ELF offsets: SingleFireComp occupies +0x000..+0x14F and this class is 0x160.
    // The corresponding PE offsets are +0x15C..+0x168 and sizeof is 0x16C because inherited STL
    // objects differ.  Preserve source order rather than copying either foreign layout.
    /* +0x150 */ int                          m_refCount;
    /* +0x154 */ SpawnObjectFireCompTemplate* m_spawnObjectTemplate;
    /* +0x158 */ ICollisionMesh*              m_collisionMesh;
    /* +0x15C */ bool                         m_tryCollisionMesh;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_WEAPONS_FIRECOMPONENTS_SPAWNOBJECTFIRECOMP_H
