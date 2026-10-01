// Path:     attested. "Animation/BoneAnimation/Ik2bSolver/AwPoint.cpp" occurs as __FILE__ in ELF.
// Contents: only the two AwPoint bodies reachable from the recovered two-bone IK solve.

#include "AwPoint.h"

#include <assert.h>

namespace maya {

AwVector AwPoint::cartesianSub(const AwPoint& rhs) const
{
    const AwPoint lhsCartesian = cartesian();
    const AwPoint rhsCartesian = rhs.cartesian();
    return AwVector(lhsCartesian.x - rhsCartesian.x,
                    lhsCartesian.y - rhsCartesian.y,
                    lhsCartesian.z - rhsCartesian.z);
}

AwPoint& AwPoint::cartesianize()
{
    if (w != 1.0)
    {
        assert(w != 0.0);
        const double inverseW = 1.0 / w;
        x *= inverseW;
        y *= inverseW;
        z *= inverseW;
        w = 1.0;
    }
    return *this;
}

} // namespace maya
