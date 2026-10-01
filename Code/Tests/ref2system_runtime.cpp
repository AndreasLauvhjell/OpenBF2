// [TEMP] [CHOSEN, not recovered] Bounded client construction/lifetime and
// render-helper fixture. No UI method or type-descriptor frontier is replaced.
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <cstdio>
#include <cmath>
#include <new>
#include <limits>
#include <Windows.h>
#include "../BF2/Core/ClassManager.h"
#include "../BF2/GeomMisc/MenuRend/IQuadDrawer.h"
// Access labels participate in MSVC decorated names. This fixture's private
// visibility macro changes only calls; bind them to the real private helpers.
#pragma comment(linker, "/alternatename:?prepareRenderView@Ref2System@menu@hfe@dice@@QAEXXZ=?prepareRenderView@Ref2System@menu@hfe@dice@@AAEXXZ")
#pragma comment(linker, "/alternatename:?prepareViewMatrix@Ref2System@menu@hfe@dice@@QAEXXZ=?prepareViewMatrix@Ref2System@menu@hfe@dice@@AAEXXZ")
#define private public
#define protected public
#include "../BF2/Menu/Ref2System.h"
#undef protected
#undef private

using dice::hfe::IBase;
using dice::hfe::menu::Ref2System;
static unsigned int checks, failures;
static void check(bool value, const char* what)
{
    ++checks;
    if (!value) { ++failures; std::printf("FAIL: %s\n", what); }
}
static bool bytesAre(const void* data, unsigned int size, unsigned char value)
{
    const unsigned char* p = static_cast<const unsigned char*>(data);
    for (unsigned int i = 0; i != size; ++i) if (p[i] != value) return false;
    return true;
}

// Only the real IBase release contract is modeled: destructor must release
// exactly +684; close-owned texture/view/singleton collaborators stay untouched.
struct RefProbe : IBase
{
    int releases = 0;
    void addRef() override {}
    int getRef() override { return 1; }
    int release() override { ++releases; return 1; }
    void* queryInterface(unsigned int) const override { return nullptr; }
};

// These raw four-byte-slot carriers model only the external plugin calls made
// by the two helpers. They are not substitute System/Ref2System objects.
struct RenderProbe { void** table; };
static std::vector<int> calls;
static float callValues[5];
static float seenMatrix[16];
static RefProbe previousView;
static IBase* selectedView;
static void __fastcall setMode(void*, void*, int value) { calls.push_back(0x48); callValues[0] = float(value); }
static void __fastcall setNear(void*, void*, float value) { calls.push_back(0x30); callValues[1] = value; }
static void __fastcall setFar(void*, void*, float value) { calls.push_back(0x38); callValues[2] = value; }
static void __fastcall setWidth(void*, void*, float value) { calls.push_back(0x50); callValues[3] = value; }
static void __fastcall setHeight(void*, void*, float value) { calls.push_back(0x58); callValues[4] = value; }
static IBase* __fastcall getView(void*, void*) { calls.push_back(0x148); return &previousView; }
static bool __fastcall setMatrix(void*, void*, const float* values)
{ calls.push_back(0x60); std::memcpy(seenMatrix, values, sizeof(seenMatrix)); return true; }
static bool __fastcall setView(void*, void*, IBase* value)
{ calls.push_back(0x144); selectedView=value; return false; } // Result is ignored by both hooks.
struct QuadProbe : dice::hfe::geom::IQuadDrawer
{
    void addRef() override {}
    int getRef() override { return 1; }
    int release() override { return 0; }
    void* queryInterface(unsigned int) const override { return nullptr; }
    void flush() override { calls.push_back(0x210); }
    void beginFrame() override { calls.push_back(0x214); }
    void resetState() override { calls.push_back(0x218); }
    void endFrame() override { calls.push_back(0x21C); }
};
static QuadProbe quadDrawer;
struct ClassProbe : dice::hfe::ClassManager
{
    IBase* getSingleton(const std::string& name) override {
        check(name=="QuadDrawer", "paint singleton name");
        calls.push_back(0x248);return &quadDrawer;
    }
};
namespace dice { namespace hfe { ClassManager* classManager; } }

int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    alignas(Ref2System) unsigned char storage[sizeof(Ref2System)];
    std::memset(storage, 0xA5, sizeof(storage));
    Ref2System* root = new(storage) Ref2System(1280.0f, 720.0f);
    check(root->m_originX == -640 && root->m_originY == -360, "centered base bounds");
    check(root->m_width == 1280 && root->m_height == 720, "base dimensions");
    check(root->m_field028 == -3 && root->m_field02c == 2, "encoded coordinate defaults");
    check(root->m_lockCount == 0 && root->m_field018 == nullptr && root->m_field024 == nullptr, "base pointers/lock");
    check(root->m_fonts.empty(), "native map starts empty");
    check(root->m_textureList == nullptr, "texture list starts null");
    check(bytesAre(&root->m_blankTexture, sizeof(root->m_blankTexture), 0xA5), "blank texture untouched");
    check(bytesAre(root->m_inputStates, sizeof(root->m_inputStates), 0xA5), "input states untouched");
    check(bytesAre(&root->m_renderView, sizeof(root->m_renderView), 0xA5), "render view untouched");
    check(root->m_renderWidth == 1280 && root->m_renderHeight == 720, "render dimensions");
    check(!root->m_previousRenderView && !root->m_renderer && !root->m_field684 && !root->m_textureFlags && !root->m_field68c, "late scalar defaults");
    for (unsigned int i = 0; i != 4; ++i)
    {
        const auto& sample = root->m_inputs[i];
        check(bytesAre(sample.inputs, sizeof(sample.inputs), 0), "sample inputs clear");
        check(!sample.mappingFlags && !sample.dword0 && !sample.dword1 && !sample.flag, "sample metadata clear");
        check(bytesAre(reinterpret_cast<const unsigned char*>(&sample) + 0x139, 7, 0xA5), "sample trailing bytes untouched");
    }
    check(root->getObjectSize() == sizeof(Ref2System), "native size virtual dispatch");
    check(root->m_output.getObjectSize() == 4, "embedded stream prefix dispatch");
    check(static_cast<dice::meme::System*>(root) == static_cast<dice::meme::Object*>(root), "primary base address");

    void* viewSlots[25] = {};
    viewSlots[0x48/4] = reinterpret_cast<void*>(&setMode);
    viewSlots[0x30/4] = reinterpret_cast<void*>(&setNear);
    viewSlots[0x38/4] = reinterpret_cast<void*>(&setFar);
    viewSlots[0x50/4] = reinterpret_cast<void*>(&setWidth);
    viewSlots[0x58/4] = reinterpret_cast<void*>(&setHeight);
    viewSlots[0x60/4] = reinterpret_cast<void*>(&setMatrix);
    void* rendererSlots[19] = {};
    rendererSlots[0x48/4] = reinterpret_cast<void*>(&getView);
    rendererSlots[0x44/4] = reinterpret_cast<void*>(&setView);
    RenderProbe view = {viewSlots}, renderer = {rendererSlots};
    root->m_renderView = reinterpret_cast<IBase*>(&view);
    root->m_renderer = reinterpret_cast<IBase*>(&renderer);
    root->prepareRenderView();
    check(calls == std::vector<int>({0x48,0x30,0x38,0x50,0x58,0x148}), "projection call order");
    check(callValues[0] == 1 && callValues[1] == 0.01f && callValues[2] == 1, "projection mode/near/far");
    check(callValues[3] == 800 && callValues[4] == 600, "fixed projection size independent of constructor size");
    check(root->m_previousRenderView == &previousView, "borrow previous render view");
    root->prepareViewMatrix();
    check(calls.back() == 0x60, "matrix sent to view");
    check(seenMatrix[0] == 1 && seenMatrix[15] == 1 && seenMatrix[5] == -1 && seenMatrix[10] == -1, "half-turn X basis");
    check(seenMatrix[6] == -seenMatrix[9] && std::fabs(seenMatrix[6]) < 0.000001f, "opposite sine terms");
    check(seenMatrix[12] == 0 && seenMatrix[13] == 0 && seenMatrix[14] == 0.02f, "view translation");

    ClassProbe classes;dice::hfe::classManager=&classes;
    calls.clear();root->beginPaint();
    check(calls==std::vector<int>({0x48,0x30,0x38,0x50,0x58,0x148,0x60,0x144,0x248,0x214,0x218}),"paint begin prepares/selects view then begins/resets drawer");
    check(selectedView==reinterpret_cast<IBase*>(&view),"paint begin selects owned view");
    calls.clear();root->endPaint();
    check(calls==std::vector<int>({0x248,0x210,0x21C,0x144}),"paint end flushes/ends drawer before view restore");
    check(selectedView==&previousView,"paint end restores borrowed view");
    const float oldSize[2]={1280,720}, sameSize[2]={1280,720}, newSize[2]={1920,1080};
    root->m_renderWidth=1;root->m_renderHeight=2;
    root->sizeChanged(oldSize,sameSize);
    check(root->m_renderWidth==1 && root->m_renderHeight==2,"equal size callback preserves render dimensions");
    root->sizeChanged(oldSize,newSize);
    check(root->m_renderWidth==1920 && root->m_renderHeight==1080,"changed size updates both render dimensions");
    const float unorderedSize[2]={std::numeric_limits<float>::quiet_NaN(),720};
    root->sizeChanged(oldSize,unorderedSize);
    check(std::isnan(root->m_renderWidth) && root->m_renderHeight==720,"unordered size comparison follows changed branch");
    root->setSize(10,20);
    check(root->m_width==1280 && root->m_height==720,"Ref2 setSize remains retail empty override");

    RefProbe releaseOwned, closeOwned;
    root->m_field684 = &releaseOwned;
    // [TEMP] Opaque identity sentinels only: this constructor/destructor fixture
    // must not dispatch texture methods or call the separate UI close method.
    root->m_blankTexture = reinterpret_cast<dice::hfe::geom::IMemeTexture*>(&closeOwned);
    root->m_textureList = reinterpret_cast<dice::hfe::geom::IMemeTexture*>(&closeOwned);
    root->m_renderView = &closeOwned;
    root->m_renderer = &closeOwned;
    root->m_fonts.emplace(std::string(96, 'f'), reinterpret_cast<dice::hfe::menu::Font*>(&closeOwned));
    root->~Ref2System();
    check(releaseOwned.releases == 1, "destructor releases only owned collaborator");
    check(closeOwned.releases == 0 && previousView.releases == 0, "destructor preserves close-owned and borrowed refs");
    std::printf("Ref2System: %u/%u checks passed; native size %u bytes.\n", checks-failures, checks, unsigned(sizeof(Ref2System)));
    return failures ? 1 : 0;
}
