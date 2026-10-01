// Path:     [INFERRED] public seam for the BF2 adapter around the Maya two-bone solver.

#ifndef ANIMATION_BONEANIMATION_IK2BSOLVER_IK2BSOLVER_H
#define ANIMATION_BONEANIMATION_IK2BSOLVER_IK2BSOLVER_H

#include "../../../Math/Mat4.h"
#include "../../../Math/Vec3.h"

namespace maya {

class AwPoint;
class AwQuaternion;
class AwVector;

void solveIK(const AwPoint& startJointPosition,
             const AwPoint& midJointPosition,
             const AwPoint& effectorPosition,
             const AwPoint& handlePosition,
             const AwVector& poleVector,
             double twist,
             AwQuaternion& startJointRotation,
             AwQuaternion& midJointRotation);

void applyIK2BoneSolver(const dice::hfe::Vec3& poleVector,
                        const dice::hfe::Vec3& startJointPosition,
                        const dice::hfe::Vec3& midJointPosition,
                        const dice::hfe::Vec3& effectorPosition,
                        const dice::hfe::Vec3& handlePosition,
                        dice::hfe::Mat4& startJointRotation,
                        dice::hfe::Mat4& midJointRotation);

} // namespace maya

#endif // ANIMATION_BONEANIMATION_IK2BSOLVER_IK2BSOLVER_H
