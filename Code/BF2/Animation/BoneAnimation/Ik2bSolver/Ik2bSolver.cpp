// Float adapter for the Maya two-bone IK solver used by Skeleton.

#include "Ik2bSolver.h"

#include <math.h>

#include "AwMath.h"
#include "AwMatrix.h"
#include "AwPoint.h"
#include "AwQuaternion.h"
#include "AwVector.h"

namespace maya {

void solveIK(const AwPoint& startJointPosition,
             const AwPoint& midJointPosition,
             const AwPoint& effectorPosition,
             const AwPoint& handlePosition,
             const AwVector& poleVector,
             double twist,
             AwQuaternion& startJointRotation,
             AwQuaternion& midJointRotation)
{
    AwVector firstSegment = midJointPosition - startJointPosition;
    AwVector secondSegment = effectorPosition - midJointPosition;
    AwVector handleVector = handlePosition - startJointPosition;
    AwVector effectorVector = effectorPosition - startJointPosition;

    const double firstLength = firstSegment.length();
    const double secondLength = secondSegment.length();
    const double handleLength = handleVector.length();

    const AwVector orthogonal =
        firstSegment - effectorVector * ((firstSegment * effectorVector)
                                       / (effectorVector * effectorVector));

    const double currentAngle = firstSegment.angle(secondSegment);
    const AwVector bendAxis = firstSegment ^ secondSegment;
    const double handleLengthSquared = handleLength * handleLength;

    double extensionCosine =
        (handleLengthSquared - firstLength * firstLength - secondLength * secondLength)
        / (2.0 * firstLength * secondLength);
    if (extensionCosine > 1.0)
        extensionCosine = 1.0;
    else if (extensionCosine < -1.0)
        extensionCosine = -1.0;

    const double extensionAngle = acos(extensionCosine);
    const AwQuaternion bendRotation(extensionAngle - currentAngle, bendAxis);

    secondSegment = secondSegment.rotateBy(bendRotation);
    effectorVector = firstSegment + secondSegment;
    const AwQuaternion handleRotation(effectorVector, handleVector);

    firstSegment = firstSegment.rotateBy(handleRotation);
    if (firstSegment.isParallel(handleVector))
        firstSegment = orthogonal.rotateBy(handleRotation);

    AwQuaternion planeRotation;
    if (!poleVector.isParallel(handleVector) && handleLengthSquared != 0.0)
    {
        const AwVector planeNormal =
            firstSegment - handleVector * ((firstSegment * handleVector) / handleLengthSquared);
        const AwVector projectedPole =
            poleVector - handleVector * ((poleVector * handleVector) / handleLengthSquared);
        const double planeDot = (planeNormal * projectedPole)
                              / (planeNormal.length() * projectedPole.length());

        if (absoluteValue(planeDot + 1.0) < kEpsilon)
            planeRotation = AwQuaternion(kPi, handleVector);
        else
            planeRotation = AwQuaternion(planeNormal, projectedPole);
    }

    const AwQuaternion twistRotation(twist, handleVector);
    midJointRotation = bendRotation;
    startJointRotation = handleRotation * planeRotation * twistRotation;
}

void applyIK2BoneSolver(const dice::hfe::Vec3& pole,
                        const dice::hfe::Vec3& start,
                        const dice::hfe::Vec3& mid,
                        const dice::hfe::Vec3& effector,
                        const dice::hfe::Vec3& handle,
                        dice::hfe::Mat4& startRotation,
                        dice::hfe::Mat4& midRotation)
{
    const AwPoint startPoint(start.x, start.y, start.z);
    const AwPoint midPoint(mid.x, mid.y, mid.z);
    const AwPoint effectorPoint(effector.x, effector.y, effector.z);
    const AwPoint handlePoint(handle.x, handle.y, handle.z);
    const AwVector poleVector(pole.x, pole.y, pole.z);

    AwQuaternion startQuaternion;
    AwQuaternion midQuaternion;
    solveIK(startPoint, midPoint, effectorPoint, handlePoint, poleVector, 0.0,
            startQuaternion, midQuaternion);

    AwMatrix startMatrix;
    startQuaternion.convertToMatrix(startMatrix);

    AwVector midAxis;
    double midAngle = 0.0;
    midQuaternion.getAxisAngle(midAxis, midAngle);
    midQuaternion.setAxisAngle(midAxis, midAngle * 0.5);
    AwMatrix midMatrix;
    midQuaternion.convertToMatrix(midMatrix);

    for (int row = 0; row != 4; ++row)
    {
        for (int column = 0; column != 4; ++column)
        {
            const int index = row * 4 + column;
            startRotation.m[index] = static_cast<float>(startMatrix.matrix[row][column]);
            midRotation.m[index] = static_cast<float>(midMatrix.matrix[row][column]);
        }
    }
}

} // namespace maya
