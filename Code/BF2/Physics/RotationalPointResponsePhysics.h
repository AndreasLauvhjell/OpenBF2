// Tumbling-point response with separate reference count; header path [INFERRED].
// Overrides collision behavior while retaining PointResponsePhysics contact bookkeeping.

#ifndef PHYSICS_ROTATIONALPOINTRESPONSEPHYSICS_H     // [INFERRED] guard style
#define PHYSICS_ROTATIONALPOINTRESPONSEPHYSICS_H

#include "PointResponsePhysics.h"

namespace dice {
namespace hfe {
namespace world {

class RotationalPointResponsePhysics : public PointResponsePhysics
{
public:
    RotationalPointResponsePhysics();
    virtual ~RotationalPointResponsePhysics();

    virtual void  addRef();                          // the +0x90 counter
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;

    virtual unsigned int getClassID() const;         // CID_RotationalPointResponsePhysics
    virtual void  checkObjectVsObject(IObject*, IObject*, float, bool);
    virtual void  checkVsTerrain(float);
    virtual void  solveImpulse(IPhysicsNode*);
    virtual void  setCollisionChecked(unsigned int); // +0x2C again
    virtual unsigned int getCollisionChecked() const;

protected:
    /* +0x90 */ int     m_refCount;                  // ctor 1
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_ROTATIONALPOINTRESPONSEPHYSICS_H
