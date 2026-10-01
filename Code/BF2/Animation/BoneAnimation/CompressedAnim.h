// [INFERRED] Source placement.

// Decoder for compressed .baf animation samples.


#ifndef ANIMATION_BONEANIMATION_COMPRESSEDANIM_H     // [INFERRED] guard style
#define ANIMATION_BONEANIMATION_COMPRESSEDANIM_H

namespace dice {
namespace hfe {
    struct Quat;
    namespace Math { struct Vec3; }     // the same forward form BoneAnimation.h uses
    typedef Math::Vec3 Vec3;
}
namespace anim {


// Six-channel table per bone, with 16-bit quantized samples. [CHOSEN] Field names.

struct BoneData
{
    /* +0x00 */ unsigned short channelEnd[6];   // cumulative sample offsets after channels 0..5
    /* +0x0C */ unsigned short* samples;        // owned; BoneAnimation::cleanup() delete[]s it
};

struct CompressedAnim
{
    /* +0x00 */ unsigned short frameCount;      // narrowed from the file's 32 bits by load
    /* +0x02 */ unsigned char  boneCount;       // narrowed to a byte by load
    /* +0x03 */ unsigned char  precisionBits;   // ctor 15; load overwrites it from the file
    /* +0x04 */ BoneData*      boneData;        // ctor 0; load: new BoneData[boneCount]

    // Non-const, as mangled (_ZN..., not _ZNK...).
    float GetValue(int bone, int frame, int channel);
    void  GetQuat(int bone, int frame, hfe::Quat& out);          // channels 0..3
    void  GetTrans(int bone, int frame, hfe::Vec3& out);         // channels 4..6
};

} // namespace anim
} // namespace dice

#endif // ANIMATION_BONEANIMATION_COMPRESSEDANIM_H
