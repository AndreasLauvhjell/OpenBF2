// Named collision geometry, material mapping and deferred-load state. Header path [INFERRED].
// The only virtual method is the destructor. ELF x86 instances occupy 56 bytes; the larger
// LOD/BSP records are owned payload, not inline fields of this class.
// create() binds the current object template and mesh directory; load() fills the geometry.
// Unknown fields and inferred load-state names are marked beside their declarations.

#ifndef PHYSICS_COLLISIONMESHTEMPLATE_H     // Guard renamed with the path
#define PHYSICS_COLLISIONMESHTEMPLATE_H

#include <string>
#include <vector>

#include "../Math/Mat4.h"
#include "../Math/Vec3.h"
#include "CollisionGeometry.h"                   // CollisionGeom / CollisionLod / CollisionFace / Bsp

namespace dice {
namespace hfe {

// See Physics/ICollisionMesh.h - the u16 triple attested in getAdjacentCollisionFaceIndices'
// mangling. Forward declarations; neither template is recovered.
template <typename T> struct VectorTraits3;
template <typename T, typename Traits> class BaseVector3;

namespace world {

class CollisionFaceInfo;
class IObjectTemplate;

class CollisionMeshTemplate
{
public:
    CollisionMeshTemplate();

    // Releases debug meshes, raw LOD storage, then the material mapping with scalar delete.
    virtual ~CollisionMeshTemplate();

    // Lazily maps template materials to one u16 id per entry, truncating each id to 16 bits.
    // Called during object precaching through CollisionMesh::createMaterialMapping.
    void createMaterialMapping(const IObjectTemplate* objectTemplate);

    // Loads geometry from stream when fromFile is true; otherwise creates the fixed
    // 17-vertex soldier capsule. owner supplies the material mapping.
    bool load(io::INonlinearStream* stream, const IObjectTemplate* owner, bool fromFile);
    void setVersionToLoad(unsigned int major, unsigned int minor);  // 22 B - the two shorts
    void addVertex(int geom, int lod, const Vec3& v);              // 153 B - push + bbox expand
    void addFace(int geom, int lod, unsigned int a, unsigned int b, unsigned int c,
                 unsigned short material);
    CollisionLod* getValidLod(int geom, int lod);                  // 43 B - the lod slot, no create
    const CollisionLod* getValidLod(int geom, int lod) const;
    void verifyPivot() const;                                       // 5 B - empty, shipped
    void setUseCollisionAsFirstPerson();                            // Drops the first geometry and shifts following entries.
    // Load finalization assigns vertex materials and marks the mesh loaded.
    void giveVerticesMaterial();                                   // Pre-v7 meshes copy each face material to its three vertices.
    void buildDebugMeshes(bool wreck);
    void destroyDebugMeshes(bool wreck);
    void buildDebugMesh(int geom, int lod, bool wreck);
                                                                   // Requires coll-load-debugmeshes to be enabled.

    bool isGeomValid(int geom) const;
    bool isLodValid(int geom, int lod) const;                       // 70 B - the remap test

    // Collision geometry queries and tool operations.
    void buildEdgeInfo();                                          // Builds the collision-layer-1 wall-edge table.
    void addEdgeIndex(int geom, int lod, int index);               // 107 B push one u16
    bool save(io::INonlinearStream* stream);                       // 678 B [FRONTIER] (tool mode)
    bool verify() const;                                           // 107 B every geom has col0
    unsigned int getVertexCount(int geom, int lod) const;
    const Vec3* getVertexVector(int geom, int lod) const;          // 38 B  data pointer / null
    int  getMaterialForVertex(int geom, int lod, int vertex) const;// 96 B  mapped when mapped
    int  getFaceCount(int geom, int lod) const;
    const CollisionFace* getFaceVector(int geom, int lod) const;   // 37 B  data pointer / null
    float getBoundingRadius(int geom, int lod) const;              // 98 B  (lod 4 floors at 0.3)
    const LocalBoundingBox* getBoundingBox(int geom, int lod) const;

    // Queries a ray and writes normal, position, signed plane distance, distance back from
    // the endpoint, and mapped material. rayEnd is rotated as a direction. noFacingFilter
    // skips facing rejection; frontFaces chooses opposing normals; firstHit stops on acceptance.
    bool getDistance(int geom, int lod, bool noFacingFilter,
        const Mat4& objectToWorld, const Mat4& worldToObject,
        const Vec3& rayStart, const Vec3& rayEnd, Vec3& outNormal, Vec3& outPosition,
        float& outPlaneDist, float& outAlong, int* outMaterial, CollisionFaceInfo* faceInfo,
        bool frontFaces, bool firstHit) const;
    // Queries a swept sphere and appends contact outputs. secondary is unused.
    // [PLATFORM] PE clamps the remapped LOD slot; ELF uses the unchecked getValidLod helper.
    bool getDistanceToSphere(int geom, int lod, bool secondary,
        const Mat4& objectToWorld, const Mat4& worldToObject,
        const Vec3& center, const Vec3& velocity, float radius,
        std::vector<Vec3>& outNormals, std::vector<Vec3>& outPositions,
        std::vector<float>& outDistancesA, std::vector<float>& outDistancesB,
        std::vector<int>* outMaterials) const;               // returns "any hit" (the ELF's v214)
    // Returns whether any capsule hits. Vector outputs are normals then positions;
    // the per-face helper takes position then normal.
    bool getDistanceToCapsules(int geom, int lod,
        const Mat4& objectToWorld, const Mat4& worldToObject,
        const Vec3& p0, const Vec3& axis, float radius,
        std::vector<Vec3>& outNormals, std::vector<Vec3>& outPositions,
        std::vector<float>& outDistances) const;
    void drawDebugMesh(int geom, int lod, const Mat4& transform,
                       bool a, bool b, bool c);
    int  getEdgeIndexVectorSize(int geom, int lod) const;
    const unsigned short* getEdgeIndexVector(int geom, int lod) const;   // 41 B data ptr / null
    const CollisionLod* getValidLodRaw(int geom, int lod) const;   // 45 B  raw slot, checked
    CollisionLod* getValidLodRaw(int geom, int lod);
    float getMaxBoundingRadius() const;                            // Returns the first valid LOD radius, not the largest radius.
    bool getBoundingBoxMinMax(Vec3& outMin, Vec3& outMax) const;   // 374 B union; "found any"
    bool sameInstance(int geomA, int lodA, int geomB, int lodB) const;   // 175 B (raw index)
    bool getAdjacentCollisionFaceIndices(int geom, int lod, unsigned int face,
        BaseVector3<unsigned short, VectorTraits3<unsigned short> >& out) const;

    // the class static the volume queries gather into (the TU's global-constructor
    // symbol is keyed to it: _GLOBAL__I__ZN...CollisionMeshTemplate16m_temporaryFacesE).
    static std::vector<const BspNode::Face*> m_temporaryFaces;

    // Per-geometry LOD vectors and five-entry collision-layer remaps.
    std::vector<CollisionGeom> m_lods;         // +0x04..+0x0F
    int             m_field10;                 // +0x10 [UNRESOLVED]
    unsigned short* m_materialIndex;           // +0x14 Owned array of material ids.
    int             m_materialCount;           // +0x18 - [UNRESOLVED]
                                               //   createMaterialMapping writes the entry count
    int             m_field1C;                 // +0x1C [UNRESOLVED]
    std::string     m_directory;               // +0x20 - "<workingPath>Meshes/"
    std::string     m_name;                    // +0x24
    IObjectTemplate* m_ownerTemplate;          // +0x28 - the active template at create() time
    unsigned char   m_loaded;                  // +0x2C [CHOSEN name] Both loader arms set this before geometry access.
    unsigned char   m_loadRequested;           // +0x2D [CHOSEN name] getCollisionMeshPart marks each returned handle requested.
    int             m_dataSize;                // +0x30 [CHOSEN name], see the layout block
    short           m_versionToLoad;           // +0x34
    short           m_versionToLoad2;          // +0x36
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_COLLISIONMESHTEMPLATE_H
