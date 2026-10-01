// Collision-mesh handle implementation. [FRONTIER] Remaining geometry operations
// belong to CollisionMeshTemplate; this wrapper does not replace them.

#include "CollisionMesh.h"

#include "../Core/Debug.h"
#include "../Core/Ids.h"
#include "CollisionManager.h"                 // collisionManager - the post-frame-update list

namespace dice {
namespace hfe {
namespace world {

extern ICollisionManager* collisionManager;   // ds:0x8B27840, Core/Globals.cpp

// Default collision-interface lifecycle. [UNRESOLVED] Original source placement is unknown.
ICollisionMesh::ICollisionMesh()
{
    m_weakControl = 0;
}

// Clear the control block's target so surviving weak handles read null, then drop the mesh's own
// reference on the block - Core/WeakPtr.h's exact discipline.
ICollisionMesh::~ICollisionMesh()
{
    if (m_weakControl)
    {
        m_weakControl->target = 0;
        if (--m_weakControl->refCount == 0)
            ::operator delete(m_weakControl);
    }
}

// The 4-byte handle by value (sret). The block is created lazily with refCount 1
// (the mesh's own reference); every handed-out handle adds one more. PE 0x6C27F0 agrees:
// the block holds an int followed by the target pointer. Its retail x86 size is 8; allocate
// the typed size so the native x64 pointer and its alignment fit too (16 bytes).
WeakPtr<ICollisionMesh> ICollisionMesh::getWeakPtr()
{
    if (!m_weakControl)
    {
        m_weakControl = static_cast<WeakPtr<ICollisionMesh>::Ctrl*>(
            ::operator new(sizeof(WeakPtr<ICollisionMesh>::Ctrl)));
        m_weakControl->refCount = 1;
        m_weakControl->target = this;
    }
    WeakPtr<ICollisionMesh> handle;
    handle.m_ctrl = m_weakControl;
    ++m_weakControl->refCount;
    return handle;
}

// The constructors - 0x839DE80 (real, 72 B) and 0x839DC60 (default, 270 B), both read end to end.
CollisionMesh::CollisionMesh(CollisionMeshTemplate* meshTemplate)
{
    m_refCount = 1;                              // +0x08
    m_template = meshTemplate;                   // +0x0C
    m_geometrySelectionPending = 1;              // +0x10
    m_geometry = 0;                              // +0x14
    m_geometryChangeQueued = 0;                  // +0x18
    m_requestedGeometry = 0;                     // +0x1C
    m_inPostFrameUpdateList = 0;                 // +0x20
}

// A shipped guard, process-fatal: constructing a CollisionMesh without a template is Debug
// type 6. The message's "collsion" typo is the binary's - preserved. Line 31 of this file is the
// attestation anchor; the ctor leaves every field but the refcount alone, as shipped.
CollisionMesh::CollisionMesh()
{
    m_refCount = 1;
    Debug(DEBUGTYPE_6, "Physics/CollisionMesh.cpp", 31, __FUNCTION__)
        << "Need collsion mesh template as argument";
}

CollisionMesh::~CollisionMesh() {}

void CollisionMesh::addRef()  { ++m_refCount; }
int  CollisionMesh::getRef()  { return m_refCount; }
int  CollisionMesh::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

void* CollisionMesh::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)           return const_cast<CollisionMesh*>(this);
    if (iid == IID_ICollisionMesh)  return const_cast<CollisionMesh*>(this);
    return 0;
}

// back to "selection pending", then force the normal geometry through the
// vtable (slot 13), exactly the lazy-apply the gated delegators use.
void CollisionMesh::reset()
{
    m_geometrySelectionPending = 1;
    setGeometry(false);
}

CollisionMeshTemplate* CollisionMesh::getTemplate() const { return m_template; }

// (sret) - the template's search directory + mesh name, one operator+.
std::string CollisionMesh::getPathAndName() const
{
    return m_template->m_directory + m_template->m_name;
}

// the slot the SimpleObject/Bundle ctor shims dispatch during precache.
void CollisionMesh::createMaterialMapping(const IObjectTemplate* objectTemplate)
{
    m_template->createMaterialMapping(objectTemplate);
}

// Gate on the template actually holding data, lazy-apply, then ask the template.
bool CollisionMesh::hasLod(int lod)
{
    if (!m_template->m_loaded)
        return false;
    if (m_geometrySelectionPending == 1)
        setGeometry(false);                      // vcall slot 13, as shipped
    return m_template->isLodValid(m_geometry, lod);
}

bool CollisionMesh::hasWreckGeom() const
{
    return getGeometryCount() > 1;
}

// the selection machine (header). Returns false only when a wreck geometry is
// requested and the template has none; an unloaded template accepts everything and does nothing.
bool CollisionMesh::setGeometry(bool wreckGeom)
{
    if (!m_template->m_loaded)
        return true;

    if (getGeometryCount() <= 1 && wreckGeom)
        return false;

    if (static_cast<int>(wreckGeom) == m_requestedGeometry)
    {
        m_geometrySelectionPending = 0;
    }
    else
    {
        const bool wasPending = (m_geometrySelectionPending == 1);
        m_geometryChangeQueued = 1;
        m_requestedGeometry = wreckGeom;
        if (wasPending)
        {
            executeSetGeometry(true);            // vcall slot 14 - apply immediately
        }
        else if (!m_inPostFrameUpdateList)
        {
            collisionManager->addToPostFrameUpdate(this);   // weak handle; commit at frame end
            m_inPostFrameUpdateList = 1;
        }
    }
    return true;
}

// the commit. The bool parameter is ignored, as shipped.
void CollisionMesh::executeSetGeometry(bool)
{
    if (m_geometryChangeQueued)
    {
        m_geometryChangeQueued = 0;
        m_geometry = m_requestedGeometry;
        m_geometrySelectionPending = 0;
        m_inPostFrameUpdateList = 0;
    }
}

// Forwards geometry, LOD, face, and output to the template and returns its adjacency result.
bool CollisionMesh::getAdjacentCollisionFaceIndices(int geometry, int lod, unsigned int face,
    BaseVector3<unsigned short, VectorTraits3<unsigned short> >& out) const
{
    return m_template->getAdjacentCollisionFaceIndices(geometry, lod, face, out);
}

// The geometry-prefixed delegators. Each lazy-applies the pending selection first.
unsigned int CollisionMesh::getVertexCount(int lod)
{
    if (m_geometrySelectionPending == 1) setGeometry(false);
    return m_template->getVertexCount(m_geometry, lod);
}

const void* CollisionMesh::getVertexVector(int lod)
{
    if (m_geometrySelectionPending == 1) setGeometry(false);
    return m_template->getVertexVector(m_geometry, lod);
}

int CollisionMesh::getMaterialForVertex(int lod, int vertex)
{
    if (m_geometrySelectionPending == 1) setGeometry(false);
    return m_template->getMaterialForVertex(m_geometry, lod, vertex);
}

int CollisionMesh::getFaceCount(int lod)
{
    if (m_geometrySelectionPending == 1) setGeometry(false);
    return m_template->getFaceCount(m_geometry, lod);
}

const void* CollisionMesh::getFaceVector(int lod)
{
    if (m_geometrySelectionPending == 1) setGeometry(false);
    return m_template->getFaceVector(m_geometry, lod);
}

float CollisionMesh::getBoundingRadius(int lod)
{
    if (m_geometrySelectionPending == 1) setGeometry(false);
    return m_template->getBoundingRadius(m_geometry, lod);
}

const LocalBoundingBox* CollisionMesh::getBoundingBox(int lod)
{
    if (m_geometrySelectionPending == 1) setGeometry(false);
    return m_template->getBoundingBox(m_geometry, lod);
}

// Queries the applied geometry and stamps face information with LOD and geometry bytes.
// Does not apply pending geometry changes before querying.
bool CollisionMesh::getDistance(int lod, bool noFacingFilter, const Mat4& objectToWorld,
    const Mat4& worldToObject, const Vec3& rayStart, const Vec3& rayEnd,
    Vec3& outNormal, Vec3& outPosition, float& outPlaneDist, float& outAlong,
    int* outMaterial, CollisionFaceInfo* faceInfo, bool frontFaces, bool firstHit) const
{
    const bool hit = m_template->getDistance(m_geometry, lod, noFacingFilter, objectToWorld,
                                             worldToObject, rayStart, rayEnd, outNormal,
                                             outPosition, outPlaneDist, outAlong, outMaterial,
                                             faceInfo, frontFaces, firstHit);
    if (faceInfo)
    {
        reinterpret_cast<unsigned char*>(faceInfo)[0] = static_cast<unsigned char>(lod);
        reinterpret_cast<unsigned char*>(faceInfo)[1] = static_cast<unsigned char>(m_geometry);
    }
    return hit;
}

// Queries requested geometry before a queued switch is applied, provided that LOD exists.
// Face information is still stamped with the live geometry, preserving the mismatch.
bool CollisionMesh::getDistancePeek(int lod, bool noFacingFilter, const Mat4& objectToWorld,
    const Mat4& worldToObject, const Vec3& rayStart, const Vec3& rayEnd,
    Vec3& outNormal, Vec3& outPosition, float& outPlaneDist, float& outAlong,
    int* outMaterial, CollisionFaceInfo* faceInfo, bool frontFaces, bool firstHit) const
{
    if (!m_template->isLodValid(m_requestedGeometry, lod))
        return false;

    const bool hit = m_template->getDistance(m_requestedGeometry, lod, noFacingFilter,
                                             objectToWorld, worldToObject, rayStart, rayEnd,
                                             outNormal, outPosition, outPlaneDist, outAlong,
                                             outMaterial, faceInfo, frontFaces, firstHit);
    if (faceInfo)
    {
        reinterpret_cast<unsigned char*>(faceInfo)[0] = static_cast<unsigned char>(lod);
        reinterpret_cast<unsigned char*>(faceInfo)[1] = static_cast<unsigned char>(m_geometry);
    }
    return hit;
}

bool CollisionMesh::getDistanceToSphere(int lod, bool secondary, const Mat4& objectToWorld,
    const Mat4& worldToObject, const Vec3& center, const Vec3& velocity, float radius,
    std::vector<Vec3>& outNormals, std::vector<Vec3>& outPositions,
    std::vector<float>& outDistancesA, std::vector<float>& outDistancesB,
    std::vector<int>* outMaterials) const
{
    return m_template->getDistanceToSphere(m_geometry, lod, secondary, objectToWorld,
                                           worldToObject, center, velocity, radius, outNormals,
                                           outPositions, outDistancesA, outDistancesB,
                                           outMaterials);
}

bool CollisionMesh::getDistanceToCapsules(int lod, const Mat4& objectToWorld,
    const Mat4& worldToObject, const Vec3& p0, const Vec3& axis, float radius,
    std::vector<Vec3>& outNormals, std::vector<Vec3>& outPositions,
    std::vector<float>& outDistances) const
{
    return m_template->getDistanceToCapsules(m_geometry, lod, objectToWorld, worldToObject,
                                             p0, axis, radius, outNormals, outPositions,
                                             outDistances);
}

// The bool doubles as the template's geom index (wreck = 1); a single-geometry
// mesh refuses the wreck request by doing nothing.
void CollisionMesh::drawDebugMesh(bool wreckGeom, int lod, const Mat4& transform,
                                  bool a, bool b, bool c)
{
    if (getGeometryCount() > 1 || !wreckGeom)
        m_template->drawDebugMesh(wreckGeom, lod, transform, a, b, c);
}

int CollisionMesh::getEdgeIndexVectorSize(int lod) const
{
    return m_template->getEdgeIndexVectorSize(m_geometry, lod);
}

const void* CollisionMesh::getEdgeIndexVector(int lod) const
{
    return m_template->getEdgeIndexVector(m_geometry, lod);
}

float CollisionMesh::getMaxBoundingRadius() const
{
    return m_template->getMaxBoundingRadius();
}

bool CollisionMesh::getBoundingBoxMinMax(Vec3& outMin, Vec3& outMax) const
{
    return m_template->getBoundingBoxMinMax(outMin, outMax);
}

// Recurses through this object's own virtual sameInstance slot, matching the shipped
// defect. Do not call this implementation; it does not delegate to the template.
bool CollisionMesh::sameInstance(int a, int b, int c, int d) const
{
    return this->sameInstance(a, b, c, d);       // virtual self-dispatch, as shipped
}

// 22 B, non-virtual: the template's geom vector size (32-byte CollisionGeom elements).
int CollisionMesh::getGeometryCount() const
{
    return static_cast<int>(m_template->m_lods.size());
}

} // namespace world
} // namespace hfe
} // namespace dice
