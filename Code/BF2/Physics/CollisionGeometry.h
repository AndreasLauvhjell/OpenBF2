// Collision geometry records, split into this header as [CHOSEN, not recovered] organization.
// Each Geom contains per-collision-layer LOD pointers and a five-entry remap; -1 marks an absent layer.
// The loader falls back from layer 2 to 1, then layer 3 to 2. Preserve that order so projectile-only
// geometry does not acquire vehicle/soldier collision layers.

#ifndef PHYSICS_COLLISIONGEOMETRY_H     // [INFERRED] guard style
#define PHYSICS_COLLISIONGEOMETRY_H

#include <vector>

#include "../Core/IBase.h"
#include "../Math/LocalBoundingBox.h"
#include "../Math/Vec3.h"
#include "Bsp.h"                                 // Bsp / BspNode::Face / BspV8 - the query trees

namespace dice {
namespace hfe {

namespace io { class IStream; class INonlinearStream; }

namespace world {

// 8 bytes, attested by addFace's (short, short, short, ushort) store and the loader's `8 * n`
// stride: three vertex indices and a material.
struct CollisionFace
{
    short v0, v1, v2;
    unsigned short material;
};

// One collision layer's geometry, vertices, bounds, BSP, and debug resources.
struct CollisionLod
{
    /* 0x00 */ std::vector<CollisionFace> faces;         // 8-byte elements
    /* 0x0C */ std::vector<Vec3> vertices;               // (IDB: vertexVec3End/vertexVec3 - the
                                                         //   IDB had begin/end mislabelled; the
                                                         //   loader's `12 * n` read pins it)
    /* 0x18 */ Bsp bsp;                                  // 0x54
    /* 0x6C */ LocalBoundingBox boundingBox;             // 0x3C - the loader reads 24 bytes
                                                         //   (min+max) into it then calcRadius;
                                                         //   its dirty byte is the ctor's
                                                         //   `gap7C[16] = 1`
    /* 0xA8 */ float maxBoundingRadius;                  // Set from boundingBox.m_radiusFromOrigin after load.
// [RETIRE-WHEN: COLLISION-DEBUG-GEOMETRY-TYPE] Replace these IBase prefixes with the
// complete geometry interface when available. Their release slot is confirmed on both ports.
    /* 0xAC */ IBase* debugMesh;                        // [INFERRED name], ctor null
    /* 0xB0 */ IBase* dwordB0;
    /* 0xB4 */ IBase* dwordB4;                          // [UNRESOLVED descriptive names]
    /* 0xB8 */ unsigned short* faceAdjacency;            // (IDB: someBuffer) v10+: three u16 per
                                                         //   face - the neighbour across each edge
                                                         //   (getAdjacentCollisionFaceIndices reads
                                                         //   [3*face + i]); -1 = none. Read from
                                                         //   the file as u32s, range-checked
                                                         //   (abort()) and narrowed. Was named
                                                         //   "aiNavRemap" until the query sitting
                                                         //   read its only consumer.
    /* 0xBC */ std::vector<unsigned short> materials;    // (IDB: materials/gapC0) v7+: one per
                                                         //   vertex, 2-byte elements
    /* 0xC8 */ std::vector<unsigned short> edgeIndices;  // (IDB: edgeIndexVector) buildEdgeInfo
                                                         //   fills: vertex-index pairs (v0,v1,
                                                         //   v1,v2, v2,v0) of every col1 face
                                                         //   steeper than the edge threshold
    CollisionLod();
    ~CollisionLod(); // ELF inlined in template D1 / PE 0x6BFAA0: adjacency, then members
};

// 32 bytes: the per-geometry Lod list plus the col-layer remap.
struct CollisionGeom
{
    /* 0x00 */ std::vector<CollisionLod*> lods;
    /* 0x0C */ int colRemap[5];                          // isLodValid's table; -1 = absent
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_COLLISIONGEOMETRY_H
