// Skinned-mesh render-path state. Registration and queryInterface both use IID_IBase.
// The primary and secondary vtables, with the listener at +0x04, are ABI boundaries.

#ifdef BF2_IMAGE_RENDDX9

#include "../../RendDX9Abi/HfeAbi.h"
#include "../../RendDX9Abi/ModuleCommon.h"
#include "../../RendDX9Abi/BfMemory.h"
#include "../../RendDX9Abi/Frontier.h"
#include "../../RendDX9Abi/Vc71String.h"
#include "../../RendDX9Abi/Vc71Map.h"

#include <new>
#include <string.h>

namespace dice {
namespace hfe {
namespace geom {

// getClassList: sub_100518A0(821015, "dice.hfe.geom.SkinnedMeshRenderer", factory). getSingletonList
// { "SkinnedMeshRenderer", &g_SkinnedMeshRenderer, iid 1, classId 821015, flags 2 }.
extern const unsigned int CID_SkinnedMeshRenderer  = 821015u;
extern const unsigned int IID_ISkinnedMeshRenderer = 1u;        // IID_IBase - the record really uses 1

static const unsigned int kSkinnedMeshRendererSize = 0x44u;     // factory alloc, 0x100B06D0

// [CHOSEN] header over the 68-byte object; every offset is named because the class is fully recovered.
struct SkinnedMeshRenderer
{
    const void**  m_vptr;          // +0x00  primary vtable (ABI)
    const void**  m_listenerVptr;  // +0x04  RenderPathListener subobject vtable (ABI)
    void*         m_renderPath;    // +0x08  set/read by the listener slots
    int           m_refCount;      // +0x0C
    vc71::map_layout m_map1;       // +0x10
    vc71::map_layout m_map2;       // +0x1C
    vc71::map_layout m_map3;       // +0x28
    unsigned char m_bool;          // +0x34  primary slots 4/5
    unsigned char m_pad35[3];
    int           m_dword38;       // +0x38
    int           m_dword3C;       // +0x3C  [UNRESOLVED] - ctor does not write it
    int           m_dword40;       // +0x40
};

namespace {

// primary vtable (0x1021C928, 6 slots) - refcount at +0x0C
int __fastcall skm_addRef(void* self, void*) { return ++static_cast<SkinnedMeshRenderer*>(self)->m_refCount; }
int __fastcall skm_getRc (void* self, void*) { return   static_cast<SkinnedMeshRenderer*>(self)->m_refCount; }
int __fastcall skm_release(void* self, void*);
// queryInterface (0x10025930): iid 1 only.
void* __fastcall skm_qi(void* self, void*, unsigned int iid) { return (iid == RD9_IID_IBASE) ? self : 0; }
// +0x10 setBool([+0x34]), +0x14 getBool.
void __fastcall skm_setBool(void* self, void*, int v) { static_cast<SkinnedMeshRenderer*>(self)->m_bool = static_cast<unsigned char>(v); }
int  __fastcall skm_getBool(void* self, void*)        { return static_cast<SkinnedMeshRenderer*>(self)->m_bool; }

// secondary vtable (0x1021C910, RenderPathListener, 6 slots). `self` is the subobject pointer at
// object+0x04, so the object is `self - 4` and the render-path back-pointer is object+0x08.
inline SkinnedMeshRenderer* objOfListener(void* self)
{
    return reinterpret_cast<SkinnedMeshRenderer*>(reinterpret_cast<char*>(self) - 4);
}
void  __fastcall skm_setRenderPath(void* self, void*, void* path) { objOfListener(self)->m_renderPath = path; }
void* __fastcall skm_getRenderPath(void* self, void*)             { return objOfListener(self)->m_renderPath; }
void  __fastcall skm_lisNull1(void*, void*)      { }   // +0x04 nullsub_15
void  __fastcall skm_lisNull2(void*, void*, int) { }   // +0x08 nullsub_53
// +0x0C: the shipped "This path is invalid" error emitter (SkinnedMeshRenderer.cpp:29). Reproduced as
// a one-line diagnostic rather than the full dice error machinery.
void __fastcall skm_lisPathInvalid(void*, void*, void*)
{
    ::renddx9abi::logf("SkinnedMeshRenderer.cpp:29: This path is invalid");
}
// +0x14 getRenderPathListenerName -> returns "SkinnedMeshRenderer" by value (hidden out-pointer).
void* __fastcall skm_lisGetName(void*, void*, void* out)
{
    new (out) vc71::string("SkinnedMeshRenderer");
    return out;
}

const void* const kPrimaryVtable[6] =
{
    reinterpret_cast<void*>(&skm_addRef),         // +0x00
    reinterpret_cast<void*>(&skm_getRc),          // +0x04
    reinterpret_cast<void*>(&skm_release),        // +0x08
    reinterpret_cast<void*>(&skm_qi),             // +0x0C
    reinterpret_cast<void*>(&skm_setBool),        // +0x10
    reinterpret_cast<void*>(&skm_getBool),        // +0x14
};

const void* const kListenerVtable[6] =
{
    reinterpret_cast<void*>(&skm_setRenderPath),  // +0x00
    reinterpret_cast<void*>(&skm_lisNull1),       // +0x04  nullsub
    reinterpret_cast<void*>(&skm_lisNull2),       // +0x08  nullsub
    reinterpret_cast<void*>(&skm_lisPathInvalid), // +0x0C  "This path is invalid"
    reinterpret_cast<void*>(&skm_getRenderPath),  // +0x10  getRenderPath (inherited shape)
    reinterpret_cast<void*>(&skm_lisGetName),     // +0x14  getName
};

int __fastcall skm_release(void* self, void*)
{
    SkinnedMeshRenderer* p = static_cast<SkinnedMeshRenderer*>(self);
    const int remaining = --p->m_refCount;
    if (remaining == 0)
    {
        // Match the shipped dtor's observable teardown: free the three map head-nodes, then the object.
        vc71::map_node* h;
        h = p->m_map3.head; if (h) ::renddx9abi::engineFree(h);
        h = p->m_map2.head; if (h) ::renddx9abi::engineFree(h);
        h = p->m_map1.head; if (h) ::renddx9abi::engineFree(h);
        ::renddx9abi::engineFree(self);
    }
    return remaining;
}

} // anonymous namespace

// geom_SkinnedMeshRenderer_ctor, 0x10123B80. Faithful except the render-path registration.
// [FRONTIER] dice::hfe::dx9rend::RendDX9_registerRenderPathListener - the listener-list layout is
// unread (same frontier RoadCompiledRenderer defers). The object is fully built; it is simply not
// yet spliced into the render-path list, so its listener slots are not driven by that list.
namespace {
void constructSkinnedMeshRenderer(SkinnedMeshRenderer* self)
{
    self->m_vptr         = const_cast<const void**>(kPrimaryVtable);
    self->m_listenerVptr = const_cast<const void**>(kListenerVtable);
    self->m_renderPath   = 0;
    self->m_refCount     = 1;
    self->m_map1.construct(0x18);   // sub_10123C60 - 0x18-byte RB head nodes
    self->m_map2.construct(0x18);
    self->m_map3.construct(0x18);
    self->m_bool = 1;
    self->m_pad35[0] = 0; self->m_pad35[1] = 0; self->m_pad35[2] = 0;
    self->m_dword38 = 0;
    self->m_dword3C = 0;
    self->m_dword40 = 0;
}
} // anonymous namespace

// RefFactory_geom_SkinnedMeshRenderer, 0x100B06D0.
IBase* __fastcall createSkinnedMeshRenderer(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;
    void* storage = ::renddx9abi::engineAlloc(kSkinnedMeshRendererSize);
    if (!storage)
        return 0;
    ::memset(storage, 0, kSkinnedMeshRendererSize);
    SkinnedMeshRenderer* self = static_cast<SkinnedMeshRenderer*>(storage);
    constructSkinnedMeshRenderer(self);

    void* itf = skm_qi(self, 0, iid);
    if (!itf)
        skm_release(self, 0);
    return static_cast<IBase*>(itf);
}

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_RENDDX9
