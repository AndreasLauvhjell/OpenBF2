// Path:     [INFERRED] beside the other dice::hfe math value types.
// Contents: minimal recovered three-dimensional ray value type.

#ifndef MATH_RAY3_H
#define MATH_RAY3_H

#include "Vec3.h"

namespace dice {
namespace hfe {

// Layout proven by findIntersection(Plane3 const&, Ray3 const&, Vec3&) in both shipped ports:
// the first Vec3 is the ray origin and the second is its (not necessarily normalised) direction.
// Member spelling remains [INFERRED].
struct Ray3
{
    Vec3 origin;       // +0x00
    Vec3 direction;    // +0x0C
};

static_assert(sizeof(Ray3) == 0x18, "Ray3 layout drift");

} // namespace hfe
} // namespace dice

#endif // MATH_RAY3_H

