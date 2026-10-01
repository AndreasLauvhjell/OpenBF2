// Owns overgrowth geometry, resources, settings, and instance tables.
#ifdef BF2_IMAGE_RENDDX9
#include "OverGrowth.h"
#include "OverGrowthCell.h"
#include "../MeshTemplate.h"                  // IGeometryTemplate (load, createInstance)
#include "../GeometryTemplateManager.h"       // IGeometryTemplateManager, g_GeometryTemplateManager
// XpLightManager.cpp's pattern: in the retail-ABI flavour HfeAbi.h already supplies IBase and the
// class-registration records, so the engine headers' own copies are suppressed around their include.
#if !BF2_PLUGIN_ABI_NATIVE
#define CORE_IBASE_H
#define CORE_CLASSMANAGER_H
#endif
#include "../../Game/Objects/IObjectTemplate.h"
#include "../../Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../../Physics/HeightmapCluster.h"
#include "../../Physics/Heightmap.h"
#if !BF2_PLUGIN_ABI_NATIVE
#undef CORE_CLASSMANAGER_H
#undef CORE_IBASE_H
#endif
#include "../ITerrainEditable.h"              // the painters' overlays (IID 820005)
#include "../Terrain.h"                       // the "Terrain" singleton's patch size / count
#include "../../RendDX9/Texture.h"            // EngineTexture, textureReleaseRef (10014E20)
#include "../../RendDX9/TextureAtlas/TextureAtlas.h"
#include "../../../RendDX9Abi/BfMemory.h"
#include "../../../RendDX9Abi/HfeAbi.h"
#include "../../../RendDX9Abi/PluginAbi.h"
#include "../../../RendDX9Abi/PluginEntry.h"  // classManager
#include "../../../RendDX9Abi/Vc71String.h"
#include "../../../RendDX9Abi/Vc71Vector.h"
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <new>
#include <set>
#include <string>

namespace dice { namespace hfe {
// PluginEntry.cpp - the cached host ObjectTemplateManager (0x10244ADC), declared as BF2Render.cpp
// declares its siblings.
extern IBase* objectTemplateManager;
} }

namespace dice { namespace hfe { namespace geom {
namespace {
template<class T> T& field(void* object, unsigned int offset)
{ return *reinterpret_cast<T*>(static_cast<unsigned char*>(object) + offset); }
template<class T> const T& field(const void* object, unsigned int offset)
{ return *reinterpret_cast<const T*>(static_cast<const unsigned char*>(object) + offset); }

// The two owner tables and the two "active" cursors the console drives.
//   +0x3C  VC7.1 vector<OverGrowthMaterial>   (first +0x40, last +0x44, end +0x48)
//   +0x4C  VC7.1 vector of 0xA0 geometry records (first +0x50, last +0x54, end +0x58)
//   +0xE8  OverGrowthMaterial*  the active material   (setActiveMaterial; addMaterial nulls it)
//   +0xEC  OverGrowthType*      the active type        (setActiveType; OvergrowthType.* act on it)
OverGrowthVector<OverGrowthMaterial>& materialTable(void* self)
{ return field<OverGrowthVector<OverGrowthMaterial> >(self, 0x3C); }
const OverGrowthVector<OverGrowthMaterial>& materialTable(const void* self)
{ return field<OverGrowthVector<OverGrowthMaterial> >(self, 0x3C); }
OverGrowthMaterial*& activeMaterial(void* self) { return field<OverGrowthMaterial*>(self, 0xE8); }
OverGrowthMaterial* activeMaterial(const void* self) { return field<OverGrowthMaterial*>(self, 0xE8); }
OverGrowthType*& activeType(void* self) { return field<OverGrowthType*>(self, 0xEC); }
OverGrowthType* activeType(const void* self) { return field<OverGrowthType*>(self, 0xEC); }

// std::operator==(const string&, const string&) from MSVCP71 - compare(): same length and the
// same bytes (memcmp over the length).
bool sameString(const vc71::string& a, const vc71::string& b)
{
    return a.size() == b.size() && std::memcmp(a.c_str(), b.c_str(), a.size()) == 0;
}

// sub_10075080 (fastcall, 63 B) - std::string equality through _stricmp on the two c_str()s.
bool equalsNoCase(const vc71::string& a, const vc71::string& b)
{
    return ::_stricmp(a.c_str(), b.c_str()) == 0;
}

// std::operator+(const string&, const char*) - VC7.1 inlines it as `copy the left, then +=`.
vc71::string concat(const vc71::string& left, const char* right)
{
    vc71::string result(left);
    result += right;
    return result;
}

// sub_10074AB0 - the image's printf-into-a-std::string helper, reproduced TU-locally as
// RaShader.cpp, QuadPatch.cpp and NametagManager.cpp already do (internal linkage each). One
// shared 4096-byte buffer, a 0xFFF limit, and an overflow replaced by the fixed warning.
char g_overGrowthFormatBuffer[0x1000];

vc71::string overGrowthFormatString(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    const int written = ::_vsnprintf(g_overGrowthFormatBuffer, 0xFFF, format, args);
    va_end(args);

    if (written != -1)
        g_overGrowthFormatBuffer[written] = '\0';
    else
        ::sprintf(g_overGrowthFormatBuffer, "%s",
                  "WARNING: The original message was too long for formatString(). \n");
    return vc71::string(g_overGrowthFormatBuffer);
}


// Geometry-table entries pair LOD and full-detail records, each with its instance cache.
// Geometry instances retain the same virtual order across host ABI flavors.
inline void* geometrySlot(void* geometry, unsigned int offset)
{
    return (*reinterpret_cast<void***>(geometry))[offset / sizeof(void*)];
}
void geometryAddRef(void* geometry)                                           // +0x00
{
    typedef int (__thiscall* Fn)(void*);
    reinterpret_cast<Fn>(geometrySlot(geometry, 0x00))(geometry);
}
void geometryRelease(void* geometry)                                          // +0x08
{
    typedef int (__thiscall* Fn)(void*);
    reinterpret_cast<Fn>(geometrySlot(geometry, 0x08))(geometry);
}

IGeometryTemplateManager* geometryTemplateManager()
{
    return static_cast<IGeometryTemplateManager*>(static_cast<void*>(g_GeometryTemplateManager));
}

// ObjectTemplateManager PE +0x34. MSVC groups the five getTemplate overloads in reverse declaration
// order at the first one's position, so +0x34 is getTemplate(const std::string&) - the one-argument
// call 1015EBB8..1015EBC1 makes. A std::string crosses: typed native, raw retail slot otherwise.
world::IObjectTemplate* objectTemplateByName(const vc71::string& name)
{
#if BF2_PLUGIN_ABI_NATIVE
    return reinterpret_cast<world::ObjectTemplateManager*>(::dice::hfe::objectTemplateManager)
        ->getTemplate(renddx9abi::toHostString(name));
#else
    typedef world::IObjectTemplate* (__thiscall* GetTemplateFn)(void*, const vc71::string&);
    void* const manager = ::dice::hfe::objectTemplateManager;
    return reinterpret_cast<GetTemplateFn>((*static_cast<void***>(manager))[0x34 / 4])(manager, name);
#endif
}

// sub_1015EB80 (123 B, read in full): members in declaration order (+0x1C, +0x30, +0x34, the vector,
// +0x4C = 1.0), the two count pairs in the body, then the two template lookups. NO null test on
// the geometry template before load()/createInstance() - shipped.
void geometryRecordConstruct(void* record, const vc71::string& name)
{
    ::new (&field<vc71::string>(record, 0x00)) vc71::string(name);
    field<void*>(record, 0x1C) = 0;
    field<void*>(record, 0x30) = 0;
    field<void*>(record, 0x34) = 0;
    field<void*>(record, 0x40) = 0;
    field<void*>(record, 0x44) = 0;
    field<void*>(record, 0x48) = 0;
    field<float>(record, 0x4C) = 1.0f;
    field<unsigned int>(record, 0x20) = 0;
    field<unsigned int>(record, 0x24) = 0;
    field<unsigned int>(record, 0x28) = 0;
    field<unsigned int>(record, 0x2C) = 0;

    world::IObjectTemplate* const objectTemplate =
        objectTemplateByName(field<vc71::string>(record, 0x00));
    field<world::IObjectTemplate*>(record, 0x30) = objectTemplate;
    if (objectTemplate)
        objectTemplate->preCache();                                             // IObjectTemplate +0x68

    field<IGeometryTemplate*>(record, 0x34) = geometryTemplateManager()->getTemplate(
        renddx9abi::toHostString(field<vc71::string>(record, 0x00)));           // +0x18
    field<IGeometryTemplate*>(record, 0x34)->load();                            // +0x2C
    field<void*>(record, 0x1C) = field<IGeometryTemplate*>(record, 0x34)->createInstance();   // +0x24
}

// sub_1015EC70 (230 B, read in full): the copy constructor. The initializer list copies the
// pointers, +0x38 and +0x4C (the vector starts empty); the body addRefs the geometry (its slot
// +0x00, NO null test), copies the two count pairs, then pushes and addRefs every vector entry.
void geometryRecordCopy(void* record, const void* other)
{
    ::new (&field<vc71::string>(record, 0x00)) vc71::string(field<vc71::string>(other, 0x00));
    field<void*>(record, 0x1C) = field<void*>(other, 0x1C);
    field<void*>(record, 0x30) = field<void*>(other, 0x30);
    field<void*>(record, 0x34) = field<void*>(other, 0x34);
    field<unsigned int>(record, 0x38) = field<unsigned int>(other, 0x38);
    field<void*>(record, 0x40) = 0;
    field<void*>(record, 0x44) = 0;
    field<void*>(record, 0x48) = 0;
    field<float>(record, 0x4C) = field<float>(other, 0x4C);
    geometryAddRef(field<void*>(record, 0x1C));
    field<unsigned int>(record, 0x20) = field<unsigned int>(other, 0x20);
    field<unsigned int>(record, 0x24) = field<unsigned int>(other, 0x24);
    field<unsigned int>(record, 0x28) = field<unsigned int>(other, 0x28);
    field<unsigned int>(record, 0x2C) = field<unsigned int>(other, 0x2C);

    const vc71::vector_layout& source = field<vc71::vector_layout>(other, 0x3C);
    vc71::vectorRef<void*> target(&field<vc71::vector_layout>(record, 0x3C));
    for (unsigned int i = 0; i < vc71::vectorSizeIn(source, 4); ++i)
    {
        void* const entry = reinterpret_cast<void* const*>(source.first)[i];
        target.push_back(entry);                                                // 1009A340 when full
        geometryAddRef(entry);                                                  // slot +0x00
    }
}

// sub_1015EC00 (101 B, read in full): release the geometry (slot +0x08, guarded), release every
// vector entry (slot +0x08, unguarded), free and zero the vector, destroy the name.
void geometryRecordDestroy(void* record)
{
    if (field<void*>(record, 0x1C))
        geometryRelease(field<void*>(record, 0x1C));
    vc71::vector_layout& entries = field<vc71::vector_layout>(record, 0x3C);
    for (unsigned int i = 0; i < vc71::vectorSizeIn(entries, 4); ++i)
        geometryRelease(reinterpret_cast<void**>(entries.first)[i]);
    if (entries.first)
        ::renddx9abi::engineFree(entries.first);
    entries.first = 0;
    entries.last = 0;
    entries.end = 0;
    field<vc71::string>(record, 0x00).~string();
}

// The 0xA0-byte vector element. Its copy (0x10160610: +0x00 then +0x50) and its destruction
// (0x10160640 / the range destroy 0x101607F0: +0x50 then +0x00) are the two records' own.
// [CHOSEN] carrier so vc71::vectorRef can copy/destroy raw records with their own bodies.
struct GeometryPair
{
    unsigned int words[0xA0 / 4];

    unsigned char*       bytes()       { return reinterpret_cast<unsigned char*>(words); }
    const unsigned char* bytes() const { return reinterpret_cast<const unsigned char*>(words); }

    GeometryPair(const vc71::string& lodName, const vc71::string& name)
    {
        geometryRecordConstruct(bytes(), lodName);
        geometryRecordConstruct(bytes() + 0x50, name);
    }
    GeometryPair(const GeometryPair& other)
    {
        geometryRecordCopy(bytes(), other.bytes());
        geometryRecordCopy(bytes() + 0x50, other.bytes() + 0x50);
    }
    ~GeometryPair()
    {
        geometryRecordDestroy(bytes() + 0x50);
        geometryRecordDestroy(bytes());
    }

private:
    GeometryPair& operator=(const GeometryPair&);
};
static_assert(sizeof(GeometryPair) == 0xA0, "10160BA0 divides by 0xA0");

// sub_101607F0 (47 B, read in full): destroy [first, last) of the geometry vector.
void destroyGeometryPairs(unsigned char* first, unsigned char* last)
{
    for (; first != last; first += 0xA0)
        reinterpret_cast<GeometryPair*>(first)->~GeometryPair();
}

// Base cleanup (1015F9B0 / the base destructor 1015FA50): the geometry table (101607F0 + free,
// 1015F9E9..1015FA13) and then the material table (100D28B0 + free, each
// element's destructor tidies its type vector and then its name), each vector's three pointers zeroed.
void releaseTypeVectors(void* self)
{
    vc71::vector_layout& geometries = field<vc71::vector_layout>(self, 0x4C);
    if (geometries.first)
    {
        destroyGeometryPairs(reinterpret_cast<unsigned char*>(geometries.first),
                             reinterpret_cast<unsigned char*>(geometries.last));
        ::renddx9abi::engineFree(geometries.first);
    }
    geometries.first = 0;
    geometries.last = 0;
    geometries.end = 0;
    overGrowthVectorTidy(materialTable(self));
}

// the two array frees are unconditional, then each field is cleared in order.
void clearBase(void* self)
{
    ::renddx9abi::engineFree(field<void*>(self, 0x80));
    void* second = field<void*>(self, 0x84);
    field<void*>(self, 0x80) = 0;
    ::renddx9abi::engineFree(second);
    field<void*>(self, 0x84) = 0;
    if (!field<unsigned char>(self, 0xB0))
        releaseTypeVectors(self);
    field<unsigned char>(self, 0x1E) = 0;
}

// sub_1015E5D0 (39 B) - the unpublished base class's setViewDistance (its own table 0x1021EBC0 holds
// it at +0x0C); the derived +0x0C calls it directly. Store the distance, then the cell size:
// distance / (grid / 2 - 1), the halving a signed `cdq; sub; sar 1` on the int at +0x10.
void baseSetViewDistance(void* self, float distance)
{
    field<float>(self, 0x24) = distance;
    field<float>(self, 0x14) = distance / static_cast<float>(field<int>(self, 0x10) / 2 - 1);
}

// sub_100D0550 (161 B, read in full), called with 3 by clear and setViewDistance: the vector
// deleting destructor. Elements are destroyed last TO first (the patch destructor is its vector's:
// free the buffer when set, zero first/last/end), then operator delete[] (1005F370) on the cookie.
// The scalar arm (flags without 2) is not reached by any caller.
void deletePatchArray(unsigned char* patches)
{
    const int count = *reinterpret_cast<int*>(patches - 4);
    for (int i = count - 1; i >= 0; --i)
    {
        unsigned char* const patch = patches + i * 0x98;
        if (field<void*>(patch, 0x10))
            ::renddx9abi::engineFree(field<void*>(patch, 0x10));
        field<void*>(patch, 0x10) = 0;
        field<void*>(patch, 0x14) = 0;
        field<void*>(patch, 0x18) = 0;
    }
    ::renddx9abi::engineFree(patches - 4);
}
}

OverGrowth::OverGrowth()
{
    // PE 1015F8F0, preserving untouched padding/fields rather than zeroing the whole object.
    field<unsigned int>(this, 0x04) = 0;
    field<unsigned int>(this, 0x08) = 0;
    field<unsigned int>(this, 0x0C) = 0;
    field<unsigned int>(this, 0x10) = 10;
    field<unsigned int>(this, 0x14) = 0;
    field<unsigned int>(this, 0x18) = 0x759DC150;
    field<unsigned char>(this, 0x1C) = 1;
    field<unsigned char>(this, 0x1D) = 0;
    field<unsigned char>(this, 0x1E) = 0;
    field<float>(this, 0x20) = 0.3f;
    field<unsigned int>(this, 0x24) = 0;
    field<float>(this, 0x30) = 0.4f;
    field<float>(this, 0x34) = 12.0f;
    field<unsigned char>(this, 0x38) = 0;
    field<float>(this, 0x28) = 1.0f;
    field<float>(this, 0x2C) = 1.0f;
    const unsigned int zeroFields[] = {
        0x40, 0x44, 0x48, 0x50, 0x54, 0x58, 0x5C, 0x60,
        0x6C, 0x70, 0x74, 0x78, 0x7C, 0x80, 0x84
    };
    for (unsigned int offset : zeroFields) field<unsigned int>(this, offset) = 0;
    ::new (static_cast<unsigned char*>(static_cast<void*>(this)) + 0x88) vc71::string();
    field<unsigned int>(this, 0x10) &= ~1u;
    field<unsigned char>(this, 0xA4) = 0;
    field<unsigned char>(this, 0xB0) = 0;
    field<float>(this, 0x24) = 80.0f;

    // PE 100CB940 derived half. The compiler supplies the complete 18-entry primary table.
    const unsigned int derivedZeroFields[] = {
        0xB8, 0xBC, 0xC0, 0xD0, 0xE0, 0xE4, 0xE8, 0xEC
    };
    for (unsigned int offset : derivedZeroFields) field<unsigned int>(this, offset) = 0;
    field<unsigned char>(this, 0xF0) = 0;
    field<unsigned char>(this, 0xF1) = 0;
    field<unsigned int>(this, 0xF8) = 0;
    field<unsigned int>(this, 0xFC) = 0;
    field<unsigned int>(this, 0x100) = 0;
    field<float>(this, 0xC8) = 0.15f;
    field<float>(this, 0xCC) = 0.3f;
    field<unsigned char>(this, 0xD4) = 1;
    field<unsigned char>(this, 0xD5) = 1;
    field<float>(this, 0xF4) = 1.0f;
    field<unsigned int>(this, 0x10) = 10;
    field<float>(this, 0x24) = 500.0f;
}

OverGrowth::~OverGrowth()
{
    // PE 100D2720 -> 100CB6C0 -> 1015FA50. Base cleanup runs again before the string dtor.
    OverGrowth::clear();
    if (field<void*>(this, 0xB8)) ::renddx9abi::engineFree(field<void*>(this, 0xB8));
    field<void*>(this, 0xB8) = 0;
    field<void*>(this, 0xBC) = 0;
    field<void*>(this, 0xC0) = 0;
    clearBase(this);
    field<vc71::string>(this, 0x88).~string();
    releaseTypeVectors(this);
}
void OverGrowth::operator delete(void* memory) noexcept
{ ::renddx9abi::engineFree(memory); }

OverGrowth* createOverGrowth()
{
    void* memory = ::renddx9abi::engineAlloc(0x154);
    return memory ? ::new (memory) OverGrowth() : 0;
}


// OverGrowth::clear - +0x30, PE 100CB6C0 (219 B), read in full. Every arm written:
//   100CB6CC  delete[] the patches (100D0550 with 3), +0xE0 = 0.
//   100CB6D8  each non-null cell of the N x N grid (row z, column x) deleted through its table's
//             slot 0 with 1, then operator delete[] (1005F370) on the grid, +0xE4 = 0.
//   100CB741  the atlas owner: sub_10015930 then operator delete; +0xFC = 0.
//   100CB75B  the adopted page: `if (!--refCount) { sub_10014E20; operator delete }`, +0x100 = 0.
//   100CB78A  the base clear (1015F9B0), then +0xA4 = 0.

void OverGrowth::clear()
{
    if (field<unsigned char*>(this, 0xE0))
        deletePatchArray(field<unsigned char*>(this, 0xE0));
    field<unsigned char*>(this, 0xE0) = 0;
    if (field<OverGrowthCell**>(this, 0xE4))
    {
        for (int z = 0; z < field<int>(this, 0x10); ++z)
        {
            for (int x = 0; x < field<int>(this, 0x10); ++x)
            {
                OverGrowthCell* const cell =
                    field<OverGrowthCell**>(this, 0xE4)[field<int>(this, 0x10) * z + x];
                if (cell)
                    cell->destroy(1);
            }
        }
        ::renddx9abi::engineFree(field<OverGrowthCell**>(this, 0xE4));
        field<OverGrowthCell**>(this, 0xE4) = 0;
    }
    if (dx9rend::TextureAtlas* const atlas = field<dx9rend::TextureAtlas*>(this, 0xFC))
    {
        dx9rend::textureAtlasDtor(atlas);
        ::renddx9abi::engineFree(atlas);
    }
    field<dx9rend::TextureAtlas*>(this, 0xFC) = 0;
    if (field<dx9rend::EngineTexture*>(this, 0x100))
        dx9rend::textureReleaseRef(field<dx9rend::EngineTexture*>(this, 0x100));
    clearBase(this);
    field<unsigned char>(this, 0xA4) = 0;
}


// Replaces the patch grid for a view distance, fitting whole cells to the terrain extent.
// Before terrain dimensions exist, only distance and base cell size are updated.

void OverGrowth::setViewDistance(float distance)
{
    if (field<unsigned char*>(this, 0xE0))
        deletePatchArray(field<unsigned char*>(this, 0xE0));
    field<unsigned char*>(this, 0xE0) = 0;
    baseSetViewDistance(this, distance);

    const float extentX = field<float>(this, 0x5C);
    const float extentZ = field<float>(this, 0x60);
    const float extent = (extentX < extentZ) ? extentX : extentZ;
    if (extent == 0.0f)
        return;

    const double cells = std::floor(static_cast<double>(extent / field<float>(this, 0x14)));
    const float cellSize = extent / static_cast<float>(cells);                         // 100CBAA8 fdivr
    field<float>(this, 0x14) = cellSize;                                               // 100CBAAF fst
    const float inverse = 1.0f / cellSize;                                             // 100CBAB2 fdivr 1.0
    field<int>(this, 0xD8) =
        static_cast<int>((field<float>(this, 0x5C) * inverse + 1.0f) * 0.5f) * 2;
    field<int>(this, 0xDC) =
        static_cast<int>((field<float>(this, 0x60) * inverse + 1.0f) * 0.5f) * 2;
}

float OverGrowth::getViewDistance() const { return field<float>(this, 0x24); }         // +0x10
OverGrowthCell* OverGrowth::getCell(int x, int z)                                      // +0x20
{
    return field<OverGrowthCell**>(this, 0xE4)[x + z * field<int>(this, 0x10)];
}


// OverGrowth::reload - +0x2C, PE 100CDDC0 (59 B), read in full: the editor's save-and-regenerate. The
// console's `Overgrowth.generateAll` (executor 100C6649, a +0x2C call) and every OvergrowthType.*
// setter once +0xA4 is set reach it. +0xB0 brackets the whole regeneration so clearBase keeps the
// material/type/geometry tables and postLoad keeps the stream counts.
// save() comes first: a live regeneration rewrites <path>Overgrowth.con before rebuilding.

void OverGrowth::reload()
{
    field<unsigned char>(this, 0xB0) = 1;
    save();
    clear();                                                               // 100CDDD3 virtual +0x30
    preLoad();
    preLoadResources();
    postLoadResources();
    generateAtlas();
    field<unsigned char>(this, 0xB0) = 0;
}


// sub_1015F450 (407 B, retn 4, read in full): the index of `name` in the geometry table, matched on
// either record's name (+0x00 the "_lod" record, then +0x50); -1 for an empty name. A miss builds a
// pair (the "_lod" record first, then the named one) and keeps it only when both got a geometry
// instance (the named record tested first), answering the new last index; else -1.
// Callers: initialize, preLoad, the static-object adoption, and populatePatch (+0x44, 100CC800).

int OverGrowth::findOrAddGeometry(const vc71::string& name)
{
    if (name.size() == 0)                                                  // [name+0x14] == 0
        return -1;
    vc71::vector_layout& geometries = field<vc71::vector_layout>(this, 0x4C);
    for (unsigned int i = 0; i < vc71::vectorSizeIn(geometries, 0xA0); ++i)
    {
        const unsigned char* const pair = reinterpret_cast<unsigned char*>(geometries.first) + i * 0xA0;
        if (sameString(field<vc71::string>(pair, 0x00), name) ||
            sameString(field<vc71::string>(pair, 0x50), name))
            return static_cast<int>(i);
    }

    const vc71::string full(name);                                        // var_20
    const vc71::string lod = concat(name, "_lod");                        // var_3C
    const GeometryPair pair(lod, full);                                   // var_F8 / var_A8
    int index = -1;
    if (field<void*>(pair.bytes(), 0x50 + 0x1C) && field<void*>(pair.bytes(), 0x1C))
    {
        vc71::vectorRef<GeometryPair>(&geometries).push_back(pair);
        index = static_cast<int>(vc71::vectorSizeIn(geometries, 0xA0)) - 1;
    }
    return index;
}


// Default vegetation-type settings. Material selection is case-insensitive; type selection
// is case-sensitive and may retain an active type from another material.
OverGrowthType::OverGrowthType()
    : name(), geometry(), probability(1.0f), density(0.0f), rotationScale(0.0f),
      minRadiusToSame(0.0f), minRadiusToOthers(0.0f), normalScale(1.0f), geometryIndex(-1)
{
    randomScale.x = 1.0f;
    randomScale.y = 1.0f;
}

// addMaterial's temporary (100CDFA0..): the name default-constructed and the three type-vector
// pointers zeroed. [CHOSEN, not recovered] the id: retail leaves the temporary's +0x1C unwritten
// (it is overwritten the moment the record lands in the table), and the allocator padding - both
// are zeroed here so no copy reads an indeterminate value.
OverGrowthMaterial::OverGrowthMaterial()
    : name(), id(0)
{
    types.allocator = 0;
    types.first = 0;
    types.last  = 0;
    types.end   = 0;
}

// Inlined at every by-value copy (100CDE20..100CDE3A, 100CDF00..): the string, +0x1C, then the
// type vector's copy constructor 100D0600.
OverGrowthMaterial::OverGrowthMaterial(const OverGrowthMaterial& other)
    : name(other.name), id(other.id)
{
    overGrowthVectorCopyConstruct(types, other.types);
}

// 100D1A70's element body (the erase copy-down): string =, +0x1C, then 100D0AE0.
OverGrowthMaterial& OverGrowthMaterial::operator=(const OverGrowthMaterial& other)
{
    name = other.name;
    id = other.id;
    overGrowthVectorAssign(types, other.types);
    return *this;
}

// 100D28B0's element body: the type vector's _Tidy, then the name.
OverGrowthMaterial::~OverGrowthMaterial()
{
    overGrowthVectorTidy(types);
}

// Overgrowth.path

// 100C92C0 (12 B): `add ecx, 88h; jmp std::string::operator=` - the only writer of +0x88 in the
// image (its one caller is the path executor 100C5407). A non-empty +0x88 is what opens
// preLoadResources / postLoadResources / render / updateCells (their +0x9C size tests).
void OverGrowth::setPath(const vc71::string& path)
{
    field<vc71::string>(this, 0x88) = path;
}

// 100C92D0 (7 B): `lea eax, [ecx+88h]; retn`.
const vc71::string& OverGrowth::getPath() const
{
    return field<vc71::string>(this, 0x88);
}

// the material table

// every material as "<name> [<id>]", backslash-separated, in table order.
vc71::string OverGrowth::getMaterialList() const
{
    vc71::string list;
    const OverGrowthVector<OverGrowthMaterial>& materials = materialTable(this);
    for (const OverGrowthMaterial* it = materials.first; it != materials.last; ++it)
    {
        if (list.size())
            list += "\\";
        list += overGrowthFormatString("%s [%d]", it->name.c_str(), it->id);
    }
    return list;
}

// 100CDEE0 (365 B), read in full. A duplicate ID answers false (the name is not compared). A new
// record is a default-constructed temporary pushed at the end (100D2D90); then ITS id is written,
// then its name, and both cursors are cleared.
bool OverGrowth::addMaterial(const vc71::string& name, unsigned int id)
{
    OverGrowthVector<OverGrowthMaterial>& materials = materialTable(this);
    for (OverGrowthMaterial* it = materials.first; it != materials.last; ++it)
    {
        const OverGrowthMaterial material(*it);                            // by value, as shipped
        if (id == material.id)
            return false;
    }

    overGrowthVectorPushBack(materials, OverGrowthMaterial());
    OverGrowthMaterial& added = materials.last[-1];                        // `[this+44h] - 30h`
    added.id = id;                                                         // +0x1C first,
    added.name = name;                                                     // then the name
    activeMaterial(this) = 0;                                              // +0xE8
    activeType(this) = 0;                                                  // +0xEC
    return true;
}

// 100CDE00 (210 B), read in full. Only the active material can be removed. Its types are cleared
// (VC7.1 clear() == _Tidy, 100CFE80) before the erase copies the tail down over it (100D1A70),
// destroys the last record (100D28B0) and shortens the table; both cursors are then cleared.
bool OverGrowth::removeActiveMaterial()
{
    if (!activeMaterial(this))
        return false;
    OverGrowthVector<OverGrowthMaterial>& materials = materialTable(this);
    for (OverGrowthMaterial* it = materials.first; it != materials.last; ++it)
    {
        const OverGrowthMaterial material(*it);                            // by value, as shipped
        if (activeMaterial(this) == it)
        {
            overGrowthVectorTidy(activeMaterial(this)->types);
            overGrowthVectorErase(materials, it);
            activeType(this) = 0;
            activeMaterial(this) = 0;
            return true;
        }
    }
    return false;
}

// the first material whose name matches case-INsensitively becomes active. A miss
// changes nothing, and the active type is never touched here.
void OverGrowth::setActiveMaterial(const vc71::string& name)
{
    OverGrowthVector<OverGrowthMaterial>& materials = materialTable(this);
    for (OverGrowthMaterial* it = materials.first; it != materials.last; ++it)
    {
        if (equalsNoCase(it->name, name))                                  // 100C9EF5 sub_10075080
        {
            activeMaterial(this) = it;
            return;
        }
    }
}

// the active material's name, or "" (unk_101DC333) without one.
vc71::string OverGrowth::getActiveMaterialName() const
{
    const OverGrowthMaterial* const material = activeMaterial(this);
    if (material)
        return material->name;
    return vc71::string("");
}

// the active material's type table

// the active material's type names ("%s"), backslash-separated; each record copied by
// value first; the end re-read through +0xE8 every pass.
vc71::string OverGrowth::getTypeList() const
{
    vc71::string list;
    const OverGrowthMaterial* const material = activeMaterial(this);
    if (material)
    {
        for (const OverGrowthType* it = material->types.first;
             it != activeMaterial(this)->types.last; ++it)
        {
            const OverGrowthType type(*it);                                // by value, as shipped
            if (list.size())
                list += "\\";
            list += overGrowthFormatString("%s", type.name.c_str());
        }
    }
    return list;
}

// 100CC6A0 (343 B), read in full. Needs an active material; a duplicate name answers false. The new
// record is a default-constructed temporary pushed at the end (100D1950); its name AND geometry are
// both set to the given name; then the previously active type is re-selected by its saved name
// (an empty name when there was none, which setActiveType turns into "no active type").
bool OverGrowth::addType(const vc71::string& name)
{
    if (!activeMaterial(this))
        return false;
    for (OverGrowthType* it = activeMaterial(this)->types.first;
         it != activeMaterial(this)->types.last; ++it)
    {
        const OverGrowthType type(*it);                                    // by value, as shipped
        if (sameString(type.name, name))
            return false;
    }

    vc71::string previous;
    if (activeType(this))
        previous = activeType(this)->name;
    overGrowthVectorPushBack(activeMaterial(this)->types, OverGrowthType());
    OverGrowthType& added = activeMaterial(this)->types.last[-1];          // `[+0E8h]+28h - 5Ch`
    added.name = name;
    added.geometry = name;
    setActiveType(previous);
    return true;
}

// 100CBB30 (130 B), read in full. Needs an active material AND an active type; erases the active
// type from the active material (100D06B0) and clears the type cursor only.
bool OverGrowth::removeActiveType()
{
    if (!activeMaterial(this))
        return false;
    if (!activeType(this))
        return false;
    for (OverGrowthType* it = activeMaterial(this)->types.first;
         it != activeMaterial(this)->types.last; ++it)
    {
        const OverGrowthType type(*it);
        if (activeType(this) == it)
        {
            overGrowthVectorErase(activeMaterial(this)->types, it);
            activeType(this) = 0;
            return true;
        }
    }
    return false;
}

// 100CA050 (215 B), read in full. The first type of the active material whose name matches
// (case-sensitive) becomes active; no active material, or a miss, nulls the active type.
void OverGrowth::setActiveType(const vc71::string& name)
{
    OverGrowthMaterial* const material = activeMaterial(this);
    if (material)
    {
        for (OverGrowthType* it = material->types.first;
             it != activeMaterial(this)->types.last; ++it)
        {
            const OverGrowthType type(*it);                                // by value, as shipped
            if (sameString(type.name, name))
            {
                activeType(this) = it;
                return;
            }
        }
    }
    activeType(this) = 0;
}

// the active type's properties (OvergrowthType.*)
// Every setter: no active type -> nothing; else the field, then `if (+0xA4) this->+0x2C()` - the
// live regeneration, armed only once postLoadResources has set +0xA4 (100CD7AA). Every getter
// without an active type answers 0.0f (flt_101DDE64), "" or (1, 1).

void OverGrowth::setTypeGeometry(const vc71::string& geometry)
{
    OverGrowthType* const type = activeType(this);
    if (!type)
        return;
    type->geometry = geometry;
    if (field<unsigned char>(this, 0xA4))
        reload();
}
vc71::string OverGrowth::getTypeGeometry() const
{
    const OverGrowthType* const type = activeType(this);
    if (type)
        return type->geometry;
    return vc71::string("");
}

void OverGrowth::setTypeDensity(float value)
{
    OverGrowthType* const type = activeType(this);
    if (!type)
        return;
    type->density = value;
    if (field<unsigned char>(this, 0xA4))
        reload();
}
float OverGrowth::getTypeDensity() const
{
    const OverGrowthType* const type = activeType(this);
    return type ? type->density : 0.0f;
}

void OverGrowth::setTypeNormalScale(float value)
{
    OverGrowthType* const type = activeType(this);
    if (!type)
        return;
    type->normalScale = value;
    if (field<unsigned char>(this, 0xA4))
        reload();
}
float OverGrowth::getTypeNormalScale() const
{
    const OverGrowthType* const type = activeType(this);
    return type ? type->normalScale : 0.0f;
}

void OverGrowth::setTypeRotationScale(float value)
{
    OverGrowthType* const type = activeType(this);
    if (!type)
        return;
    type->rotationScale = value;
    if (field<unsigned char>(this, 0xA4))
        reload();
}
float OverGrowth::getTypeRotationScale() const
{
    const OverGrowthType* const type = activeType(this);
    return type ? type->rotationScale : 0.0f;
}

void OverGrowth::setTypeMinRadiusToSame(float value)
{
    OverGrowthType* const type = activeType(this);
    if (!type)
        return;
    type->minRadiusToSame = value;
    if (field<unsigned char>(this, 0xA4))
        reload();
}
float OverGrowth::getTypeMinRadiusToSame() const
{
    const OverGrowthType* const type = activeType(this);
    return type ? type->minRadiusToSame : 0.0f;
}

void OverGrowth::setTypeMinRadiusToOthers(float value)
{
    OverGrowthType* const type = activeType(this);
    if (!type)
        return;
    type->minRadiusToOthers = value;
    if (field<unsigned char>(this, 0xA4))
        reload();
}
float OverGrowth::getTypeMinRadiusToOthers() const
{
    const OverGrowthType* const type = activeType(this);
    return type ? type->minRadiusToOthers : 0.0f;
}

void OverGrowth::setTypeRandomScale(Vec2 value)                            // 100C9610, `retn 8`
{
    OverGrowthType* const type = activeType(this);
    if (!type)
        return;
    type->randomScale.x = value.x;                                         // +0x44
    type->randomScale.y = value.y;                                         // +0x48
    if (field<unsigned char>(this, 0xA4))
        reload();
}
Vec2 OverGrowth::getTypeRandomScale() const
{
    const OverGrowthType* const type = activeType(this);
    if (type)
        return type->randomScale;
    Vec2 unit;
    unit.x = 1.0f;                                                         // 3F800000h
    unit.y = 1.0f;
    return unit;
}

// plain fields and small accessors

float OverGrowth::getViewDistanceScale() const { return field<float>(this, 0x28); }    // exec 100C60A7
void  OverGrowth::setViewDistanceScale(float scale) { field<float>(this, 0x28) = scale; }
float OverGrowth::getCloseDistPercentage() const { return field<float>(this, 0xC8); }  // exec 100C680A
void  OverGrowth::setCloseDistPercentage(float value) { field<float>(this, 0xC8) = value; }
float OverGrowth::getCloseDistFade() const { return field<float>(this, 0xCC); }        // exec 100C690E
void  OverGrowth::setCloseDistFade(float value) { field<float>(this, 0xCC) = value; }
float OverGrowth::getLodTimer() const { return field<float>(this, 0xF8); }             // exec 100C6A12
void  OverGrowth::setLodTimer(float value) { field<float>(this, 0xF8) = value; }

// 100C8C40 (13 B) / the editor byte +0xF0. When set, preLoadResources skips the
// isOvergrowth-object adoption (100CC683 -> 100CBC80), the atlas texture collection 100CC1F0 adds
// those objects' "_lod" templates (100CC282), and rebuildCells branches on it (100CAF2A).
void OverGrowth::setEditorEnable(bool enable) { field<unsigned char>(this, 0xF0) = enable; }
bool OverGrowth::getEditorEnable() const { return field<unsigned char>(this, 0xF0) != 0; }

// 100C92E0 (16 B): +0xF4 (VideoOptions' scale) * +0xC8 (closeDistPercentage) * +0x24 (viewDistance),
// multiplied left to right.
float OverGrowth::getLodDistance() const
{
    return field<float>(this, 0xF4) * field<float>(this, 0xC8) * field<float>(this, 0x24);
}

// 100CDC20 (407 B), read in full: a std::set<std::string> of every NON-empty type geometry name
// over every material (index loops), then the set's names in order, backslash-separated.
vc71::string OverGrowth::getGeometryList() const
{
    std::set<std::string> names;                                           // 100CDC2A sub_100CE890
    const OverGrowthVector<OverGrowthMaterial>& materials = materialTable(this);
    for (unsigned int m = 0; materials.first && m < overGrowthVectorSize(materials); ++m)
    {
        const OverGrowthMaterial& material = materials.first[m];
        for (unsigned int t = 0;
             material.types.first && t < overGrowthVectorSize(material.types); ++t)
        {
            const OverGrowthType& type = material.types.first[t];
            if (!type.geometry.empty())                                    // type +0x30, _Mysize
                names.insert(std::string(type.geometry.c_str(), type.geometry.size()));
        }
    }
    vc71::string list;
    for (std::set<std::string>::const_iterator it = names.begin(); it != names.end(); ++it)
    {
        if (list.size())
            list += "\\";
        list += it->c_str();
    }
    return list;
}


// Editor painting requires initialized maps and an editable terrain singleton.
// Disc bounds are half-open. Export uses patch-width rows and stops at the first out-of-range patch.

namespace {

// dword_1021AE18 - ITerrain, the id the "Terrain" singleton is asked for.
const unsigned int kIID_ITerrain = 820004u;

// dword_10239050 - the material overlay's colours for map values 0..15, A8R8G8B8 from the image bytes
// (the same sixteen values as QuadPatchEditable.cpp's dword_1023AB28, a separate table in the image).
const unsigned int kMaterialOverlayPalette[16] =
{
    0xFF000000u, 0xFF0000FFu, 0xFF00FFFFu, 0xFF00FF00u, 0xFFFFFF00u, 0xFFFF0000u, 0xFFFF00FFu, 0xFFFFFFFFu,
    0xFF000077u, 0xFF007777u, 0xFF007700u, 0xFF777700u, 0xFF770000u, 0xFF770077u, 0xFF777777u, 0xFF333333u,
};

// dword_10258E4C / dword_10258E50 - zero-initialised (.bss) ints the material / shadow exporter adds to
// every map index; nothing in the image writes either (the 2006 build carries the same pair,
// 0x105A6580 / 0x105A6584). [UNRESOLVED] purpose and names.
int g_materialOverlayIndexBias = 0;
int g_shadowOverlayIndexBias = 0;

// ClassManager +0x48 getSingleton("HeightmapCluster") - typed in both flavours (HfeAbi.h). Every body
// below asks by name; none uses the cluster initialize cached at +0xAC.
world::IHeightmapCluster* heightmapClusterByName()
{
    return static_cast<world::IHeightmapCluster*>(static_cast<void*>(
        ::dice::hfe::classManager->getSingleton(renddx9abi::HostString("HeightmapCluster"))));
}

// ClassManager +0x4C getSingletonAs("Terrain", ITerrain) (ELF +0x50 _ZN...14getSingletonAsERKSsj -
// `void* getSingletonAs(const std::string&, unsigned int)`). RAW in both flavours: HfeAbi.h's
// IClassManager, which both flavours dispatch through with the HostString carrier, still declares the
// slot reserved_getSingletonAs(). [CHOSEN, not recovered] - the third copy of the RoadEditable.cpp /
// UndergrowthSystemLoad.cpp seam; all three retire by typing the slot in HfeAbi.h.
Terrain* terrainSingleton()
{
    typedef void* (__thiscall* GetSingletonAsFn)(void*, const renddx9abi::HostString&, unsigned int);
    void* const manager = ::dice::hfe::classManager;
    return static_cast<Terrain*>(static_cast<IBase*>(reinterpret_cast<GetSingletonAsFn>(
        (*reinterpret_cast<void***>(manager))[0x4C / 4])(manager, renddx9abi::HostString("Terrain"),
                                                         kIID_ITerrain)));
}

// The painters' distance terms, `fild a; fisub b` / `fild a; fsub [float b]` under RendDX9's 24-bit
// precision control: the exact difference, rounded once to single precision.
float differenceAsFloat(double a, double b)
{
    return static_cast<float>(a - b);
}

} // namespace

// 100CA130 (`retn 10h`) - Overgrowth.paintMaterial. A non-zero material must be the id (+0x1C) of a
// material in the table (the 2006 build's `missing` flag) or nothing happens; 0 erases. The rectangle
// goes to the overlay exporter only while the terrain shows technique 5.
void OverGrowth::paintMaterial(float x, float z, float radius, unsigned char material)
{
    if (material != 0)                                                                   // `cmp byte ptr`
    {
        const OverGrowthVector<OverGrowthMaterial>& materials = materialTable(this);
        bool missing = true;
        for (unsigned int i = 0; i < overGrowthVectorSize(materials); ++i)
        {
            if (materials.first[i].id == material)                                       // movzx
            {
                missing = false;
                break;
            }
        }
        if (missing)
            return;
    }

    world::IHeightmap* const heightmap = heightmapClusterByName()->getPrimaryHeightmap();   // +0x40
    int sizeX = 0;
    int sizeZ = 0;
    heightmap->getSize(sizeX, sizeZ);                                                         // +0x2C
    const float gridX = x / heightmap->getScale().x + static_cast<float>(sizeX / 2);         // +0x54, three
    const float gridZ = z / heightmap->getScale().z + static_cast<float>(sizeZ / 2);         // calls, in
    const float gridRadius = radius / heightmap->getScale().x;                                // this order
    const int centreX = static_cast<int>(gridX);          // fistp under a truncating control word
    const int centreZ = static_cast<int>(gridZ);
    const int reach = static_cast<int>(gridRadius);       // __ftol2
    const int x0 = centreX - reach <= 0 ? 0 : centreX - reach;
    const int x1 = sizeX < centreX + reach ? sizeX : centreX + reach;
    const int z0 = centreZ - reach <= 0 ? 0 : centreZ - reach;
    const int z1 = sizeZ < centreZ + reach ? sizeZ : centreZ + reach;
    for (int ix = x0; ix < x1; ++ix)
    {
        const float dx = differenceAsFloat(ix, centreX);
        const float dx2 = dx * dx;
        const float centreZf = static_cast<float>(centreZ);
        for (int iz = z0; iz < z1; ++iz)
        {
            const float dz = differenceAsFloat(iz, centreZf);
            if (!(std::sqrt(dz * dz + dx2) > gridRadius))                                 // test ah, 41h
                field<unsigned char*>(this, 0x80)[iz * sizeX + ix] = material;
        }
    }

    ITerrainEditable* const editable =
        static_cast<ITerrainEditable*>(terrainSingleton()->queryInterface(kIID_ITerrainEditable));
    if (editable->getShaderTechnique() == 5)                                                  // E+0x8C
        exportMaterialRectToTerrainTexture(static_cast<unsigned int>(x0), static_cast<unsigned int>(x1),
                                           static_cast<unsigned int>(z0), static_cast<unsigned int>(z1));
}

// 100C9670 (`retn 10h`) - Overgrowth.paintShadowmap: paintMaterial's disc over the +0x84 shadow map,
// with no check on the value (the 2006 build logs every call first; 1.50 does not); technique 6.
void OverGrowth::paintShadowmap(float x, float z, float radius, unsigned char value)
{
    world::IHeightmap* const heightmap = heightmapClusterByName()->getPrimaryHeightmap();   // +0x40
    int sizeX = 0;
    int sizeZ = 0;
    heightmap->getSize(sizeX, sizeZ);                                                         // +0x2C
    const float gridX = x / heightmap->getScale().x + static_cast<float>(sizeX / 2);
    const float gridZ = z / heightmap->getScale().z + static_cast<float>(sizeZ / 2);
    const float gridRadius = radius / heightmap->getScale().x;
    const int centreX = static_cast<int>(gridX);
    const int centreZ = static_cast<int>(gridZ);
    const int reach = static_cast<int>(gridRadius);
    const int x0 = centreX - reach <= 0 ? 0 : centreX - reach;
    const int x1 = sizeX < centreX + reach ? sizeX : centreX + reach;
    const int z0 = centreZ - reach <= 0 ? 0 : centreZ - reach;
    const int z1 = sizeZ < centreZ + reach ? sizeZ : centreZ + reach;
    for (int ix = x0; ix < x1; ++ix)
    {
        const float dx = differenceAsFloat(ix, centreX);
        const float dx2 = dx * dx;
        const float centreZf = static_cast<float>(centreZ);
        for (int iz = z0; iz < z1; ++iz)
        {
            const float dz = differenceAsFloat(iz, centreZf);
            if (!(std::sqrt(dz * dz + dx2) > gridRadius))
                field<unsigned char*>(this, 0x84)[iz * sizeX + ix] = value;
        }
    }

    ITerrainEditable* const editable =
        static_cast<ITerrainEditable*>(terrainSingleton()->queryInterface(kIID_ITerrainEditable));
    if (editable->getShaderTechnique() == 6)
        exportShadowmapRectToTerrainTexture(static_cast<unsigned int>(x0), static_cast<unsigned int>(x1),
                                            static_cast<unsigned int>(z0), static_cast<unsigned int>(z1));
}

// 100C8F40 (`retn`) - Overgrowth.exportToTerrainTexture, also TerrainEditable's technique-5 switch:
// the whole material map, texels 0 .. size - 2 per axis.
void OverGrowth::exportToTerrainTexture()
{
    int sizeX = 0;
    int sizeZ = 0;
    heightmapClusterByName()->getPrimaryHeightmap()->getSize(sizeX, sizeZ);
    exportMaterialRectToTerrainTexture(0, static_cast<unsigned int>(sizeX - 2),
                                       0, static_cast<unsigned int>(sizeZ - 2));
}

// 100C9250 (`retn`) - the shadow-map twin, reached only from TerrainEditable's technique-6 switch.
void OverGrowth::exportShadowmapToTerrainTexture()
{
    int sizeX = 0;
    int sizeZ = 0;
    heightmapClusterByName()->getPrimaryHeightmap()->getSize(sizeX, sizeZ);
    exportShadowmapRectToTerrainTexture(0, static_cast<unsigned int>(sizeX - 2),
                                        0, static_cast<unsigned int>(sizeZ - 2));
}

// 100C8C90 (`retn 10h`) - the material map into the technique-5 overlay (E+0xEC) of every patch the
// texel rectangle touches, patch columns outer: map value v <= 15 takes the palette, anything else
// v * 0x010101 (a grey with alpha 0). The order of the host calls is the shipped one.
void OverGrowth::exportMaterialRectToTerrainTexture(unsigned int x0, unsigned int x1,
                                                    unsigned int z0, unsigned int z1)
{
    Terrain* const terrain = terrainSingleton();
    ITerrainEditable* const editable =
        static_cast<ITerrainEditable*>(terrain->queryInterface(kIID_ITerrainEditable));
    if (editable->getShaderTechnique() != 5)                                                  // E+0x8C
        return;
    if (!field<unsigned char*>(this, 0x80))
    {
        editable->setShaderTechnique(0);                                                      // E+0x88
        return;
    }
    int sizeX = 0;
    int sizeZ = 0;
    heightmapClusterByName()->getPrimaryHeightmap()->getSize(sizeX, sizeZ);
    const unsigned int patchSize = static_cast<unsigned int>(terrain->getPatchSize());       // P+0x34
    const unsigned int patchCount = static_cast<unsigned int>(terrain->getPatchCount());     // P+0x30
    const unsigned int lastPatchX = x1 / patchSize;                                           // `div`
    const unsigned int firstPatchZ = z0 / patchSize;
    const unsigned int lastPatchZ = z1 / patchSize;
    for (unsigned int patchX = x0 / patchSize; patchX <= lastPatchX; ++patchX)
    {
        const unsigned int texelX = patchX * patchSize;
        unsigned int patchIndex = firstPatchZ * patchCount + patchX;
        for (unsigned int patchZ = firstPatchZ; patchZ <= lastPatchZ; ++patchZ, patchIndex += patchCount)
        {
            if (patchIndex >= patchCount * patchCount)
                return;
            const unsigned int texelZ = patchZ * patchSize;
            IDirect3DSurface9* const surface = dx9rend::textureGetLevelSurface(
                static_cast<dx9rend::EngineTexture*>(editable->getPatchOvergrowthOverlay(
                    static_cast<int>(patchIndex))), 0, 0x7FFFFFFFu);                          // E+0xEC
            RECT rect;
            rect.left = 0;
            rect.top = 0;
            rect.right = static_cast<LONG>(patchSize);
            rect.bottom = static_cast<LONG>(patchSize);
            D3DLOCKED_RECT locked;
            surface->LockRect(&locked, &rect, 0);                                             // HRESULT unread
            unsigned int* column = static_cast<unsigned int*>(locked.pBits);
            for (unsigned int j = 0; j < patchSize; ++j, ++column)
            {
                unsigned int* texel = column;
                for (unsigned int i = 0; i < patchSize; ++i, texel += patchSize)
                {
                    const unsigned int value = field<unsigned char*>(this, 0x80)[
                        (texelZ + i) * static_cast<unsigned int>(sizeX) + texelX + j
                        + static_cast<unsigned int>(g_materialOverlayIndexBias)];
                    *texel = value <= 15 ? kMaterialOverlayPalette[value] : value * 0x010101u;
                }
            }
            surface->UnlockRect();
            surface->Release();
        }
    }
}

// 100C8FB0 (`retn 10h`) - the shadow map into the technique-6 overlay (E+0xF0), every value as the grey
// v * 0x010101 (alpha 0); otherwise 100C8C90 line for line.
void OverGrowth::exportShadowmapRectToTerrainTexture(unsigned int x0, unsigned int x1,
                                                     unsigned int z0, unsigned int z1)
{
    Terrain* const terrain = terrainSingleton();
    ITerrainEditable* const editable =
        static_cast<ITerrainEditable*>(terrain->queryInterface(kIID_ITerrainEditable));
    if (editable->getShaderTechnique() != 6)
        return;
    if (!field<unsigned char*>(this, 0x84))
    {
        editable->setShaderTechnique(0);
        return;
    }
    int sizeX = 0;
    int sizeZ = 0;
    heightmapClusterByName()->getPrimaryHeightmap()->getSize(sizeX, sizeZ);
    const unsigned int patchSize = static_cast<unsigned int>(terrain->getPatchSize());
    const unsigned int patchCount = static_cast<unsigned int>(terrain->getPatchCount());
    const unsigned int lastPatchX = x1 / patchSize;
    const unsigned int firstPatchZ = z0 / patchSize;
    const unsigned int lastPatchZ = z1 / patchSize;
    for (unsigned int patchX = x0 / patchSize; patchX <= lastPatchX; ++patchX)
    {
        const unsigned int texelX = patchX * patchSize;
        unsigned int patchIndex = firstPatchZ * patchCount + patchX;
        for (unsigned int patchZ = firstPatchZ; patchZ <= lastPatchZ; ++patchZ, patchIndex += patchCount)
        {
            if (patchIndex >= patchCount * patchCount)
                return;
            const unsigned int texelZ = patchZ * patchSize;
            IDirect3DSurface9* const surface = dx9rend::textureGetLevelSurface(
                static_cast<dx9rend::EngineTexture*>(editable->getPatchOvergrowthShadowOverlay(
                    static_cast<int>(patchIndex))), 0, 0x7FFFFFFFu);                          // E+0xF0
            RECT rect;
            rect.left = 0;
            rect.top = 0;
            rect.right = static_cast<LONG>(patchSize);
            rect.bottom = static_cast<LONG>(patchSize);
            D3DLOCKED_RECT locked;
            surface->LockRect(&locked, &rect, 0);
            unsigned int* column = static_cast<unsigned int*>(locked.pBits);
            for (unsigned int j = 0; j < patchSize; ++j, ++column)
            {
                unsigned int* texel = column;
                for (unsigned int i = 0; i < patchSize; ++i, texel += patchSize)
                {
                    const unsigned int value = field<unsigned char*>(this, 0x84)[
                        (texelZ + i) * static_cast<unsigned int>(sizeX) + texelX + j
                        + static_cast<unsigned int>(g_shadowOverlayIndexBias)];
                    *texel = value * 0x010101u;
                }
            }
            surface->UnlockRect();
            surface->Release();
        }
    }
}

} } }

namespace dice { namespace hfe { namespace dx9rend {
extern void* g_OverGrowth;
// ResourceManager's direct calls have no null guard in the PE. The owner exists from DLL load
// (BF2Render.cpp's g_OverGrowth, the 0x101D22E0 initialiser) and again after every CEPreLoad.
void overGrowthPreLoadResources() { static_cast<geom::OverGrowth*>(g_OverGrowth)->preLoadResources(); }
void overGrowthPostLoadResources() { static_cast<geom::OverGrowth*>(g_OverGrowth)->postLoadResources(); }
} } }
#endif
