// Quaternion conversion and interpolation. [INFERRED] Source placement.

#include <cmath>          // acosf / sinf - slerp

#include "Quat.h"

#include <math.h>

#include "Mat4.h"

namespace dice {
namespace hfe {

namespace {
const int nxt[3] = { 1, 2, 0 };
} // namespace

// See the header for the shape and for the one shipped oddity.
void Quat::fromMat(const Mat4& m)
{
    const float trace = m.m[0] + m.m[5] + m.m[10];

    if (trace > 0.0f)
    {
        const float s = sqrtf(trace + 1.0f);
        const float t = 0.5f / s;
        w = 0.5f * s;
        x = (m.m[9] - m.m[6]) * t;
        y = (m.m[2] - m.m[8]) * t;
        z = t * (m.m[4] - m.m[1]);
    }
    else
    {
        // The largest diagonal element decides which component is built first.
        int i = (m.m[5] > m.m[0]) ? 1 : 0;
        if (m.m[10] > m.m[5 * i])
            i = 2;
        const int j = nxt[i];
        const int k = nxt[j];

        float s = sqrtf(m.m[5 * i] - (m.m[5 * k] + m.m[5 * j]) + 1.0f);

        float* const q = &x;                                 // x/y/z are indexable; w is separate
        q[i] = s * 0.5f;

        // the reciprocal is conditional - see the header. A tiny s is multiplied by, not
        // divided by, and the normalise below turns the result into the identity.
        if (fabsf(s) > 0.00000023841858f)
            s = 0.5f / s;

        w    = (m.m[4 * k + j] - m.m[4 * j + k]) * s;
        q[j] = (m.m[4 * i + j] + m.m[4 * j + i]) * s;
        q[k] = s * (m.m[4 * i + k] + m.m[4 * k + i]);
    }

    const float length = sqrtf(w * w + x * x + y * y + z * z);
    if (length >= 0.00000011920929f)                         // FLT_EPSILON
    {
        w /= length;
        x /= length;
        y /= length;
        z /= length;
    }
    else
    {
        w = 1.0f;                                            // the identity, never a NaN
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }
}


// Converts using the self-normalizing quaternion factor; preserve interleaved row-store order.

void Quat::toMat(Mat4& out) const
{
    // two over the squared norm - not the textbook bare 2.
    const float s = 2.0f / (y * y + x * x + z * z + w * w);

    const float xs = x * s, ys = y * s, zs = z * s;
    const float xx = x * xs, yy = y * ys, zz = z * zs;
    const float xy = x * ys, xz = x * zs, yz = z * ys;
    const float xw = w * xs, yw = w * ys, zw = w * zs;

    out.m[0]  = 1.0f - (yy + zz);
    out.m[1]  = xy - zw;
    out.m[2]  = xz + yw;
    out.m[3]  = 0.0f;

    out.m[4]  = xy + zw;
    out.m[5]  = 1.0f - (zz + xx);
    out.m[6]  = yz - xw;
    out.m[7]  = 0.0f;

    out.m[8]  = xz - yw;
    out.m[9]  = xw + yz;
    out.m[10] = 1.0f - (xx + yy);
    out.m[11] = 0.0f;

    // the translation ROW is zeroed and m[15] set - toMat owns the whole matrix.
    out.m[12] = 0.0f;
    out.m[13] = 0.0f;
    out.m[14] = 0.0f;
    out.m[15] = 1.0f;
}


// Interpolates a to b along the short arc, using linear interpolation near coincident rotations.

void Quat::slerp(const Quat& a, const Quat& b, float t)
{
    const float dot = b.y * a.y + b.x * a.x + b.z * a.z + a.w * b.w;

    float scale0;
    float scale1;

    if (dot >= 0.0f)
    {
        if (1.0f - dot <= 0.000001f)
        {
            scale0 = 1.0f - t;                       // a plain lerp - see the header
            scale1 = t;
        }
        else
        {
            float c = dot;
            if (dot < -1.0f)      c = -1.0f;         // dead here: dot >= 0
            else if (dot > 1.0f)  c = 1.0f;

            const float theta = acosf(c);
            const float sinTheta = sinf(theta);
            scale0 = sinf((1.0f - t) * theta) / sinTheta;
            scale1 = sinf(t * theta) / sinTheta;
        }

        w = a.w * scale0 + b.w * scale1;
        x = a.x * scale0 + b.x * scale1;
        y = a.y * scale0 + b.y * scale1;
        z = a.z * scale0 + b.z * scale1;
    }
    else
    {
        // the short WAY round: work on -dot and subtract the second term.
        if (dot + 1.0f <= 0.000001f)
        {
            scale0 = 1.0f - t;
            scale1 = t;
        }
        else
        {
            float c = -dot;
            if (c < -1.0f)      c = -1.0f;
            else if (c > 1.0f)  c = 1.0f;

            const float theta = acosf(c);
            const float sinTheta = sinf(theta);
            scale0 = sinf((1.0f - t) * theta) / sinTheta;
            scale1 = sinf(t * theta) / sinTheta;
        }

        w = a.w * scale0 - b.w * scale1;
        x = a.x * scale0 - b.x * scale1;
        y = a.y * scale0 - b.y * scale1;
        z = a.z * scale0 - b.z * scale1;
    }
}

} // namespace hfe
} // namespace dice
