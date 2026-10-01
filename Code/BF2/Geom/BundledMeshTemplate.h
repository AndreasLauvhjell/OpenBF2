// Implementation path attested by BF2 1.50 RendDX9.dll string 0x1021A3C8.
// PE-only renderer class: factory100B05C8, ctor100EA460, table1021A760.
#ifndef BF2_GEOM_BUNDLEDMESHTEMPLATE_H
#define BF2_GEOM_BUNDLEDMESHTEMPLATE_H
#ifdef BF2_IMAGE_RENDDX9

#include "MeshTemplate.h"
#include <cstddef>

namespace dice { namespace hfe {
namespace dx9rend { namespace Ra { struct RaShader; } }
namespace geom {
class BundledMeshMaterial;

// readLods100E9F50 / instance101049B0 prove the 72/32-byte strides.
struct BundledMeshLod
{
    unsigned int partCount;              // +00
    MeshBounds bounds;                   // +04
    int renderKind;                      // +40, readMaterials defaults3, alpha changes2
    unsigned char hasAlphaTest;          // +44
    unsigned char pad45[3];
};
struct BundledMeshGeom
{
    vc71::vector_layout lods;            // BundledMeshLod
    vc71::vector_layout distances;       // float
};
static_assert(sizeof(BundledMeshLod)==72, "Bundled lod stride");
static_assert(sizeof(BundledMeshGeom)==32, "Bundled geom stride");

class BundledMeshTemplate : public MeshTemplate
{
public:
    BundledMeshTemplate();
    void destroyMembers();
    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual unsigned int getClassId() const;
    virtual void setName(const vc71::string& name);
    virtual void* createInstance();
    virtual bool load();
    virtual void setLodDistance(int geom, int lod, float distance);
    virtual float getLodDistance(int geom, int lod);
    virtual void slot_0x40(int geom, int lod, int material, int valueBits);
    virtual float slot_0x44(int geom, int lod, int material);
    virtual void slot_0x48(int value);
    virtual bool slot_0x4C();
    virtual unsigned int getMemoryUsage(int mask);
    virtual int getPrimitiveCount(int geom, int lod, int material);
    virtual int getVertexCount(int geom, int lod, int material);
    virtual void setCompressVertexData(bool value);
    virtual bool getCompressVertexData() const;
    virtual void slot_0x80(int value);
    virtual bool slot_0x84();
    virtual void setMaxTextureRepeat(int value);
    virtual int getMaxTextureRepeat() const;
    virtual void slot_0x90(int value);
    virtual bool slot_0x94();
    virtual void slot_0x98(int value);
    virtual bool slot_0x9C();
    virtual int getMaterialCount(unsigned int lod);
    virtual int getLodPrimitiveCount(unsigned int lod);
    virtual Vec3* decompressPosition(Vec3* out, const Vec3* in);
    virtual void releaseDeviceResources();
    virtual void* destroy(unsigned char flags);
    virtual void readVertices(renddx9abi::HostNonlinearStream* stream);
    virtual void mergeMaterials();
    virtual void buildVertexBuffers();
    virtual void finalizeMaterials();
    virtual void readLods(renddx9abi::HostNonlinearStream* stream);
    virtual void readMaterials(renddx9abi::HostNonlinearStream* stream);
    virtual void setSpecularStaticGloss(int geom, int lod, unsigned char material, float value); // +F0,100E91C0
    virtual float getSpecularStaticGloss(int geom, int lod, unsigned char material); // +F4,100E9240
    virtual void setMaterialValue100(int geom, int lod, unsigned char material, int value); // +F8,100E92A0
    virtual int getMaterialValue100(int geom, int lod, unsigned char material); // +FC,100E9320
    virtual bool makeScript(::dice::hfe::io::IStream* stream);

    int m_refCount;                       // +0C8
    vc71::vector_layout m_materials;       // +0CC geom -> lod -> BundledMeshMaterial*
    vc71::set_layout m_instances;          // +0DC
    vc71::map_layout m_materialValues0E8;  // +0E8 packed geom/lod/material -> float
    vc71::map_layout m_map0F4;             // +0F4 packed key -> static gloss float
    vc71::map_layout m_map100;             // +100 packed key -> int, draw material+94
    unsigned int m_dword10C;              // +10C
    vc71::vector_layout m_geoms;           // +110 BundledMeshGeom
    unsigned int m_maxPartCount;          // +120
    unsigned char m_hasOpaqueMaterials;   // +124 readMaterials kind!=1
    unsigned char m_byte125;              // +125 slots48/4C
    unsigned char m_pad126[2];
    MeshBounds* m_partBounds[26];          // +128
    unsigned char m_hasEnvironmentMap;    // +190
    unsigned char m_compressVertexData;   // +191
    unsigned char m_byte192;              // +192 slots80/84
    unsigned char m_pad193;
    int m_maxTextureRepeat;               // +194
    unsigned char m_byte198;              // +198 slots90/94
    unsigned char m_byte199;              // +199 slots98/9C
    unsigned char m_pad19A[2];
    MeshBounds m_bounds;                  // +19C
    Vec3 m_compressScale;                 // +1D8
    Vec3 m_decompressScale;               // +1E4
    float m_uvDecompress;                 // +1F0
    unsigned int m_dword1F4;              // +1F4 ctor untouched
    unsigned int m_liveInstanceCount;     // +1F8
};
static_assert(sizeof(BundledMeshTemplate)==0x1FC, "factory allocation");
static_assert(offsetof(BundledMeshTemplate,m_materials)==0xCC, "material tree");
static_assert(offsetof(BundledMeshTemplate,m_geoms)==0x110, "geometry tree");
static_assert(offsetof(BundledMeshTemplate,m_bounds)==0x19C, "template bounds");
static_assert(offsetof(BundledMeshTemplate,m_liveInstanceCount)==0x1F8, "instance count");

void bundledMeshInitTechniques();
dx9rend::Ra::RaShader* bundledMeshShaderForBits(unsigned int bits);
unsigned int bundledMeshQualityMask();                      // draw's unchecked quality indexing
unsigned int bundledMeshQualityMask(unsigned int index);
IBase* __fastcall createBundledMeshTemplate(unsigned int iid, IBase* outer);

// Instance-owned bodies, published by BundledMesh.cpp.
void* bundledMeshConstruct(void* storage, BundledMeshTemplate* templ);
void bundledMeshInitTemplate(void* instance);
void* bundledMeshDestroyFlags(void* instance, unsigned char flags);

} } }
#endif
#endif
