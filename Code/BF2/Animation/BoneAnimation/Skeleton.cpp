// Bone hierarchy, animation blending, inverse kinematics, and collision poses.
// Native arrays use actual element size/alignment; file widths remain fixed.


#include <cctype>        // tolower - the .ske parser lowercases every bone name
#include <cmath>        // fabsf - the parser's epsilon scrub
#include <cfloat>       // FLT_EPSILON - collision-normal normalisation
#include <cstdio>       // [TEMP] fprintf/fflush - the staging-index probe
#include <cstdlib>      // [TEMP] abort       - the staging-index probe
#include <string>
#include <new>          // placement new - operator= constructs the bone array

#include "Skeleton.h"
#include "BoneManager.h"    // bone names are interned, not stored
#include "BoneAnimation.h"
#include "AnimationManager.h" // [TEMP] the staging-index probe asks it whether m_animA is registered
#include "Ik2bSolver/Ik2bSolver.h"

#include "../../Core/Debug.h"
#include "../../IO/File/File.h"          // the .ske parser opens its own file
#include "../../IO/Stream/IStream.h"     // ... or takes a stream it is handed
#include "../../Math/Quat.h"             // Quat::toMat / fromMat / slerp
#include "../../Math/Vec3.h"             // Quat::toMat - one call per bone
#include "../../Physics/GeomUtil.h"       // getIntersectionOfCapsAndEdgeNew

namespace dice {
namespace anim {

namespace {
// [CHOSEN, not recovered] Align native arrays while retaining a four-byte count before the first element.
// Allocate with sizeof(T); x86 keeps the original sizes. This storage is separate from the .ske format.
template<class T>
constexpr std::size_t skeletonArrayPrefix()
{
    return alignof(T) > sizeof(int) ? alignof(T) : sizeof(int);
}

template<class T>
T* newSkeletonArray(int count)
{
    unsigned char* block = static_cast<unsigned char*>(
        operator new[](sizeof(T) * count + skeletonArrayPrefix<T>()));
    unsigned char* first = block + skeletonArrayPrefix<T>();
    *reinterpret_cast<int*>(first - sizeof(int)) = count;
    T* values = reinterpret_cast<T*>(first);
    // Construct array elements in forward order.
    for (int i = 0; i < count; ++i)
        new (static_cast<void*>(&values[i])) T();
    return values;
}

template<class T>
void deleteSkeletonArray(T* values)
{
    unsigned char* first = reinterpret_cast<unsigned char*>(values);
    const int count = *reinterpret_cast<const int*>(first - sizeof(int));
    for (int i = count - 1; i >= 0; --i)
        values[i].~T();
    operator delete[](first - skeletonArrayPrefix<T>());
}

static_assert(sizeof(int) == 4, "Skeleton array count is four bytes");
static_assert(sizeof(Skeleton::BoneTemplate) == 36, "Skeleton metadata layout");
static_assert(sizeof(hfe::Mat4) == 64, "Skeleton transform layout");
#if defined(_WIN64) || defined(__LP64__)
static_assert(sizeof(Skeleton::Bone) == 80 && alignof(Skeleton::Bone) == 8,
              "Native x64 Skeleton bone storage");
#else
static_assert(sizeof(Skeleton::Bone) == 72 && alignof(Skeleton::Bone) == 4,
              "Retail x86 Skeleton bone layout");
#endif
}


// Memory lifecycle hooks are empty in this implementation.

void Skeleton::initMemory()
{
}

void Skeleton::closeMemory()
{
}


// Creates an empty skeleton with animation weight one.

Skeleton::Skeleton()
    : m_float00(1.0f)
    , m_bones(0)
    , m_boneTemplates(0)
    , m_hasLoadedArrays(false)
    , m_transforms(0)
    , m_byte3C(false)
{
}


// Initializes through +0x1C; leaves +0x20 untouched.

Skeleton::BoneTemplate::BoneTemplate()
    : m_index(-1)
    , m_nameId(-1)
    , m_parentIndex(-1)
    , m_depth(0)
    , m_float18(1.0f)
    , m_float1C(-1.0f)
{
    m_collisionOffset.x = 0.0f;       // +0x0C..+0x14 - three zero stores
    m_collisionOffset.y = 0.0f;
    m_collisionOffset.z = 0.0f;
    // m_dword20 deliberately left alone - the ELF does not touch it.
}

// Loads a version-2 skeleton from stream, or opens name when stream is null.
// Reports success through loaded after evaluating the initial pose; other versions leave it false.
// Names are trimmed/lowercased, mesh bones use identity transforms, and components below 0.001
// Magnitude become zero. This implementation bounds the name read, unlike the retail loader.
Skeleton::Skeleton(const std::string& name, bool* loaded, hfe::io::IStream* stream)
    : m_float00(1.0f)
    , m_bones(0)
    , m_boneTemplates(0)
    , m_hasLoadedArrays(false)
    , m_transforms(0)
    , m_byte3C(false)
{

    hfe::io::File file;
    *loaded = false;

    if (!stream)
    {
        if (!file.open(name, hfe::io::IFileSystem::FileMode(2),
                       hfe::io::IFileSystem::FileAccess(0), 0))
            return;                                  // *loaded stays false; the skeleton is blank
        stream = file.getStream();
    }

    int version = 0;
    stream->read(&version, 4);
    if (version != 2)                                // the only accepted version
        return;

    int boneCount = 0;
    stream->read(&boneCount, 4);
    m_boneCount = boneCount;

    // The two parallel arrays, each with its own leading count cookie
    m_bones = newSkeletonArray<Bone>(boneCount);
    m_boneTemplates = newSkeletonArray<BoneTemplate>(boneCount);
    m_hasLoadedArrays = true;

    // One identity transform per bone, written twice, exactly as operator= does
    m_transforms = static_cast<hfe::Mat4*>(operator new[](boneCount * 64));
    for (int i = 0; i < boneCount; ++i)
    {
        hfe::Mat4& t = m_transforms[i];
        for (int k = 0; k < 16; ++k)
            t.m[k] = 0.0f;
        t.m[0] = 1.0f; t.m[5] = 1.0f; t.m[10] = 1.0f; t.m[15] = 1.0f;
    }
    for (int i = 0; i < boneCount; ++i)
    {
        hfe::Mat4& t = m_transforms[i];
        t.m[0]  = 1.0f; t.m[1]  = 0.0f; t.m[2]  = 0.0f; t.m[3]  = 0.0f;
        t.m[4]  = 0.0f; t.m[5]  = 1.0f; t.m[6]  = 0.0f; t.m[7]  = 0.0f;
        t.m[8]  = 0.0f; t.m[9]  = 0.0f; t.m[10] = 1.0f; t.m[11] = 0.0f;
        t.m[12] = 0.0f; t.m[13] = 0.0f; t.m[14] = 0.0f; t.m[15] = 1.0f;
    }

    // The bones
    for (int i = 0; i < boneCount; ++i)
    {
        unsigned short nameLength = 0;
        stream->read(&nameLength, 2);

        // divergence, and the only one in this body: the ELF reads straight into a 256-byte stack
        // buffer with no bound on the file's 16-bit length. Bounded here - see the block comment.
        char nameBuffer[256];
        const unsigned int toRead = nameLength < sizeof(nameBuffer) - 1
                                        ? nameLength : sizeof(nameBuffer) - 1;
        stream->read(nameBuffer, toRead);
        if (toRead < nameLength)
            stream->skip(nameLength - toRead);       // keep the stream in step
        nameBuffer[toRead] = '\0';

        // trailing spaces OFF, then lowercase - in that order.
        for (int t = static_cast<int>(toRead) - 1; t >= 0 && nameBuffer[t] == ' '; --t)
            nameBuffer[t] = '\0';
        for (char* c = nameBuffer; *c; ++c)
            *c = static_cast<char>(tolower(static_cast<unsigned char>(*c)));

        m_boneTemplates[i].m_index = i;
        // Intern the bone name and narrow its index to the stored short.
        m_boneTemplates[i].m_nameId = static_cast<short>(
            BoneManager::getInstance().addBoneName(std::string(nameBuffer)));

        unsigned short parent = 0;
        stream->read(&parent, 2);
        m_boneTemplates[i].m_parentIndex = static_cast<short>(parent);   // 0xFFFF -> -1

        // A "mesh" bone gets the identity and its 28 bytes thrown away.
        if (std::string(nameBuffer).find("mesh") != std::string::npos)
        {
            hfe::Mat4& t = m_bones[i].m_transform;
            t.m[0]  = 1.0f; t.m[1]  = 0.0f; t.m[2]  = 0.0f; t.m[3]  = 0.0f;
            t.m[4]  = 0.0f; t.m[5]  = 1.0f; t.m[6]  = 0.0f; t.m[7]  = 0.0f;
            t.m[8]  = 0.0f; t.m[9]  = 0.0f; t.m[10] = 1.0f; t.m[11] = 0.0f;
            t.m[12] = 0.0f; t.m[13] = 0.0f; t.m[14] = 0.0f; t.m[15] = 1.0f;
            stream->skip(28);                        // IStream +0x18
            continue;
        }

        hfe::Quat q;
        q.x = 0.0f; q.y = 0.0f; q.z = 0.0f; q.w = 1.0f;
        stream->read(&q.x, 12);                      // x, y, z - w is read separately below
        if (fabsf(q.x) < 0.001f) q.x = 0.0f;         // the epsilon scrub, every component
        if (fabsf(q.y) < 0.001f) q.y = 0.0f;
        if (fabsf(q.z) < 0.001f) q.z = 0.0f;
        stream->read(&q.w, 4);
        if (fabsf(q.w) < 0.001f) q.w = 0.0f;

        float pos[3] = { 0.0f, 0.0f, 0.0f };
        stream->read(pos, 12);
        if (fabsf(pos[0]) < 0.001f) pos[0] = 0.0f;
        if (fabsf(pos[1]) < 0.001f) pos[1] = 0.0f;
        if (fabsf(pos[2]) < 0.001f) pos[2] = 0.0f;

        // order matters: toMat zeroes the translation row, so the position goes in after.
        q.toMat(m_bones[i].m_transform);
        m_bones[i].m_transform.m[12] = pos[0];
        m_bones[i].m_transform.m[13] = pos[1];
        m_bones[i].m_transform.m[14] = pos[2];
    }

    // The depth pass: hops to a root, per bone. Quadratic, done once.
    for (int i = 0; i < boneCount; ++i)
    {
        int depth = 0;
        for (short p = m_boneTemplates[i].m_parentIndex; p != -1;
             p = m_boneTemplates[p].m_parentIndex)
            ++depth;
        m_boneTemplates[i].m_depth = depth;
    }

    transform(0, -1);
    *loaded = true;
}


// Initializes bone transform and animation state.

Skeleton::Bone::Bone()
    : m_override(0)
    , m_ikHandleIndex(-1)
    , m_transformEnabled(true)
{
    for (int i = 0; i < 16; ++i)
        m_transform.m[i] = 0.0f;                 // redundant - see the note

    m_transform.m[0]  = 1.0f;  m_transform.m[1]  = 0.0f;  m_transform.m[2]  = 0.0f;  m_transform.m[3]  = 0.0f;
    m_transform.m[4]  = 0.0f;  m_transform.m[5]  = 1.0f;  m_transform.m[6]  = 0.0f;  m_transform.m[7]  = 0.0f;
    m_transform.m[8]  = 0.0f;  m_transform.m[9]  = 0.0f;  m_transform.m[10] = 1.0f;  m_transform.m[11] = 0.0f;
    m_transform.m[12] = 0.0f;  m_transform.m[13] = 0.0f;  m_transform.m[14] = 0.0f;  m_transform.m[15] = 1.0f;
}

// Deletes the bone's owned override storage.
Skeleton::Bone::~Bone()
{
    operator delete(m_override);
}


// Releases owned arrays and per-bone state. Shared template data is released only by its owner.

Skeleton::~Skeleton()
{
    if (m_hasLoadedArrays)
    {
        if (m_boneTemplates)
        {
            deleteSkeletonArray(m_boneTemplates);
        }
    }

    if (m_transforms)
        operator delete[](m_transforms);

    if (m_bones)
    {
        deleteSkeletonArray(m_bones);
    }
}


// Copies the source rig into newly allocated bone and transform arrays.
// Per-bone m_owned40 pointers are copied shallowly; nonnull values create shared ownership and risk double deletion.
// Other destination playback state remains unchanged.

Skeleton& Skeleton::operator=(Skeleton& other)
{
    if (this == &other)
        return *this;

    m_boneCount = other.m_boneCount;

    // Free the old bone array, destructing backwards from the cookie's count
    if (m_bones)
    {
        deleteSkeletonArray(m_bones);
    }
    m_bones = 0;

    if (m_transforms)
        operator delete[](m_transforms);
    m_transforms = 0;

    // The new bone array, count cookie first, elements constructed forwards
    const int count = other.m_boneCount;
    m_bones = newSkeletonArray<Bone>(count);

    // A pointer, copied verbatim - shared with the source, not cloned.
    m_boneTemplates = other.m_boneTemplates;

    // One transform per bone, identity
    m_transforms = static_cast<hfe::Mat4*>(operator new[](count * 64));
    for (int i = 0; i < count; ++i)
    {
        hfe::Mat4& t = m_transforms[i];
        for (int k = 0; k < 16; ++k)
            t.m[k] = 0.0f;
        t.m[0] = 1.0f; t.m[5] = 1.0f; t.m[10] = 1.0f; t.m[15] = 1.0f;
    }
    // and again, in a second pass - both are in the shipped code.
    for (int i = 0; i < count; ++i)
    {
        hfe::Mat4& t = m_transforms[i];
        t.m[0]  = 1.0f; t.m[1]  = 0.0f; t.m[2]  = 0.0f; t.m[3]  = 0.0f;
        t.m[4]  = 0.0f; t.m[5]  = 1.0f; t.m[6]  = 0.0f; t.m[7]  = 0.0f;
        t.m[8]  = 0.0f; t.m[9]  = 0.0f; t.m[10] = 1.0f; t.m[11] = 0.0f;
        t.m[12] = 0.0f; t.m[13] = 0.0f; t.m[14] = 0.0f; t.m[15] = 1.0f;
    }

    // And the bones themselves: 16 dwords, one dword, two bytes
    for (int i = 0; i < count; ++i)
    {
        Bone& dst = m_bones[i];
        const Bone& src = other.m_bones[i];
        for (int k = 0; k < 16; ++k)
            dst.m_transform.m[k] = src.m_transform.m[k];
        dst.m_override = src.m_override;         // shallow - see the defect note above
        dst.m_ikHandleIndex  = src.m_ikHandleIndex;
        dst.m_transformEnabled = src.m_transformEnabled;
    }

    m_vector24 = other.m_vector24;
    return *this;
}


// The collision-bone authoring chain. Stock SoldierTemplate tweaks call the string overload; the
// name is resolved through the process-wide BoneManager and narrowed to the short id stored in each
// BoneTemplate. The indexed overload rejects unknown and root bones through retail's fatal Error
// diagnostic, writes the authored capsule fields, then appends the bone index without de-duplication.

short Skeleton::getBoneIndex(short nameId) const
{
    short index = 0;
    for (int i = 0; i < m_boneCount; ++i, ++index)
    {
        if (m_boneTemplates[i].m_nameId == nameId)
            return index;
    }
    return -1;
}

void Skeleton::setSkeletonCollisionBone(short nameId, int material, const hfe::Vec3& offset,
                                         float radius, float length)
{
    const int boneIndex = getBoneIndex(nameId);
    if (boneIndex < 0 || m_boneTemplates[boneIndex].m_parentIndex < 0)
    {
        hfe::Debug(hfe::DEBUGTYPE_6, "Animation/BoneAnimation/Skeleton.cpp", 462, "")
            << "setSkeletonCollisionBone, bone not found.";
        return;
    }

    BoneTemplate& bone = m_boneTemplates[boneIndex];
    bone.m_collisionOffset = offset;
    bone.m_float18 = length;
    bone.m_float1C = radius;
    bone.m_dword20 = material;
    m_vector24.push_back(boneIndex);
}

void Skeleton::setSkeletonCollisionBone(const std::string& name, int material,
                                         const hfe::Vec3& offset, float radius, float length)
{
    const short nameId = static_cast<short>(
        BoneManager::getInstance().getBoneNameIndex(name));
    setSkeletonCollisionBone(nameId, material, offset, radius, length);
}


// Tests a ray against the skeleton's collision bones and writes hit information when found.

bool Skeleton::checkRayVsSkeleton(hfe::Vec3* hitPosition, hfe::Vec3* hitNormal,
                                  int* hitPart, int* hitBone, float* fraction,
                                  const hfe::Vec3& start, const hfe::Vec3& end,
                                  const hfe::Mat4& transformation)
{
    hfe::Vec3 hitPoints[2] = {};
    float hitTimes[2];
    int intersectionCount = 0;
    int boneIndex = 0;
    BoneTemplate* collisionTemplate = 0;
    hfe::Mat4 boneTransform;

    for (unsigned int i = 0; i < m_vector24.size(); ++i)
    {
        boneTransform = hfe::Mat4::getIdentity();
        boneIndex = m_vector24[i];
        collisionTemplate = &m_boneTemplates[boneIndex];
        getCollisionBoneTransform(
            &boneTransform, transformation, boneIndex, m_bones[boneIndex]);

        const float capsuleLength = fabsf(collisionTemplate->m_float18);
        hfe::Vec3 capsuleAxis;
        capsuleAxis.x = boneTransform.m[8] * capsuleLength;
        capsuleAxis.y = boneTransform.m[9] * capsuleLength;
        capsuleAxis.z = boneTransform.m[10] * capsuleLength;

        hfe::Vec3 rayAxis;
        rayAxis.x = end.x - start.x;
        rayAxis.y = end.y - start.y;
        rayAxis.z = end.z - start.z;

        hfe::Vec3 capsuleStart;
        capsuleStart.x = boneTransform.m[12];
        capsuleStart.y = boneTransform.m[13];
        capsuleStart.z = boneTransform.m[14];

        intersectionCount = hfe::world::getIntersectionOfCapsAndEdgeNew(
            start, rayAxis, capsuleStart, capsuleAxis, collisionTemplate->m_float1C,
            hitTimes, hitPoints);
        if (intersectionCount > 0)
            break;
    }

    if (intersectionCount <= 0)
        return false;

    int selected = 0;
    if (intersectionCount == 2)
        selected = hitTimes[1] <= hitTimes[0];

    if (hitPosition)
        *hitPosition = hitPoints[selected];

    if (hitNormal)
    {
        const float dx = hitPoints[selected].x - boneTransform.m[12];
        const float dy = hitPoints[selected].y - boneTransform.m[13];
        const float dz = hitPoints[selected].z - boneTransform.m[14];

        const float firstProjection =
            dx * boneTransform.m[0] + dy * boneTransform.m[1] + dz * boneTransform.m[2];
        const float secondProjection =
            dx * boneTransform.m[4] + dy * boneTransform.m[5] + dz * boneTransform.m[6];

        hitNormal->x = boneTransform.m[0] * firstProjection
                     + boneTransform.m[4] * secondProjection;
        hitNormal->y = boneTransform.m[1] * firstProjection
                     + boneTransform.m[5] * secondProjection;
        hitNormal->z = boneTransform.m[2] * firstProjection
                     + boneTransform.m[6] * secondProjection;

        const float lengthSq = hitNormal->z * hitNormal->z
                             + hitNormal->y * hitNormal->y
                             + hitNormal->x * hitNormal->x;
        if (lengthSq <= 0.00001f)
        {
            hitNormal->x = 0.0f;
            hitNormal->y = 1.0f;
            hitNormal->z = 0.0f;
        }
        else if (fabsf(lengthSq - 1.0f) >= FLT_EPSILON)
        {
            if (fabsf(lengthSq) >= FLT_EPSILON)
            {
                const float inverseLength = 1.0f / sqrtf(lengthSq);
                hitNormal->x *= inverseLength;
                hitNormal->y *= inverseLength;
                hitNormal->z *= inverseLength;
            }
            else
            {
                hitNormal->x = 0.0f;
                hitNormal->y = 0.0f;
                hitNormal->z = 0.0f;
            }
        }

        if (fraction)
        {
            float cosine = boneTransform.m[0] * hitNormal->x
                         + boneTransform.m[1] * hitNormal->y
                         + boneTransform.m[2] * hitNormal->z;
            if (cosine < -1.0f)
                cosine = -1.0f;
            else if (cosine > 1.0f)
                cosine = 1.0f;

            const float angle = acosf(cosine) * 57.295776f;
            *fraction = angle;
            if (boneTransform.m[4] * hitNormal->x
                + boneTransform.m[5] * hitNormal->y
                + boneTransform.m[6] * hitNormal->z > 0.0f)
            {
                *fraction = 360.0f - angle;
            }
        }
    }

    if (hitPart)
        *hitPart = collisionTemplate->m_dword20;
    if (hitBone)
        *hitBone = boneIndex;
    return true;
}


// Evaluates a pose for usedBones and their ancestors under the optional parent transform.
// Indices must be valid and parents must precede children in the bone array.
// Re-enables every bone after evaluation; previous enable flags are not restored.
void Skeleton::transformUsedBones(const hfe::Mat4* parent, const std::vector<int>& usedBones)
{
    for (int i = 0; i < m_boneCount; ++i)
        m_bones[i].m_transformEnabled = false;

    for (std::vector<int>::const_iterator it = usedBones.begin(); it != usedBones.end(); ++it)
        m_bones[*it].m_transformEnabled = true;

    // Backwards, so one pass carries every used bone's whole ancestry up to its root.
    for (int i = m_boneCount - 1; i >= 0; --i)
    {
        if (m_bones[i].m_transformEnabled)
        {
            const int parentIndex = m_boneTemplates[i].m_parentIndex;   // a short; -1 is a root
            if (parentIndex >= 0)
                m_bones[parentIndex].m_transformEnabled = true;
        }
    }

    transform(parent, -1);

    for (int i = 0; i < m_boneCount; ++i)
        m_bones[i].m_transformEnabled = true;
}

// Evaluates animations and propagates bone transforms under the optional parent matrix.
// MaxDepth < 0 includes all depths. Parents must precede children in the bone array.
// Disabled/excluded bones retain orientation but inherit parent translation; child overrides
// Change rotation only, while root overrides may also translate.
void Skeleton::transform(const hfe::Mat4* parent, int maxDepth)
{
    executeAnimations();

    // Preserve the accumulated overrides even though this path never consumes the result.
    hfe::Mat4 accum;
    for (int k = 0; k < 16; ++k)
        accum.m[k] = 0.0f;
    accum.m[0] = 1.0f; accum.m[5] = 1.0f; accum.m[10] = 1.0f; accum.m[15] = 1.0f;

    for (int i = 0; i < m_boneCount; ++i)
    {
        Bone& bone = m_bones[i];
        BoneTemplate& tmpl = m_boneTemplates[i];
        hfe::Mat4& out = m_transforms[i];

        const bool takePart = bone.m_transformEnabled
                              && (maxDepth < 0 || tmpl.m_depth <= maxDepth);

        if (!takePart)
        {
            // not A no-OP: inherit the parent's position and keep the existing orientation.
            if (tmpl.m_parentIndex >= 0)
            {
                const hfe::Mat4& p = m_transforms[tmpl.m_parentIndex];
                out.m[12] = p.m[12];
                out.m[13] = p.m[13];
                out.m[14] = p.m[14];
            }
            continue;
        }

        // Skip to the solver's last posed bone so the hierarchy walk cannot overwrite its requested matrix.
        if (bone.m_ikHandleIndex != -1)
        {
            i = applyIkSolver(m_ikHandles[static_cast<size_t>(bone.m_ikHandleIndex)]);
            continue;
        }

        if (bone.m_override)
        {
            hfe::Mat4 prev = accum;
            accum.rotMult(*bone.m_override, prev);        // the accumulator nothing reads
        }

        if (tmpl.m_parentIndex < 0)
        {
            // A root bone: the caller's matrix is the parent, or the bone stands alone
            if (parent)
                out.mult(bone.m_transform, *parent);
            else
                out = bone.m_transform;

            // the root ARM uses A full mult - an override here does reach the translation.
            if (bone.m_override)
            {
                hfe::Mat4 tmp;
                for (int k = 0; k < 16; ++k)
                    tmp.m[k] = 0.0f;
                tmp.m[0] = 1.0f; tmp.m[5] = 1.0f; tmp.m[10] = 1.0f; tmp.m[15] = 1.0f;
                tmp.mult(out, *bone.m_override);
                out = tmp;
            }
        }
        else
        {
            // A child bone: local * the parent's already-finished absolute matrix
            const hfe::Mat4& parentTransform = m_transforms[tmpl.m_parentIndex];

            if (!bone.m_override)
            {
                out.mult(bone.m_transform, parentTransform);
            }
            else
            {
                hfe::Mat4 tmp;
                for (int k = 0; k < 16; ++k)
                    tmp.m[k] = 0.0f;
                tmp.m[0] = 1.0f; tmp.m[5] = 1.0f; tmp.m[10] = 1.0f; tmp.m[15] = 1.0f;
                tmp.mult(bone.m_transform, parentTransform);

                // rotation from the override, translation from the product - an override on a child
                // turns the limb, it does not move it.
                out.rotMult(tmp, *bone.m_override);
                out.m[12] = tmp.m[12];
                out.m[13] = tmp.m[13];
                out.m[14] = tmp.m[14];
            }
        }
    }
}


// Converts absolute bone poses to local transforms relative to transformation.

void Skeleton::calculateRelativeTransforms(const hfe::Mat4& transformation)
{
    if (m_boneCount == 0)
        return;

    removeAllPostAbsoluteBoneTransforms();

    for (int i = 0; i < m_boneCount; ++i)
    {
        Bone& bone = m_bones[i];
        const short parentIndex = m_boneTemplates[i].m_parentIndex;

        if (parentIndex < 0)
        {
            bone.m_transform = m_transforms[i];
        }
        else
        {
            // The retail source shape retains the signed parent guard in the transform selection,
            // even though this arm is reached only for a non-negative parent index.
            const hfe::Mat4& selectedParent = parentIndex < 0
                                               ? hfe::Mat4::getIdentity()
                                               : m_transforms[parentIndex];
            hfe::Mat4 parentTransform = selectedParent;
            hfe::Mat4 inverseParent = parentTransform;
            inverseParent.setInverse(parentTransform);
            bone.m_transform.mult(m_transforms[i], inverseParent);
        }
    }

    for (int i = 0; i < m_boneCount; ++i)
    {
        if (m_boneTemplates[i].m_parentIndex < 0)
        {
            hfe::Mat4 oldTransform = m_bones[i].m_transform;
            m_bones[i].m_transform.mult(oldTransform, transformation);
        }
    }
}


// Process-wide staging table for 256 bones; not reentrant. Each bone holds at most five stages.
// A full-weight stage clears earlier stages; a sixth stage evicts the oldest.
// Bone IDs must be below 256. [TEMP] The diagnostic probe below reports invalid indices.

namespace {

const int kMaxStagesPerBone = 5;                     // 140 / 28, attested by the arithmetic
const int kStagingTableBones = 256;                  // attested - 0x9000 of .bss / 144

struct BoneStaging
{
    Skeleton::ApplyStage m_stages[kMaxStagesPerBone];
    int                  m_count;
};

BoneStaging g_boneStaging[kStagingTableBones];
// [TEMP] [CHOSEN, not recovered] Reports invalid staging indices before they overwrite the shared table.
// Unloaded animations and freed sample buffers can both produce invalid IDs.
// [RETIRE-WHEN: bone-staging-index] Fix and validate upstream lifetime/loading, then restore unguarded staging.

bool stagingIndexIsSane(const char* who, const Skeleton::ApplyStage& stage, int i, unsigned boneIndex)
{
    if (boneIndex < static_cast<unsigned>(kStagingTableBones))
        return true;

    const BoneAnimation* const a = stage.m_animA;
    std::fprintf(stderr,
                 "[TEMP][BONESTAGE] %s: bone id %u at list index %d is outside the %d-entry staging "
                 "table.\n  anim=%p loaded=%d boneCount=%d boneIds=%p idToIndex=%p maxId=%u "
                 "minId=%u memoryUsed=%d length=%f\n  stage weight=%f timeA=%f weightFactor=%f "
                 "baseBone=%d\n",
                 who, boneIndex, i, kStagingTableBones,
                 static_cast<const void*>(a),
                 a ? static_cast<int>(a->m_loaded) : -1,
                 a ? a->m_boneCount : -1,
                 static_cast<const void*>(a ? a->m_boneIds : 0),
                 static_cast<const void*>(a ? a->m_boneIdToIndex : 0),
                 a ? static_cast<unsigned>(a->m_maxBoneId) : 0u,
                 a ? static_cast<unsigned>(a->m_minBoneId) : 0u,
                 a ? a->m_memoryUsed : -1,
                 a ? a->length : 0.0f,
                 stage.m_weight, stage.m_timeA, stage.m_weightFactor,
                 a ? a->m_baseBone : -1);

    // [TEMP] Check registration to diagnose invalid animation storage during staging.
    // [RETIRE-WHEN: bone-staging-index] Remove after resolving and validating animation registration during staging.
    if (animationManager)
    {
        const std::string* const registered = animationManager->debugFindRegisteredName(a);
        std::fprintf(stderr, "  registered=%s%s  postLoadCalls=%d\n",
                     registered ? "YES as " : "NO - the manager does not know this pointer",
                     registered ? registered->c_str() : "",
                     animationManager->debugPostLoadCalls());
    }
    else
    {
        std::fprintf(stderr, "  registered=UNKNOWN - animationManager is null\n");
    }

    std::fflush(stderr);
    std::abort();
    return false;
}


} // namespace


// Selects a staging slot, replacing prior full-weight stages or evicting the oldest when full.

namespace {
int stagingSlotFor(BoneStaging& staging, float weight)
{
    int count = staging.m_count;

    // full weight wipes what came before.
    if (weight >= 1.0f && count > 0)
    {
        count = 0;
        staging.m_count = 0;
    }

    // A sixth stage evicts the oldest.
    if (count > kMaxStagesPerBone - 1)
    {
        for (int i = 1; i < staging.m_count; ++i)
            staging.m_stages[i - 1] = staging.m_stages[i];
        count = staging.m_count - 1;
        staging.m_count = count;
    }

    staging.m_count = count + 1;
    return count;
}
} // namespace


// Stages one animation on every bone it contains.

void Skeleton::applySimpleAnimationStage(const ApplyStage& stage)
{
    const int boneCount = stage.m_animA->m_boneCount;
    const unsigned short* indices = stage.m_animA->m_boneIds;

    for (int i = 0; i < boneCount; ++i)
    {
        const unsigned short boneIndex = indices[i];      // no 0xFFFF test - see above

        const int baseBone = stage.m_animA->m_baseBone;
        if (baseBone && boneIndex < baseBone)
            continue;

        stagingIndexIsSane("applySimpleAnimationStage", stage, i, boneIndex);   // [TEMP] - see above

        BoneStaging& staging = g_boneStaging[boneIndex];
        const int slot = stagingSlotFor(staging, stage.m_weight);

        ApplyStage& out = staging.m_stages[slot];
        out.m_weight       = stage.m_weight;
        out.m_animA        = stage.m_animA;
        out.m_timeA        = stage.m_timeA;
        out.m_weightFactor = stage.m_weightFactor;
        out.m_animB        = 0;                            // the single-animation marker
    }
}


// Queues a two-animation blend for each mapped bone, skipping 0xFFFF entries.

void Skeleton::applyBlendedAnimationStage(const ApplyStage& stage)
{
    const int boneCount = stage.m_animA->m_boneCount;
    const unsigned short* indices = stage.m_animA->m_boneIds;

    for (int i = 0; i < boneCount; ++i)
    {
        const unsigned short boneIndex = indices[i];
        if (boneIndex == 0xFFFF)                           // the sentinel, tested only here
            continue;

        const int baseBone = stage.m_animA->m_baseBone;
        if (baseBone && boneIndex < baseBone)
            continue;

        stagingIndexIsSane("applyBlendedAnimationStage", stage, i, boneIndex);  // [TEMP] - see above

        BoneStaging& staging = g_boneStaging[boneIndex];
        const int slot = stagingSlotFor(staging, stage.m_weight);

        ApplyStage& out = staging.m_stages[slot];
        out.m_weight       = stage.m_weight;
        out.m_animA        = stage.m_animA;
        out.m_timeA        = stage.m_timeA;
        out.m_blend        = stage.m_blend;
        out.m_weightFactor = stage.m_weightFactor;
        out.m_animB        = stage.m_animB;
        out.m_timeB        = stage.m_timeB;
    }
}


// Builds the per-bone staging table from queued animations; optionally clears the queue.

void Skeleton::buildApplyAnimationData(bool clearQueue)
{
    // [TEMP] the other way into the same table: this walk and executeAnimations' both index off the
    // skeleton's bone count, so a skeleton of more than 256 bones (or a corrupt m_boneCount) smashes
    // .bss here rather than in the fan-outs. Same probe family, same retirement.
    // [RETIRE-WHEN: bone-staging-index]
    if (static_cast<unsigned>(m_boneCount) > static_cast<unsigned>(kStagingTableBones))
    {
        std::fprintf(stderr, "[TEMP][BONESTAGE] buildApplyAnimationData: skeleton m_boneCount=%d "
                             "exceeds the %d-entry staging table (skeleton=%p).\n",
                     m_boneCount, kStagingTableBones, static_cast<const void*>(this));
        std::fflush(stderr);
        std::abort();
    }

    for (int i = 0; i < m_boneCount; ++i)
        g_boneStaging[i].m_count = 0;

    // the end is re-read each time round - see the note above
    for (size_t i = 0; i < m_pendingStages.size(); ++i)
    {
        const ApplyStage& stage = m_pendingStages[i];
        if (stage.m_animB)
            applyBlendedAnimationStage(stage);
        else
            applySimpleAnimationStage(stage);
    }

    if (clearQueue)
        m_pendingStages.erase(m_pendingStages.begin(), m_pendingStages.end());
}


// Blends staged rotations and positions into each bone's local pose, then clears its stage count.
// Partial total weight blends with the existing pose; full-weight stages replace earlier results.

void Skeleton::executeAnimations()
{
    buildApplyAnimationData(true);                   // fans the queue into the table

    for (int boneIndex = 0; boneIndex < m_boneCount; ++boneIndex)
    {
        Bone& bone = m_bones[boneIndex];
        BoneStaging& staging = g_boneStaging[boneIndex];

        if (!bone.m_transformEnabled)
        {
            staging.m_count = 0;                     // cleared even when skipped
            continue;
        }

        const int stageCount = staging.m_count;
        if (stageCount == 0)
            continue;                                // note: the count is not cleared on this path

        // The accumulator: identity rotation, zero translation.
        hfe::Quat accRotation;
        accRotation.x = 0.0f; accRotation.y = 0.0f; accRotation.z = 0.0f; accRotation.w = 1.0f;
        hfe::Vec3 accPosition;
        accPosition.x = 0.0f; accPosition.y = 0.0f; accPosition.z = 0.0f;

        // the skeleton-wide base weight, multiplied by every stage's own factor.
        float runningWeight = m_float00;

        for (int stageIndex = 0; stageIndex < stageCount; ++stageIndex)
        {
            const ApplyStage& stage = staging.m_stages[stageIndex];
            const float stageWeight = stage.m_weight;
            runningWeight *= stage.m_weightFactor;

            hfe::Quat stageRotation;
            stageRotation.x = 0.0f; stageRotation.y = 0.0f; stageRotation.z = 0.0f;
            stageRotation.w = 1.0f;
            hfe::Vec3 stagePosition;
            stagePosition.x = 0.0f; stagePosition.y = 0.0f; stagePosition.z = 0.0f;

            if (stage.m_animB)
            {
                // Two animations cross-faded by m_blend
                hfe::Quat rotA, rotB;
                hfe::Vec3 posA, posB;
                rotA.x = 0.0f; rotA.y = 0.0f; rotA.z = 0.0f; rotA.w = 1.0f;
                rotB.x = 0.0f; rotB.y = 0.0f; rotB.z = 0.0f; rotB.w = 1.0f;
                posA.x = 0.0f; posA.y = 0.0f; posA.z = 0.0f;
                posB.x = 0.0f; posB.y = 0.0f; posB.z = 0.0f;

                stage.m_animA->getValue(boneIndex, stage.m_timeA, rotA, posA,
                                      stage.m_weight);
                stage.m_animB->getValue(boneIndex, stage.m_timeB, rotB, posB,
                                      stageWeight);

                const float blend = stage.m_blend;
                stageRotation.slerp(rotA, rotB, blend);
                stagePosition.x = posA.x + (posB.x - posA.x) * blend;   // a lerp, not a slerp
                stagePosition.y = posA.y + (posB.y - posA.y) * blend;
                stagePosition.z = posA.z + (posB.z - posA.z) * blend;
            }
            else
            {
                // one animation. A bone that is the animation's base bone takes a different
                // path entirely - calculateTransformed rather than a plain sample.
                if (stage.m_animA->m_baseBone == boneIndex)
                    calculateTransformed(boneIndex, stage.m_animA, stage.m_timeA,
                                         stageRotation, stagePosition);
                else
                    stage.m_animA->getValue(boneIndex, stage.m_timeA,
                                          stageRotation, stagePosition, stageWeight);
            }

            // the first stage always replaces; a later one at full weight replaces too.
            if (stageIndex > 0 && stageWeight < 1.0f)
            {
                hfe::Quat previous = accRotation;
                accRotation.slerp(previous, stageRotation, stageWeight);
                accPosition.x += (stagePosition.x - accPosition.x) * stageWeight;
                accPosition.y += (stagePosition.y - accPosition.y) * stageWeight;
                accPosition.z += (stagePosition.z - accPosition.z) * stageWeight;
            }
            else
            {
                accRotation = stageRotation;
                accPosition = stagePosition;
            }
        }

        // under full weight, blend against what the bone already was.
        if (runningWeight < 1.0f)
        {
            hfe::Quat current;
            current.x = 0.0f; current.y = 0.0f; current.z = 0.0f; current.w = 0.0f;
            current.fromMat(bone.m_transform);

            // the same 0.001 scrub the .ske parser uses, on three of the four components.
            if (fabsf(current.x) < 0.001f) current.x = 0.0f;
            if (fabsf(current.y) < 0.001f) current.y = 0.0f;
            if (fabsf(current.z) < 0.001f) current.z = 0.0f;

            hfe::Quat animated = accRotation;
            accRotation.slerp(current, animated, runningWeight);

            const float bx = bone.m_transform.m[12];
            const float by = bone.m_transform.m[13];
            const float bz = bone.m_transform.m[14];
            accPosition.x = bx + (accPosition.x - bx) * runningWeight;
            accPosition.y = by + (accPosition.y - by) * runningWeight;
            accPosition.z = bz + (accPosition.z - bz) * runningWeight;
        }

        // toMat owns the whole matrix, so the translation goes in afterwards - the same ordering
        // the .ske parser depends on.
        accRotation.toMat(bone.m_transform);
        bone.m_transform.m[12] = accPosition.x;
        bone.m_transform.m[13] = accPosition.y;
        bone.m_transform.m[14] = accPosition.z;

        staging.m_count = 0;
    }
}


// Clears inverse-kinematics state before skeleton reuse.

void Skeleton::clearAllIk()
{
    if (m_ikHandles.empty())
        return;

    for (std::vector<IkHandle>::iterator it = m_ikHandles.begin(); it != m_ikHandles.end(); ++it)
    {
        const int boneIndex = it->m_boneIndex;
        m_bones[boneIndex].m_ikHandleIndex = -1;      // unchecked - see above
    }

    m_ikHandles.clear();
}

namespace {

// The inline hfe::Vec3 normalisation emitted twice in both shipped applyIkSolver bodies. It leaves
// an already-unit vector alone and zeroes a vector whose squared length is below FLT_EPSILON.
void normalizeIkVector(hfe::Vec3& value)
{
    const float squaredLength = value.x * value.x + value.y * value.y + value.z * value.z;
    if (fabsf(squaredLength - 1.0f) >= FLT_EPSILON)
    {
        if (fabsf(squaredLength) >= FLT_EPSILON)
        {
            const float scale = 1.0f / sqrtf(squaredLength);
            value.x *= scale;
            value.y *= scale;
            value.z *= scale;
        }
        else
        {
            value.x = 0.0f;
            value.y = 0.0f;
            value.z = 0.0f;
        }
    }
}

void rebuildIkTranslation(hfe::Mat4& absolute,
                          const hfe::Mat4& local,
                          const hfe::Mat4& parent)
{
    const float x = local.m[12];
    const float y = local.m[13];
    const float z = local.m[14];
    absolute.m[12] = x * parent.m[0] + y * parent.m[4] + z * parent.m[8] + parent.m[12];
    absolute.m[13] = x * parent.m[1] + y * parent.m[5] + z * parent.m[9] + parent.m[13];
    absolute.m[14] = x * parent.m[2] + y * parent.m[6] + z * parent.m[10] + parent.m[14];
}

} // namespace


// Solves the three-joint chain driven by handle.

int Skeleton::applyIkSolver(const IkHandle& handle)
{
    const int boneIndex = handle.m_boneIndex;
    hfe::Mat4& startJoint = m_transforms[boneIndex - 3];
    hfe::Mat4& middleJoint = m_transforms[boneIndex - 2];
    hfe::Mat4& endJoint = m_transforms[boneIndex - 1];
    hfe::Mat4& drivenBone = m_transforms[boneIndex];

    drivenBone.mult(m_bones[boneIndex].m_transform, endJoint);

    hfe::Vec3 currentEffector;
    if (handle.m_effectorBoneIndex == boneIndex)
    {
        currentEffector.x = drivenBone.m[12];
        currentEffector.y = drivenBone.m[13];
        currentEffector.z = drivenBone.m[14];
    }
    else
    {
        hfe::Mat4& child = m_transforms[boneIndex + 1];
        child.mult(m_bones[boneIndex + 1].m_transform, drivenBone);
        currentEffector.x = child.m[12];
        currentEffector.y = child.m[13];
        currentEffector.z = child.m[14];
    }

    const float segmentX = endJoint.m[12] - middleJoint.m[12];
    const float segmentY = endJoint.m[13] - middleJoint.m[13];
    const float segmentZ = endJoint.m[14] - middleJoint.m[14];
    const float segmentLength = sqrtf(segmentX * segmentX
                                    + segmentY * segmentY
                                    + segmentZ * segmentZ);

    hfe::Vec3 projectedHandle;
    projectedHandle.x = startJoint.m[12] - handle.m_transform.m[12];
    projectedHandle.y = startJoint.m[13] - handle.m_transform.m[13];
    projectedHandle.z = startJoint.m[14] - handle.m_transform.m[14];
    normalizeIkVector(projectedHandle);
    projectedHandle.x = handle.m_transform.m[12] + projectedHandle.x * segmentLength;
    projectedHandle.y = handle.m_transform.m[13] + projectedHandle.y * segmentLength;
    projectedHandle.z = handle.m_transform.m[14] + projectedHandle.z * segmentLength;

    hfe::Vec3 projectedEffector;
    projectedEffector.x = startJoint.m[12] - currentEffector.x;
    projectedEffector.y = startJoint.m[13] - currentEffector.y;
    projectedEffector.z = startJoint.m[14] - currentEffector.z;
    normalizeIkVector(projectedEffector);
    projectedEffector.x = currentEffector.x + projectedEffector.x * segmentLength;
    projectedEffector.y = currentEffector.y + projectedEffector.y * segmentLength;
    projectedEffector.z = currentEffector.z + projectedEffector.z * segmentLength;

    hfe::Vec3 startPosition;
    startPosition.x = startJoint.m[12];
    startPosition.y = startJoint.m[13];
    startPosition.z = startJoint.m[14];
    hfe::Vec3 middlePosition;
    middlePosition.x = middleJoint.m[12];
    middlePosition.y = middleJoint.m[13];
    middlePosition.z = middleJoint.m[14];

    hfe::Mat4 startRotation = hfe::Mat4::getIdentity();
    hfe::Mat4 bendRotation = hfe::Mat4::getIdentity();
    maya::applyIK2BoneSolver(handle.m_target, startPosition, middlePosition,
                             projectedEffector, projectedHandle,
                             startRotation, bendRotation);

    hfe::Mat4 previous = startJoint;
    startJoint.rotMult(previous, startRotation);

    previous = middleJoint;
    middleJoint.rotMult(previous, bendRotation);
    previous = middleJoint;
    middleJoint.rotMult(previous, startRotation);
    rebuildIkTranslation(middleJoint, m_bones[boneIndex - 2].m_transform, startJoint);

    previous = endJoint;
    endJoint.rotMult(previous, bendRotation);
    previous = endJoint;
    endJoint.rotMult(previous, bendRotation);
    previous = endJoint;
    endJoint.rotMult(previous, startRotation);
    rebuildIkTranslation(endJoint, m_bones[boneIndex - 1].m_transform, middleJoint);

    if (handle.m_effectorBoneIndex != boneIndex)
        drivenBone.mult(m_bones[boneIndex].m_transform, endJoint);

    m_transforms[handle.m_effectorBoneIndex] = handle.m_transform;
    return handle.m_effectorBoneIndex;
}


// Removes all per-bone absolute-transform overrides.

void Skeleton::removeAllPostAbsoluteBoneTransforms()
{
    for (int i = 0; i < m_boneCount; ++i)                  // +0x14
    {
        ::operator delete(m_bones[i].m_override);          // +0x04 + 0x48 * i, then +0x40
        m_bones[i].m_override = 0;
    }
}


// Queues animation stages for later skeleton evaluation.

void Skeleton::applyAnimation(BoneAnimation* animation, float time, float weight, float weightFactor)
{
    if (weight > 0.0f)
    {
        ApplyStage stage;
        stage.m_animA        = animation;
        stage.m_animB        = 0;
        stage.m_weight       = (weight >= 1.0f) ? 1.0f : weight;
        stage.m_timeA        = time;
        stage.m_weightFactor = weightFactor;
        stage.m_blend        = 0.0f;                     // unread for a single stage - see above
        stage.m_timeB        = 0.0f;
        m_pendingStages.push_back(stage);
    }
}

void Skeleton::applyAnimations(BoneAnimation* animationA, BoneAnimation* animationB,
                               float timeA, float timeB, float blend, float weight,
                               float weightFactor)
{
    if (weight > 0.0f)
    {
        ApplyStage stage;
        stage.m_animA        = animationA;
        stage.m_animB        = animationB;
        stage.m_timeA        = timeA;
        stage.m_timeB        = timeB;
        stage.m_weight       = (weight >= 1.0f) ? 1.0f : weight;
        stage.m_blend        = blend;
        stage.m_weightFactor = weightFactor;
        m_pendingStages.push_back(stage);
    }
}


// Samples and transforms an animation pose along the bone's parent chain.

void Skeleton::calculateTransformed(int boneIndex, BoneAnimation* animation, float time,
                                    hfe::Quat& outRotation, hfe::Vec3& outPosition)
{
    hfe::Quat accumulated;
    accumulated.x = 0.0f; accumulated.y = 0.0f; accumulated.z = 0.0f; accumulated.w = 1.0f;
    hfe::Mat4 hierarchy = hfe::Mat4::getIdentity();
    hfe::Quat sample;
    sample.x = 0.0f; sample.y = 0.0f; sample.z = 0.0f; sample.w = 1.0f;
    hfe::Vec3 position;
    position.x = 0.0f; position.y = 0.0f; position.z = 0.0f;

    if (boneIndex != -1)
    {
        int bone = boneIndex;
        do
        {
            animation->getValue(bone, time, sample, position, 1.0f);

            // accumulated = accumulated * sample
            const float ax = accumulated.x, ay = accumulated.y, az = accumulated.z, aw = accumulated.w;
            const float w = sample.w * aw - (az * sample.z + ay * sample.y + ax * sample.x);
            const float x = sample.w * ax + (ay * sample.z - az * sample.y) + sample.x * aw;
            const float y = sample.y * aw + sample.x * az - ax * sample.z + ay * sample.w;
            const float z = ax * sample.y - sample.x * ay + sample.z * aw + az * sample.w;
            accumulated.x = x;
            accumulated.y = y;
            accumulated.z = z;
            accumulated.w = w;

            if (bone == boneIndex)
            {
                outPosition = position;
            }
            else
            {
                hfe::Mat4 product = hfe::Mat4::getIdentity();
                product.rotMult(hierarchy, m_bones[bone].m_transform);
                hierarchy = product;
            }
            bone = m_boneTemplates[bone].m_parentIndex;
        }
        while (bone != -1);
    }

    hfe::Quat inverse;
    inverse.x = 0.0f; inverse.y = 0.0f; inverse.z = 0.0f; inverse.w = 1.0f;
    inverse.fromMat(hierarchy);
    const float lengthSq = inverse.x * inverse.x + inverse.y * inverse.y
                         + inverse.z * inverse.z + inverse.w * inverse.w;
    inverse.x = -inverse.x;
    inverse.y = -inverse.y;
    inverse.z = -inverse.z;
    if (fabsf(lengthSq) <= FLT_EPSILON)
    {
        inverse.x = 0.0f; inverse.y = 0.0f; inverse.z = 0.0f; inverse.w = 0.0f;
    }
    else
    {
        const float scale = 1.0f / lengthSq;
        inverse.w = inverse.w * scale;
        inverse.x = inverse.x * scale;
        inverse.y = inverse.y * scale;
        inverse.z = inverse.z * scale;
    }

    // outRotation = accumulated * inverse
    const float ax = accumulated.x, ay = accumulated.y, az = accumulated.z, aw = accumulated.w;
    outRotation.w = inverse.w * aw - (ay * inverse.y + ax * inverse.x + az * inverse.z);
    outRotation.y = az * inverse.x - ax * inverse.z;
    outRotation.z = ax * inverse.y - ay * inverse.x;
    outRotation.x = (inverse.x * aw + (ay * inverse.z - az * inverse.y)) + inverse.w * ax;
    outRotation.y = inverse.y * aw + outRotation.y;
    outRotation.z = inverse.z * aw + outRotation.z;
    outRotation.y = inverse.w * ay + outRotation.y;
    outRotation.z = inverse.w * az + outRotation.z;
}


// Writes the requested collision-bone transform relative to transformation.

void Skeleton::getCollisionBoneTransform(hfe::Mat4* out, const hfe::Mat4& transformation,
                                         int boneIndex, const Bone& /*bone*/) const
{
    const BoneTemplate& collisionTemplate = m_boneTemplates[boneIndex];        // +0x08 each
    const bool flipped = collisionTemplate.m_float18 < 0.0f;

    *out = hfe::Mat4::getIdentity();
    const hfe::Vec3& offset = collisionTemplate.m_collisionOffset;             // +0x0C
    if (flipped)
    {
        out->m[12] = offset.x + out->m[12];
        out->m[13] = offset.y + out->m[13];
        out->m[14] = offset.z + out->m[14];
    }
    else
    {
        out->m[12] = offset.x + out->m[12];
        out->m[13] = offset.y + out->m[13];
        out->m[14] = -offset.z + out->m[14];
    }

    hfe::Mat4 rotation = hfe::Mat4::getIdentity();
    if (flipped)
        hfe::setRotateXDeg(rotation, -90.0f);
    else
        hfe::setRotateXDeg(rotation, 90.0f);

    {
        hfe::Mat4 copy = *out;
        out->mult(copy, rotation);
    }
    {
        const hfe::Mat4& boneTransform = (boneIndex < 0) ? hfe::Mat4::getIdentity()
                                                         : m_transforms[boneIndex];     // +0x10
        hfe::Mat4 copy = *out;
        out->mult(copy, boneTransform);
    }
    {
        hfe::Mat4 copy = *out;
        out->mult(copy, transformation);
    }
}


// Creates or updates the inverse-kinematics handle for the requested bone.

void Skeleton::applyIk(int boneIndex, int effectorBoneIndex,
                       const hfe::Vec3& target, const hfe::Mat4& transform)
{
    const char existing = m_bones[boneIndex].m_ikHandleIndex;                 // +0x04 + 72n + 0x44
    int handleIndex;
    if (existing == -1)
    {
        handleIndex = static_cast<int>(m_ikHandles.size());
        IkHandle blank;
        blank.m_target.x = 0.0f;
        blank.m_target.y = 0.0f;
        blank.m_target.z = 0.0f;
        blank.m_transform = hfe::Mat4::getIdentity();
        blank.m_boneIndex = 0;
        blank.m_effectorBoneIndex = 0;
        m_ikHandles.push_back(blank);                    // resize(size + 1, blank), as compiled
        m_ikHandles[handleIndex].m_boneIndex = boneIndex;                     // +0x4C
        m_bones[boneIndex].m_ikHandleIndex = static_cast<char>(handleIndex); // +0x44 narrowed
    }
    else
    {
        handleIndex = existing;
    }

    IkHandle& handle = m_ikHandles[handleIndex];
    handle.m_target.x = target.x;
    handle.m_target.y = target.y;
    handle.m_target.z = target.z;
    handle.m_effectorBoneIndex = effectorBoneIndex;                           // +0x50
    handle.m_transform = transform;                                           // +0x0C
}

} // namespace anim
} // namespace dice
