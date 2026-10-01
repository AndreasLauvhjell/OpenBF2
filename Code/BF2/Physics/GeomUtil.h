// Primitive collision tests for mesh queries and BSP walkers; header path [INFERRED].
// Tests update shared collision statistics; closest-distance helpers do not.
// checkSphereAndEdgeCollision remains [FRONTIER]; checkFaceAndSphereCollision remains [FRONTIER].

#ifndef PHYSICS_GEOMUTIL_H     // [INFERRED] guard style
#define PHYSICS_GEOMUTIL_H

#include <vector>

#include "../Math/Vec3.h"

namespace dice {
namespace hfe {

struct Mat4;

namespace world {

// ELF 0x83A9830 / PE 0x6B5B20: point-to-segment distance; no zero-length guard in retail.
float getClosestDistanceToLine(const Vec3& point, const Vec3* start, const Vec3* end);

// Tests segment p0->p1 against the triangle plane and projected edges; false on no accepted hit.
// Both sidedness modes require a front-to-back plane crossing. twoSided additionally admits coplanar
// segments, including the resulting 0/0 interpolation. Degenerate segments are rejected.
// outPlaneDist is the endpoint's signed plane distance; outAlong is signed distance from the endpoint
// back to outHit. unusedTolerance is ignored.
bool checkFaceAndEdgeCollision(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& normal,
                               const Vec3& p0, const Vec3& p1, Vec3& outHit, float& outPlaneDist,
                               float& outAlong, bool twoSided, float unusedTolerance);

// checkFaceAndEdgeCollision(..., true, 0.0f).
bool checkFaceAndEdgeCollisionDoubleSided(const Vec3& v0, const Vec3& v1, const Vec3& v2,
                                          const Vec3& normal, const Vec3& p0, const Vec3& p1,
                                          Vec3& outHit, float& outPlaneDist, float& outAlong);

// The exported helper closure used by the swept-sphere edge fallback. These are external world
// symbols in the ELF, not TU-local implementation details.
float getClosestDistanceBetweenLines(const Vec3&, const Vec3&, const Vec3&, const Vec3&,
                                     float&, float&);
void generateOrthonormalBasis(Vec3&, Vec3&, Vec3&, bool);
int getIntersectionOfCapsAndEdgeInternal(const Vec3&, const Vec3&, const Vec3&, const Vec3&,
                                         float, float*);
int getIntersectionOfCapsAndEdgeNew(const Vec3&, const Vec3&, const Vec3&, const Vec3&,
                                    float, float*, Vec3*);
unsigned int classifyPointToTriEdges(const Vec3**, const Vec3&, const Vec3&);
bool checkSphereLineCollision(const Vec3&, float, const Vec3*, const Vec3*, Vec3&, Vec3&,
                              float&, int&);
bool checkSpherePointCollision(const Vec3&, float, const Vec3*, Vec3&, Vec3&, float&);

// The volume tests - declared from their manglings
bool checkSphereAndEdgeCollision(const Vec3&, float, const Vec3&, const Vec3&, std::vector<float>&);
bool capsuleVsCapsuleCollision(const Vec3&, const Vec3&, const Vec3&, const Vec3&, float,
                               Vec3&, Vec3&, float&);
bool checkFaceAndSphereCollision(const Vec3*, const Vec3*, const Vec3*, const Vec3&, const Vec3&,
                                 float, Vec3&, float&, float&, bool);
bool checkShiftedFaceCollision(const Vec3&, const Vec3&, const Vec3&, const Vec3&, const Vec3&,
                               const Vec3&, float, Vec3&, float&, float&, const Mat4&);
bool checkCapsuleEdgesCollision(const Vec3& v0, const Vec3& v1, const Vec3& v2,
                                const Vec3& faceNormal, const Vec3& start, const Vec3& end,
                                float radius, Vec3& outPosition, Vec3& outNormal,
                                float& outDistance);
bool checkSphereTriCollision(const Vec3**, const Vec3&, const Vec3&, float, Vec3&, Vec3&, float&);
bool checkExpandedEdgesCollision(unsigned int, const Vec3&, const Vec3&, const Vec3&, const Vec3&,
                                 const Vec3&, const Vec3&, float, Vec3&, Vec3&, float&, float&,
                                 const Mat4&);

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_GEOMUTIL_H
