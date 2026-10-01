// Loads, generates and selects vegetation instances for camera cells.
// [CHOSEN, not recovered] split from OverGrowth.cpp. Record/field names are [INFERRED].
// Game instances come from level objects; procedural generation requires editorEnable.
// Preserve float evaluation order and 24-bit precision.
// [FRONTIER] RaStreamDataEditable device hooks remain unavailable.
// [RETIRE-WHEN: overgrowth-stream-device-reset] Implement the resource-tracker/device hooks
// and verify managed-stream lifecycle through device loss/reset before removing the stops.
#ifdef BF2_IMAGE_RENDDX9
#include "OverGrowth.h"
#include "OverGrowthCell.h"
#include "../MeshTemplate.h"                        // MeshBounds, meshBoundsCopy / meshBoundsExpand
#include "../Terrain.h"                             // terrainFrustumIntersectsBox (sub_101C7980)
#include "../../Math/Frustum.h"
#include "../../Math/Mat4.h"
#include "../../RendMisc/IRenderView.h"
#include "../../RendDX9/Ra/RaShader.h"              // getShader, the object block, RaVertexElementDesc
#include "../../RendDX9/Ra/RaStreamDataEditable.h"  // the cell's two vertex streams
#include "../../RendDX9/Texture.h"                  // EngineTexture / textureDtor (sub_10014E20)
#include "../../../RendDX9Abi/BfMemory.h"
#include "../../../RendDX9Abi/Vc71String.h"
#include "../../../RendDX9Abi/Vc71Vector.h"

// The host interfaces, typed in both flavours (RW-266): every slot called below sits at the retail PE
// offset in this tree's MSVC tables in both the native and the retail-ABI build (probe\probe_calls.cmd:
// getHeightInWorldCoords(float,float) +0x6C, getNormalInWorldCoords(float,float,Vec3&) +0x7C, IHeightmap
// getScale +0x54, IObjectTemplate getCullRadiusScale +0x70 and getAnchorPos +0xAC) and none passes an STL
// type. The retail-ABI flavour borrows HfeAbi's IBase, exactly as XpLightManager.cpp does.
#if !BF2_PLUGIN_ABI_NATIVE
#define CORE_IBASE_H
#define CORE_CLASSMANAGER_H
#endif
#include "../../Game/Objects/IObjectTemplate.h"
#include "../../Physics/HeightmapCluster.h"
#if !BF2_PLUGIN_ABI_NATIVE
#undef CORE_CLASSMANAGER_H
#undef CORE_IBASE_H
#endif

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>
#include <vector>

namespace dice { namespace hfe { namespace dx9rend {
extern void*  g_OverGrowth;     // BF2Render.cpp - dword_10258F38, the level OverGrowth
extern IBase* g_ObjectDrawer;   // BF2Render.cpp - dword_1025A39C
extern void*  g_renderQueues;   // RenderQueues.cpp - dword_1023E408, the Ra shader table
} } }

namespace dice { namespace hfe { namespace geom {


// Shared vegetation records retain fixed strides: instance 0x60, patch 0x98 and cell 0x2AC.
// Instance transforms, spacing and generation flags feed patch coverage and cell batching.


namespace {

template<class T> T& field(void* object, unsigned int offset)
{ return *reinterpret_cast<T*>(static_cast<unsigned char*>(object) + offset); }
template<class T> const T& field(const void* object, unsigned int offset)
{ return *reinterpret_cast<const T*>(static_cast<const unsigned char*>(object) + offset); }

// A raw __thiscall slot on one of RendDX9's OWN objects (geometry instances, the texture's shared
// table, the ObjectDrawer's hand-built table) - the house `dispatch` from BundledMesh.cpp.
template<class R, class... A> R dispatch(void* self, unsigned int offset, A... args)
{
    typedef R(__fastcall* Fn)(void*, void*, A...);
    return reinterpret_cast<Fn>((*static_cast<void***>(self))[offset / 4])(self, 0, args...);
}

// [TEMP] [CHOSEN, not recovered] Known-signature boundaries always report, flush and stop
// (OverGrowth.cpp's house style); the stream's device-reset pair below is the only user.
__declspec(noreturn) void unresolvedAt(const char* qualified)
{
    std::fprintf(stderr, "[FRONTIER] %s reached\n", qualified);
    std::fflush(stderr);
    std::abort();
}

const unsigned int kCellBytes         = 0x2AC;  // sub_1005F320(0x2AC) at 0x100CD6DE
const unsigned int kCellMeshBytes     = 0x3C8;  // sub_1005F320(0x3C8) at 0x100D0ECE / 0x100D0F2B
const unsigned int kPatchBytes        = 0x98;
const unsigned int kInstanceBytes     = 0x60;   // `imul 2AAAAAABh; sar edx, 4` = /0x60
const unsigned int kGeometryTypeBytes = 0xA0;   // `lea eax,[eax+eax*4]; shl eax, 5`
const unsigned int kMaterialBytes     = 0x30;   // `imul 2AAAAAABh; sar edx, 3` = /0x30
const unsigned int kMaterialTypeBytes = 0x5C;   // `imul 0B21642C9h; add edx, ecx; sar edx, 6` = /0x5C

// flt_1020CF68 (0x33800000) = 2^-24 and flt_102193D8 (0x37B40000) = 360 / 2^24.
const float kRandomUnit      = 5.9604644775390625e-08f;
const float kDegreesPerCount = 2.1457672119140625e-05f;


// byte_10260610 - .bss, ONE reader (0x1015F119) and neither a writer nor an address-of anywhere in the
// image, so it is false for the whole run; byte_1023A041 / byte_1023A040 - .data, file-backed 1, one
// reader each (0x1015F2D8 / 0x1015F30D), no writer. Kept as mutable globals so each test is the
// independent load the PE performs (BF2Render.cpp's rule for byte_10238249). [INFERRED] names.

} // anonymous namespace
unsigned char g_overGrowthBuildEveryCell     = 0;   // byte_10260610
unsigned char g_overGrowthCellDistanceCull   = 1;   // byte_1023A041
unsigned char g_overGrowthCellFrustumCull    = 1;   // byte_1023A040
namespace {


// Compatibility-vector helpers for instance records and pointers, both trivially copyable.
// Inserts grow by half capacity, at least size+count; assignment grows exactly to source size.

unsigned int vectorSize(const vc71::vector_layout& v, unsigned int bytes)
{ return v.first ? static_cast<unsigned int>(v.last - v.first) / bytes : 0u; }
unsigned int vectorCapacity(const vc71::vector_layout& v, unsigned int bytes)
{ return v.first ? static_cast<unsigned int>(v.end - v.first) / bytes : 0u; }
unsigned char* vectorElement(const vc71::vector_layout& v, unsigned int index, unsigned int bytes)
{ return reinterpret_cast<unsigned char*>(v.first) + index * bytes; }

// _Insert_n(where, count, value) - 100CF0D0 / 100D0270. The value is copied before anything moves.
void vectorInsertN(vc71::vector_layout& v, unsigned int bytes, char* where, unsigned int count,
                   const void* value)
{
    unsigned char copy[kInstanceBytes];
    std::memcpy(copy, value, bytes);
    if (count == 0)
        return;
    const unsigned int maxSize = 0xFFFFFFFFu / bytes;
    const unsigned int size = vectorSize(v, bytes);
    if (maxSize - size < count)
        throw std::length_error("vector<T> too long");              // _Xlen, sub_100E7130
    const unsigned int capacity = vectorCapacity(v, bytes);
    const unsigned int head = v.first ? static_cast<unsigned int>(where - v.first) : 0u;
    const unsigned int tail = v.first ? static_cast<unsigned int>(v.last - where) : 0u;
    if (capacity >= size + count)
    {
        std::memmove(where + count * bytes, where, tail);
        for (unsigned int i = 0; i < count; ++i)
            std::memcpy(where + i * bytes, copy, bytes);
        v.last += count * bytes;
        return;
    }
    unsigned int grown = (maxSize - (capacity >> 1) >= capacity) ? capacity + (capacity >> 1) : 0u;
    if (grown < size + count)
        grown = size + count;
    char* const buffer = static_cast<char*>(::renddx9abi::engineAlloc(grown * bytes));
    if (head)
        std::memcpy(buffer, v.first, head);
    for (unsigned int i = 0; i < count; ++i)
        std::memcpy(buffer + head + i * bytes, copy, bytes);
    if (tail)
        std::memcpy(buffer + head + count * bytes, where, tail);
    if (v.first)
        ::renddx9abi::engineFree(v.first);
    v.end = buffer + grown * bytes;
    v.last = buffer + (size + count) * bytes;
    v.first = buffer;
}

// push_back - the inline fast path (0x100CB257 / 0x100D0E62) or _Insert_n(end(), 1, value).
void vectorPushBack(vc71::vector_layout& v, unsigned int bytes, const void* value)
{
    if (v.first && vectorSize(v, bytes) < vectorCapacity(v, bytes))
    {
        std::memcpy(v.last, value, bytes);
        v.last += bytes;
        return;
    }
    vectorInsertN(v, bytes, v.last, 1, value);
}

// resize(count, value) - grow through _Insert_n, shrink by erasing the tail.
void vectorResize(vc71::vector_layout& v, unsigned int bytes, unsigned int count, const void* value)
{
    const unsigned int size = vectorSize(v, bytes);
    if (size < count)
        vectorInsertN(v, bytes, v.last, count - size, value);
    else if (v.first && count < size)
        v.last = v.first + count * bytes;
}

// _Tidy - free the buffer and zero first/last/end (the allocator word is never written).
void vectorTidy(vc71::vector_layout& v)
{
    if (v.first)
        ::renddx9abi::engineFree(v.first);
    v.first = 0;
    v.last = 0;
    v.end = 0;
}

// operator= - 100D0700 (instances).
void vectorAssign(vc71::vector_layout& dst, const vc71::vector_layout& src, unsigned int bytes)
{
    if (&dst == &src)
        return;
    const unsigned int count = vectorSize(src, bytes);
    if (count == 0)
    {
        vectorTidy(dst);
        return;
    }
    const unsigned int size = vectorSize(dst, bytes);
    if (count <= size)                                               // copy, then drop the tail
    {
        std::memmove(dst.first, src.first, count * bytes);
        dst.last = dst.first + count * bytes;
        return;
    }
    if (count <= vectorCapacity(dst, bytes))                         // copy the overlap, construct the rest
    {
        std::memmove(dst.first, src.first, size * bytes);
        std::memcpy(dst.last, src.first + size * bytes, (count - size) * bytes);
        dst.last = dst.first + count * bytes;
        return;
    }
    if (dst.first)
        ::renddx9abi::engineFree(dst.first);
    dst.first = 0;                                                   // _Buy (100CE6D0) zeroes first
    dst.last = 0;
    dst.end = 0;
    if (count > 0xFFFFFFFFu / bytes)
        throw std::length_error("vector<T> too long");
    char* const buffer = static_cast<char*>(::renddx9abi::engineAlloc(count * bytes));
    dst.end = buffer + count * bytes;
    dst.first = buffer;
    dst.last = buffer;
    std::memcpy(buffer, src.first, count * bytes);                   // _Ucopy (100CFC90)
    dst.last = buffer + count * bytes;
}


// Local placement math helpers.


// sub_1000ED40 (__stdcall, retn 0Ch): `fcomi high; fcmovnb` then `fcomi low; fcmovb`. The high bound
// is applied first; a NaN comes back as `low`.
float clampFloat(float value, float low, float high)
{
    if (value >= high)
        value = high;
    if (!(value >= low))
        value = low;
    return value;
}

// sub_100777A0 - normalise in place unless |v|^2 is within FLT_EPSILON (flt_10208958) of 1; below
// FLT_EPSILON it zeroes and answers -20001. |v|^2 is ((x*x + y*y) + z*z).
int normalizeVec3(Vec3& v)
{
    const float lengthSquared = v.x * v.x + v.y * v.y + v.z * v.z;
    if (std::fabs(lengthSquared - 1.0f) < 1.1920928955078125e-07f)
        return 0;
    if (std::fabs(lengthSquared) < 1.1920928955078125e-07f)
    {
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = 0.0f;
        return -20001;
    }
    const float scale = 1.0f / std::sqrt(lengthSquared);
    v.x = scale * v.x;
    v.y = scale * v.y;
    v.z = scale * v.z;
    return 0;
}

// sub_10077E30 (retn 4) - the Y rotation: identity, then m0 = cos, m2 = -sin, m8 = sin, m10 = cos of
// flt_10238738 (0x3C8EFA35, pi/180 in .data, never written) times the angle, through x87 fsincos on the
// float-rounded radians (0x10077E36..0x10077E92).
void setRotationY(Mat4& matrix, float degrees)
{
    const float radians = 0.01745329238474369f * degrees;
    float sine;
    float cosine;
    __asm
    {
        fld radians
        fsincos
        fstp cosine
        fstp sine
    }
    for (unsigned int i = 0; i < 16; ++i)
        matrix.m[i] = 0.0f;
    matrix.m[0] = 1.0f;
    matrix.m[5] = 1.0f;
    matrix.m[10] = 1.0f;
    matrix.m[15] = 1.0f;
    matrix.m[0] = cosine;
    matrix.m[2] = -sine;
    matrix.m[8] = sine;
    matrix.m[10] = cosine;
}

// sub_100C8820 - re-orthonormalise a basis keeping its y row: normalise rows 1 and 2, row0 = row1 x
// row2, row2 = row0 x row1 (from the new row0). Operand order as the fmul pairs read.
void orthonormalizeKeepingY(Mat4& matrix)
{
    Vec3& row0 = *reinterpret_cast<Vec3*>(&matrix.m[0]);
    Vec3& row1 = *reinterpret_cast<Vec3*>(&matrix.m[4]);
    Vec3& row2 = *reinterpret_cast<Vec3*>(&matrix.m[8]);
    normalizeVec3(row1);
    normalizeVec3(row2);
    row0.x = row2.z * row1.y - row2.y * row1.z;
    row0.y = row1.z * row2.x - row2.z * row1.x;
    row0.z = row2.y * row1.x - row1.y * row2.x;
    row2.x = row1.z * row0.y - row1.y * row0.z;
    row2.y = row1.x * row0.z - row1.z * row0.x;
    row2.z = row1.y * row0.x - row1.x * row0.y;
}

// lcgrand_step, sub_1009EF30 - RendDX9's copy of dice::hfe::Random::getRaw: two Schrage steps of the
// Law-Kelton lcgrand (24112 = 0x5E30, then 26143 = 0x661F) over the owner's +0x18. Written in unsigned
// arithmetic with the shipped `sar`s (NametagManager.cpp's randomStep, repeated TU-locally).
unsigned int randomStep(unsigned int value, unsigned int multiplier)
{
    const unsigned int low = (value & 0xFFFFu) * multiplier;
    const unsigned int high = static_cast<unsigned int>(static_cast<int>(value) >> 16) * multiplier;
    const int middle = static_cast<int>((low >> 16) + high);
    const unsigned int next = (low & 0xFFFFu) + ((static_cast<unsigned int>(middle) & 0x7FFFu) << 16)
        + static_cast<unsigned int>(middle >> 15);
    const int reduced = static_cast<int>(next - 0x7FFFFFFFu);
    return reduced < 0 ? next : static_cast<unsigned int>(reduced);
}

// Every draw in this half is `lcgrand_step(this + 0x18); or al, 80h; sar eax, 7; add eax, 1` - a count
// in [1, 2^24] (0x100C9995, 0x1015E895, 0x100CCA00).
int random24(OverGrowth* self)
{
    unsigned int& seed = field<unsigned int>(self, 0x18);
    seed = randomStep(randomStep(seed, 24112u), 26143u);
    return (static_cast<int>(seed | 0x80u) >> 7) + 1;
}

world::IHeightmapCluster* heightmapCluster(OverGrowth* self)
{ return field<world::IHeightmapCluster*>(self, 0xAC); }                  // initialize: "HeightmapCluster"


// sub_101C5C50 (retn 8) - does the sphere (point, radius) reach the box? The squared distance from the
// point to the box is summed per axis ((x + y) + z; an axis inside contributes nothing) and the answer
// is `distance <= radius * radius` (test ah, 41h / jp - a NaN answers false).
bool boundsSphereIntersects(const MeshBounds& bounds, const Vec3& point, float radius)
{
    float distance = 0.0f;
    if (point.x < bounds.min.x)
    {
        const float t = point.x - bounds.min.x;
        distance = t * t;
    }
    else if (point.x > bounds.max.x)
    {
        const float t = point.x - bounds.max.x;
        distance = t * t;
    }
    if (point.y < bounds.min.y)
    {
        const float t = point.y - bounds.min.y;
        distance = distance + t * t;
    }
    else if (point.y > bounds.max.y)
    {
        const float t = point.y - bounds.max.y;
        distance = distance + t * t;
    }
    if (point.z < bounds.min.z)
    {
        const float t = point.z - bounds.min.z;
        distance = distance + t * t;
    }
    else if (point.z > bounds.max.z)
    {
        const float t = point.z - bounds.max.z;
        distance = distance + t * t;
    }
    return distance <= radius * radius;
}

// The 96-byte eight-corner box: sub_100CE120 (retn 8) zeroes the 24 floats and then calls
// sub_101C6750 (min/max to corners, in bit order x = 4, y = 2, z = 1) and sub_101C67F0 (every corner
// through the full matrix). sub_101C67F0 associates corner 0's x and y sums differently from corners
// 1-7; both orders are kept (BundledMeshDraw.cpp's worldHeight reads the same body the same way).
struct OverGrowthBoxCorners
{
    float corner[8][3];
};

void constructBoxCorners(OverGrowthBoxCorners& box, const MeshBounds& bounds, const Mat4& world)
{
    for (unsigned int i = 0; i < 8; ++i)
        for (unsigned int j = 0; j < 3; ++j)
            box.corner[i][j] = 0.0f;
    for (unsigned int i = 0; i < 8; ++i)
    {
        box.corner[i][0] = (i & 4) ? bounds.max.x : bounds.min.x;
        box.corner[i][1] = (i & 2) ? bounds.max.y : bounds.min.y;
        box.corner[i][2] = (i & 1) ? bounds.max.z : bounds.min.z;
    }
    const float* const m = world.m;
    for (unsigned int i = 0; i < 8; ++i)
    {
        float* const c = box.corner[i];
        const float x = c[0];
        const float y = c[1];
        const float z = c[2];
        float tx;
        float ty;
        float tz;
        if (i == 0)
        {
            tz = m[2] * x + z * m[10] + y * m[6] + m[14];
            ty = m[1] * x + y * m[5] + z * m[9] + m[13];
            tx = m[8] * z + m[0] * x + y * m[4] + m[12];
        }
        else
        {
            tz = z * m[10] + x * m[2] + y * m[6] + m[14];
            ty = x * m[1] + z * m[9] + y * m[5] + m[13];
            tx = m[4] * y + z * m[8] + x * m[0] + m[12];
        }
        c[0] = tx;
        c[1] = ty;
        c[2] = tz;
    }
}

// sub_101C6E10 - the component-wise minimum: `if (current > corner) current = corner` (test ah, 41h).
Vec3 boxCornersMin(const OverGrowthBoxCorners& box)
{
    Vec3 out;
    out.x = box.corner[0][0];
    out.y = box.corner[0][1];
    out.z = box.corner[0][2];
    for (unsigned int i = 1; i < 8; ++i)
    {
        if (out.x > box.corner[i][0]) out.x = box.corner[i][0];
        if (out.y > box.corner[i][1]) out.y = box.corner[i][1];
        if (out.z > box.corner[i][2]) out.z = box.corner[i][2];
    }
    return out;
}

// sub_101C6C70 - the component-wise maximum: `if (current < corner) current = corner` (test ah, 5).
Vec3 boxCornersMax(const OverGrowthBoxCorners& box)
{
    Vec3 out;
    out.x = box.corner[0][0];
    out.y = box.corner[0][1];
    out.z = box.corner[0][2];
    for (unsigned int i = 1; i < 8; ++i)
    {
        if (out.x < box.corner[i][0]) out.x = box.corner[i][0];
        if (out.y < box.corner[i][1]) out.y = box.corner[i][1];
        if (out.z < box.corner[i][2]) out.z = box.corner[i][2];
    }
    return out;
}

// sub_101C5450 (retn 4) - grow the MeshBounds by the corners' minimum, then their maximum.
void boundsExpandByCorners(MeshBounds& bounds, const OverGrowthBoxCorners& box)
{
    meshBoundsExpand(bounds, boxCornersMin(box));
    meshBoundsExpand(bounds, boxCornersMax(box));
}

// The geometry instance slots this half reads (RendDX9's own StaticMesh table; StaticMesh.h names them):
// +0x38 getTemplate, +0x40 getLocalBounds (min/max at +0 / +0xC), +0x44 getBoundingRadius.
void* geometryTemplate(void* geometry) { return dispatch<void*>(geometry, 0x38); }
const MeshBounds* geometryLocalBounds(void* geometry)
{ return dispatch<const MeshBounds*>(geometry, 0x40); }
float geometryBoundingRadius(void* geometry) { return dispatch<float>(geometry, 0x44); }


// Density and shadow maps share the stored width and height.


// sub_100C88F0 (retn 18h) - world (x, z) to shadow/density map coordinates through the primary
// heightmap's scale (+0xA8, IHeightmap::getScale called twice, as shipped). The integer part is the
// floor, re-centred by width/2 (unsigned shr); the fraction goes out separately.
// The clamp allows index == width (`cmp; ja` then `test; jge`): an index one past the row is not
// rejected here. Preserved.
void shadowCoords(OverGrowth* self, float x, float z, int* ix, int* iz, float* fx, float* fz)
{
    world::IHeightmap* const heightmap = field<world::IHeightmap*>(self, 0xA8);
    const float scaledX = x / heightmap->getScale().x;               // vslot +0x54, 0x100C88FC
    const float scaledZ = z / heightmap->getScale().z;               // vslot +0x54 again, 0x100C8911
    const float floorX = static_cast<float>(std::floor(static_cast<double>(scaledX)));
    const float floorZ = static_cast<float>(std::floor(static_cast<double>(scaledZ)));
    *fx = scaledX - floorX;
    *fz = scaledZ - floorZ;
    *ix = static_cast<int>(floorX);
    *iz = static_cast<int>(floorZ);
    *ix = static_cast<int>(static_cast<unsigned int>(*ix) + (field<unsigned int>(self, 0x78) >> 1));
    *iz = static_cast<int>(static_cast<unsigned int>(*iz) + (field<unsigned int>(self, 0x7C) >> 1));
    if (static_cast<unsigned int>(*ix) > field<unsigned int>(self, 0x78) || *ix < 0)
        *ix = 0;
    if (static_cast<unsigned int>(*iz) > field<unsigned int>(self, 0x7C) || *iz < 0)
        *iz = 0;
}

// sub_100C92F0 (retn 8) - the bilinear shadow-map value at (x, z): four taps (byte * 1/255,
// flt_101E2B98 = 0x3B808081; a tap outside the map is 0), blended ((s10 - s00) * fx + s00) and
// ((s11 - s01) * fx + s01), then ((b - a) * fz + a).
float sampleShadow(OverGrowth* self, float x, float z)
{
    int ix;
    int iz;
    float fx;
    float fz;
    shadowCoords(self, x, z, &ix, &iz, &fx, &fz);
    const float k = 0.003921568859368563f;
    const unsigned char* const shadow = field<unsigned char*>(self, 0x84);
    const unsigned int width = field<unsigned int>(self, 0x78);
    const unsigned int height = field<unsigned int>(self, 0x7C);
    const int ix1 = ix + 1;
    const int iz1 = iz + 1;
    float s00 = 0.0f;
    if (ix >= 0 && static_cast<unsigned int>(ix) < width && iz >= 0 && static_cast<unsigned int>(iz) < height)
        s00 = static_cast<float>(shadow[width * iz + ix]) * k;
    float s10 = 0.0f;
    if (ix1 >= 0 && static_cast<unsigned int>(ix1) < width && iz >= 0 && static_cast<unsigned int>(iz) < height)
        s10 = static_cast<float>(shadow[width * iz + ix + 1]) * k;
    float s01 = 0.0f;
    if (ix >= 0 && static_cast<unsigned int>(ix) < width && iz1 >= 0 && static_cast<unsigned int>(iz1) < height)
        s01 = static_cast<float>(shadow[width * iz1 + ix]) * k;
    float s11 = 0.0f;
    if (ix1 >= 0 && static_cast<unsigned int>(ix1) < width && iz1 >= 0 && static_cast<unsigned int>(iz1) < height)
        s11 = static_cast<float>(shadow[width * iz1 + ix + 1]) * k;
    const float a = (s10 - s00) * fx + s00;
    const float b = (s11 - s01) * fx + s01;
    return (b - a) * fz + a;
}

// sub_100C9AE0 (retn 4) - one patch's density histogram: every density texel from the patch corner's
// map cell to the far corner's (inclusive, rows then columns), then coverage[i] = count[i] / texels for
// the first sixteen values. The two ends come from shadowCoords, so a patch touching the map's far edge
// reads texel index == width (the next row's first byte, or one past the buffer on the last row) - the
// shipped clamp's off-by-one, preserved.
void computeCoverage(OverGrowth* self, unsigned char* patch)
{
    const float x0 = field<float>(patch, 0x00);
    const float z0 = field<float>(patch, 0x08);
    const float x1 = field<float>(self, 0x14) + field<float>(patch, 0x00);
    const float z1 = field<float>(self, 0x14) + field<float>(patch, 0x08);
    int ix0;
    int iz0;
    int ix1;
    int iz1;
    float fx;
    float fz;
    shadowCoords(self, x0, z0, &ix0, &iz0, &fx, &fz);
    shadowCoords(self, x1, z1, &ix1, &iz1, &fx, &fz);
    unsigned int counts[256];
    for (unsigned int i = 0; i < 256; ++i)
        counts[i] = 0;
    const unsigned char* const density = field<unsigned char*>(self, 0x80);
    for (int iz = iz0; iz <= iz1; ++iz)
        for (int ix = ix0; ix <= ix1; ++ix)
            ++counts[density[field<unsigned int>(self, 0x78) * iz + ix]];
    const int texels = (iz1 - iz0 + 1) * (ix1 - ix0 + 1);
    const float total = static_cast<float>(static_cast<unsigned int>(texels));
    for (unsigned int i = 0; i < 16; ++i)
        field<float>(patch, 0x1C + i * 4) = static_cast<float>(counts[i]) / total;
}

// sub_1015EB20 (retn 4) - the material whose density value (+0x1C) is `value`, or null.
unsigned char* findMaterial(OverGrowth* self, int value)
{
    const vc71::vector_layout& materials = field<vc71::vector_layout>(self, 0x3C);
    for (unsigned int i = 0; i < vectorSize(materials, kMaterialBytes); ++i)
    {
        unsigned char* const material = vectorElement(materials, i, kMaterialBytes);
        if (field<int>(material, 0x1C) == value)
            return material;
    }
    return 0;
}

// sub_100C88C0 (retn 8) - patches[z * patchCountX + x].
unsigned char* patchAt(OverGrowth* self, int x, int z)
{ return field<unsigned char*>(self, 0xE0) + (field<int>(self, 0xD8) * z + x) * kPatchBytes; }

// sub_1015E880 (retn 8) - a uniformly random point of the patch's cell square, dropped onto the terrain;
// answers the density value there (vslot +0x18). Two draws: x first, then z; the cell size is re-read
// between them. Each offset is ((float)count * cellSize) * 2^-24.
unsigned int pickPosition(OverGrowth* self, const unsigned char* patch, Vec3* out)
{
    const float cellSizeX = field<float>(self, 0x14);
    const float offsetX = static_cast<float>(random24(self)) * cellSizeX * kRandomUnit;
    const float cellSizeZ = field<float>(self, 0x14);
    const float offsetZ = static_cast<float>(random24(self)) * cellSizeZ * kRandomUnit;
    out->x = offsetX + field<float>(patch, 0x00);
    out->y = field<float>(patch, 0x04);
    out->z = offsetZ + field<float>(patch, 0x08);
    out->y = heightmapCluster(self)->getHeightInWorldCoords(out->x, out->z);   // vslot +0x6C
    return self->sampleDensity(out->x, out->z);                      // vslot +0x18, 0x1015E919
}

// sub_1015E730 (retn 4) - the cell's distance to a point: the smallest of its four corners' squared
// distances ((dz*dz + dy*dy) + dx*dx), corner walked in place (x += size, z += size, x -= size - so the
// last x is (x0 + size) - size, not x0), then one fsqrt. `m = m < d ? m : d` (fcom; jnp keeps m).
float cellDistanceToPoint(const OverGrowthCell* cell, const Vec3* point)
{
    float cornerX = field<float>(cell, 0x08);
    const float cornerY = field<float>(cell, 0x0C);
    float cornerZ = field<float>(cell, 0x10);
    const float size = field<float>(cell, 0x14);
    float dx = cornerX - point->x;
    float dy = cornerY - point->y;
    float dz = cornerZ - point->z;
    float nearest = dz * dz + dy * dy + dx * dx;
    cornerX = cornerX + size;
    dx = cornerX - point->x;
    dy = cornerY - point->y;
    dz = cornerZ - point->z;
    float candidate = dz * dz + dy * dy + dx * dx;
    nearest = nearest < candidate ? nearest : candidate;
    cornerZ = cornerZ + size;
    dx = cornerX - point->x;
    dy = cornerY - point->y;
    dz = cornerZ - point->z;
    candidate = dz * dz + dy * dy + dx * dx;
    nearest = nearest < candidate ? nearest : candidate;
    dx = (cornerX - size) - point->x;
    dy = cornerY - point->y;
    dz = cornerZ - point->z;
    candidate = dz * dz + dy * dy + dx * dx;
    nearest = nearest < candidate ? nearest : candidate;
    return std::sqrt(nearest);
}

// loc_1015E580 / sub_1015E5A0 - update()'s two sort predicates (ltcg register convention in the PE:
// ecx = a, edx = b). Both compare the camera distance +0x1C. 1015E580 answers a < b (test ah, 5 / jp);
// 1015E5A0 answers a > b (test ah, 41h / jnz).
bool cellNearerFirst(const OverGrowthCell* a, const OverGrowthCell* b)
{ return field<float>(a, 0x1C) < field<float>(b, 0x1C); }
bool cellFartherFirst(const OverGrowthCell* a, const OverGrowthCell* b)
{ return field<float>(a, 0x1C) > field<float>(b, 0x1C); }

// sub_1015E930 - the instance's default constructor. The matrix half is the retail Mat4() inlined: all
// sixteen zeroed, then 1.0f on the diagonal. +0x54 (the shadow value) and the pad after +0x58 are NOT
// written - every shipped user stores +0x54 before reading it.
void constructInstance(unsigned char* instance)
{
    field<int>(instance, 0x00) = -1;
    for (unsigned int i = 0; i < 16; ++i)
        field<float>(instance, 0x04 + i * 4) = 0.0f;
    field<float>(instance, 0x40) = 1.0f;
    field<float>(instance, 0x2C) = 1.0f;
    field<float>(instance, 0x18) = 1.0f;
    field<float>(instance, 0x04) = 1.0f;
    field<float>(instance, 0x44) = 1.0f;
    field<float>(instance, 0x48) = 0.0f;
    field<float>(instance, 0x4C) = 0.0f;
    field<float>(instance, 0x50) = 0.0f;
    field<unsigned char>(instance, 0x58) = 0;
    field<float>(instance, 0x5C) = 1.0f;
}

// The occlusion query update() makes for every candidate cell: `mov ecx, dword_1025A39C; call
// [eax+3Ch]` with the cell's MeshBounds (0x1015F32B), through the drawer's table like the PE and
// unguarded. The slot is ObjectDrawer::isBoundsOccluded (0x100F8000, Geom/ObjectDrawer.cpp). It
// refreshes the box's derived data IN place when its dirty byte is set, hence the non-const box.
bool objectDrawerOccludes(MeshBounds& bounds)
{
    return dispatch<bool>(static_cast<void*>(dx9rend::g_ObjectDrawer), 0x3C,
                          static_cast<void*>(&bounds));
}

// The texture store rebuildCells inlines for each stream (0x100CAE5E..0x100CAEAC, 0x100CAEAC..0x100CAEFA):
// the stream's material-block texture slot (+0xC8) takes the level OverGrowth's atlas texture (+0x100 of
// dword_10258F38 - the global, not `this`). An unchanged pointer is left alone; the old one is released
// owner-side (`if (!--ref) { sub_10014E20; free }`, the slot is not cleared first) and the new one is
// addRef'd through its shared table's +0x08.
void assignAtlasTexture(unsigned char* stream)
{
    dx9rend::EngineTexture* const atlas = field<dx9rend::EngineTexture*>(dx9rend::g_OverGrowth, 0x100);
    dx9rend::EngineTexture* const old = field<dx9rend::EngineTexture*>(stream, 0xC8);
    if (old == atlas)
        return;
    if (old && --old->refCount == 0)
    {
        dx9rend::textureDtor(old);
        ::renddx9abi::engineFree(old);
    }
    field<dx9rend::EngineTexture*>(stream, 0xC8) = atlas;
    if (atlas)
        dispatch<void>(static_cast<void*>(atlas), 0x08);             // the shared table's addRef
}

} // anonymous namespace


// OverGrowthCellMesh

OverGrowthCellMesh::OverGrowthCellMesh(unsigned int vertexCapacity, unsigned int indexCapacity,
                                       const vc71::vector_layout& elements)
    : RaStreamDataEditable(vertexCapacity, indexCapacity, 16u, elements, dx9rend::Ra::raRendererDevice())
{
    dx9rend::Ra::raObjectParametersConstruct(m_objectParameters);    // sub_100458E0(this + 0x1A4)
    field<float>(this, 0x354) = 1.0f;
}

// 100D0C60 (retn 4) - the block (sub_1004D900), then RaStreamDataEditable's members (sub_10048F00), then
// the engine free when bit 0 is set.
void* OverGrowthCellMesh::destroy(unsigned char flags)
{
    dx9rend::Ra::raObjectParametersDestroy(m_objectParameters);
    destroyMembers();
    if (flags & 1)
        ::renddx9abi::engineFree(this);
    return this;
}

// [TEMP] [CHOSEN, not recovered] RaStreamDataEditable's device-reset pair. Nothing in this tree calls
// either slot; declared only so +0x38/+0x3C land at their retail offsets.
// [RETIRE-WHEN: overgrowth-stream-device-reset] - the pair recovered into RaStreamDataEditable.cpp.
void OverGrowthCellMesh::releaseBuffers()
{
    unresolvedAt("dice::hfe::dx9rend::Ra::RaStreamDataEditable +0x30 (10048D60, release the D3D buffers)");
}
void OverGrowthCellMesh::createBuffers()
{
    unresolvedAt("dice::hfe::dx9rend::Ra::RaStreamDataEditable +0x40 (10048C70, create the D3D buffers)");
}

// 10048EF0: `mov eax, [ecx+190h]; sub eax, [ecx+18Ch]`.
unsigned int OverGrowthCellMesh::getFreeVertexCount() const { return m_vertexCapacity - m_vertexCount; }
// 10112970: `mov eax, [ecx+190h]`.   100EE9D0: `mov eax, [ecx+194h]`.
unsigned int OverGrowthCellMesh::getVertexCapacity() const { return m_vertexCapacity; }
unsigned int OverGrowthCellMesh::getIndexCapacity() const { return m_indexCapacity; }


// Initializes an unplaced cell and its leaf/trunk streams using the global level budgets.
// Vertices use SHORT4 position, D3DCOLOR normal, and SHORT2 texcoord (16 bytes total).
// Failed engine allocations leave the corresponding stream null.
OverGrowthCell::OverGrowthCell()
{

    field<unsigned char>(this, 0x04) = 0;
    field<unsigned char>(this, 0x05) = 0;
    field<unsigned char>(this, 0x06) = 1;
    for (unsigned int offset = 0x08; offset <= 0x3C; offset += 4)
        field<unsigned int>(this, offset) = 0;
    field<unsigned char>(this, 0x40) = 1;
    for (unsigned int offset = 0x44; offset <= 0x58; offset += 4)
        field<unsigned int>(this, offset) = 0;
    field<int>(this, 0x5C) = 0x2CEE9D7;
    field<int>(this, 0x60) = 0x2CEE9D7;


    field<unsigned char>(this, 0x64) = 0;
    dx9rend::Ra::raObjectParametersConstruct(&field<unsigned char>(this, 0x68));
    field<char*>(this, 0x290) = 0;
    field<char*>(this, 0x294) = 0;
    field<char*>(this, 0x298) = 0;
    field<unsigned int>(this, 0x29C) = 0;
    field<void*>(this, 0x2A0) = 0;

    static const dx9rend::Ra::RaVertexElementDesc s_position(0, 7, 0, "Vtx_Position");
    static const dx9rend::Ra::RaVertexElementDesc s_normal(1, 4, 3, "Vtx_Normal");
    static const dx9rend::Ra::RaVertexElementDesc s_texCoord(2, 6, 5, "Vtx_TBase");

    vc71::vector_layout elements = { 0, 0, 0, 0 };
    const dx9rend::Ra::RaVertexElementDesc* element = &s_position;
    vectorInsertN(elements, 4u, elements.last, 1, &element);         // (the empty vector)
    element = &s_normal;
    vectorPushBack(elements, 4u, &element);
    element = &s_texCoord;
    vectorPushBack(elements, 4u, &element);

    void* const owner = dx9rend::g_OverGrowth;
    void* const leafMemory = ::renddx9abi::engineAlloc(kCellMeshBytes);
    field<OverGrowthCellMesh*>(this, 0x2A4) = leafMemory
        ? ::new (leafMemory) OverGrowthCellMesh(field<unsigned int>(owner, 0x104),
                                                field<unsigned int>(owner, 0x10C), elements)
        : 0;
    void* const trunkMemory = ::renddx9abi::engineAlloc(kCellMeshBytes);
    field<OverGrowthCellMesh*>(this, 0x2A8) = trunkMemory
        ? ::new (trunkMemory) OverGrowthCellMesh(field<unsigned int>(owner, 0x108),
                                                 field<unsigned int>(owner, 0x110), elements)
        : 0;
    vectorTidy(elements);
}

// +0x00: PE 0x100D1890 (the scalar deleting wrapper) over 0x100D18B0 (the body, read in full) and
// (the base destructor, which only republishes the base table - not modelled). Each stream is
// destroyed through its own +0x00 with 1 (no null store afterwards), the far list is freed and zeroed,
// then the Ra object block (sub_1004D900).
void* OverGrowthCell::destroy(unsigned char flags)
{
    OverGrowthCellMesh* const leaf = field<OverGrowthCellMesh*>(this, 0x2A4);
    if (leaf)
        leaf->destroy(1);
    OverGrowthCellMesh* const trunk = field<OverGrowthCellMesh*>(this, 0x2A8);
    if (trunk)
        trunk->destroy(1);
    vectorTidy(field<vc71::vector_layout>(this, 0x28C));
    dx9rend::Ra::raObjectParametersDestroy(&field<unsigned char>(this, 0x68));
    if (flags & 1)
        ::renddx9abi::engineFree(this);
    return this;
}

// +0x04, appendGeometry, the LOD-vertex packer: OverGrowthCellRender.cpp.

// +0x08, leaf then trunk, each through its own +0x24 (lock); no null tests.
void OverGrowthCell::lock()
{
    field<OverGrowthCellMesh*>(this, 0x2A4)->lock();
    field<OverGrowthCellMesh*>(this, 0x2A8)->lock();
}

// +0x0C, the same pair through +0x28 (unlock).
void OverGrowthCell::unlock()
{
    field<OverGrowthCellMesh*>(this, 0x2A4)->unlock();
    field<OverGrowthCellMesh*>(this, 0x2A8)->unlock();
}

// +0x10, draw, the cell's two render-queue submissions: OverGrowthCellRender.cpp.

// +0x14, leaf +0x38 first, then trunk +0x38, summed.
unsigned int OverGrowthCell::getVertexCapacity()
{
    const unsigned int leaf = field<OverGrowthCellMesh*>(this, 0x2A4)->getVertexCapacity();
    return leaf + field<OverGrowthCellMesh*>(this, 0x2A8)->getVertexCapacity();
}

// +0x18, the same through +0x3C.
unsigned int OverGrowthCell::getIndexCapacity()
{
    const unsigned int leaf = field<OverGrowthCellMesh*>(this, 0x2A4)->getIndexCapacity();
    return leaf + field<OverGrowthCellMesh*>(this, 0x2A8)->getIndexCapacity();
}

// +0x1C, PE 0x100D1880 (16 B): `push 0; push 0; add ecx, 28Ch; call sub_100CF850` - the far list's
// resize(0) (erase, the buffer is kept).
void OverGrowthCell::clearFarList()
{
    const void* const none = 0;
    vectorResize(field<vc71::vector_layout>(this, 0x28C), 4u, 0u, &none);
}

// The allocation postLoadResources makes per grid slot (0x100CD6DE..0x100CD6FB): sub_1005F320(0x2AC), then
// the constructor when the heap answered. Published for the load half.
OverGrowthCell* createOverGrowthCell()
{
    void* const memory = ::renddx9abi::engineAlloc(kCellBytes);
    return memory ? ::new (memory) OverGrowthCell() : 0;
}

namespace {


// Updates the camera-centred ring of cells, reseating changed slots and building visible cells.
// World-cell coordinates truncate toward zero; generation seeds depend on cell coordinates.

void baseUpdateCells(OverGrowth* self, IRenderView* view)
{
    if (!field<unsigned char>(self, 0x1C))
        return;
    if (!field<unsigned char>(self, 0x1E))
        self->initialize();                                                       // vslot +0x04
    const Mat4* const camera = view->getViewMatrixPtr();                          // vslot +0x64
    field<float>(self, 0x08) = camera->m[13];
    field<float>(self, 0x04) = camera->m[12];
    field<float>(self, 0x0C) = camera->m[14];
    const float inverseCellSize = 1.0f / field<float>(self, 0x14);
    const float viewDistance = field<float>(self, 0x24);
    const int cameraCellX = static_cast<int>(inverseCellSize * field<float>(self, 0x04));
    const int cameraCellZ = static_cast<int>(inverseCellSize * field<float>(self, 0x0C));
    for (int z = 0; z < field<int>(self, 0x10); ++z)
    {
        const float inverseViewDistance = 1.0f / viewDistance;                    // 0x1015EF51, per row
        for (int x = 0; x < field<int>(self, 0x10); ++x)
        {
            OverGrowthCell* const cell = self->getCell(x, z);                     // vslot +0x20
            const int n = field<int>(self, 0x10);
            const int cellX = n / 2 - (100 * n + x + cameraCellX) % n + cameraCellX;
            const int cellZ = n / 2 - (100 * n + z + cameraCellZ) % n + cameraCellZ;
            const bool moved = cellX != field<int>(cell, 0x5C) || cellZ != field<int>(cell, 0x60)
                               || field<unsigned char>(self, 0x38) != 0;
            const bool built = field<unsigned char>(cell, 0x05) != 0 && !moved;
            field<unsigned char>(cell, 0x05) = built;
            if (!built && moved)
            {
                field<int>(cell, 0x5C) = cellX;
                field<int>(cell, 0x60) = cellZ;
                field<float>(cell, 0x14) = field<float>(self, 0x14);
                field<float>(cell, 0x08) = static_cast<float>(cellX) * field<float>(self, 0x14);
                field<float>(cell, 0x0C) = 0.0f;
                field<float>(cell, 0x10) = static_cast<float>(cellZ) * field<float>(self, 0x14);
                const float half = field<float>(self, 0x14) * 0.5f;
                field<float>(cell, 0x0C) = heightmapCluster(self)->getHeightInWorldCoords(
                    half + field<float>(cell, 0x08), half + field<float>(cell, 0x10));   // vslot +0x6C
                cell->clearFarList();                                             // cell vslot +0x1C
            }
            const float half = field<float>(self, 0x14) * 0.5f;
            const float dx = (half + field<float>(cell, 0x08)) - field<float>(self, 0x04);
            const float dy = field<float>(cell, 0x0C) - field<float>(self, 0x08);
            const float dz = (half + field<float>(cell, 0x10)) - field<float>(self, 0x0C);
            field<float>(cell, 0x1C) = std::sqrt(dz * dz + dy * dy + dx * dx);
            const float reach = cellDistanceToPoint(
                cell, reinterpret_cast<const Vec3*>(&field<float>(self, 0x04))) * inverseViewDistance;
            const bool inRange = reach < 1.0f;
            const float fadeBand = field<float>(self, 0x20);
            if (fadeBand == 0.0f || !(1.0f - fadeBand < reach))
                field<float>(cell, 0x18) = 1.0f;
            else
                field<float>(cell, 0x18) = clampFloat((1.0f - reach) / fadeBand, 0.0f, 1.0f);
            bool visible = false;
            if (g_overGrowthBuildEveryCell || inRange)
            {
                if (!field<unsigned char>(cell, 0x05))
                {
                    field<unsigned int>(self, 0x18) =
                        (static_cast<unsigned int>(field<int>(cell, 0x60)) * 4711u
                         + static_cast<unsigned int>(field<int>(cell, 0x5C)) * 13u + 23u) & 0x7FFFFFFFu;
                    vc71::vector_layout instances = { 0, 0, 0, 0 };
                    self->collectCell(cell, reinterpret_cast<OverGrowthInstanceVector*>(&instances));
                    self->buildCell(cell, reinterpret_cast<OverGrowthInstanceVector*>(&instances));
                    field<unsigned char>(cell, 0x05) = 1;
                    vectorTidy(instances);
                }
                visible = field<unsigned char>(cell, 0x05) != 0;
            }
            field<unsigned char>(cell, 0x04) = visible;
        }
    }
    field<unsigned char>(self, 0x38) = 0;
}


// sub_100CA3A0, 176 B, read in full - the editor's patch pass (rebuildCells, when +0xF0). Row-major over
// the patch grid; each patch gets seed = (pz * 4711 + px * 13 + 23) & 0x7FFFFFFF (patch indices, from
// 0 - unlike the cells' signed world indices), its coverage (100C9AE0), then populatePatch (vslot +0x44).

void regeneratePatches(OverGrowth* self)
{
    int rowSeed = 23;
    for (int pz = 0; pz < field<int>(self, 0xDC); ++pz)
    {
        int seed = rowSeed;
        for (int px = 0; px < field<int>(self, 0xD8); ++px)
        {
            unsigned char* const patch = field<unsigned char*>(self, 0xE0)
                + (field<int>(self, 0xD8) * pz + px) * kPatchBytes;
            field<unsigned int>(self, 0x18) = static_cast<unsigned int>(seed) & 0x7FFFFFFFu;
            computeCoverage(self, patch);
            self->populatePatch(reinterpret_cast<OverGrowthPatch*>(patch));       // vslot +0x44
            seed += 13;
        }
        rowSeed += 4711;
    }
}

} // anonymous namespace

// OverGrowthCell.h: render reaches the base arm directly when +0xD5 is clear (0x100CA5AE).
void overGrowthBaseUpdateCells(OverGrowth* self, IRenderView* view)
{
    baseUpdateCells(self, view);
}


// Updates, culls and draws visible cells near-to-far with distance and height fade.
// Equal-distance ordering follows the host sort and may differ from the compatibility STL.
void OverGrowth::update(IRenderView* view)
{
    updateCells(view);                                                            // vslot +0x1C
    if (!field<unsigned char>(this, 0x1C))
        return;
    if (!field<unsigned char>(this, 0x1E))
        initialize();                                                             // vslot +0x04
    std::vector<OverGrowthCell*> fartherFirst;                                    // [ebp-0x40]
    fartherFirst.reserve(static_cast<unsigned int>(field<int>(this, 0x10) * field<int>(this, 0x10)));
    std::vector<OverGrowthCell*> nearerFirst;                                     // [ebp-0x30]
    nearerFirst.reserve(static_cast<unsigned int>(field<int>(this, 0x10) * field<int>(this, 0x10)));

    const Mat4* const camera = view->getViewMatrixPtr();                          // vslot +0x64
    const float cameraX = camera->m[12];
    const float cameraY = camera->m[13];
    const float cameraZ = camera->m[14];
    const float heightFade = clampFloat(
        std::fabs(cameraY - heightmapCluster(this)->getHeightInWorldCoords(cameraX, cameraZ))
            / field<float>(this, 0x2C),
        field<float>(this, 0x30), 1.0f);

    for (int z = 0; z < field<int>(this, 0x10); ++z)
    {
        for (int x = 0; x < field<int>(this, 0x10); ++x)
        {
            OverGrowthCell* const cell = getCell(x, z);                           // vslot +0x20
            if (!field<unsigned char>(cell, 0x04) || field<unsigned char>(cell, 0x06))
                continue;
            if (g_overGrowthCellDistanceCull)
            {
                const Mat4* const again = view->getViewMatrixPtr();               // vslot +0x64, again
                const float distance =
                    cellDistanceToPoint(cell, reinterpret_cast<const Vec3*>(&again->m[12]));
                if (view->getFarPlane() < distance)                               // vslot +0x3C
                    continue;
            }
            if (g_overGrowthCellFrustumCull)
            {
                if (!terrainFrustumIntersectsBox(view->getFrustumPtr(),           // vslot +0x74
                                                 &field<MeshBounds>(cell, 0x20).min.x))
                    continue;
            }
            if (objectDrawerOccludes(field<MeshBounds>(cell, 0x20)))
                continue;
            field<float>(cell, 0x18) =
                clampFloat(field<float>(cell, 0x1C) / field<float>(this, 0x34), heightFade, 1.0f)
                * field<float>(cell, 0x18);
            nearerFirst.push_back(cell);
        }
    }
    std::sort(fartherFirst.begin(), fartherFirst.end(), cellFartherFirst);
    std::sort(nearerFirst.begin(), nearerFirst.end(), cellNearerFirst);
    nearerFirst.insert(nearerFirst.end(), fartherFirst.begin(), fartherFirst.end());
    for (unsigned int i = 0; i < nearerFirst.size(); ++i)
    {
        OverGrowthCell* const cell = nearerFirst[i];
        cell->draw(view, reinterpret_cast<const Vec3*>(&field<float>(cell, 0x08)),
                   field<float>(cell, 0x18));                                     // cell vslot +0x10
    }
}

// +0x14, PE 0x100CAD70, 625 B, read in full. Every grid cell forgets itself - visible, built and has-near
// cleared, its far list freed (not just emptied), +0x29C and the remembered patch zeroed - and both
// streams get the Ra technique ("LeafOG" / "TrunkOG" through sub_10041F20 on dword_1023E408, skipFileCheck
// false, into the material block's +0x158) and the atlas texture. With +0xF0 (the editor's generate flag)
// the patches are regenerated (100CA3A0) and then walked once more by a loop whose body compiled to
// nothing (0x100CAF4A..0x100CAFD3, kept). +0x38 = 1 makes the next updateCells re-seat every cell.
void OverGrowth::rebuildCells()
{
    for (int z = 0; z < field<int>(this, 0x10); ++z)
    {
        for (int x = 0; x < field<int>(this, 0x10); ++x)
        {
            OverGrowthCell* const cell = field<OverGrowthCell**>(this, 0xE4)[field<int>(this, 0x10) * z + x];
            field<unsigned char>(cell, 0x04) = 0;
            field<unsigned char>(cell, 0x05) = 0;
            field<unsigned char>(cell, 0x64) = 0;
            vectorTidy(field<vc71::vector_layout>(cell, 0x28C));
            field<unsigned int>(cell, 0x29C) = 0;
            field<void*>(cell, 0x2A0) = 0;
            {
                const vc71::string name("LeafOG");
                dx9rend::Ra::RaShader* const shader = dx9rend::Ra::getShader(
                    static_cast<dx9rend::Ra::RaShaderTable*>(dx9rend::g_renderQueues), name, false);
                field<dx9rend::Ra::RaShader*>(field<unsigned char*>(cell, 0x2A4), 0x158) = shader;
            }
            {
                const vc71::string name("TrunkOG");
                dx9rend::Ra::RaShader* const shader = dx9rend::Ra::getShader(
                    static_cast<dx9rend::Ra::RaShaderTable*>(dx9rend::g_renderQueues), name, false);
                field<dx9rend::Ra::RaShader*>(field<unsigned char*>(cell, 0x2A8), 0x158) = shader;
            }
            assignAtlasTexture(field<unsigned char*>(cell, 0x2A4));
            assignAtlasTexture(field<unsigned char*>(cell, 0x2A8));
        }
    }
    if (field<unsigned char>(this, 0xF0))
    {
        regeneratePatches(this);
        for (int pz = 0; pz < field<int>(this, 0xDC); ++pz)
        {
            for (int px = 0; px < field<int>(this, 0xD8); ++px)
            {
                const vc71::vector_layout& instances = field<vc71::vector_layout>(
                    field<unsigned char*>(this, 0xE0) + (field<int>(this, 0xD8) * pz + px) * kPatchBytes, 0x0C);
                for (unsigned int i = 0; i < vectorSize(instances, kInstanceBytes); ++i)
                {
                    // The shipped loop's body compiled to nothing.
                }
            }
        }
    }
    field<unsigned char>(this, 0x38) = 1;
}

// +0x18, PE 0x1015E600, 163 B, read in full. The density map value under (x, z): each axis
// (world - origin(+0x64 / +0x68)) / extent(+0x5C / +0x60) * (float)(unsigned)size, truncated to int;
// outside [0, size) (signed low test, unsigned high test) answers 0, else the byte at +0x80.
unsigned int OverGrowth::sampleDensity(float x, float z)
{
    const unsigned int width = field<unsigned int>(this, 0x78);
    const int ix = static_cast<int>((x - field<float>(this, 0x64)) / field<float>(this, 0x5C)
                                    * static_cast<float>(width));
    const unsigned int height = field<unsigned int>(this, 0x7C);
    const int iz = static_cast<int>((z - field<float>(this, 0x68)) / field<float>(this, 0x60)
                                    * static_cast<float>(height));
    if (ix < 0 || static_cast<unsigned int>(ix) >= width
        || iz < 0 || static_cast<unsigned int>(iz) >= height)
        return 0;
    return field<unsigned char*>(this, 0x80)[width * iz + ix];
}

// +0x1C, PE 0x100C9450, 350 B, read in full. Unloaded (empty path, size +0x9C) does nothing. Otherwise the
// base grid pass (1015EEB0), then - enabled and with patches - the near pass: the close distance
// +0xC4 = ((view zoom * +0xF4) * +0xC8) * +0x24 (zoom x quality x closeDistPercentage x view distance), and
// for every grid slot whose patch (world cell + patch grid / 2) is on the map, updateCell (vslot +0x3C)
// when the patch's bounds reach within (+0xCC + 1) * +0xC4 of the camera or the cell still has near
// instances (+0x64). `changed` is "a different patch than last time" (+0x2A0), which is then recorded.
// The slot is read straight from the array (N * z + x), not through getCell.
void OverGrowth::updateCells(IRenderView* view)
{
    if (field<vc71::string>(this, 0x88).empty())
        return;
    baseUpdateCells(this, view);
    if (!field<unsigned char>(this, 0x1C))
        return;
    if (!field<void*>(this, 0xE0))
        return;
    field<float>(this, 0xC4) = view->getZoom() * field<float>(this, 0xF4) * field<float>(this, 0xC8)
                               * field<float>(this, 0x24);                        // vslot +0x28
    for (int z = 0; z < field<int>(this, 0x10); ++z)
    {
        for (int x = 0; x < field<int>(this, 0x10); ++x)
        {
            OverGrowthCell* const cell = field<OverGrowthCell**>(this, 0xE4)[field<int>(this, 0x10) * z + x];
            const int patchCountX = field<int>(this, 0xD8);
            const int patchCountZ = field<int>(this, 0xDC);
            const int px = patchCountX / 2 + field<int>(cell, 0x5C);
            const int pz = patchCountZ / 2 + field<int>(cell, 0x60);
            if (px < 0 || pz < 0 || px >= patchCountX || pz >= patchCountZ)
                continue;
            unsigned char* const patch = field<unsigned char*>(this, 0xE0) + (patchCountX * pz + px) * kPatchBytes;
            const float radius = (field<float>(this, 0xCC) + 1.0f) * field<float>(this, 0xC4);
            if (boundsSphereIntersects(field<MeshBounds>(patch, 0x5C),
                                       *reinterpret_cast<const Vec3*>(&field<float>(this, 0x04)), radius)
                || field<unsigned char>(cell, 0x64))
            {
                const bool changed = field<unsigned char*>(cell, 0x2A0) != patch;
                updateCell(cell, reinterpret_cast<OverGrowthInstanceVector*>(patch + 0x0C), changed);
                field<unsigned char*>(cell, 0x2A0) = patch;
            }
        }
    }
}

// +0x24, PE 0x1015EA40, 212 B, read in full (the base's slot, reused). Lock both streams (cell +0x08),
// append every instance whose geometry type has a LOD geometry (+0x1C) - cell +0x04 with (that
// geometry's +0x38 template, &transform, scale +0x44, 0, shadow +0x54) - unlock (cell +0x0C), and record
// whether the list was empty (+0x06, `setz`).
void OverGrowth::buildCell(OverGrowthCell* cell, OverGrowthInstanceVector* instanceVector)
{
    const vc71::vector_layout& instances = *reinterpret_cast<const vc71::vector_layout*>(instanceVector);
    cell->lock();                                                                 // cell vslot +0x08
    for (unsigned int i = 0; i < vectorSize(instances, kInstanceBytes); ++i)
    {
        unsigned char* const instance = vectorElement(instances, i, kInstanceBytes);
        const unsigned char* const type = vectorElement(field<vc71::vector_layout>(this, 0x4C),
                                                        field<unsigned int>(instance, 0x00), kGeometryTypeBytes);
        void* const lodGeometry = field<void*>(type, 0x1C);
        if (lodGeometry)
            cell->appendGeometry(geometryTemplate(lodGeometry),
                                 reinterpret_cast<const Mat4*>(instance + 0x04),
                                 field<float>(instance, 0x44), 0, field<float>(instance, 0x54));
    }
    cell->unlock();                                                               // cell vslot +0x0C
    field<unsigned char>(cell, 0x06) = vectorSize(instances, kInstanceBytes) == 0;
}

// +0x28, PE 0x100CBBC0, 192 B, read in full. Without patches: nothing. A cell off the patch grid gets an
// emptied list (resize(0) - the Instance() temporary 1015E930 is the by-value argument). Otherwise the
// cell takes its patch's instance list (operator=, 100D0700), bounds (sub_100AE1F0) and corner, then the
// list is walked by a loop whose body compiled to nothing (0x100CBC3E..0x100CBC62, kept).
void OverGrowth::collectCell(OverGrowthCell* cell, OverGrowthInstanceVector* instanceVector)
{
    if (!field<void*>(this, 0xE0))
        return;
    vc71::vector_layout& instances = *reinterpret_cast<vc71::vector_layout*>(instanceVector);
    const int patchCountX = field<int>(this, 0xD8);
    const int patchCountZ = field<int>(this, 0xDC);
    const int px = patchCountX / 2 + field<int>(cell, 0x5C);
    const int pz = patchCountZ / 2 + field<int>(cell, 0x60);
    if (px < 0 || pz < 0 || px >= patchCountX || pz >= patchCountZ)
    {
        unsigned char empty[kInstanceBytes];
        constructInstance(empty);
        vectorResize(instances, kInstanceBytes, 0u, empty);
        return;
    }
    unsigned char* const patch = patchAt(this, px, pz);
    vectorAssign(instances, field<vc71::vector_layout>(patch, 0x0C), kInstanceBytes);
    meshBoundsCopy(field<MeshBounds>(cell, 0x20), field<MeshBounds>(patch, 0x5C));
    field<float>(cell, 0x08) = field<float>(patch, 0x00);
    field<float>(cell, 0x0C) = field<float>(patch, 0x04);
    field<float>(cell, 0x10) = field<float>(patch, 0x08);
    for (unsigned int i = 0; i < vectorSize(instances, kInstanceBytes); ++i)
    {
        // The shipped loop's body compiled to nothing.
    }
}

// Updates the far-instance list, retaining its matching prefix and rebuilding streams when needed.
// Near instances use full meshes; distance thresholds combine template radius and type settings.
void OverGrowth::updateCell(OverGrowthCell* cell, OverGrowthInstanceVector* instanceVector, bool changed)
{
    vc71::vector_layout& farList = field<vc71::vector_layout>(cell, 0x28C);
    const vc71::vector_layout& instances = *reinterpret_cast<const vc71::vector_layout*>(instanceVector);
    bool modified = false;                                                        // [esp+0x13]
    unsigned int kept = 0;                                                        // [esp+0x1C]
    for (unsigned int i = 0; i < vectorSize(instances, kInstanceBytes); ++i)
    {
        unsigned char* const instance = vectorElement(instances, i, kInstanceBytes);
        const float dx = field<float>(this, 0x04) - field<float>(instance, 0x34);
        const float dy = field<float>(this, 0x08) - field<float>(instance, 0x38);
        const float dz = field<float>(this, 0x0C) - field<float>(instance, 0x3C);
        const unsigned char* const type = vectorElement(field<vc71::vector_layout>(this, 0x4C),
                                                        field<unsigned int>(instance, 0x00), kGeometryTypeBytes);
        world::IObjectTemplate* const objectTemplate = field<world::IObjectTemplate*>(type, 0x80);
        const float reach = objectTemplate->getCullRadiusScale() * field<float>(instance, 0x48)   // +0x70
                            + field<float>(type, 0x9C) * field<float>(this, 0xC4) * field<float>(this, 0x28);
        const bool isNear = (dx * dx + dz * dz + dy * dy) / (reach * reach) < 1.0f;
        if (!modified && farList.first && kept < vectorSize(farList, 4u))
        {
            unsigned char* const listed = field<unsigned char*>(farList.first, kept * 4u);
            if (isNear)
            {
                if (listed == instance)
                {
                    const void* const none = 0;
                    modified = true;
                    vectorResize(farList, 4u, kept, &none);
                }
            }
            else if (listed == instance)
            {
                ++kept;
            }
            else
            {
                const void* const none = 0;
                modified = true;
                vectorResize(farList, 4u, kept, &none);
                vectorPushBack(farList, 4u, &instance);
            }
            continue;
        }
        if (!isNear)
        {
            modified = true;
            vectorPushBack(farList, 4u, &instance);
        }
    }

    const bool hasNear = vectorSize(instances, kInstanceBytes) != vectorSize(farList, 4u);
    if (!modified)
        modified = hasNear != (field<unsigned char>(cell, 0x64) != 0);
    field<unsigned char>(cell, 0x64) = hasNear;
    const bool wasEmpty = field<unsigned char>(cell, 0x06) != 0;
    const bool nowEmpty = vectorSize(farList, 4u) == 0 || vectorSize(instances, kInstanceBytes) == 0;
    field<unsigned char>(cell, 0x06) = nowEmpty;
    if (!changed && (!modified || (nowEmpty && wasEmpty)))
        return;

    // the cell's patch, re-derived; its bounds copied member-wise (the inlined 100AE1F0).
    const int patchCountX = field<int>(this, 0xD8);
    const unsigned char* const patch = field<unsigned char*>(this, 0xE0)
        + ((field<int>(this, 0xDC) / 2 + field<int>(cell, 0x60)) * patchCountX
           + patchCountX / 2 + field<int>(cell, 0x5C)) * kPatchBytes;
    meshBoundsCopy(field<MeshBounds>(cell, 0x20), field<MeshBounds>(patch, 0x5C));
    cell->lock();                                                                 // cell vslot +0x08
    for (unsigned int k = 0; k < vectorSize(farList, 4u); ++k)
    {
        unsigned char* const listed = field<unsigned char*>(farList.first, k * 4u);
        const unsigned char* const type = vectorElement(field<vc71::vector_layout>(this, 0x4C),
                                                        field<unsigned int>(listed, 0x00), kGeometryTypeBytes);
        void* const lodGeometry = field<void*>(type, 0x1C);
        if (lodGeometry)
            cell->appendGeometry(geometryTemplate(lodGeometry),
                                 reinterpret_cast<const Mat4*>(listed + 0x04),
                                 field<float>(listed, 0x44), 0, field<float>(listed, 0x54));
    }
    cell->unlock();                                                               // cell vslot +0x0C
}

// +0x40, PE 0x100C9980, 341 B, read in full. One instance's transform. Draw 1: a yaw of
// count * 360/2^24 - 180 degrees (10077E30). The terrain normal at the position (cluster +0x7C) has x and z
// scaled by the type's normal scale (+0x54); draws 2 and 3 add a tilt in [-range, range] (range = type
// +0x40) to x, then to z, each as ((count * (range - (-range))) * 2^-24 + n) + (-range) - the PE's
// association. The normalised result becomes row 1, the basis is re-orthonormalised around it (100C8820)
// and the translation row takes the position.
void OverGrowth::placeInstance(OverGrowthType* typeRecord, Vec3* position, OverGrowthInstance* instanceRecord)
{
    const unsigned char* const type = reinterpret_cast<const unsigned char*>(typeRecord);
    unsigned char* const instance = reinterpret_cast<unsigned char*>(instanceRecord);
    Mat4& transform = *reinterpret_cast<Mat4*>(instance + 0x04);
    setRotationY(transform, static_cast<float>(random24(this)) * kDegreesPerCount - 180.0f);
    Vec3 normal;
    normal.x = 0.0f;
    normal.y = 0.0f;
    normal.z = 0.0f;
    heightmapCluster(this)->getNormalInWorldCoords(position->x, position->z, normal);          // vslot +0x7C
    normal.x = normal.x * field<float>(type, 0x54);
    normal.z = normal.z * field<float>(type, 0x54);
    float low = -field<float>(type, 0x40);
    normal.x = static_cast<float>(random24(this)) * (field<float>(type, 0x40) - low) * kRandomUnit
               + normal.x + low;
    low = -field<float>(type, 0x40);
    normal.z = static_cast<float>(random24(this)) * (field<float>(type, 0x40) - low) * kRandomUnit
               + normal.z + low;
    normalizeVec3(normal);
    field<float>(instance, 0x14) = normal.x;
    field<float>(instance, 0x18) = normal.y;
    field<float>(instance, 0x1C) = normal.z;
    orthonormalizeKeepingY(transform);
    field<float>(instance, 0x34) = position->x;
    field<float>(instance, 0x38) = position->y;
    field<float>(instance, 0x3C) = position->z;
}

// Generates patch instances within material-density targets and stream budgets.
// Limits attempts to five times the target and enforces spacing against generated instances.
void OverGrowth::populatePatch(OverGrowthPatch* patchRecord)
{
    unsigned char* const patch = reinterpret_cast<unsigned char*>(patchRecord);
    int targets[16];
    int nothingLeft[16];
    for (unsigned int i = 0; i < 16; ++i)
    {
        targets[i] = 0;                                                           // rep stosd, 0x100CC81A
        nothingLeft[i] = 0;                                                       // rep stosd, 0x100CC828
    }
    int total = 0;
    const vc71::vector_layout& materials = field<vc71::vector_layout>(this, 0x3C);
    for (unsigned int i = 0; i < vectorSize(materials, kMaterialBytes); ++i)
    {
        const unsigned char* const material = vectorElement(materials, i, kMaterialBytes);
        const vc71::vector_layout& types = field<vc71::vector_layout>(material, 0x20);
        float expected = 0.0f;
        for (unsigned int j = 0; j < vectorSize(types, kMaterialTypeBytes); ++j)
            expected = expected + field<float>(vectorElement(types, j, kMaterialTypeBytes), 0x3C)
                                  * field<float>(this, 0x14) * field<float>(this, 0x14) * 0.001f;
        const int value = field<int>(material, 0x1C);
        const int count = static_cast<int>(expected * field<float>(patch, 0x1C + value * 4));   // __ftol2
        total += count;
        targets[value] = count;
    }

    vc71::vector_layout& instances = field<vc71::vector_layout>(patch, 0x0C);
    unsigned int leafVertices = field<unsigned int>(this, 0x104);
    unsigned int trunkVertices = field<unsigned int>(this, 0x108);
    unsigned int leafIndices = field<unsigned int>(this, 0x10C);
    unsigned int trunkIndices = field<unsigned int>(this, 0x110);
    for (unsigned int i = 0; i < vectorSize(instances, kInstanceBytes); ++i)
    {
        const unsigned char* const existing = vectorElement(instances, i, kInstanceBytes);
        if (field<unsigned char>(existing, 0x58))
            continue;
        const unsigned char* const cost = vectorElement(field<vc71::vector_layout>(this, 0x4C),
                                                        field<unsigned int>(existing, 0x00), kGeometryTypeBytes);
        leafVertices -= field<unsigned int>(cost, 0x20);
        trunkVertices -= field<unsigned int>(cost, 0x24);
        leafIndices -= field<unsigned int>(cost, 0x28);
        trunkIndices -= field<unsigned int>(cost, 0x2C);
    }

    bool keepGoing = true;
    const unsigned int attempts = static_cast<unsigned int>(total * 5);
    for (unsigned int attempt = 0; attempt < attempts && keepGoing; ++attempt)
    {
        Vec3 position;
        position.x = 0.0f;
        position.y = 0.0f;
        position.z = 0.0f;
        const unsigned int value = pickPosition(this, patch, &position);
        float draw = static_cast<float>(random24(this)) * kRandomUnit;
        unsigned char* const material = findMaterial(this, static_cast<int>(value));
        if (!material || targets[field<int>(material, 0x1C)] == 0)
            continue;
        const vc71::vector_layout& types = field<vc71::vector_layout>(material, 0x20);
        for (unsigned int j = 0; j < vectorSize(types, kMaterialTypeBytes); ++j)
        {
            unsigned char* const type = vectorElement(types, j, kMaterialTypeBytes);
            if (!(draw < field<float>(type, 0x38)))
            {
                draw = draw - field<float>(type, 0x38);
                continue;
            }
            if (field<int>(type, 0x58) < 0)
            {
                // sub_1015F450(&type->+0x1C): the geometry table's find-or-add (OverGrowth.cpp). A
                // -1 (a missing template) is stored and the next type is tried with the draw left as it
                // is (0x100CCAA5..0x100CCAAA -> 0x100CCC33).
                field<int>(type, 0x58) = findOrAddGeometry(field<vc71::string>(type, 0x1C));
                if (field<int>(type, 0x58) < 0)
                    continue;
            }
            const unsigned char* const geometryType = vectorElement(
                field<vc71::vector_layout>(this, 0x4C), field<unsigned int>(type, 0x58), kGeometryTypeBytes);
            if (field<unsigned int>(geometryType, 0x20) > leafVertices
                || field<unsigned int>(geometryType, 0x28) > leafIndices
                || field<unsigned int>(geometryType, 0x24) > trunkVertices
                || field<unsigned int>(geometryType, 0x2C) > trunkIndices)
            {
                keepGoing = false;
                break;
            }
            unsigned char instance[kInstanceBytes];
            constructInstance(instance);
            field<int>(instance, 0x00) = field<int>(type, 0x58);
            field<float>(instance, 0x54) = sampleShadow(this, position.x, position.z);
            field<float>(instance, 0x44) = 1.0f;
            void* const fullGeometry = field<void*>(geometryType, 0x6C);
            field<float>(instance, 0x48) = geometryBoundingRadius(fullGeometry);
            field<float>(instance, 0x4C) = geometryBoundingRadius(fullGeometry) * field<float>(type, 0x4C);
            field<float>(instance, 0x50) = geometryBoundingRadius(fullGeometry) * field<float>(type, 0x50);
            field<unsigned char>(instance, 0x58) = 1;
            placeInstance(reinterpret_cast<OverGrowthType*>(type), &position,
                          reinterpret_cast<OverGrowthInstance*>(instance));      // vslot +0x40
            {
                const unsigned char* const placedType = vectorElement(field<vc71::vector_layout>(this, 0x4C),
                    field<unsigned int>(instance, 0x00), kGeometryTypeBytes);
                const Vec3 anchor = field<world::IObjectTemplate*>(placedType, 0x80)->getAnchorPos();   // +0xAC
                field<float>(instance, 0x38) = field<float>(instance, 0x38) - anchor.y;
            }
            bool spaced = true;
            for (unsigned int k = 0; k < vectorSize(instances, kInstanceBytes); ++k)
            {
                const unsigned char* const other = vectorElement(instances, k, kInstanceBytes);
                if (!field<unsigned char>(other, 0x58))
                    continue;
                const float dx = position.x - field<float>(other, 0x34);
                const float dz = position.z - field<float>(other, 0x3C);
                const float distanceSq = dz * dz + dx * dx;
                const float spacing = field<int>(other, 0x00) == field<int>(instance, 0x00)
                    ? field<float>(instance, 0x4C) + field<float>(other, 0x4C)
                    : field<float>(instance, 0x50) + field<float>(other, 0x50);
                if (distanceSq < spacing * spacing)
                {
                    spaced = false;
                    break;
                }
            }
            if (!spaced)
                break;
            leafIndices -= field<unsigned int>(geometryType, 0x28);
            trunkVertices -= field<unsigned int>(geometryType, 0x24);
            leafVertices -= field<unsigned int>(geometryType, 0x20);
            trunkIndices -= field<unsigned int>(geometryType, 0x2C);
            --targets[field<int>(material, 0x1C)];
            vectorPushBack(instances, kInstanceBytes, instance);
            OverGrowthBoxCorners corners;
            constructBoxCorners(corners, *geometryLocalBounds(fullGeometry),
                                *reinterpret_cast<const Mat4*>(instance + 0x04));
            boundsExpandByCorners(field<MeshBounds>(patch, 0x5C), corners);
            if (std::memcmp(targets, nothingLeft, sizeof(targets)) == 0)         // repe cmpsd, 0x100CCCF3
                keepGoing = false;
            break;
        }
    }
}

} } }
#endif
