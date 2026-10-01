// [INFERRED] private renderer callback declarations beside the attested manager TU.
// PE 10171A00/10170CE0 are thiscall members stored at sprite descriptor +0x2A0.
// The existing plugin's explicit-this fastcall bridge preserves ECX and stack arguments.
#pragma once
namespace dice { namespace hfe {
namespace dx9rend { class IRenderView; }
namespace geom {
class IParticleSystemInstance;
class IParticleSystemTemplate;
void __fastcall particleRendererCollectSorted(void*, void*, dx9rend::IRenderView*,
    IParticleSystemInstance*, IParticleSystemTemplate*);
void __fastcall particleRendererCollectAdditive(void*, void*, dx9rend::IRenderView*,
    IParticleSystemInstance*, IParticleSystemTemplate*);
} } }
