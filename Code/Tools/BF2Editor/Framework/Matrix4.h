// Code/Tools/BF2Editor/Framework/Matrix4.h

// Small 4x4 helpers for the editor's viewport maths, on plain float[16] in the engine's layout:
// row-major, row vectors (p' = p * M), translation in elements 12..14 - the layout of Math/Mat4.h and
// of ImGuizmo. Engine-free (checked in bf2_editor_checks). Editor-only code (D3).
#ifndef BF2EDITOR_FRAMEWORK_MATRIX4_H
#define BF2EDITOR_FRAMEWORK_MATRIX4_H

#include "Framework/EditorConfig.h"

namespace bf2editor
{

// out = a * b. `out` may not alias `a` or `b`.
inline void matrixMultiply(const float a[16], const float b[16], float out[16])
{
    for (int row = 0; row < 4; ++row)
        for (int column = 0; column < 4; ++column)
        {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k)
                sum += a[row * 4 + k] * b[k * 4 + column];
            out[row * 4 + column] = sum;
        }
}

// The inverse of an affine matrix (last column 0, 0, 0, 1) that may carry scale or shear: the 3x3
// block's inverse, and the translation carried through it with the sign turned. False when the block
// is singular; `out` is then untouched.
inline bool matrixInvertAffine(const float m[16], float out[16])
{
    const float a = m[0], b = m[1], c = m[2];
    const float d = m[4], e = m[5], f = m[6];
    const float g = m[8], h = m[9], i = m[10];
    const float c00 = e * i - f * h, c01 = c * h - b * i, c02 = b * f - c * e;
    const float c10 = f * g - d * i, c11 = a * i - c * g, c12 = c * d - a * f;
    const float c20 = d * h - e * g, c21 = b * g - a * h, c22 = a * e - b * d;
    const float determinant = a * c00 + b * c10 + c * c20;
    if (determinant == 0.0f)
        return false;
    const float s = 1.0f / determinant;
    out[0] = c00 * s; out[1] = c01 * s; out[2] = c02 * s; out[3] = 0.0f;
    out[4] = c10 * s; out[5] = c11 * s; out[6] = c12 * s; out[7] = 0.0f;
    out[8] = c20 * s; out[9] = c21 * s; out[10] = c22 * s; out[11] = 0.0f;
    for (int column = 0; column < 3; ++column)
        out[12 + column] = -(m[12] * out[column] + m[13] * out[4 + column] + m[14] * out[8 + column]);
    out[15] = 1.0f;
    return true;
}

// A group moved with a pivot: every member keeps its placement relative to the pivot. Given a member's
// matrix and the pivot's when the move began, and the pivot's now, the member's now is
// start * inverse(pivotStart) * pivot. False when pivotStart is singular.
inline bool matrixFollowPivot(const float start[16], const float pivotStart[16], const float pivot[16], float out[16])
{
    float inverse[16];
    if (!matrixInvertAffine(pivotStart, inverse))
        return false;
    float change[16];
    matrixMultiply(inverse, pivot, change);
    matrixMultiply(start, change, out);
    return true;
}

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_MATRIX4_H
