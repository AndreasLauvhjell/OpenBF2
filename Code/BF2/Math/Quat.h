// [INFERRED] Source placement.

// Quaternion stored as x, y, z, w. Identity has w = 1 and zero vector components.

#ifndef MATH_QUAT_H                 // [INFERRED] guard style
#define MATH_QUAT_H

namespace dice {
namespace hfe {

struct Mat4;

// partial. sizeof is 16 from the layout above; fromMat is the one member recovered.
struct Quat
{
    /* +0x00 */ float x;
    /* +0x04 */ float y;
    /* +0x08 */ float z;
    /* +0x0C */ float w;    // last — see the header comment; the identity pre-fill proves it


    // Extracts a unit quaternion from the rotational part of m.

    void fromMat(const Mat4& m);


    // Writes this quaternion's rotation to out.

    void toMat(Mat4& out) const;


    // Interpolates rotations a and b using spherical interpolation at t.

    void slerp(const Quat& a, const Quat& b, float t);
};

} // namespace hfe
} // namespace dice

#endif
