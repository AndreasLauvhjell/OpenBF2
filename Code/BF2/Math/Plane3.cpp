// Plane transforms and intersection queries.

#include "Plane3.h"
#include "Mat4.h"

namespace dice {
namespace hfe {

// n' = R * n (the 3x3, row-vector form: n'.x = m0*nx + m4*ny + m8*nz ...);
// the plane's point p = -n * d goes through the full affine (R * p + t, t = m12..m14) and
// d' = -(n' . p').
Plane3 Plane3::transform(const Plane3& plane, const Mat4& m)
{
    const float nx = plane.normal.x, ny = plane.normal.y, nz = plane.normal.z;
    const float px = -nx * plane.d, py = -ny * plane.d, pz = -nz * plane.d;   // a point on the plane
    Plane3 r;
    r.normal.x = m.m[4] * ny + m.m[0] * nx + m.m[8] * nz;
    r.normal.y = m.m[9] * nz + (m.m[5] * ny + m.m[1] * nx);
    r.normal.z = nz * m.m[10] + (ny * m.m[6] + nx * m.m[2]);
    r.d = -((pz * m.m[10] + py * m.m[6] + px * m.m[2] + m.m[14]) * r.normal.z
          + (m.m[9] * pz + m.m[5] * py + m.m[1] * px + m.m[13]) * r.normal.y
          + (m.m[4] * py + m.m[0] * px + m.m[8] * pz + m.m[12]) * r.normal.x);
    return r;
}

// aidll / The body constructs one point on the plane as
// `-normal * d`, intersects the infinite line, records whether the parameter lies in [0, 1], and
// returns true for every non-parallel line. The range flag does not control the return value.
// On a parallel line neither output is written.
bool Plane3::intersectLine(const Vec3& a, const Vec3& b, Vec3& out, bool& insideSegment)
{
    Vec3 pointOnPlane;
    pointOnPlane.x = -d * normal.x;
    pointOnPlane.y = -d * normal.y;
    pointOnPlane.z = -d * normal.z;

    const float dx = b.x - a.x;
    const float dy = b.y - a.y;
    const float dz = b.z - a.z;
    const float denominator = dx * normal.x + dz * normal.z + dy * normal.y;
    if (denominator == 0.0f)
        return false;

    const float amount = ((pointOnPlane.x - a.x) * normal.x
                        + (pointOnPlane.z - a.z) * normal.z
                        + (pointOnPlane.y - a.y) * normal.y) / denominator;
    insideSegment = amount >= 0.0f && amount <= 1.0f;
    out.x = a.x + dx * amount;
    out.y = a.y + dy * amount;
    out.z = a.z + dz * amount;
    return true;
}

// transcribed term for term (a2 = p1, a3 = p2, a4 = p3 in the decompile).
Vec3 intersect(const Plane3& p1, const Plane3& p2, const Plane3& p3)
{
    const float n1x = p1.normal.x, n1y = p1.normal.y, n1z = p1.normal.z;
    const float n2x = p2.normal.x, n2y = p2.normal.y, n2z = p2.normal.z;
    const float n3x = p3.normal.x, n3y = p3.normal.y, n3z = p3.normal.z;
    const float md1 = -p1.d, md2 = -p2.d, md3 = -p3.d;
    const float v9  = (n1z * n3y - n1y * n3z) * md2 + (n3z * n2y - n3y * n2z) * md1;
    const float v10 = (n1x * n3z - n1z * n3x) * md2 + (n3x * n2z - n3z * n2x) * md1;
    const float v11 = (n1y * n3x - n1x * n3y) * md2 + md1 * (n3y * n2x - n3x * n2y);
    const float inv = 1.0f / ((n3x * n2z - n3z * n2x) * n1y
                            + (n3z * n2y - n3y * n2z) * n1x
                            + (n3y * n2x - n3x * n2y) * n1z);
    Vec3 r;
    r.x = ((n2z * n1y - n2y * n1z) * md3 + v9) * inv;
    r.y = ((n2x * n1z - n2z * n1x) * md3 + v10) * inv;
    r.z = (v11 + (n2y * n1x - n2x * n1y) * md3) * inv;
    return r;
}

} // namespace hfe
} // namespace dice
