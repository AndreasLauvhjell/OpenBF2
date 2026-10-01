// An odd-sided grid of heightmap tiles; sector (0,0) is the primary terrain.
// [INFERRED] header beside the ELF-attested Physics/HeightmapCluster.cpp.
// CEResourcesLoaded triggers water-mesh baking. IEventListener is a separate base;
// preserve declaration order instead of copying ELF slot numbers into MSVC calls.

#ifndef PHYSICS_HEIGHTMAPCLUSTER_H     // [INFERRED] guard style, per Core/IBase.h
#define PHYSICS_HEIGHTMAPCLUSTER_H

#include <string>

#include "../Core/IBase.h"
#include "../Game/IEventListener.h"
#include "../Math/Vec3.h"
#include "Heightmap.h"                 // IHeightmap - dispatched through, so the real declaration

namespace dice {
namespace hfe {
namespace world {

// IHeightmapCluster - ELF slots 4..44. IID_IHeightmapCluster = 820000 (Core/Ids.h).
class IHeightmapCluster : public IBase
{
public:
    // ELF slots 4/5. first thing this interface declares - the D1/D0 pair sits immediately after
    // the IBase quartet in every table that overrides it.
    virtual ~IHeightmapCluster() {}

    // Lifecycle, ELF 6-8
    virtual void  init()                                            = 0;
    virtual void  cleanup()                                         = 0;
    virtual void  reset()                                           = 0;

    // Geometry, ELF 9-13
    virtual void  setClusterSize(int size)                          = 0;
    virtual int   getClusterSize() const                            = 0;
    // setHeightmapSize takes an int and stores A float; getHeightmapSize truncates it back.
    // Reproduced as shipped - see the .cpp.
    virtual void  setHeightmapSize(int size)                        = 0;
    virtual int   getHeightmapSize() const                          = 0;
    virtual int   getWorldSize() const                              = 0;

    // Tile management, ELF 14-21
    virtual void        addHeightmap(const std::string& name, int sectorX, int sectorZ) = 0;
    virtual void        setActiveHeightmap(IHeightmap* heightmap)   = 0;
    virtual IHeightmap* getActiveHeightmap() const                  = 0;
    virtual IHeightmap* getPrimaryHeightmap() const                 = 0;
    virtual IHeightmap* getHeightmapInSector(int sectorX, int sectorZ) const = 0;
    virtual bool        getSectorOfHeightmap(const IHeightmap* heightmap,
                                             int& sectorX, int& sectorZ) const = 0;
    virtual IHeightmap* getHeightmapInWorldCoords(float x, float z) const    = 0;
    virtual void        getSectorOfWorldCoords(float x, float z,
                                               int* sectorX, int* sectorZ) const = 0;

    // Scale, ELF 22-23
    virtual const Vec3& getPrimaryScale() const                     = 0;
    virtual const Vec3& getSecondaryScale() const                   = 0;

    // Water level, ELF 24-25
    virtual void  setSeaWaterLevel(float level)                     = 0;
    virtual float getSeaWaterLevel()                                = 0;   // non-const, as shipped

    // Queries, ELF 26-40
    virtual int   getHeightFromGrid(int x, int z) const             = 0;   // shipped stub, see .cpp
    virtual float getHeightInWorldCoords(float x, float z) const    = 0;
    virtual float getHeightInWorldCoords(const Vec3& p) const       = 0;
    virtual float getWorldScaledHeightInWorldCoords(float x, float z) const = 0;
    virtual float getWorldScaledHeightInWorldCoords(const Vec3& p) const    = 0;
    virtual void  getNormalInWorldCoords(float x, float z, Vec3& normal) const = 0;
    virtual void  getNormalInWorldCoords(const Vec3& p, Vec3& normal) const    = 0;
    virtual float getHeightAndNormalInWorldCoords(float x, float z, Vec3& normal,
                                                  unsigned char& material) const = 0;
    virtual bool  getTriangleAndNormalAndMaterialInWorldCoords(
                      float x, float z, Vec3& normal, Vec3& v0, Vec3& v1,
                      Vec3& v2, unsigned char& material) const       = 0;
    virtual bool  getSquareInWorldCoords(float x, float z, Vec3* corners,
                                         bool& flipped) const        = 0;
    virtual bool  intersectRayInWorldCoords(const Vec3& from, const Vec3& to, Vec3& hitPoint,
                                            float& hitFraction, Vec3& hitNormal,
                                            unsigned char& material, bool twoSided) const = 0;
    virtual unsigned char getMaterialInWorldCoords(float x, float z) const = 0;
    virtual float getWaterLevelInWorldCoords(float x, float z) const = 0;
    virtual float getWaterLevelInWorldCoords(const Vec3& p) const    = 0;
    virtual bool  isInsideHeightmapCluster(float x, float z) const   = 0;

    // ELF 41-44
    virtual void  intersectCameraRay()                               = 0;
    virtual void  compileWaterMeshesIntoHeightmap()                  = 0;
    virtual void  setUsePhongNormalInterpolation(bool on)            = 0;
    virtual bool  getUsePhongNormalInterpolation() const             = 0;
};

// Cluster state with terrain and event-listener interface views; construction starts with one reference.
// The primary tile is sector (0,0), and cluster size is forced odd.
// m_usePhongNormalInterpolation is intentionally uninitialized until its setter is called.
class HeightmapCluster : public IHeightmapCluster, public IEventListener
{
public:
    HeightmapCluster();
    virtual ~HeightmapCluster();

    // IBase quartet, ELF slots 0-3
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;            // 0x8386EA0  - _ZNK..., const

    // IHeightmapCluster, ELF slots 6-44, in order
    virtual void  init();
    virtual void  cleanup();
    virtual void  reset();
    virtual void  setClusterSize(int size);
    virtual int   getClusterSize() const;
    virtual void  setHeightmapSize(int size);
    virtual int   getHeightmapSize() const;
    virtual int   getWorldSize() const;
    virtual void        addHeightmap(const std::string& name, int sectorX, int sectorZ);
    virtual void        setActiveHeightmap(IHeightmap* heightmap);
    virtual IHeightmap* getActiveHeightmap() const;
    virtual IHeightmap* getPrimaryHeightmap() const;
    virtual IHeightmap* getHeightmapInSector(int sectorX, int sectorZ) const;
    virtual bool        getSectorOfHeightmap(const IHeightmap* heightmap,
                                             int& sectorX, int& sectorZ) const;
    virtual IHeightmap* getHeightmapInWorldCoords(float x, float z) const;
    virtual void        getSectorOfWorldCoords(float x, float z, int* sectorX, int* sectorZ) const;
    virtual const Vec3& getPrimaryScale() const;
    virtual const Vec3& getSecondaryScale() const;
    virtual void  setSeaWaterLevel(float level);
    virtual float getSeaWaterLevel();
    virtual int   getHeightFromGrid(int x, int z) const;
    virtual float getHeightInWorldCoords(float x, float z) const;
    virtual float getHeightInWorldCoords(const Vec3& p) const;
    virtual float getWorldScaledHeightInWorldCoords(float x, float z) const;
    virtual float getWorldScaledHeightInWorldCoords(const Vec3& p) const;
    virtual void  getNormalInWorldCoords(float x, float z, Vec3& normal) const;
    virtual void  getNormalInWorldCoords(const Vec3& p, Vec3& normal) const;
    virtual float getHeightAndNormalInWorldCoords(float x, float z, Vec3& normal,
                                                  unsigned char& material) const;
    virtual bool  getTriangleAndNormalAndMaterialInWorldCoords(
                      float x, float z, Vec3& normal, Vec3& v0, Vec3& v1,
                      Vec3& v2, unsigned char& material) const;
    virtual bool  getSquareInWorldCoords(float x, float z, Vec3* corners, bool& flipped) const;
    virtual bool  intersectRayInWorldCoords(const Vec3& from, const Vec3& to, Vec3& hitPoint,
                                            float& hitFraction, Vec3& hitNormal,
                                            unsigned char& material, bool twoSided) const;
    virtual unsigned char getMaterialInWorldCoords(float x, float z) const;
    virtual float getWaterLevelInWorldCoords(float x, float z) const;
    virtual float getWaterLevelInWorldCoords(const Vec3& p) const;
    virtual bool  isInsideHeightmapCluster(float x, float z) const;
    virtual void  intersectCameraRay();
    virtual void  compileWaterMeshesIntoHeightmap();
    virtual void  setUsePhongNormalInterpolation(bool on);
    virtual bool  getUsePhongNormalInterpolation() const;

    // IEventListener, ELF slot 45 (primary) / thunk at secondary +0x00
    virtual void  handleEvent(EventCategory category, unsigned int id,
                              EventNode* node, void* userData);

    // Non-virtual. Present in the binary, reached only by direct calls.
    int  getGridIndex(int sectorX, int sectorZ) const;
    bool isSectorValid(int sectorX, int sectorZ) const;
    // [CHOSEN] Returns a pointer to the component written in to while clipping against an
    // axis plane. Parameter types are established; pointer return spelling remains provisional.
    float* clipRay(const Vec3& from, Vec3& to, float plane, int axis) const;

    // The factory ClassManager::createInstance calls. Signature fixed by ClassRegInfo::create
    // (Core/ClassManager.h) and by _ZN4dice3hfe5world16HeightmapCluster6createEjPNS0_5IBaseE.
    static IBase* create(unsigned int iid, IBase* outer);

protected:
    // HeightmapClusterEditable (derived, Physics/HeightmapClusterEditable.h) reads these directly:
    // its IBase quartet shares m_refCount, and its bodies read m_clusterSize, m_heightmaps and
    // m_primaryHeightmap (ELF this+4 based offsets). Access only - no offset moved.
    /* 0x08 */ int          m_refCount;
    /* 0x0C */ IHeightmap*  m_activeHeightmap;
    /* 0x10 */ IHeightmap*  m_primaryHeightmap;
    /* 0x14 */ int          m_clusterSize;
    /* 0x18 */ IHeightmap** m_heightmaps;
    /* 0x1C */ float        m_heightmapSize;
    /* 0x20 */ bool         m_usePhongNormalInterpolation;   // never initialised - see above
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_HEIGHTMAPCLUSTER_H
