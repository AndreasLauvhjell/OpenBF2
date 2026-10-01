// Path:     attested. "Animation/BoneAnimation/Ik2bSolver/AwQuaternion.cpp" occurs as __FILE__.
// Contents: every AwQuaternion body reachable from maya::solveIK and its float-matrix adapter.

#include "AwQuaternion.h"

#include <math.h>

#include "AwMath.h"
#include "AwMatrix.h"
#include "AwVector.h"

namespace maya {

const AwQuaternion AwQuaternion::identity;

AwQuaternion::AwQuaternion(const AwVector& from, const AwVector& to)
    : x(0.0), y(0.0), z(0.0), w(1.0)
{
    const double factor = from.length() * to.length();
    if (fabs(factor) > kFloatEpsilon)
    {
        const double dot = from.dotProduct(to) / factor;
        const double angle = acos(clamp(dot, -1.0, 1.0));
        AwVector axis = from ^ to;

        if (dot < 0.0 && axis.length() < kFloatEpsilon)
        {
            const unsigned dominant = from.dominantAxis();
            axis[dominant] = -from[(dominant + 1) % 3];
            axis[(dominant + 1) % 3] = from[dominant];
            axis[(dominant + 2) % 3] = 0.0;
        }

        setAxisAngle(axis, angle);
    }
}

AwQuaternion AwQuaternion::operator*(const AwQuaternion& rhs) const
{
    AwQuaternion result;
    result.w = rhs.w * w - (rhs.x * x + rhs.y * y + rhs.z * z);
    result.x = rhs.w * x + rhs.x * w + rhs.y * z - rhs.z * y;
    result.y = rhs.w * y + rhs.y * w + rhs.z * x - rhs.x * z;
    result.z = rhs.w * z + rhs.z * w + rhs.x * y - rhs.y * x;
    return result;
}

void AwQuaternion::convertToMatrix(AwMatrix& out) const
{
    const double ww = w * w;
    const double xx = x * x;
    const double yy = y * y;
    const double zz = z * z;
    const double scale = 2.0 / (ww + xx + yy + zz);
    const double xy = x * y;
    const double xz = x * z;
    const double yz = y * z;
    const double wx = w * x;
    const double wy = w * y;
    const double wz = w * z;

    out.matrix[0][0] = 1.0 - scale * (yy + zz);
    out.matrix[1][0] = scale * (xy - wz);
    out.matrix[2][0] = scale * (xz + wy);
    out.matrix[3][0] = 0.0;
    out.matrix[0][1] = scale * (xy + wz);
    out.matrix[1][1] = 1.0 - scale * (xx + zz);
    out.matrix[2][1] = scale * (yz - wx);
    out.matrix[3][1] = 0.0;
    out.matrix[0][2] = scale * (xz - wy);
    out.matrix[1][2] = scale * (yz + wx);
    out.matrix[2][2] = 1.0 - scale * (xx + yy);
    out.matrix[3][2] = 0.0;
    out.matrix[0][3] = 0.0;
    out.matrix[1][3] = 0.0;
    out.matrix[2][3] = 0.0;
    out.matrix[3][3] = 1.0;
}

AwQuaternion& AwQuaternion::setAxisAngle(const AwVector& axis, double angle)
{
    const double squaredLength = axis.norm();
    if (squaredLength <= kDoubleEpsilon)
    {
        *this = identity;
    }
    else
    {
        angle *= 0.5;
        w = cos(angle);
        double factor = sin(angle);
        if (!equivalent(squaredLength, 1.0))
            factor /= sqrt(squaredLength);
        x = factor * axis.x;
        y = factor * axis.y;
        z = factor * axis.z;
    }
    return *this;
}

bool AwQuaternion::getAxisAngle(AwVector& axis, double& angle) const
{
    if (!equivalent(w, 1.0))
    {
        const double halfAngle = acos(clamp(w, -1.0, 1.0));
        angle = halfAngle * 2.0;
        const double inverseSin = 1.0 / sin(halfAngle);
        axis.x = x * inverseSin;
        axis.y = y * inverseSin;
        axis.z = z * inverseSin;
        return true;
    }

    angle = 0.0;
    if (axis.length() < kDoubleEpsilon)
        axis = AwVector(0.0, 0.0, 1.0);
    return false;
}

} // namespace maya
