// Vector operations used by the two-bone IK solver.

#include "AwVector.h"

#include "AwQuaternion.h"

namespace maya {

AwVector& AwVector::normalize()
{
    const double squaredLength = norm();
    if (squaredLength > kDoubleEpsilonSqr
        && !equivalent(squaredLength, 1.0, 2.0 * kDoubleEpsilon))
    {
        const double scale = 1.0 / sqrt(squaredLength);
        x *= scale;
        y *= scale;
        z *= scale;
    }
    return *this;
}

AwVector AwVector::rotateBy(const AwQuaternion& q) const
{
    const double rw = -q.x * x - q.y * y - q.z * z;
    const double rx =  q.w * x + q.y * z - q.z * y;
    const double ry =  q.w * y + q.z * x - q.x * z;
    const double rz =  q.w * z + q.x * y - q.y * x;

    return AwVector(-rw * q.x + rx * q.w - ry * q.z + rz * q.y,
                    -rw * q.y + ry * q.w - rz * q.x + rx * q.z,
                    -rw * q.z + rz * q.w - rx * q.y + ry * q.x);
}

bool AwVector::isParallel(const AwVector& rhs, double tolerance) const
{
    AwVector lhsNormal(*this);
    AwVector rhsNormal(rhs);
    lhsNormal.normalize();
    rhsNormal.normalize();
    return equivalent(fabs(lhsNormal * rhsNormal), 1.0, tolerance);
}

unsigned AwVector::dominantAxis() const
{
    const double absX = fabs(x);
    const double absY = fabs(y);
    const double absZ = fabs(z);
    if (absX > absY)
        return absX > absZ ? 0u : 2u;
    return absY > absZ ? 1u : 2u;
}

double AwVector::angle(const AwVector& rhs) const
{
    AwVector lhsNormal(*this);
    AwVector rhsNormal(rhs);
    lhsNormal.normalize();
    rhsNormal.normalize();

    const double cosine = lhsNormal * rhsNormal;
    if (cosine >= 1.0)
        return 0.0;
    if (cosine <= -1.0)
        return kPi;
    return acos(cosine);
}

} // namespace maya
