// Point-body collision response, with its own reference count and per-frame contact state.
// Header path [INFERRED]. The manager dispatches CID_PointResponsePhysics to point-object tests.

#ifndef PHYSICS_POINTRESPONSEPHYSICS_H     // [INFERRED] guard style
#define PHYSICS_POINTRESPONSEPHYSICS_H

#include "StaticResponsePhysics.h"

namespace dice {
namespace hfe {
namespace world {

class CollisionFaceInfo;                             // doResponse's 7th argument - see CollisionMeshTemplate.h

class PointResponsePhysics : public StaticResponsePhysics
{
public:
    PointResponsePhysics();
    virtual ~PointResponsePhysics();

    virtual void  addRef();                          // the +0x28 counter
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;   // IBase / IID_IResponsePhysics -> this

    virtual unsigned int getClassID() const;         // CID_PointResponsePhysics
    virtual void  checkObjectVsObject(IObject*, IObject*, float, bool);
    virtual void  checkVsTerrain(float);
    virtual void  solveImpulse(IPhysicsNode*);
    virtual void  setCollisionChecked(unsigned int); // +0x2C
    virtual unsigned int getCollisionChecked() const;

    // Non-virtual.
    void doResponse(IObject*, IObject*, const Vec3&, float, const Vec3&, int, CollisionFaceInfo*,
                    const Vec3&, IPhysicsNode*);

protected:
    // [CHOSEN] Names describe the deepest contact retained each frame. Terrain/object checks
    // replace it when penetration wins; solveImpulse passes it to handleCollision and resets
    // the contact-depth sentinel to -1.
    /* +0x28 */ int     m_refCount;                  // ctor 1
    /* +0x2C */ unsigned int m_collisionChecked;     // 0
    /* +0x30 */ float   m_contactDepthSq;            // -1.0f = no contact this frame
    /* +0x34 */ IObject* m_contactObject;            // 0 - the other object; 0 for terrain/water
    /* +0x38 */ Vec3    m_contactPoint;              // the hit point (terrain: the ray hit)
    /* +0x44 */ Vec3    m_contactPosition;           // the object's own position at the contact
    /* +0x50 */ Vec3    m_contactSurfaceSpeed;       // Vec3::getZero() for terrain
    /* +0x5C */ Vec3    m_contactSpeed;              // the node's positional speed at the contact
    /* +0x68 */ float   m_contactScale;              // 1.0f on every written path [UNRESOLVED meaning]
    /* +0x6C */ float   m_contactFraction;           // point: the ray's hit fraction; rotational: the depth
    /* +0x70 */ Vec3    m_contactNormal;             // the surface normal at the hit
    /* +0x7C */ int     m_contactOwnMaterial;        // handleCollision's 5th: 0 (point) / getMaterial() (rotational)
    /* +0x80 */ int     m_contactMaterial;           // handleCollision's 6th: the surface material
    /* +0x84 */ bool    m_hasFaceInfo;               // 0 - set by the object path only
    /* +0x88 */ int     m_faceInfo;                  // exact 4-B POD copy; internal two-short layout [UNRESOLVED]
    /* +0x8C */ float   m_underWaterDepth;           // 0 - what setUnderWater receives
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_POINTRESPONSEPHYSICS_H
