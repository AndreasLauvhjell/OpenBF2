// [TEMP] [CHOSEN, not recovered] BF2Render wrapper fixture. Production bodies
// are linked from their real TU. Sparse typed callback tables model only the
// external renderer/game-logic calls under test; no production method is replaced.
#include <string>
#include <vector>
#include <new>
#define private public
#include "../BF2/Game/Main/BF2ClientSeams.h"
#undef private
#include "../BF2/Game/IGameLogic.h"
#include "../BF2/Core/IBase.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cstddef>
#include <cstdint>
#include <climits>

namespace dice { namespace hfe { IGameLogic* gameLogic = nullptr; } }
using namespace dice::hfe;
namespace {
int checks = 0, failures = 0;
std::vector<int> order;
void check(bool value, const char* label)
{
    ++checks;
    if (!value) { ++failures; std::printf("FAIL: %s\n", label); }
}
struct BorrowedObject : IBase
{
    int references = 17, ownershipCalls = 0;
    void addRef() override { ++ownershipCalls; ++references; }
    int getRef() override { ++ownershipCalls; return references; }
    int release() override { ++ownershipCalls; return --references; }
    void* queryInterface(unsigned int) const override { return nullptr; }
};
struct RendererRecord;
struct RendererTable
{
    void* uncalled00[5]; // +00..10 are never dispatched by this fixture
    bool (__fastcall* query14)(RendererRecord*, void*);
    bool (__fastcall* query18)(RendererRecord*, void*);
    void* uncalled1c;
    void (__fastcall* beforeFrame)(RendererRecord*, void*);
    void (__fastcall* update)(RendererRecord*, void*, double);
    void* uncalled28[8]; // no invented signatures for these unused entries
    IBase* (__fastcall* getView)(RendererRecord*, void*);
};
struct RendererRecord
{
    RendererTable* table;
    int id, beforeCalls, updateCalls, viewCalls, query14Calls, query18Calls;
    bool answer14, answer18;
    double frameTime;
    IBase* view;
};
void __fastcall beforeFrame(RendererRecord* self, void*)
{ ++self->beforeCalls; order.push_back(self->id); }
void __fastcall update(RendererRecord* self, void*, double time)
{ ++self->updateCalls; self->frameTime = time; order.push_back(self->id); }
IBase* __fastcall getView(RendererRecord* self, void*)
{ ++self->viewCalls; return self->view; }
bool __fastcall query14(RendererRecord* self, void*)
{ ++self->query14Calls; return self->answer14; }
bool __fastcall query18(RendererRecord* self, void*)
{ ++self->query18Calls; return self->answer18; }

struct LogicRecord;
struct LogicTable
{
    void* uncalled[0x21c / 4];
    bool (__fastcall* isLevelLoaded)(const LogicRecord*, void*);
};
struct LogicRecord
{
    LogicTable* table;
    BF2Render* wrapper;
    mutable int calls;
    bool loaded, replaceRenderer, clearDll, clearLogic;
    dx9rend::IRenderer* replacement;
};
bool __fastcall isLevelLoaded(const LogicRecord* self, void*)
{
    ++self->calls;
    order.push_back(1);
    if (self->replaceRenderer) self->wrapper->m_renderer = self->replacement;
    if (self->clearDll) self->wrapper->m_rendererDll = nullptr;
    if (self->clearLogic) gameLogic = nullptr;
    return self->loaded;
}
// __fastcall's ECX self and unused EDX adapt the known MSVC thiscall shape.
// These tables are fixture-only and never registered as production objects.
static_assert(offsetof(RendererTable, query14) == 0x14, "renderer bool14");
static_assert(offsetof(RendererTable, query18) == 0x18, "renderer bool18");
static_assert(offsetof(RendererTable, beforeFrame) == 0x20, "renderer pre-frame");
static_assert(offsetof(RendererTable, update) == 0x24, "renderer double update");
static_assert(offsetof(RendererTable, getView) == 0x48, "renderer borrowed view");
static_assert(offsetof(LogicTable, isLevelLoaded) == 0x21c, "game logic const bool query");

// Isolated SEH functions have no C++ objects requiring unwinding. Only access
// violations reading address zero are caught, to verify retail's deliberately
// missing null guards. An executable RVA-0 frontier is not accepted as evidence.
int nullReadFilter(EXCEPTION_POINTERS* exception)
{
    const EXCEPTION_RECORD* record = exception->ExceptionRecord;
    return record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION &&
           record->NumberParameters >= 2 && record->ExceptionInformation[0] == 0 &&
           record->ExceptionInformation[1] == 0
        ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}
bool beforeFaults(BF2Render* render)
{
    __try { render->beforeFrame(); }
    __except(nullReadFilter(GetExceptionInformation()))
    { return true; }
    return false;
}
bool updateFaults(BF2Render* render)
{
    __try { render->update(0.125); }
    __except(nullReadFilter(GetExceptionInformation()))
    { return true; }
    return false;
}
bool getterFaults(BF2Render* render, int slot)
{
    __try
    {
        if (slot == 0x14) (void)render->rendererSlot0x14();
        else if (slot == 0x18) (void)render->rendererSlot0x18();
        else (void)render->getRenderView();
    }
    __except(nullReadFilter(GetExceptionInformation()))
    { return true; }
    return false;
}
}

int main()
{
    ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    // The real constructor is exercised. Teardown is outside this frame-only
    // fixture: placement storage avoids invoking shutdown's plugin dependencies.
    alignas(BF2Render) unsigned char storage[sizeof(BF2Render)];
    BF2Render* render = new (storage) BF2Render(nullptr);
    check(sizeof(BF2Render) == 0x20, "native x86 wrapper size");
    check(g_bf2Render == render, "real constructor publishes wrapper");
    check(!render->m_rendererDll && !render->m_renderer && !render->getWindow(), "real constructor clears collaborators");
    check(render->getDisplayModeIndex() == -1, "constructor initializes signed display index");
    check(!render->getPostProductionFrameWork(), "constructor initializes framework pointer");

    RendererTable rendererTable = {};
    rendererTable.query14 = &query14; rendererTable.query18 = &query18;
    rendererTable.beforeFrame = &beforeFrame; rendererTable.update = &update;
    rendererTable.getView = &getView;
    RendererRecord first = {&rendererTable, 2, 0, 0, 0, 0, 0, false, true, 0, nullptr};
    RendererRecord second = {&rendererTable, 3, 0, 0, 0, 0, 0, true, false, 0, nullptr};
    // [TEMP] The fixture intentionally supplies only the sparse slots it exercises; production
    // owns the complete typed IRenderer implementation.
    dx9rend::IRenderer* firstPtr = reinterpret_cast<dx9rend::IRenderer*>(&first);
    dx9rend::IRenderer* secondPtr = reinterpret_cast<dx9rend::IRenderer*>(&second);
    LogicTable logicTable = {};
    logicTable.isLevelLoaded = &isLevelLoaded;
    LogicRecord logic = {&logicTable, render, 0, false, false, false, false, nullptr};
    gameLogic = reinterpret_cast<IGameLogic*>(&logic);

    render->m_renderer = firstPtr;
    render->beforeFrame(); render->update(3.5);
    check(logic.calls == 0 && first.beforeCalls == 0 && first.updateCalls == 0,
          "absent DLL prevents both renderer and game-logic calls");
    render->m_rendererDll = &rendererTable;
    render->m_renderer = nullptr;
    render->update(3.5);
    check(logic.calls == 0, "update checks renderer before querying game logic");
    check(!beforeFaults(render) && logic.calls == 1,
          "beforeFrame queries game logic with null renderer and stops on unloaded level");
    logic.loaded = true;
    check(beforeFaults(render) && logic.calls == 2,
          "beforeFrame lacks renderer guard when level is loaded");
    gameLogic = nullptr;
    check(beforeFaults(render), "beforeFrame lacks renderer guard with no game logic");
    check(!updateFaults(render), "update skips null renderer with no game logic");

    render->m_renderer = firstPtr;
    order.clear(); render->beforeFrame();
    check(first.beforeCalls == 1 && order == std::vector<int>({2}),
          "no game logic permits one renderer pre-frame call");
    const std::uint64_t timePatterns[] = {
        0x3ff0000000000001ULL, 0xbfd5555555555555ULL, 0x8000000000000000ULL,
        0x0000000000000001ULL, 0x7fefffffffffffffULL
    };
    for (std::uint64_t bits : timePatterns)
    {
        double value; std::memcpy(&value, &bits, sizeof(value));
        const int previous = first.updateCalls;
        render->update(value);
        check(first.updateCalls == previous + 1 && std::memcmp(&first.frameTime, &value, sizeof(value)) == 0,
              "update forwards the complete double including low bits and signed zero");
    }
    gameLogic = reinterpret_cast<IGameLogic*>(&logic);
    logic.loaded = false;
    const int previousBefore = first.beforeCalls, previousUpdate = first.updateCalls;
    order.clear(); render->beforeFrame(); render->update(7.0);
    check(order == std::vector<int>({1,1}) && first.beforeCalls == previousBefore && first.updateCalls == previousUpdate,
          "unloaded level suppresses both renderer callbacks");
    logic.loaded = true;
    order.clear(); render->beforeFrame(); render->update(7.0);
    check(order == std::vector<int>({1,2,1,2}), "loaded level queries once before each renderer call");

    logic.replaceRenderer = true; logic.replacement = secondPtr;
    render->m_renderer = firstPtr;
    order.clear(); render->beforeFrame();
    check(order == std::vector<int>({1,3}) && second.beforeCalls == 1,
          "beforeFrame reloads renderer after game-logic callback");
    render->m_renderer = firstPtr;
    order.clear(); render->update(-13.125);
    check(order == std::vector<int>({1,3}) && second.updateCalls == 1 && second.frameTime == -13.125,
          "update reloads renderer after game-logic callback");
    render->m_renderer = nullptr;
    order.clear(); render->beforeFrame();
    check(order == std::vector<int>({1,3}), "beforeFrame accepts renderer installed during level query");
    render->m_renderer = nullptr;
    order.clear(); render->update(9.0);
    check(order.empty() && !render->m_renderer, "update cannot install renderer through skipped level query");

    logic.replacement = nullptr;
    render->m_renderer = firstPtr;
    check(updateFaults(render), "update does not recheck renderer after callback clears it");
    render->m_renderer = firstPtr;
    check(beforeFaults(render), "beforeFrame uses renderer cleared by callback");
    logic.replacement = secondPtr; logic.clearDll = true; logic.clearLogic = true;
    for (int updating = 0; updating != 2; ++updating)
    {
        render->m_rendererDll = &rendererTable; render->m_renderer = firstPtr;
        gameLogic = reinterpret_cast<IGameLogic*>(&logic);
        order.clear();
        if (updating) render->update(2.25); else render->beforeFrame();
        check(order == std::vector<int>({1,3}) && !render->m_rendererDll && !gameLogic,
              "frame wrapper does not repeat DLL or global-logic guards after callback");
    }

    BorrowedObject view, framework;
    first.view = &view;
    render->m_renderer = firstPtr; render->m_rendererDll = nullptr;
    logic.loaded = false;
    gameLogic = reinterpret_cast<IGameLogic*>(&logic);
    const int queries = logic.calls;
    check(render->getRenderView() == &view && first.viewCalls == 1 && logic.calls == queries,
          "view getter dispatches without DLL or level guard");
    check(view.references == 17 && view.ownershipCalls == 0, "view result is borrowed without reference operations");
    first.view = nullptr;
    check(!render->getRenderView() && first.viewCalls == 2, "view getter preserves null renderer result");
    render->m_postProductionFrameWork = &framework;
    check(render->getPostProductionFrameWork() == &framework && framework.references == 17 && framework.ownershipCalls == 0,
          "framework getter directly borrows stored pointer");
    for (int value : {INT_MIN, -1, 0, INT_MAX})
    {
        render->m_displayModeIndex = value;
        check(render->getDisplayModeIndex() == value, "display getter retains full signed index");
    }
    check(!render->rendererSlot0x14() && render->rendererSlot0x18(), "bool query wrappers preserve false and true");
    first.answer14 = true; first.answer18 = false;
    check(render->rendererSlot0x14() && !render->rendererSlot0x18(), "bool query wrappers preserve opposite answers");
    check(first.query14Calls == 2 && first.query18Calls == 2 && logic.calls == queries,
          "boolean getters dispatch once without DLL or level guards");
    render->m_renderer = nullptr;
    check(getterFaults(render, 0x14), "bool14 has no renderer null guard");
    check(getterFaults(render, 0x18), "bool18 has no renderer null guard");
    check(getterFaults(render, 0x48), "view getter has no renderer null guard");
    gameLogic = nullptr;
    std::printf("BF2Render frame wrappers: %d/%d checks passed (/FORCE uncalled init/shutdown/render/display dependencies).\n", checks-failures, checks);
    return failures ? 1 : 0;
}
