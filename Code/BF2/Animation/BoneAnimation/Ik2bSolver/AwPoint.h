// Path:     [INFERRED] companion header for attested Ik2bSolver/AwPoint.cpp.

#ifndef ANIMATION_BONEANIMATION_IK2BSOLVER_AWPOINT_H
#define ANIMATION_BONEANIMATION_IK2BSOLVER_AWPOINT_H

#include "AwVector.h"

namespace maya {

class AwPoint
{
public:
    AwPoint() : x(0.0), y(0.0), z(0.0), w(1.0) {}
    AwPoint(double xx, double yy, double zz, double ww = 1.0)
        : x(xx), y(yy), z(zz), w(ww) {}

    AwVector operator-(const AwPoint& rhs) const
    {
        if (w == 1.0 && rhs.w == 1.0)
            return AwVector(x - rhs.x, y - rhs.y, z - rhs.z);
        return cartesianSub(rhs);
    }

    AwPoint cartesian() const
    {
        AwPoint result(*this);
        result.cartesianize();
        return result;
    }

    AwPoint& cartesianize();

    double x;
    double y;
    double z;
    double w;

private:
    AwVector cartesianSub(const AwPoint& rhs) const;
};

} // namespace maya

#endif // ANIMATION_BONEANIMATION_IK2BSOLVER_AWPOINT_H
