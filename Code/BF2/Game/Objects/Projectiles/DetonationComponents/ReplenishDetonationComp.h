// Projectile detonation that applies replenishment effects.

#ifndef GAME_OBJECTS_PROJECTILES_DETONATIONCOMPONENTS_REPLENISHDETONATIONCOMP_H
#define GAME_OBJECTS_PROJECTILES_DETONATIONCOMPONENTS_REPLENISHDETONATIONCOMP_H

#include "DefaultDetonationComp.h"

namespace dice {
namespace hfe {
namespace world {

class ReplenishDetonationCompTemplate;

// DefaultDetonation plus a five-tick, weakly-held replenishment target. The subclass has its own
// reference count, just as every other ProjectileComp leaf does; the base count remains untouched.
class ReplenishDetonationComp : public DefaultDetonationComp
{
public:
    explicit ReplenishDetonationComp(ReplenishDetonationCompTemplate* objectTemplate);
    virtual ~ReplenishDetonationComp();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual unsigned int getInterfaceID() const;

    virtual void activate();
    virtual void updateComponent(float seconds, unsigned int flags);
    virtual void detonate(IObject* source, int collisionType, Vec3 position,
                          unsigned int tick, Vec3 normal,
                          CollisionFaceInfo* face);

protected:
    virtual bool checkIfObjectCanTrigger(IObject* candidate);

private:
    bool doReplenish(IObject* candidate, bool testOnly);

    /* +0x34 */ int m_refCount;
    /* +0x38 */ WeakPtr<IObject> m_replenishTarget;
    /* +0x3C */ unsigned int m_replenishTick;
    /* +0x40 */ ReplenishDetonationCompTemplate* m_template;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_PROJECTILES_DETONATIONCOMPONENTS_REPLENISHDETONATIONCOMP_H
