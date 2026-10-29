
// Native renderer console bindings for fog, presentation and editor capture/LOD settings.
// [INFERRED] companion source placement. Availability follows the owning renderer or
// post-processing singleton; only the bindings declared below are published.

#ifdef BF2_IMAGE_RENDDX9
#include "../../RendDX9Abi/PluginAbi.h"
#if BF2_PLUGIN_ABI_NATIVE
#include "../Core/ConsoleObjectMacro.h"
#include "../Core/ConsoleObjects.h"
#include "../../RendDX9Abi/PluginEntry.h"
#include "../Core/IBase.h"
#include "../Core/Types.h"
#include "../Math/Vec3.h"
#include "../Geom/Terrain.h"
#include "../../RendDX9Abi/Vc71String.h"
#include "RendDX9Base.h"

#include <string>

namespace dice {
namespace hfe {
namespace dx9rend {

extern IBase* g_PostProductionFrameWorkOwner;     // BF2Render.cpp - ds:0x1023F7C0
extern void*  g_rendererBase;                     // RendDX9Init.cpp - dword_1023B970
extern IBase* g_Renderer;                         // BF2Render.cpp - dword_1023D098

// BF2Render.cpp - Renderer::makeTopWorldScreen (0x1002C5F0) and the two forced-lod
// setters (0x1002D480 / 0x1002D4A0) with the renderer dwords their GET arms read (+0x7DC / +0x7E0).
void rendererMakeTopWorldScreen(unsigned int resolution, renddx9abi::HostString name,
                                Vec3 position, float orthoExtent);
void rendererSetSkinnedMeshLod(int lod);
void rendererSetBundledMeshLod(int lod);
int  rendererGetSkinnedMeshLod();
int  rendererGetBundledMeshLod();
// BF2Render.cpp - drawHud's arms (0x1005CCC0) over the renderer byte +0x6B1.
bool rendererGetDrawHud();
void rendererSetDrawHud(bool on);

namespace {

// Stands in for the original's file-scope list (the static at 0x10241668).
const char kRendererConsoleTag[] = "dx9rend.renderer";


// Boolean presentation properties: zero arguments reads, one writes.
// Require an active renderer; changing vsync resets its device.

bool getVSync()                     { return rendererGetVSync(g_rendererBase); }
void setVSync(bool value)           { rendererSetVSync(g_rendererBase, value); }
bool getPresentAsync()              { return rendererGetPresentAsync(g_rendererBase); }
void setPresentAsync(bool value)    { rendererSetPresentAsync(g_rendererBase, value); }
bool getPresentSpinIfBusy()         { return rendererGetPresentSpinIfBusy(g_rendererBase); }
void setPresentSpinIfBusy(bool value) { rendererSetPresentSpinIfBusy(g_rendererBase, value); }

// fogColor - ctor 0x100586B0 (type "Vec3"). Read arm 0x100588D0 -> the stored,
// normalised colour times 255. Write arm 0x10058930 -> divided by 255 on the way in.
Vec3 getFogColor()
{
    Vec3 v;
    postProductionGetFogColor255(&v.x);
    return v;
}

void setFogColor(const Vec3& v)
{
    postProductionSetFogColor255(&v.x);
}

// waterFogColor - ctor 0x10058950 (type "Vec3"). Read arm 0x10058AF1 -> 0x1004F700, write arm
// 0x10058B3D -> 0x1004F690, which stores the one colour into both water-fog colour slots.
Vec3 getWaterFogColor()
{
    Vec3 v;
    postProductionGetWaterFogColor255(&v.x);
    return v;
}

void setWaterFogColor(const Vec3& v)
{
    postProductionSetWaterFogColor255(&v.x);
}

// waterFogStartEndAndBase - ctor 0x10058B50 (type "Vec4"), active while the post-production
// owner exists (0x10057370). Read arm 0x1004F730 -> the water-fog range record B; write arm
// 0x1004FD50 -> both records, then raises the terrain's lighting-dirty byte.
Vec4 getWaterFogStartEndAndBase()
{
    Vec4 v;
    postProductionGetWaterFogRange(&v.x);
    return v;
}

void setWaterFogStartEndAndBase(const Vec4& v)
{
    postProductionSetWaterFogRange(&v.x);
}


// The terrain water bindings (ctors 0x1005BBE0..0x1005C9A0), active while g_Renderer exists
// (0x1005B930). Each forwards to the terrain singleton; without a terrain the read answers zero
// and the write is dropped.

// waterScroll - ctor 0x1005BBE0 (type "Vec2"), executor 0x1005BE40: Terrain +0x88 / +0x8C.
Vec2 getWaterScroll()
{
    Vec2 v;
    if (geom::Terrain* const t = geom::terrain())
    {
        Vec2 out;
        v = *t->getWaterScroll(&out);
    }
    else
    {
        v.x = 0.0f;
        v.y = 0.0f;
    }
    return v;
}

void setWaterScroll(const Vec2& v)
{
    if (geom::Terrain* const t = geom::terrain())
        t->setWaterScroll(v.x, v.y);
}

// waterAnimSpeed - ctor 0x1005BF40 (type "float"), executor 0x1005C090: Terrain +0x90 / +0x94.
float getWaterAnimSpeed()
{
    if (geom::Terrain* const t = geom::terrain())
        return t->getWaterAnimSpeed();
    return 0.0f;
}

void setWaterAnimSpeed(float speed)
{
    if (geom::Terrain* const t = geom::terrain())
        t->setWaterAnimSpeed(speed);
}

// waterNormalMap - ctor 0x1005C110 (type "std::string"), executor 0x1005C200: Terrain +0x98 /
// +0x9C, the setter re-running setupWater.
std::string getWaterNormalMap()
{
    if (geom::Terrain* const t = geom::terrain())
        return std::string(t->getWaterNormalMap().c_str());
    return std::string();
}

void setWaterNormalMap(const std::string& name)
{
    if (geom::Terrain* const t = geom::terrain())
        t->setWaterNormalMap(vc71::string(name.c_str()));
}

// waterSpecularColor - ctor 0x1005C440 (type "Vec4"), executor 0x1005C5D0: Terrain +0xA0 / +0xA4.
Vec4 getWaterSpecularColor()
{
    Vec4 v;
    if (geom::Terrain* const t = geom::terrain())
    {
        Vec4 out;
        v = *t->getWaterSpecularColor(&out);
    }
    else
    {
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = 0.0f;
        v.w = 0.0f;
    }
    return v;
}

void setWaterSpecularColor(const Vec4& v)
{
    if (geom::Terrain* const t = geom::terrain())
        t->setWaterSpecularColor(v.x, v.y, v.z, v.w);
}

// waterColor - ctor 0x1005C710 (type "Vec3"), executor 0x1005C890: the first three floats of the
// terrain's water record (+0x710), read by 0x100DB980 and written by 0x100DB9A0.
Vec3 getWaterColor()
{
    Vec3 v;
    if (geom::Terrain* const t = geom::terrain())
    {
        v.x = t->m_waterRecord[0];
        v.y = t->m_waterRecord[1];
        v.z = t->m_waterRecord[2];
    }
    else
    {
        v.x = 0.0f;
        v.y = 0.0f;
        v.z = 0.0f;
    }
    return v;
}

void setWaterColor(const Vec3& v)
{
    if (geom::Terrain* const t = geom::terrain())
    {
        t->m_waterRecord[0] = v.x;
        t->m_waterRecord[1] = v.y;
        t->m_waterRecord[2] = v.z;
    }
}

// waterSpecularPower - ctor 0x1005C9A0 (type "float"), executor 0x1005CAF0: Terrain +0xA8 / +0xAC.
float getWaterSpecularPower()
{
    if (geom::Terrain* const t = geom::terrain())
        return t->getWaterSpecularPower();
    return 0.0f;
}

void setWaterSpecularPower(float power)
{
    if (geom::Terrain* const t = geom::terrain())
        t->setWaterSpecularPower(power);
}


// Native capture adapter. The renderer's compatibility path captures at a fixed 2048 square.

void consoleMakeTopWorldScreen(U32 resolution, const std::string& name, const Vec3& position,
                               float orthoExtent)
{
    rendererMakeTopWorldScreen(resolution, renddx9abi::HostString(name), position, orthoExtent);
}

BF2_CONSOLE_OBJECT("renderer", 2, 0)

// in the shipped static-initialiser order, ahead of fogColor.
BF2_CONSOLE_PROPERTY_FN(vsync, bool, g_rendererBase != 0,
                        getVSync(), setVSync, kRendererConsoleTag)
BF2_CONSOLE_PROPERTY_FN(presentAsync, bool, g_rendererBase != 0,
                        getPresentAsync(), setPresentAsync, kRendererConsoleTag)
BF2_CONSOLE_PROPERTY_FN(presentSpinIfBusy, bool, g_rendererBase != 0,
                        getPresentSpinIfBusy(), setPresentSpinIfBusy, kRendererConsoleTag)

BF2_CONSOLE_METHOD_4_STATIC_ACTIVE(makeTopWorldScreen, consoleMakeTopWorldScreen, g_Renderer != 0,
                                   U32, std::string, Vec3, float, kRendererConsoleTag)

BF2_CONSOLE_PROPERTY_FN(fogColor, Vec3, g_PostProductionFrameWorkOwner != 0,
                        getFogColor(), setFogColor, kRendererConsoleTag)
BF2_CONSOLE_PROPERTY_FN(waterFogColor, Vec3, g_PostProductionFrameWorkOwner != 0,
                        getWaterFogColor(), setWaterFogColor, kRendererConsoleTag)
BF2_CONSOLE_PROPERTY_FN(waterFogStartEndAndBase, Vec4, g_PostProductionFrameWorkOwner != 0,
                        getWaterFogStartEndAndBase(), setWaterFogStartEndAndBase, kRendererConsoleTag)

BF2_CONSOLE_PROPERTY_FN(forceSkinnedMeshLod, int, g_Renderer != 0,
                        rendererGetSkinnedMeshLod(), rendererSetSkinnedMeshLod, kRendererConsoleTag)
BF2_CONSOLE_PROPERTY_FN(forceBundledMeshLod, int, g_Renderer != 0,
                        rendererGetBundledMeshLod(), rendererSetBundledMeshLod, kRendererConsoleTag)

// The terrain water properties, in their shipped static-initialiser order (0x1005BBE0..0x1005C9A0).
BF2_CONSOLE_PROPERTY_FN(waterScroll, Vec2, g_Renderer != 0,
                        getWaterScroll(), setWaterScroll, kRendererConsoleTag)
BF2_CONSOLE_PROPERTY_FN(waterAnimSpeed, float, g_Renderer != 0,
                        getWaterAnimSpeed(), setWaterAnimSpeed, kRendererConsoleTag)
BF2_CONSOLE_PROPERTY_FN(waterNormalMap, std::string, g_Renderer != 0,
                        getWaterNormalMap(), setWaterNormalMap, kRendererConsoleTag)
BF2_CONSOLE_PROPERTY_FN(waterSpecularColor, Vec4, g_Renderer != 0,
                        getWaterSpecularColor(), setWaterSpecularColor, kRendererConsoleTag)
BF2_CONSOLE_PROPERTY_FN(waterColor, Vec3, g_Renderer != 0,
                        getWaterColor(), setWaterColor, kRendererConsoleTag)
BF2_CONSOLE_PROPERTY_FN(waterSpecularPower, float, g_Renderer != 0,
                        getWaterSpecularPower(), setWaterSpecularPower, kRendererConsoleTag)

// drawHud - ctor 0x1005CB70 (type "bool"), active while g_Renderer exists (0x1005B930).
BF2_CONSOLE_PROPERTY_FN(drawHud, bool, g_Renderer != 0,
                        rendererGetDrawHud(), rendererSetDrawHud, kRendererConsoleTag)

} // anonymous namespace

bool registerRendererConsoleObjects()
{
    return mainConsoleObjects->registerConsoleObjects(
        consoleBindingList(kRendererConsoleTag));
}

bool unregisterRendererConsoleObjects()
{
    return mainConsoleObjects->unregisterConsoleObjects(
        consoleBindingList(kRendererConsoleTag));
}

} // namespace dx9rend
} // namespace hfe
} // namespace dice

#endif // BF2_PLUGIN_ABI_NATIVE
#endif // BF2_IMAGE_RENDDX9
