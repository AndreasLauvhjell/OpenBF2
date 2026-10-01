// Material implementation is attested to BundledMeshTemplate.cpp (100E9520/100E9840).
// [INFERRED] this split header/TU only separates the recovered material owner.
#ifndef BF2_GEOM_BUNDLEDMESHMATERIAL_H
#define BF2_GEOM_BUNDLEDMESHMATERIAL_H
#ifdef BF2_IMAGE_RENDDX9
#include "../../RendDX9Abi/Vc71Vector.h"
#include "../../RendDX9Abi/Vc71Map.h"
#include <cstddef>
namespace dice { namespace hfe { namespace geom {
class BundledMeshTemplate;
class BundledMeshState;
// The 24-byte unsigned-key node shared by this material's declaration map and the
// template's value maps. Allocators100493C0/100EB100 prove the flags at+14/+15.
struct BundledMeshMapNode
{
    BundledMeshMapNode *left, *parent, *right;
    unsigned int key, value;
    unsigned char color, isNil, pad[2];
};
static_assert(sizeof(BundledMeshMapNode)==24, "Bundled map node");
void bundledMeshMapConstruct(vc71::map_layout& map);
BundledMeshMapNode* bundledMeshMapFind(const vc71::map_layout& map, unsigned int key);
unsigned int& bundledMeshMapAt(vc71::map_layout& map, unsigned int key);
void bundledMeshMapDestroy(vc71::map_layout& map);
BundledMeshMapNode* bundledMeshMapNext(BundledMeshMapNode* node);
class BundledMeshMaterial
{
public:
    BundledMeshMaterial(BundledMeshTemplate* templ, bool hasUv1, bool hasTangent);
    void destroyMembers();
    virtual void* destroy(unsigned char flags);
    virtual bool applyStreams(void* shader);
    virtual int getVertexCount();
    virtual int getBaseVertexIndex();
    virtual int getPrimitiveCount();
    virtual int getPrimitiveType();
    virtual int getVertexStart();
    virtual int getIndexOffset();
    virtual int getIndexStart();
    unsigned int m_dword04;                                  // +004
    BundledMeshTemplate* m_template;                          // +008
    BundledMeshState* m_state;                               // +00C
    unsigned char m_parameters[0x150];                       // +010 Ra material block
    int m_kind;                                             // +160
    unsigned int m_vertexStart;                             // +164
    unsigned int m_indexStart;                              // +168
    unsigned int m_primitiveCount;                          // +16C
    unsigned int m_vertexCount;                             // +170
    unsigned int m_primitiveType;                           // +174
    void* m_vertexBuffer;                                   // +178 native D3D handle
    unsigned int m_vertexStride;                            // +17C
    void* m_indexBuffer;                                    // +180 native D3D handle
    vc71::vector_layout m_elements;                         // +184 RaVertexElementDesc*
    vc71::map_layout m_declarations;                        // +194 shader build serial -> declaration
};
static_assert(sizeof(BundledMeshMaterial)==0x1A0, "material allocation");
static_assert(offsetof(BundledMeshMaterial,m_parameters)==0x10, "material parameters");
static_assert(offsetof(BundledMeshMaterial,m_kind)==0x160, "material kind");
static_assert(offsetof(BundledMeshMaterial,m_declarations)==0x194, "declaration map");
} } }
#endif
#endif
