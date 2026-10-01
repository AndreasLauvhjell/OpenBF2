// Plane representation and transform operations. [INFERRED] Source placement.


#ifndef MATH_PLANE3_H     // [INFERRED] guard style
#define MATH_PLANE3_H

#include "Vec3.h"

namespace dice {
namespace hfe {

struct Mat4;

struct Plane3
{
    Vec3  normal;                                              // +0x00
    float d;                                                   // +0x0C

    // static, by value.
    static Plane3 transform(const Plane3& plane, const Mat4& m);
    // projectVector: engine [FRONTIER].
    // intersectLine: aidll / - written.
    void projectVector(const Vec3& v, Vec3& out);
    bool intersectLine(const Vec3& a, const Vec3& b, Vec3& out, bool& insideSegment);
};

// the point common to three planes (Frustum::recalcCorners' corners).
Vec3 intersect(const Plane3& p1, const Plane3& p2, const Plane3& p3);

} // namespace hfe
} // namespace dice

#endif // MATH_PLANE3_H
