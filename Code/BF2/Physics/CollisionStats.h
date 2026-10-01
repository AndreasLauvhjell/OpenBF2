// Shared collision counters; [CHOSEN, not recovered] header organization.
// The global pointer references fifteen dwords cleared at frame start. Keep [UNRESOLVED] counters
// in place to preserve the 60-byte block. All users must share this declared type
// for consistent MSVC linkage.

#ifndef PHYSICS_COLLISIONSTATS_H     // [CHOSEN] guard style
#define PHYSICS_COLLISIONSTATS_H

namespace dice {
namespace hfe {
namespace world {

struct CollisionStatsData                       // 0x3C - size recovered; field names [CHOSEN]
{
    int unknown00;                              // +0x00 [UNRESOLVED]
    int unknown04;                              // +0x04 [UNRESOLVED]
    int unknown08;                              // +0x08 [UNRESOLVED]
    int heightAndNormalQueries;                 // +0x0C attested: HeightmapCluster::
                                                //       getHeightAndNormalInWorldCoords 0x8385726
    int unknown10;                              // +0x10 [UNRESOLVED]
    int unknown14;                              // +0x14 [UNRESOLVED]
    int unknown18;                              // +0x18 [UNRESOLVED]
    int unknown1C;                              // +0x1C [UNRESOLVED]
    int unknown20;                              // +0x20 [UNRESOLVED]
    int unknown24;                              // +0x24 [UNRESOLVED]
    int faceAndEdgeChecks;                      // +0x28 attested: checkFaceAndEdgeCollision
                                                //       0x83A9C79 (`inc [edx+28h]`)
    int sphereChecks;                           // +0x2C attested: checkSphereAndEdgeCollision
                                                //       0x83AA229 and checkSphereTriCollision
                                                //       both bump this one
    int unknown30;                              // +0x30 [UNRESOLVED]
    int unknown34;                              // +0x34 [UNRESOLVED]
    int shiftedFaceChecks;                      // +0x38 attested: checkShiftedFaceCollision

};

extern CollisionStatsData  collisionStatsData_internal;    // bss, 0x3C
extern CollisionStatsData* collisionStatsData;             // ds:0x898BA4C -> the block above

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_COLLISIONSTATS_H
