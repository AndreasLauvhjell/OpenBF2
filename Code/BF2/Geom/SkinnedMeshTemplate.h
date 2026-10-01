// Path: [INFERRED] header beside SkinnedMeshTemplate.cpp, attested at RendDX9 0x1021BF40.
// Soldier mesh template, recovered from RendDX9.dll 1.50 (PE only).
// Retail layout: 0x10C bytes; constructor 0x101106B0, primary table 0x1021C160.
// Override positions come from the vtable, including shared and empty bodies.

#ifndef GEOM_SKINNEDMESHTEMPLATE_H     // [INFERRED] guard style, matching StaticMeshTemplate.h
#define GEOM_SKINNEDMESHTEMPLATE_H

#ifdef BF2_IMAGE_RENDDX9

#include "MeshTemplate.h"
#include "../../RendDX9Abi/Vc71Vector.h"
#include "../../RendDX9Abi/Vc71Map.h"

namespace dice {
namespace hfe {
namespace geom {

// sub_10112090 through the template's +0x100 set - erase one live instance. Published because
// `SkinnedMesh::~SkinnedMesh` is what calls it; see the definition's note for why the shipped
// inlined `equal_range` + range-erase and this `find` + `erase(node)` are the same operation.
void instanceSetErase(vc71::set_layout& set, void* instance);

// Soldier mesh loader with derived reference counting at +0xC8.
// readTrailer is intentionally empty: reading the base trailer would shift the stream.
// Preserve untouched vector allocator words and the unresolved +0xED..+0xFB bytes.
class SkinnedMeshTemplate : public MeshTemplate
{
public:
    SkinnedMeshTemplate();

    // The overridden quartet counts at +0xC8 - the same three bodies StaticMeshTemplate uses,
    // shared in the image (0x100EEA10 / 0x100EEA20 / 0x100EEA30) with its own queryInterface.
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    // Returns the skinned-template class ID used by shadow selection and effect lookup.
    virtual unsigned int getClassId() const;                        // +0x10
    virtual void* createInstance();                                 // +0x24
    virtual bool  load();                                           // +0x2C
    virtual void  setLodDistance(int geom, int lod, float distance);// +0x30
    virtual float getLodDistance(int geom, int lod);                // +0x34
    virtual unsigned int getMemoryUsage(int mask);                  // +0x54
    virtual int   getPrimitiveCount(int geom, int lod, int material);// +0x58

    // Skinned meshes omit the base trailer dword. LODs include bounds, bone matrices
    // and fallback distances; materials initialize technique masks and shared shadow state.
    virtual void  readTrailer(::renddx9abi::HostNonlinearStream* stream);

    virtual void  readLods(::renddx9abi::HostNonlinearStream* stream);

    // Skinned-template overrides used by geometry queries and the load pipeline.
    virtual int   getVertexCount(int geom, int lod, int material);   // +0x5C
    virtual int   getMaterialCount(unsigned int lod);                // +0xA0
    virtual int   getLodPrimitiveCount(unsigned int lod);            // +0xA4
    virtual void  mergeMaterials();                                  // +0xC4
    virtual void  finalizeMaterials();                               // +0xD0

    virtual void  readMaterials(::renddx9abi::HostNonlinearStream* stream);

    // data, in shipped order
    int                 m_refCount;          // +0xC8
    vc71::vector_layout m_vector0CC;          // +0xCC  [UNRESOLVED] element type
    vc71::vector_layout m_vector0DC;          // +0xDC  [UNRESOLVED] element type
    unsigned char       m_byte0EC;            // +0xEC
    unsigned char       m_unknown0ED[15];     // +0xED..+0xFB  [UNRESOLVED], ctor-untouched
    unsigned char       m_byte0FC;            // +0xFC
    unsigned char       m_pad0FD[3];          // +0xFD..+0xFF
    vc71::set_layout    m_instances;          // +0x100  SkinnedMesh*
};

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_RENDDX9
#endif // GEOM_SKINNEDMESHTEMPLATE_H
