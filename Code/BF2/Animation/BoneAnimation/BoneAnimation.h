
// Shared bone animation cached by name. Bundles borrow it; the animation manager controls its lifetime.


#ifndef ANIMATION_BONEANIMATION_BONEANIMATION_H     // [INFERRED] guard style
#define ANIMATION_BONEANIMATION_BONEANIMATION_H

#include <string>

#include "CompressedAnim.h"

namespace dice {

namespace hfe {
    struct Quat;
    namespace Math { struct Vec3; }
    typedef Math::Vec3 Vec3;
    namespace io { class IStream; }
}

namespace anim {

class Skeleton;

// Per-bone channel records are declared with CompressedAnim.

class BoneAnimation
{
public:
    BoneAnimation();
    ~BoneAnimation();

    // the .baf reader. A missing file is DEBUGTYPE_6 - Error, which abort()s - so a .baf named
    // by a .inc and absent from the archives kills the server. Line 122, and that Debug's literal
    // is what attests this file's path.
    bool load(const std::string& path);                           // [FRONTIER]
    bool reload(const std::string& path);                         // [FRONTIER]
    void makeScript(hfe::io::IStream* stream);

    // it precomputes the reciprocal, and it does it with no guard: `m_length = v; m_invLength =
    // 1.0f / v`. A .inc line `animationBundle.setAnimationLength x 0` divides by zero here.
    void setLength(float length);
    void cleanup();

    // the runtime samplers. applyOnSkeleton is seven bytes and empty on this build.
    void applyOnSkeleton(Skeleton& skeleton, float a, float b, int c);
    // [FRONTIER] Single-bone sampling also requires Skeleton::setRelativeBoneTransform.
    void applyOnSkeletonBone(Skeleton& skeleton, int bone,
                             float time, float weight, int c);   // [FRONTIER]
    // [FRONTIER] - no caller in the image either; the same body as getValue minus the epsilon
    // test and the root-bone jitter, returning whether the bone was found.
    bool getBonePose(int bone, float time, hfe::Quat& rotation, hfe::Vec3& position);
                                                                  // [FRONTIER]
    // the sampler. Called by Skeleton::executeAnimations for every staged
    // bone and by Skeleton::calculateTransformed up the parent chain. Identity/zero for a bone
    // the animation does not carry. `jitter` scales the root bone's random shake (see the .cpp).
    void getValue(int bone, float time, hfe::Quat& rotation, hfe::Vec3& position, float jitter);
    // not an ELF symbol - the sixty-instruction head the three samplers carry inline,
    // identically. Written once here; see the .cpp for the argument.
    void updateFrameCache(float time);


    // the layout, by `load`. It was a fourteen-store sketch for one sitting; the
    // reader is what gives the offsets meaning, and it named nine of them.

    // +0x00 is the "HAS DATA" gate. Bundle::adjustLength skips an animation whose +0x00 is zero;
    // ~BoneAnimation only calls cleanup() when it is set; and load returns early when it is set, so
    // an animation is read exactly once. Three readers, one meaning.
    /* +0x00 */ bool m_loaded;                 // ctor false; load sets it last
    /* +0x01 */ bool looping;                  // ctor true (ConsoleClass188)
    // resolved: load computes `frameCount / 24.0f` into this and then defaults `length` from it if
    // nothing had set one. So this is the animation's natural length and `length` is the length in
    // use - which is exactly what ConsoleClass187 `animationManager.originalLength` reads.
    /* +0x04 */ float originalLength;          // ctor 0.0f
    /* +0x08 */ float length;                  // ctor 0.0f (ConsoleClass186, via setLength)
    // not initialised by the constructor. setLength writes it, and so does load's default path.
    /* +0x0C */ float m_invLength;             // 1.0f / length
    // Blend-in duration in seconds, configured through animationManager.fadeInTime. [CHOSEN] Member name.
    /* +0x10 */ float m_blendInTime;           // ctor 0
    // the frame cache, by the three samplers: the last time sampled (-1
    // from the ctor, so the first call always recomputes), the two frames it fell between and
    // the fraction between them. One cache per animation, shared by every skeleton that samples
    // it - which is safe because every sampler recomputes when the time differs.
    /* +0x14 */ float m_lastTime;              // ctor -1.0f
    /* +0x18 */ int   m_frameA;                // not ctor-written; the cache fills all three
    /* +0x1C */ float m_frameFraction;
    /* +0x20 */ int   m_frameB;
    // Base-bone staging filter, set by animationManager.ignoreMotherOrientation; -1 disables it.
    /* +0x24 */ int   m_baseBone;              // ctor -1
    /* +0x28 */ int   m_boneCount;             // load: a u16 from the file, widened
    // the bone-ID list, straight from the file with no transformation. This is the array the
    // skeleton's staging layer walks (+0x2C, sixteen bits per entry).
    /* +0x2C */ unsigned short* m_boneIds;     // load: new u16[m_boneCount]
    // Maps bone ids to animation indices; 0xFFFF marks ids absent from this animation.
    /* +0x30 */ unsigned short* m_boneIdToIndex;
    // Each 16-bit value occupies a four-byte storage slot in the retail layout; preserve the explicit padding.
    /* +0x34 */ unsigned short m_maxBoneId;    // load: 0, then the running maximum
    /* +0x36 */ unsigned short m_hole36;       // [UNRESOLVED] - never read or written
    // initialised to 4711 - the Eau-de-Cologne joke constant, used here as an impossibly large
    // starting minimum. It is 0x1267 in the disassembly and it is unmistakable.
    /* +0x38 */ unsigned short m_minBoneId;    // load: 4711, then the running minimum
    /* +0x3A */ unsigned short m_hole3A;       // [UNRESOLVED] - never read or written
    // Compressed sample decoder. File frame and bone counts narrow to 16 and 8 bits respectively.
    /* +0x3C */ CompressedAnim m_compressed;
    // Root-bone jitter probability and amplitude, exposed by the original noice console properties. [CHOSEN] Member names.
    /* +0x44 */ int   m_jitterChance;          // ctor 0
    /* +0x48 */ float m_jitterAmplitude;       // ctor 0
    // resolved: load adds to it at every allocation and cleanup zeroes it. It is a byte total of
    // everything this animation owns - which is why it is the one field cleanup clears.
    /* +0x4C */ int m_memoryUsed;              // ctor 0
};

} // namespace anim
} // namespace dice

#endif // ANIMATION_BONEANIMATION_BONEANIMATION_H
