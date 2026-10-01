// Vector operations used by the two-bone IK solver.

#ifndef ANIMATION_BONEANIMATION_IK2BSOLVER_AWVECTOR_H
#define ANIMATION_BONEANIMATION_IK2BSOLVER_AWVECTOR_H

#include <math.h>

#include "AwMath.h"

namespace maya {

class AwQuaternion;

class AwVector
{
public:
    AwVector() : x(0.0), y(0.0), z(0.0) {}
    AwVector(double xx, double yy, double zz) : x(xx), y(yy), z(zz) {}

    double& operator[](unsigned index) { return (&x)[index]; }
    double operator[](unsigned index) const { return (&x)[index]; }

    AwVector operator+(const AwVector& rhs) const
    {
        return AwVector(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    AwVector operator-(const AwVector& rhs) const
    {
        return AwVector(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    AwVector operator*(double scalar) const
    {
        return AwVector(x * scalar, y * scalar, z * scalar);
    }

    double operator*(const AwVector& rhs) const
    {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    AwVector operator^(const AwVector& rhs) const
    {
        return AwVector(y * rhs.z - z * rhs.y,
                        z * rhs.x - x * rhs.z,
                        x * rhs.y - y * rhs.x);
    }

    double dotProduct(const AwVector& rhs) const { return *this * rhs; }
    double norm() const { return x * x + y * y + z * z; }
    double length() const { return sqrt(norm()); }

    AwVector& normalize();
    AwVector rotateBy(const AwQuaternion& rotation) const;
    bool isParallel(const AwVector& rhs,
                    double tolerance = kDoubleEpsilon) const;
    unsigned dominantAxis() const;
    double angle(const AwVector& rhs) const;

    double x;
    double y;
    double z;
};

inline AwVector operator*(double scalar, const AwVector& vector)
{
    return vector * scalar;
}

} // namespace maya

#endif // ANIMATION_BONEANIMATION_IK2BSOLVER_AWVECTOR_H
