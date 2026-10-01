// Per-object handle interface over shared collision-template geometry; header path [INFERRED].
// Preserve the observed 33-slot order. Parameter types are attested; names are [CHOSEN].
// Unproven return types remain [CHOSEN, not recovered] and affect MSVC symbol linkage.

#ifndef PHYSICS_ICOLLISIONMESH_H     // [INFERRED] guard style
#define PHYSICS_ICOLLISIONMESH_H

#include <string>
#include <vector>

#include "../Core/IBase.h"
#include "../Core/WeakPtr.h"
#include "../Math/Mat4.h"
#include "../Math/Vec3.h"

namespace dice {
namespace hfe {

struct LocalBoundingBox;

// Three unsigned-short face indices passed by reference; template declarations suffice here.
template <typename T> struct VectorTraits3;
template <typename T, typename Traits> class BaseVector3;

namespace world {

class CollisionFaceInfo;                 // still no recovered layout - see getDistance's note
class CollisionMeshTemplate;
class IObjectTemplate;

// 33 slots, in vtable order. The interface's own vtable (0x88CD2E8) exists because two slots are
// concrete here rather than pure: the destructor and getWeakPtr - the weak-handle machinery that
// CollisionManager's post-frame-update list depends on (Core/WeakPtr.h; control block: refCount
// then target, target cleared by ~ICollisionMesh, block freed by the last handle).

// the destructor is first after the IBase quartet. Slots +0x10/+0x14. Moving it moves 27 slots.
class ICollisionMesh : public IBase
{
public:
    ICollisionMesh();                                    // zeroes the control ptr
    virtual ~ICollisionMesh();                           // slots 4/5 - clears + releases the block

    virtual void reset()                                                    = 0;   // slot 6
    virtual WeakPtr<ICollisionMesh> getWeakPtr();        // slot 7 - 0x835BC40, 74 B (read):
                                                         //   4-byte handle by value (sret; the
                                                         //   IObject.h +0x1C trap, same shape).
                                                         //   Creates the block lazily.
    virtual CollisionMeshTemplate* getTemplate() const                      = 0;   // slot 8  (read)
    virtual std::string getPathAndName() const                              = 0;   // slot 9  (read:
                                                         //   sret - template directory + name)
    virtual void createMaterialMapping(const IObjectTemplate* objectTemplate) = 0; // slot 10 - the
                                                         //   slot SimpleObject/Bundle ctors reach
                                                         //   through their raw-vslot shim (MSVC
                                                         //   byte +0x24 - arithmetic re-verified
                                                         //   against this declaration order)
    virtual bool hasLod(int lod)                                            = 0;   // slot 11 (read)
    virtual bool hasWreckGeom() const                                       = 0;   // slot 12 (read:
                                                         //   geometry count > 1)
    virtual bool setGeometry(bool wreckGeom)                                = 0;   // slot 13 (read:
                                                         //   false = refused, see concrete body)
    virtual void executeSetGeometry(bool)                                   = 0;   // The bool argument is unused.
    // Returns the three neighbors of a face in the selected geometry/LOD, or false when
    // that LOD has no adjacency table.
    virtual bool getAdjacentCollisionFaceIndices(int geometry, int lod, unsigned int face,
        BaseVector3<unsigned short, VectorTraits3<unsigned short> >& out) const = 0;   // slot 15
    virtual unsigned int getVertexCount(int lod)                            = 0;   // slot 16 (read)
    virtual const void* getVertexVector(int lod)                            = 0;   // slot 17
                                                         //   [CHOSEN] pointer-shaped; pointee
                                                         //   unread (the template Lod interior)
    virtual int getMaterialForVertex(int lod, int vertex)                   = 0;   // slot 18
    virtual int getFaceCount(int lod)                                       = 0;   // slot 19
    virtual const void* getFaceVector(int lod)                              = 0;   // slot 20 [CHOSEN]
    virtual float getBoundingRadius(int lod)                                = 0;   // slot 21
    // Return type materialised by DefaultDetonationComp::checkIfObjectCanTrigger in both shipped
    // ports: the caller reads LocalBoundingBox's dirty byte and derived size directly.
    virtual const LocalBoundingBox* getBoundingBox(int lod)                 = 0;   // slot 22 (read)

    // The distance/query family. Parameter lists are the manglings', verbatim.
    // names (the collision query sitting read the template bodies): the two
    // Vec3& outs are (normal, position) in that order; `secondary` is BspV8's noFacingFilter; the
    // matrices are objectToWorld / worldToObject; the floats are the end point's signed plane
    // distance and the along-ray distance from the end back to the hit (both <= 0); `outIndex`
    // is the hit face's material id; the trailing bools are frontFaces / firstHit. See
    // Physics/CollisionMeshTemplate.h's getDistance block for the mapping from the old names.
    virtual bool getDistance(int lod, bool noFacingFilter, const Mat4& objectToWorld,
        const Mat4& worldToObject, const Vec3& rayStart, const Vec3& rayEnd,
        Vec3& outNormal, Vec3& outPosition, float& outPlaneDist, float& outAlong,
        int* outMaterial, CollisionFaceInfo* faceInfo,
        bool frontFaces, bool firstHit) const                               = 0;   // slot 23 (read:
                                                         //   bool, and the concrete body stamps
                                                         //   faceInfo bytes 0/1 - see the .cpp)
    virtual bool getDistancePeek(int lod, bool noFacingFilter, const Mat4& objectToWorld,
        const Mat4& worldToObject, const Vec3& rayStart, const Vec3& rayEnd,
        Vec3& outNormal, Vec3& outPosition, float& outPlaneDist, float& outAlong,
        int* outMaterial, CollisionFaceInfo* faceInfo,
        bool frontFaces, bool firstHit) const                               = 0;   // slot 24 (read)
    // The two volume queries return the template's "any hit" (the delegates propagate eax; the
    // earlier void was [CHOSEN]).
    virtual bool getDistanceToSphere(int lod, bool secondary, const Mat4& objectToWorld,
        const Mat4& worldToObject, const Vec3& center, const Vec3& velocity, float radius,
        std::vector<Vec3>& outNormals, std::vector<Vec3>& outPositions,
        std::vector<float>& outDistancesA, std::vector<float>& outDistancesB,
        std::vector<int>* outMaterials) const                               = 0;   // slot 25
    virtual bool getDistanceToCapsules(int lod, const Mat4& objectToWorld,
        const Mat4& worldToObject, const Vec3& p0, const Vec3& axis, float radius,
        std::vector<Vec3>& outNormals, std::vector<Vec3>& outPositions,
        std::vector<float>& outDistances) const                             = 0;   // slot 26

    virtual void drawDebugMesh(bool wreckGeom, int lod, const Mat4& transform,
                               bool a, bool b, bool c)                      = 0;   // slot 27
    virtual int getEdgeIndexVectorSize(int lod) const                       = 0;   // slot 28
    virtual const void* getEdgeIndexVector(int lod) const                   = 0;   // slot 29 [CHOSEN]
    virtual float getMaxBoundingRadius() const                              = 0;   // slot 30
    // Returns whether any valid LOD contributed bounds; leaves outputs unchanged on failure.
    virtual bool getBoundingBoxMinMax(Vec3& outMin, Vec3& outMax) const     = 0;   // slot 31
    virtual bool sameInstance(int, int, int, int) const                     = 0;   // slot 32 - see
                                                         //   the concrete body's recursion note

protected:
    // +0x04. The one-word weak-handle control block (Core/WeakPtr.h). Zeroed by the ctor; cleared
    // and released by the dtor; created lazily by getWeakPtr. sizeof(ICollisionMesh) == 8.
    WeakPtr<ICollisionMesh>::Ctrl* m_weakControl;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_ICOLLISIONMESH_H
