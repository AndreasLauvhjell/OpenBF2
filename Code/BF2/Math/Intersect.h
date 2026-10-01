// Intersection helpers for the engine's geometry types. [INFERRED] Source placement.

#ifndef MATH_INTERSECT_H     // [INFERRED] guard style
#define MATH_INTERSECT_H

#include "Vec3.h"

namespace dice {
namespace hfe {

// Sum of squared per-axis distances from the centre to the box <= radius^2. (`>=` on r*r, as
// shipped - a sphere exactly touching counts.)
bool sphereIntersectsAABB(const Vec3& centre, float radius, const Vec3& boxMin, const Vec3& boxMax);

// Slab test. tMin/tMax receive the entry/exit fractions along `dir` even when the function
// returns false (they are written before the final range test, as shipped). 1/dir[i] is formed
// without a zero guard - a zero component gives ±inf slabs, which the comparisons tolerate.
bool rayIntersectsAABB(const Vec3& origin, const Vec3& dir, const Vec3& boxMin, const Vec3& boxMax,
                       float& tMin, float& tMax);

// The same slab test against the box expanded by `radius` on every axis - a swept sphere / thick
// ray. Same tMin/tMax contract.
bool rayRadiusIntersectsAABB(const Vec3& origin, const Vec3& dir, float radius,
                             const Vec3& boxMin, const Vec3& boxMax, float& tMin, float& tMax);

} // namespace hfe
} // namespace dice

#endif // MATH_INTERSECT_H
