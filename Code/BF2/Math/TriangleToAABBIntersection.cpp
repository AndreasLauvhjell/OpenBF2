// Triangle/aabb separating-axis test without epsilon or degenerate-triangle rejection.
#include "TriangleToAABBIntersection.h"
#include <cmath>

namespace dice { namespace hfe {
namespace {

// . The caller tests the full integer result before
// materializing bool. PE's register calling convention is compiler-local only.
int planeBoxOverlap(float* normal, float distance, float* halfSize)
{
    float vmin[3], vmax[3];
    for (int axis = 0; axis < 3; ++axis) {
        if (normal[axis] > 0.0f) {
            vmin[axis] = -halfSize[axis];
            vmax[axis] = halfSize[axis];
        } else {
            vmin[axis] = halfSize[axis];
            vmax[axis] = -halfSize[axis];
        }
    }
    // [PLATFORM] PE 68D901..68D939 adds X,Z,Y; ELF 83B8D8E..83B8DC2 adds X,Y,Z.
    // Keep the ordered > rejection followed by >= acceptance (also for NaNs).
#if defined(_WIN32)
    if (normal[0] * vmin[0] + normal[2] * vmin[2] + normal[1] * vmin[1] + distance > 0.0f)
        return 0;
    return normal[0] * vmax[0] + normal[2] * vmax[2] + normal[1] * vmax[1] + distance >= 0.0f;
#else
    if (normal[0] * vmin[0] + normal[1] * vmin[1] + normal[2] * vmin[2] + distance > 0.0f)
        return 0;
    return normal[0] * vmax[0] + normal[1] * vmax[1] + normal[2] * vmax[2] + distance >= 0.0f;
#endif
}

} // anonymous namespace

bool triangleToAABBIntersection(float* center, float* halfSize, float (*triangle)[3])
{
    float v0[3], v1[3], v2[3], e0[3], e1[3], e2[3];
    for (int axis = 0; axis < 3; ++axis) {
        v0[axis] = triangle[0][axis] - center[axis];
        v1[axis] = triangle[1][axis] - center[axis];
        v2[axis] = triangle[2][axis] - center[axis];
    }
    for (int axis = 0; axis < 3; ++axis) {
        e0[axis] = v1[axis] - v0[axis];
        e1[axis] = v2[axis] - v1[axis];
        e2[axis] = v0[axis] - v2[axis];
    }

    // [CHOSEN source factoring] Shared projection overlap test; preserve strict comparison order.
    // Z12 compares p2 with p1.
    const auto overlaps = [](float first, float second, float radius) {
        float minimum, maximum;
        if (first < second) { minimum = first; maximum = second; }
        else { minimum = second; maximum = first; }
        return !(minimum > radius || -radius > maximum);
    };

    float fex = std::fabs(e0[0]);
    float fey = std::fabs(e0[1]);
    float fez = std::fabs(e0[2]);
    if (!overlaps(e0[2]*v0[1] - e0[1]*v0[2], e0[2]*v2[1] - e0[1]*v2[2],
                  fez*halfSize[1] + fey*halfSize[2])) return false;
    if (!overlaps(e0[0]*v0[2] - e0[2]*v0[0], e0[0]*v2[2] - e0[2]*v2[0],
                  fex*halfSize[2] + fez*halfSize[0])) return false;
    if (!overlaps(e0[1]*v2[0] - e0[0]*v2[1], e0[1]*v1[0] - e0[0]*v1[1],
                  fex*halfSize[1] + fey*halfSize[0])) return false;

    fex = std::fabs(e1[0]);
    fey = std::fabs(e1[1]);
    fez = std::fabs(e1[2]);
    if (!overlaps(e1[2]*v0[1] - e1[1]*v0[2], e1[2]*v2[1] - e1[1]*v2[2],
                  fez*halfSize[1] + fey*halfSize[2])) return false;
    if (!overlaps(e1[0]*v0[2] - e1[2]*v0[0], e1[0]*v2[2] - e1[2]*v2[0],
                  fex*halfSize[2] + fez*halfSize[0])) return false;
    if (!overlaps(e1[1]*v0[0] - e1[0]*v0[1], e1[1]*v1[0] - e1[0]*v1[1],
                  fex*halfSize[1] + fey*halfSize[0])) return false;

    fex = std::fabs(e2[0]);
    fey = std::fabs(e2[1]);
    fez = std::fabs(e2[2]);
    if (!overlaps(e2[2]*v0[1] - e2[1]*v0[2], e2[2]*v1[1] - e2[1]*v1[2],
                  fez*halfSize[1] + fey*halfSize[2])) return false;
    if (!overlaps(e2[0]*v0[2] - e2[2]*v0[0], e2[0]*v1[2] - e2[2]*v1[0],
                  fex*halfSize[2] + fez*halfSize[0])) return false;
    if (!overlaps(e2[1]*v2[0] - e2[0]*v2[1], e2[1]*v1[0] - e2[0]*v1[1],
                  fex*halfSize[1] + fey*halfSize[0])) return false;

    for (int axis = 0; axis < 3; ++axis) {
        float minimum = v0[axis], maximum = v0[axis];
        if (v1[axis] < minimum) minimum = v1[axis];
        if (v1[axis] > maximum) maximum = v1[axis];
        if (v2[axis] < minimum) minimum = v2[axis];
        if (v2[axis] > maximum) maximum = v2[axis];
        if (minimum > halfSize[axis] || -halfSize[axis] > maximum) return false;
    }

    float normal[3] = {
        e1[2]*e0[1] - e1[1]*e0[2],
        e1[0]*e0[2] - e1[2]*e0[0],
        e1[1]*e0[0] - e1[0]*e0[1]
    };
    // [PLATFORM] PE 68DFE6..68DFF9 sums Z,Y,X; ELF 83B958A..83B95B0 sums X,Y,Z.
#if defined(_WIN32)
    const float distance = -(normal[2]*v0[2] + normal[1]*v0[1] + normal[0]*v0[0]);
#else
    const float distance = -(normal[0]*v0[0] + normal[1]*v0[1] + normal[2]*v0[2]);
#endif
    return planeBoxOverlap(normal, distance, halfSize) != 0;
}

bool triangleToAABBIntersection(const Vec3& boxMin, const Vec3& boxMax,
                                const Vec3& v0, const Vec3& v1, const Vec3& v2)
{
    float triangle[3][3] = {{v0.x, v0.y, v0.z}, {v1.x, v1.y, v1.z}, {v2.x, v2.y, v2.z}};
    float center[3] = {(boxMax.x + boxMin.x)*0.5f, (boxMax.y + boxMin.y)*0.5f,
                       (boxMax.z + boxMin.z)*0.5f};
    float halfSize[3] = {(boxMax.x - boxMin.x)*0.5f, (boxMax.y - boxMin.y)*0.5f,
                         (boxMax.z - boxMin.z)*0.5f};
    return triangleToAABBIntersection(center, halfSize, triangle);
}

} } // dice::hfe
