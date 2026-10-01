// Reference-counted per-object collision handle sharing a CollisionMeshTemplate.
// Geometry 0 is normal and geometry 1 is the wreck. The first selection is applied on demand;
// later changes use weak references in the post-frame queue so geometry never changes mid-frame.

#ifndef PHYSICS_COLLISIONMESH_H     // [INFERRED] guard style
#define PHYSICS_COLLISIONMESH_H

#include "ICollisionMesh.h"
#include "CollisionMeshTemplate.h"

namespace dice {
namespace hfe {
namespace world {

class CollisionMesh : public ICollisionMesh
{
public:
    // 0x839DE80 (C2) / 0x839DED0 (C1), 72 B - the real constructor: refcount 1, selection pending.
    explicit CollisionMesh(CollisionMeshTemplate* meshTemplate);

    // the default ctor is A shipped guard: Debug type 6, process-fatal,
    // "Need collsion mesh template as argument" (the "collsion" typo is the binary's) at
    // Physics/CollisionMesh.cpp:31 - the line the attestation comes from.
    CollisionMesh();

    virtual ~CollisionMesh();                        // 27 B (D1) / 40 B (D0) - vptr + base only

    // The IBase quartet, refcount at +0x08
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();                         // 57 B - delete this at zero (vcall D0)
    virtual void* queryInterface(unsigned int iid) const;   // 32 B - IBase / ICollisionMesh only

    virtual void reset();                            // 31 B - selection back to pending + normal
    // getWeakPtr: not overridden - slot 7 stays ICollisionMesh::getWeakPtr, per the vtable.
    virtual CollisionMeshTemplate* getTemplate() const;
    virtual std::string getPathAndName() const;      // 54 B - template directory + name (sret)
    virtual void createMaterialMapping(const IObjectTemplate* objectTemplate);  // 18 B - forwards
                                                     //   to the template (the precache-path slot)
    virtual bool hasLod(int lod);
    virtual bool hasWreckGeom() const;
    virtual bool setGeometry(bool wreckGeom);        // 156 B - the machine above
    virtual void executeSetGeometry(bool);           // 40 B - the commit (param ignored, as shipped)
    virtual bool getAdjacentCollisionFaceIndices(int geometry, int lod, unsigned int face,
        BaseVector3<unsigned short, VectorTraits3<unsigned short> >&) const;    // 55 B - bool, see ICollisionMesh.h
    virtual unsigned int getVertexCount(int lod);
    virtual const void* getVertexVector(int lod);
    virtual int getMaterialForVertex(int lod, int vertex);
    virtual int getFaceCount(int lod);
    virtual const void* getFaceVector(int lod);
    virtual float getBoundingRadius(int lod);
    virtual const LocalBoundingBox* getBoundingBox(int lod);
    // Ray-query outputs follow ICollisionMesh order: normal, position, plane distance and distance along the ray.
    virtual bool getDistance(int lod, bool noFacingFilter, const Mat4& objectToWorld,
        const Mat4& worldToObject, const Vec3& rayStart, const Vec3& rayEnd,
        Vec3& outNormal, Vec3& outPosition, float& outPlaneDist, float& outAlong,
        int* outMaterial, CollisionFaceInfo* faceInfo,
        bool frontFaces, bool firstHit) const;       // 159 B - stamps faceInfo bytes 0/1
    virtual bool getDistancePeek(int lod, bool noFacingFilter, const Mat4& objectToWorld,
        const Mat4& worldToObject, const Vec3& rayStart, const Vec3& rayEnd,
        Vec3& outNormal, Vec3& outPosition, float& outPlaneDist, float& outAlong,
        int* outMaterial, CollisionFaceInfo* faceInfo,
        bool frontFaces, bool firstHit) const;       // 231 B - queries the pending geometry
    virtual bool getDistanceToSphere(int lod, bool secondary, const Mat4& objectToWorld,
        const Mat4& worldToObject, const Vec3& center, const Vec3& velocity, float radius,
        std::vector<Vec3>& outNormals, std::vector<Vec3>& outPositions,
        std::vector<float>& outDistancesA, std::vector<float>& outDistancesB,
        std::vector<int>* outMaterials) const;       // 117 B - propagates the template's bool
    virtual bool getDistanceToCapsules(int lod, const Mat4& objectToWorld,
        const Mat4& worldToObject, const Vec3& p0, const Vec3& axis, float radius,
        std::vector<Vec3>& outNormals, std::vector<Vec3>& outPositions,
        std::vector<float>& outDistances) const;     // 97 B  - propagates the template's bool
    virtual void drawDebugMesh(bool wreckGeom, int lod, const Mat4& transform,
                               bool a, bool b, bool c);
    virtual int getEdgeIndexVectorSize(int lod) const;
    virtual const void* getEdgeIndexVector(int lod) const;
    virtual float getMaxBoundingRadius() const;
    virtual bool getBoundingBoxMinMax(Vec3& outMin, Vec3& outMax) const;
    virtual bool sameInstance(int, int, int, int) const;     // 55 B - self-recursive, see .cpp

    // Non-virtual (appears in no vtable): the template's geom-vector size, 32-byte elements.
    int getGeometryCount() const;

private:
    // layout +0x08..+0x23 (ICollisionMesh supplies +0x00 vptr and +0x04 weak-control). Every
    // offset from the 72-byte constructor's raw disassembly - Hex-Rays had swapped two of them.
    /* 0x08 */ int   m_refCount;                     // ctor 1
    /* 0x0C */ CollisionMeshTemplate* m_template;    // ctor = the argument (never null on the real
                                                     //   ctor path; the default ctor is the fatal)
    /* 0x10 */ unsigned char m_geometrySelectionPending;  // ctor 1
    /* 0x11 */ char  _pad11[3];
    /* 0x14 */ int   m_geometry;                     // ctor 0 - the applied geom index
    /* 0x18 */ unsigned char m_geometryChangeQueued; // ctor 0
    /* 0x19 */ char  _pad19[3];
    /* 0x1C */ int   m_requestedGeometry;            // ctor 0 - the selected geom index
    /* 0x20 */ unsigned char m_inPostFrameUpdateList;// ctor 0
    /* 0x21 */ char  _pad21[3];
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_COLLISIONMESH_H
