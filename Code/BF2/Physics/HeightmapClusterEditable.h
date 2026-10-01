// Editable terrain cluster with brushes, tile stitching, simulated water and .con/.raw/.mat saving.
// Header path [INFERRED]. IHeightmapClusterEditable is the primary base; HeightmapCluster follows it.
// IBase operations use HeightmapCluster's reference count, and queryInterface must return the
// appropriate adjusted base pointer. Preserve the distinct six-argument ray overload and virtual order.
// The editor selects this class for the HeightmapCluster singleton before singleton construction.

#ifndef PHYSICS_HEIGHTMAPCLUSTEREDITABLE_H
#define PHYSICS_HEIGHTMAPCLUSTEREDITABLE_H

#include <string>

#include "../Core/IBase.h"
#include "../Math/Vec3.h"
#include "HeightmapCluster.h"

namespace dice {
namespace hfe {
namespace world {

class IHeightmap;

// IHeightmapClusterEditable - IID_IHeightmapClusterEditable = 820003 (Core/Ids.h). Constness and
// parameters from the mangled names; returns from the bodies (save and
// importSurroundingRawHeightMap return 1/0 explicitly; isDisabled/getSimulateWater the byte;
// the brushes and setters leave postEvent's handle or a tail call's residue: void).
class IHeightmapClusterEditable : public IBase
{
public:
    virtual ~IHeightmapClusterEditable() {}

    virtual bool  save(const std::string& path)                                    = 0;
    virtual void  disable(bool disabled)                                           = 0;
    virtual bool  isDisabled()                                                     = 0;
    virtual void  setSimulateWater(bool simulate)                                  = 0;
    virtual bool  getSimulateWater()                                               = 0;
    virtual void  setSimulatedWaterDepth(float depth)                              = 0;
    virtual float getSimulatedWaterDepth()                                         = 0;
    virtual float getHeightInWorldCoords(float x, float z) const                   = 0;
    virtual void  setHeightInWorldCoords(float x, float z, float height)           = 0;
    virtual void  addHeightInWorldCoords(float x, float z, float delta)            = 0;
    virtual float getWorldScaledHeightInWorldCoords(float x, float z) const        = 0;
    virtual void  setWorldScaledHeightInWorldCoords(float x, float z, float height, int flags) = 0;
    virtual void  addWorldScaledHeightInWorldCoords(float x, float z, float delta, int flags)  = 0;
    virtual bool  importSurroundingRawHeightMap(std::string fileName)              = 0;
    virtual void  modifyHeight(int x, int z, float radius, float strength, int flags)          = 0;
    virtual void  setHeight(int x, int z, float radius, float strength, float height, int flags) = 0;
    virtual void  smooth(int x, int z, float radius, float strength, int flags)                = 0;
    virtual void  random(int x, int z, float radius, float strength, int flags)                = 0;
    virtual void  smoothEdges(int width, float amplitude, float falloff)           = 0;
    virtual void  stitchEdges()                                                    = 0;
};

// Editable cluster combines its editing interface with HeightmapCluster. [PLATFORM]
// ELF places flags in base tail padding; PE appends them after the complete base.
// Do not copy member offsets between those layouts.
class HeightmapClusterEditable : public IHeightmapClusterEditable, public HeightmapCluster
{
public:
    HeightmapClusterEditable();
    virtual ~HeightmapClusterEditable();

    static IBase* create(unsigned int iid, IBase* outer);

    // IBase (HeightmapCluster's refcount)
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    // IHeightmapClusterEditable
    virtual bool  save(const std::string& path);
    virtual void  disable(bool disabled);
    virtual bool  isDisabled();
    virtual void  setSimulateWater(bool simulate);
    virtual bool  getSimulateWater();
    virtual void  setSimulatedWaterDepth(float depth);
    virtual float getSimulatedWaterDepth();
    virtual float getHeightInWorldCoords(float x, float z) const;
    virtual void  setHeightInWorldCoords(float x, float z, float height);
    virtual void  addHeightInWorldCoords(float x, float z, float delta);
    virtual float getWorldScaledHeightInWorldCoords(float x, float z) const;
    virtual void  setWorldScaledHeightInWorldCoords(float x, float z, float height, int flags);

    virtual void  addWorldScaledHeightInWorldCoords(float x, float z, float delta, int flags);

    virtual bool  importSurroundingRawHeightMap(std::string fileName);
    virtual void  modifyHeight(int x, int z, float radius, float strength, int flags);

    virtual void  setHeight(int x, int z, float radius, float strength, float height, int flags);

    virtual void  smooth(int x, int z, float radius, float strength, int flags);
    virtual void  random(int x, int z, float radius, float strength, int flags);
    virtual void  smoothEdges(int width, float amplitude, float falloff);
    virtual void  stitchEdges();

    // HeightmapCluster overrides, in the ELF'S append order (primary slots 28..33)
    virtual IHeightmap* getHeightmapInSector(int sectorX, int sectorZ) const;
    virtual IHeightmap* getHeightmapInWorldCoords(float x, float z) const;
    virtual float getHeightAndNormalInWorldCoords(float x, float z, Vec3& normal,
                                                  unsigned char& material) const;
    // NEW (six arguments, no twoSided): PE primary slot 25.
    virtual bool  intersectRayInWorldCoords(const Vec3& from, const Vec3& to, Vec3& hitPoint,
                                            float& hitFraction, Vec3& hitNormal,
                                            unsigned char& material) const;
    virtual float getWaterLevelInWorldCoords(float x, float z) const;
    virtual bool  isInsideHeightmapCluster(float x, float z) const;

    // The names the six-argument declaration above would otherwise hide.
    using HeightmapCluster::intersectRayInWorldCoords;
    using HeightmapCluster::getHeightInWorldCoords;
    using HeightmapCluster::getWorldScaledHeightInWorldCoords;
    using HeightmapCluster::getWaterLevelInWorldCoords;

    // non-virtual
    int  getUsedGridSize() const;
    void _setHeight(float x, float z, float height);

private:
    bool  m_disabled;
    bool  m_simulateWater;
    float m_simulatedWaterDepth;
};

class IHeightmapEditable;

// The editable views of the "HeightmapCluster" singleton - two named world functions of the ELF's
// PhysicsModule TU (0x88417A0 / 0x8841750), defined in Physics/HeightmapConsole.cpp. Null when there
// is no cluster or it is the game's plain one.
IHeightmapClusterEditable* getHeightmapClusterEditable();
IHeightmapEditable*        getActiveHeightmapEditable();

} // namespace world
} // namespace hfe
} // namespace dice

#endif // PHYSICS_HEIGHTMAPCLUSTEREDITABLE_H
