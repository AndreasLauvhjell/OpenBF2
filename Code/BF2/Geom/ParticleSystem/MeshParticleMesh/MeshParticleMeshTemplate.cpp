// Mesh-particle geometry template, LOD/material loading and shared shader state.


#ifdef BF2_IMAGE_RENDDX9

#include "MeshParticleMeshTemplate.h"
#include "MeshParticleMesh.h"
#include "../../BundledMeshStateManager.h"
#include "../../../RendDX9/RendDX9Base.h"        // shaderManagerGetEffect, effectGetShaderParameterTyped
#include "../../../RendDX9/ShaderParameter.h"
#include "../../../../RendDX9Abi/HfeAbi.h"
#include "../../../../RendDX9Abi/BfMemory.h"
#include "../../../../RendDX9Abi/Frontier.h"      // logf
#include "../../../../RendDX9Abi/PluginEntry.h"   // dice::hfe::fileManager
#include "../../../../RendDX9Abi/HostFileAbi.h"   // HostFileManager / HostStream / makeHostString

#include <new>
#include <cstring>
#include <string.h>

namespace dice {
namespace hfe {

namespace dx9rend {
// dword_1023B97C. Declared per-TU exactly as SkinnedMesh.cpp and Effect.cpp declare it.
extern void* g_shaderManager;
}

namespace geom {


// The fourteen statics, ds:1025BEDC..1025BF10, in their retail order. See the header note.

MeshParticleMeshEffectState g_meshParticleMeshEffectState =
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

namespace {

const unsigned int kIID_IBase             = 1u;             // dword_102088DC
const unsigned int kIID_IGeometryTemplate = 0x17701FFFu;    // dword_1021AE5C
const unsigned int kCID_MeshParticleMesh  = 0x870CA655u;    // ds:1021AF08

// The literal behind the file-scope std::string unk_10263DD4 ("Meshes/", static init 0x101D5760),
// held as a literal for the reason MeshTemplate.cpp's kMeshesSubDirectory gives.
const char kMeshesFolder[]  = "Meshes/";
// The literal behind unk_10263DF0 ("Shaders/", static init 0x101D5740) - SkinnedMesh.cpp's reading.
const char kShadersFolder[] = "Shaders/";


// sub_10128790 - the std::set header node: VC7.1's _Buynode, which constructs the three links with
// placement NEW - and placement new tests the address it is handed. The first test is therefore the
// node itself (0x101287C9); the other two are node+4 / node+8 (0x101287E0 / 0x101287F6), which are
// never null. The colour and nil bytes are plain stores, so a failed allocation faults at [4], as in
// retail.
// Byte-identical in shape to SkinnedMeshTemplate.cpp's 0x101117A0, which is file-local there.

vc71::set_node* setConstructHead()
{
    vc71::set_node* node =
        static_cast<vc71::set_node*>(::renddx9abi::engineAlloc(sizeof(vc71::set_node)));
    ::new (static_cast<void*>(&node->left)) vc71::set_node*(0);        // [+0], tested on the node
    ::new (static_cast<void*>(&node->parent)) vc71::set_node*(0);      // [+4], tested on node+4
    ::new (static_cast<void*>(&node->right)) vc71::set_node*(0);       // [+8], tested on node+8
    node->color = 1;
    node->isNil = 0;
    return node;
}


// sub_100750C0 - `n == 0 ? true : _strnicmp(a.c_str(), b.c_str(), n) == 0`. A case-insensitive
// prefix test with the string pair in ecx/edx and the count on the stack. [INFERRED] name.

bool stringEqualsNoCase(const vc71::string& a, const vc71::string& b, unsigned int count)
{
    if (!count)
        return true;
    return ::_strnicmp(a.c_str(), b.c_str(), count) == 0;
}


// Engine-heap compatibility-vector helpers. Growth copies a prototype; shrink destroys
// nested buffers, while POD material shrink does not release state references.
// [CHOSEN, not recovered] [RETIRE-WHEN: RW-74] Replace allocation details after shared
// container behavior, layout and ownership are validated.

unsigned int vectorCount(const vc71::vector_layout& v, unsigned int elemSize)
{
    return v.first ? static_cast<unsigned int>(v.last - v.first) / elemSize : 0u;
}

void vectorFreeNestedRange(char* begin, char* end, bool twoLevels)
{
    for (char* p = begin; p != end; p += sizeof(vc71::vector_layout))
    {
        vc71::vector_layout& element = *reinterpret_cast<vc71::vector_layout*>(p);
        if (element.first)
        {
            if (twoLevels)
                vectorFreeNestedRange(element.first, element.last, false);   // sub_1012AC30's inner
            ::renddx9abi::engineFree(element.first);
        }
        element.first = 0;
        element.last  = 0;
        element.end   = 0;
    }
}

void vectorResize(vc71::vector_layout& v, unsigned int count, unsigned int elemSize,
                  const void* prototype, int nestedLevels)
{
    const unsigned int size = vectorCount(v, elemSize);
    if (count <= size)
    {
        if (count < size && v.first)
        {
            char* const newLast = v.first + elemSize * count;
            if (nestedLevels)
                vectorFreeNestedRange(newLast, v.last, nestedLevels > 1);
            v.last = newLast;
        }
        return;
    }

    const unsigned int capacity =
        v.first ? static_cast<unsigned int>(v.end - v.first) / elemSize : 0u;
    if (count > capacity)
    {
        unsigned int grown = capacity + (capacity >> 1);
        if (grown < count)
            grown = count;
        char* const buffer = static_cast<char*>(::renddx9abi::engineAlloc(elemSize * grown));
        if (size)
            std::memcpy(buffer, v.first, elemSize * size);
        if (v.first)
            ::renddx9abi::engineFree(v.first);
        v.first = buffer;
        v.last  = buffer + elemSize * size;
        v.end   = buffer + elemSize * grown;
    }

    for (unsigned int i = size; i < count; ++i)
        std::memcpy(v.first + elemSize * i, prototype, elemSize);
    v.last = v.first + elemSize * count;
}

// A zeroed vector_layout whose allocator word is left alone - what every nested-resize caller pushes
// (`mov [eax+4], ebx` / `[eax+8]` / `[eax+0Ch]`, nothing at [eax]).
const vc71::vector_layout kEmptyNestedVector = { 0, 0, 0, 0 };


// The release block inside the destructor (0x10127B8D..0x10127C98), run when the last template dies.
// Order is the shipped order: the twelve parameter slots first (ds:1025BEE4 .. 1025BF10, unused08
// included), the Effect (ds:1025BEE0) last. Every
// slot is released through its own IBase +0x08 and nulled, guarded on being non-null.

void releaseParameter(dx9rend::ShaderParameter*& slot)
{
    if (slot)
        slot->release();                                   // [reg+8]
    slot = 0;
}

void releaseSharedEffectState()
{
    MeshParticleMeshEffectState& s = g_meshParticleMeshEffectState;
    releaseParameter(s.worldViewProjection);               // ds:1025BEE4
    releaseParameter(s.unused08);                          // ds:always null in 1.50
    releaseParameter(s.color1);                            // ds:1025BEEC
    releaseParameter(s.color2);                            // ds:1025BEF0
    releaseParameter(s.colorBlendGraph);                   // ds:1025BEF4
    releaseParameter(s.fogRange);                          // ds:1025BEF8
    releaseParameter(s.fogColor);                          // ds:1025BEFC
    releaseParameter(s.effectSunColor);                    // ds:1025BF00
    releaseParameter(s.effectShadowColor);                 // ds:1025BF04
    releaseParameter(s.texture0);                          // ds:1025BF08
    releaseParameter(s.texture1);                          // ds:1025BF0C
    releaseParameter(s.oneBoneSkinning);                   // ds:1025BF10
    if (s.effect)
        dx9rend::effectRelease(s.effect);                  // ds:1025BEE0, Effect +0x08
    s.effect = 0;
}

// The one-time parameter lookups inside readMaterials, each `if (!slot) slot = sub_1001E420(effect,
// name)` - the counted getShaderParameter (addRef'd; the destructor above drops that reference).
// No null guard on the Effect, as shipped (SkyDome.cpp and SkinnedMesh.cpp record the same).
void lookupParameterOnce(dx9rend::ShaderParameter*& slot, const char* name)
{
    if (!slot)
        slot = dx9rend::effectGetShaderParameterTyped(g_meshParticleMeshEffectState.effect,
                                                      vc71::string(name));
}

} // anonymous namespace


// The factory registered under "dice.hfe.geom.GeometryTemplate.MeshParticleMesh" (getClassList
// 0x100C8049..0x100C8071, between BundledMesh and SkinnedMesh). PE 0x100B07D2.

// Allocation failure reaches an unguarded queryInterface call.

IBase* __fastcall createMeshParticleMeshTemplate(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;

    void* const storage = ::renddx9abi::engineAlloc(0x164);
    MeshParticleMeshTemplate* self = 0;
    if (storage)
        self = new (storage) MeshParticleMeshTemplate();

    IBase* const result = static_cast<IBase*>(self->queryInterface(iid));   // [eax+0Ch]
    if (!result)
        self->release();                                                      // [eax+8]
    return result;
}


// 0x10127A30, 133 bytes. The base runs first (0x101924F0 = MeshTemplate::MeshTemplate), then the two
// vptrs, then this class's stores in the order below.

// Leaves allocator/comparator storage and the 26-part array untouched.
// Increments the live-template count after clearing the load/keep flag.

MeshParticleMeshTemplate::MeshParticleMeshTemplate()
    : MeshTemplate()
{
    m_refCount = 1;                              // +0xC8

    m_materials.first = 0;                       // +0xD0 .. +0xD8
    m_materials.last  = 0;
    m_materials.end   = 0;

    // sub_10128790, then `[head+11h] = 1` and the three self links, then size = 0.
    m_instances.head = setConstructHead();
    m_instances.head->isNil  = 1;
    m_instances.head->parent = m_instances.head;
    m_instances.head->left   = m_instances.head;
    m_instances.head->right  = m_instances.head;
    m_instances.size = 0;

    m_geoms.first = 0;                           // +0xEC .. +0xF4
    m_geoms.last  = 0;
    m_geoms.end   = 0;

    m_isDecal  = 0;                              // +0x160
    m_keepData = 0;                              // +0x04
    ++g_meshParticleMeshEffectState.templateCount;   // `add dword_1025BEDC, 1`
}


// Clears instance template pointers before destroying containers. Surviving instances must not draw.
// Shared effect release occurs inside the material loop, so a last empty template leaves
// shared handles alive. Material storage must cover all declared LOD counts.

void MeshParticleMeshTemplate::destroyMembers()
{
    --g_meshParticleMeshEffectState.templateCount;   // `sub dword_1025BEDC, 1`

    const unsigned int geomCount = vectorCount(m_lodCounts, 4);
    for (unsigned int geom = 0; geom < geomCount; ++geom)
    {
        const unsigned int lodCount = reinterpret_cast<const unsigned int*>(m_lodCounts.first)[geom];
        for (unsigned int lod = 0; lod < lodCount; ++lod)
        {
            // `mov eax,[this+0D0h]; mov esi,[eax+geom*16+4]; add esi, lod*16` - re-read every lod.
            vc71::vector_layout* const lodMaterials =
                reinterpret_cast<vc71::vector_layout*>(
                    reinterpret_cast<vc71::vector_layout*>(m_materials.first)[geom].first) + lod;
            if (!lodMaterials->first)
                continue;

            // Signed division by 24 (`imul 2AAAAAABh; sar edx,2` plus the sign fix-up).
            const int materialCount =
                static_cast<int>(lodMaterials->last - lodMaterials->first) / 24;
            for (int m = 0; m < materialCount; ++m)
            {
                // `mov eax,[esi+4]` - `first` is re-read on every material.
                MeshParticleMeshMaterial* const record =
                    reinterpret_cast<MeshParticleMeshMaterial*>(lodMaterials->first) + m;
                if (record->state)
                    record->state->release();                 // [edx+8]

                if (g_meshParticleMeshEffectState.templateCount == 0)
                    releaseSharedEffectState();
            }
        }
    }

    // Detach every live instance: `mov [key+84h], 0` along the in-order walk.
    for (vc71::set_node* node = m_instances.head->left;
         node != m_instances.head;
         node = vc71::set_layout::next(node))
    {
        reinterpret_cast<MeshParticleMesh*>(node->key)->m_template = 0;
    }

    // +0xE8: free each geom's lod buffer, then the outer buffer.
    if (m_geoms.first)
    {
        vectorFreeNestedRange(m_geoms.first, m_geoms.last, false);
        ::renddx9abi::engineFree(m_geoms.first);
    }
    m_geoms.first = 0;
    m_geoms.last  = 0;
    m_geoms.end   = 0;

    // +0xDC: erase(begin, end) (sub_10128FA0's whole-tree arm), free the head, clear head and size.
    // The comparator word is left alone, as it was at construction.
    m_instances.clear();
    ::renddx9abi::engineFree(m_instances.head);
    m_instances.head = 0;
    m_instances.size = 0;

    // +0xCC: sub_1012AC30 over [first, last) - every geom's lod vectors' buffers, then each geom's
    // buffer - then the outer buffer.
    if (m_materials.first)
    {
        vectorFreeNestedRange(m_materials.first, m_materials.last, true);
        ::renddx9abi::engineFree(m_materials.first);
    }
    m_materials.first = 0;
    m_materials.last  = 0;
    m_materials.end   = 0;

    MeshTemplate::destroyMembers();
}

// +0xB0, the scalar deleting destructor: the body above, then the engine free when bit 0
// of the flag is set. Returns this.
void* MeshParticleMeshTemplate::destroy(unsigned char flags)
{
    destroyMembers();
    if (flags & 1)
        ::renddx9abi::engineFree(this);
    return this;
}


// The overridden IBase trio - 0x100EEA10 / 0x100EEA20 / 0x100EEA30, one body serving four template
// classes. The PE addRef returns the new count in eax; IBase::addRef is void in this tree, which
// is the sibling classes' declared shape too.

void MeshParticleMeshTemplate::addRef()
{
    ++m_refCount;
}

int MeshParticleMeshTemplate::getRef()
{
    return m_refCount;
}

int MeshParticleMeshTemplate::release()
{
    const int remaining = --m_refCount;
    if (!remaining)
        destroy(1);                                      // [eax+0B0h]
    return remaining;
}

// shared with GeometryTemplate.RoadCompiled. `this` for IBase (1) or IGeometryTemplate
// (0x17701FFF), zero otherwise. Unlike MeshTemplate's 0x100EE940 it does NOT hand out the
// io::IConsoleSaveable subobject for 0x3709A4FF. The `setnz / sub 1 / and` idiom is a branch-free
// select of `this` or zero; it has no null guard to reproduce.
void* MeshParticleMeshTemplate::queryInterface(unsigned int iid) const
{
    if (iid == kIID_IBase || iid == kIID_IGeometryTemplate)
        return const_cast<MeshParticleMeshTemplate*>(this);
    return 0;
}

// +0x10, 0x1012B0F0 - `mov eax, ds:1021AF08; retn`.
unsigned int MeshParticleMeshTemplate::getClassId() const
{
    return kCID_MeshParticleMesh;
}


// Names starting with "decal_" set the decal flag; renaming never clears it.

void MeshParticleMeshTemplate::setName(const vc71::string& name)
{
    m_name = name;                                                   // +0x14, MSVCP71 operator=
    const bool decal = stringEqualsNoCase(m_name, vc71::string("decal_"), 6);
    if (decal)
        m_isDecal = 1;
}


// Creates an instance and loads the template if needed, registering it before load refreshes.
// Failure removes and destroys the instance; its destructor tolerates the repeated removal.

void* MeshParticleMeshTemplate::createInstance()
{
    void* const storage = ::renddx9abi::engineAlloc(0x88);
    MeshParticleMesh* const instance =
        storage ? meshParticleMeshConstruct(storage, this) : 0;

    if (m_loaded)                                                    // +0x4C
    {
        m_keepData = 1;                                              // +0x04
        return instance;
    }

    m_instances.insert(reinterpret_cast<unsigned int>(instance));
    if (m_loaded
        || dx9rend::resourceManagerIsReady()                         // sub_1000D950(ds:1023B988)
        || load())                                                   // [edx+2Ch]
    {
        m_keepData = 1;
        return instance;
    }

    vc71::setEraseValue(m_instances, instance);
    if (instance)
        meshParticleMeshDestroyFlags(instance, 1);                   // [edx+34h] (1)
    return 0;
}


// Loads a .bundledMesh, preferring a GPU variant when available after the plain-path check.
// Parse failure keeps loaded clear for retry; success marks it before updating live instances.

bool MeshParticleMeshTemplate::load()
{
    if (m_loaded)                                                    // +0x4C
        return false;

    vc71::string path(m_path);                                       // var_58
    path += kMeshesFolder;
    path += m_name;
    path += ".bundledMesh";

    vc71::string gpuPath(m_path);                                    // var_74
    gpuPath += kMeshesFolder;
    gpuPath += "gpu_";
    gpuPath += m_name;
    gpuPath += ".bundledMesh";

    ::renddx9abi::HostFileManager* const files = ::dice::hfe::fileManager;   // dword_10244AC4

    if (!files->fileExists(::renddx9abi::makeHostString(path)))      // [edx+1Ch]
    {
        ::renddx9abi::logf("MeshParticleMeshTemplate.cpp:185: Cannot load BundledMesh %s"
                           " Please make sure it exists!", path.c_str());
        return false;
    }

    const bool gpuExists = files->fileExists(::renddx9abi::makeHostString(gpuPath));   // [edx+1Ch]
    ::renddx9abi::HostStream* const stream = files->open(
        ::renddx9abi::makeHostString(gpuExists ? gpuPath : path),
        static_cast< ::renddx9abi::HostFileMode>(2),
        static_cast< ::renddx9abi::HostFileAccess>(0), 0);           // [edx+18h] (path, 2, 0, 0)
    if (!stream)
        return false;

    const bool parsed = loadFromStream(
        static_cast< ::renddx9abi::HostNonlinearStream*>(stream));   // [edx+0B4h]
    stream->release();                                               // [eax+8]
    if (!parsed)
        return false;

    m_loaded = 1;                                                    // +0x4C, before the walk

    // `mov ecx,[node+0Ch]; call sub_101B0070` then the iterator ++ (sub_1009F6E0).
    for (vc71::set_node* node = m_instances.head->left;
         node != m_instances.head;
         node = vc71::set_layout::next(node))
    {
        meshParticleMeshRefreshSelection(reinterpret_cast<MeshParticleMesh*>(node->key));
    }
    return true;
}


// Reads every LOD without skipping or distance filtering.
// [CHOSEN, not recovered] Prototype padding and fields overwritten by parsing are zeroed.

void MeshParticleMeshTemplate::readLods(::renddx9abi::HostNonlinearStream* stream)
{
    const unsigned int geomCount = vectorCount(m_lodCounts, 4);      // var_10

    vectorResize(m_geoms, geomCount, sizeof(vc71::vector_layout),
                 &kEmptyNestedVector, 1);

    for (unsigned int geom = 0; geom < geomCount; ++geom)
    {
        const unsigned int lodCount = reinterpret_cast<const unsigned int*>(m_lodCounts.first)[geom];

        MeshParticleMeshLod prototype;
        std::memset(&prototype, 0, sizeof(prototype));
        prototype.bounds.dirty = 1;

        vc71::vector_layout& lods =
            reinterpret_cast<vc71::vector_layout*>(m_geoms.first)[geom];
        vectorResize(lods, lodCount, sizeof(MeshParticleMeshLod), &prototype, 0);

        for (unsigned int lod = 0; lod < lodCount; ++lod)
        {
            // `mov eax,[this+0ECh]; add eax, geom*16; mov esi,[eax+4]; add esi, lod*72` - re-read.
            MeshParticleMeshLod* const record =
                reinterpret_cast<MeshParticleMeshLod*>(
                    reinterpret_cast<vc71::vector_layout*>(m_geoms.first)[geom].first) + lod;

            // The version gate every mesh template shares: head 0 and version < 10 is the old
            // 36-byte bounds record. (ecx = bounds, edx = stream - the tree's two readers.)
            if (m_head == 0 && m_version < 10u)
                meshBoundsReadLegacy(record->bounds, stream);
            else
                meshBoundsReadMinMax(record->bounds, stream);

            stream->read(&record->partCount, 4);                     // [edx+10h] (lod+0, 4)
        }
    }
}


// Reads materials using the shared MeshParticleMesh effect, ignoring file effect names.
// Requires a BundledMesh state manager and effect-name lengths of at least three.
// Unlike other mesh templates, this path does not swap mip classes or build a technique table.

void MeshParticleMeshTemplate::readMaterials(::renddx9abi::HostNonlinearStream* stream)
{
    MeshParticleMeshEffectState& shared = g_meshParticleMeshEffectState;
    const unsigned int geomCount = vectorCount(m_lodCounts, 4);      // var_10

    vectorResize(m_materials, geomCount, sizeof(vc71::vector_layout),
                 &kEmptyNestedVector, 2);

    for (unsigned int geom = 0; geom < geomCount; ++geom)            // var_C / var_4 = 16*geom
    {
        const unsigned int lodCount = reinterpret_cast<const unsigned int*>(m_lodCounts.first)[geom];

        vc71::vector_layout& geomMaterials =
            reinterpret_cast<vc71::vector_layout*>(m_materials.first)[geom];
        vectorResize(geomMaterials, lodCount, sizeof(vc71::vector_layout),
                     &kEmptyNestedVector, 1);

        for (unsigned int lod = 0; lod < lodCount; ++lod)            // var_24 counts down
        {
            // Both trees are re-read from `this` for every lod.
            vc71::vector_layout* const lodMaterials =
                reinterpret_cast<vc71::vector_layout*>(
                    reinterpret_cast<vc71::vector_layout*>(m_materials.first)[geom].first) + lod;

            unsigned int materialCount = 0;                          // var_1C
            stream->read(&materialCount, 4);

            // An all-zero 24-byte prototype - `[edx] .. [edx+14h]` all written, this one complete.
            const MeshParticleMeshMaterial prototype = { 0, 0, 0, 0, 0, 0 };
            vectorResize(*lodMaterials, materialCount, sizeof(MeshParticleMeshMaterial),
                         &prototype, 0);

            MeshParticleMeshLod* const lodRecord =
                reinterpret_cast<MeshParticleMeshLod*>(
                    reinterpret_cast<vc71::vector_layout*>(m_geoms.first)[geom].first) + lod;
            lodRecord->renderKind   = 3;                             // lod +0x40
            lodRecord->hasAlphaTest = 0;                             // lod +0x44 (byte)

            for (unsigned int m = 0; m < materialCount; ++m)         // var_14
            {
                // `mov edx,[ebx+4]; lea ecx,[m+m*2]; lea ebx,[edx+ecx*8]` - first re-read per material.
                MeshParticleMeshMaterial* const record =
                    reinterpret_cast<MeshParticleMeshMaterial*>(lodMaterials->first) + m;
                std::memset(record, 0, sizeof(*record));             // the six zero stores

                if (m_version >= 5u)                                 // +0xA8
                {
                    stream->read(&record->alphaMode, 4);
                }
                else
                {
                    // `mov [ebx], 0; jnz; mov [ebx], eax` with eax == the material index: the second
                    // material of an old file is transparent. BundledMeshTemplate's `i == 1 ? 1 : 0`.
                    record->alphaMode = 0;
                    if (m == 1)
                        record->alphaMode = static_cast<int>(m);
                }

                const int alphaMode = record->alphaMode;
                if (alphaMode == 1)
                    lodRecord->renderKind = 2;
                else if (alphaMode == 2)
                    lodRecord->hasAlphaTest = 1;

                unsigned int fxNameLength = 0;                       // var_8
                stream->read(&fxNameLength, 4);

                if (!shared.effect)                                  // ds:1025BEE0
                {
                    if (fxNameLength)
                    {
                        char* const fxName =
                            static_cast<char*>(::renddx9abi::engineAlloc(fxNameLength + 1));
                        stream->read(fxName, fxNameLength);
                        fxName[fxNameLength - 3] = 0;                // [len - 3], see the header note
                        ::renddx9abi::engineFree(fxName);
                    }
                    vc71::string effectName(kShadersFolder);         // *unk_10263DF0
                    effectName += "MeshParticleMesh";
                    shared.effect = static_cast<dx9rend::Effect*>(
                        dx9rend::shaderManagerGetEffect(dx9rend::g_shaderManager,
                                                        effectName.c_str()));
                }
                else
                {
                    stream->skip(fxNameLength);                      // [eax+18h]
                }

                if (!shared.worldViewProjection)
                {
                    shared.worldViewProjection = dx9rend::effectGetShaderParameterTyped(
                        shared.effect, vc71::string("WorldViewProjection"));
                    if (!shared.worldViewProjection)
                    {
                        // Debug type 6, MeshParticleMeshTemplate.cpp:318 (0x13E). Unreachable in
                        // practice: sub_1001E420 addRefs its result unguarded before returning.
                        ::renddx9abi::logf("MeshParticleMeshTemplate.cpp:318: Could not load "
                                           "WorldViewProjection parameter from shader");
                    }
                }
                lookupParameterOnce(shared.color1,            "COLOR1");
                lookupParameterOnce(shared.color2,            "COLOR2");
                lookupParameterOnce(shared.colorBlendGraph,   "COLORBLENDGRAPH");
                lookupParameterOnce(shared.fogRange,          "fogRange");
                lookupParameterOnce(shared.fogColor,          "fogColor");
                lookupParameterOnce(shared.effectSunColor,    "EffectSunColor");
                lookupParameterOnce(shared.effectShadowColor, "EffectShadowColor");
                lookupParameterOnce(shared.texture0,          "Texture0");
                lookupParameterOnce(shared.texture1,          "Texture1");
                lookupParameterOnce(shared.oneBoneSkinning,   "matONEBONESKINNING");

                // sub_101935E0(effect, &m_path, stream, 1, 0) on dword_102597F4 - unguarded.
                record->state = g_bundledMeshStateManager->read(
                    shared.effect, m_path, stream, true, false);

                stream->read(&record->vertexStart, 4);               // record +0x08
                stream->read(&record->indexStart, 4);                // record +0x0C

                unsigned int indexCount = 0;                         // var_20
                stream->read(&indexCount, 4);
                // derived, and only for two primitive types; anything else leaves the zero.
                if (m_primitiveType == 4u)                           // D3DPT_TRIANGLELIST
                    record->primitiveCount = indexCount / 3u;        // unsigned (`mul 0AAAAAAABh`)
                else if (m_primitiveType == 5u)                      // D3DPT_TRIANGLESTRIP
                    record->primitiveCount = indexCount - 2u;

                stream->read(&record->vertexCount, 4);               // record +0x14
                stream->skip(4);
                stream->skip(4);
            }
        }
    }
}

// +0xF0, 0x1012B100, 20 bytes: `mov [ecx+edx*4+0F8h], eax; retn 8`. No bounds check on index.
void MeshParticleMeshTemplate::setDwordF8(int index, unsigned int value)
{
    m_dwordF8[index] = value;
}

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_RENDDX9
