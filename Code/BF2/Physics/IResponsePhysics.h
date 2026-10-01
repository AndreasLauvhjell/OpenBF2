// Collision-response interface and intrusive list node. Header path [INFERRED].
// Keep the 42 virtual slots in order. Manager lists use the object's own previous/next links;
// the list tag is 0 when detached, otherwise 1..5 identifying the owning manager list.

#ifndef PHYSICS_IRESPONSEPHYSICS_H     // [INFERRED] guard style
#define PHYSICS_IRESPONSEPHYSICS_H

#include "../Core/IBase.h"
#include "../Math/Vec3.h"
#include "../Math/Mat4.h"

namespace dice {
namespace hfe {
namespace world {

class IObject;
class IPhysicsNode;
struct ResponsePhysicsList;            // ResponsePhysicsManager.h - {head, tail}

class IResponsePhysics : public IBase
{
public:
    IResponsePhysics();                              // zeroes the link triple
    virtual ~IResponsePhysics();                     // +0x10/+0x14 - vptr reset only, no abort
                                                     //   (unlike IPhysicsNode's)

    virtual void  setEnabled(bool) = 0;                                        // +0x18
    virtual bool  getEnabled() const = 0;                                      // +0x1C
    virtual void  reset() = 0;                                                 // +0x20
    virtual unsigned int getClassID() const = 0;                               // +0x24 - the CID
                                                                               //   addObject routes on
    virtual void  checkObjectVsObject(IObject*, IObject*, float, bool) = 0;    // +0x28
    virtual void  checkVsTerrain(float) = 0;                                   // +0x2C
    virtual void  impulseOn(const Vec3&, const Vec3&, const Vec3&, float, int, int) = 0; // +0x30
    virtual void  solveImpulse(IPhysicsNode*) = 0;                             // +0x34
    virtual void  addFriction(IPhysicsNode*) = 0;                              // +0x38
    virtual void  setObject(IObject*) = 0;                                     // +0x3C
    virtual IObject* getObject() const = 0;                                    // +0x40
    virtual void  setCollisionChecked(unsigned int) = 0;                       // +0x44
    virtual unsigned int getCollisionChecked() const = 0;                      // +0x48
    virtual void  addToCollisionGroup(unsigned int) = 0;                       // +0x4C
    virtual void  removeFromCollisionGroup(unsigned int) = 0;                  // +0x50
    virtual unsigned int getCollisionGroups() const = 0;                       // +0x54
    virtual void  setCollisionGroups(unsigned int) = 0;                        // +0x58
    virtual void  setDirtyNextToCheck() = 0;                                   // +0x5C
    virtual IResponsePhysics* getNextToCheck() = 0;                            // +0x60
    virtual IResponsePhysics* getSimpleNextToCheck() = 0;                      // +0x64
    virtual void  setNextToCheck(IResponsePhysics*) = 0;                       // +0x68
    virtual void  resetNextToCheck() = 0;                                      // +0x6C
    virtual bool  shouldCheckCollision() = 0;                                  // +0x70
    virtual bool  hasCollisionLod(int) const = 0;                              // +0x74
    virtual void* getCollisionMesh() const = 0;                                // +0x78 - the
                                                                               //   object's +0x40
                                                                               //   cache; concrete
                                                                               //   type [UNRESOLVED]
    virtual bool  getIsIntersectingTerrain(Vec3&) const = 0;                   // +0x7C
    virtual unsigned char getPermanentGrip() const = 0;                        // +0x80
    virtual void  setPermanentGrip(unsigned char) = 0;                         // +0x84
    virtual void  setSurfacePositionalSpeed(const Vec3&) = 0;                  // +0x88
    virtual const Vec3& getSurfacePositionalSpeed() const = 0;                 // +0x8C
    virtual const Vec3& getPositionalAdjusts() const = 0;                      // +0x90
    virtual IResponsePhysics* getRootResponse() const = 0;                     // +0x94
    virtual bool  poseChangeTest(IObject*, IObject*, float) = 0;               // +0x98
    virtual bool  testIfValidTransformation(const Mat4&, bool) = 0;            // +0x9C
    virtual void  updateEffect(float) = 0;                                     // +0xA0

    // +0xA4 - the one real default (0x835C1F0, 12 B): returns the tag byte raw. Non-zero = listed,
    // and the value says on which of the manager's five lists.
    virtual unsigned char isInList() const;

    // Appends to an intrusive response list and stores its tag. Returns false unchanged
    // when already listed; a corrupt tail also returns false after zeroing this node's links.
    bool pushBackOnList(ResponsePhysicsList& list, unsigned char tag);

    // Unlinks from the response list, clearing links and tag on success. Returns false
    // when unlisted or when head/tail/middle consistency checks fail.
    bool removeFromList(ResponsePhysicsList& list);

protected:
    friend class ResponsePhysicsManager;             // the list owner - [CHOSEN] access mechanism
    /* +0x04 */ IResponsePhysics* m_listPrev;
    /* +0x08 */ IResponsePhysics* m_listNext;
    /* +0x0C */ unsigned char     m_listTag;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_IRESPONSEPHYSICS_H
