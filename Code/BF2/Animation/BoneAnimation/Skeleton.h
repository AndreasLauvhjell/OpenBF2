// Skeleton hierarchy used for animation, pose transforms and collision queries. [INFERRED] Header placement.


#ifndef ANIMATION_BONEANIMATION_SKELETON_H     // [INFERRED] guard style
#define ANIMATION_BONEANIMATION_SKELETON_H

#include <string>
#include <vector>

#include "../../Math/Mat4.h"
#include "../../Math/Vec3.h"

namespace dice {
// Vec3 is a typedef in dice::hfe, not a struct - forward-declaring it as one is a hard
// error. Include the header instead. (Quat really is a struct and can be declared.)
namespace hfe { namespace io { class IStream; } struct Quat; }
namespace anim {

class BoneAnimation;

class Skeleton
{
public:
    // Inverse-kinematics handle describing the driven bone, effector and target.
    struct IkHandle
    {
        /* +0x00 */ hfe::Vec3 m_target;
        /* +0x0C */ hfe::Mat4 m_transform;
        /* +0x4C */ int       m_boneIndex;
        /* +0x50 */ int       m_effectorBoneIndex;
    };


    // Animation sample and weights queued for a skeleton bone.

    struct ApplyStage
    {
        /* +0x00 */ float m_weight;          // >= 1.0 wipes the bone's earlier stages - see the .cpp
        /* +0x04 */ BoneAnimation* m_animA;
        /* +0x08 */ float m_timeA;
        /* +0x0C */ float m_blend;           // only read when m_animB is set
        /* +0x10 */ float m_weightFactor;    // multiplied into the skeleton-wide running weight
        /* +0x14 */ BoneAnimation* m_animB;  // null = the simple, single-animation form
        /* +0x18 */ float m_timeB;
    };


    // Per-bone pose and optional owned state.

    struct Bone
    {
        Bone();
        ~Bone();

        /* +0x00 */ hfe::Mat4 m_transform;                     // ctor: identity - the bone's local
                                                               // Owned rotation override. Child-bone translation remains derived from the unmodified parent/local product.
        /* +0x40 */ hfe::Mat4* m_override;                     // ctor 0 - owned, see ~Bone
        // an index into the skeleton'S IK-handle array, -1 for "no IK".
        // A bone that has one is posed by applyIkSolver instead of by the hierarchy walk.
        /* +0x44 */ char  m_ikHandleIndex;                     // ctor -1
        // does this bone take part in transform() at all. False skips the
        // whole computation and leaves the bone inheriting its parent's position only.
        /* +0x45 */ bool  m_transformEnabled;                  // ctor true
    };


    // Template bone hierarchy and initial pose.

    struct BoneTemplate
    {
        BoneTemplate();

        /* +0x00 */ int   m_index;                             // ctor -1; the parser writes the
                                                               // bone's own ordinal
        /* +0x04 */ short m_nameId;                            // ctor -1; BoneManager::addBoneName's
                                                               // answer - names are interned, not
                                                               // stored per skeleton
        /* +0x06 */ short m_parentIndex;                       // ctor -1; -1 means a root
        /* +0x08 */ int   m_depth;                             // ctor 0; hops to a root, computed
                                                               // in a second pass after the load
        // by getCollisionBoneTransform: the collision capsule'S offset from
        // the bone, added to the capsule matrix before the +-90 degree X rotation (Z negated
        // when m_float18 is not negative). Three zero stores in the ctor. [CHOSEN] name.
        /* +0x0C */ hfe::Vec3 m_collisionOffset;               // ctor (0, 0, 0)
        /* +0x18 */ float m_float18;                           // signed collision-capsule length
        /* +0x1C */ float m_float1C;                           // collision-capsule radius
        /* +0x20 */ int   m_dword20;                           // material / hit-part id; ctor omits it
    };

    Skeleton();                                                // `new(0x40)`

    // the .ske parser. Loads a skeleton from `name` (or from an already-open
    // stream), sets `*loaded` on success and leaves it false on any failure. See Skeleton.cpp for
    // the file format, which this is the only recovered description of.
    Skeleton(const std::string& name, bool* loaded, hfe::io::IStream* stream);
    ~Skeleton();
    // . It clones the bone array and reallocates the transform array;
    // read the shipped-defect note in Skeleton.cpp before relying on it.
    Skeleton& operator=(Skeleton& other);
    // . the pose evaluation - see the long note in Skeleton.cpp for what the
    // second parameter really is (a depth limit, not a bone index).
    void transform(const hfe::Mat4* parent, int maxDepth);

    // Transforms only the listed bones using parent as the starting transform.
    void transformUsedBones(const hfe::Mat4* parent, const std::vector<int>& usedBones);

    // Converts absolute poses to local bone transforms, applying transformation to each root.
    void calculateRelativeTransforms(const hfe::Mat4& transformation);

    // The collision-bone authoring chain used by SoldierTemplate's five-argument console method.
    // getBoneIndex returns a short (both callers explicitly sign-extend AX), despite indexing an
    // int-sized bone count. The string overload narrows BoneManager's name id before delegating.
    short getBoneIndex(short nameId) const;
    void setSkeletonCollisionBone(short nameId, int material, const hfe::Vec3& offset,
                                  float radius, float length);
    void setSkeletonCollisionBone(const std::string& name, int material,
                                  const hfe::Vec3& offset, float radius, float length);

    // transform's first act. Collapses each bone's staged animations into a
    // single pose and writes it back into the bone's local transform.
    void executeAnimations();

    // all three the staging layer is complete. The two fan-outs differ in
    // one place only, and it is not cosmetic: the blended one skips the 0xFFFF sentinel in the
    // bone-index list and the simple one does not. See Skeleton.cpp.
    void buildApplyAnimationData(bool clearQueue);
    void applyBlendedAnimationStage(const ApplyStage& stage);
    void applySimpleAnimationStage(const ApplyStage& stage);
    // the entry points of the staging layer. BundlePlayer pushes one
    // ApplyStage per call onto m_pendingStages; transform() consumes the queue. The weight is
    // clamped to 1.0 on the way in and a non-positive weight is not queued at all.
    void applyAnimation(BoneAnimation* animation, float time, float weight,
                        float weightFactor);
    void applyAnimations(BoneAnimation* animationA, BoneAnimation* animationB,
                         float timeA, float timeB, float blend, float weight,
                         float weightFactor);
    void calculateTransformed(int boneIndex, BoneAnimation* animation, float time,
                              hfe::Quat& outRotation, hfe::Vec3& outPosition);
    // Writes a collision bone's transformed matrix. The bone argument is unused.
    void getCollisionBoneTransform(hfe::Mat4* out, const hfe::Mat4& transformation,
                                   int boneIndex, const Bone& bone) const;

    // Tests a ray against the selected collision skeleton and writes hit details on success.
    bool checkRayVsSkeleton(hfe::Vec3* hitPosition, hfe::Vec3* hitNormal,
                            int* hitPart, int* hitBone, float* fraction,
                            const hfe::Vec3& start, const hfe::Vec3& end,
                            const hfe::Mat4& transformation);
    // and it is what Soldier::resetInstance calls on the collision rig.
    void clearAllIk();
    // solves and distributes the two-bone rotation over the three preceding
    // joints, rebuilds their translations, and installs the requested effector transform.
    // Returns the last posed bone, which transform uses to advance past a child effector.
    int applyIkSolver(const IkHandle& handle);
    // Creates the bone's IK handle on first use, then stores its target and transform.
    void applyIk(int boneIndex, int effectorBoneIndex, const hfe::Vec3& target,
                 const hfe::Mat4& transform);

    // . Frees and nulls every bone's m_override. Its caller
    // is world::Soldier::onEnter, and it is live on a dedicated server: a soldier being parked in a
    // vehicle does have a collision skeleton, so this runs on every vehicle entry. See the header
    // note above on why a post-absolute channel exists at all.
    void removeAllPostAbsoluteBoneTransforms();
    static void initMemory();
    static void closeMemory();


    // the layout, 64 bytes, read from the default constructor. three 12-byte blocks at +0x18,
    // +0x24 and +0x30, each zeroed as three words - the std::vector shape, three of them in a row.
    // +0x14 is the one word the default constructor does not write. Everything else is zeroed
    // except +0x00, which gets 1.0f.

    /* +0x00 */ float m_float00;                               // ctor 1.0f [UNRESOLVED] - a scale?
    // by operator=: a `new[]`-allocated array of Bone with a leading
    // element-count cookie, one entry per bone.
    /* +0x04 */ Bone* m_bones;
    // Owned bone-template array. Assignment copies this pointer, so assigned skeletons share storage and destruction requires care.
    /* +0x08 */ BoneTemplate* m_boneTemplates;
    // The .ske parser sets it to 1 once the two arrays exist; the default constructor leaves it
    // false. [CHOSEN] name - it reads as "this skeleton owns loaded arrays".
    /* +0x0C */ bool  m_hasLoadedArrays;
    // . This tree carried the pair as [CHOSEN] names "m_transforms /
    // m_transformCount"; operator= settles both. +0x10 is `new[](boneCount * 64)` - one Mat4 PER
    // bone, each initialised to identity - and +0x14 is the bone count itself, copied from the
    // source skeleton before anything is allocated. The chosen names were right.
    /* +0x10 */ hfe::Mat4* m_transforms;
    /* +0x14 */ int   m_boneCount;                             // not written by the default ctor
    // by Skeleton::transform, which indexes it as `m_ikHandles + 84 * n`:
    // the IK-handle array, 84 bytes per element, indexed by Bone::m_ikHandleIndex.
    // operator= does not copy it - an assigned skeleton keeps the destination's handles.
    /* +0x18 */ std::vector<IkHandle> m_ikHandles;
    /* +0x24 */ std::vector<int> m_vector24;                   // collision-bone indices; populated by
                                                               // Pending animation stages; assignment preserves the destination queue.
    /* +0x30 */ std::vector<ApplyStage> m_pendingStages;
    /* +0x3C */ bool  m_byte3C;                                // [UNRESOLVED]
};

} // namespace anim
} // namespace dice

#endif // ANIMATION_BONEANIMATION_SKELETON_H
