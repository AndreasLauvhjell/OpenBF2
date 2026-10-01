// Path:     [INFERRED] header for the attested Ik2bSolver/AwMatrix.cpp family.
// Contents: the exact 128-byte double matrix shape needed by the two-bone solver.

#ifndef ANIMATION_BONEANIMATION_IK2BSOLVER_AWMATRIX_H
#define ANIMATION_BONEANIMATION_IK2BSOLVER_AWMATRIX_H

namespace maya {

class AwMatrix
{
public:
    AwMatrix()
    {
        matrix[0][0] = 1.0; matrix[0][1] = 0.0; matrix[0][2] = 0.0; matrix[0][3] = 0.0;
        matrix[1][0] = 0.0; matrix[1][1] = 1.0; matrix[1][2] = 0.0; matrix[1][3] = 0.0;
        matrix[2][0] = 0.0; matrix[2][1] = 0.0; matrix[2][2] = 1.0; matrix[2][3] = 0.0;
        matrix[3][0] = 0.0; matrix[3][1] = 0.0; matrix[3][2] = 0.0; matrix[3][3] = 1.0;
    }

    double matrix[4][4];
};

} // namespace maya

#endif // ANIMATION_BONEANIMATION_IK2BSOLVER_AWMATRIX_H
