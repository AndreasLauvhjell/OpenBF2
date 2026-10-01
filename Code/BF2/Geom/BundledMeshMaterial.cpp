// Material ctor/apply/dtor are attested to Geom/BundledMeshTemplate.cpp.
// [INFERRED] split TU, kept beside its recovered owner. BF2 1.50 PE only.
#ifdef BF2_IMAGE_RENDDX9
#include "BundledMeshMaterial.h"
#include "BundledMeshTemplate.h"
#include "../RendDX9/Ra/RaShader.h"
#include "../../RendDX9Abi/EngineFile.h"
#include "../../RendDX9Abi/Frontier.h"
#include <cstdlib>
#include <new>

namespace dice { namespace hfe { namespace geom {
namespace {
namespace Ra = ::dice::hfe::dx9rend::Ra;
void destroyMapNodes(BundledMeshMapNode* node)
{
    while (!node->isNil)
    {
        destroyMapNodes(node->right);
        BundledMeshMapNode* const left = node->left;
        ::renddx9abi::engineFree(node);
        node = left;
    }
}
void pushDescriptor(vc71::vector_layout& v, const Ra::RaVertexElementDesc* value)
{
    vc71::vectorRef<const Ra::RaVertexElementDesc*>(&v).push_back(value);
}

// off_101E2428 is the real abstract material base: one deleting destructor and
// eight _purecall slots, read fresh alongside100ED400. These are retail pure slots.
extern "C" int __cdecl _purecall(void);
void* __fastcall destroyedMaterialDestroy(void* self, void*, unsigned char flags);
void* const kDestroyedMaterialTable[] = {
    reinterpret_cast<void*>(&destroyedMaterialDestroy),
    reinterpret_cast<void*>(&_purecall), reinterpret_cast<void*>(&_purecall),
    reinterpret_cast<void*>(&_purecall), reinterpret_cast<void*>(&_purecall),
    reinterpret_cast<void*>(&_purecall), reinterpret_cast<void*>(&_purecall),
    reinterpret_cast<void*>(&_purecall), reinterpret_cast<void*>(&_purecall)
};
void* __fastcall destroyedMaterialDestroy(void* self, void*, unsigned char flags)
{
    *static_cast<void* const**>(self) = kDestroyedMaterialTable;
    if (flags & 1) ::renddx9abi::engineFree(self);
    return self;
}
}

// The container operations preserve source-level map semantics and the proven
// 24-byte ABI. Tree balancing is the existing RW-74 chosen-container policy.
void bundledMeshMapConstruct(vc71::map_layout& map)
{
    BundledMeshMapNode* n = static_cast<BundledMeshMapNode*>(::renddx9abi::engineAlloc(24));
    if (n) n->left = 0;
    if (reinterpret_cast<unsigned int>(n) != 0xFFFFFFFCu) n->parent = 0;
    if (reinterpret_cast<unsigned int>(n) != 0xFFFFFFF8u) n->right = 0;
    n->color = 1;
    n->isNil = 0;
    map.head = reinterpret_cast<vc71::map_node*>(n);
    n->isNil = 1;
    n->left = n->parent = n->right = n;
    map.size = 0;
}
BundledMeshMapNode* bundledMeshMapFind(const vc71::map_layout& map, unsigned int key)
{
    BundledMeshMapNode* const head = reinterpret_cast<BundledMeshMapNode*>(map.head);
    BundledMeshMapNode* best = head;
    for (BundledMeshMapNode* n = head->parent; !n->isNil; )
    {
        if (n->key >= key) { best = n; n = n->left; }
        else n = n->right;
    }
    return best == head || key < best->key ? head : best;
}
unsigned int& bundledMeshMapAt(vc71::map_layout& map, unsigned int key)
{
    BundledMeshMapNode* const head = reinterpret_cast<BundledMeshMapNode*>(map.head);
    BundledMeshMapNode* found = bundledMeshMapFind(map, key);
    if (found != head) return found->value;
    BundledMeshMapNode* parent = head;
    BundledMeshMapNode* cursor = head->parent;
    bool left = true;
    while (!cursor->isNil)
    {
        parent = cursor;
        left = key < cursor->key;
        cursor = left ? cursor->left : cursor->right;
    }
    BundledMeshMapNode* n = static_cast<BundledMeshMapNode*>(::renddx9abi::engineAlloc(24));
    n->left = n->right = head;
    n->parent = parent;
    n->key = key;
    n->value = 0;
    n->color = 0;
    n->isNil = 0;
    if (parent == head) head->left = head->right = head->parent = n;
    else if (left)
    {
        parent->left = n;
        if (parent == head->left) head->left = n;
    }
    else
    {
        parent->right = n;
        if (parent == head->right) head->right = n;
    }
    ++map.size;
    return n->value;
}
void bundledMeshMapDestroy(vc71::map_layout& map)
{
    BundledMeshMapNode* head = reinterpret_cast<BundledMeshMapNode*>(map.head);
    destroyMapNodes(head->parent);
    ::renddx9abi::engineFree(head);
    map.head = 0;
    map.size = 0;
}
BundledMeshMapNode* bundledMeshMapNext(BundledMeshMapNode* n)
{
    if (!n->isNil)
    {
        if (!n->right->isNil)
        {
            n = n->right;
            while (!n->left->isNil) n = n->left;
        }
        else
        {
            BundledMeshMapNode* parent = n->parent;
            while (!parent->isNil && n == parent->right) { n = parent; parent = parent->parent; }
            n = parent;
        }
    }
    return n;
}

//Descriptor selection is ordered, and the two template virtual queries
// are unconditional. Missing tangent support logs the shipped diagnostic.
BundledMeshMaterial::BundledMeshMaterial(BundledMeshTemplate* templ, bool hasUv1, bool hasTangent)
{
    m_dword04 = 0;
    m_template = templ;
    m_state = 0;
    Ra::raMaterialParametersConstruct(m_parameters);
    m_kind = 0;
    m_vertexStart = m_indexStart = m_primitiveCount = m_vertexCount = m_primitiveType = 0;
    m_vertexBuffer = m_indexBuffer = 0;
    m_vertexStride = 0;
    m_elements.first = m_elements.last = m_elements.end = 0;
    bundledMeshMapConstruct(m_declarations);
    const bool compressed = m_template->getCompressVertexData();
    const bool normal16 = m_template->slot_0x84();
    pushDescriptor(m_elements, compressed ? &Ra::g_desc_1023ED9C : &Ra::g_desc_1023EF04);
    pushDescriptor(m_elements, compressed ? (normal16 ? &Ra::g_desc_1023EF54 : &Ra::g_desc_1023EE14)
                                         : &Ra::g_desc_1023EF7C);
    pushDescriptor(m_elements, &Ra::g_desc_1023EEDC);
    pushDescriptor(m_elements, compressed ? &Ra::g_desc_1023EDEC : &Ra::g_desc_1023ED4C);
    if (hasUv1) pushDescriptor(m_elements, compressed ? &Ra::g_desc_1023EE8C : &Ra::g_desc_1023F01C);
    if (hasTangent)
        pushDescriptor(m_elements, compressed ? (normal16 ? &Ra::g_desc_1023EDC4 : &Ra::g_desc_1023EEB4)
                                             : &Ra::g_desc_1023EFF4);
    else
        ::renddx9abi::logf("BundledMeshTemplate.cpp:111: tl: Material created _without_ tangent support. "
                          "Something must be wrong! (because we haven't implemented this yet!!)");
}

//100ED400, then100ED3D0's deleting flag. The state is released by the template,
// before invoking this body; the material destructor itself does not release it.
void BundledMeshMaterial::destroyMembers()
{
    bundledMeshMapDestroy(m_declarations);
    if (m_elements.first) ::renddx9abi::engineFree(m_elements.first);
    m_elements.first = m_elements.last = m_elements.end = 0;
    Ra::raMaterialParametersDestroy(m_parameters);
    *reinterpret_cast<void* const**>(this) = kDestroyedMaterialTable;
}
void* BundledMeshMaterial::destroy(unsigned char flags)
{
    destroyMembers();
    if (flags & 1) ::renddx9abi::engineFree(this);
    return this;
}

//starvation refuses before inserting. Bind order is stream, declaration,
// indices. Declarations belong to the process-wide cache and are not released here.
bool BundledMeshMaterial::applyStreams(void* shaderState)
{
    Ra::RaShader* const shader = static_cast<Ra::RaShader*>(shaderState);
    const unsigned int key = static_cast<unsigned int>(shader->m_buildSerial);
    BundledMeshMapNode* node = bundledMeshMapFind(m_declarations, key);
    if (node == reinterpret_cast<BundledMeshMapNode*>(m_declarations.head))
    {
        Ra::RaStreamRequirements* required = Ra::raStreamRequirementsCreate(shader->m_requiredVertexElements);
        Ra::RaDeclarationBuilder builder;
        Ra::raDeclarationBuilderConstruct(builder);
        if (Ra::raStreamRequirementsConsume(required, m_elements))
            Ra::raDeclarationBuilderAddStream(builder, m_elements);
        if (Ra::raStreamRequirementsStarving(required))
        {
            char missing[1024];
            Ra::raStreamRequirementsDescribe(required, missing, sizeof(missing));
            ::renddx9abi::logf("BundledMeshTemplate.cpp:137: Starving streams in the shader %s, %s",
                              shader->m_name.c_str(), missing);
            Ra::raDeclarationBuilderDestroy(builder);
            Ra::raStreamRequirementsDestroy(required);
            return false;
        }
        bundledMeshMapAt(m_declarations, key) = reinterpret_cast<unsigned int>(
            Ra::raVertexDeclarationCached(Ra::raDeclarationBuilderFinish(builder), Ra::raRendererDevice()));
        node = bundledMeshMapFind(m_declarations, key);
        Ra::raDeclarationBuilderDestroy(builder);
        Ra::raStreamRequirementsDestroy(required);
    }
    dx9rend::stateManagerSetStreamSource(0, static_cast<IDirect3DVertexBuffer9*>(m_vertexBuffer), 0, m_vertexStride);
    dx9rend::stateManagerSetVertexDeclaration(reinterpret_cast<IDirect3DVertexDeclaration9*>(node->value));
    dx9rend::stateManagerSetIndices(static_cast<IDirect3DIndexBuffer9*>(m_indexBuffer));
    return true;
}
int BundledMeshMaterial::getVertexCount() { return m_vertexCount; }
int BundledMeshMaterial::getBaseVertexIndex() { return -1; }
int BundledMeshMaterial::getPrimitiveCount() { return m_primitiveCount; }
int BundledMeshMaterial::getPrimitiveType() { return m_primitiveType; }
int BundledMeshMaterial::getVertexStart() { return m_vertexStart; }
int BundledMeshMaterial::getIndexOffset() { return 0; }
int BundledMeshMaterial::getIndexStart() { return m_indexStart; }

} } }
#endif
