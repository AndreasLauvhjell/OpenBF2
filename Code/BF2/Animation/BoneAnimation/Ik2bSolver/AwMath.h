// Path:     [INFERRED] companion header for the attested Ik2bSolver translation units.
// Contents: the constants/helpers used by the recovered Maya two-bone IK sample.

#ifndef ANIMATION_BONEANIMATION_IK2BSOLVER_AWMATH_H
#define ANIMATION_BONEANIMATION_IK2BSOLVER_AWMATH_H

#include <math.h>

namespace maya {

static const double kPi               = 3.14159265358979323846;
static const float  kFloatEpsilon     = 1.0e-5f;
static const double kDoubleEpsilon    = 1.0e-10;
static const double kDoubleEpsilonSqr = 1.0e-20;
static const double kEpsilon          = 1.0e-5;

inline double absoluteValue(double value)
{
    return value < 0.0 ? -value : value;
}

inline bool equivalent(double lhs, double rhs, double tolerance = kDoubleEpsilon)
{
    return absoluteValue(lhs - rhs) <= tolerance;
}

inline double clamp(double value, double minimum, double maximum)
{
    return value < minimum ? minimum : (value > maximum ? maximum : value);
}

} // namespace maya

#endif // ANIMATION_BONEANIMATION_IK2BSOLVER_AWMATH_H
