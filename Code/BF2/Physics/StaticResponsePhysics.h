// Nonmoving collision response with enabled/group/object and next-chain bookkeeping; header path [INFERRED].
// Impulse/friction and permanent-grip setters are original no-ops. Parentless statics are found by
// collision queries rather than the manager's mobile-response lists.

#ifndef PHYSICS_STATICRESPONSEPHYSICS_H     // [INFERRED] guard style
#define PHYSICS_STATICRESPONSEPHYSICS_H

#include "IResponsePhysics.h"

namespace dice {
namespace hfe {
namespace world {

class StaticResponsePhysics : public IResponsePhysics
{
public:
    StaticResponsePhysics();
    virtual ~StaticResponsePhysics();

    virtual void  addRef();                          // refcount +0x10
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;   // IBase / IID_IResponsePhysics -> this

    virtual void  setEnabled(bool);                  // +0x14, a byte
    virtual bool  getEnabled() const;
    virtual void  reset();                           // empty
    virtual unsigned int getClassID() const;         // CID_StaticResponsePhysics
    virtual void  checkObjectVsObject(IObject*, IObject*, float, bool);   // empty
    virtual void  checkVsTerrain(float);             // empty
    virtual void  impulseOn(const Vec3&, const Vec3&, const Vec3&, float, int, int);   // empty
    virtual void  solveImpulse(IPhysicsNode*);       // empty
    virtual void  addFriction(IPhysicsNode*);        // empty
    virtual void  setObject(IObject*);               // +0x18
    virtual IObject* getObject() const;
    virtual void  setCollisionChecked(unsigned int); // empty
    virtual unsigned int getCollisionChecked() const;// 0
    virtual void  addToCollisionGroup(unsigned int);       // +0x1C |=
    virtual void  removeFromCollisionGroup(unsigned int);  // +0x1C &= ~
    virtual unsigned int getCollisionGroups() const;
    virtual void  setCollisionGroups(unsigned int);
    virtual void  setDirtyNextToCheck();             // 66 B - walks the chain resetting each link
    virtual IResponsePhysics* getNextToCheck();      // 271 B - , see the .cpp
    virtual IResponsePhysics* getSimpleNextToCheck();// +0x24, raw
    virtual void  setNextToCheck(IResponsePhysics*); // clears the reset flag, stores +0x24
    virtual void  resetNextToCheck();                // sets the reset flag, zeroes +0x24
    virtual bool  shouldCheckCollision();            // 107 B - the flags/mesh/CID test, see .cpp
    virtual bool  hasCollisionLod(int) const;        // -> m_object (IObject slot 51)
    virtual void* getCollisionMesh() const;          // m_object's +0x40 cache, raw field read
    virtual bool  getIsIntersectingTerrain(Vec3&) const;   // false
    virtual unsigned char getPermanentGrip() const;  // 0
    virtual void  setPermanentGrip(unsigned char);   // empty - see the header block
    virtual void  setSurfacePositionalSpeed(const Vec3&);  // empty
    virtual const Vec3& getSurfacePositionalSpeed() const; // Vec3::getZero
    virtual const Vec3& getPositionalAdjusts() const;      // Vec3::getZero
    virtual IResponsePhysics* getRootResponse() const;     // root parent's +0x48 cache
    virtual bool  poseChangeTest(IObject*, IObject*, float);        // true
    virtual bool  testIfValidTransformation(const Mat4&, bool);     // true
    virtual void  updateEffect(float);               // empty
    // isInList: inherited - the concrete table's +0xA4 entry is IResponsePhysics::isInList.

    // Collects responses for getNextToCheck without adding a virtual slot.
    void addToTmpResponseList(const IObject* object);

protected:
    // Derived response classes directly access enabled/object fields but use their own
    // reference counts. This base refcount remains at its initial value for those instances.
    /* +0x10 */ int     m_refCount;                  // ctor 1
    /* +0x14 */ bool    m_enabled;                   // ctor true (byte)
    /* +0x18 */ IObject* m_object;                   // ctor 0
    /* +0x1C */ unsigned int m_collisionGroups;      // ctor 0
    /* +0x20 */ bool    m_nextToCheckIsReset;        // ctor true - [CHOSEN] name; resetNextToCheck
                                                     //   sets it, setNextToCheck clears it
    /* +0x24 */ IResponsePhysics* m_simpleNextToCheck;   // ctor 0
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_STATICRESPONSEPHYSICS_H
