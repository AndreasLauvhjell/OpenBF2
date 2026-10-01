// Path: [INFERRED] header beside MeshParticleMesh.cpp, attested at RendDX9 0x10221A88.
// Mesh particle geometry, recovered from RendDX9.dll 1.50.
// Retail layout: 0x88 bytes, 64 primary slots; IScaleable is the subobject at +0x30.

#ifndef GEOM_PARTICLESYSTEM_MESHPARTICLEMESH_MESHPARTICLEMESH_H
#define GEOM_PARTICLESYSTEM_MESHPARTICLEMESH_MESHPARTICLEMESH_H

#ifdef BF2_IMAGE_RENDDX9

#include "../../GeometryInstanceData.h"
#include "../../../Math/Mat4.h"
#include "../../../Math/Vec3.h"
#include "../../../../RendDX9Abi/Vc71Vector.h"
#include <cstddef>

namespace dice {
namespace hfe {
namespace geom {

class  MeshParticleMeshTemplate;
struct MeshParticleMeshLod;

// Batches type-1 particles and queues legacy Particle/Bundle geometry for drawExtras.
// Retail vtables are adapter arrays in MeshParticleMesh.cpp; preserve all 64 primary slots.
// The template tracks live instances and clears their creator pointers during destruction.
class MeshParticleMesh : public GeometryInstanceData
{
public:
    explicit MeshParticleMesh(MeshParticleMeshTemplate* objectTemplate);
    ~MeshParticleMesh();

    // the IGeometry slots with a body of their own (the rest are shared stubs)
    int   addRef();                                          // +0x00  (StaticMesh's)
    int   getRef() const;                                    // +0x04
    int   release();                                         // +0x08 / +0x34
    void* queryInterface(unsigned int iid);                  // +0x0C
    void  setFov(float value);                               // +0x10
    float getFov() const;                                    // +0x14
    void  queueDraw(void* renderView, void* occluder, int flag);   // +0x18
    int   getGeometryType() const;                           // +0x2C
    MeshParticleMeshTemplate* getTemplate() const;           // +0x38  (shared body)
    const void* getLocalBounds() const;                      // +0x40
    float getBoundingRadius() const;                         // +0x44
    const Mat4* getGeometryTransformation() const;           // +0x48
    void  setPartTransformation(const Mat4* value, int part);// +0x4C
    void  setGeometryTransformation(const Mat4* value);      // +0x50  0x101AFC20 -> +0x4C(v, 0)
    void  setGeomIndex(int geom);                            // +0x6C
    int   getGeomIndex() const;                              // +0x70
    int   getGeomCount() const;                              // +0x74
    void  setLodIndex(int lod);                              // +0x78
    int   getLodIndex() const;                               // +0x7C
    int   getLodCount() const;                               // +0x80
    void  setDword58(unsigned int value);                    // +0xB4  (shared body)
    // The four byte slots move AL only: the getters answer the raw stored byte (upper eax bits
    // undefined in retail), the setters store the argument's LOW byte.
    unsigned char getFirstPerson() const;                    // +0xD0
    void  setFirstPerson(unsigned char value);               // +0xD4
    unsigned char getAlternateView() const;                  // +0xD8
    void  setAlternateView(unsigned char value);             // +0xDC
    void  slotF8(unsigned int a, unsigned int b, unsigned int c);   // +0xF8  0x101AFC60, empty
    void  setAlpha(float value);                             // +0xFC  (shared body)

    // IScaleable, the +0x30 subobject (0x10221B74)
    void  setScale(const Vec3& scale);                       // sub+0x10  0x101AFD40
    const Vec3& getScale() const;                            // sub+0x14  0x101AFD70

    // Draws selected-LOD materials with the requested blend mode and particle constants.
    // Alpha-mode-1 materials are skipped; transparent requests also report the unsupported path.
    void  draw(int blendMode, bool transparentPass);
    // (31 B, retn 0Ch). Writes the static AgeAndAlphaArray entry `index` (.x = age,
    // .y = alpha; .z/.w are never written). `this` is loaded by every caller and never read, and
    // `index` is unchecked. [INFERRED] names from the effect parameter "AgeAndAlphaArray".
    void  setAgeAndAlpha(float age, float alpha, int index);

    // sub_101B0070 (92 B) - the per-instance refresh MeshParticleMeshTemplate::load runs over its
    // live set: geom 1 when the template has two or more geoms, then re-point +0x44/+0x48 at the
    // current (geom, lod). Not a vtable slot. The constructor inlines the same body.
    void  refreshSelection();

    // layout
    void* const*              m_scaleableVptr;   // +0x030
    int                       m_refCount;        // +0x034
    const Mat4**              m_partTransforms;  // +0x038  26 entries
    unsigned int              m_dword3C;         // +0x03C  ctor 0 [UNRESOLVED]
    float                     m_fov;             // +0x040  ctor -1.0f
    MeshParticleMeshLod*      m_lodRecord;       // +0x044
    vc71::vector_layout*      m_lodMaterials;    // +0x048  vector<MeshParticleMeshMaterial>
    int                       m_geomIndex;       // +0x04C
    int                       m_lodIndex;        // +0x050
    unsigned char             m_lodExplicit;     // +0x054
    unsigned char             m_pad55[3];
    unsigned int              m_dword58;         // +0x058  ctor-untouched [UNRESOLVED]
    Vec4                      m_scale;           // +0x05C  ctor (1,1,1,1)
    unsigned char             m_unknown6C[16];   // +0x06C  ctor-untouched [UNRESOLVED]
    unsigned char             m_firstPerson;     // +0x07C  [INFERRED] name
    unsigned char             m_alternateView;   // +0x07D  [INFERRED] name
    unsigned char             m_pad7E[2];
    float                     m_alpha;           // +0x080  ctor 1.0f, [INFERRED] name
    MeshParticleMeshTemplate* m_template;        // +0x084
};

#if defined(_M_IX86)
#define BF2_MPM_OFFSET(member, offset) \
    static_assert(offsetof(MeshParticleMesh, member) == offset, #member)
BF2_MPM_OFFSET(m_name, 0x04);           BF2_MPM_OFFSET(m_float28, 0x28);
BF2_MPM_OFFSET(m_scaleableVptr, 0x30);  BF2_MPM_OFFSET(m_refCount, 0x34);
BF2_MPM_OFFSET(m_partTransforms, 0x38); BF2_MPM_OFFSET(m_dword3C, 0x3C);
BF2_MPM_OFFSET(m_fov, 0x40);            BF2_MPM_OFFSET(m_lodRecord, 0x44);
BF2_MPM_OFFSET(m_lodMaterials, 0x48);   BF2_MPM_OFFSET(m_geomIndex, 0x4C);
BF2_MPM_OFFSET(m_lodIndex, 0x50);       BF2_MPM_OFFSET(m_lodExplicit, 0x54);
BF2_MPM_OFFSET(m_dword58, 0x58);        BF2_MPM_OFFSET(m_scale, 0x5C);
BF2_MPM_OFFSET(m_unknown6C, 0x6C);      BF2_MPM_OFFSET(m_firstPerson, 0x7C);
BF2_MPM_OFFSET(m_alternateView, 0x7D);  BF2_MPM_OFFSET(m_alpha, 0x80);
BF2_MPM_OFFSET(m_template, 0x84);
#undef BF2_MPM_OFFSET
static_assert(sizeof(MeshParticleMesh) == 0x88,
              "MeshParticleMeshTemplate::createInstance (0x10127870) allocates 0x88");
#endif

// The retail primary and IScaleable tables as adapter arrays (MeshParticleMesh.cpp).
extern void* const g_meshParticleMeshVtable[64];
extern void* const g_meshParticleMeshScaleableVtable[6];

// 0x101B0200 on storage the template allocated; returns the object.
MeshParticleMesh* meshParticleMeshConstruct(void* storage, MeshParticleMeshTemplate* objectTemplate);
// +0x34, the scalar deleting destructor (the template's failure path calls it).
void* meshParticleMeshDestroyFlags(void* instance, unsigned char flags);
// sub_101B0070 - see refreshSelection above.
void  meshParticleMeshRefreshSelection(MeshParticleMesh* instance);

// sub_101B04C0 (84 B) - the MeshParticlesRenderer's inline push_back onto its retained pointer list
// at renderer +0x1D80 (a VC7.1 vector_layout; the renderer's constructor zeroes +0x1D84..+0x1D8C).
// The body sits in this TU's address range because MSVC instantiated the renderer's inline member
// here; IGeometry +0x18 is its only caller. `renderer` is g_ParticleSystemManager + 0x5F30.
void  meshParticlesRendererQueueMesh(unsigned char* renderer, MeshParticleMesh* mesh);

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_RENDDX9
#endif // GEOM_PARTICLESYSTEM_MESHPARTICLEMESH_MESHPARTICLEMESH_H
