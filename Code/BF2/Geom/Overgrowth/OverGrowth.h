// Vegetation resources, cell batching, near-instance rendering and shadow collection.
// Shared internal records retain compatibility offsets; console records use typed values.

#ifndef BF2_GEOM_OVERGROWTH_H
#define BF2_GEOM_OVERGROWTH_H

#ifdef BF2_IMAGE_RENDDX9
#include "../../Math/Vec3.h"
#include "../../../RendDX9Abi/Vc71String.h"     // the records' 28-byte strings
#include "../../../RendDX9Abi/Vc71Vector.h"     // engineAlloc/engineFree, <new>

namespace dice { namespace hfe {
namespace dx9rend { class IRenderView; }
namespace geom {
using dx9rend::IRenderView;

// [INFERRED] names for instruction-proven pointer carriers. These declarations do not claim
// recovered record members, STL element types beyond the observed strides, or source constness.
struct OverGrowthCell;             // 0x2AC bytes, defined in OverGrowthCell.h (constructor 100D0C90)
struct OverGrowthPatch;            // 0x98-byte entries at owner +0xE0
struct OverGrowthInstance;         // 0x60-byte entries
struct OverGrowthInstanceVector;   // VC7.1 vector of the preceding entries
struct OverGrowthShadowVolumes;    // vector of 0x498-byte records
struct OverGrowthShadowBounds;
struct OverGrowthShadowState;
struct OverGrowthGeometryVector;


// [CHOSEN, not recovered] Nonowning VC7.1 vector view for raw owner storage.
// Companion helpers construct/copy/destroy elements and manage buffers on the engine heap.

template <class T>
struct OverGrowthVector
{
    void* allocator;   // +0x00  std::allocator<T>: empty, padded to 4 - never read by the image
    T*    first;       // +0x04  _Myfirst
    T*    last;        // +0x08  _Mylast
    T*    end;         // +0x0C  _Myend
};

template <class T>
inline unsigned int overGrowthVectorSize(const OverGrowthVector<T>& v)
{
    return v.first ? static_cast<unsigned int>(v.last - v.first) : 0u;
}

template <class T>
inline unsigned int overGrowthVectorCapacity(const OverGrowthVector<T>& v)
{
    return v.first ? static_cast<unsigned int>(v.end - v.first) : 0u;
}

// _Destroy(first, last)
template <class T>
inline void overGrowthDestroyRange(T* first, T* last)
{
    for (; first != last; ++first)
        first->~T();
}

// _Ucopy(first, last, dest) - copy-construct into raw storage, answer the new end.
template <class T>
inline T* overGrowthUninitializedCopy(const T* first, const T* last, T* dest)
{
    for (; first != last; ++first, ++dest)
        ::new (static_cast<void*>(dest)) T(*first);
    return dest;
}

// std::copy for class types - element-wise assignment, answer the new end.
template <class T>
inline T* overGrowthCopyAssign(const T* first, const T* last, T* dest)
{
    for (; first != last; ++first, ++dest)
        *dest = *first;
    return dest;
}

// _Tidy - VC7.1's clear() AND the destructor's body: destroy, free, null all three pointers.
// (VC7.1 vector::clear() is `_Tidy()`, which is why removeActiveMaterial's `types.clear()` and the
// base cleanup's table release are the same 100CFE80 body.)
template <class T>
inline void overGrowthVectorTidy(OverGrowthVector<T>& v)
{
    if (v.first)
    {
        overGrowthDestroyRange(v.first, v.last);
        ::renddx9abi::engineFree(v.first);
    }
    v.first = 0;
    v.last  = 0;
    v.end   = 0;
}

// _Buy(n): allocate exactly n, nothing for n == 0.
template <class T>
inline void overGrowthVectorBuy(OverGrowthVector<T>& v, unsigned int capacity)
{
    v.first = 0;
    v.last  = 0;
    v.end   = 0;
    if (capacity == 0)
        return;
    v.first = static_cast<T*>(::renddx9abi::engineAlloc(capacity * sizeof(T)));
    v.last  = v.first;
    v.end   = v.first + capacity;
}

// vector(const vector&) - 100D0600: _Buy(right.size()) then _Ucopy.
template <class T>
inline void overGrowthVectorCopyConstruct(OverGrowthVector<T>& v, const OverGrowthVector<T>& right)
{
    v.allocator = 0;   // [CHOSEN, not recovered] the empty allocator's padding; never read
    overGrowthVectorBuy(v, overGrowthVectorSize(right));
    if (v.first)
        v.last = overGrowthUninitializedCopy(right.first, right.last, v.first);
}

// operator= - 100D0AE0, the VC7.1 four-arm assignment.
template <class T>
inline void overGrowthVectorAssign(OverGrowthVector<T>& v, const OverGrowthVector<T>& right)
{
    if (&v == &right)
        return;
    const unsigned int count = overGrowthVectorSize(right);
    if (count == 0)
    {
        overGrowthVectorTidy(v);                                  // clear(): free storage
    }
    else if (count <= overGrowthVectorSize(v))
    {
        T* const newLast = overGrowthCopyAssign(right.first, right.last, v.first);
        overGrowthDestroyRange(newLast, v.last);                  // destroy old
        v.last = v.first + count;
    }
    else if (count <= overGrowthVectorCapacity(v))
    {
        const T* const middle = right.first + overGrowthVectorSize(v);
        overGrowthCopyAssign(right.first, middle, v.first);
        v.last = overGrowthUninitializedCopy(middle, right.last, v.last);
    }
    else
    {
        if (v.first)
        {
            overGrowthDestroyRange(v.first, v.last);             // discard old array
            ::renddx9abi::engineFree(v.first);
        }
        overGrowthVectorBuy(v, count);
        if (v.first)
            v.last = overGrowthUninitializedCopy(right.first, right.last, v.first);
    }
}

// push_back - construct in place when there is room, otherwise _Insert_n at
// end() (100D0880 / 100D2A20): copy the value first, grow by half (at least to size + 1), copy the
// prefix, construct the new element, then destroy and free the old array.
template <class T>
inline void overGrowthVectorPushBack(OverGrowthVector<T>& v, const T& value)
{
    const unsigned int size = overGrowthVectorSize(v);
    if (v.first && size < overGrowthVectorCapacity(v))
    {
        ::new (static_cast<void*>(v.last)) T(value);             // _Ufill(_Mylast, 1, _Val)
        ++v.last;
        return;
    }

    const T copy(value);                                          // _Tmp = _Val
    const unsigned int oldCapacity = overGrowthVectorCapacity(v);
    unsigned int capacity = oldCapacity + oldCapacity / 2u;
    if (capacity < size + 1u)
        capacity = size + 1u;
    T* const buffer = static_cast<T*>(::renddx9abi::engineAlloc(capacity * sizeof(T)));
    T* p = overGrowthUninitializedCopy(v.first, v.last, buffer);  // prefix (the suffix is empty)
    ::new (static_cast<void*>(p)) T(copy);
    if (v.first)
    {
        overGrowthDestroyRange(v.first, v.last);
        ::renddx9abi::engineFree(v.first);
    }
    v.end   = buffer + capacity;
    v.last  = buffer + size + 1u;
    v.first = buffer;
}

// erase(where) - std::copy(where + 1, last, where); destroy the last; --last.
template <class T>
inline T* overGrowthVectorErase(OverGrowthVector<T>& v, T* where)
{
    overGrowthCopyAssign(where + 1, static_cast<const T*>(v.last), where);
    overGrowthDestroyRange(v.last - 1, v.last);
    --v.last;
    return where;
}


// Per-material vegetation type, including placement weight and preloaded geometry.

struct OverGrowthType
{
    OverGrowthType();

    vc71::string name;                   // +0x00  addType/setActiveType key (case-sensitive ==)
    vc71::string geometry;               // +0x1C  OvergrowthType.geometry; addType seeds it with name
    float        probability;            // +0x38  [INFERRED] 1.0f; postLoad: density / sum(density)
    float        density;                // +0x3C  OvergrowthType.density
    float        rotationScale;          // +0x40  OvergrowthType.rotationScale
    Vec2         randomScale;            // +0x44  OvergrowthType.randomScale (1,1); save omits it
    float        minRadiusToSame;        // +0x4C  OvergrowthType.minRadiusToSame
    float        minRadiusToOthers;      // +0x50  OvergrowthType.minRadiusToOthers
    float        normalScale;            // +0x54  OvergrowthType.normalScale
    int          geometryIndex;          // +0x58  -1 until preLoad resolves `geometry` (1015F450)
};
static_assert(sizeof(OverGrowthType) == 0x5C, "OverGrowthType is 0x5C in the PE (100D0600 divides by 92)");


// OverGrowthMaterial - one Overgrowth.addMaterial record, 0x30 bytes, in the owner's +0x3C vector
// (preLoad/save/getMaterialList divide by 48). The copy constructor is inlined at every by-value
// copy (string, +0x1C, then 100D0600 on the types); assignment is 100D1A70's element body (string =,
// +0x1C, 100D0AE0); the destructor is 100D28B0's element body (_Tidy the types, then the string).

struct OverGrowthMaterial
{
    OverGrowthMaterial();
    OverGrowthMaterial(const OverGrowthMaterial& other);
    OverGrowthMaterial& operator=(const OverGrowthMaterial& other);
    ~OverGrowthMaterial();

    vc71::string                     name;   // +0x00  setActiveMaterial key (_stricmp)
    unsigned int                     id;     // +0x1C  addMaterial's second argument (unique): the
                                             //        Overgrowth.raw value, 0..15 (the coverage slot)
    OverGrowthVector<OverGrowthType> types;  // +0x20  first +0x24, last +0x28, end +0x2C
};
static_assert(sizeof(OverGrowthMaterial) == 0x30, "OverGrowthMaterial is 0x30 in the PE");

// The native host and compatibility image retain this image-owned layout. In particular,
// VideoOptions writes +0xF4 and post-load/draw test the VC7.1 string length at +0x9C.
// The unpublished base's constructor/cleanup are factored as helpers; no base type name is guessed.
class OverGrowth
{
public:
    OverGrowth();
    virtual ~OverGrowth();
    virtual void initialize();
    virtual void update(IRenderView* view);
    virtual void setViewDistance(float distance);                   // +0C 100CBA30
    virtual float getViewDistance() const;
    virtual void rebuildCells();
    virtual unsigned int sampleDensity(float x, float z);
    virtual void updateCells(IRenderView* view);                     // +1C 100C9450
    virtual OverGrowthCell* getCell(int x, int z);
    virtual void buildCell(OverGrowthCell*, OverGrowthInstanceVector*);
    virtual void collectCell(OverGrowthCell*, OverGrowthInstanceVector*);
    virtual void reload();                                         // +2C 100CDDC0
    virtual void clear();
    virtual void render(IRenderView* view, float time);
    // +38 the LightManager's directional records (VC7.1 vector, 0x498-byte elements), the
    // pulled-back light Frustum it culls patches with, the caster Frustum whose near plane it grows,
    // and the geometry vector it fills (LightManager +0x970). The opaque carrier names stay; the
    // call site casts.
    virtual void collectShadowGeometry(OverGrowthShadowVolumes*, OverGrowthShadowBounds*,
                                       OverGrowthShadowState*, OverGrowthGeometryVector*);
    virtual void updateCell(OverGrowthCell*, OverGrowthInstanceVector*, bool changed); // +3C 100CB080
    virtual void placeInstance(OverGrowthType*, Vec3*, OverGrowthInstance*);
    virtual void populatePatch(OverGrowthPatch*);

    void preLoadResources();                                        // direct 100CC500
    void postLoadResources();                                       // direct 100CD020
    static void operator delete(void* memory) noexcept;


    // The non-virtual members the Overgrowth / OvergrowthType console executors call (thiscall,
    // `mov ecx, dword_10258F38`), and the load path's geometry-table lookup. Names are [INFERRED]
    // from the console spelling; getter constness is [INFERRED]. Every OvergrowthType.* member acts
    // on the active type (+0xEC) and is a no-op / default read when there is none.

    void                setPath(const vc71::string& path);
    const vc71::string& getPath() const;
    void                preLoad();
    vc71::string        getMaterialList() const;
    bool                addMaterial(const vc71::string& name, unsigned int id);
    bool                removeActiveMaterial();
    void                setActiveMaterial(const vc71::string& name);
    vc71::string        getActiveMaterialName() const;
    vc71::string        getTypeList() const;
    bool                addType(const vc71::string& name);
    bool                removeActiveType();
    void                setActiveType(const vc71::string& name);
    void                save();                                             // 100CD7E0 (editor)
    vc71::string        getGeometryList() const;
    float               getLodDistance() const;
    void                generateAtlas();                                    // 100CCD20 (editor)
    void                setEditorEnable(bool enable);
    bool                getEditorEnable() const;

    // Direct fields the retail executors read and write with no call at all ([CHOSEN] typed
    // accessors in the SkyDome manner; the executor addresses are in OverGrowthConsole.cpp).
    float getViewDistanceScale() const;       void setViewDistanceScale(float scale);     // +0x28
    float getCloseDistPercentage() const;     void setCloseDistPercentage(float value);   // +0xC8
    float getCloseDistFade() const;           void setCloseDistFade(float value);         // +0xCC
    float getLodTimer() const;                void setLodTimer(float value);              // +0xF8

    // The active type's properties - OvergrowthType.*. Each setter writes the field and, once the
    // level is loaded (+0xA4, set by postLoadResources), regenerates everything through reload().
    void         setTypeGeometry(const vc71::string& geometry);  vc71::string getTypeGeometry() const;
    void         setTypeDensity(float value);           float getTypeDensity() const;
    void         setTypeNormalScale(float value);       float getTypeNormalScale() const;
    void         setTypeRotationScale(float value);     float getTypeRotationScale() const;
    void         setTypeMinRadiusToSame(float value);   float getTypeMinRadiusToSame() const;
    void         setTypeMinRadiusToOthers(float value); float getTypeMinRadiusToOthers() const;
    void         setTypeRandomScale(Vec2 value);        Vec2  getTypeRandomScale() const;

    // the geometry table's find-or-add: the index of the 0xA0 record whose "_lod" (+0x00)
    // or full (+0x50) name is `name`; else a new record built from `name + "_lod"` and `name`, kept
    // only when both templates gave a geometry; -1 for an empty name or a dropped record. preLoad,
    // initialize, the static-object adoption and the editor's populatePatch call it.
    int findOrAddGeometry(const vc71::string& name);


    // The editor's painting (OverGrowth.cpp). x / z / radius are world metres on the primary
    // heightmap, centred on the origin; the painters write the +0x80 material map / +0x84 shadow map
    // directly and then refresh the TerrainEditable overlay of the painted rectangle when the terrain
    // shows it (technique 5 / 6). The last argument is an 8-bit value: both shipped builds read only
    // its low byte (1.50 `cmp byte ptr` / `movzx` / `mov r8`; the 2006 editor's executor narrows the
    // console's int to a byte before the call). [INFERRED] names for the members without a console row.

    void paintMaterial(float x, float z, float radius, unsigned char material);    // 100CA130 Overgrowth.paintMaterial
    void paintShadowmap(float x, float z, float radius, unsigned char value);      // 100C9670 Overgrowth.paintShadowmap
    void exportToTerrainTexture();                                                 // 100C8F40 Overgrowth.exportToTerrainTexture
    void exportShadowmapToTerrainTexture();                                        // 100C9250 TerrainEditable technique 6
    // The rectangle exporters: x0..x1 / z0..z1 are primary-heightmap texels, divided unsigned by the
    // terrain's patch size into an inclusive patch range.
    void exportMaterialRectToTerrainTexture(unsigned int x0, unsigned int x1,
                                            unsigned int z0, unsigned int z1);
    void exportShadowmapRectToTerrainTexture(unsigned int x0, unsigned int x1,
                                             unsigned int z0, unsigned int z1);

private:
    // [UNRESOLVED] Names for the unrecovered members; constructor writes only observed fields.
    // The string at +0x88 is explicitly constructed/destroyed in its original order.
    alignas(4) unsigned char m_storage[0x150];
};
static_assert(sizeof(OverGrowth) == 0x154, "OverGrowth PE image-owned layout");

OverGrowth* createOverGrowth();
} } }
#endif
#endif
