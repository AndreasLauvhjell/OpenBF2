// Post-processing settings and device/event-listener owner. [INFERRED] source placement.
// The published singleton points to the interface at owner+8; preserve that adjustment.
// [CHOSEN] named storage wraps the fixed owner layout. Resource acquisition and full-screen
// rendering remain subject to the temporary boundaries beside their implementations.

#ifdef BF2_IMAGE_RENDDX9

#include "../../RendDX9Abi/HfeAbi.h"
#include "../../RendDX9Abi/ModuleCommon.h"
#include "../../RendDX9Abi/BfMemory.h"
#include "../../RendDX9Abi/Frontier.h"
#include "../../RendDX9Abi/Vc71String.h"
#include "RendDX9Base.h"
#include "Texture.h"          // textureReleaseRef for the listener's lost slot
#include "../Geom/Terrain.h"  // the water-fog range setter raises Terrain +0x41C

#include <new>
#include <string.h>

namespace dice {
namespace hfe {
namespace dx9rend {

// Defined in BF2Render.cpp; slots 21/22 read floats out of the live Renderer (this reads it after
// gameplay has started, by which point registerSingleton has written it).
extern IBase* g_Renderer;
extern IBase* g_PostProductionFrameWorkOwner;

namespace {

// Byte-offset field accessors over the raw owner block. `p` is a subobject base (owner+0/4/8) and the
// offset is copied verbatim from the decompile, so these read exactly what the shipped bodies read.
inline unsigned&      U (void* p, unsigned off) { return *reinterpret_cast<unsigned*>(static_cast<char*>(p) + off); }
inline float&         F (void* p, unsigned off) { return *reinterpret_cast<float*>(static_cast<char*>(p) + off); }
inline unsigned char& Bt(void* p, unsigned off) { return *reinterpret_cast<unsigned char*>(static_cast<char*>(p) + off); }


// The published singleton's vtable (owner+8). Every slot `this` is owner+8.


// +0x00 deleting destructor (0x10051700 -> sub_100517C0: shutdown then free-if-flag).
// The owner block is a single 548-byte allocation freed by DX9RendModule::close(); the shipped
// teardown (sub_10050830 shutdown + heap free of the MI-adjusted pointer) is part of the deferred
// render lifetime and is not reached during play (nothing releases the singleton mid-session).
// A logged, non-freeing body: it must not free owner+8 (the allocation base is owner+0).
// [RETIRE-WHEN: the post-production render path is recovered]
void* __fastcall pp_dtor(void* self, void*, int /*flags*/)
{
    static bool noted = false;
    if (!noted) { noted = true;
        ::renddx9abi::logf("PostProductionFrameWork::~ (slot 0): teardown deferred with the render "
                           "path; the owner block is freed by DX9RendModule::close"); }
    return self;
}

// +0x04 / +0x08 / +0x0C / +0x10 / +0x14 / +0x18 / +0x1C / +0x20 - eight paired float accessors.
// (this+40..43 <=> byte 160/164/168/172.)
void  __fastcall pp_set160(void* self, void*, unsigned v) { U(self, 160) = v; }             // +0x04
float __fastcall pp_get160(void* self, void*)             { return F(self, 160); }          // +0x08
void  __fastcall pp_set164(void* self, void*, unsigned v) { U(self, 164) = v; }             // +0x0C
float __fastcall pp_get164(void* self, void*)             { return F(self, 164); }          // +0x10
void  __fastcall pp_set168(void* self, void*, unsigned v) { U(self, 168) = v; }             // +0x14
float __fastcall pp_get168(void* self, void*)             { return F(self, 168); }          // +0x18
void  __fastcall pp_set172(void* self, void*, unsigned v) { U(self, 172) = v; }             // +0x1C
float __fastcall pp_get172(void* self, void*)             { return F(self, 172); }          // +0x20

// +0x24 setStateVec4 (0x1004F7A0): store a vec4 at this+11..14 (byte 44..56); mark a render-state
// dirty flag. That flag lives on dword_1024B930, a render-context global this front does not model;
// the shipped guard is `if (dword_1024B930) *(dword_1024B930+1052)=1`, so with it absent the poke is
// simply skipped - the field store, the observable half, is faithful.
void __fastcall pp_setStateVec4(void* self, void*, const float* state)
{
    U(self, 44) = *reinterpret_cast<const unsigned*>(&state[0]);
    F(self, 48) = state[1];
    F(self, 52) = state[2];
    F(self, 56) = state[3];
}

// +0x28 getStateVec4 (0x1004F7E0).
void* __fastcall pp_getStateVec4(void* self, void*, float* out)
{
    out[0] = F(self, 44);
    out[1] = F(self, 48);
    out[2] = F(self, 52);
    out[3] = F(self, 56);
    return out;
}

// Stores the near/far range.
// [FRONTIER] AmbientIllumination range/color holder updates remain unimplemented.
void __fastcall pp_setRange(void* self, void*, const float* a2)
{
    U(self, 144) = *reinterpret_cast<const unsigned*>(&a2[0]);
    F(self, 148) = a2[1];
    F(self, 152) = a2[2];
    F(self, 156) = a2[3];
}

// +0x30 get the range vec4 (0x1004F8C0).
void* __fastcall pp_getRange(void* self, void*, float* a2)
{
    a2[0] = F(self, 144);
    a2[1] = F(self, 148);
    a2[2] = F(self, 152);
    a2[3] = F(self, 156);
    return a2;
}

// +0x34 setGasMaskBlend (0x1004F330) - THE per-frame caller. Store the blend at this+216; if the
// previous blend was exactly 0, latch the "just became active" byte at this+220.
void __fastcall pp_setBlend(void* self, void*, unsigned a2)
{
    if (F(self, 216) == 0.0f)
        Bt(self, 220) = 1;
    U(self, 216) = a2;
}

// +0x38 getGasMaskBlend (return this[54] == byte 216).
float __fastcall pp_getBlend(void* self, void*) { return F(self, 216); }

// +0x3C / +0x40 two 0x24-byte (3x3 matrix) copies into this+256 / this+292 (0x10051680 / 0x100516A0).
void __fastcall pp_setMat0(void* self, void*, const void* a2) { memcpy(static_cast<char*>(self) + 256, a2, 0x24u); }
void __fastcall pp_setMat1(void* self, void*, const void* a2) { memcpy(static_cast<char*>(self) + 292, a2, 0x24u); }

// +0x44 / +0x48 two dword setters (this+62 / this+63 == byte 248/252).
void __fastcall pp_set248(void* self, void*, unsigned v) { U(self, 248) = v; }
void __fastcall pp_set252(void* self, void*, unsigned v) { U(self, 252) = v; }

// +0x4C / +0x50 two byte flag setters (this+242 / this+241) (0x100516E0 / 0x100516F0).
void __fastcall pp_setByte242(void* self, void*, unsigned char v) { Bt(self, 242) = v; }
void __fastcall pp_setByte241(void* self, void*, unsigned char v) { Bt(self, 241) = v; }

// +0x54 / +0x58 two getters that read the live Renderer (renderer+1852/1856).
float __fastcall pp_getRendererFloat1852(void*, void*) { return F(g_Renderer, 1852); }
float __fastcall pp_getRendererFloat1856(void*, void*) { return F(g_Renderer, 1856); }

// +0x5C setNoiseTextureName (0x1004FC30). The shipped body compares an incoming name against the
// stored one at this+127, releases the old texture at this+126, stores the new name, then acquires a
// new texture via the texture manager (sub_10019260). The acquisition is exactly the deferred half of
// the constructor; with no texture ever loaded (this+126 stays null) there is nothing to release and
// nothing to reacquire, so this is a visible no-op until the render path lands. Logged once.
// [RETIRE-WHEN: the post-production render path is recovered]
void __fastcall pp_setNoiseTextureName(void* self, void*, const void* /*name*/)
{
    (void)self;
    static bool noted = false;
    if (!noted) { noted = true;
        ::renddx9abi::logf("PostProductionFrameWork::setNoiseTextureName (slot 23): screen-distortion "
                           "texture deferred with the render path; name not stored"); }
}

const void* const kPostProdVtable[24] =
{
    reinterpret_cast<void*>(&pp_dtor),                    // +0x00
    reinterpret_cast<void*>(&pp_set160),                  // +0x04
    reinterpret_cast<void*>(&pp_get160),                  // +0x08
    reinterpret_cast<void*>(&pp_set164),                  // +0x0C
    reinterpret_cast<void*>(&pp_get164),                  // +0x10
    reinterpret_cast<void*>(&pp_set168),                  // +0x14
    reinterpret_cast<void*>(&pp_get168),                  // +0x18
    reinterpret_cast<void*>(&pp_set172),                  // +0x1C
    reinterpret_cast<void*>(&pp_get172),                  // +0x20
    reinterpret_cast<void*>(&pp_setStateVec4),            // +0x24
    reinterpret_cast<void*>(&pp_getStateVec4),            // +0x28
    reinterpret_cast<void*>(&pp_setRange),                // +0x2C
    reinterpret_cast<void*>(&pp_getRange),                // +0x30
    reinterpret_cast<void*>(&pp_setBlend),                // +0x34  gas-mask blend  <- per-frame caller
    reinterpret_cast<void*>(&pp_getBlend),                // +0x38
    reinterpret_cast<void*>(&pp_setMat0),                 // +0x3C
    reinterpret_cast<void*>(&pp_setMat1),                 // +0x40
    reinterpret_cast<void*>(&pp_set248),                  // +0x44
    reinterpret_cast<void*>(&pp_set252),                  // +0x48
    reinterpret_cast<void*>(&pp_setByte242),              // +0x4C
    reinterpret_cast<void*>(&pp_setByte241),              // +0x50
    reinterpret_cast<void*>(&pp_getRendererFloat1852),    // +0x54
    reinterpret_cast<void*>(&pp_getRendererFloat1856),    // +0x58
    reinterpret_cast<void*>(&pp_setNoiseTextureName),     // +0x5C
};


// Releases and clears the owned distortion-noise texture on device loss.
void __fastcall dl_onLost(void* self, void*)
{
    EngineTexture*& noise = *reinterpret_cast<EngineTexture**>(static_cast<char*>(self) + 0x1FC);
    textureReleaseRef(noise);
}

// slot 1 onDeviceReset, only while the name is non-empty (its _Mysize at
// listener+0x214 = owner+0x218; `jbe` = unsigned, so "non-zero"): force the TextureManager's managed
// flag to 0 (sub_10017E20), reload the texture BY name (sub_10019260, the string copied by value)
// into owner+0x200, and restore the flag. The old pointer is not released first - the lost slot
// already nulled it.
void __fastcall dl_onReset(void* self, void*)
{
    const vc71::string& name = *reinterpret_cast<const vc71::string*>(static_cast<char*>(self) + 0x200);
    if (name.size() == 0)
        return;
    const int oldManaged = textureManagerSwapManaged(0);
    *reinterpret_cast<void**>(static_cast<char*>(self) + 0x1FC) = textureManagerGetTexture(name);
    textureManagerSwapManaged(oldManaged);
}

// slot 2 (0x1004E960): nullsub in the shipped image.
void __fastcall dl_slot2(void*, void*) { }

const void* const kDeviceLostVtable[3] =
{
    reinterpret_cast<void*>(&dl_onLost),
    reinterpret_cast<void*>(&dl_onReset),
    reinterpret_cast<void*>(&dl_slot2),
};


// EventListener / IBase spine vtable (owner+0). Registration is deferred, so handleEvent is not
// reached; the reference spine at +4/+8/+C/+10 is the recovered IBase implementation. Refcount is the
// dword at this+0x0C.


// slot 0 handleEvent (0x1004F0F0): react to (class 0,id 13) device-reset and (class 5,id 1)
// GSViewDistance change. Recovered verbatim; unreached until the EventManager registration is undone
// from deferral. a4 is the event payload.
int __fastcall el_handleEvent(void* self, void*, int a2, int a3, int a4, int a5)
{
    int result = a2;
    if (a2)
    {
        if (a2 == 5 && a3 == 1)
        {
            const vc71::string* s = reinterpret_cast<const vc71::string*>(a5);
            return strcmp(s->c_str(), "GSViewDistance") == 0 ? 1 : 0;
        }
    }
    else if (a3 == 13)
    {
        result = *reinterpret_cast<int*>(a4 + 44);
        if (result)
        {
            U(self, 356) = *reinterpret_cast<unsigned*>(result);
            U(self, 360) = *reinterpret_cast<unsigned*>(result + 4);
            U(self, 364) = *reinterpret_cast<unsigned*>(result + 8);
            U(self, 368) = *reinterpret_cast<unsigned*>(result + 12);
            result       = *reinterpret_cast<int*>(result + 16);
            U(self, 372) = static_cast<unsigned>(result);
            if (U(self, 356) == 1)
            {
                int v6 = static_cast<int>((F(self, 368) + F(self, 364) + F(self, 360)) * 30.0f);
                if (v6 < 20) v6 = 20;
                U(self, 28) = static_cast<unsigned>(v6) + U(self, 16);
                F(self, 32) = static_cast<float>(v6);
                result = v6;
            }
        }
    }
    return result;
}

int  __fastcall el_addRef (void* self, void*) { return ++U(self, 12); }
int  __fastcall el_getRc  (void* self, void*) { return   U(self, 12); }

// decrement; at zero invoke the PostProductionFrameWork subobject's deleting dtor
// (owner+8 vtable slot 0), i.e. `(*(*(this+8)))(this+8, 1)`.
int __fastcall el_release(void* self, void*)
{
    int v = --U(self, 12);
    if (!v)
    {
        void* sub = static_cast<char*>(self) + 8;
        void** vt = *reinterpret_cast<void***>(sub);
        typedef void* (__fastcall* DtorFn)(void*, void*, int);
        reinterpret_cast<DtorFn>(vt[0])(sub, 0, 1);
    }
    return v;
}

// queryInterface - accepts iid 1 (IBase) only.
void* __fastcall el_qi(void* self, void*, unsigned iid) { return iid != 1u ? 0 : self; }

const void* const kOwnerVtable[5] =
{
    reinterpret_cast<void*>(&el_handleEvent),
    reinterpret_cast<void*>(&el_addRef),
    reinterpret_cast<void*>(&el_getRc),
    reinterpret_cast<void*>(&el_release),
    reinterpret_cast<void*>(&el_qi),
};

// sub_1004E860 - the second field-default arm the constructor calls after the acquisition block. Pure
// stores; reproduced with the same values (non-zero only; the block is memset to 0 first).
void applySecondaryDefaults(void* owner)
{
    U (owner, 176) = 1056964608u;
    U (owner, 180) = 1056964608u;
    U (owner, 192) = 1056964608u;
    Bt(owner,  38) = 1;
    U (owner, 172) = 1036831949u;
    U (owner, 184) = 1080033280u;
    U (owner, 216) = 1055286886u;
    U (owner, 232) = 1060320051u;
    U (owner, 236) = 1075838976u;
    U (owner, 240) = 1077936128u;
    U (owner, 244) = 1075838976u;
    // owner+224 (the gas-mask blend), owner+188/208/252/256/260, and the +212/229/249/250 bytes are 0
    // here - already zero from the initial memset.
}

} // anonymous namespace


// Initializes the preallocated 548-byte owner and returns it.
// [FRONTIER] Effect acquisition, parameter handles, helper textures, listener registration,
// and fullscreen rendering remain deferred; resource fields stay zero until implemented.
// [RETIRE-WHEN: the post-production render path is recovered]

void* postProductionFrameWorkOwnerCtor(void* mem)
{
    // The shipped ctor sets every field explicitly; we memset first and then write only the non-zero
    // finals, which is the identical resulting state.
    memset(mem, 0, 548);

    // The three interface vtables (owner+0 / +4 / +8).
    U(mem,  0) = ::renddx9abi::addr(kOwnerVtable);
    U(mem,  4) = ::renddx9abi::addr(kDeviceLostVtable);
    U(mem,  8) = ::renddx9abi::addr(kPostProdVtable);

    U(mem, 12) = 1u;                      // this+3  refcount

    // this+10..16 (byte 40..64): the ctor sets these twice; the final values are written here.
    U(mem, 40) = 1065353216u;             // 1.0f
    U(mem, 44) = 1065353216u;
    U(mem, 48) = 1065353216u;
    U(mem, 52) = 1133903872u;
    U(mem, 56) = 1140457472u;
    U(mem, 60) = 1065353216u;
    // owner+64 (this+16): set 0.5f then 0 -> final 0 (memset).

    // this+17..22 (byte 68..88): the "final" second-pass values.
    U(mem, 68) = 1050589266u;
    U(mem, 72) = 1047233823u;
    U(mem, 76) = 1034952901u;
    U(mem, 80) = 1050589266u;
    U(mem, 84) = 1047233823u;
    U(mem, 88) = 1034952901u;

    // this+23..30 (byte 92..120): v2 block + trailing scalars (finals).
    U(mem,  96) = 1077936128u;            // v2[1]
    U(mem, 100) = 1065353216u;            // v2[2]
    U(mem, 112) = 1106247680u;            // this+28
    U(mem, 116) = 1065353216u;            // this+29

    // this+31..51 (byte 124..204).
    U(mem, 124) = 1045352145u;
    U(mem, 128) = 1045352145u;
    U(mem, 132) = 1056997505u;
    U(mem, 136) = 1092616192u;
    U(mem, 140) = 1132068864u;
    U(mem, 144) = 1065353216u;
    U(mem, 152) = 1092616192u;
    U(mem, 156) = 1148846080u;
    U(mem, 168) = 1028443341u;            // this+42
    U(mem, 172) = 1036831949u;            // this+43
    U(mem, 176) = 1056964608u;            // this+44
    U(mem, 180) = 1056964608u;            // this+45
    U(mem, 192) = 1056964608u;            // this+48

    // Byte flags set by the ctor (this+220 and this+228 both == 1).
    Bt(mem, 220) = 1;
    Bt(mem, 228) = 1;

    // The embedded VC7.1 std::string at this+129 (byte 516) - the noise-texture name, empty.
    new (static_cast<char*>(mem) + 516) vc71::string();

    // this+66..83 (byte 264..332): the two 3x3 matrices, set once near the end of the ctor.
    U(mem, 264) = 1028443341u; U(mem, 268) = 1045220557u; U(mem, 272) = 1056964608u;
    U(mem, 276) = 1063675494u; U(mem, 280) = 1065353216u; U(mem, 284) = 1063675494u;
    U(mem, 288) = 1056964608u; U(mem, 292) = 1045220557u; U(mem, 296) = 1028443341u;
    U(mem, 300) = 1028443341u; U(mem, 304) = 1045220557u; U(mem, 308) = 1056964608u;
    U(mem, 312) = 1063675494u; U(mem, 316) = 1065353216u; U(mem, 320) = 1063675494u;
    U(mem, 324) = 1056964608u; U(mem, 328) = 1045220557u; U(mem, 332) = 1028443341u;

    // The constructor's second default arm.
    applySecondaryDefaults(mem);

    // the constructor's last act, 0x100507A9 `push 3` / 0x100507AB
    // `lea eax, [esi+4]` ... 0x1005080E `mov ecx, dword_1023B970; push eax; call sub_10009D60`:
    // RendDX9_registerDeviceLostResetListener(owner + 4, 3). DX9RendModule::init constructs this
    // after the Renderer (pass two of initDll), so the list at dword_1023B970+0x5C exists.
    registerDeviceLostResetListener(static_cast<char*>(mem) + 4, 3);

    ::renddx9abi::logf("PostProductionFrameWork constructed at owner 0x%08X (singleton owner+8) - "
                       "vtables live, state defaulted, device-lost listener registered (priority "
                       "3); effect/params/textures + the EventManager handler deferred with the "
                       "render path", ::renddx9abi::addr(mem));
    return mem;
}


// Detaches the owner's device listener before its storage is freed.
// [FRONTIER] Remaining destructor work depends on deferred event/resource acquisition.
void postProductionFrameWorkOwnerDetach(void* owner)
{
    U(owner, 0) = ::renddx9abi::addr(kOwnerVtable);
    U(owner, 4) = ::renddx9abi::addr(kDeviceLostVtable);
    U(owner, 8) = ::renddx9abi::addr(kPostProdVtable);
    unregisterDeviceLostResetListener(static_cast<char*>(owner) + 4);
}

// Stores 0..255 RGB as normalized fog colour. Requires the post-production owner.
// Helpers use owner-relative offsets; the public singleton is its +8 subobject.
void postProductionSetFogColor255(const float rgb[3])
{
    void* const o = g_PostProductionFrameWorkOwner;
    const float k = 1.0f / 255.0f;
    F(o, 0x30) = rgb[2] * k;
    F(o, 0x28) = rgb[0] * k;
    F(o, 0x2C) = rgb[1] * k;
}

// the read arm: the stored colour times 255 (0x437F0000).
void postProductionGetFogColor255(float rgb[3])
{
    void* const o = g_PostProductionFrameWorkOwner;
    rgb[0] = F(o, 0x28) * 255.0f;
    rgb[1] = F(o, 0x2C) * 255.0f;
    rgb[2] = F(o, 0x30) * 255.0f;
}

// renderer.waterFogColor's write arm: the one colour, divided by 255, into both
// water-fog colour slots (B at +0x50 first, then A at +0x44).
void postProductionSetWaterFogColor255(const float rgb[3])
{
    void* const o = g_PostProductionFrameWorkOwner;
    const float k = 1.0f / 255.0f;
    F(o, 0x58) = rgb[2] * k;
    F(o, 0x50) = rgb[0] * k;
    F(o, 0x54) = rgb[1] * k;
    F(o, 0x4C) = rgb[2] * k;
    F(o, 0x44) = rgb[0] * k;
    F(o, 0x48) = rgb[1] * k;
}

// its read arm reads slot B (+0x50) times 255.
void postProductionGetWaterFogColor255(float rgb[3])
{
    void* const o = g_PostProductionFrameWorkOwner;
    rgb[0] = F(o, 0x50) * 255.0f;
    rgb[1] = F(o, 0x54) * 255.0f;
    rgb[2] = F(o, 0x58) * 255.0f;
}

// renderer.waterFogStartEndAndBase's write arm (0x1004FD50): the start/end/base/max record into
// both water-fog range slots (B at +0x6C first, then A at +0x5C), then the terrain's
// lighting-dirty byte when a terrain exists.
void postProductionSetWaterFogRange(const float range[4])
{
    void* const o = g_PostProductionFrameWorkOwner;
    F(o, 0x6C) = range[0];
    F(o, 0x70) = range[1];
    F(o, 0x74) = range[2];
    F(o, 0x78) = range[3];
    F(o, 0x5C) = range[0];
    F(o, 0x60) = range[1];
    F(o, 0x64) = range[2];
    F(o, 0x68) = range[3];
    if (geom::Terrain* const t = geom::terrain())
        t->m_lightingDirty = 1;
}

// its read arm (0x1004F730) reads slot B (+0x6C).
void postProductionGetWaterFogRange(float range[4])
{
    void* const o = g_PostProductionFrameWorkOwner;
    range[0] = F(o, 0x6C);
    range[1] = F(o, 0x70);
    range[2] = F(o, 0x74);
    range[3] = F(o, 0x78);
}

// Updates shader and device fog from camera depth below the water plane.
// Interpolates water fog over ten units; NaN depth selects the low bound.
// Fog coefficient division is unchecked when start and end coincide.
void postProductionUpdateFog(float underwaterDepth)
{
    void* const o = g_PostProductionFrameWorkOwner;

    float t = underwaterDepth * 0.1f;
    if (t >= 1.0f)
        t = 1.0f;
    if (!(t >= 0.0f))
        t = 0.0f;
    const float u = 1.0f - t;

    float c[3];
    c[0] = F(o, 0x50) * u + F(o, 0x44) * t;
    c[1] = F(o, 0x54) * u + F(o, 0x48) * t;
    c[2] = F(o, 0x58) * u + F(o, 0x4C) * t;

    float r[4];
    r[0] = F(o, 0x6C) * u + F(o, 0x5C) * t;
    r[1] = F(o, 0x70) * u + F(o, 0x60) * t;
    r[2] = F(o, 0x74) * u + F(o, 0x64) * t;
    r[3] = F(o, 0x78) * u + F(o, 0x68) * t;

    const float ws = 1.0f / (r[1] - r[0]);
    float waterRange[4];
    waterRange[0] = ws;
    waterRange[1] = -(r[2] * ws);
    waterRange[2] = -(r[0] * ws);
    waterRange[3] = r[1] * ws;
    raShaderConstantsSetWaterFogRange(waterRange);
    raShaderConstantsSetWaterFogColor(c);

    if (!cameraIsUnderWater())
    {
        float rgbw[4];
        rgbw[0] = F(o, 0x28);
        rgbw[1] = F(o, 0x2C);
        rgbw[2] = F(o, 0x30);
        rgbw[3] = F(o, 0x40);
        raShaderConstantsSetFogColor(rgbw);

        const float s = 1.0f / (F(o, 0x38) - F(o, 0x34));      // 1 / (end - start)
        float range[4];
        range[0] = s;
        range[1] = -(F(o, 0x3C) * s);
        range[2] = -(F(o, 0x34) * s);
        range[3] = F(o, 0x38) * s;
        raShaderConstantsSetFogRange(range);
    }
    else
    {
        float rgbw[4];
        rgbw[0] = c[0];
        rgbw[1] = c[1];
        rgbw[2] = c[2];
        rgbw[3] = r[3];
        raShaderConstantsSetFogColor(rgbw);
        raShaderConstantsSetFogRange(waterRange);
    }
}

void postProductionGetHemiState(float outState[4])
{
    pp_getStateVec4(static_cast<char*>(static_cast<void*>(g_PostProductionFrameWorkOwner)) + 8,
                    0, outState);
}

void postProductionSetHemiState(const float state[4])
{
    pp_setStateVec4(static_cast<char*>(static_cast<void*>(g_PostProductionFrameWorkOwner)) + 8,
                    0, state);
}

} // namespace dx9rend
} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_RENDDX9
