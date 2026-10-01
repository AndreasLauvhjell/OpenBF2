// Vegetation resource loading, atlas adoption and editor export.
// preLoad loads only the atlas; resource pre/post-load performs level initialization.
#ifdef BF2_IMAGE_RENDDX9
#include "OverGrowth.h"
#include "OverGrowthCell.h"                 // createOverGrowthCell (100D0C90)
#include "../MeshTemplate.h"                  // MeshBounds, meshBoundsExpand/Finalize, IGeometryTemplate
#include "../StaticMeshTemplate.h"            // StaticMeshTemplate::m_materials, StaticMeshMaterial, ShaderMaterial
#include "../GeometryTemplateManager.h"       // IGeometryTemplateManager, g_GeometryTemplateManager
#include "../../Core/Ids.h"                   // IID_IGeometry (dword_1021AE58 = 0x492FE0FE)
#include "../../Math/Mat4.h"
// XpLightManager.cpp's pattern: in the retail-ABI flavour HfeAbi.h already supplies IBase and the
// class-registration records, so the engine headers' own copies are suppressed around their include.
#if !BF2_PLUGIN_ABI_NATIVE
#define CORE_IBASE_H
#define CORE_CLASSMANAGER_H
#endif
#include "../../Game/Objects/IObject.h"
#include "../../Game/Objects/IObjectTemplate.h"
#include "../../Game/Objects/ObjectLayoutAccess.h"
#include "../../Physics/HeightmapCluster.h"
#include "../../Physics/Heightmap.h"
#include "../../Scene/Object/ObjectManager.h"
#include "../../Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#if !BF2_PLUGIN_ABI_NATIVE
#undef CORE_CLASSMANAGER_H
#undef CORE_IBASE_H
#endif
#include "../../RendDX9/RendDX9Base.h"        // pathToLower, pathStripExtension, readStringSettingDefault,
                                              // textureManagerSwapDefaultMipClass
#include "../../RendDX9/Texture.h"            // EngineTexture, textureAddRef, textureReleaseRef
#include "../../RendDX9/TextureAtlas/TextureAtlas.h"
#include "../../../RendDX9Abi/BfMemory.h"
#include "../../../RendDX9Abi/EngineFile.h"   // engineFileExists (FileManager +0x1C)
#include "../../../RendDX9Abi/Frontier.h"     // logf
#include "../../../RendDX9Abi/HostFileAbi.h"
#include "../../../RendDX9Abi/PluginAbi.h"
#include "../../../RendDX9Abi/PluginEntry.h"  // classManager, fileManager
#include "../../../RendDX9Abi/Vc71String.h"
#include "../../../RendDX9Abi/Vc71Vector.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <map>
#include <new>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace dice { namespace hfe {
// PluginEntry.cpp - two more cached host singletons (defined there beside the boot set; BF2Render.cpp
// declares objectManager the same way).
extern IBase* objectManager;
extern IBase* objectTemplateManager;
} }

namespace dice { namespace hfe { namespace geom {
namespace {


// House helpers - the same two OverGrowth.cpp keeps in its anonymous namespace.

template<class T> T& field(void* object, unsigned int offset)
{ return *reinterpret_cast<T*>(static_cast<unsigned char*>(object) + offset); }
template<class T> const T& field(const void* object, unsigned int offset)
{ return *reinterpret_cast<const T*>(static_cast<const unsigned char*>(object) + offset); }


// VC7.1 std::string operations the bodies use, over the tree's 28-byte vc71::string.

// std::operator+(const string&, const char*) - VC7.1 inlines it as `copy the left, then +=`.
vc71::string concat(const vc71::string& left, const char* right)
{
    vc71::string result(left);
    result += right;
    return result;
}


// std::string::find(const char*, size_type off, size_type count) (MSVCP71 import at 100CD108 /
// 100CD22B, called with off 0 and count 5). The library algorithm: scan for the first character,
// then compare the whole needle; npos when absent.
const unsigned int kNpos = 0xFFFFFFFFu;
unsigned int find(const vc71::string& haystack, const char* needle, unsigned int count)
{
    const unsigned int size = haystack.size();
    if (count == 0)
        return 0;
    if (count > size)
        return kNpos;
    const char* const text = haystack.c_str();
    for (unsigned int i = 0; i + count <= size; ++i)
        if (text[i] == needle[0] && std::memcmp(text + i, needle, count) == 0)
            return i;
    return kNpos;
}

// std::less<std::string> - compare(): the common prefix by memcmp, then the lengths.
struct Vc71StringLess
{
    bool operator()(const vc71::string& left, const vc71::string& right) const
    {
        const unsigned int n = left.size() < right.size() ? left.size() : right.size();
        const int c = std::memcmp(left.c_str(), right.c_str(), n);
        return c != 0 ? c < 0 : left.size() < right.size();
    }
};
typedef std::set<vc71::string, Vc71StringLess> TextureNameSet;   // set<std::string>, 0x2C-byte nodes

// std::operator<<(ostream&, const string&) (sub_10001C10, 509 B, the image's own instantiation).
// Every caller here streams with width 0, where the inserter's padding arms are dead, so the
// characters are written as they are.
std::ostream& operator<<(std::ostream& out, const vc71::string& value)
{
    return out.write(value.c_str(), static_cast<std::streamsize>(value.size()));
}

// sub_10074E80 (55 B, read in full): copy, sub_10074D10 (pathToLower) the copy, return it.
vc71::string toLower(const vc71::string& value)
{
    vc71::string lowered(value);
    dx9rend::pathToLower(lowered);
    return lowered;
}

// sub_1000ED40 (34 B, __stdcall, read in full): fcomi/fcmovnb against the high bound first
// (moves when value >= high, ordered), then fcomi/fcmovb against the LOW bound (moves when below
// OR unordered). So a NaN comes back as `low`.
// SkyDome.cpp and SkinnedMesh.cpp carry file-local transcriptions of the same body that differ
// from this reading and from each other (NaN pass-through; `<=` at the low bound).
float clampFloat(float value, float low, float high)
{
    float result = value;
    if (result >= high)
        result = high;
    if (!(result >= low))
        result = low;
    return result;
}


// Host-side seams (RW-266 rule). Typed in both flavours where the tree's MSVC table matches the
// retail PE slot and no STL crosses; otherwise typed native / raw retail slot in the #else.

IGeometryTemplateManager* geometryTemplateManager()
{
    return static_cast<IGeometryTemplateManager*>(static_cast<void*>(g_GeometryTemplateManager));
}

// FileManager +0x2C readFile / +0x30 writeFile - HostFileAbi.h types both flavours.
bool hostReadFile(const vc71::string& path, void* buffer, unsigned int size, unsigned int& read)
{
    return ::dice::hfe::fileManager->readFile(renddx9abi::toHostString(path), buffer, size, read);
}

bool hostWriteFile(const vc71::string& path, const void* data, unsigned int size, bool flag)
{
    return ::dice::hfe::fileManager->writeFile(renddx9abi::toHostString(path), data, size, flag);
}

// ObjectTemplateManager PE +0x34. MSVC groups the five getTemplate overloads in reverse declaration
// order at the first one's position, so +0x34 is getTemplate(const std::string&) - the one-argument
// call 1015EBB8..1015EBC1 makes. A std::string crosses: typed native, raw retail slot otherwise.

// ObjectManager +0x34 getAllObjectsFromGrid - a std::map<unsigned int, IObject*> by reference.
// Typed native; the retail flavour walks the VC7.1 map with the inlined _Tree::iterator::_Inc the
// callers carry (0x100CBF4C..0x100CBF98, 0x100CC359..0x100CC397): _Isnil at node +0x15.
#if BF2_PLUGIN_ABI_NATIVE
typedef std::map<unsigned int, world::IObject*> HostObjectMap;
#else
struct HostObjectNode
{
    HostObjectNode* left;      // +0x00
    HostObjectNode* parent;    // +0x04
    HostObjectNode* right;     // +0x08
    unsigned int    key;       // +0x0C
    world::IObject* object;    // +0x10  it->second, the only field the walks read
    unsigned char   color;     // +0x14
    unsigned char   isNil;     // +0x15
};
struct HostObjectMap
{
    void*           comp;      // +0x00
    HostObjectNode* head;      // +0x04  _Myhead (re-read by every loop test)
    unsigned int    size;      // +0x08
};
#endif

HostObjectMap& allObjectsFromGrid()
{
#if BF2_PLUGIN_ABI_NATIVE
    return reinterpret_cast<world::ObjectManager*>(::dice::hfe::objectManager)->getAllObjectsFromGrid();
#else
    typedef HostObjectMap* (__thiscall* GetAllFn)(void*);
    void* const manager = ::dice::hfe::objectManager;
    return *reinterpret_cast<GetAllFn>((*static_cast<void***>(manager))[0x34 / 4])(manager);
#endif
}

class HostObjectWalk
{
public:
#if BF2_PLUGIN_ABI_NATIVE
    HostObjectWalk() : m_map(allObjectsFromGrid()), m_it(m_map.begin()) {}
    bool done() const { return m_it == m_map.end(); }
    world::IObject* object() const { return m_it->second; }
    void next() { ++m_it; }
private:
    HostObjectMap&          m_map;
    HostObjectMap::iterator m_it;
#else
    HostObjectWalk() : m_map(allObjectsFromGrid()), m_node(m_map.head->left) {}
    bool done() const { return m_node == m_map.head; }
    world::IObject* object() const { return m_node->object; }
    void next()
    {
        if (m_node->isNil)
            return;
        HostObjectNode* right = m_node->right;
        if (!right->isNil)
        {
            while (!right->left->isNil)
                right = right->left;
            m_node = right;
            return;
        }
        HostObjectNode* parent = m_node->parent;
        while (!parent->isNil && m_node == parent->right)
        {
            m_node = parent;
            parent = parent->parent;
        }
        m_node = parent;
    }
private:
    HostObjectMap&  m_map;
    HostObjectNode* m_node;
#endif
    HostObjectWalk(const HostObjectWalk&);
    HostObjectWalk& operator=(const HostObjectWalk&);
};


// RendDX9's own geometry instances (StaticMesh & co.): not modelled as a C++ interface in this tree,
// so each slot is reached by offset through the instance's retail-ordered table - XpLightManager.cpp
// geometrySlot / NametagManager.cpp geometryBoundingRadius precedent. Both flavours lay these out
// identically (the objects are this image's).

inline void* geometrySlot(void* geometry, unsigned int offset)
{
    return (*reinterpret_cast<void***>(geometry))[offset / sizeof(void*)];
}
IGeometryTemplate* geometryGetTemplate(void* geometry)                         // +0x38
{
    typedef void* (__thiscall* Fn)(void*);
    return static_cast<IGeometryTemplate*>(reinterpret_cast<Fn>(geometrySlot(geometry, 0x38))(geometry));
}
MeshBounds* geometryLocalBounds(void* geometry)                                // +0x40 (no argument)
{
    typedef void* (__thiscall* Fn)(void*);
    return static_cast<MeshBounds*>(reinterpret_cast<Fn>(geometrySlot(geometry, 0x40))(geometry));
}
float geometryBoundingRadius(void* geometry)                                   // +0x44
{
    typedef float (__thiscall* Fn)(void*);
    return reinterpret_cast<Fn>(geometrySlot(geometry, 0x44))(geometry);
}

// The first geom's first lod's material vector: m_materials.first -> [0].first -> the lod-0
// vector<StaticMeshMaterial> (516-byte elements) - 100CD0A4..100CD0B8 / 100CC3BE..100CC3C7. The
// retail code reads the template this way with NO type check (vegetation is StaticMesh).
const vc71::vector_layout& firstLodMaterials(IGeometryTemplate* geometryTemplate)
{
    StaticMeshTemplate* const t = static_cast<StaticMeshTemplate*>(geometryTemplate);
    const vc71::vector_layout* const geoms = reinterpret_cast<const vc71::vector_layout*>(t->m_materials.first);
    return *reinterpret_cast<const vc71::vector_layout*>(geoms->first);
}
// A material's first texture reference, slot 1 of the ShaderMaterial (+0x28 -> +0x2C), and its name
// (+0x58) - 100CD0E3..100CD0EE / 100CC3FC..100CC405. No null tests, as shipped.
const vc71::string& materialTextureName(const StaticMeshMaterial* material)
{
    return static_cast<const dx9rend::EngineTexture*>(material->m_shaderMaterial->textures[1])->name;
}


// sub_100CE120 / the 96-byte eight-corner box.
// A copy of XpLightManager.cpp's file-local ShadowBoxCorners (anonymous namespace there); 0x101C6750,
// (all eight corners, including corner 0's different y/x association) and 0x101C6E10
// were re-read for this draft and agree with it.

struct BoxCorners
{
    Vec3 corner[8];

    BoxCorners(const MeshBounds& bounds, const Mat4& transform)        // zero, set, move
    {
        for (unsigned int i = 0; i < 8; ++i)
            corner[i].x = corner[i].y = corner[i].z = 0.0f;
        for (unsigned int i = 0; i < 8; ++i)
        {
            corner[i].x = (i & 4u) ? bounds.max.x : bounds.min.x;
            corner[i].y = (i & 2u) ? bounds.max.y : bounds.min.y;
            corner[i].z = (i & 1u) ? bounds.max.z : bounds.min.z;
        }
        const float* const m = transform.m;
        for (unsigned int i = 0; i < 8; ++i)
        {
            const float x = corner[i].x;
            const float y = corner[i].y;
            const float z = corner[i].z;
            const float tz = z * m[10] + x * m[2] + y * m[6] + m[14];
            const float ty = i == 0 ? x * m[1] + y * m[5] + z * m[9] + m[13]
                                    : x * m[1] + z * m[9] + y * m[5] + m[13];
            const float tx = i == 0 ? z * m[8] + x * m[0] + y * m[4] + m[12]
                                    : y * m[4] + z * m[8] + x * m[0] + m[12];
            corner[i].x = tx;
            corner[i].y = ty;
            corner[i].z = tz;
        }
    }

    Vec3 maximum() const
    {
        Vec3 result = corner[0];
        for (unsigned int i = 1; i < 8; ++i)
        {
            if (result.x < corner[i].x) result.x = corner[i].x;
            if (result.y < corner[i].y) result.y = corner[i].y;
            if (result.z < corner[i].z) result.z = corner[i].z;
        }
        return result;
    }

    Vec3 minimum() const
    {
        Vec3 result = corner[0];
        for (unsigned int i = 1; i < 8; ++i)
        {
            if (result.x > corner[i].x) result.x = corner[i].x;
            if (result.y > corner[i].y) result.y = corner[i].y;
            if (result.z > corner[i].z) result.z = corner[i].z;
        }
        return result;
    }
};

// sub_101C5450 (59 B, read in full): expand the bounds by the box's minimum corner, then by its
// maximum corner - both through sub_100796A0 (meshBoundsExpand, MeshTemplate.cpp).
void meshBoundsExpandByBox(MeshBounds& bounds, const BoxCorners& box)
{
    meshBoundsExpand(bounds, box.minimum());
    meshBoundsExpand(bounds, box.maximum());
}


// THE instance - 0x60 bytes, a patch's vector entries.
//   +0x00 int geometryIndex (-1)   +0x44 float scale (1.0)       +0x54 float shadow (NOT constructed)
//   +0x04 Mat4 transform (identity) +0x48 Vec3 (0,0,0)           +0x58 bool (0)  +0x5C float (1.0)

struct Instance
{
    unsigned int words[0x60 / 4];
    unsigned char* bytes() { return reinterpret_cast<unsigned char*>(words); }
};
static_assert(sizeof(Instance) == 0x60, "100D19E0 divides by 0x60");

// sub_1015E930 (103 B, read in full). The sixteen transform floats are zeroed and the diagonal set
// to 1.0 (+0x40, +0x2C, +0x18, +0x04 - the identity), then the tail; +0x54 is left as it was.
void instanceConstruct(void* instance)
{
    field<int>(instance, 0x00) = -1;
    for (unsigned int offset = 0x04; offset <= 0x40; offset += 4)
        field<float>(instance, offset) = 0.0f;
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


// THE patch - 0x98 bytes, the countX * countZ array at OverGrowth +0xE0 (operator new[] with the
// element count in the dword before the first element).
//   +0x00 Vec3 position             +0x1C float density[16] (fraction of texels per material index)
//   +0x0C vector<Instance>          +0x5C MeshBounds bounds (60 bytes, ends at +0x98)

// sub_100CAFF0 (86 B, read in full): position and the vector zeroed, the bounds zeroed with their
// dirty byte (+0x7C) set; density[] is left unconstructed.
void patchConstruct(unsigned char* patch)
{
    field<float>(patch, 0x00) = 0.0f;
    field<float>(patch, 0x04) = 0.0f;
    field<float>(patch, 0x08) = 0.0f;
    field<void*>(patch, 0x10) = 0;
    field<void*>(patch, 0x14) = 0;
    field<void*>(patch, 0x18) = 0;
    for (unsigned int offset = 0x5C; offset <= 0x78; offset += 4)
        field<unsigned int>(patch, offset) = 0;
    field<unsigned char>(patch, 0x7C) = 1;
    for (unsigned int offset = 0x80; offset <= 0x94; offset += 4)
        field<unsigned int>(patch, offset) = 0;
}

// The allocation operator new[] (1005F340) of count * 0x98 + 4, the count
// stored first, then the non-EH vector constructor iterator sub_10001000 (42 B, read: `count - 1;
// js` skips a count <= 0, else count forward constructions). A failed allocation leaves a null.
unsigned char* newPatchArray(int count)
{
    void* const memory = ::renddx9abi::engineAlloc(static_cast<unsigned int>(count) * 0x98u + 4u);
    if (!memory)
        return 0;
    *static_cast<int*>(memory) = count;
    unsigned char* const patches = static_cast<unsigned char*>(memory) + 4;
    for (int i = 0; i < count; ++i)
        patchConstruct(patches + i * 0x98);
    return patches;
}


// Grid helpers over the two byte maps (+0x80 density = material index per texel, +0x84 shadow).

// sub_100C88F0 (226 B, retn 18h, read in full): world (x, z) -> map texel plus the fraction. Both
// scale divisions go through the primary heightmap's getScale (+0x54), called once per axis; the
// texel is floor() offset by half the map size, and a texel above the size (unsigned compare, so an
// index equal to the size passes) or below zero is replaced by 0.
void worldToGrid(OverGrowth* self, float x, float z, int* gridX, int* gridZ,
                 float* fracX, float* fracZ)
{
    x = x / field<world::IHeightmap*>(self, 0xA8)->getScale().x;
    z = z / field<world::IHeightmap*>(self, 0xA8)->getScale().z;
    const float floorX = static_cast<float>(std::floor(static_cast<double>(x)));
    const float floorZ = static_cast<float>(std::floor(static_cast<double>(z)));
    *fracX = x - floorX;
    *fracZ = z - floorZ;
    *gridX = static_cast<int>(floorX);
    *gridZ = static_cast<int>(floorZ);
    *gridX += static_cast<int>(field<unsigned int>(self, 0x78) >> 1);
    *gridZ += static_cast<int>(field<unsigned int>(self, 0x7C) >> 1);
    if (static_cast<unsigned int>(*gridX) > field<unsigned int>(self, 0x78) || *gridX < 0)
        *gridX = 0;
    if (static_cast<unsigned int>(*gridZ) > field<unsigned int>(self, 0x7C) || *gridZ < 0)
        *gridZ = 0;
}

// One texel of the shadow map as 0..1, or 0 outside the map - the bounds test inlined four times in
// 100C92F0 (a signed negative test and an unsigned upper test per axis).
float shadowTexel(OverGrowth* self, int x, int z)
{
    const unsigned int width = field<unsigned int>(self, 0x78);
    if (x < 0 || static_cast<unsigned int>(x) >= width || z < 0 ||
        static_cast<unsigned int>(z) >= field<unsigned int>(self, 0x7C))
        return 0.0f;                                                      // ds:101DDE64 (0.0)
    const unsigned char texel = field<unsigned char*>(self, 0x84)[width * z + x];
    return static_cast<float>(static_cast<int>(texel)) * 0.003921568859368563f;   // ds:101E2B98
}

// sub_100C92F0 (345 B, retn 8, read in full): the bilinear shadow-map sample at world (x, z). The four
// texels are fetched in the order (x,z) (x+1,z) (x,z+1) (x+1,z+1); the two rows are lerped along x,
// then the rows along z. Reached by addStaticObjects and by populatePatch (+0x44, 100CC800).
float sampleShadow(OverGrowth* self, float x, float z)
{
    int gridX = 0;
    int gridZ = 0;
    float fracX = 0.0f;
    float fracZ = 0.0f;
    worldToGrid(self, x, z, &gridX, &gridZ, &fracX, &fracZ);
    const float s00 = shadowTexel(self, gridX, gridZ);
    const float s10 = shadowTexel(self, gridX + 1, gridZ);
    const float s01 = shadowTexel(self, gridX, gridZ + 1);
    const float s11 = shadowTexel(self, gridX + 1, gridZ + 1);
    const float top = (s10 - s00) * fracX + s00;
    const float bottom = (s11 - s01) * fracX + s01;
    return (bottom - top) * fracZ + top;
}

// sub_100C9AE0 (495 B, retn 4, read in full): a patch's material coverage. The texel range of the
// patch's [p, p + cellSize] square (both corners through worldToGrid, the fractions discarded) is
// histogrammed over all 256 byte values; density[l] = count(l) / texel count for l < 16. The texel
// count is (gz1 - gz0 + 1) * (gx1 - gx0 + 1), converted as unsigned. Also called by 100CA3A0.
void computePatchDensity(OverGrowth* self, unsigned char* patch)
{
    const float x0 = field<float>(patch, 0x00);
    const float z0 = field<float>(patch, 0x08);
    const float x1 = field<float>(self, 0x14) + field<float>(patch, 0x00);
    const float z1 = field<float>(self, 0x14) + field<float>(patch, 0x08);
    int gridX0 = 0, gridZ0 = 0, gridX1 = 0, gridZ1 = 0;
    float fracX = 0.0f, fracZ = 0.0f;
    worldToGrid(self, x0, z0, &gridX0, &gridZ0, &fracX, &fracZ);
    worldToGrid(self, x1, z1, &gridX1, &gridZ1, &fracX, &fracZ);

    unsigned int histogram[256];
    std::memset(histogram, 0, sizeof(histogram));
    for (int gz = gridZ0; gz <= gridZ1; ++gz)
    {
        const unsigned char* const row =
            field<unsigned char*>(self, 0x80) + field<unsigned int>(self, 0x78) * gz;
        for (int gx = gridX0; gx <= gridX1; ++gx)
            ++histogram[row[gx]];
    }
    const unsigned int texels =
        static_cast<unsigned int>((gridZ1 - gridZ0 + 1) * (gridX1 - gridX0 + 1));
    const float total = static_cast<float>(texels);
    for (unsigned int l = 0; l < 16; ++l)
        field<float>(patch, 0x1C + 4 * l) = static_cast<float>(histogram[l]) / total;
}

// sub_100CA450 (296 B, read in full): zero the sixteen per-material maxima (+0x114..+0x150, the
// object's last field), then every patch (row by row) computes its coverage and raises the maxima on
// a strict, ordered greater-than.
void computePatchDensities(OverGrowth* self)
{
    for (unsigned int l = 0; l < 16; ++l)
        field<float>(self, 0x114 + 4 * l) = 0.0f;
    for (int z = 0; z < field<int>(self, 0xDC); ++z)
    {
        for (int x = 0; x < field<int>(self, 0xD8); ++x)
        {
            unsigned char* const patch = field<unsigned char*>(self, 0xE0) +
                                         (field<int>(self, 0xD8) * z + x) * 0x98;
            computePatchDensity(self, patch);
            for (unsigned int l = 0; l < 16; ++l)
                if (field<float>(patch, 0x1C + 4 * l) > field<float>(self, 0x114 + 4 * l))
                    field<float>(self, 0x114 + 4 * l) = field<float>(patch, 0x1C + 4 * l);
        }
    }
}

// The patch instance vector's push_back, the inline store when there is room, else
// _Insert_n 100D09B0 -> 100D0270 (VC7.1 growth: half again, at least one more).
void pushBackInstance(unsigned char* patch, Instance& instance)
{
    vc71::vectorRef<Instance>(&field<vc71::vector_layout>(patch, 0x0C)).push_back(instance);
}


// Converts static overgrowth objects into patch instances and detaches their geometry component.
// Requires an in-range patch index and matching _lod template; missing templates are logged.

void addStaticObjects(OverGrowth* self)
{
    for (HostObjectWalk walk; !walk.done(); walk.next())
    {
        world::IObject* const object = walk.object();
        void* const geometry = world::object_detail::LayoutAccess::geometry(object);   // [esi+38h]
        if (!object->getIsOvergrowth() || !geometry)                                     // +0x12C
            continue;

        const int patchX = static_cast<int>(
            (object->getAbsoluteTransformation().m[12] - field<float>(self, 0x6C)) /
            field<float>(self, 0x14));                                                   // +0x80
        const int patchZ = static_cast<int>(
            (object->getAbsoluteTransformation().m[14] - field<float>(self, 0x74)) /
            field<float>(self, 0x14));                                                   // +0x80
        unsigned char* const patch = field<unsigned char*>(self, 0xE0) +
                                     (patchZ * field<int>(self, 0xD8) + patchX) * 0x98;

        const vc71::string name(geometryGetTemplate(geometry)->getName());       // +0x38, then +0x18
        IGeometryTemplate* lodTemplate = 0;
        {
            const vc71::string lodName = concat(name, "_lod");
            lodTemplate = geometryTemplateManager()->getTemplate(renddx9abi::toHostString(lodName));
        }
        if (!lodTemplate)
        {
            // sub_10074300(6, __FILE__, 964, "") << ... ; sub_10074690 - dice's debug stream, kept as
            // one log line as every recovered diagnostic in this image is.
            ::renddx9abi::logf("OverGrowth.cpp:964 Growth can't find geometryTemplate: %s_lod",
                               name.c_str());
            continue;
        }

        Instance instance;
        instanceConstruct(instance.bytes());
        field<int>(instance.bytes(), 0x00) = self->findOrAddGeometry(name);
        field<Mat4>(instance.bytes(), 0x04) = object->getAbsoluteTransformation(); // +0x80
        field<float>(instance.bytes(), 0x48) =
            geometryBoundingRadius(geometry) * field<float>(instance.bytes(), 0x44);
        field<float>(instance.bytes(), 0x4C) =
            geometryBoundingRadius(geometry) * field<float>(instance.bytes(), 0x44);
        field<float>(instance.bytes(), 0x50) =
            geometryBoundingRadius(geometry) * field<float>(instance.bytes(), 0x44);
        field<unsigned char>(instance.bytes(), 0x58) = 0;
        field<float>(instance.bytes(), 0x54) = sampleShadow(
            self, field<float>(instance.bytes(), 0x34), field<float>(instance.bytes(), 0x3C));
        pushBackInstance(patch, instance);

        const BoxCorners box(*geometryLocalBounds(geometry), field<Mat4>(instance.bytes(), 0x04));
        meshBoundsExpandByBox(field<MeshBounds>(patch, 0x5C), box);
        object->setComponent(IID_IGeometry, 0);                                    // +0x54
    }
}


// sub_100CC1F0 (782 B, retn 4, read in full) - the texture files the overgrowth draws.
// The distinct geometry templates of every pair's "_lod" record, plus - only when +0xF0 is set - the
// "_lod" templates of the static overgrowth objects (here getIsOvergrowth runs before the geometry
// field is read). Every template is load()ed (+0x2C) in set (pointer) order, and each material of its
// first geom's first lod adds "<texture name, separators '/', extension cut>.dds" to `out`.
// Callers: postLoadResources (the set is discarded - the loads are the effect), save, generateAtlas.

void collectTextureNames(OverGrowth* self, TextureNameSet& out)
{
    std::set<IGeometryTemplate*> templates;
    const vc71::vector_layout& geometries = field<vc71::vector_layout>(self, 0x4C);
    for (unsigned int i = 0; i < vc71::vectorSizeIn(geometries, 0xA0); ++i)
        templates.insert(geometryGetTemplate(field<void*>(
            reinterpret_cast<unsigned char*>(geometries.first) + i * 0xA0, 0x1C)));

    if (field<unsigned char>(self, 0xF0))
    {
        for (HostObjectWalk walk; !walk.done(); walk.next())
        {
            world::IObject* const object = walk.object();
            if (!object->getIsOvergrowth())                                               // +0x12C
                continue;
            void* const geometry = world::object_detail::LayoutAccess::geometry(object); // +0x38
            if (!geometry)
                continue;
            const vc71::string name(geometryGetTemplate(geometry)->getName());
            IGeometryTemplate* lodTemplate = 0;
            {
                const vc71::string lodName = concat(name, "_lod");
                lodTemplate = geometryTemplateManager()->getTemplate(renddx9abi::toHostString(lodName));
            }
            if (lodTemplate)
                templates.insert(lodTemplate);
        }
    }

    for (std::set<IGeometryTemplate*>::const_iterator it = templates.begin(); it != templates.end(); ++it)
    {
        IGeometryTemplate* const geometryTemplate = *it;
        geometryTemplate->load();                                                          // +0x2C
        const unsigned int count = vc71::vectorSizeIn(firstLodMaterials(geometryTemplate), 0x204);
        for (unsigned int k = 0; k < count; ++k)
        {
            const StaticMeshMaterial* const material =
                reinterpret_cast<const StaticMeshMaterial*>(firstLodMaterials(geometryTemplate).first) + k;
            vc71::string stripped;
            dx9rend::pathStripExtension(stripped, materialTextureName(material));
            out.insert(concat(stripped, ".dds"));
        }
    }
}

// The atlas owner's delete: `if (p) { sub_10015930(p); operator delete(p) }`.
void deleteAtlas(dx9rend::TextureAtlas* atlas)
{
    if (atlas)
    {
        dx9rend::textureAtlasDtor(atlas);
        ::renddx9abi::engineFree(atlas);
    }
}

// operator new(0x14) + sub_10015E60, a failed allocation giving null.
dx9rend::TextureAtlas* newAtlas()
{
    void* const memory = ::renddx9abi::engineAlloc(0x14);
    return memory ? dx9rend::textureAtlasCtor(memory) : 0;
}

// The page adoption postLoadResources (100CD747..100CD799) and generateAtlas (100CCF65..100CCFB7)
// share: with an atlas, its first page (sub_10015850's distinct-page list, a local vector) becomes
// +0x100 and is addRef'd through its table's +0x08. A previous +0x100 is NOT released here (clear does
// that) - shipped.
void adoptFirstAtlasPage(OverGrowth* self)
{
    if (!field<dx9rend::TextureAtlas*>(self, 0xFC))
        return;
    std::vector<dx9rend::EngineTexture*> pages;
    dx9rend::textureAtlasCollectPages(field<dx9rend::TextureAtlas*>(self, 0xFC), pages);
    if (!pages.empty())
    {
        dx9rend::EngineTexture* const page = pages[0];
        field<dx9rend::EngineTexture*>(self, 0x100) = page;
        dx9rend::textureAddRef(page);                                                 // vslot +0x08
    }
}

// sub_100C8790 (131 B, __fastcall(ostream&, const Mat4&), read in full): "[a/b/c/d]" per row, four rows.
std::ostream& writeMatrix(std::ostream& out, const Mat4& matrix)
{
    const float* row = matrix.m;
    for (unsigned int r = 0; r < 4; ++r, row += 4)
        out << '[' << row[0] << '/' << row[1] << '/' << row[2] << '/' << row[3] << ']';
    return out;
}

// sub_100CAA50 (786 B, read in full) - "<path>OvergrowthCollision.con": per patch a "rem Patch: x, z"
// header and, for every instance with a geometry index >= 0 AND flag +0x58 set, an Object.create of
// the named record (+0x50) with its transform and isOvergrowth; then the patch and object totals.
// Returns at once when there is no patch array.
void saveCollision(OverGrowth* self)
{
    if (!field<void*>(self, 0xE0))
        return;
    std::ostringstream out;
    unsigned int objects = 0;
    for (int z = 0; z < field<int>(self, 0xDC); ++z)
    {
        for (int x = 0; x < field<int>(self, 0xD8); ++x)
        {
            unsigned char* const patch = field<unsigned char*>(self, 0xE0) +
                                         (field<int>(self, 0xD8) * z + x) * 0x98;
            out << "rem Patch: " << x << ", " << z << std::endl << std::endl;
            const vc71::vector_layout& instances = field<vc71::vector_layout>(patch, 0x0C);
            for (unsigned int k = 0; k < vc71::vectorSizeIn(instances, 0x60); ++k)
            {
                const unsigned char* const instance =
                    reinterpret_cast<unsigned char*>(instances.first) + k * 0x60;
                if (field<int>(instance, 0x00) < 0 || !field<unsigned char>(instance, 0x58))
                    continue;
                const unsigned char* const pair =
                    reinterpret_cast<unsigned char*>(field<vc71::vector_layout>(self, 0x4C).first) +
                    field<int>(instance, 0x00) * 0xA0;
                out << "Object.create " << field<vc71::string>(pair, 0x50) << std::endl;
                writeMatrix(out << "Object.absoluteTransformation ",
                            field<Mat4>(instance, 0x04)) << std::endl;
                out << "Object.isOvergrowth 1" << std::endl;
                ++objects;
                out << std::endl;
            }
        }
    }
    out << "rem Patches: " << field<int>(self, 0xD8) << "x" << field<int>(self, 0xDC) << std::endl;
    out << "rem Objects: " << objects << std::endl;

    const std::string sizeText = out.str();                                               // str() #1: size
    const std::string dataText = out.str();                                               // str() #2: data
    hostWriteFile(concat(field<vc71::string>(self, 0x88), "OvergrowthCollision.con"),
                  dataText.c_str(), static_cast<unsigned int>(sizeText.size()), true);  // FileManager +0x30
}

// sub_100C98A0 / sub_100C9910 (107 B each, read in full): the two byte maps written back whole.
// These names carry a leading backslash ("\\Overgrowth.raw") that initialize's reads do not.
void saveDensityMap(OverGrowth* self)
{
    const unsigned int bytes = field<unsigned int>(self, 0x7C) * field<unsigned int>(self, 0x78);
    hostWriteFile(concat(field<vc71::string>(self, 0x88), "\\Overgrowth.raw"),
                  field<unsigned char*>(self, 0x80), bytes, true);
}
void saveShadowMap(OverGrowth* self)
{
    const unsigned int bytes = field<unsigned int>(self, 0x7C) * field<unsigned int>(self, 0x78);
    hostWriteFile(concat(field<vc71::string>(self, 0x88), "\\OvergrowthShadowmap.raw"),
                  field<unsigned char*>(self, 0x84), bytes, true);
}

} // namespace


// Initializes maps and geometry indices from the primary heightmap, then applies view distance.
// Requires heightmap singletons; missing raw maps retain zero density/full shadow defaults.

void OverGrowth::initialize()
{
    IBase* const cluster = ::dice::hfe::classManager->getSingleton(
        renddx9abi::HostString("HeightmapCluster"));                               // +0x48
    field<world::IHeightmapCluster*>(this, 0xAC) =
        static_cast<world::IHeightmapCluster*>(static_cast<void*>(cluster));
    world::IHeightmap* const heightmap =
        static_cast<world::IHeightmapCluster*>(static_cast<void*>(cluster))->getPrimaryHeightmap();   // +0x40
    field<world::IHeightmap*>(this, 0xA8) = heightmap;
    int sizeX = 0;
    int sizeZ = 0;
    heightmap->getSize(sizeX, sizeZ);                                                 // +0x2C
    field<unsigned int>(this, 0x78) = static_cast<unsigned int>(sizeX);
    field<unsigned int>(this, 0x7C) = static_cast<unsigned int>(sizeZ);

    field<float>(this, 0x5C) = static_cast<float>(field<unsigned int>(this, 0x78)) *
                               field<world::IHeightmap*>(this, 0xA8)->getScale().x;  // +0x54
    field<float>(this, 0x60) = static_cast<float>(field<unsigned int>(this, 0x7C)) *
                               field<world::IHeightmap*>(this, 0xA8)->getScale().z;  // +0x54
    const float halfX = field<float>(this, 0x5C) * -0.5f;                             // ds:102089FC
    field<float>(this, 0x64) = halfX;
    const float halfZ = field<float>(this, 0x60) * -0.5f;
    field<float>(this, 0x68) = halfZ;
    field<float>(this, 0x70) = 0.0f;
    field<float>(this, 0x6C) = halfX;
    field<float>(this, 0x74) = halfZ;

    unsigned char* const density = static_cast<unsigned char*>(::renddx9abi::engineAlloc(
        field<unsigned int>(this, 0x7C) * field<unsigned int>(this, 0x78)));
    field<unsigned char*>(this, 0x80) = density;
    std::memset(density, 0, field<unsigned int>(this, 0x7C) * field<unsigned int>(this, 0x78));
    unsigned char* const shadow = static_cast<unsigned char*>(::renddx9abi::engineAlloc(
        field<unsigned int>(this, 0x7C) * field<unsigned int>(this, 0x78)));
    field<unsigned char*>(this, 0x84) = shadow;
    std::memset(shadow, 0xFF, field<unsigned int>(this, 0x7C) * field<unsigned int>(this, 0x78));

    unsigned int bytesRead = 0;                                                        // var_10
    const vc71::string& path = field<vc71::string>(this, 0x88);
    if (::renddx9abi::engineFileExists(concat(path, "Overgrowth.raw")))               // +0x1C
    {
        const unsigned int bytes = field<unsigned int>(this, 0x7C) * field<unsigned int>(this, 0x78);
        hostReadFile(concat(path, "Overgrowth.raw"), field<unsigned char*>(this, 0x80), bytes,
                     bytesRead);                                                        // +0x2C
    }
    if (::renddx9abi::engineFileExists(concat(path, "OvergrowthShadowmap.raw")))
    {
        const unsigned int bytes = field<unsigned int>(this, 0x7C) * field<unsigned int>(this, 0x78);
        hostReadFile(concat(path, "OvergrowthShadowmap.raw"), field<unsigned char*>(this, 0x84),
                     bytes, bytesRead);
    }

    setViewDistance(field<float>(this, 0x24));                                        // vtable +0x0C

    const vc71::vector_layout& layers = field<vc71::vector_layout>(this, 0x3C);
    for (unsigned int i = 0; i < vc71::vectorSizeIn(layers, 0x30); ++i)
    {
        unsigned char* const layer = reinterpret_cast<unsigned char*>(layers.first) + i * 0x30;
        const vc71::vector_layout& types = field<vc71::vector_layout>(layer, 0x20);
        for (unsigned int j = 0; j < vc71::vectorSizeIn(types, 0x5C); ++j)
        {
            unsigned char* const type = reinterpret_cast<unsigned char*>(types.first) + j * 0x5C;
            field<int>(type, 0x58) = findOrAddGeometry(field<vc71::string>(type, 0x1C));
        }
    }
    field<unsigned char>(this, 0x1E) = 1;
}


// Initializes resources and patch bounds at terrain height, then adopts static objects unless disabled.
// Empty paths leave state unchanged.

void OverGrowth::preLoadResources()
{
    if (field<vc71::string>(this, 0x88).empty())
        return;
    OverGrowth::initialize();

    field<unsigned char*>(this, 0xE0) = newPatchArray(field<int>(this, 0xD8) * field<int>(this, 0xDC));
    for (int z = 0; z < field<int>(this, 0xDC); ++z)
    {
        const float rowZ = static_cast<float>(z);
        for (int x = 0; x < field<int>(this, 0xD8); ++x)
        {
            unsigned char* const patch = field<unsigned char*>(this, 0xE0) +
                                         (field<int>(this, 0xD8) * z + x) * 0x98;
            const float offsetZ = rowZ * field<float>(this, 0x14);
            field<float>(patch, 0x00) = static_cast<float>(x) * field<float>(this, 0x14) +
                                        field<float>(this, 0x6C);
            field<float>(patch, 0x04) = field<float>(this, 0x70);
            field<float>(patch, 0x08) = offsetZ + field<float>(this, 0x74);

            const float half = field<float>(this, 0x14) * 0.5f;                         // ds:101DD4F0
            const float height = field<world::IHeightmapCluster*>(this, 0xAC)->getHeightInWorldCoords(
                half + field<float>(patch, 0x00), half + field<float>(patch, 0x08));    // +0x6C
            field<float>(patch, 0x04) = height;

            const float centreHalf = field<float>(this, 0x14) * 0.5f;
            const float centreX = centreHalf + field<float>(patch, 0x00);
            const float centreZ = centreHalf + field<float>(patch, 0x08);
            MeshBounds& bounds = field<MeshBounds>(patch, 0x5C);
            bounds.min.x = centreX;
            bounds.min.y = height;
            bounds.min.z = centreZ;
            bounds.dirty = 1;
            bounds.max.x = centreX;
            bounds.max.y = height;
            bounds.max.z = centreZ;
            bounds.dirty = 1;
        }
    }

    if (!field<unsigned char>(this, 0xF0))
        addStaticObjects(this);
}


// Finalizes patch densities, normalizes type weights and allocates cell buffers.
// Adopts the atlas page and rebuilds cells; an empty resource path does nothing.

void OverGrowth::postLoadResources()
{
    if (field<vc71::string>(this, 0x88).empty())
        return;

    // per geometry: the vegetation stream sizes and the lod fade (skipped while reloading)
    if (!field<unsigned char>(this, 0xB0))
    {
        for (unsigned int i = 0;
             i < vc71::vectorSizeIn(field<vc71::vector_layout>(this, 0x4C), 0xA0); ++i)
        {
            static const unsigned int kRecords[2] = { 0x50, 0x00 };   // the named record, then "_lod"
            for (unsigned int r = 0; r < 2; ++r)
            {
                unsigned char* const record =
                    reinterpret_cast<unsigned char*>(field<vc71::vector_layout>(this, 0x4C).first) +
                    i * 0xA0 + kRecords[r];
                IGeometryTemplate* const geometryTemplate =
                    geometryGetTemplate(field<void*>(record, 0x1C));                  // +0x38
                for (unsigned int k = 0;
                     k < vc71::vectorSizeIn(firstLodMaterials(geometryTemplate), 0x204); ++k)
                {
                    const StaticMeshMaterial* const material = reinterpret_cast<const StaticMeshMaterial*>(
                        firstLodMaterials(geometryTemplate).first) + k;
                    const vc71::string lowered = toLower(materialTextureName(material));
                    unsigned int tiled = 0;
                    if (find(lowered, "tile_", 5) != kNpos)
                        tiled = 1;
                    field<unsigned int>(record, 0x20 + tiled * 4) += material->m_vegetationVertexCount;
                    field<unsigned int>(record, 0x28 + tiled * 4) += material->m_vegetationIndexCount;
                }                                                   // `lowered` dies here (100CD13F)
                MeshBounds* const bounds = geometryLocalBounds(field<void*>(record, 0x1C));   // +0x40
                if (bounds->dirty)
                    meshBoundsFinalize(*bounds);
                const float scaled = bounds->radius * 0.2f;                             // ds:101DDF30
                field<float>(record, 0x4C) = clampFloat(scaled * scaled, 0.3f, 1.0f);
            }
        }
    }

    computePatchDensities(this);

    // per layer: the geometry each unit of coverage costs, and the type weights
    unsigned int layerVertices[32];                                                     // var_100
    unsigned int layerIndices[32];                                                      // var_80
    std::memset(layerVertices, 0, sizeof(layerVertices));
    std::memset(layerIndices, 0, sizeof(layerIndices));
    for (unsigned int i = 0;
         i < vc71::vectorSizeIn(field<vc71::vector_layout>(this, 0x3C), 0x30); ++i)
    {
        unsigned char* const layer =
            reinterpret_cast<unsigned char*>(field<vc71::vector_layout>(this, 0x3C).first) + i * 0x30;
        for (unsigned int j = 0; j < 2; ++j)
        {
            float densitySum = 0.0f;                                                    // ds:101DDE64
            const unsigned int slot = field<unsigned int>(layer, 0x1C) * 2 + j;
            layerVertices[slot] = 0;
            layerIndices[slot] = 0;
            const vc71::vector_layout& types = field<vc71::vector_layout>(layer, 0x20);
            for (unsigned int k = 0; k < vc71::vectorSizeIn(types, 0x5C); ++k)
            {
                unsigned char* const type = reinterpret_cast<unsigned char*>(types.first) + k * 0x5C;
                const int perCell = static_cast<int>(field<float>(this, 0x14) * field<float>(this, 0x14) *
                                                     field<float>(type, 0x3C) * 0.001f);   // ds:101DDE54
                const unsigned char* const pair =
                    reinterpret_cast<unsigned char*>(field<vc71::vector_layout>(this, 0x4C).first) +
                    field<int>(type, 0x58) * 0xA0;
                layerVertices[field<unsigned int>(layer, 0x1C) * 2 + j] +=
                    field<unsigned int>(pair, 0x20 + j * 4) * perCell;
                densitySum += field<float>(type, 0x3C);
                layerIndices[field<unsigned int>(layer, 0x1C) * 2 + j] +=
                    field<unsigned int>(pair, 0x28 + j * 4) * perCell;
            }
            for (unsigned int k = 0; k < vc71::vectorSizeIn(types, 0x5C); ++k)
            {
                unsigned char* const type = reinterpret_cast<unsigned char*>(types.first) + k * 0x5C;
                field<float>(type, 0x38) = field<float>(type, 0x3C) / densitySum;
            }
        }
    }

    // the largest per-patch vertex / index counts, per stream
    field<unsigned int>(this, 0x104) = 10;
    field<unsigned int>(this, 0x108) = 10;
    field<unsigned int>(this, 0x10C) = 10;
    field<unsigned int>(this, 0x110) = 10;
    for (int z = 0; z < field<int>(this, 0xDC); ++z)
    {
        for (int x = 0; x < field<int>(this, 0xD8); ++x)
        {
            unsigned char* const patch = field<unsigned char*>(this, 0xE0) +
                                         (field<int>(this, 0xD8) * z + x) * 0x98;
            for (unsigned int j = 0; j < 2; ++j)
            {
                unsigned int vertices = 0;
                unsigned int indices = 0;
                for (unsigned int l = 0; l < 16; ++l)
                {
                    vertices += static_cast<int>(static_cast<float>(layerVertices[l * 2 + j]) *
                                                 field<float>(patch, 0x1C + 4 * l));
                    indices += static_cast<int>(static_cast<float>(layerIndices[l * 2 + j]) *
                                                field<float>(patch, 0x1C + 4 * l));
                }
                const vc71::vector_layout& instances = field<vc71::vector_layout>(patch, 0x0C);
                for (unsigned int k = 0; k < vc71::vectorSizeIn(instances, 0x60); ++k)
                {
                    const unsigned char* const instance =
                        reinterpret_cast<unsigned char*>(instances.first) + k * 0x60;
                    if (field<unsigned char>(instance, 0x58))
                        continue;
                    const unsigned char* const pair =
                        reinterpret_cast<unsigned char*>(field<vc71::vector_layout>(this, 0x4C).first) +
                        field<int>(instance, 0x00) * 0xA0;
                    vertices += field<unsigned int>(pair, 0x20 + j * 4);
                    indices += field<unsigned int>(pair, 0x28 + j * 4);
                }
                if (vertices > field<unsigned int>(this, 0x104 + j * 4))
                    field<unsigned int>(this, 0x104 + j * 4) = vertices;
                if (indices > field<unsigned int>(this, 0x10C + j * 4))
                    field<unsigned int>(this, 0x10C + j * 4) = indices;
            }
        }
    }

    // the N x N cells
    const int gridSize = field<int>(this, 0x10);
    field<OverGrowthCell**>(this, 0xE4) = static_cast<OverGrowthCell**>(
        ::renddx9abi::engineAlloc(static_cast<unsigned int>(gridSize * gridSize * 4)));
    for (int z = 0; z < field<int>(this, 0x10); ++z)
    {
        for (int x = 0; x < field<int>(this, 0x10); ++x)
        {
            OverGrowthCell** const slot =
                field<OverGrowthCell**>(this, 0xE4) + (field<int>(this, 0x10) * z + x);
            *slot = createOverGrowthCell();
        }
    }

    // the atlas, the fade state, and the first cell build
    {
        TextureNameSet names;
        collectTextureNames(this, names);
        adoptFirstAtlasPage(this);
        field<float>(this, 0x20) = 0.0f;
        field<float>(this, 0x30) = 1.0f;
        field<unsigned char>(this, 0xA4) = 1;
        rebuildCells();                                                                 // vtable +0x14
    }
}


// sub_100C9CD0 (359 B, read in full) - resolve still-unresolved type geometry indices (index < 0,
// signed), then create the atlas owner and load "<path>OvergrowthAtlas.tai" with the texture manager's
// default mip class forced to 9 around the load; a failed load deletes the owner again. A previous
// owner at +0xFC is NOT deleted here - shipped (reload clears first; the console's direct call does
// not). Callers: reload and the console command at 100C5538.

void OverGrowth::preLoad()
{
    OverGrowth* const self = this;
    const vc71::vector_layout& layers = field<vc71::vector_layout>(self, 0x3C);
    for (unsigned int i = 0; i < vc71::vectorSizeIn(layers, 0x30); ++i)
    {
        unsigned char* const layer = reinterpret_cast<unsigned char*>(layers.first) + i * 0x30;
        const vc71::vector_layout& types = field<vc71::vector_layout>(layer, 0x20);
        for (unsigned int j = 0; j < vc71::vectorSizeIn(types, 0x5C); ++j)
        {
            unsigned char* const type = reinterpret_cast<unsigned char*>(types.first) + j * 0x5C;
            if (field<int>(type, 0x58) < 0)
                field<int>(type, 0x58) = self->findOrAddGeometry(field<vc71::string>(type, 0x1C));
        }
    }

    field<dx9rend::TextureAtlas*>(self, 0xFC) = newAtlas();
    const int previousMipClass = dx9rend::textureManagerSwapDefaultMipClass(9);
    if (!dx9rend::textureAtlasLoadTAI(field<dx9rend::TextureAtlas*>(self, 0xFC),
                                      concat(field<vc71::string>(self, 0x88), "OvergrowthAtlas.tai")))
    {
        deleteAtlas(field<dx9rend::TextureAtlas*>(self, 0xFC));
        field<dx9rend::TextureAtlas*>(self, 0xFC) = 0;
    }
    dx9rend::textureManagerSwapDefaultMipClass(previousMipClass);
}


// Regenerates the editor atlas using Editor_LevelSavePath, falling back to GSModDirectory.
// Reloads its first page with mip class 9. The previous page is overwritten without release.

void OverGrowth::generateAtlas()
{
    OverGrowth* const self = this;
    TextureNameSet names;
    collectTextureNames(self, names);
    std::list<vc71::string> files(names.begin(), names.end());

    vc71::string saveDirectory;
    dx9rend::readStringSettingDefault("Editor_LevelSavePath", "", saveDirectory);          // +0x40
    if (saveDirectory.size() == 0)
        dx9rend::readStringSettingDefault("GSModDirectory", "", saveDirectory);            // +0x40

    deleteAtlas(field<dx9rend::TextureAtlas*>(self, 0xFC));
    field<dx9rend::TextureAtlas*>(self, 0xFC) = newAtlas();
    const int previousMipClass = dx9rend::textureManagerSwapDefaultMipClass(9);
    dx9rend::textureAtlasGenerate(field<dx9rend::TextureAtlas*>(self, 0xFC), saveDirectory,
                                  concat(field<vc71::string>(self, 0x88), "OvergrowthAtlas"),
                                  saveDirectory, files, true);
    if (!dx9rend::textureAtlasLoadTAI(field<dx9rend::TextureAtlas*>(self, 0xFC),
                                      concat(field<vc71::string>(self, 0x88), "OvergrowthAtlas.tai")))
    {
        deleteAtlas(field<dx9rend::TextureAtlas*>(self, 0xFC));
        field<dx9rend::TextureAtlas*>(self, 0xFC) = 0;
    }
    adoptFirstAtlasPage(self);
    dx9rend::textureManagerSwapDefaultMipClass(previousMipClass);
}


// sub_100CD7E0 (1082 B, read in full) - the editor save: regenerates "<path>Overgrowth.con" (settings,
// then per layer its material line and per type its six properties, then path and preLoad), writes
// the collision script and both byte maps, and - unless reloading - rebuilds the atlas when the
// overgrowth draws any texture. Callers: reload, the console command at 100C5E09.

void OverGrowth::save()
{
    OverGrowth* const self = this;
    std::ostringstream out;
    out << "Overgrowth.viewDistance " << field<float>(self, 0x24) << std::endl;
    out << "Overgrowth.viewDistanceScale " << field<float>(self, 0x28) << std::endl;
    out << "Overgrowth.closeDistPercentage " << field<float>(self, 0xC8) << std::endl;
    out << std::endl;
    const vc71::vector_layout& layers = field<vc71::vector_layout>(self, 0x3C);
    for (unsigned char* layer = reinterpret_cast<unsigned char*>(layers.first);
         layer != reinterpret_cast<unsigned char*>(layers.last); layer += 0x30)
    {
        out << "Overgrowth.addMaterial " << field<vc71::string>(layer, 0x00) << " "
            << field<unsigned int>(layer, 0x1C) << std::endl;
        out << "Overgrowth.setActiveMaterial " << field<vc71::string>(layer, 0x00) << std::endl;
        const vc71::vector_layout& types = field<vc71::vector_layout>(layer, 0x20);
        for (unsigned char* type = reinterpret_cast<unsigned char*>(types.first);
             type != reinterpret_cast<unsigned char*>(types.last); type += 0x5C)
        {
            out << "Overgrowth.addType " << field<vc71::string>(type, 0x00) << std::endl;
            out << "Overgrowth.setActiveType " << field<vc71::string>(type, 0x00) << std::endl;
            out << "OvergrowthType.geometry " << field<vc71::string>(type, 0x1C) << std::endl;
            out << "OvergrowthType.density " << field<float>(type, 0x3C) << std::endl;
            out << "OvergrowthType.normalScale " << field<float>(type, 0x54) << std::endl;
            out << "OvergrowthType.rotationScale " << field<float>(type, 0x40) << std::endl;
            out << "OvergrowthType.minRadiusToSame " << field<float>(type, 0x4C) << std::endl;
            out << "OvergrowthType.minRadiusToOthers " << field<float>(type, 0x50) << std::endl;
        }
        out << std::endl;
    }
    out << "Overgrowth.path " << field<vc71::string>(self, 0x88) << std::endl;
    out << "Overgrowth.preLoad" << std::endl;
    {
        const std::string sizeText = out.str();                                           // str() #1
        const std::string dataText = out.str();                                           // str() #2
        hostWriteFile(concat(field<vc71::string>(self, 0x88), "Overgrowth.con"),
                      dataText.c_str(), static_cast<unsigned int>(sizeText.size()), true);
    }
    saveCollision(self);
    saveDensityMap(self);
    saveShadowMap(self);
    if (!field<unsigned char>(self, 0xB0))
    {
        TextureNameSet names;
        collectTextureNames(self, names);
        if (names.size() > 0)
            self->generateAtlas();
    }
}

} } }
#endif