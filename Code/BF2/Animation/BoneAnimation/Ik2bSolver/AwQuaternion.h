// Path:     [INFERRED] companion header for attested Ik2bSolver/AwQuaternion.cpp.

#ifndef ANIMATION_BONEANIMATION_IK2BSOLVER_AWQUATERNION_H
#define ANIMATION_BONEANIMATION_IK2BSOLVER_AWQUATERNION_H

namespace maya {

class AwMatrix;
class AwVector;

class AwQuaternion
{
public:
    AwQuaternion() : x(0.0), y(0.0), z(0.0), w(1.0) {}
    AwQuaternion(double xx, double yy, double zz, double ww)
        : x(xx), y(yy), z(zz), w(ww) {}
    AwQuaternion(const AwVector& from, const AwVector& to);
    AwQuaternion(double angle, const AwVector& axis)
        : x(0.0), y(0.0), z(0.0), w(1.0)
    {
        setAxisAngle(axis, angle);
    }

    AwQuaternion operator*(const AwQuaternion& rhs) const;
    void convertToMatrix(AwMatrix& matrix) const;
    AwQuaternion& setAxisAngle(const AwVector& axis, double angle);
    bool getAxisAngle(AwVector& axis, double& angle) const;

    static const AwQuaternion identity;

    double x;
    double y;
    double z;
    double w;
};

} // namespace maya

#endif // ANIMATION_BONEANIMATION_IK2BSOLVER_AWQUATERNION_H
