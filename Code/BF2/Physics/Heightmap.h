// Terrain height, material and collision queries. Header path [INFERRED].
// IHeightmap derives from IBase and declares its virtual destructor before terrain methods.
// Preserve virtual order; Itanium's two destructor entries become one under MSVC.
// Return types not established by the binary remain marked [CHOSEN, not recovered] at declarations.

#ifndef PHYSICS_HEIGHTMAP_H     // [INFERRED] guard style, per Core/IBase.h
#define PHYSICS_HEIGHTMAP_H

#include <string>
#include <vector>
#include <cstdint>

#include "../Core/IBase.h"
#include "../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace world {

class IObject;      // [UNRESOLVED] layout - only ever passed by pointer through slot 58 below

// Heightmap interface with paired local/world-coordinate queries. Cluster callers choose
// a tile in world coordinates, then use the world variants. Preserve virtual order.
class IHeightmap : public IBase
{
public:
    // ELF slots 4/5. Declared first because it is first - see the header block.
    virtual ~IHeightmap() {}                                              // Heightmap D1 0x837E040 / D0 0x837E170

    // Persistence, ELF slots 6-8
    virtual bool  loadHeightData(const std::string& name)          = 0;   // 0x837E2B0  [CHOSEN] bool
    virtual bool  saveHeightData(const std::string& name) const    = 0;   // 0x837E400  [CHOSEN] bool
    virtual bool  createEmpty(const std::string& name)             = 0;   // 0x837E410  [CHOSEN] bool

    // Dirty flag, ELF slots 9-10
    virtual bool  getModified() const                              = 0;   // 0x8384A90  `xor eax,eax` - integral
    virtual void  setModified(bool modified)                       = 0;

    // Grid geometry, ELF slots 11-18
    virtual void  setSize(int x, int z)                            = 0;
    virtual void  getSize(int& x, int& z) const                    = 0;
    virtual void  setPos(int x, int z)                             = 0;   // 0x837E4F0  addHeightmap calls this
    virtual void  getPos(int& x, int& z) const                     = 0;
    virtual float getLowBoundaryX() const                          = 0;   // 0x8384AB0  fsubp - float
    virtual float getHighBoundaryX() const                         = 0;   // 0x8384AE0  float
    virtual float getLowBoundaryZ() const                          = 0;   // 0x8384B10  float
    virtual float getHighBoundaryZ() const                         = 0;   // 0x8384B40  float

    // Resolution and scale, ELF slots 19-22
    virtual void        setBitResolution(int bits)                 = 0;
    virtual int         getBitResolution() const                   = 0;   // 0x837E540  returns [this+0x20]
    virtual void        setScale(const Vec3& scale)                = 0;
    // Returns a reference to the stored scale.
    virtual const Vec3& getScale() const                           = 0;

    // Shading and water, ELF slots 23-26
    virtual void  setUsePhongNormalInterpolation(bool on)          = 0;
    virtual bool  getUsePhongNormalInterpolation() const           = 0;
    virtual void  setSeaWaterLevel(float level)                    = 0;
    // Non-const in the original interface; preserve this signature.
    virtual float getSeaWaterLevel()                               = 0;

    // Materials, ELF slots 27-33
    // getMaterialScale exists because the material grid has a different resolution from the height
    // grid - Physics/Heightmap.cpp records this.
    virtual int   getMaterialScale() const                         = 0;   // 0x837E620  returns [this+0xB8]
    virtual void  setMaterialScale(int scale)                      = 0;
    virtual bool  loadMaterialData(const std::string& name)        = 0;   // 0x837E650  [CHOSEN] bool
    virtual bool  saveMaterialData(const std::string& name) const  = 0;   // 0x837E700  [CHOSEN] bool
    // [CHOSEN] Material ids use the unsigned-byte type shared by setters and query outputs.
    virtual unsigned char getMaterialFromGrid(int x, int z) const           = 0;
    virtual unsigned char getMaterialInWorldCoords(float x, float z) const  = 0;
    virtual void  setMaterialInGrid(int x, int z, unsigned char material)   = 0;

    // Returns an integer sample. Normal/triangle callers convert it as signed; min/max
    // logic uses unsigned comparisons. Stored grid values are unsigned 8- or 16-bit samples.
    virtual int   getHeightFromGrid(int x, int z) const                     = 0;
    virtual float getHeightInLocalCoords(float x, float z) const            = 0;
    virtual float getHeightInWorldCoords(float x, float z) const            = 0;
    virtual float getHeightInLocalCoords(const Vec3& p) const               = 0;
    virtual float getHeightInWorldCoords(const Vec3& p) const               = 0;

    // Writes the normal through the output reference; returns no query value.
    virtual void  getNormalInLocalCoords(float x, float z, Vec3& normal) const     = 0;
    virtual void  getNormalInWorldCoords(float x, float z, Vec3& normal) const     = 0;
    virtual void  getNormalInLocalCoords(const Vec3& p, Vec3& normal) const        = 0;
    virtual void  getNormalInWorldCoords(const Vec3& p, Vec3& normal) const        = 0;

    // World-scaled heights, ELF slots 43-44
    virtual float getWorldScaledHeightInWorldCoords(float x, float z) const        = 0;
    virtual float getWorldScaledHeightInWorldCoords(const Vec3& p) const           = 0;

    // Combined queries, ELF slots 45-49. The API is shaped to avoid re-walking the grid.
    virtual float getHeightAndNormalInLocalCoords(float x, float z, Vec3& normal,
                                                  unsigned char& material) const   = 0;
    virtual float getHeightAndNormalInWorldCoords(float x, float z, Vec3& normal,
                                                  unsigned char& material) const   = 0;
    // The first Vec3 is the normal. These tile queries return void; the cluster wrappers return
    // a separate bool indicating whether they found a tile (ELF 0x83857C0 / 0x8385840).
    virtual void  getTriangleAndNormalAndMaterialInWorldCoords(
                      float x, float z, Vec3& normal, Vec3& v0, Vec3& v1,
                      Vec3& v2, unsigned char& material) const                     = 0;
    virtual void  getTriangleAndNormalAndMaterialInLocalCoords(
                      float x, float z, Vec3& normal, Vec3& v0, Vec3& v1,
                      Vec3& v2, unsigned char& material) const                     = 0;
    virtual void  getSquareInWorldCoords(float x, float z, Vec3* corners,
                                         bool& flipped) const                      = 0;

    // Mutable ray queries in local or world coordinates; return whether an intersection is found.
    virtual bool  intersectRayInLocalCoords(const Vec3& from, const Vec3& to, Vec3& hitPoint,
                                            float& hitFraction, Vec3& hitNormal,
                                            unsigned char& material, bool twoSided) = 0;
    virtual bool  intersectRayInWorldCoords(const Vec3& from, const Vec3& to, Vec3& hitPoint,
                                            float& hitFraction, Vec3& hitNormal,
                                            unsigned char& material, bool twoSided) = 0;

    // Height extents, ELF slots 52-53. Non-const.
    virtual void  getMinMaxHeightInLocalCoords(float x0, float z0, float x1, float z1,
                                               float& minY, float& maxY)           = 0;
    virtual void  getMinMaxHeightInWorldCoords(float x0, float z0, float x1, float z1,
                                               float& minY, float& maxY)           = 0;

    // Water, ELF slots 54-57. Water is baked into the heightmap; see slot 58.
    virtual float getWaterLevelInLocalCoords(float x, float z) const               = 0;
    virtual float getWaterLevelInWorldCoords(float x, float z) const               = 0;
    virtual float getWaterLevelInLocalCoords(const Vec3& p) const                  = 0;
    virtual float getWaterLevelInWorldCoords(const Vec3& p) const                  = 0;

    // [CHOSEN] Appends consumed water objects for the cluster's later callbacks; returns no value.
    virtual void  compileWaterMeshesIntoHeightmap(std::vector<IObject*>& waterObjects) = 0;

    // ELF slots 59-60
    virtual bool  getUsingValidMaterials() const                   = 0;   // 0x8384780 / PE 0x6A90D0 AL
    virtual void  replaceNonMappedMaterials(int material)          = 0;   // 0x83847E0 / PE 0x6A9120: no count
};

// Center-origin terrain tile: signed grid coordinates are offset by half the dimensions.
// Supports 8/16-bit heights; other resolutions abort. Loaded 16-bit samples clamp to 0xFF00.
// Water uses a quarter-resolution grid; until populated, queries return seaWaterLevel (initially -10000).
class Heightmap : public IHeightmap
{
public:
    Heightmap();                                     // C2 0x837DC30 / C1 0x837DDA0, 364 B - raw-read
    virtual ~Heightmap();                            // ELF D1 0x837E040 / PE 0x6A8290: ordered array teardown

    static IBase* create(unsigned int iid, IBase* outer);   // Rejects aggregation; allocates and queries iid, aborting if unsupported.

    // IBase quartet, refcount +0x04
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;// 32 B - IBase / IHeightmap only

    // Persistence
    virtual bool  loadHeightData(const std::string& name);   // 325 B - the terrain parser
    virtual bool  saveHeightData(const std::string& name) const;   // 7 B - shipped stub
    virtual bool  createEmpty(const std::string& name);      // 144 B - zeroed grid, drops materials

    virtual bool  getModified() const;               // `xor eax, eax` - always false (no field)
    virtual void  setModified(bool modified);        // empty, 5 B

    // Grid geometry
    virtual void  setSize(int x, int z);             // stores size and half-size
    virtual void  getSize(int& x, int& z) const;
    virtual void  setPos(int x, int z);              // +0x10/+0x14 - the tile's world offset
    virtual void  getPos(int& x, int& z) const;
    virtual float getLowBoundaryX() const;           // (pos - half) * scale, per axis
    virtual float getHighBoundaryX() const;
    virtual float getLowBoundaryZ() const;
    virtual float getHighBoundaryZ() const;

    virtual void  setBitResolution(int bits);        // +0x20
    virtual int   getBitResolution() const;
    virtual void  setScale(const Vec3& scale);       // 123 B - also precomputes 1/scale,
                                                     //   zero-guarded per axis
    virtual const Vec3& getScale() const;

    virtual void  setUsePhongNormalInterpolation(bool on);   // 14 B - +0x44, written
    virtual bool  getUsePhongNormalInterpolation() const;    // 12 B - +0x44
    virtual void  setSeaWaterLevel(float level);     // +0x48
    virtual float getSeaWaterLevel();                // non-const, as shipped

    // Materials
    virtual int   getMaterialScale() const;          // +0xB8
    virtual void  setMaterialScale(int scale);
    virtual bool  loadMaterialData(const std::string& name);
    virtual bool  saveMaterialData(const std::string& name) const;   // 7 B - shipped stub
    virtual unsigned char getMaterialFromGrid(int x, int z) const;          // 208 B - written
    virtual unsigned char getMaterialInWorldCoords(float x, float z) const; // 97 B  recovered
    virtual void  setMaterialInGrid(int x, int z, unsigned char material);  // 89 B  recovered

    // Heights
    virtual int   getHeightFromGrid(int x, int z) const;     // 406 B - 8/16-bit arms + the fatal
    virtual float getHeightInLocalCoords(float x, float z) const;   // Interpolates four grid samples across the diagonal, scaled by scale.y.
    virtual float getHeightInWorldCoords(float x, float z) const;   // 66 B - world - pos, local
    virtual float getHeightInLocalCoords(const Vec3& p) const;
    virtual float getHeightInWorldCoords(const Vec3& p) const;

    virtual void  getNormalInLocalCoords(float x, float z, Vec3& normal) const;   // 800 B recovered
    virtual void  getNormalInWorldCoords(float x, float z, Vec3& normal) const;   // 77 B  recovered
    virtual void  getNormalInLocalCoords(const Vec3& p, Vec3& normal) const;      // 53 B  recovered
    virtual void  getNormalInWorldCoords(const Vec3& p, Vec3& normal) const;      // 75 B  recovered

    virtual float getWorldScaledHeightInWorldCoords(float x, float z) const;      // 66 B  recovered
    virtual float getWorldScaledHeightInWorldCoords(const Vec3& p) const;         // 46 B  recovered

    virtual float getHeightAndNormalInLocalCoords(float x, float z, Vec3& normal,
                                                  unsigned char& material) const; // 1,037 B - written
    virtual float getHeightAndNormalInWorldCoords(float x, float z, Vec3& normal,
                                                  unsigned char& material) const; // 84 B  - written
    // ELF 0x8384160 / PE 0x6AB250: normalize the two X blends, then the Z blend.
    void getPhongInterpolation(float x, float z, Vec3& normal) const;
    virtual void  getTriangleAndNormalAndMaterialInWorldCoords(
                      float x, float z, Vec3& normal, Vec3& v0, Vec3& v1,
                      Vec3& v2, unsigned char& material) const;
    virtual void  getTriangleAndNormalAndMaterialInLocalCoords(
                      float x, float z, Vec3& normal, Vec3& v0, Vec3& v1,
                      Vec3& v2, unsigned char& material) const;
    virtual void  getSquareInWorldCoords(float x, float z, Vec3* corners,
                                         bool& flipped) const;

    virtual bool  intersectRayInLocalCoords(const Vec3& from, const Vec3& to, Vec3& hitPoint,
                                            float& hitFraction, Vec3& hitNormal,
                                            unsigned char& material, bool twoSided);
    virtual bool  intersectRayInWorldCoords(const Vec3& from, const Vec3& to, Vec3& hitPoint,
                                            float& hitFraction, Vec3& hitNormal,
                                            unsigned char& material, bool twoSided);

    virtual void  getMinMaxHeightInLocalCoords(float x0, float z0, float x1, float z1,
                                               float& minY, float& maxY);         // 461 B recovered
    virtual void  getMinMaxHeightInWorldCoords(float x0, float z0, float x1, float z1,
                                               float& minY, float& maxY);         // 128 B recovered

    // Water
    virtual float getWaterLevelInLocalCoords(float x, float z) const;   // 115 B - quarter-res
                                                     //   grid, else m_seaWaterLevel
    virtual float getWaterLevelInWorldCoords(float x, float z) const;
    virtual float getWaterLevelInLocalCoords(const Vec3& p) const;
    virtual float getWaterLevelInWorldCoords(const Vec3& p) const;
    virtual void  compileWaterMeshesIntoHeightmap(std::vector<IObject*>& waterObjects);
    // Fills the water grid by raycasting scene objects, then builds its water quad-tree.

    virtual bool  getUsingValidMaterials() const;
    virtual void  replaceNonMappedMaterials(int material);

    // non-virtual
    int  createQuadTree(int size);                   // 696 B - the u8 max-height mip pyramid
                                                     //   loadHeightData builds
    float getWaterLevelFromGrid(int x, int z) const; // 88 B - the quarter-res sample
    // [CHOSEN, not recovered] return carrier: existing recovery retains a scratch pointer or
    // last-cell index, with no caller consuming it. intptr_t preserves that native value on x64
    // and is the original int type on MSVC x86; this does not claim an attested return type.
    std::intptr_t createWaterQuadTree();              // 757 B - the u8 max-water-height mip
                                                     //   pyramid over the quarter-res grid
    bool intersectRay(int level, int cellX, int cellZ,
                      float fromX, float fromY, float fromZ,
                      float toX, float toY, float toZ, bool twoSided);
    bool splitVerticalRay(float fromX, float fromY, float fromZ, float splitZ,
                          float& outX, float& outY, float& outZ) const;
    bool splitHorizontalRay(float fromX, float fromY, float fromZ, float splitX,
                            float& outX, float& outY, float& outZ) const;
    void internalPhongGetNormal(int x, int z, Vec3& normal) const;
    void getSquareInLocalCoords(float x, float z, Vec3* corners,
                                bool& flipped) const;

protected:
    // Editable heightmaps access the stored grid directly; the cluster needs access
    // to each tile's height data and bit resolution.
    friend class HeightmapClusterEditable;

    // Layout, 0xCC bytes, raw-read from the constructor
    /* 0x04 */ int   m_refCount;                     // ctor 1
    /* 0x08 */ std::string m_name;                   // loadHeightData/createEmpty assign it
    /* 0x0C */ void* m_heightData;                   // u8 or u16 grid, owned (new[]/delete[])
    /* 0x10 */ int   m_posX;                         // setPos - the tile's world-grid offset
    /* 0x14 */ int   m_posZ;
    /* 0x18 */ int   m_sizeX;
    /* 0x1C */ int   m_sizeZ;
    /* 0x20 */ int   m_bitResolution;                // 8 or 16; anything else is the fatal
    /* 0x24 */ int   m_halfSizeX;                    // setSize stores size/2 - center origin
    /* 0x28 */ int   m_halfSizeZ;
    /* 0x2C */ Vec3  m_scale;                        // ctor zero
    /* 0x38 */ Vec3  m_invScale;                     // setScale precomputes, zero-guarded
    /* 0x44 */ bool  m_usePhongNormalInterpolation;  // Initially false; separate from the no-op modified-flag API.
    /* 0x48 */ float m_seaWaterLevel;                // ctor -10000.0f
    /* 0x4C */ int   m_quadTreeLevels;               // createQuadTree: log2(size-1)
    /* 0x50 */ unsigned char** m_quadTree;           // one u8 max-grid per level, owned
    /* 0x54 */ int   _unresolved54[7];               // +0x54..+0x6F - ctor-zeroed [UNRESOLVED]
    /* 0x70 */ Vec3  m_rayHitPoint;                  // intersectRay primary (terrain) result
    /* 0x7C */ Vec3  m_rayHitNormal;
    /* 0x88 */ Vec3  m_rayWaterHitPoint;             // water candidate, arbitrated by local wrapper
    /* 0x94 */ Vec3  m_rayWaterHitNormal;
    /* 0xA0 */ float m_rayHitFraction;               // shipped name inferred from output role; the
                                                     //   leaf stores its signed plane distance
    /* 0xA4 */ float m_rayDzOverDx;
    /* 0xA8 */ float m_rayDxOverDz;
    /* 0xAC */ float m_rayDyOverDx;
    /* 0xB0 */ float m_rayDyOverDz;
    /* 0xB4 */ bool  m_rayZeroDeltaX;
    /* 0xB5 */ bool  m_rayZeroDeltaZ;
    /* 0xB8 */ int   m_materialScale;                // ctor 1
    /* 0xBC */ unsigned char* m_materialData;        // owned; loadHeightData lazily zero-fills
    /* 0xC0 */ unsigned char** m_waterQuadTree;      // createWaterQuadTree's
                                                     //   u8 max-water-height mip pyramid, one grid
                                                     //   per level (m_quadTreeLevels - 1 levels),
                                                     //   lazily allocated
    /* 0xC4 */ float* m_waterData;                   // quarter-res float grid, null until
                                                     //   compileWaterMeshesIntoHeightmap
    /* 0xC8 */ int   m_rayHitKind;                   // -1 terrain/no-water; 1 water candidate
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_HEIGHTMAP_H
