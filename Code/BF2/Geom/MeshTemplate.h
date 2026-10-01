// Path: [INFERRED] header beside MeshTemplate.cpp, attested at RendDX9 0x10220A20.
// Shared mesh loading base, recovered from RendDX9.dll 1.50 (PE only).
// Retail layout: 0xC8 bytes; constructor 0x101924F0, primary table 0x10220A88.
// Embedded vectors/maps retain VC7.1 layouts and untouched allocator words.

#ifndef BF2_GEOM_MESHTEMPLATE_H
#define BF2_GEOM_MESHTEMPLATE_H

#include "IGeometryTemplate.h"

#include "../../RendDX9Abi/HfeAbi.h"
#include "../../RendDX9Abi/HostFileAbi.h"
#include "../../RendDX9Abi/Vc71String.h"
#include "../../RendDX9Abi/Vc71Vector.h"
#include "../../RendDX9Abi/Vc71Map.h"
#include "../Core/IBase.h"
#include "../IO/Console/IConsoleSaveable.h"
#include "../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace geom {


// Cached mesh bounds. Expansion marks them dirty; finalization computes centre, radii and extent.
// Copies preserve named fields while leaving padding untouched.

struct MeshBounds
{
    Vec3          min;            // +0x00
    Vec3          max;            // +0x0C
    float         originRadius;   // +0x18  sqrt(max over the 8 corners of |corner|^2)
    float         radius;         // +0x1C  sqrt(max over the 8 corners of |corner - center|^2)
    unsigned char dirty;          // +0x20  set by expand()/read, cleared by finalize()
    unsigned char pad21[3];       // +0x21  the old 36-byte reader overwrites these too
    Vec3          extent;         // +0x24  max - min
    Vec3          center;         // +0x30  (min + max) * 0.5
};

static_assert(sizeof(MeshBounds) == 0x3C, "MeshBounds is 60 bytes: the copy at 0x100AE1F0 ends at +0x38");

void meshBoundsExpand(MeshBounds& bounds, const Vec3& point);
void meshBoundsFinalize(MeshBounds& bounds);
bool meshBoundsReadLegacy(MeshBounds& bounds, renddx9abi::HostStream* stream);
bool meshBoundsReadMinMax(MeshBounds& bounds, renddx9abi::HostStream* stream);
void meshBoundsCopy(MeshBounds& dst, const MeshBounds& src);


// The 12-byte GeometryTemplate prefix that sits between the primary vptr and the console-saveable
// vptr. [INFERRED] as a class of its own from the layout; the PE never shows its vtable because
// MeshTemplate's constructor overwrites the slot before anything observes it.
class GeometryTemplate : public IGeometryTemplate
{
public:
    unsigned char m_keepData;   // +0x04
    unsigned char m_pad05[3];
    int           m_dword08;    // +0x08  ctor 0 [UNRESOLVED]
};

static_assert(sizeof(GeometryTemplate) == 0x0C, "GeometryTemplate prefix is 12 bytes");

// Shared mesh readers and resources; IConsoleSaveable must remain the second base at +0x0C.
// The packed material map uses 24-byte nodes, not Vc71Map.h's string-keyed nodes.
// Slot +0xAC preloads resources and stamps the frame; it does not release them.
class MeshTemplate : public GeometryTemplate, public ::dice::hfe::io::IConsoleSaveable
{
public:
    MeshTemplate();
    void destroyMembers();                                          // sub_101925C0 (the dtor body)

    // IBase quartet - the base counts at +0x10. StaticMeshTemplate overrides all four.
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();                                        // 0x100E2800 -> destroy(1)
    virtual void* queryInterface(unsigned int iid) const;

    virtual unsigned int getClassId() const;
    virtual void setName(const vc71::string& name);
    virtual const vc71::string& getName() const;
    virtual void setPath(const vc71::string& path);
    virtual const vc71::string& getPath() const;
    virtual void* createInstance();
    virtual bool isLoaded() const;
    virtual bool load();
    virtual void setLodDistance(int geom, int lod, float distance); // nullsub_33
    virtual float getLodDistance(int geom, int lod);
    virtual void setMaterialValue(int geom, int lod, unsigned char material, float value); // nullsub_32
    virtual float getMaterialValue(int geom, int lod, unsigned char material);
    virtual void slot_0x40(int a, int b, int c, int d);             // nullsub_32
    virtual float slot_0x44(int a, int b, int c);
    virtual void slot_0x48(int a);                                  // nullsub_53
    virtual bool slot_0x4C();
    virtual bool isWater() const;
    virtual unsigned int getMemoryUsage(int mask);
    virtual int getPrimitiveCount(int geom, int lod, int material);
    virtual int getVertexCount(int geom, int lod, int material);
    virtual void setDoNotGenerateLightmaps(bool value);             // nullsub_53
    virtual bool getDoNotGenerateLightmaps() const;
    virtual void setLightmapOverride(bool value);                   // nullsub_53
    virtual bool getLightmapOverride() const;
    virtual void setLightmapSize(int a, int b, int size);           // nullsub_33
    virtual int getLightmapSize(int a, int b);
    virtual void setCompressVertexData(bool value);                 // nullsub_53
    virtual bool getCompressVertexData() const;
    virtual void slot_0x80(int a);                                  // nullsub_53
    virtual bool slot_0x84();
    virtual void setMaxTextureRepeat(int value);                    // nullsub_53
    virtual int getMaxTextureRepeat() const;
    virtual void slot_0x90(int a);                                  // nullsub_53
    virtual bool slot_0x94();
    virtual void slot_0x98(int a);                                  // nullsub_53
    virtual bool slot_0x9C();
    virtual int getMaterialCount(unsigned int lod);
    virtual int getLodPrimitiveCount(unsigned int lod);
    virtual Vec3* decompressPosition(Vec3* out, const Vec3* in);
    virtual void releaseDeviceResources();

    // MeshTemplate-specific virtuals follow the 44-slot geometry-template interface.
    virtual void* destroy(unsigned char flags);
    virtual bool loadFromStream(renddx9abi::HostNonlinearStream* stream);
    virtual bool readHeader(renddx9abi::HostNonlinearStream* stream);
    virtual void readGeometry(renddx9abi::HostNonlinearStream* stream);
    virtual void readVertices(renddx9abi::HostNonlinearStream* stream);
    virtual void mergeMaterials();                                  // nullsub_91
    virtual void buildVertexBuffers();
    virtual void buildIndexBuffer();
    virtual void finalizeMaterials();                               // nullsub_15
    virtual void readTrailer(renddx9abi::HostNonlinearStream* stream);
    virtual void readLods(renddx9abi::HostNonlinearStream* stream);       // nullsub_92
    virtual void readMaterials(renddx9abi::HostNonlinearStream* stream);  // nullsub_93
    virtual void setMaxSkip3pLods(int value);
    virtual int getMaxSkip3pLods() const;
    virtual void setMaxSkipWreckLods(int value);
    virtual int getMaxSkipWreckLods() const;

    // io::IConsoleSaveable (secondary table off_10220A74): the quartet thunks to the primary,
    // then makeScript = 0x10191D70.
    virtual bool makeScript(::dice::hfe::io::IStream* stream);

    // data, in shipped order
    int                 m_refCount;          // +0x10
    vc71::string        m_name;              // +0x14
    vc71::string        m_path;              // +0x30
    unsigned char       m_loaded;            // +0x4C
    unsigned char       m_pad4D[3];
    vc71::vector_layout m_lodCounts;         // +0x50  vector<int>
    vc71::vector_layout m_lodDistances;      // +0x60  vector<vector<float>> (inner = vector_layout)
    vc71::vector_layout m_geomInts;          // +0x70  vector<int>
    vc71::map_layout    m_materialValues;    // +0x80  map<unsigned, float>
    int                 m_dword8C;           // +0x8C
    void*               m_vertexData;        // +0x90
    void*               m_vertexBuffer;      // +0x94
    unsigned short*     m_indexData;         // +0x98
    void*               m_indexBuffer;       // +0x9C
    void*               m_vertexDeclaration; // +0xA0
    unsigned int        m_head;              // +0xA4
    unsigned int        m_version;           // +0xA8
    unsigned int        m_primitiveType;     // +0xAC
    unsigned int        m_vertexStride;      // +0xB0
    unsigned int        m_vertexCount;       // +0xB4
    unsigned int        m_indexCount;        // +0xB8
    unsigned int        m_u2;                // +0xBC
    int                 m_maxSkip3pLods;     // +0xC0
    int                 m_maxSkipWreckLods;  // +0xC4
};

static_assert(sizeof(MeshTemplate) == 0xC8, "MeshTemplate is 0xC8 bytes; StaticMeshTemplate's refcount follows at +0xC8");

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_GEOM_MESHTEMPLATE_H
