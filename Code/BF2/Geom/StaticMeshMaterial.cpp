// Static-mesh material lifecycle, stream construction and material-list operations.
// [CHOSEN] split from StaticMeshTemplate.cpp.
// Preserve the shared nine-slot material interface. Destruction omits the final abstract-base
// vptr store because the corresponding base type is not represented here.


#ifdef BF2_IMAGE_RENDDX9

#include "../../RendDX9Abi/ModuleCommon.h"
#include "../../RendDX9Abi/BfMemory.h"
#include "../../RendDX9Abi/Frontier.h"
#include "../../RendDX9Abi/HostFileAbi.h"
#include "../../RendDX9Abi/Vc71String.h"
#include "../../RendDX9Abi/Vc71Vector.h"
#include "../RendDX9/Texture.h"
#include "../RendDX9/Ra/RaShader.h"   // the twenty vertex-element descriptors
#include "StaticMeshTemplate.h"
#include "../Game/Main/IVideoOptions.h"   // setupTechnique reads getQuality(2)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>

#include <math.h>
#include <new>
#include <set>
#include <string.h>

namespace dice {
namespace hfe {

namespace dx9rend {
// BufferManager.cpp - declared here the way MeshTemplate.cpp declares the same three entry points.
extern void* g_bufferManager;                                              // dword_1023B978
void* createVertexBuffer(void* bufferManager, unsigned int count, unsigned int stride,
                         const vc71::string& name, unsigned int usage);
void* vertexBufferLock(void* wrapper, unsigned int offsetVerts, unsigned int countVerts,
                       unsigned int flags);
bool  vertexBufferUnlock(void* wrapper);
void  vertexDeclarationRelease(void* wrapper);                             // the 0x10014790 dance
// RendDX9Init.cpp - dword_1023B980, the DeviceCaps record. buildVertexBuffers needs the feature
// mask at +0xEC and there is no typed accessor for it, so it is read by documented raw offset the
// same way deviceCapsPixelShaderMajor reads +0xCC.
extern void* g_deviceCaps;
// BF2Render.cpp - the two typed Renderer seams StaticMeshMaterial::setupTechnique's
// lightmap gate needs (+0x74 then VideoOptions +0x04, and +0xAC). Declared here exactly
// the way Geom/XpLightManager.cpp declares the same pair.
IVideoOptions* rendererGetVideoOptions();
bool rendererIsExpansionPack();
// BufferManager.cpp - the two stream binds applyStreams makes, declared here the way SkyDome.cpp
// declares them: the wrapper's buffer at offset 0 with its stride (state manager +0x58), the
// index buffer (+0x54).
void vertexBufferBindStream(void* wrapper, unsigned int stream);
void indexBufferBind(void* wrapper);
} // namespace dx9rend

namespace geom {


// dword_1025B590 / dword_1025B594[] - two file-scope counters this translation unit owns.

//   dword_1025B590  finalizeMaterials accumulates its per-template stream-bytes figure into it.
//                   Read back at 0x10118DE7 (the `+=`) and nowhere else in the image.
//   dword_1025B594  mergeMaterials increments [lod] once per merged pair. write-only: the single
//                   cross-reference in the whole image is the `++` at 0x10119CE9. [INFERRED] extent
//                   of five dwords, bounded by the next named datum at 0x1025B5A8.

namespace {
unsigned int g_staticMeshStreamBytes = 0;          // dword_1025B590
unsigned int g_staticMeshMergedCount[5] = { 0, 0, 0, 0, 0 };   // dword_1025B594
} // anonymous namespace


// Material parameter storage has a fixed 0x150-byte layout.
// Holder data is available, but dispatch through the unpublished holder classes aborts.
// [RETIRE-WHEN: the Ra material/parameter translation unit is recovered.]
// Replace the diagnostic tables after holder signatures and behavior are implemented and validated.

namespace {

void __fastcall raParameterHolderFrontier(void* self, void*)
{
    RD9_FRONTIER_AT("dice::hfe::dx9rend::Ra parameter holder "
                    "(off_101E1480 / 148C / 149C / 14A8 / 14BC / 1E78 / 1E80 / 2210 - "
                    "the eight unrecovered holder classes of the 0x150-byte material "
                    "parameter block)",
                    self, 0x00);
}

// Eight slots: longer than any of the shipped tables (the longest observed is five), so a dispatch
// through any of them lands on a body that names itself and stops rather than running off the end.
void* const kRaParameterHolderVtable[8] =
{
    reinterpret_cast<void*>(&raParameterHolderFrontier),
    reinterpret_cast<void*>(&raParameterHolderFrontier),
    reinterpret_cast<void*>(&raParameterHolderFrontier),
    reinterpret_cast<void*>(&raParameterHolderFrontier),
    reinterpret_cast<void*>(&raParameterHolderFrontier),
    reinterpret_cast<void*>(&raParameterHolderFrontier),
    reinterpret_cast<void*>(&raParameterHolderFrontier),
    reinterpret_cast<void*>(&raParameterHolderFrontier)
};

void* const* raHolderVtable() { return kRaParameterHolderVtable; }

struct RaParamBool    { void* vptr; unsigned char value; unsigned char pad[3]; };
struct RaParamFloat   { void* vptr; float value; };
struct RaParamGloss   { void* vptr; float value; };
struct RaParamInt     { void* vptr; int   value; };
struct RaParamTexture { void* vptr; void* value; };
struct RaParamVec4    { void* vptr; float value[4]; };
struct RaParamVec3    { void* vptr; float value[3]; };

// Parameter IDs are the byte offsets recorded on each member.
struct RaMaterialParameters
{
    void*         vptr;                    // +0x000  off_101E2210
    const void*   descriptor;              // +0x004  &unk_1023E7B0  [UNRESOLVED] - a .data? record
    RaParamBool   alphaBlendEnable;        // +0x008  "AlphaBlendEnable"        8
    RaParamBool   depthWrite;              // +0x010  "DepthWrite"             16
    RaParamBool   alphaTest;               // +0x018  "AlphaTest"              24
    RaParamVec4   diffuseColor;            // +0x020  "DiffuseColor"           32
    RaParamVec4   ambient;                 // +0x034  "Ambient"                52
    RaParamVec4   pointColor;              // +0x048  "PointColor"             72
    RaParamInt    pointSun;                // +0x05C  "PointSun"               92
    RaParamFloat  disableLightmapShadows;  // +0x064  "DisableLightmapShadows" 100
    RaParamVec4   specularColor;           // +0x06C  "SpecularColor"          108
    RaParamFloat  specularPower;           // +0x080  "SpecularPower"          128
    RaParamFloat  opacity;                 // +0x088  "Opacity"                136
    RaParamFloat  reflectivity;            // +0x090  "Reflectivity"           144
    RaParamInt    cullMode;                // +0x098  "CullMode"               152
    RaParamInt    alphaTestRef;            // +0x0A0  "AlphaTestRef"           160
    RaParamInt    srcBlend;                // +0x0A8  "SrcBlend"               168
    RaParamInt    destBlend;               // +0x0B0  "DestBlend"              176
    RaParamTexture diffuseMap;             // +0x0B8  "DiffuseMap"             184
    RaParamTexture detailMap;              // +0x0C0  "DetailMap"              192
    RaParamTexture dirtMap;                // +0x0C8  "DirtMap"                200
    RaParamTexture crackMap;               // +0x0D0  "CrackMap"               208
    RaParamTexture normalMap;              // +0x0D8  "NormalMap"              216
    RaParamTexture crackNormalMap;         // +0x0E0  "CrackNormalMap"         224
    RaParamTexture giMap;                  // +0x0E8  "GIMap"                  232
    RaParamGloss  staticGloss;             // +0x0F0  "StaticGloss"            240
    RaParamFloat  objRadius;               // +0x0F8  "ObjRadius"              248
    RaParamFloat  invHemiHeightScale;      // +0x100  "InvHemiHeightScale"     256
    RaParamVec4   parallaxScaleBias;       // +0x108  "ParallaxScaleBias"      264
    RaParamTexture cubeMap;                // +0x11C  "CubeMap"                284
    RaParamVec4   posUnpack;               // +0x124  "PosUnpack"              292
    RaParamFloat  texUnpack;               // +0x138  "TexUnpack"              312
    RaParamVec3   normalUnpack;            // +0x140  "NormalUnpack"           320
};

#if defined(_M_IX86)
static_assert(sizeof(RaMaterialParameters) == 0x150,
              "the block spans StaticMeshMaterial +0x2C .. +0x17B; the constructor's last store is "
              "index 83 (+0x14C)");
#endif

// Each parameter block stores its own offset-vector address as its descriptor.
// Apply resolves registry indices through this table before accessing holder data.
extern vc71::vector_layout g_raParameterOffsets;               // defined a few lines below
const void* const kRaMaterialDescriptor = &g_raParameterOffsets;


// Process-wide parameter names and matching offsets, initialized while the offset vector is empty.

vc71::vector_layout g_raParameterNames   = { 0, 0, 0, 0 };   // dword_1023E7C0
vc71::vector_layout g_raParameterOffsets = { 0, 0, 0, 0 };   // dword_1023E7B4..BC

struct RaParameterName { const char* name; int offset; };

const RaParameterName kRaParameterNames[31] =
{
    { "AlphaBlendEnable",        8 }, { "SrcBlend",             168 },
    { "DestBlend",             176 }, { "DepthWrite",            16 },
    { "CullMode",              152 }, { "AlphaTest",             24 },
    { "AlphaTestRef",          160 }, { "DiffuseColor",          32 },
    { "Ambient",                52 }, { "PointColor",            72 },
    { "PointSun",               92 }, { "DisableLightmapShadows", 100 },
    { "SpecularColor",         108 }, { "SpecularPower",        128 },
    { "Opacity",               136 }, { "Reflectivity",         144 },
    { "DiffuseMap",            184 }, { "DetailMap",            192 },
    { "NormalMap",             216 }, { "CubeMap",              284 },
    { "DirtMap",               200 }, { "CrackMap",             208 },
    { "CrackNormalMap",        224 }, { "ParallaxScaleBias",    264 },
    { "GIMap",                 232 }, { "StaticGloss",          240 },
    { "ObjRadius",             248 }, { "InvHemiHeightScale",   256 },
    { "PosUnpack",             292 }, { "TexUnpack",            312 },
    { "NormalUnpack",          320 }
};

void raRegistryPush(vc71::vector_layout& v, const void* element, unsigned int stride,
                    void (*copyConstruct)(void* dst, const void* src))
{
    const unsigned int size  = v.first ? static_cast<unsigned int>((v.last - v.first) / stride) : 0u;
    const unsigned int capac = v.first ? static_cast<unsigned int>((v.end  - v.first) / stride) : 0u;
    if (!v.first || size >= capac)
    {
        unsigned int grown = capac + (capac >> 1);
        if (grown < size + 1u)
            grown = size + 1u;
        char* const buffer = static_cast<char*>(::renddx9abi::engineAlloc(grown * stride));
        if (size)
            ::memcpy(buffer, v.first, size * stride);
        if (v.first)
            ::renddx9abi::engineFree(v.first);
        v.first = buffer;
        v.last  = buffer + size * stride;
        v.end   = buffer + grown * stride;
    }
    copyConstruct(v.last, element);
    v.last += stride;
}

void raCopyString(void* dst, const void* src)
{
    new (dst) vc71::string(*static_cast<const vc71::string*>(src));
}

void raCopyInt(void* dst, const void* src)
{
    *static_cast<int*>(dst) = *static_cast<const int*>(src);
}

// sub_10044270's one-time tail.
void raRegisterParameterNames()
{
    if (g_raParameterOffsets.first &&
        ((g_raParameterOffsets.last - g_raParameterOffsets.first) >> 2) != 0)
        return;

    for (int i = 0; i < 31; ++i)
    {
        vc71::string name(kRaParameterNames[i].name);
        raRegistryPush(g_raParameterNames, &name, sizeof(vc71::string), &raCopyString);
        raRegistryPush(g_raParameterOffsets, &kRaParameterNames[i].offset, 4, &raCopyInt);
    }
}

// sub_10044270 - construct. Defaults read straight out of the instruction stream: 1.0f is
// 0x3F800000, 20.0f is 0x41A00000 (1101004800) and 0.1f is 0x3DCCCCCD (1045220557).
void raParametersConstruct(void* storage)
{
    RaMaterialParameters* const p = static_cast<RaMaterialParameters*>(storage);
    void* const holder = const_cast<void*>(static_cast<const void*>(raHolderVtable()));

    p->descriptor = kRaMaterialDescriptor;
    p->vptr       = holder;

    p->alphaBlendEnable.vptr = dx9rend::Ra::raHolderBool();  p->alphaBlendEnable.value = 0;
    p->depthWrite.vptr       = dx9rend::Ra::raHolderBool();  p->depthWrite.value       = 1;
    p->alphaTest.vptr        = dx9rend::Ra::raHolderBool();  p->alphaTest.value        = 0;

    p->diffuseColor.vptr = dx9rend::Ra::raHolderVec4();
    p->diffuseColor.value[0] = 1.0f; p->diffuseColor.value[1] = 1.0f;
    p->diffuseColor.value[2] = 1.0f; p->diffuseColor.value[3] = 1.0f;

    p->ambient.vptr = dx9rend::Ra::raHolderVec4();
    p->ambient.value[0] = 1.0f; p->ambient.value[1] = 1.0f;
    p->ambient.value[2] = 1.0f; p->ambient.value[3] = 1.0f;

    p->pointColor.vptr = dx9rend::Ra::raHolderVec4();
    p->pointColor.value[0] = 0.0f; p->pointColor.value[1] = 0.0f;
    p->pointColor.value[2] = 0.0f; p->pointColor.value[3] = 0.0f;

    // Shipped order: the value at index 24 is written before the vptr at index 23, and likewise for
    // the pair at 25/26. Kept.
    p->pointSun.value = 0;               p->pointSun.vptr = dx9rend::Ra::raHolderInt();
    p->disableLightmapShadows.value = 0.0f; p->disableLightmapShadows.vptr = dx9rend::Ra::raHolderFloat();

    p->specularColor.vptr = dx9rend::Ra::raHolderVec4();
    p->specularColor.value[0] = 1.0f; p->specularColor.value[1] = 1.0f;
    p->specularColor.value[2] = 1.0f; p->specularColor.value[3] = 1.0f;

    p->specularPower.vptr = dx9rend::Ra::raHolderFloat();  p->specularPower.value = 20.0f;
    p->opacity.vptr       = dx9rend::Ra::raHolderFloat();  p->opacity.value       = 1.0f;
    p->reflectivity.vptr  = dx9rend::Ra::raHolderFloat();  p->reflectivity.value  = 0.0f;

    p->cullMode.vptr     = dx9rend::Ra::raHolderInt();  p->cullMode.value     = 3;
    p->alphaTestRef.vptr = dx9rend::Ra::raHolderInt();  p->alphaTestRef.value = 0;
    p->srcBlend.vptr     = dx9rend::Ra::raHolderInt();  p->srcBlend.value     = 5;
    p->destBlend.vptr    = dx9rend::Ra::raHolderInt();  p->destBlend.value    = 6;

    p->diffuseMap.value    = 0; p->diffuseMap.vptr     = dx9rend::Ra::raHolderEngineTexture();
    p->detailMap.vptr      = dx9rend::Ra::raHolderEngineTexture(); p->detailMap.value     = 0;
    p->dirtMap.vptr        = dx9rend::Ra::raHolderEngineTexture(); p->dirtMap.value       = 0;
    p->crackMap.vptr       = dx9rend::Ra::raHolderEngineTexture(); p->crackMap.value      = 0;
    p->normalMap.vptr      = dx9rend::Ra::raHolderEngineTexture(); p->normalMap.value     = 0;
    p->crackNormalMap.vptr = dx9rend::Ra::raHolderEngineTexture(); p->crackNormalMap.value= 0;
    p->giMap.vptr          = dx9rend::Ra::raHolderEngineTexture(); p->giMap.value         = 0;

    p->staticGloss.vptr        = dx9rend::Ra::raHolderFloatSplat(); p->staticGloss.value        = 0.1f;
    p->objRadius.vptr          = dx9rend::Ra::raHolderFloat(); p->objRadius.value          = 0.0f;
    p->invHemiHeightScale.vptr = dx9rend::Ra::raHolderFloat(); p->invHemiHeightScale.value = 0.0f;

    p->parallaxScaleBias.vptr = dx9rend::Ra::raHolderVec4();
    p->parallaxScaleBias.value[0] = 1.0f; p->parallaxScaleBias.value[1] = 1.0f;
    p->parallaxScaleBias.value[2] = 1.0f; p->parallaxScaleBias.value[3] = 1.0f;

    p->cubeMap.vptr = dx9rend::Ra::raHolderEngineTexture(); p->cubeMap.value = 0;

    p->posUnpack.vptr = dx9rend::Ra::raHolderVec4();
    p->posUnpack.value[0] = 1.0f; p->posUnpack.value[1] = 1.0f;
    p->posUnpack.value[2] = 1.0f; p->posUnpack.value[3] = 1.0f;

    p->texUnpack.vptr = dx9rend::Ra::raHolderFloat(); p->texUnpack.value = 0.0f;

    p->normalUnpack.vptr = dx9rend::Ra::raHolderVec2();
    p->normalUnpack.value[0] = 0.0f;
    p->normalUnpack.value[1] = 0.0f;
    p->normalUnpack.value[2] = 0.0f;

    raRegisterParameterNames();
}

// The texture-holder release the destructor performs eight times, inline: the same explicit
// `if (t && !--t->refCount)` pattern the rest of this subsystem uses.
void raReleaseTexture(void*& slot)
{
    ::dice::hfe::dx9rend::EngineTexture* const tex =
        static_cast<::dice::hfe::dx9rend::EngineTexture*>(slot);
    if (tex && --tex->refCount == 0)
    {
        ::dice::hfe::dx9rend::textureDtor(tex);
        ::renddx9abi::engineFree(tex);
    }
}

// sub_10043F00 - destroy. It republishes every holder's vtable on the way out (the compiler's
// base-class unwind) and releases the eight texture references in the shipped order: CubeMap first,
// then GIMap, CrackNormalMap, NormalMap, CrackMap, DirtMap, DetailMap and DiffuseMap.
void raParametersDestroy(void* storage)
{
    RaMaterialParameters* const p = static_cast<RaMaterialParameters*>(storage);
    void* const holder = const_cast<void*>(static_cast<const void*>(raHolderVtable()));

    p->vptr = holder;
    p->normalUnpack.vptr = dx9rend::Ra::raHolderVec2();
    p->texUnpack.vptr    = dx9rend::Ra::raHolderFloat();
    p->posUnpack.vptr    = dx9rend::Ra::raHolderVec4();
    raReleaseTexture(p->cubeMap.value);
    p->cubeMap.vptr = dx9rend::Ra::raHolderEngineTexture();

    p->parallaxScaleBias.vptr    = dx9rend::Ra::raHolderVec4();
    p->invHemiHeightScale.vptr   = dx9rend::Ra::raHolderFloat();
    p->objRadius.vptr            = dx9rend::Ra::raHolderFloat();
    p->staticGloss.vptr          = dx9rend::Ra::raHolderFloatSplat();
    raReleaseTexture(p->giMap.value);            p->giMap.vptr          = dx9rend::Ra::raHolderEngineTexture();
    raReleaseTexture(p->crackNormalMap.value);   p->crackNormalMap.vptr = dx9rend::Ra::raHolderEngineTexture();
    raReleaseTexture(p->normalMap.value);        p->normalMap.vptr      = dx9rend::Ra::raHolderEngineTexture();
    raReleaseTexture(p->crackMap.value);         p->crackMap.vptr       = dx9rend::Ra::raHolderEngineTexture();
    raReleaseTexture(p->dirtMap.value);          p->dirtMap.vptr        = dx9rend::Ra::raHolderEngineTexture();
    raReleaseTexture(p->detailMap.value);        p->detailMap.vptr      = dx9rend::Ra::raHolderEngineTexture();
    raReleaseTexture(p->diffuseMap.value);       p->diffuseMap.vptr     = dx9rend::Ra::raHolderEngineTexture();

    p->destBlend.vptr    = dx9rend::Ra::raHolderInt();
    p->srcBlend.vptr     = dx9rend::Ra::raHolderInt();
    p->alphaTestRef.vptr = dx9rend::Ra::raHolderInt();
    p->cullMode.vptr     = dx9rend::Ra::raHolderInt();
    p->pointSun.vptr     = dx9rend::Ra::raHolderInt();
    p->reflectivity.vptr = dx9rend::Ra::raHolderFloat();
    p->opacity.vptr      = dx9rend::Ra::raHolderFloat();
    p->specularPower.vptr= dx9rend::Ra::raHolderFloat();
    p->specularColor.vptr= dx9rend::Ra::raHolderVec4();
    p->disableLightmapShadows.vptr = dx9rend::Ra::raHolderFloat();
    p->pointColor.vptr   = dx9rend::Ra::raHolderVec4();
    p->ambient.vptr      = dx9rend::Ra::raHolderVec4();
    p->diffuseColor.vptr = dx9rend::Ra::raHolderVec4();
    p->alphaTest.vptr    = dx9rend::Ra::raHolderBool();
    p->depthWrite.vptr   = dx9rend::Ra::raHolderBool();
    p->alphaBlendEnable.vptr = dx9rend::Ra::raHolderBool();
}

// Copies texture pointers without retaining them; destroying both copies over-releases textures.
void raParametersCopyConstruct(void* storage, const void* source)
{
    RaMaterialParameters* const p = static_cast<RaMaterialParameters*>(storage);
    const RaMaterialParameters* const s = static_cast<const RaMaterialParameters*>(source);
    void* const holder = const_cast<void*>(static_cast<const void*>(raHolderVtable()));

    p->vptr       = holder;
    p->descriptor = s->descriptor;

    p->alphaBlendEnable.vptr = dx9rend::Ra::raHolderBool(); p->alphaBlendEnable.value = s->alphaBlendEnable.value;
    p->depthWrite.vptr       = dx9rend::Ra::raHolderBool(); p->depthWrite.value       = s->depthWrite.value;
    p->alphaTest.vptr        = dx9rend::Ra::raHolderBool(); p->alphaTest.value        = s->alphaTest.value;

    p->diffuseColor.vptr = dx9rend::Ra::raHolderVec4();
    for (int i = 0; i < 4; ++i) p->diffuseColor.value[i] = s->diffuseColor.value[i];
    p->ambient.vptr = dx9rend::Ra::raHolderVec4();
    for (int i = 0; i < 4; ++i) p->ambient.value[i] = s->ambient.value[i];
    p->pointColor.vptr = dx9rend::Ra::raHolderVec4();
    for (int i = 0; i < 4; ++i) p->pointColor.value[i] = s->pointColor.value[i];

    p->pointSun.vptr = dx9rend::Ra::raHolderInt();               p->pointSun.value = s->pointSun.value;
    p->disableLightmapShadows.vptr = dx9rend::Ra::raHolderFloat(); p->disableLightmapShadows.value =
                                                 s->disableLightmapShadows.value;

    p->specularColor.vptr = dx9rend::Ra::raHolderVec4();
    for (int i = 0; i < 4; ++i) p->specularColor.value[i] = s->specularColor.value[i];

    p->specularPower.vptr = dx9rend::Ra::raHolderFloat(); p->specularPower.value = s->specularPower.value;
    p->opacity.vptr       = dx9rend::Ra::raHolderFloat(); p->opacity.value       = s->opacity.value;
    p->reflectivity.vptr  = dx9rend::Ra::raHolderFloat(); p->reflectivity.value  = s->reflectivity.value;
    p->cullMode.vptr      = dx9rend::Ra::raHolderInt(); p->cullMode.value      = s->cullMode.value;
    p->alphaTestRef.vptr  = dx9rend::Ra::raHolderInt(); p->alphaTestRef.value  = s->alphaTestRef.value;
    p->srcBlend.vptr      = dx9rend::Ra::raHolderInt(); p->srcBlend.value      = s->srcBlend.value;
    p->destBlend.vptr     = dx9rend::Ra::raHolderInt(); p->destBlend.value     = s->destBlend.value;

    p->diffuseMap.vptr     = dx9rend::Ra::raHolderEngineTexture(); p->diffuseMap.value     = s->diffuseMap.value;
    p->detailMap.vptr      = dx9rend::Ra::raHolderEngineTexture(); p->detailMap.value      = s->detailMap.value;
    p->dirtMap.vptr        = dx9rend::Ra::raHolderEngineTexture(); p->dirtMap.value        = s->dirtMap.value;
    p->crackMap.vptr       = dx9rend::Ra::raHolderEngineTexture(); p->crackMap.value       = s->crackMap.value;
    p->normalMap.vptr      = dx9rend::Ra::raHolderEngineTexture(); p->normalMap.value      = s->normalMap.value;
    p->crackNormalMap.vptr = dx9rend::Ra::raHolderEngineTexture(); p->crackNormalMap.value = s->crackNormalMap.value;
    p->giMap.vptr          = dx9rend::Ra::raHolderEngineTexture(); p->giMap.value          = s->giMap.value;

    p->staticGloss.vptr        = dx9rend::Ra::raHolderFloatSplat(); p->staticGloss.value        = s->staticGloss.value;
    p->objRadius.vptr          = dx9rend::Ra::raHolderFloat(); p->objRadius.value          = s->objRadius.value;
    p->invHemiHeightScale.vptr = dx9rend::Ra::raHolderFloat(); p->invHemiHeightScale.value = s->invHemiHeightScale.value;

    p->parallaxScaleBias.vptr = dx9rend::Ra::raHolderVec4();
    for (int i = 0; i < 4; ++i) p->parallaxScaleBias.value[i] = s->parallaxScaleBias.value[i];

    p->cubeMap.vptr = dx9rend::Ra::raHolderEngineTexture(); p->cubeMap.value = s->cubeMap.value;

    p->posUnpack.vptr = dx9rend::Ra::raHolderVec4();
    for (int i = 0; i < 4; ++i) p->posUnpack.value[i] = s->posUnpack.value[i];

    p->texUnpack.vptr = dx9rend::Ra::raHolderFloat(); p->texUnpack.value = s->texUnpack.value;

    p->normalUnpack.vptr = dx9rend::Ra::raHolderVec2();
    p->normalUnpack.value[0] = s->normalUnpack.value[0];
    p->normalUnpack.value[1] = s->normalUnpack.value[1];
    p->normalUnpack.value[2] = s->normalUnpack.value[2];
}

// sub_10110F70 - the counted texture-holder assignment: release the old, take the new, addRef.
void raAssignTextureValue(RaParamTexture& dst, void* incoming)
{
    ::dice::hfe::dx9rend::EngineTexture* const old =
        static_cast<::dice::hfe::dx9rend::EngineTexture*>(dst.value);
    if (old != incoming)
    {
        if (old && --old->refCount == 0)
        {
            ::dice::hfe::dx9rend::textureDtor(old);
            ::renddx9abi::engineFree(old);
        }
        dst.value = incoming;
        if (dst.value)
            ::dice::hfe::dx9rend::textureAddRef(
                static_cast<::dice::hfe::dx9rend::EngineTexture*>(dst.value));
    }
}

void raAssignTexture(RaParamTexture& dst, const RaParamTexture& src)
{
    if (&dst == &src)
        return;
    raAssignTextureValue(dst, src.value);
}

// sub_10111110 - copy-assign. No vtable stores (the objects already carry theirs) and the eight
// texture slots DO go through the counted assignment.
void raParametersAssign(void* storage, const void* source)
{
    RaMaterialParameters* const p = static_cast<RaMaterialParameters*>(storage);
    const RaMaterialParameters* const s = static_cast<const RaMaterialParameters*>(source);

    p->descriptor              = s->descriptor;
    p->alphaBlendEnable.value  = s->alphaBlendEnable.value;
    p->depthWrite.value        = s->depthWrite.value;
    p->alphaTest.value         = s->alphaTest.value;
    for (int i = 0; i < 4; ++i) p->diffuseColor.value[i]   = s->diffuseColor.value[i];
    for (int i = 0; i < 4; ++i) p->ambient.value[i]        = s->ambient.value[i];
    for (int i = 0; i < 4; ++i) p->pointColor.value[i]     = s->pointColor.value[i];
    p->pointSun.value               = s->pointSun.value;
    p->disableLightmapShadows.value = s->disableLightmapShadows.value;
    for (int i = 0; i < 4; ++i) p->specularColor.value[i]  = s->specularColor.value[i];
    p->specularPower.value = s->specularPower.value;
    p->opacity.value       = s->opacity.value;
    p->reflectivity.value  = s->reflectivity.value;
    p->cullMode.value      = s->cullMode.value;
    p->alphaTestRef.value  = s->alphaTestRef.value;
    p->srcBlend.value      = s->srcBlend.value;
    p->destBlend.value     = s->destBlend.value;

    raAssignTexture(p->diffuseMap,     s->diffuseMap);
    raAssignTexture(p->detailMap,      s->detailMap);
    raAssignTexture(p->dirtMap,        s->dirtMap);
    raAssignTexture(p->crackMap,       s->crackMap);
    raAssignTexture(p->normalMap,      s->normalMap);
    raAssignTexture(p->crackNormalMap, s->crackNormalMap);
    raAssignTexture(p->giMap,          s->giMap);

    p->staticGloss.value        = s->staticGloss.value;
    p->objRadius.value          = s->objRadius.value;
    p->invHemiHeightScale.value = s->invHemiHeightScale.value;
    for (int i = 0; i < 4; ++i) p->parallaxScaleBias.value[i] = s->parallaxScaleBias.value[i];
    raAssignTexture(p->cubeMap, s->cubeMap);
    for (int i = 0; i < 4; ++i) p->posUnpack.value[i] = s->posUnpack.value[i];
    p->texUnpack.value       = s->texUnpack.value;
    p->normalUnpack.value[0] = s->normalUnpack.value[0];
    p->normalUnpack.value[1] = s->normalUnpack.value[1];
    p->normalUnpack.value[2] = s->normalUnpack.value[2];
}

inline RaMaterialParameters& params(StaticMeshMaterial* m)
{
    return *reinterpret_cast<RaMaterialParameters*>(m->m_parameters);
}
inline const RaMaterialParameters& params(const StaticMeshMaterial* m)
{
    return *reinterpret_cast<const RaMaterialParameters*>(m->m_parameters);
}

} // anonymous namespace


// [CHOSEN, not recovered] Shared forwards let RaShader construct/destroy the material
// parameter block and trigger its one-time parameter-name registration.
} // namespace geom

namespace dx9rend {
namespace Ra {

void raMaterialParametersConstruct(void* storage) { ::dice::hfe::geom::raParametersConstruct(storage); }
void raMaterialParametersDestroy  (void* storage) { ::dice::hfe::geom::raParametersDestroy(storage); }
// Published for SkinnedMeshTemplate.cpp's material copy-assign: sub_10111110 is the same body for
// both material classes (they share the base whose vptr both destructors restore), and it sat in
// this file's anonymous namespace only because StaticMesh was its sole caller.
void raMaterialParametersAssign   (void* storage, const void* source)
{ ::dice::hfe::geom::raParametersAssign(storage, source); }

// Returns material parameter names populated by the block's one-time constructor tail.
// Initialization must precede shader binding; an empty registry silently misbinds offsets.
const vc71::vector_layout& raMaterialParameterNames()
{
    return ::dice::hfe::geom::g_raParameterNames;
}

} // namespace Ra
} // namespace dx9rend

namespace geom {


// Material cache nodes are 0x28 bytes, with an 8-byte key and 16-byte vector value.
// Use size-relative color/nil offsets; the generic 0x30-byte map node is incompatible.

namespace {

const unsigned int kMaterialMapNodeSize = 0x28;

inline void*& mapNodeLeft  (void* n) { return reinterpret_cast<void**>(n)[0]; }
inline void*& mapNodeParent(void* n) { return reinterpret_cast<void**>(n)[1]; }
inline void*& mapNodeRight (void* n) { return reinterpret_cast<void**>(n)[2]; }
inline unsigned char& mapNodeColor(void* n)
{ return reinterpret_cast<unsigned char*>(n)[kMaterialMapNodeSize - 4]; }
inline unsigned char& mapNodeIsNil(void* n)
{ return reinterpret_cast<unsigned char*>(n)[kMaterialMapNodeSize - 3]; }
inline vc71::vector_layout& mapNodeVector(void* n)
{ return *reinterpret_cast<vc71::vector_layout*>(static_cast<char*>(n) + 20); }

// sub_1011B560 plus the four stores its caller makes. Note the `comp` word of the map head is NOT
// written by either, so it keeps whatever the allocation held - shipped.
void materialMapConstruct(vc71::map_layout& map)
{
    void* const node = ::renddx9abi::engineAlloc(kMaterialMapNodeSize);
    if (node)
    {
        mapNodeLeft(node)   = 0;
        mapNodeParent(node) = 0;
        mapNodeRight(node)  = 0;
        mapNodeColor(node)  = 1;
        mapNodeIsNil(node)  = 0;
    }
    map.head = static_cast<vc71::map_node*>(node);
    mapNodeIsNil(node)  = 1;
    mapNodeParent(node) = node;
    mapNodeLeft(node)   = node;
    mapNodeRight(node)  = node;
    map.size = 0;
}

// sub_1011D9B0 - the recursive subtree destroy: right subtree first, then this node's vector
// buffer, then the node, then continue down the left link.
void materialMapDestroySubtree(void* node)
{
    while (node && !mapNodeIsNil(node))
    {
        materialMapDestroySubtree(mapNodeRight(node));
        void* const left = mapNodeLeft(node);
        vc71::vector_layout& v = mapNodeVector(node);
        if (v.first)
            ::renddx9abi::engineFree(v.first);
        v.first = 0;
        v.last  = 0;
        v.end   = 0;
        ::renddx9abi::engineFree(node);
        node = left;
    }
}

// sub_1011E470's erase(begin, end) fast path, which is the only one the destructor takes.
void materialMapClear(vc71::map_layout& map)
{
    if (!map.head)
        return;
    materialMapDestroySubtree(mapNodeParent(map.head));
    mapNodeParent(map.head) = map.head;
    mapNodeLeft(map.head)   = map.head;
    mapNodeRight(map.head)  = map.head;
    map.size = 0;
}

} // anonymous namespace


// StaticMeshMaterial


// The vptr is published by the compiler; everything else is the shipped store order.
StaticMeshMaterial::StaticMeshMaterial()
{
    m_vector04.first = 0;
    m_vector04.last  = 0;
    m_vector04.end   = 0;
    m_vector14.first = 0;
    m_vector14.last  = 0;
    m_vector14.end   = 0;
    m_flags24        = 0;
    m_shaderMaterial = 0;

    raParametersConstruct(m_parameters);

    m_alphaMode      = 0;
    m_vertexStart    = 0;
    m_indexStart     = 0;
    m_primitiveCount = 0;
    m_vertexCount    = 0;

    m_bounds.min.x = 0.0f; m_bounds.min.y = 0.0f; m_bounds.min.z = 0.0f;
    m_bounds.max.x = 0.0f; m_bounds.max.y = 0.0f; m_bounds.max.z = 0.0f;
    m_bounds.originRadius = 0.0f;
    m_bounds.radius       = 0.0f;
    m_bounds.dirty        = 1;      // only the byte; pad21[3] stays as allocated, as shipped
    m_bounds.extent.x = 0.0f; m_bounds.extent.y = 0.0f; m_bounds.extent.z = 0.0f;
    m_bounds.center.x = 0.0f; m_bounds.center.y = 0.0f; m_bounds.center.z = 0.0f;

    // m_dword1D4 and m_dword1D8 are NOT written here. Shipped; the copy constructor still copies
    // them, so an uninitialised value propagates.
    m_vegetationVertices    = 0;
    m_vegetationIndices     = 0;
    m_vegetationVertexCount = 0;
    m_vegetationIndexCount  = 0;
    m_vegetationNonTiled    = 0;
    m_dword1F0 = 0;
    m_template = 0;

    materialMapConstruct(m_map1F8);
}

// the destructor body. Order is shipped: the map, then the two vegetation buffers,
// then the parameter block, then the 4-byte vector, then the vector-of-vectors.

// The shipped tail (`*(_DWORD *)this = &off_101E2428`) is NOT reproduced - see the file header.
void StaticMeshMaterial::destroyMembers()
{
    materialMapClear(m_map1F8);
    ::renddx9abi::engineFree(m_map1F8.head);
    m_map1F8.head = 0;
    m_map1F8.size = 0;

    ::renddx9abi::engineFree(m_vegetationIndices);
    ::renddx9abi::engineFree(m_vegetationVertices);
    m_vegetationVertices    = 0;
    m_vegetationIndices     = 0;
    m_vegetationVertexCount = 0;
    m_vegetationIndexCount  = 0;

    raParametersDestroy(m_parameters);

    if (m_vector14.first)
        ::renddx9abi::engineFree(m_vector14.first);
    m_vector14.first = 0;
    m_vector14.last  = 0;
    m_vector14.end   = 0;

    if (m_vector04.first)
    {
        // sub_100ED1C0 - free each inner vector's buffer and zero it. The elements are 4-byte PODs,
        // so there is nothing else to destroy.
        for (char* cursor = m_vector04.first; cursor != m_vector04.last; cursor += 16)
        {
            vc71::vector_layout& inner = *reinterpret_cast<vc71::vector_layout*>(cursor);
            if (inner.first)
                ::renddx9abi::engineFree(inner.first);
            inner.first = 0;
            inner.last  = 0;
            inner.end   = 0;
        }
        ::renddx9abi::engineFree(m_vector04.first);
    }
    m_vector04.first = 0;
    m_vector04.last  = 0;
    m_vector04.end   = 0;
}

namespace {

// sub_1011DAA0/sub_1011C4E0 - the two "copy a POD range into a fresh buffer" helpers behind the
// vector copy constructors, rendered as one source-level operation.
void copyPodVector(vc71::vector_layout& dst, const vc71::vector_layout& src, unsigned int stride)
{
    const unsigned int bytes = static_cast<unsigned int>(src.last - src.first);
    dst.first = 0;
    dst.last  = 0;
    dst.end   = 0;
    if (!bytes)
        return;
    char* const buffer = static_cast<char*>(::renddx9abi::engineAlloc(bytes));
    ::memcpy(buffer, src.first, bytes);
    dst.first = buffer;
    dst.last  = buffer + bytes;
    dst.end   = buffer + bytes;
    (void)stride;
}

// sub_1011E740 - vector<vector<uint>>'s copy constructor: the outer buffer is raw, but each inner
// vector is deep-copied.
void copyVectorOfVectors(vc71::vector_layout& dst, const vc71::vector_layout& src)
{
    const unsigned int count =
        src.first ? static_cast<unsigned int>((src.last - src.first) >> 4) : 0u;
    dst.first = 0;
    dst.last  = 0;
    dst.end   = 0;
    if (!count)
        return;
    char* const buffer = static_cast<char*>(::renddx9abi::engineAlloc(count * 16u));
    for (unsigned int i = 0; i < count; ++i)
    {
        copyPodVector(*reinterpret_cast<vc71::vector_layout*>(buffer + i * 16u),
                      *reinterpret_cast<const vc71::vector_layout*>(src.first + i * 16u), 4u);
    }
    dst.first = buffer;
    dst.last  = buffer + count * 16u;
    dst.end   = buffer + count * 16u;
}

} // anonymous namespace

// the copy constructor.

// Copies shader and vegetation pointers without retaining or duplicating their resources.
// Copies the two trailing words even though the default constructor leaves them uninitialized.
StaticMeshMaterial::StaticMeshMaterial(const StaticMeshMaterial& other)
{
    copyVectorOfVectors(m_vector04, other.m_vector04);
    copyPodVector(m_vector14, other.m_vector14, 4u);

    m_flags24        = other.m_flags24;
    m_shaderMaterial = other.m_shaderMaterial;

    raParametersCopyConstruct(m_parameters, other.m_parameters);

    m_alphaMode      = other.m_alphaMode;
    m_vertexStart    = other.m_vertexStart;
    m_indexStart     = other.m_indexStart;
    m_primitiveCount = other.m_primitiveCount;
    m_vertexCount    = other.m_vertexCount;
    m_u4             = other.m_u4;
    m_u5u6           = other.m_u5u6;

    meshBoundsCopy(m_bounds, other.m_bounds);

    m_dword1D4              = other.m_dword1D4;
    m_dword1D8              = other.m_dword1D8;
    m_vegetationVertices    = other.m_vegetationVertices;
    m_vegetationIndices     = other.m_vegetationIndices;
    m_vegetationVertexCount = other.m_vegetationVertexCount;
    m_vegetationIndexCount  = other.m_vegetationIndexCount;
    m_vegetationNonTiled    = other.m_vegetationNonTiled;
    m_dword1F0              = other.m_dword1F0;
    m_template              = other.m_template;

    // sub_1011ECC0 - a fresh head node, then the source's nodes are re-inserted. The re-insertion
    // (sub_1011E320) walks the source in order and copies each {key, vector} pair.
    materialMapConstruct(m_map1F8);
    // [FRONTIER] the ordered copy-insert itself. The node's 8-byte key is not identified by any
    // recovered reader in this tranche, so inserting one would mean inventing a comparator.
    if (other.m_map1F8.head && other.m_map1F8.size != 0)
    {
        RD9_FRONTIER("dice::hfe::geom::StaticMeshMaterial::copyMap "
                     "(sub_1011ECC0 -> sub_1011E320, the 0x28-byte +0x1F8 map's copy-insert)",
                     0x00);
    }
}

// the copy assignment.

// IT does NOT clear m_vector04 first. The shipped loop (PE 0x1011EB70..0x1011EBFE) appends one
// copy of every source element to whatever the destination already holds, so repeated assignment -
// which is exactly what mergeMaterials' erase does - grows the vector without bound. Reproduced.
// m_vector14 is not assigned at all, and neither is the +0x1F8 map.
StaticMeshMaterial& StaticMeshMaterial::operator=(const StaticMeshMaterial& other)
{
    m_shaderMaterial     = other.m_shaderMaterial;
    m_alphaMode          = other.m_alphaMode;
    m_vertexStart        = other.m_vertexStart;
    m_indexStart         = other.m_indexStart;
    m_primitiveCount     = other.m_primitiveCount;
    m_vertexCount        = other.m_vertexCount;
    m_template           = other.m_template;
    m_dword1F0           = other.m_dword1F0;
    m_flags24            = other.m_flags24;
    m_vegetationNonTiled = other.m_vegetationNonTiled;

    meshBoundsCopy(m_bounds, other.m_bounds);

    m_vegetationVertices    = other.m_vegetationVertices;
    m_vegetationIndices     = other.m_vegetationIndices;
    m_vegetationVertexCount = other.m_vegetationVertexCount;
    m_vegetationIndexCount  = other.m_vegetationIndexCount;

    raParametersAssign(m_parameters, other.m_parameters);

    for (const char* cursor = other.m_vector04.first;
         cursor != other.m_vector04.last;
         cursor += 16)
    {
        vc71::vector_layout scratch = { 0, 0, 0, 0 };
        copyPodVector(scratch, *reinterpret_cast<const vc71::vector_layout*>(cursor), 4u);

        // push_back(scratch) - a deep copy of the temporary, then the temporary's buffer is freed.
        const unsigned int size =
            m_vector04.first ? static_cast<unsigned int>((m_vector04.last - m_vector04.first) >> 4)
                             : 0u;
        const unsigned int capacity =
            m_vector04.first ? static_cast<unsigned int>((m_vector04.end - m_vector04.first) >> 4)
                             : 0u;
        if (!m_vector04.first || size >= capacity)
        {
            unsigned int grown = capacity + (capacity >> 1);
            if (grown < size + 1u)
                grown = size + 1u;
            char* const buffer = static_cast<char*>(::renddx9abi::engineAlloc(grown * 16u));
            if (size)
                ::memcpy(buffer, m_vector04.first, size * 16u);
            if (m_vector04.first)
                ::renddx9abi::engineFree(m_vector04.first);
            m_vector04.first = buffer;
            m_vector04.last  = buffer + size * 16u;
            m_vector04.end   = buffer + grown * 16u;
        }
        copyPodVector(*reinterpret_cast<vc71::vector_layout*>(m_vector04.last), scratch, 4u);
        m_vector04.last += 16;

        if (scratch.first)
            ::renddx9abi::engineFree(scratch.first);
    }
    return *this;
}

// the nine virtual slots of off_1021C4E4
// +0x00  the deleting destructor.
void* StaticMeshMaterial::destroy(unsigned char flags)
{
    destroyMembers();
    if ((flags & 1) != 0)
        ::renddx9abi::engineFree(this);
    return this;
}

// Applies material streams and a cached declaration for the shader's build serial.
// Missing required streams report the material/shader mismatch.
namespace {

// The +0x1F8 map's value - what sub_1011EC20 zeroes into a fresh node and applyStreams fills:
// the declaration the streams need, and which of the material's streams (indices into m_vector04
// and m_vector14, in the order they were bound) the shader takes.
struct StreamDeclarationRecord
{
    IDirect3DVertexDeclaration9* declaration;   // node +0x10
    vc71::vector_layout          streams;       // node +0x14  vector<unsigned int>
};

static_assert(sizeof(StreamDeclarationRecord) == 20, "the 0x28 node carries a 20-byte value");

inline StreamDeclarationRecord& streamRecord(vc71::map_node_u32* node)
{
    return *reinterpret_cast<StreamDeclarationRecord*>(node->value);
}

// sub_100EC310 - vector<unsigned int>::operator= onto the node's vector: an empty source frees,
// a source that fits is copied over, a larger one reallocates to its exact size.
void assignUnsignedVector(vc71::vector_layout& dst, const vc71::vector_layout& src)
{
    const unsigned int count = src.first ? static_cast<unsigned int>(src.last - src.first) >> 2 : 0u;
    if (!count)
    {
        if (dst.first)
            ::renddx9abi::engineFree(dst.first);
        dst.first = 0;
        dst.last  = 0;
        dst.end   = 0;
        return;
    }
    const unsigned int capacity = dst.first ? static_cast<unsigned int>(dst.end - dst.first) >> 2 : 0u;
    if (count > capacity)
    {
        if (dst.first)
            ::renddx9abi::engineFree(dst.first);
        dst.first = static_cast<char*>(::renddx9abi::engineAlloc(count * 4u));
        dst.end   = dst.first + count * 4u;
    }
    ::memcpy(dst.first, src.first, count * 4u);
    dst.last = dst.first + count * 4u;
}

} // anonymous namespace

// Binds the declaration, material streams, and template index buffer for the shader.
// Caches bindings by shader build serial. A cache miss with missing elements returns
// false after insertion; later hits bind the partial record and return true.
bool StaticMeshMaterial::applyStreams(void* shaderState)
{
    namespace Ra = ::dice::hfe::dx9rend::Ra;
    Ra::RaShader* const shader = static_cast<Ra::RaShader*>(shaderState);
    const unsigned int key = static_cast<unsigned int>(shader->m_buildSerial);        // +0x174

    vc71::map_node_u32* node = vc71::mapFindU32(m_map1F8, key);
    if (!node)
    {
        Ra::RaStreamRequirements* const required =
            Ra::raStreamRequirementsCreate(shader->m_requiredVertexElements);         // sub_10049330 + sub_1004A0F0
        Ra::RaDeclarationBuilder builder;
        Ra::raDeclarationBuilderConstruct(builder);
        vc71::vector_layout streamIndices = { 0, 0, 0, 0 };

        const unsigned int streamCount =
            m_vector04.first ? static_cast<unsigned int>(m_vector04.last - m_vector04.first) >> 4 : 0u;
        for (unsigned int i = 0; i < streamCount; ++i)
        {
            const vc71::vector_layout& descriptors =
                reinterpret_cast<const vc71::vector_layout*>(m_vector04.first)[i];
            if (Ra::raStreamRequirementsConsume(required, descriptors))
            {
                vc71::vectorPushBackIn(streamIndices, sizeof(unsigned int), &i);
                Ra::raDeclarationBuilderAddStream(builder, descriptors);
            }
        }

        IDirect3DDevice9* const device = Ra::raRendererDevice();                     // renderer +0x0C
        IDirect3DVertexDeclaration9* const declaration = Ra::raVertexDeclarationCached(
            Ra::raDeclarationBuilderFinish(builder), device);

        vc71::map_node_u32* const inserted = vc71::mapInsertU32(m_map1F8, key);
        streamRecord(inserted).declaration = declaration;
        assignUnsignedVector(streamRecord(inserted).streams, streamIndices);
        node = vc71::mapFindU32(m_map1F8, key);

        const bool starving = Ra::raStreamRequirementsStarving(required);
        if (starving)
        {
            char missing[1024];
            Ra::raStreamRequirementsDescribe(required, missing, sizeof(missing));
            ::renddx9abi::logf("StaticMeshTemplate.cpp:202 Starving streams in the shader %s, %s",
                               shader->m_name.c_str(), missing);
        }
        if (streamIndices.first)
            ::renddx9abi::engineFree(streamIndices.first);
        Ra::raDeclarationBuilderDestroy(builder);
        Ra::raStreamRequirementsDestroy(required);
        if (starving)
            return false;
    }

    StreamDeclarationRecord& record = streamRecord(node);
    ::dice::hfe::dx9rend::stateManagerSetVertexDeclaration(record.declaration);      // +0x5C
    const unsigned int* const indices = reinterpret_cast<const unsigned int*>(record.streams.first);
    const unsigned int count =
        record.streams.first ? static_cast<unsigned int>(record.streams.last - record.streams.first) >> 2 : 0u;
    for (unsigned int i = 0; i < count; ++i)
    {
        void* const wrapper = reinterpret_cast<void**>(m_vector14.first)[indices[i]];
        ::dice::hfe::dx9rend::vertexBufferBindStream(wrapper, i);                    // +0x58
    }
    ::dice::hfe::dx9rend::indexBufferBind(m_template->m_indexBuffer);                // +0x54
    return true;
}

int StaticMeshMaterial::getVertexCount() const    { return static_cast<int>(m_vertexCount); }
int StaticMeshMaterial::slot_0x0C() const         { return -1; }
int StaticMeshMaterial::getPrimitiveCount() const { return static_cast<int>(m_primitiveCount); }
int StaticMeshMaterial::getDword1F0() const       { return static_cast<int>(m_dword1F0); }
int StaticMeshMaterial::getVertexStart() const    { return static_cast<int>(m_vertexStart); }
int StaticMeshMaterial::slot_0x1C() const         { return 0; }
int StaticMeshMaterial::getIndexStart() const     { return static_cast<int>(m_indexStart); }


// Resizes nested material storage on the engine heap.
// Default fill values are constructed locally; callers supply only the requested count.

void staticMeshMaterialsResize(vc71::vector_layout& materials, unsigned int count)
{
    const unsigned int size =
        materials.first ? static_cast<unsigned int>((materials.last - materials.first) / 516) : 0u;

    if (size >= count)
    {
        // Shrink: destroy the tail through slot 0 with flags 0, then retract `last`.
        if (materials.first && count < size)
        {
            for (char* cursor = materials.first + 516 * count;
                 cursor != materials.last;
                 cursor += 516)
            {
                reinterpret_cast<StaticMeshMaterial*>(cursor)->destroy(0);
            }
            materials.last = materials.first + 516 * count;
        }
        return;
    }

    const unsigned int wanted = count - size;
    const unsigned int capacity =
        materials.first ? static_cast<unsigned int>((materials.end - materials.first) / 516) : 0u;

    if (capacity < size + wanted)
    {
        unsigned int grown = capacity + (capacity >> 1);
        if (grown < size + wanted)
            grown = size + wanted;
        char* const buffer = static_cast<char*>(::renddx9abi::engineAlloc(grown * 516u));
        for (unsigned int i = 0; i < size; ++i)
        {
            new (buffer + i * 516u)
                StaticMeshMaterial(*reinterpret_cast<StaticMeshMaterial*>(materials.first + i * 516u));
            reinterpret_cast<StaticMeshMaterial*>(materials.first + i * 516u)->destroy(0);
        }
        if (materials.first)
            ::renddx9abi::engineFree(materials.first);
        materials.first = buffer;
        materials.last  = buffer + size * 516u;
        materials.end   = buffer + grown * 516u;
    }

    for (unsigned int i = 0; i < wanted; ++i)
    {
        new (materials.last) StaticMeshMaterial();
        materials.last += 516;
    }
}

namespace {

// Resizes a vector of 16-byte vector headers; destroyElement releases removed contents.
// [CHOSEN, not recovered] Growth transfers inner ownership rather than deep-copying it.
// Current callers size from empty, where that allocation difference is unobservable.
void resizeVectorOfVectors(vc71::vector_layout& v, unsigned int count,
                           void (*destroyElement)(vc71::vector_layout&))
{
    const unsigned int size = v.first ? static_cast<unsigned int>((v.last - v.first) >> 4) : 0u;

    if (size >= count)
    {
        if (v.first && count < size)
        {
            for (char* cursor = v.first + 16u * count; cursor != v.last; cursor += 16)
                destroyElement(*reinterpret_cast<vc71::vector_layout*>(cursor));
            v.last = v.first + 16u * count;
        }
        return;
    }

    const unsigned int wanted   = count - size;
    const unsigned int capacity = v.first ? static_cast<unsigned int>((v.end - v.first) >> 4) : 0u;

    if (capacity < size + wanted)
    {
        unsigned int grown = capacity + (capacity >> 1);
        if (grown < size + wanted)
            grown = size + wanted;
        char* const buffer = static_cast<char*>(::renddx9abi::engineAlloc(grown * 16u));
        if (size)
            ::memcpy(buffer, v.first, size * 16u);     // the moved-from heads are not destroyed
        if (v.first)
            ::renddx9abi::engineFree(v.first);
        v.first = buffer;
        v.last  = buffer + size * 16u;
        v.end   = buffer + grown * 16u;
    }

    for (unsigned int i = 0; i < wanted; ++i)
    {
        vc71::vector_layout* const fresh = reinterpret_cast<vc71::vector_layout*>(v.last);
        fresh->alloc = 0;
        fresh->first = 0;
        fresh->last  = 0;
        fresh->end   = 0;
        v.last += 16;
    }
}

void destroyMaterialVector(vc71::vector_layout& materials)
{
    if (materials.first)
    {
        for (char* cursor = materials.first; cursor != materials.last; cursor += 516)
            reinterpret_cast<StaticMeshMaterial*>(cursor)->destroy(0);
        ::renddx9abi::engineFree(materials.first);
    }
    materials.first = 0;
    materials.last  = 0;
    materials.end   = 0;
}

void destroyLodVector(vc71::vector_layout& lods)
{
    if (lods.first)
    {
        staticMeshMaterialsDestroyLods(reinterpret_cast<unsigned char*>(lods.first),
                                       reinterpret_cast<unsigned char*>(lods.last));
        ::renddx9abi::engineFree(lods.first);
    }
    lods.first = 0;
    lods.last  = 0;
    lods.end   = 0;
}

} // anonymous namespace

void staticMeshMaterialsResizeLods(vc71::vector_layout& lods, unsigned int count)
{
    resizeVectorOfVectors(lods, count, &destroyMaterialVector);
}

void staticMeshMaterialsResizeGeoms(vc71::vector_layout& geoms, unsigned int count)
{
    resizeVectorOfVectors(geoms, count, &destroyLodVector);
}

// for every vector<StaticMeshMaterial> in [first, last): destroy each element through
// slot 0 with flags 0, free the buffer, and zero the three words.
void staticMeshMaterialsDestroyLods(unsigned char* first, unsigned char* last)
{
    for (unsigned char* cursor = first; cursor != last; cursor += 16)
        destroyMaterialVector(*reinterpret_cast<vc71::vector_layout*>(cursor));
}

// the same, one level up.
void staticMeshMaterialsDestroyGeoms(unsigned char* first, unsigned char* last)
{
    for (unsigned char* cursor = first; cursor != last; cursor += 16)
        destroyLodVector(*reinterpret_cast<vc71::vector_layout*>(cursor));
}


// The material-container walk every StaticMeshTemplate body below performs:
//     m_materials.first   -> vector_layout[geom]
//     [geom].first        -> vector_layout[lod]
//     [lod].first         -> StaticMeshMaterial[material], 516 bytes each

namespace {

inline vc71::vector_layout* lodVectors(StaticMeshTemplate* t, unsigned int geom)
{
    return reinterpret_cast<vc71::vector_layout*>(
        reinterpret_cast<vc71::vector_layout*>(t->m_materials.first)[geom].first);
}

inline vc71::vector_layout& materialVector(StaticMeshTemplate* t, unsigned int geom,
                                           unsigned int lod)
{
    return lodVectors(t, geom)[lod];
}

inline unsigned int materialCount(const vc71::vector_layout& v)
{
    return v.first ? static_cast<unsigned int>((v.last - v.first) / 516) : 0u;
}

inline StaticMeshMaterial* materialAt(const vc71::vector_layout& v, unsigned int i)
{
    return reinterpret_cast<StaticMeshMaterial*>(v.first + 516u * i);
}

inline unsigned int geomCount(StaticMeshTemplate* t)
{
    return t->m_lodCounts.first
               ? static_cast<unsigned int>((t->m_lodCounts.last - t->m_lodCounts.first) >> 2)
               : 0u;
}

inline unsigned int lodCount(StaticMeshTemplate* t, unsigned int geom)
{
    return reinterpret_cast<const unsigned int*>(t->m_lodCounts.first)[geom];
}

} // anonymous namespace


// StaticMeshTemplate::getPrimitiveCount - 0x10115C60 (slot +0x58)

int StaticMeshTemplate::getPrimitiveCount(int geom, int lod, int material)
{
    const vc71::vector_layout& materials =
        materialVector(this, static_cast<unsigned int>(geom), static_cast<unsigned int>(lod));
    if (material != -1)
        return materialAt(materials, static_cast<unsigned int>(material))->getPrimitiveCount();

    int total = 0;
    const unsigned int count = materialCount(materials);
    for (unsigned int i = 0; i < count; ++i)
        total += materialAt(materials, i)->getPrimitiveCount();
    return total;
}


// With geom and lod both -1, sums every geometry/LOD through this virtual slot.

int StaticMeshTemplate::getVertexCount(int geom, int lod, int material)
{
    if (geom == -1 && lod == -1)
    {
        int total = 0;
        const unsigned int geoms = geomCount(this);
        for (unsigned int g = 0; g < geoms; ++g)
        {
            const unsigned int lods = lodCount(this, g);
            for (unsigned int l = 0; l < lods; ++l)
                total += this->getVertexCount(static_cast<int>(g), static_cast<int>(l), -1);
        }
        return total;
    }

    const vc71::vector_layout& materials =
        materialVector(this, static_cast<unsigned int>(geom), static_cast<unsigned int>(lod));
    if (material != -1)
        return materialAt(materials, static_cast<unsigned int>(material))->getVertexCount();

    int total = 0;
    const unsigned int count = materialCount(materials);
    for (unsigned int i = 0; i < count; ++i)
        total += materialAt(materials, i)->getVertexCount();
    return total;
}


// Reports bytes for mask bits 1 (vertices), 2 (indices), and 4 (unique textures).
// -1 sums those through virtual calls. Bit 8 instead returns triangle count, discarding bytes.

unsigned int StaticMeshTemplate::getMemoryUsage(int mask)
{
    if (mask == -1)
    {
        const unsigned int a = this->getMemoryUsage(1);
        const unsigned int b = this->getMemoryUsage(2) + a;
        return b + this->getMemoryUsage(4);
    }

    unsigned int total = 0;

    if ((mask & 1) != 0)
    {
        // +0x20 of the wrapper is D3DVERTEXBUFFER_DESC::Size - the desc occupies +0x10..+0x27 in
        // BufferManager.cpp's VertexBuffer record, so Size is its fifth dword.
        void* const streams[7] =
        {
            m_vertexStream, m_tangentStream,
            m_uvStreams[0], m_uvStreams[1], m_uvStreams[2], m_uvStreams[3], m_uvStreams[4]
        };
        for (int i = 0; i < 7; ++i)
        {
            if (streams[i])
                total += *reinterpret_cast<const unsigned int*>(
                    static_cast<const unsigned char*>(streams[i]) + 32);
        }
    }

    if ((mask & 2) != 0 && m_indexBuffer)
        total += 2u * m_indexCount;

    if ((mask & 8) != 0 && m_indexBuffer)
        return m_indexCount / 3u;      // shipped: `total` is discarded on this arm

    if ((mask & 4) != 0)
    {
        std::set<const void*> unique;
        const unsigned int geoms =
            m_geoms.first ? static_cast<unsigned int>((m_geoms.last - m_geoms.first) >> 5) : 0u;
        for (unsigned int g = 0; g < geoms; ++g)
        {
            const StaticMeshGeom& geomRecord =
                reinterpret_cast<const StaticMeshGeom*>(m_geoms.first)[g];
            if (!geomRecord.lods.first)
                continue;
            const unsigned int lods =
                static_cast<unsigned int>(geomRecord.lods.last - geomRecord.lods.first) / 104u;
            for (unsigned int l = 0; l < lods; ++l)
            {
                const vc71::vector_layout& materials = materialVector(this, g, l);
                const unsigned int count = materialCount(materials);
                for (unsigned int m = 0; m < count; ++m)
                {
                    const ShaderMaterial* const shader = materialAt(materials, m)->m_shaderMaterial;
                    for (int t = 0; t < 8; ++t)
                    {
                        if (shader->textures[t])
                            unique.insert(shader->textures[t]);
                    }
                }
            }
        }
        for (std::set<const void*>::const_iterator it = unique.begin(); it != unique.end(); ++it)
        {
            total += static_cast<const ::dice::hfe::dx9rend::EngineTexture*>(*it)->totalBytes;
        }
    }

    return total;
}


// Merges materials with equal quality-masked shader flags, alpha mode, and active textures.
// After each merge, restart the inner scan at the current outer material.

namespace {

// sub_10115870 - add `delta` to a run of indices in the template's index data.
void mergeShiftIndices(StaticMeshTemplate* t, short delta, int indexStart, int primitiveCount)
{
    unsigned short* const indices = t->m_indexData;
    const int end = indexStart + primitiveCount + 2 * primitiveCount;   // indexStart + 3 * count
    for (int i = indexStart; i < end; ++i)
        indices[i] = static_cast<unsigned short>(indices[i] + delta);
}

// Shifts subsequent material starts by primitiveDelta * 3 indices and vertexDelta vertices.
void mergeShiftStarts(StaticMeshTemplate* t, unsigned int fromGeom, unsigned int fromLod,
                      unsigned int fromMaterial, int primitiveDelta, int vertexDelta)
{
    const unsigned int geoms = geomCount(t);
    for (unsigned int g = fromGeom; g < geoms; ++g)
    {
        const unsigned int lods = lodCount(t, g);
        unsigned int l = (g != fromGeom) ? 0u : fromLod;
        for (; l < lods; ++l)
        {
            const vc71::vector_layout& materials = materialVector(t, g, l);
            const unsigned int count = materialCount(materials);
            const unsigned int firstMaterial =
                (g == fromGeom && l == fromLod) ? fromMaterial : 0u;
            for (unsigned int m = firstMaterial; m < count; ++m)
            {
                StaticMeshMaterial* const material = materialAt(materials, m);
                material->m_vertexStart += static_cast<unsigned int>(vertexDelta);
                material->m_indexStart  += static_cast<unsigned int>(3 * primitiveDelta);
            }
        }
    }
}

// sub_10116890 - move material `to`'s vertex and index runs so they sit immediately after material
// `from`'s. Two rotations, each done through a scratch buffer: one over the interleaved vertex data
// at +0x90, one over the 16-bit index data at +0x98.
void mergeRotateGeometry(StaticMeshTemplate* t, unsigned int geom, unsigned int lod,
                         unsigned int to, unsigned int from)
{
    const vc71::vector_layout& materials = materialVector(t, geom, lod);
    const StaticMeshMaterial* const src = materialAt(materials, from);
    const StaticMeshMaterial* const dst = materialAt(materials, to);

    const unsigned int stride = t->m_vertexStride;
    const unsigned int firstVertex = dst->m_vertexStart;
    const unsigned int firstIndex  = dst->m_indexStart;

    // vertices
    {
        const unsigned int tailBytes =
            stride * (t->m_vertexCount - src->m_vertexCount - firstVertex);
        char* const scratch = static_cast<char*>(::renddx9abi::engineAlloc(tailBytes));
        char* const base    = static_cast<char*>(t->m_vertexData);

        const unsigned int headBytes = stride * (src->m_vertexStart - firstVertex);
        ::memcpy(scratch, base + stride * firstVertex, headBytes);

        const unsigned int afterSrc = stride * (src->m_vertexStart + src->m_vertexCount);
        ::memcpy(scratch + headBytes, base + afterSrc, stride * t->m_vertexCount - afterSrc);

        const unsigned int movedBytes = stride * src->m_vertexCount;
        ::memmove(base + stride * firstVertex, base + stride * src->m_vertexStart, movedBytes);
        ::memcpy(base + stride * firstVertex + movedBytes, scratch, tailBytes);
        ::renddx9abi::engineFree(scratch);
    }

    // indices
    {
        unsigned short* const base = t->m_indexData;
        const unsigned int tailBytes =
            2u * (t->m_indexCount - firstIndex - 3u * src->m_primitiveCount);
        char* const scratch = static_cast<char*>(::renddx9abi::engineAlloc(tailBytes));

        const unsigned int headBytes = 2u * (src->m_indexStart - firstIndex);
        ::memcpy(scratch, base + firstIndex, headBytes);

        const unsigned int afterSrc = 2u * (src->m_indexStart + 3u * src->m_primitiveCount);
        ::memcpy(scratch + headBytes,
                 reinterpret_cast<const char*>(base) + afterSrc,
                 2u * t->m_indexCount - afterSrc);

        const unsigned int movedBytes = 6u * src->m_primitiveCount;
        ::memmove(base + firstIndex, base + src->m_indexStart, movedBytes);
        ::memcpy(reinterpret_cast<char*>(base + firstIndex) + movedBytes, scratch, tailBytes);
        ::renddx9abi::engineFree(scratch);
    }
}

// sub_10119950 - move material `from` to slot `to` in the record vector, re-basing every start on
// the way. The shipped body keeps a full 516-byte copy of the moved material on its own stack and
// re-bases that copy's two start fields before re-inserting it - `v10`/`v11` in the decompiler's
// frame are the scratch material's +0x180 / +0x184, not arguments to the shift.
void mergeMoveMaterial(StaticMeshTemplate* t, unsigned int geom, unsigned int lod,
                       unsigned int to, unsigned int from)
{
    vc71::vector_layout& materials = materialVector(t, geom, lod);

    StaticMeshMaterial scratch(*materialAt(materials, from));
    const StaticMeshMaterial* const previous = materialAt(materials, to - 1);

    // Everything after the moved material slides back by its own run.
    mergeShiftStarts(t, geom, lod, from + 1u,
                     -static_cast<int>(scratch.m_primitiveCount),
                     -static_cast<int>(scratch.m_vertexCount));

    // sub_1011F810 - erase(first + from): assign the tail down one slot, destroy the duplicate at
    // the end, retract `last`.
    for (char* k = materials.first + 516u * from + 516; k != materials.last; k += 516)
        *reinterpret_cast<StaticMeshMaterial*>(k - 516) =
            *reinterpret_cast<const StaticMeshMaterial*>(k);
    reinterpret_cast<StaticMeshMaterial*>(materials.last - 516)->destroy(0);
    materials.last -= 516;

    // The moved copy takes the slot immediately after `to - 1`.
    scratch.m_vertexStart = previous->m_vertexStart + previous->m_vertexCount;
    scratch.m_indexStart  = previous->m_indexStart + 3u * previous->m_primitiveCount;

    // sub_101207D0 -> sub_10120360 - insert(first + to, scratch).
    {
        const unsigned int size =
            static_cast<unsigned int>((materials.last - materials.first) / 516);
        const unsigned int capacity =
            materials.first
                ? static_cast<unsigned int>((materials.end - materials.first) / 516)
                : 0u;
        if (capacity < size + 1u)
        {
            unsigned int grown = capacity + (capacity >> 1);
            if (grown < size + 1u)
                grown = size + 1u;
            char* const buffer = static_cast<char*>(::renddx9abi::engineAlloc(grown * 516u));
            for (unsigned int k = 0; k < size; ++k)
            {
                new (buffer + k * 516u) StaticMeshMaterial(
                    *reinterpret_cast<StaticMeshMaterial*>(materials.first + k * 516u));
                reinterpret_cast<StaticMeshMaterial*>(materials.first + k * 516u)->destroy(0);
            }
            if (materials.first)
                ::renddx9abi::engineFree(materials.first);
            materials.first = buffer;
            materials.last  = buffer + size * 516u;
            materials.end   = buffer + grown * 516u;
        }
        new (materials.last) StaticMeshMaterial();
        materials.last += 516;
        for (char* k = materials.last - 516; k != materials.first + 516u * to; k -= 516)
            *reinterpret_cast<StaticMeshMaterial*>(k) =
                *reinterpret_cast<const StaticMeshMaterial*>(k - 516);
        *reinterpret_cast<StaticMeshMaterial*>(materials.first + 516u * to) = scratch;
    }

    // ... and everything after it slides forward again by the same run.
    mergeShiftStarts(t, geom, lod, to + 1u,
                     static_cast<int>(scratch.m_primitiveCount),
                     static_cast<int>(scratch.m_vertexCount));

    scratch.destroy(0);
}

// sub_10119A80 - fold material `*j` into material `i`.
bool mergeFold(StaticMeshTemplate* t, unsigned int geom, unsigned int lod,
               unsigned int i, unsigned int* j)
{
    if (*j - i > 1u)
    {
        mergeRotateGeometry(t, geom, lod, i + 1u, *j);
        mergeMoveMaterial(t, geom, lod, i + 1u, *j);
        *j = i + 1u;
    }

    const vc71::vector_layout& materials = materialVector(t, geom, lod);
    StaticMeshMaterial* const dst = materialAt(materials, i);
    StaticMeshMaterial* const src = materialAt(materials, *j);

    mergeShiftIndices(t, static_cast<short>(dst->m_vertexCount),
                      static_cast<int>(src->m_indexStart),
                      static_cast<int>(src->m_primitiveCount));

    dst->m_primitiveCount += src->m_primitiveCount;
    dst->m_vertexCount    += src->m_vertexCount;

    meshBoundsExpand(dst->m_bounds, src->m_bounds.min);
    meshBoundsExpand(dst->m_bounds, src->m_bounds.max);
    return true;
}

} // anonymous namespace

void StaticMeshTemplate::mergeMaterials()
{
    const unsigned int geoms = geomCount(this);
    for (unsigned int geom = 0; geom < geoms; ++geom)
    {
        const unsigned int lods = lodCount(this, geom);
        for (unsigned int lod = 0; lod < lods; ++lod)
        {
            vc71::vector_layout& materials = materialVector(this, geom, lod);
            const unsigned int mask = staticMeshQualityMask();

            unsigned int i = 0;
            for (char* a = materials.first; a != materials.last; a += 516, ++i)
            {
                unsigned int j = i + 1;
                for (char* b = a + 516; b != materials.last; b += 516, ++j)
                {
                    const StaticMeshMaterial* const left  =
                        reinterpret_cast<const StaticMeshMaterial*>(a);
                    const StaticMeshMaterial* const right =
                        reinterpret_cast<const StaticMeshMaterial*>(b);
                    const ShaderMaterial* const ls = left->m_shaderMaterial;
                    const ShaderMaterial* const rs = right->m_shaderMaterial;

                    const unsigned int flags = mask & g_shaderComboFlags[ls->shaderCombo];
                    if (flags == (mask & g_shaderComboFlags[rs->shaderCombo])
                        && left->m_alphaMode == right->m_alphaMode
                        && ((flags & 0x002u) == 0 || ls->textures[1] == rs->textures[1])
                        && ((flags & 0x080u) == 0 || ls->textures[5] == rs->textures[5])
                        && ((flags & 0x004u) == 0 || ls->textures[2] == rs->textures[2])
                        && ((flags & 0x008u) == 0 || ls->textures[3] == rs->textures[3])
                        && ((flags & 0x100u) == 0 || ls->textures[6] == rs->textures[6])
                        && ((flags & 0x010u) == 0 || ls->textures[4] == rs->textures[4])
                        && mergeFold(this, geom, lod, i, &j))
                    {
                        ++g_staticMeshMergedCount[lod];

                        // Release the shader, shift by assignment, then destroy the duplicate tail element.
                        StaticMeshMaterial* const dead = materialAt(materials, j);
                        if (dead->m_shaderMaterial)
                            shaderMaterialRelease(dead->m_shaderMaterial);
                        for (char* k = materials.first + 516u * j + 516; k != materials.last; k += 516)
                            *reinterpret_cast<StaticMeshMaterial*>(k - 516) =
                                *reinterpret_cast<const StaticMeshMaterial*>(k);
                        reinterpret_cast<StaticMeshMaterial*>(materials.last - 516)->destroy(0);
                        materials.last -= 516;

                        // Shipped: the inner cursor goes back to `i`, and the ++ below moves it to
                        // the slot the erase just filled.
                        j = i;
                        b = a;
                    }
                }
            }
        }
    }
}


// buildVertexBuffers - 0x10117410, 3421 bytes: the largest body in this file.

namespace {

// sub_10049F20 - the 10:10:10 packed normal.
unsigned int packNormal10(const float* n)
{
    return (static_cast<int>(n[0] * 511.0f) & 0x3FF)
         | ((((static_cast<int>(n[1] * 511.0f) & 0x3FF)
              | ((static_cast<int>(n[2] * 511.0f) & 0x3FF) << 10)) << 10));
}

// sub_10049E80 - the D3DCOLOR packed normal.
unsigned int packNormalColor(const float* n)
{
    const unsigned int x = static_cast<unsigned int>(127 - static_cast<int>(n[0] * -127.0f));
    const unsigned int y = static_cast<unsigned int>(127 - static_cast<int>(n[1] * -127.0f));
    const unsigned int z = static_cast<unsigned int>(127 - static_cast<int>(n[2] * -127.0f));
    return (x << 16) | ((y & 0xFF) << 8) | (z & 0xFF) | 0xFF000000u;
}

// Packs a normal using the encoding supported by device feature 14 (capability bit 0x80).
unsigned int packNormal(const float* n)
{
    const unsigned int featureMask = *reinterpret_cast<const unsigned int*>(
        static_cast<const unsigned char*>(::dice::hfe::dx9rend::g_deviceCaps) + 0xEC);
    return (featureMask & 0x80u) != 0 ? packNormal10(n) : packNormalColor(n);
}

// sub_100777A0 - Vec3 normalize in place, with the shipped |v|^2 within-epsilon short circuit.
void normalizeInPlace(float* v)
{
    const float lengthSquared = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (::fabs(static_cast<double>(lengthSquared) - 1.0) < 0.00000011920929)
        return;
    if (::fabs(static_cast<double>(lengthSquared)) < 0.00000011920929)
    {
        v[0] = 0.0f; v[1] = 0.0f; v[2] = 0.0f;
        return;
    }
    const float inverse = static_cast<float>(1.0 / ::sqrt(static_cast<double>(lengthSquared)));
    v[0] *= inverse;
    v[1] *= inverse;
    v[2] *= inverse;
}

inline float maxAbs(float a, float b)
{
    const float fa = static_cast<float>(::fabs(static_cast<double>(a)));
    const float fb = static_cast<float>(::fabs(static_cast<double>(b)));
    return fa >= fb ? fa : fb;
}

const float kMinCompressExtent = 0.00000011920929f;

} // anonymous namespace

void StaticMeshTemplate::buildVertexBuffers()
{
    // the declaration wrapper's D3D object sits at wrapper+8, and GetDeclaration is
    // slot +0x10 of IDirect3DVertexDeclaration9. BufferManager.cpp keeps its wrapper private, so
    // the field is reached by documented raw offset.
    D3DVERTEXELEMENT9 elements[256];
    {
        IDirect3DVertexDeclaration9* const decl =
            *reinterpret_cast<IDirect3DVertexDeclaration9* const*>(
                static_cast<const unsigned char*>(m_vertexDeclaration) + 8);
        decl->GetDeclaration(elements, &m_declElementCount);
    }

    const bool compress = this->getCompressVertexData();
    const int  maxRepeat = this->getMaxTextureRepeat();
    const float uvScale = 32767.0f / static_cast<float>(maxRepeat);

    if (compress)
    {
        float z = maxAbs(m_bounds.max.z, m_bounds.min.z);
        float y = maxAbs(m_bounds.max.y, m_bounds.min.y);
        float x = maxAbs(m_bounds.max.x, m_bounds.min.x);
        if (z < kMinCompressExtent) z = kMinCompressExtent;
        if (y < kMinCompressExtent) y = kMinCompressExtent;
        if (x < kMinCompressExtent) x = kMinCompressExtent;

        // sub_1007B220 returns the lazily-built shared Vec3(1,1,1) at dword_10247BFC.
        m_compressScale.z = (1.0f / z) * 32767.0f;
        m_compressScale.x = (1.0f / x) * 32767.0f;
        m_compressScale.y = (1.0f / y) * 32767.0f;

        m_decompressScale.x = 1.0f / m_compressScale.x;
        m_decompressScale.z = 1.0f / m_compressScale.z;
        m_decompressScale.y = 1.0f / m_compressScale.y;
        m_uvDecompress = 1.0f / uvScale;
    }
    else
    {
        m_compressScale.x = 1.0f; m_compressScale.y = 1.0f; m_compressScale.z = 1.0f;
        m_decompressScale.x = 1.0f; m_decompressScale.y = 1.0f; m_decompressScale.z = 1.0f;
        m_uvDecompress = 1.0f;
    }

    // classify the declaration's elements from index 3 on
    // The source vertex begins with position (12 B), normal (12 B) and four unread bytes; the first
    // texcoord therefore lands at byte 28, which is what the cursor starts at.
    // Both lists are bounded by the element array, so they cannot overflow where the shipped
    // vectors would merely grow.
    unsigned int texcoordOffsets[256];
    unsigned int texcoordCount = 0;
    unsigned int tangentOffsets[256];
    unsigned int tangentCount = 0;
    {
        unsigned int cursor = 28;
        for (unsigned int i = 3; i < m_declElementCount; ++i)
        {
            switch (elements[i].Usage)
            {
            case D3DDECLUSAGE_TEXCOORD:                  // 5
                texcoordOffsets[texcoordCount++] = cursor;
                cursor += 8;
                break;
            case D3DDECLUSAGE_TANGENT:                   // 6
                tangentOffsets[tangentCount++] = cursor;
                cursor += 12;
                break;
            case D3DDECLUSAGE_BINORMAL:                  // 7
                // sub_100E22D0 pushes onto the tangent list as well.
                tangentOffsets[tangentCount++] = cursor;
                cursor += 12;
                break;
            default:
                break;
            }
        }
    }

    m_extraUvCount      = static_cast<int>(texcoordCount) - 1;
    const unsigned int tangentStrideRaw = 12u * tangentCount;
    m_hasTwoTangentSets = static_cast<unsigned char>(tangentCount == 2);

    const unsigned int stream0Stride = compress ? 16u : 32u;

    // create the buffers
    {
        vc71::string name("StaticMesh : ");
        name += m_name;
        m_vertexStream = ::dice::hfe::dx9rend::createVertexBuffer(
            ::dice::hfe::dx9rend::g_bufferManager, m_vertexCount, stream0Stride, name, 8);
    }

    // The three gating masks, in the shipped order {4, 8, 16}: an extra uv set beyond the first
    // three is always created, the first three only when the template's used-flag set matches its
    // whole-flag set under that bit.
    {
        static const unsigned int kUvGateMasks[3] = { 4u, 8u, 16u };
        for (unsigned int i = 0; i < texcoordCount - 1u; ++i)
        {
            unsigned int gated = texcoordCount - 2u;
            if (gated > 3u)
                gated = 3u;
            if (i >= gated ||
                (kUvGateMasks[i] & m_shaderFlagsAll) == (kUvGateMasks[i] & m_shaderFlagsUsed))
            {
                vc71::string name("StaticMesh : ");
                name += m_name;
                m_uvStreams[i] = ::dice::hfe::dx9rend::createVertexBuffer(
                    ::dice::hfe::dx9rend::g_bufferManager, m_vertexCount,
                    compress ? 4u : 8u, name, 8);
            }
        }
    }

    {
        vc71::string name("StaticMesh : ");
        name += m_name;
        m_tangentStream = ::dice::hfe::dx9rend::createVertexBuffer(
            ::dice::hfe::dx9rend::g_bufferManager, m_vertexCount,
            compress ? 4u * tangentCount : tangentStrideRaw, name, 8);
    }

    // lock everything
    char* const stream0 = static_cast<char*>(
        ::dice::hfe::dx9rend::vertexBufferLock(m_vertexStream, 0, m_vertexCount, 0));
    char* const tangentStream = static_cast<char*>(
        ::dice::hfe::dx9rend::vertexBufferLock(m_tangentStream, 0, m_vertexCount, 0));
    char* uvStreams[5] = { 0, 0, 0, 0, 0 };
    for (unsigned int i = 0; i < texcoordCount - 1u; ++i)
    {
        if (m_uvStreams[i])
            uvStreams[i] = static_cast<char*>(
                ::dice::hfe::dx9rend::vertexBufferLock(m_uvStreams[i], 0, m_vertexCount, 0));
    }

    if (stream0 && tangentStream)
    {
        // vegetation pass 1: per-material normal recompute around the material centroid

        // Vegetation processing visits only geometry 0.
        if (m_isVegetation)
        {
            const vc71::vector_layout& geom0 =
                *reinterpret_cast<const vc71::vector_layout*>(m_materials.first);
            const unsigned int lods =
                geom0.first ? static_cast<unsigned int>((geom0.last - geom0.first) >> 4) : 0u;
            for (unsigned int l = 0; l < lods; ++l)
            {
                const vc71::vector_layout& materials =
                    reinterpret_cast<const vc71::vector_layout*>(geom0.first)[l];
                const unsigned int count = materialCount(materials);
                for (unsigned int m = 0; m < count; ++m)
                {
                    StaticMeshMaterial* const material = materialAt(materials, m);
                    const ShaderMaterial* const shader = material->m_shaderMaterial;
                    if (shader->textures[2])
                        continue;
                    const ::dice::hfe::dx9rend::EngineTexture* const base =
                        static_cast<const ::dice::hfe::dx9rend::EngineTexture*>(shader->textures[1]);
                    if (::strstr(base->name.c_str(), "tile_"))
                        continue;

                    material->m_vegetationNonTiled = 1;
                    if (m_keepVegetationNormals)
                        continue;

                    const unsigned int vertices = material->m_vertexCount;
                    float cx = 0.0f, cy = 0.0f, cz = 0.0f;
                    if (vertices)
                    {
                        const char* cursor = static_cast<const char*>(m_vertexData)
                                           + m_vertexStride * material->m_vertexStart;
                        for (unsigned int v = 0; v < vertices; ++v, cursor += m_vertexStride)
                        {
                            const float* const p = reinterpret_cast<const float*>(cursor);
                            cx += p[0]; cy += p[1]; cz += p[2];
                        }
                        const float inverse = 1.0f / static_cast<float>(vertices);
                        cx *= inverse; cy *= inverse; cz *= inverse;

                        for (unsigned int v = 0; v < vertices; ++v)
                        {
                            float* const p = reinterpret_cast<float*>(
                                static_cast<char*>(m_vertexData)
                                + m_vertexStride * (v + material->m_vertexStart));
                            float n[3];
                            n[0] = p[0] - cx;
                            n[1] = p[1] - cy;
                            n[2] = p[2] - cz;
                            normalizeInPlace(n);
                            p[3] = n[0];
                            p[4] = n[1];
                            p[5] = n[2];
                        }
                    }
                }
            }

            // vegetation pass 2: the 32-byte per-vertex copies into +0x1DC..+0x1E8
            // The shipped body re-tests m_isVegetation here although it is already inside the same
            // guard; kept.
            if (m_isVegetation)
            {
                const unsigned int lods2 =
                    geom0.first ? static_cast<unsigned int>((geom0.last - geom0.first) >> 4) : 0u;
                for (unsigned int l = 0; l < lods2; ++l)
                {
                    const vc71::vector_layout& materials =
                        reinterpret_cast<const vc71::vector_layout*>(geom0.first)[l];
                    const unsigned int count = materialCount(materials);
                    for (unsigned int m = 0; m < count; ++m)
                    {
                        StaticMeshMaterial* const material = materialAt(materials, m);
                        const unsigned int vertices = material->m_vertexCount;
                        material->m_vegetationVertexCount = vertices;

                        char* const copy =
                            static_cast<char*>(::renddx9abi::engineAlloc(32u * vertices));
                        if (copy)
                        {
                            float* fill = reinterpret_cast<float*>(copy);
                            for (unsigned int v = 0; v < vertices; ++v, fill += 8)
                            {
                                for (int k = 0; k < 8; ++k)
                                    fill[k] = -1.0f;
                            }
                        }
                        material->m_vegetationVertices = copy;

                        for (unsigned int v = 0; v < vertices; ++v)
                        {
                            const unsigned int* const src = reinterpret_cast<const unsigned int*>(
                                static_cast<const char*>(m_vertexData)
                                + m_vertexStride * (v + material->m_vertexStart));
                            unsigned int* const dst =
                                reinterpret_cast<unsigned int*>(copy + 32u * v);
                            dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
                            dst[3] = src[3]; dst[4] = src[4]; dst[5] = src[5];
                            dst[6] = src[7]; dst[7] = src[8];        // the uv set at byte 28
                        }
                    }
                }
            }
        }

        // the packing loop
        // Both cursors advance every iteration regardless of which branch runs - shipped.
        char* compressedCursor   = stream0 + 6;
        char* uncompressedCursor = stream0;
        unsigned int uvCursor    = 0;      // the byte offset into every extra uv stream
        char* tangentCursor      = tangentStream;

        for (unsigned int v = 0, sourceOffset = 0; v < m_vertexCount; ++v)
        {
            const char* const source = static_cast<const char*>(m_vertexData) + sourceOffset;

            if (compress)
            {
                const float* const p = reinterpret_cast<const float*>(source);
                short* const out = reinterpret_cast<short*>(compressedCursor);
                out[-3] = static_cast<short>(static_cast<int>(p[0] * m_compressScale.x));
                out[-2] = static_cast<short>(static_cast<int>(p[1] * m_compressScale.y));
                out[-1] = static_cast<short>(static_cast<int>(p[2] * m_compressScale.z));
                out[0]  = static_cast<short>(m_isRoad
                              ? 1
                              : *reinterpret_cast<const unsigned char*>(source + 26));

                *reinterpret_cast<unsigned int*>(compressedCursor + 2) =
                    packNormal(reinterpret_cast<const float*>(source + 12));

                unsigned int sourceUv = 28;
                for (unsigned int t = 0; t < texcoordCount; ++t, sourceUv += 8)
                {
                    const float* const uv = reinterpret_cast<const float*>(source + sourceUv);
                    const short u = static_cast<short>(
                        static_cast<int>(::floor(uvScale * uv[0] + 0.5f)));
                    const short w = static_cast<short>(
                        static_cast<int>(::floor(uvScale * uv[1] + 0.5f)));
                    if (t == 0)
                    {
                        *reinterpret_cast<short*>(compressedCursor + 6) = u;
                        *reinterpret_cast<short*>(compressedCursor + 8) = w;
                    }
                    else if (uvStreams[t - 1])
                    {
                        short* const slot =
                            reinterpret_cast<short*>(uvStreams[t - 1] + uvCursor);
                        slot[0] = u;
                        slot[1] = w;
                    }
                }

                const char* const tangentSource = source + sourceUv;
                *reinterpret_cast<unsigned int*>(tangentCursor) =
                    packNormal(reinterpret_cast<const float*>(tangentSource));
                tangentCursor += 4;
                if (tangentCount > 1)
                {
                    *reinterpret_cast<unsigned int*>(tangentCursor) =
                        packNormal(reinterpret_cast<const float*>(tangentSource + 12));
                    tangentCursor += 4;
                }
                uvCursor += 4;
            }
            else
            {
                const unsigned int* const src = reinterpret_cast<const unsigned int*>(source);
                unsigned int* const dst = reinterpret_cast<unsigned int*>(uncompressedCursor);
                dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
                dst[3] = src[3]; dst[4] = src[4]; dst[5] = src[5];

                unsigned int sourceUv = 28;
                const unsigned int* uv = src + 7;
                for (unsigned int t = 0; t < texcoordCount; ++t, sourceUv += 8, uv += 2)
                {
                    if (t == 0)
                    {
                        dst[6] = uv[0];
                        dst[7] = uv[1];
                    }
                    else if (uvStreams[t - 1])
                    {
                        unsigned int* const slot =
                            reinterpret_cast<unsigned int*>(uvStreams[t - 1] + uvCursor);
                        slot[0] = uv[0];
                        slot[1] = uv[1];
                    }
                }
                uvCursor += 8;
                ::memcpy(tangentCursor, source + sourceUv, tangentStrideRaw);
                tangentCursor += tangentStrideRaw;
            }

            sourceOffset       += m_vertexStride;
            compressedCursor   += 16;
            uncompressedCursor += 32;
        }

        ::dice::hfe::dx9rend::vertexBufferUnlock(m_vertexStream);
        ::dice::hfe::dx9rend::vertexBufferUnlock(m_tangentStream);
        for (unsigned int i = 0; i < texcoordCount - 1u; ++i)
        {
            if (m_uvStreams[i])
                ::dice::hfe::dx9rend::vertexBufferUnlock(m_uvStreams[i]);
        }
    }

    // m_vertexData IS freed BUT NOT nulled - only the declaration pointer is cleared. Shipped

    ::dice::hfe::dx9rend::vertexDeclarationRelease(m_vertexDeclaration);
    void* const vertexData = m_vertexData;
    m_vertexDeclaration = 0;
    ::renddx9abi::engineFree(vertexData);
}


// Copies index runs into vegetation materials in geometry 0, then builds the base buffer.
// Only primitive types 4 and 5 assign a count; other types reuse the previous value,
// including an uninitialized value for the first material. Allocations are not checked.

void StaticMeshTemplate::buildIndexBuffer()
{
    if (m_isVegetation)
    {
        const vc71::vector_layout& geom0 =
            *reinterpret_cast<const vc71::vector_layout*>(m_materials.first);
        int indexCount = 0;      // var_10: the shipped slot is NOT initialised - see the note above
        unsigned int lod = 0;
        while (geom0.first &&
               lod < static_cast<unsigned int>((geom0.last - geom0.first) >> 4))
        {
            const vc71::vector_layout& materials =
                reinterpret_cast<const vc71::vector_layout*>(geom0.first)[lod];
            const unsigned int count = materialCount(materials);
            for (unsigned int i = 0; i < count; ++i)
            {
                StaticMeshMaterial* const material = materialAt(materials, i);
                if (m_primitiveType == 4)
                    indexCount = 3 * static_cast<int>(material->m_primitiveCount);
                else if (m_primitiveType == 5)
                    indexCount = static_cast<int>(material->m_primitiveCount) + 2;

                material->m_vegetationIndexCount = static_cast<unsigned int>(indexCount);
                void* const copy = ::renddx9abi::engineAlloc(2u * indexCount);
                material->m_vegetationIndices = static_cast<unsigned short*>(copy);
                ::memcpy(copy, m_indexData + material->m_indexStart, 2u * indexCount);
            }
            ++lod;
        }
    }

    MeshTemplate::buildIndexBuffer();     // sub_10191A80, the unconditional tail call
}


// finalizeMaterials - 0x10118B00 (slot +0xD0)

namespace {

// Writes four normal-unpack constants matching the device's packed-normal format.
// DEC3N uses (1,0), D3DCOLOR uses (2,-1); both use (1/32768,0) for the second pair.
// out must hold four floats and device caps must be initialized.
// [RETIRE-WHEN: RW-ra-normal-format-writer] Replace on-demand calculation when the shared
// reset/format writer is implemented and verified, including both packed-8 descriptor updates.
void raNormalUnpackConstants(float out[4])
{
    const unsigned int featureMask = *reinterpret_cast<const unsigned int*>(
        static_cast<const unsigned char*>(::dice::hfe::dx9rend::g_deviceCaps) + 0xEC);

    if ((featureMask & 0x80u) != 0)
    {
        out[0] = 1.0f;
        out[1] = 0.0f;
    }
    else
    {
        out[0] = 2.0f;
        out[1] = -1.0f;
    }

    out[2] = 3.0517578125e-05f;   // 0x38000000 = 2^-15, both arms
    out[3] = 0.0f;
}


// The two push_backs setupTechnique uses.  Both are the ordinary MSVC vector growth this file
// already reproduces five times over (see the block above resizeMaterialVector): grow to 1.5x, or
// to exactly what is needed when that is larger, with sub_1005F320 / sub_1005F360 as the allocator.

// sub_1004D3E0 (Ra) and sub_1008A9B0 (here) are the same one-dword push written twice in two
// translation units; sub_1004CD70 is the same thing again with its capacity test inlined at the
// call site.  One body serves all three.

void pointerVectorPush(vc71::vector_layout& v, const void* value)
{
    const unsigned int size =
        v.first ? static_cast<unsigned int>((v.last - v.first) >> 2) : 0u;
    const unsigned int capacity =
        v.first ? static_cast<unsigned int>((v.end - v.first) >> 2) : 0u;

    if (size == capacity)
    {
        unsigned int grown = capacity + (capacity >> 1);
        if (grown < size + 1u)
            grown = size + 1u;
        char* const buffer = static_cast<char*>(::renddx9abi::engineAlloc(grown * 4u));
        if (size)
            ::memcpy(buffer, v.first, size * 4u);
        if (v.first)
            ::renddx9abi::engineFree(v.first);
        v.first = buffer;
        v.last  = buffer + size * 4u;
        v.end   = buffer + grown * 4u;
    }

    *reinterpret_cast<const void**>(v.last) = value;
    v.last += 4;
}

// sub_1011E7E0 - push_back on the vector OF vectors at material+0x04.  The element is
// copy-constructed (sub_1011D490), which is why every caller below still owns, and frees, the local
// it passed.
void elementListPush(vc71::vector_layout& v, const vc71::vector_layout& list)
{
    const unsigned int size =
        v.first ? static_cast<unsigned int>((v.last - v.first) >> 4) : 0u;
    const unsigned int capacity =
        v.first ? static_cast<unsigned int>((v.end - v.first) >> 4) : 0u;

    if (size == capacity)
    {
        unsigned int grown = capacity + (capacity >> 1);
        if (grown < size + 1u)
            grown = size + 1u;
        char* const buffer = static_cast<char*>(::renddx9abi::engineAlloc(grown * 16u));
        if (size)
            ::memcpy(buffer, v.first, size * 16u);
        if (v.first)
            ::renddx9abi::engineFree(v.first);
        v.first = buffer;
        v.last  = buffer + size * 16u;
        v.end   = buffer + grown * 16u;
    }

    vc71::vector_layout* const fresh = reinterpret_cast<vc71::vector_layout*>(v.last);
    fresh->alloc = 0;
    fresh->first = 0;
    fresh->last  = 0;
    fresh->end   = 0;

    const unsigned int bytes = static_cast<unsigned int>(list.last - list.first);
    if (bytes)
    {
        char* const copy = static_cast<char*>(::renddx9abi::engineAlloc(bytes));
        ::memcpy(copy, list.first, bytes);
        fresh->first = copy;
        fresh->last  = copy + bytes;
        fresh->end   = copy + bytes;
    }

    v.last += 16;
}

// Builds parallel vertex-element lists and vertex-buffer references for a material.
// Each element list is copied; buffer entries hold wrapper values, not addresses of slots.
// Packed and unpacked descriptors share element IDs and differ in declaration type.
void staticMeshMaterialSetupTechnique(StaticMeshMaterial* material)
{
    namespace Ra = ::dice::hfe::dx9rend::Ra;

    // the quality mask AND the combo's flag word.  Both halves are already recovered
    // in this file; the shipped expression is the one finalizeMaterials' sort also uses.
    const unsigned int quality =
        staticMeshQualityMask() & g_shaderComboFlags[material->m_shaderMaterial->shaderCombo];

    StaticMeshTemplate* const tmpl = material->m_template;          // +0x1F4

    // template vslot +0x7C.  ONE call, and every arm below reads the answer - the shipped body
    // keeps it in a byte on the frame rather than asking again.
    const bool compressed = tmpl->getCompressVertexData();

    // the base stream: position, normal, base uv.  Always.
    vc71::vector_layout baseElements = { 0, 0, 0, 0 };
    if (compressed)
    {
        pointerVectorPush(baseElements, &Ra::g_desc_1023ED9C);      // Vtx_PositionPacked
        pointerVectorPush(baseElements, &Ra::g_desc_1023EE14);      // Vtx_NormalPacked8
        pointerVectorPush(baseElements, &Ra::g_desc_1023EDEC);      // Vtx_TBasePacked
    }
    else
    {
        pointerVectorPush(baseElements, &Ra::g_desc_1023EF04);      // Vtx_Position
        pointerVectorPush(baseElements, &Ra::g_desc_1023EF7C);      // Vtx_Normal
        pointerVectorPush(baseElements, &Ra::g_desc_1023ED4C);      // Vtx_TBase
    }
    elementListPush(material->m_vector04, baseElements);
    pointerVectorPush(material->m_vector14, tmpl->m_vertexStream);         // +0x154

    // the detail uv set, on quality bit 2
    if ((quality & 0x004u) != 0)
    {
        vc71::vector_layout detail = { 0, 0, 0, 0 };
        pointerVectorPush(detail, compressed ? &Ra::g_desc_1023EE8C     // Vtx_TDetailPacked
                                             : &Ra::g_desc_1023F01C);   // Vtx_TDetail
        elementListPush(material->m_vector04, detail);
        pointerVectorPush(material->m_vector14, tmpl->m_uvStreams[tmpl->m_streamIndexA]);
        if (detail.first)
            ::renddx9abi::engineFree(detail.first);
    }

    // the tangent stream.  Always.
    vc71::vector_layout tangentElements = { 0, 0, 0, 0 };
    pointerVectorPush(tangentElements, compressed ? &Ra::g_desc_1023EEB4   // Vtx_TangentSpacePacked8
                                                  : &Ra::g_desc_1023EFF4); // Vtx_TangentSpace
    elementListPush(material->m_vector04, tangentElements);
    pointerVectorPush(material->m_vector14, tmpl->m_tangentStream);       // +0x16C

    // the dirt uv set, on quality bit 3
    if ((quality & 0x008u) != 0)
    {
        vc71::vector_layout dirt = { 0, 0, 0, 0 };
        pointerVectorPush(dirt, compressed ? &Ra::g_desc_1023EFA4       // Vtx_TDirtPacked
                                           : &Ra::g_desc_1023EFCC);     // Vtx_TDirt
        elementListPush(material->m_vector04, dirt);
        pointerVectorPush(material->m_vector14, tmpl->m_uvStreams[tmpl->m_streamIndexB]);
        if (dirt.first)
            ::renddx9abi::engineFree(dirt.first);
    }

    // the crack uv set, on quality bit 4
    if ((quality & 0x010u) != 0)
    {
        vc71::vector_layout crack = { 0, 0, 0, 0 };
        pointerVectorPush(crack, compressed ? &Ra::g_desc_1023EE3C      // Vtx_TCrackPacked
                                            : &Ra::g_desc_1023F044);    // Vtx_TCrack
        elementListPush(material->m_vector04, crack);
        pointerVectorPush(material->m_vector14, tmpl->m_uvStreams[tmpl->m_streamIndexC]);
        if (crack.first)
            ::renddx9abi::engineFree(crack.first);
    }

    // Lightmap UVs require finished materials, the lighting/water/expansion gate,
    // and a separate non-road check in this order.
    if (tmpl->m_finished)
    {
        const bool wantLightmap =
            tmpl->m_isWater != 0
            || ::dice::hfe::dx9rend::rendererGetVideoOptions()->getQuality(2) > 1  // +0x74, +0x04
            || ::dice::hfe::dx9rend::rendererIsExpansionPack();                    // +0xAC
        if (wantLightmap)
        {
            if (!tmpl->m_isRoad)
            {
                vc71::vector_layout lightmap = { 0, 0, 0, 0 };
                pointerVectorPush(lightmap, compressed ? &Ra::g_desc_1023EE64   // Vtx_TLightMapPacked
                                                       : &Ra::g_desc_1023EF2C); // Vtx_TLightMap
                elementListPush(material->m_vector04, lightmap);

                // `&template->dword[extraUvCount + 85]`, which is &m_uvStreams[extraUvCount - 1]
                // because m_uvStreams starts one dword after m_vertexStream.  The off-by-one is the
                // shipped arithmetic, not a correction of it.
                pointerVectorPush(material->m_vector14,
                                  tmpl->m_uvStreams[tmpl->m_extraUvCount - 1]);
                if (lightmap.first)
                    ::renddx9abi::engineFree(lightmap.first);
            }
        }
    }

    if (tangentElements.first)
        ::renddx9abi::engineFree(tangentElements.first);
    if (baseElements.first)
        ::renddx9abi::engineFree(baseElements.first);
}

// sub_101158B0 - bind the shader record's textures into the parameter block, set AlphaTest, fill
// ParallaxScaleBias when the combo carries bit 5 (0x20), and publish ObjRadius from the template's
// bounds.
void staticMeshMaterialBindTextures(StaticMeshMaterial* material, int geom, int lod, int index)
{
    const ShaderMaterial* const shader = material->m_shaderMaterial;
    RaMaterialParameters& p = params(material);

    // The first slot is assigned inline by the shipped body rather than through sub_10031180, but
    // with the identical release/assign/addRef shape.
    {
        void* const wanted = shader->textures[1];
        if (p.diffuseMap.value != wanted)
        {
            ::dice::hfe::dx9rend::EngineTexture* const old =
                static_cast<::dice::hfe::dx9rend::EngineTexture*>(p.diffuseMap.value);
            if (old && --old->refCount == 0)
            {
                ::dice::hfe::dx9rend::textureDtor(old);
                ::renddx9abi::engineFree(old);
            }
            p.diffuseMap.value = wanted;
            if (wanted)
                ::dice::hfe::dx9rend::textureAddRef(
                    static_cast<::dice::hfe::dx9rend::EngineTexture*>(wanted));
        }
    }

    // sub_10031180 for slots 2..6 - and only when the shader record actually holds one, which is
    // the shipped guard (a null slot leaves the holder untouched rather than clearing it).
    RaParamTexture* const holders[5] =
        { &p.detailMap, &p.dirtMap, &p.crackMap, &p.normalMap, &p.crackNormalMap };
    for (int i = 0; i < 5; ++i)
    {
        if (shader->textures[2 + i])
            raAssignTextureValue(*holders[i], shader->textures[2 + i]);
    }

    p.alphaTest.value =
        static_cast<unsigned char>(material->m_alphaMode == 2 || material->m_alphaMode == 1);

    if ((g_shaderComboFlags[shader->shaderCombo] & 0x20u) != 0)
    {
        // sub_10084660 - {1/width, 1/height} of the detail map, scaled by the template's material
        // value, and then the bias pair at -0.5 * each.
        const ::dice::hfe::dx9rend::EngineTexture* const detail =
            static_cast<const ::dice::hfe::dx9rend::EngineTexture*>(shader->textures[2]);
        const unsigned int* const dims = ::dice::hfe::dx9rend::textureDims(detail);
        const float invW = static_cast<float>(1.0 / static_cast<double>(dims[0]));
        const float invH = static_cast<float>(1.0 / static_cast<double>(dims[1]));

        const float value = material->m_template->getMaterialValue(
            geom, lod, static_cast<unsigned char>(index));
        const float sx = invW * value;
        const float sy = invH * value;
        p.parallaxScaleBias.value[3] = sy * -0.5f;
        p.parallaxScaleBias.value[2] = sx * -0.5f;
        p.parallaxScaleBias.value[0] = sx;
        p.parallaxScaleBias.value[1] = sy;
    }

    StaticMeshTemplate* const owner = material->m_template;
    if (owner->m_bounds.dirty)
    {
        meshBoundsFinalize(owner->m_bounds);
        p.objRadius.value = owner->m_bounds.radius;
    }
    else
    {
        p.objRadius.value = owner->m_bounds.radius;
    }
}

} // anonymous namespace

void StaticMeshTemplate::finalizeMaterials()
{
    const unsigned int geoms = geomCount(this);

    // The three extra uv stream indices, allocated in flag order: detail, dirt, crack.
    int next = 0;
    if ((m_shaderFlagsAll & 4u) != 0)
    {
        m_streamIndexA = 0;
        next = 1;
    }
    if ((m_shaderFlagsAll & 8u) != 0)
        m_streamIndexB = next++;
    if ((m_shaderFlagsAll & 0x10u) != 0)
        m_streamIndexC = next;

    for (unsigned int geom = 0; geom < geoms; ++geom)
    {
        const unsigned int lods = lodCount(this, geom);
        for (unsigned int lod = 0; lod < lods; ++lod)
        {
            const vc71::vector_layout& materials = materialVector(this, geom, lod);
            for (unsigned int i = 0; i < materialCount(materials); ++i)
            {
                StaticMeshMaterial* const material = materialAt(materials, i);
                material->m_template = this;
                material->m_dword1F0 = m_primitiveType;

                staticMeshMaterialSetupTechnique(material);
                staticMeshMaterialBindTextures(material, static_cast<int>(geom),
                                               static_cast<int>(lod), static_cast<int>(i));

                RaMaterialParameters& p = params(material);
                p.posUnpack.value[1] = m_decompressScale.y;
                p.posUnpack.value[0] = m_decompressScale.x;
                p.posUnpack.value[2] = m_decompressScale.z;
                p.texUnpack.value    = m_uvDecompress;

                if (this->getCompressVertexData())
                {
                    float constants[4];
                    raNormalUnpackConstants(constants);
                    if (this->slot_0x84())
                    {
                        p.normalUnpack.value[0] = constants[2];
                        p.normalUnpack.value[1] = constants[3];
                    }
                    else
                    {
                        p.normalUnpack.value[0] = constants[0];
                        p.normalUnpack.value[1] = constants[1];
                    }
                }
                else
                {
                    p.normalUnpack.value[0] = 1.0f;
                    p.normalUnpack.value[1] = 0.0f;
                }
            }
        }
    }

    // The second pass: eight bytes per vertex for every uv set the material's flags do NOT drive.
    // It uses the RAW combo flags, not the quality-masked ones, and it returns 0 without touching
    // the accumulator when the template has no geoms at all. Both shipped.
    if (!geoms)
        return;

    unsigned int bytes = 0;
    for (unsigned int geom = 0; geom < geoms; ++geom)
    {
        const unsigned int lods = lodCount(this, geom);
        if (!lods)
            continue;
        for (unsigned int lod = 0; lod < lods; ++lod)
        {
            const vc71::vector_layout& materials = materialVector(this, geom, lod);
            const unsigned int count = materialCount(materials);
            for (unsigned int i = 0; i < count; ++i)
            {
                const StaticMeshMaterial* const material = materialAt(materials, i);
                const unsigned int flags =
                    g_shaderComboFlags[material->m_shaderMaterial->shaderCombo];
                int used = (flags & 2u) != 0 ? 1 : 0;
                if ((flags & 4u)    != 0) ++used;
                if ((flags & 8u)    != 0) ++used;
                if ((flags & 0x10u) != 0) ++used;
                bytes += 8u * static_cast<unsigned int>(
                    static_cast<int>(material->m_vertexCount) * (m_extraUvCount - used));
            }
        }
    }
    g_staticMeshStreamBytes += bytes;
}

} // namespace geom

namespace dx9rend { namespace Ra {
// The same retail encoder is called by both mesh families. Keep its implementation owned above.
unsigned int packVertexNormal(const float* normal)
{
    return ::dice::hfe::geom::packNormal(normal);
}
} }

} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_RENDDX9
