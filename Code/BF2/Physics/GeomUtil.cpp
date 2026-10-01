// Primitive collision tests; Physics/GeomUtil.cpp is attested by a diagnostic source path.

#include "GeomUtil.h"

#include <cfloat>
#include <cmath>

#include "CollisionStats.h"
#include "../Math/Mat4.h"

namespace dice {
namespace hfe {
namespace world {

// Returns the closest distance to a segment. Preserve both endpoint comparisons and
// the separate conversion of along-distance into a segment fraction.
float getClosestDistanceToLine(const Vec3& point, const Vec3* start, const Vec3* end)
{
    const Vec3 direction = {end->x - start->x, end->y - start->y, end->z - start->z};
    Vec3 offset = {point.x - start->x, point.y - start->y, point.z - start->z};
    const float length = sqrtf(direction.z * direction.z + direction.y * direction.y + direction.x * direction.x);
    const float inverseLength = 1.0f / length;
    const float along = (direction.z * offset.z + offset.y * direction.y + offset.x * direction.x) * inverseLength;
    if (along < 0.0f)
        return sqrtf(offset.z * offset.z + offset.y * offset.y + offset.x * offset.x);
    if (along > length)
        offset = Vec3{point.x - end->x, point.y - end->y, point.z - end->z};
    else
        offset = Vec3{point.x - (direction.x * along * inverseLength + start->x),
                      point.y - (direction.y * along * inverseLength + start->y),
                      point.z - (direction.z * along * inverseLength + start->z)};
    return sqrtf(offset.z * offset.z + offset.y * offset.y + offset.x * offset.x);
}

// checkFaceAndEdgeCollision - 0x83A9C40, 1,364 B.
bool checkFaceAndEdgeCollision(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& normal,
                               const Vec3& p0, const Vec3& p1, Vec3& outHit, float& outPlaneDist,
                               float& outAlong, bool twoSided, float /*unusedTolerance*/)
{
    ++collisionStatsData->faceAndEdgeChecks;                            // inc [edx+28h]

    // d = p1 - p0; a zero-length segment is no hit.
    const float d[3] = { p1.x - p0.x, p1.y - p0.y, p1.z - p0.z };
    const float dx = d[0], dy = d[1], dz = d[2];
    if (dx == 0.0f && dy == 0.0f && dz == 0.0f)
        return false;

    // The end point's signed plane distance - must be on or behind the plane.
    const float dEnd = (p1.x - v0.x) * normal.x + (p1.y - v0.y) * normal.y + (p1.z - v0.z) * normal.z;
    if (dEnd > 0.0f)
        return false;

    // The start point's - must be on or in front of it.
    const float dStart = (p0.x - v0.x) * normal.x + (p0.y - v0.y) * normal.y + (p0.z - v0.z) * normal.z;
    if (dStart < 0.0f)
        return false;

    // The direction's - unless twoSided, it must point into the plane.
    const float dDir = dx * normal.x + dy * normal.y + dz * normal.z;
    if (!twoSided && dDir >= 0.0f)
        return false;

    const float t = dStart / -dDir;                                     // fdivp

    // The projection axes: drop the normal's dominant component. i0/i1 = the tested pair,
    // i2 = the one filled in last.
    int i0, i1, i2;
    if (fabsf(normal.y) >= 0.7f)          { i0 = 0; i1 = 2; i2 = 1; }  // mostly horizontal face
    else if (fabsf(normal.z) <= 0.3f)     { i0 = 1; i1 = 2; i2 = 0; }  // x-dominant
    else                                  { i0 = 0; i1 = 1; i2 = 2; }  // z-dominant

    const float* P0 = &p0.x;
    const float* D  = d;
    const float* V0 = &v0.x;
    const float* V1 = &v1.x;
    const float* V2 = &v2.x;
    float*       H  = &outHit.x;

    H[i0] = t * D[i0] + P0[i0];
    H[i1] = t * D[i1] + P0[i1];

    // The three edge sides in the (i0, i1) plane, each the ELF's own operand order.
    const float eps = 1.1920929e-7f;                                    // FLT_EPSILON
    const float c1 = (V1[i1] - V0[i1]) * (H[i0] - V0[i0]) - (V1[i0] - V0[i0]) * (H[i1] - V0[i1]);
    const float c2 = (H[i0] - V1[i0]) * (V2[i1] - V1[i1]) - (H[i1] - V1[i1]) * (V2[i0] - V1[i0]);
    if (c1 < -eps && c2 > eps)  return false;
    if (c1 >  eps && c2 < -eps) return false;
    const float c3 = (H[i0] - V2[i0]) * (V0[i1] - V2[i1]) - (H[i1] - V2[i1]) * (V0[i0] - V2[i0]);
    if (c1 < -eps && c3 > eps)  return false;
    if (c1 >  eps && c3 < -eps) return false;
    // c2 vs c3 is never compared - as shipped.

    H[i2] = t * D[i2] + P0[i2];
    outPlaneDist = dEnd;
    outAlong     = sqrtf(dz * dz + dy * dy + dx * dx) * (t - 1.0f);
    return true;
}


bool checkFaceAndEdgeCollisionDoubleSided(const Vec3& v0, const Vec3& v1, const Vec3& v2,
                                          const Vec3& normal, const Vec3& p0, const Vec3& p1,
                                          Vec3& outHit, float& outPlaneDist, float& outAlong)
{
    return checkFaceAndEdgeCollision(v0, v1, v2, normal, p0, p1, outHit, outPlaneDist, outAlong,
                                     true, 0.0f);
}

namespace {

static Vec3 add(const Vec3& a, const Vec3& b)
{
    Vec3 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    return result;
}

static Vec3 subtract(const Vec3& a, const Vec3& b)
{
    Vec3 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    return result;
}

static Vec3 scaled(const Vec3& value, float scale)
{
    Vec3 result;
    result.x = value.x * scale;
    result.y = value.y * scale;
    result.z = value.z * scale;
    return result;
}

static float dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static float lengthSquared(const Vec3& value)
{
    return value.z * value.z + value.y * value.y + value.x * value.x;
}

// The common engine normalise sequence: vectors already within FLT_EPSILON of unit length are
// left bit-identical; degenerate vectors become zero.
static Vec3 normalizedLikeEngine(const Vec3& value)
{
    Vec3 result = value;
    const float lengthSq = lengthSquared(value);
    if (fabsf(lengthSq - 1.0f) >= FLT_EPSILON)
    {
        if (fabsf(lengthSq) >= FLT_EPSILON)
        {
            const float inverseLength = 1.0f / sqrtf(lengthSq);
            result.x *= inverseLength;
            result.y *= inverseLength;
            result.z *= inverseLength;
        }
        else
        {
            result.x = result.y = result.z = 0.0f;
        }
    }
    return result;
}

} // anonymous namespace

// All three axes are non-const references in the ELF mangling; the target calls
// this with an already-normalised z axis.
void generateOrthonormalBasis(Vec3& x, Vec3& y, Vec3& z, bool alreadyNormalized)
{
    if (!alreadyNormalized)
        z = normalizedLikeEngine(z);

    if (fabsf(z.x) < fabsf(z.y))
    {
        const float inverseLength = 1.0f / sqrtf(z.y * z.y + z.z * z.z);
        x.x = 0.0f;
        x.y = z.z * inverseLength;
        x.z = -z.y * inverseLength;
    }
    else
    {
        const float inverseLength = 1.0f / sqrtf(z.x * z.x + z.z * z.z);
        x.x = -z.z * inverseLength;
        x.y = 0.0f;
        x.z = z.x * inverseLength;
    }

    y.x = z.y * x.z - z.z * x.y;
    y.y = z.z * x.x - z.x * x.z;
    y.z = z.x * x.y - z.y * x.x;
}

// Closest distance between two finite line segments. This is the target's
// complete nine-region solver; its unusually broad 0.001 determinant threshold is present in both
// the ELF and PE (PE 0x6B6260).
float getClosestDistanceBetweenLines(const Vec3& p0, const Vec3& p1,
                                     const Vec3& q0, const Vec3& q1,
                                     float& outP, float& outQ)
{
    const Vec3 direction0 = subtract(p1, p0);
    const Vec3 direction1 = subtract(q1, q0);
    const Vec3 difference = subtract(p0, q0);
    const float a00 = dot(direction0, direction0);
    const float a01 = -dot(direction0, direction1);
    const float a11 = dot(direction1, direction1);
    const float b0 = dot(difference, direction0);
    const float c = dot(difference, difference);
    const float determinant = fabsf(a00 * a11 - a01 * a01);
    float b1;
    float s;
    float t;
    float squaredDistance;

    if (determinant >= 0.001f)
    {
        b1 = -dot(difference, direction1);
        const float sNumerator = a01 * b1 - a11 * b0;
        const float tNumerator = a01 * b0 - a00 * b1;
        if (sNumerator >= 0.0f)
        {
            if (sNumerator <= determinant)
            {
                if (tNumerator >= 0.0f)
                {
                    if (tNumerator <= determinant)                         // region 0
                    {
                        const float inverseDeterminant = 1.0f / determinant;
                        s = sNumerator * inverseDeterminant;
                        t = tNumerator * inverseDeterminant;
                        squaredDistance = s * (a00 * s + a01 * t + 2.0f * b0)
                                        + t * (a01 * s + a11 * t + 2.0f * b1) + c;
                    }
                    else                                                    // region 3
                    {
                        t = 1.0f;
                        const float temporary = a01 + b0;
                        if (temporary >= 0.0f)
                        {
                            s = 0.0f;
                            squaredDistance = a11 + 2.0f * b1 + c;
                        }
                        else if (-temporary >= a00)
                        {
                            s = 1.0f;
                            squaredDistance = a00 + a11 + c
                                            + 2.0f * (b1 + temporary);
                        }
                        else
                        {
                            s = -temporary / a00;
                            squaredDistance = temporary * s + a11 + 2.0f * b1 + c;
                        }
                    }
                }
                else                                                        // region 7
                {
                    t = 0.0f;
                    if (b0 >= 0.0f)
                    {
                        s = 0.0f;
                        squaredDistance = c;
                    }
                    else if (-b0 >= a00)
                    {
                        s = 1.0f;
                        squaredDistance = a00 + 2.0f * b0 + c;
                    }
                    else
                    {
                        s = -b0 / a00;
                        squaredDistance = b0 * s + c;
                    }
                }
            }
            else
            {
                if (tNumerator >= 0.0f)
                {
                    if (tNumerator <= determinant)                         // region 1
                    {
                        s = 1.0f;
                        const float temporary = a01 + b1;
                        if (temporary >= 0.0f)
                        {
                            t = 0.0f;
                            squaredDistance = a00 + 2.0f * b0 + c;
                        }
                        else if (-temporary >= a11)
                        {
                            t = 1.0f;
                            squaredDistance = a00 + a11 + c
                                            + 2.0f * (b0 + temporary);
                        }
                        else
                        {
                            t = -temporary / a11;
                            squaredDistance = temporary * t + a00 + 2.0f * b0 + c;
                        }
                    }
                    else                                                    // region 2
                    {
                        const float temporary = a01 + b0;
                        if (-temporary <= a00)
                        {
                            t = 1.0f;
                            if (temporary >= 0.0f)
                            {
                                s = 0.0f;
                                squaredDistance = a11 + 2.0f * b1 + c;
                            }
                            else
                            {
                                s = -temporary / a00;
                                squaredDistance = temporary * s + a11 + 2.0f * b1 + c;
                            }
                        }
                        else
                        {
                            s = 1.0f;
                            const float side = a01 + b1;
                            if (side >= 0.0f)
                            {
                                t = 0.0f;
                                squaredDistance = a00 + 2.0f * b0 + c;
                            }
                            else if (-side >= a11)
                            {
                                t = 1.0f;
                                squaredDistance = a00 + a11 + c
                                                + 2.0f * (b0 + side);
                            }
                            else
                            {
                                t = -side / a11;
                                squaredDistance = side * t + a00 + 2.0f * b0 + c;
                            }
                        }
                    }
                }
                else                                                        // region 8
                {
                    if (-b0 < a00)
                    {
                        t = 0.0f;
                        if (b0 >= 0.0f)
                        {
                            s = 0.0f;
                            squaredDistance = c;
                        }
                        else
                        {
                            s = -b0 / a00;
                            squaredDistance = b0 * s + c;
                        }
                    }
                    else
                    {
                        s = 1.0f;
                        const float temporary = a01 + b1;
                        if (temporary >= 0.0f)
                        {
                            t = 0.0f;
                            squaredDistance = a00 + 2.0f * b0 + c;
                        }
                        else if (-temporary >= a11)
                        {
                            t = 1.0f;
                            squaredDistance = a00 + a11 + c
                                            + 2.0f * (b0 + temporary);
                        }
                        else
                        {
                            t = -temporary / a11;
                            squaredDistance = temporary * t + a00 + 2.0f * b0 + c;
                        }
                    }
                }
            }
        }
        else
        {
            if (tNumerator >= 0.0f)
            {
                if (tNumerator <= determinant)                             // region 5
                {
                    s = 0.0f;
                    if (b1 >= 0.0f)
                    {
                        t = 0.0f;
                        squaredDistance = c;
                    }
                    else if (-b1 >= a11)
                    {
                        t = 1.0f;
                        squaredDistance = a11 + 2.0f * b1 + c;
                    }
                    else
                    {
                        t = -b1 / a11;
                        squaredDistance = b1 * t + c;
                    }
                }
                else                                                        // region 4
                {
                    const float temporary = a01 + b0;
                    if (temporary < 0.0f)
                    {
                        t = 1.0f;
                        if (-temporary >= a00)
                        {
                            s = 1.0f;
                            squaredDistance = a00 + a11 + c
                                            + 2.0f * (b1 + temporary);
                        }
                        else
                        {
                            s = -temporary / a00;
                            squaredDistance = temporary * s + a11 + 2.0f * b1 + c;
                        }
                    }
                    else
                    {
                        s = 0.0f;
                        if (b1 >= 0.0f)
                        {
                            t = 0.0f;
                            squaredDistance = c;
                        }
                        else if (-b1 >= a11)
                        {
                            t = 1.0f;
                            squaredDistance = a11 + 2.0f * b1 + c;
                        }
                        else
                        {
                            t = -b1 / a11;
                            squaredDistance = b1 * t + c;
                        }
                    }
                }
            }
            else                                                            // region 6
            {
                if (b0 < 0.0f)
                {
                    t = 0.0f;
                    if (-b0 >= a00)
                    {
                        s = 1.0f;
                        squaredDistance = a00 + 2.0f * b0 + c;
                    }
                    else
                    {
                        s = -b0 / a00;
                        squaredDistance = b0 * s + c;
                    }
                }
                else
                {
                    s = 0.0f;
                    if (b1 >= 0.0f)
                    {
                        t = 0.0f;
                        squaredDistance = c;
                    }
                    else if (-b1 >= a11)
                    {
                        t = 1.0f;
                        squaredDistance = a11 + 2.0f * b1 + c;
                    }
                    else
                    {
                        t = -b1 / a11;
                        squaredDistance = b1 * t + c;
                    }
                }
            }
        }
    }
    else
    {
        // Almost-parallel segment pair: retail selects one of two direction-dependent boundary
        // trees instead of dividing by the small determinant.
        if (a01 > 0.0f)
        {
            t = 0.0f;
            if (b0 >= 0.0f)
            {
                s = 0.0f;
                squaredDistance = c;
            }
            else if (-b0 <= a00)
            {
                s = -b0 / a00;
                squaredDistance = b0 * s + c;
            }
            else
            {
                b1 = -dot(difference, direction1);
                s = 1.0f;
                const float temporary = a00 + b0;
                if (-temporary >= a01)
                {
                    t = 1.0f;
                    squaredDistance = a00 + a11 + c
                                    + 2.0f * (a01 + b0 + b1);
                }
                else
                {
                    t = -temporary / a01;
                    squaredDistance = a00 + 2.0f * b0 + c
                                    + t * (a11 * t + 2.0f * (a01 + b1));
                }
            }
        }
        else
        {
            t = 0.0f;
            if (-b0 >= a00)
            {
                s = 1.0f;
                squaredDistance = a00 + 2.0f * b0 + c;
            }
            else if (b0 <= 0.0f)
            {
                s = -b0 / a00;
                squaredDistance = b0 * s + c;
            }
            else
            {
                s = 0.0f;
                b1 = -dot(difference, direction1);
                if (b0 >= -a01)
                {
                    t = 1.0f;
                    squaredDistance = a11 + 2.0f * b1 + c;
                }
                else
                {
                    t = -b0 / a01;
                    squaredDistance = c + t * (2.0f * b1 + a11 * t);
                }
            }
        }
    }

    outP = s;
    outQ = t;
    return sqrtf(fabsf(squaredDistance));
}

// Times are fractions of `axis`, not world distances.
int getIntersectionOfCapsAndEdgeInternal(const Vec3& start, const Vec3& axis,
                                         const Vec3& edgeStart, const Vec3& edgeAxis,
                                         float radius, float* times)
{
    const float edgeLength = sqrtf(lengthSquared(edgeAxis));
    if (edgeLength <= FLT_EPSILON)
        return 0;

    Vec3 z = scaled(edgeAxis, 1.0f / edgeLength);
    Vec3 x, y;
    generateOrthonormalBasis(x, y, z, true);

    const float speed = sqrtf(lengthSquared(axis));
    if (speed <= FLT_EPSILON)
        return 0;
    const Vec3 direction = scaled(axis, 1.0f / speed);
    const Vec3 relative = subtract(start, edgeStart);
    const float nx = dot(direction, x);
    const float ny = dot(direction, y);
    const float nz = dot(direction, z);
    const float px = dot(relative, x);
    const float py = dot(relative, y);
    const float pz = dot(relative, z);
    const float radiusSquared = radius * radius;
    // Retail treats an almost-parallel sweep as exactly parallel. This is not just an optimisation:
    // its parallel arm uses the fixed radial offset and the unnormalised axis dot to order the two
    // end-cap intersections.
    if (!(fabsf(nz) < 0.99999988f))            // target is `if (<) general; else parallel`
    {
        const float axialDirection = dot(axis, edgeAxis);
        const float radialRemainder = radiusSquared - px * px - py * py;
        if (axialDirection < 0.0f && radialRemainder >= 0.0f)
        {
            const float capHeight = sqrtf(radialRemainder);
            times[0] = (pz + capHeight) / speed;
            times[1] = -(edgeLength - pz + capHeight) / speed;
            return 2;
        }
        if (axialDirection <= 0.0f || radialRemainder < 0.0f)
            return 0;
        else
        {
            const float capHeight = sqrtf(radialRemainder);
            times[0] = -(pz + capHeight) / speed;
            times[1] = (edgeLength - pz + capHeight) / speed;
        }
        return 2;
    }

    int count = 0;
    const float cylinderA = nx * nx + ny * ny;
    const float cylinderB = px * nx + py * ny;
    const float cylinderC = px * px + py * py - radiusSquared;
    const float cylinderDiscriminant = cylinderB * cylinderB - cylinderA * cylinderC;
    if (cylinderDiscriminant < 0.0f)
        return 0;
    if (cylinderDiscriminant == 0.0f)
    {
        const float root = -cylinderB / cylinderA;
        const float edgeCoordinate = pz + nz * root;
        if (edgeCoordinate >= 0.0f && edgeCoordinate <= edgeLength)
        {
            times[0] = root / speed;
            return 1;
        }
    }
    else
    {
        const float squareRoot = sqrtf(cylinderDiscriminant);
        const float roots[2] = {
            (-cylinderB - squareRoot) / cylinderA,
            ( squareRoot - cylinderB) / cylinderA
        };
        for (int i = 0; i < 2; ++i)
        {
            const float edgeCoordinate = pz + nz * roots[i];
            if (edgeCoordinate >= 0.0f && edgeCoordinate <= edgeLength)
                times[count++] = roots[i] / speed;
        }
        if (count == 2)
            return 2;
    }

    // Start hemispherical cap.
    const float startB = cylinderB + pz * nz;
    const float startC = cylinderC + pz * pz;
    const float startDiscriminant = startB * startB - startC;
    if (startDiscriminant >= 0.0f)
    {
        const float squareRoot = sqrtf(startDiscriminant);
        const float roots[2] = { -startB - squareRoot, squareRoot - startB };
        const int rootCount = startDiscriminant == 0.0f ? 1 : 2;
        for (int i = 0; i < rootCount && count < 2; ++i)
        {
            if (pz + nz * roots[i] <= 0.0f)
                times[count++] = roots[i] / speed;
        }
        if (count == 2)
            return 2;
    }

    // End hemispherical cap.
    const float endB = startB - edgeLength * nz;
    const float endC = startC + (edgeLength - (pz + pz)) * edgeLength;
    const float endDiscriminant = endB * endB - endC;
    if (endDiscriminant >= 0.0f)
    {
        const float squareRoot = sqrtf(endDiscriminant);
        const float roots[2] = { -endB - squareRoot, squareRoot - endB };
        const int rootCount = endDiscriminant == 0.0f ? 1 : 2;
        for (int i = 0; i < rootCount && count < 2; ++i)
        {
            if (pz + nz * roots[i] >= edgeLength)
                times[count++] = roots[i] / speed;
        }
    }
    return count;
}


int getIntersectionOfCapsAndEdgeNew(const Vec3& start, const Vec3& axis,
                                    const Vec3& edgeStart, const Vec3& edgeAxis,
                                    float radius, float* times, Vec3* points)
{
    const Vec3 end = add(start, axis);
    const Vec3 edgeEnd = add(edgeStart, edgeAxis);
    float firstParameter = 0.0f;
    float secondParameter = 0.0f;
    if (getClosestDistanceBetweenLines(start, end, edgeStart, edgeEnd,
                                       firstParameter, secondParameter) > radius)
        return 0;

    float candidateTimes[2];
    const int candidateCount = getIntersectionOfCapsAndEdgeInternal(
        start, axis, edgeStart, edgeAxis, radius, candidateTimes);
    int count = 0;
    for (int i = 0; i < candidateCount; ++i)
    {
        const float time = candidateTimes[i];
        if (time >= 0.0f && time <= 1.0f)
        {
            times[count] = time;
            points[count] = add(start, scaled(axis, time));
            ++count;
        }
    }
    return count;
}

static float projectedComponent(const Vec3& value, int component)
{
    return component == 0 ? value.x : (component == 1 ? value.y : value.z);
}

// Bits correspond to edges (v2,v0), (v0,v1), (v1,v2).
unsigned int classifyPointToTriEdges(const Vec3** vertices, const Vec3& normal,
                                     const Vec3& point)
{
    int firstAxis;
    int secondAxis;
    if (fabsf(normal.y) >= 0.7f)
    {
        firstAxis = 0;
        secondAxis = 2;
    }
    else if (fabsf(normal.z) <= 0.3f)
    {
        firstAxis = 1;
        secondAxis = 2;
    }
    else
    {
        firstAxis = 0;
        secondAxis = 1;
    }

    const Vec3 centroid = scaled(add(add(*vertices[0], *vertices[1]), *vertices[2]), 1.0f / 3.0f);
    const int starts[3] = { 2, 0, 1 };
    const int ends[3] = { 0, 1, 2 };
    unsigned int mask = 0;
    for (int edge = 0; edge < 3; ++edge)
    {
        const Vec3& edgeStart = *vertices[starts[edge]];
        const Vec3& edgeEnd = *vertices[ends[edge]];
        const float px = projectedComponent(point, firstAxis) - projectedComponent(edgeStart, firstAxis);
        const float py = projectedComponent(point, secondAxis) - projectedComponent(edgeStart, secondAxis);
        const float ex = projectedComponent(edgeEnd, firstAxis) - projectedComponent(edgeStart, firstAxis);
        const float ey = projectedComponent(edgeEnd, secondAxis) - projectedComponent(edgeStart, secondAxis);
        if (px * ey - py * ex >= 0.0f)
            mask |= 1u << edge;
    }

    const Vec3& orientationStart = *vertices[1];
    const Vec3& orientationEnd = *vertices[2];
    const float cx = projectedComponent(centroid, firstAxis)
                   - projectedComponent(orientationStart, firstAxis);
    const float cy = projectedComponent(centroid, secondAxis)
                   - projectedComponent(orientationStart, secondAxis);
    const float ox = projectedComponent(orientationEnd, firstAxis)
                   - projectedComponent(orientationStart, firstAxis);
    const float oy = projectedComponent(orientationEnd, secondAxis)
                   - projectedComponent(orientationStart, secondAxis);
    if (cx * oy - cy * ox < 0.0f)
        mask ^= 7u;
    return mask;
}

// An endpoint miss reports which endpoint the caller should test as a sphere.
bool checkSphereLineCollision(const Vec3& center, float radius, const Vec3* lineStart,
                              const Vec3* lineEnd, Vec3& outPosition, Vec3& outNormal,
                              float& outDistance, int& outEndpoint)
{
    const Vec3 line = subtract(*lineEnd, *lineStart);
    const float lineLength = sqrtf(lengthSquared(line));
    if (lineLength <= 0.0f)
        return false;
    const float projection = dot(subtract(center, *lineStart), line) / lineLength;
    if (projection < 0.0f)
    {
        outEndpoint = 0;
        return false;
    }
    if (projection > lineLength)
    {
        outEndpoint = 1;
        return false;
    }

    outPosition = add(*lineStart, scaled(line, projection / lineLength));
    outNormal = subtract(center, outPosition);
    const float normalLengthSquared = lengthSquared(outNormal);
    if (normalLengthSquared > radius * radius)
        return false;
    const float normalLength = sqrtf(normalLengthSquared);
    const float inverseLength = 1.0f / normalLength; // zero-length division is shipped behavior
    outNormal.x *= inverseLength;
    outNormal.y *= inverseLength;
    outNormal.z *= inverseLength;
    outDistance = normalLength - radius;
    return true;
}


bool checkSpherePointCollision(const Vec3& center, float radius, const Vec3* point,
                               Vec3& outPosition, Vec3& outNormal, float& outDistance)
{
    const Vec3 normal = subtract(center, *point);
    const float normalLengthSquared = lengthSquared(normal);
    if (normalLengthSquared > radius * radius)
        return false;
    outPosition = *point;
    outNormal = normal;
    const float normalLength = sqrtf(normalLengthSquared);
    const float inverseLength = 1.0f / normalLength; // zero-length division is shipped behavior
    outNormal.x *= inverseLength;
    outNormal.y *= inverseLength;
    outNormal.z *= inverseLength;
    outDistance = normalLength - radius;
    return true;
}


bool checkShiftedFaceCollision(const Vec3& v0, const Vec3& v1, const Vec3& v2, const Vec3& normal,
                               const Vec3& start, const Vec3& end, float radius,
                               Vec3& outPosition, float& outDistanceA, float& outDistanceB,
                               const Mat4& /*objectToWorld*/)
{
    ++collisionStatsData->shiftedFaceChecks;
    const Vec3 shifted0 = add(v0, scaled(normal, radius));
    const Vec3 shifted1 = add(v1, scaled(normal, radius));
    const Vec3 shifted2 = add(v2, scaled(normal, radius));
    if (lengthSquared(subtract(end, start)) < FLT_EPSILON)
        return false;
    if (!checkFaceAndEdgeCollision(shifted0, shifted1, shifted2, normal, start, end,
                                   outPosition, outDistanceA, outDistanceB, false, 0.0f))
        return false;
    outPosition = subtract(outPosition, scaled(normal, radius));
    return true;
}


bool checkSphereTriCollision(const Vec3** vertices, const Vec3& normal, const Vec3& center,
                             float radius, Vec3& outPosition, Vec3& outNormal, float& outDistance)
{
    ++collisionStatsData->sphereChecks;
    const float planeDistance = dot(subtract(center, *vertices[0]), normal);
    if (fabsf(planeDistance) > radius)
        return false;
    const Vec3 projected = subtract(center, scaled(normal, planeDistance));
    const unsigned int edgeMask = classifyPointToTriEdges(vertices, normal, projected);
    if (edgeMask == 7u)
    {
        outPosition = projected;
        outNormal = normal;
        outDistance = planeDistance - radius;
        return true;
    }

    int rememberedVertex = -1;
    int previous = 2;
    for (int current = 0; current < 3; ++current)
    {
        if ((edgeMask & (1u << current)) == 0)
        {
            int endpoint = -1;
            if (checkSphereLineCollision(center, radius, vertices[current], vertices[previous],
                                         outPosition, outNormal, outDistance, endpoint))
                return true;
            if (endpoint == 0)
                rememberedVertex = current;
            else if (endpoint == 1)
                rememberedVertex = previous;
        }
        previous = current;
    }
    return rememberedVertex >= 0
        && checkSpherePointCollision(center, radius, vertices[rememberedVertex],
                                     outPosition, outNormal, outDistance);
}

// The large middle arm in retail draws the candidate capsules when the
// global debug toggle is enabled; it has no effect on the collision result and the server tree has
// no recovered DebugRenderer cylinder surface yet.
bool checkExpandedEdgesCollision(unsigned int /*edgeMask*/, const Vec3& v0, const Vec3& v1,
                                 const Vec3& v2, const Vec3& /*faceNormal*/, const Vec3& start,
                                 const Vec3& end, float radius, Vec3& outPosition,
                                 Vec3& outNormal, float& outDistanceA, float& outDistanceB,
                                 const Mat4& /*objectToWorld*/)
{
    const Vec3* vertices[3] = { &v0, &v1, &v2 };
    const Vec3 velocity = subtract(end, start);
    float bestTime = FLT_MAX;
    Vec3 bestCenter;
    int bestCurrent = -1;
    int bestPrevious = -1;
    int previous = 2;
    for (int current = 0; current < 3; ++current)
    {
        const Vec3 edgeAxis = subtract(*vertices[previous], *vertices[current]);
        float times[2];
        Vec3 points[2];
        const int count = getIntersectionOfCapsAndEdgeNew(start, velocity, *vertices[current],
                                                          edgeAxis, radius, times, points);
        if (count > 0)
        {
            int selected = 0;
            if (count == 2 && times[1] <= times[0])
                selected = 1;
            if (times[selected] < bestTime)
            {
                bestTime = times[selected];
                bestCenter = points[selected];
                bestCurrent = current;
                bestPrevious = previous;
            }
        }
        previous = current;
    }
    if (bestCurrent < 0)
        return false;

    const Vec3 edge = subtract(*vertices[bestPrevious], *vertices[bestCurrent]);
    const float edgeLengthSquared = lengthSquared(edge);
    const Vec3 edgeDirection = normalizedLikeEngine(edge);
    const float edgeProjection = dot(subtract(bestCenter, *vertices[bestCurrent]), edgeDirection);
    if (edgeProjection <= 0.0f)
        outPosition = *vertices[bestCurrent];
    else if (edgeProjection * edgeProjection < edgeLengthSquared)
        outPosition = add(*vertices[bestCurrent], scaled(edgeDirection, edgeProjection));
    else
        outPosition = *vertices[bestPrevious];

    outNormal = normalizedLikeEngine(subtract(bestCenter, outPosition));
    const Vec3 remainingTravel = subtract(end, bestCenter);
    outDistanceA = dot(remainingTravel, outNormal);
    outDistanceB = -sqrtf(lengthSquared(remainingTravel));
    return true;
}

// checkCapsuleEdgesCollision - /

// Tests the moving capsule axis against the three finite triangle-edge capsules, remembers one
// edge candidate, then projects that candidate's centre onto the selected triangle edge. The face
// normal argument is not read. Preserve two unusual retail choices: a two-intersection edge
// replaces every earlier candidate without comparing times, and a downward result normal is
// flipped while the radius is added back to the reported distance.
bool checkCapsuleEdgesCollision(const Vec3& v0, const Vec3& v1, const Vec3& v2,
                                const Vec3& /*faceNormal*/, const Vec3& start, const Vec3& end,
                                float radius, Vec3& outPosition, Vec3& outNormal,
                                float& outDistance)
{
    const Vec3* vertices[3] = { &v0, &v1, &v2 };
    const Vec3 velocity = subtract(end, start);

    const Vec3* bestEdgeStart = 0;
    const Vec3* bestEdgeEnd = 0;
    Vec3 bestCenter = {};
    float bestTime = FLT_MAX;
    bool found = false;

    int previous = 2;
    for (int current = 0; current < 3; ++current)
    {
        float times[2];
        Vec3 points[2] = {};
        const Vec3 edgeAxis = subtract(*vertices[previous], *vertices[current]);
        const int count = getIntersectionOfCapsAndEdgeNew(start, velocity, *vertices[current],
                                                          edgeAxis, radius, times, points);
        if (count > 0)
        {
            if (count == 2)
            {
                found = true;
                bestCenter = add(points[0], scaled(subtract(points[1], points[0]), 0.5f));
                bestTime = times[0];
                bestEdgeStart = vertices[current];
                bestEdgeEnd = vertices[previous];
            }
            else if (times[0] < bestTime)
            {
                found = true;
                bestCenter = points[0];
                bestTime = times[0];
                bestEdgeStart = vertices[current];
                bestEdgeEnd = vertices[previous];
            }
        }
        previous = current;
    }

    if (!found)
        return false;

    const Vec3 edge = subtract(*bestEdgeEnd, *bestEdgeStart);
    const float edgeLengthSquared = lengthSquared(edge);
    const Vec3 edgeDirection = normalizedLikeEngine(edge);
    const float projection = dot(subtract(bestCenter, *bestEdgeStart), edgeDirection);
    if (projection <= 0.0f)
        outPosition = *bestEdgeStart;
    else if (projection * projection < edgeLengthSquared)
        outPosition = add(*bestEdgeStart, scaled(edgeDirection, projection));
    else
        outPosition = *bestEdgeEnd;

    const Vec3 separation = subtract(bestCenter, outPosition);
    outNormal = normalizedLikeEngine(separation);
    outDistance = sqrtf(lengthSquared(separation)) - radius;
    if (outNormal.y < 0.0f)
    {
        outNormal.x = -outNormal.x;
        outNormal.y = -outNormal.y;
        outNormal.z = -outNormal.z;
        outDistance += radius;
    }
    return true;
}

// Tests two equal-radius capsules using their axis closest points. Reports only
// nondegenerate axis separation and nonpositive overlap depth. The normal points from
// the second axis toward the first; closest points are recomputed from the returned parameters.
bool capsuleVsCapsuleCollision(const Vec3& p0, const Vec3& p1, const Vec3& q0, const Vec3& q1,
                               float radius, Vec3& outContact, Vec3& outNormal, float& outDepth)
{
    float s = 0.0f;
    float t = 0.0f;
    const float d = getClosestDistanceBetweenLines(p0, p1, q0, q1, s, t);

    const float px = p1.x - p0.x;
    const float py = p1.y - p0.y;
    const float pz = p1.z - p0.z;
    const float qx = q1.x - q0.x;
    const float qy = q1.y - q0.y;
    const float qz = q1.z - q0.z;

    if (fabsf(d) >= FLT_EPSILON)
    {
        const float ax = px * s + p0.x;
        const float ay = py * s + p0.y;
        const float az = s * pz + p0.z;
        const float bx = qx * t + q0.x;
        const float by = qy * t + q0.y;
        const float bz = qz * t + q0.z;
        const float depth = -(radius + radius - d);
        if (depth <= 0.0f)
        {
            const float inv = -1.0f / d;
            const float ny = (by - ay) * inv;
            const float nz = (bz - az) * inv;
            const float nx = (bx - ax) * inv;
            outNormal.x = nx;
            outNormal.y = ny;
            const float k = radius - (radius + radius - d);
            outNormal.z = nz;
            outContact.x = ax - nx * k;
            outContact.y = ay - ny * k;
            outContact.z = az - k * nz;
            outDepth = depth;
            return true;
        }
    }
    return false;
}

} // namespace world
} // namespace hfe
} // namespace dice
