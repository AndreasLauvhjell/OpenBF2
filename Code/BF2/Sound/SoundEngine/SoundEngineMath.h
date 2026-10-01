// [CHOSEN, not recovered] Shared file for inline sound math helpers.
// [RETIRE-WHEN: sound-engine-math] Complete and validate the corresponding SoundEngineMath operation.

#ifndef SOUND_SOUNDENGINE_SOUNDENGINEMATH_H
#define SOUND_SOUNDENGINE_SOUNDENGINEMATH_H

#include "../../Math/Vec3.h"
#include "../../Math/Mat4.h"

#include <cmath>
#include <cfloat>

namespace dice {
namespace hfe {
namespace sndmath {

// sub_100040A0, 63 B: out = a x b.
inline Vec3& cross(Vec3& out, const Vec3& a, const Vec3& b)
{
    out.x = b.z * a.y - b.y * a.z;
    out.y = b.x * a.z - a.x * b.z;
    out.z = a.x * b.y - b.x * a.y;
    return out;
}

// sub_1000D7A0, 135 B: normalise in place, but only when the length is not already 1 within
// FLT_EPSILON; a degenerate vector is zeroed and -20001 comes back, otherwise 0. The engine's own
// Vec3::normalize convention (Math/Vec3.h's makeOrthonormalBasis shows the same codes).
inline int normalizeIfNeeded(Vec3& v)
{
    const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    if (std::fabs(lenSq - 1.0f) >= FLT_EPSILON)
    {
        if (std::fabs(lenSq) < FLT_EPSILON)
        {
            v.x = 0.0f; v.y = 0.0f; v.z = 0.0f;
            return -20001;
        }
        const float inv = 1.0f / std::sqrt(lenSq);
        v.x *= inv; v.y *= inv; v.z *= inv;
    }
    return 0;
}

// sub_1001C870, 61 B: snap components below `eps` to exactly 0.
inline void snapToZero(Vec3& v, float eps)
{
    if (std::fabs(v.x) < eps) v.x = 0.0f;
    if (std::fabs(v.y) < eps) v.y = 0.0f;
    if (std::fabs(v.z) < eps) v.z = 0.0f;
}

// Builds an orthonormal basis, writing the third axis to third.
// Failed second normalization of up zeroes it and continues.
inline int makeOrthonormalBasis(Vec3& third, Vec3& forward, Vec3& up)
{
    if (normalizeIfNeeded(forward)) return -20003;
    if (normalizeIfNeeded(up))      return -20003;
    third.x = up.z * forward.y - up.y * forward.z;
    third.y = forward.z * up.x - up.z * forward.x;
    third.z = up.y * forward.x - forward.y * up.x;
    if (normalizeIfNeeded(third))   return -20003;
    cross(up, third, forward);
    normalizeIfNeeded(up);
    cross(forward, up, third);
    if (normalizeIfNeeded(forward)) return -20003;
    snapToZero(third,   FLT_EPSILON);
    snapToZero(forward, FLT_EPSILON);
    snapToZero(up,      FLT_EPSILON);
    return 0;
}

// sub_10004160, 106 B: build a matrix from three axis rows and a translation row.
inline Mat4& setRows(Mat4& m, const Vec3& r0, const Vec3& r1, const Vec3& r2, const Vec3& t)
{
    m.m[0]  = r0.x; m.m[1]  = r0.y; m.m[2]  = r0.z; m.m[3]  = 0.0f;
    m.m[4]  = r1.x; m.m[5]  = r1.y; m.m[6]  = r1.z; m.m[7]  = 0.0f;
    m.m[8]  = r2.x; m.m[9]  = r2.y; m.m[10] = r2.z; m.m[11] = 0.0f;
    m.m[12] = t.x;  m.m[13] = t.y;  m.m[14] = t.z;  m.m[15] = 1.0f;
    return m;
}

// sub_100041D0, 92 B: out = p * m (point: translation applied).
inline Vec3& transformPoint(const Mat4& m, const Vec3& p, Vec3& out)
{
    const float x = m.m[8] * p.z + m.m[4] * p.y + p.x * m.m[0] + m.m[12];
    const float y = m.m[1] * p.x + m.m[9] * p.z + m.m[5] * p.y + m.m[13];
    const float z = m.m[2] * p.x + m.m[10] * p.z + m.m[6] * p.y + m.m[14];
    out.x = x; out.y = y; out.z = z;
    return out;
}

// sub_10004230, 83 B: out = v * m (vector: rotation only).
inline Vec3& transformVector(const Mat4& m, const Vec3& v, Vec3& out)
{
    const float x = m.m[8] * v.z + m.m[4] * v.y + v.x * m.m[0];
    const float y = m.m[1] * v.x + m.m[9] * v.z + m.m[5] * v.y;
    const float z = m.m[2] * v.x + m.m[10] * v.z + m.m[6] * v.y;
    out.x = x; out.y = y; out.z = z;
    return out;
}

// sub_10012740, 80 B: v = v * m3x3 in place (rotation only) - OpenALSample::update turns the
// source direction with the listener frame this way.
inline Vec3& transformVectorInPlace(const Mat4& m, Vec3& v)
{
    const float z = m.m[10] * v.z + m.m[2] * v.x + m.m[6] * v.y;
    const float y = m.m[9] * v.z + m.m[1] * v.x + m.m[5] * v.y;
    const float x = m.m[8] * v.z + m.m[4] * v.y + v.x * m.m[0];
    v.x = x; v.y = y; v.z = z;
    return v;
}

// sub_1000DAD0, 162 B: the inverse of a rigid transform - rotation transposed, translation
// negated through the transposed rotation, last column (0,0,0,1).
inline Mat4& setInverseRigid(Mat4& out, const Mat4& src)
{
    out.m[0] = src.m[0]; out.m[1] = src.m[4]; out.m[2]  = src.m[8];
    out.m[4] = src.m[1]; out.m[5] = src.m[5]; out.m[6]  = src.m[9];
    out.m[8] = src.m[2]; out.m[9] = src.m[6]; out.m[10] = src.m[10];
    out.m[12] = -(src.m[12] * out.m[0] + out.m[4] * src.m[13] + src.m[14] * out.m[8]);
    out.m[13] = -(src.m[14] * out.m[9] + out.m[5] * src.m[13] + src.m[12] * out.m[1]);
    out.m[14] = -(src.m[14] * out.m[10] + src.m[13] * out.m[6] + src.m[12] * out.m[2]);
    out.m[11] = 0.0f; out.m[7] = 0.0f; out.m[3] = 0.0f; out.m[15] = 1.0f;
    return out;
}

// sub_100127D0, 79 B: any component not finite.
inline bool hasNaN(const Vec3& v)
{
    return !_finite(v.x) || !_finite(v.y) || !_finite(v.z);
}

// sub_10015770, 82 B: component-wise equality within FLT_EPSILON.
inline bool nearlyEqual(const Vec3& a, const Vec3& b)
{
    return std::fabs(a.x - b.x) <= FLT_EPSILON
        && std::fabs(a.y - b.y) <= FLT_EPSILON
        && std::fabs(a.z - b.z) <= FLT_EPSILON;
}

} // namespace sndmath
} // namespace hfe
} // namespace dice

#endif // SOUND_SOUNDENGINE_SOUNDENGINEMATH_H
