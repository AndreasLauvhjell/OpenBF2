// The source path is attested by the ELF anonymous namespace of planeBoxOverlap.
#ifndef MATH_TRIANGLETOAABBINTERSECTION_H
#define MATH_TRIANGLETOAABBINTERSECTION_H

#include "Vec3.h"

namespace dice { namespace hfe {

// Read-only pointer arguments retain the original non-const signature.
bool triangleToAABBIntersection(float* center, float* halfSize, float (*triangle)[3]);

// Tests three vertices against the supplied box bounds.
bool triangleToAABBIntersection(const Vec3& boxMin, const Vec3& boxMax,
                                const Vec3& v0, const Vec3& v1, const Vec3& v2);

} }

#endif
