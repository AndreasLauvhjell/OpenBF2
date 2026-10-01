// [TEMP] [CHOSEN, not recovered] bounded window-bundle fixture, never a production target.
// Real constructors, module class lists, template IDs and recovered method calls. Mesh/effect/query
// fixtures observe the specific branches below. Runtime objects live to process exit; no boot,
// renderer, network listener, map-load or runtime-object destruction acceptance is implied.
#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <deque>
#include <fstream>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <DbgHelp.h>
#include <process.h>
#define private public
#define protected public
#include "../BF2/Game/Objects/Game/DestroyableWindowsBundle.h"
#include "../BF2/Game/Objects/TemplateLeaves.h"
#include "../BF2/Game/Objects/Networkables/Networkables.h"
#include "../BF2/Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../BF2/Scene/Object/ObjectManager.h"
#include "../BF2/IO/BitStream.h"
#undef private
#undef protected
#include "../BF2/Core/ClassManager.h"
#include "../BF2/Core/Modules.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/Ids.h"
#include "../BF2/Core/System.h"
#include "../BF2/Game/Common/GameEvent.h"
#include "../BF2/IO/Console/Console.h"
#include "../BF2/IO/Stream/MemStream.h"
#include "../BF2/Math/LocalBoundingBox.h"
#include "../BF2/Physics/CollisionMesh.h"
#include "../BF2/Physics/ResponsePhysicsManager.h"
#include "../BF2/Scene/Object/BinTreeCuller.h"
#include "../BF2/Scene/Object/QuadTreeCuller.h"
#include "../BF2/Game/GameLogic/GameLogicCollision.h"
#include "../BF2/Game/GameServer/GameServer.h"

namespace dice { namespace hfe { extern ClassManager* classManager;
namespace world { extern ResponsePhysicsManager* responsePhysicsManager; } } }
using namespace dice::hfe;
using namespace dice::hfe::world;
namespace {
int checks = 0, failures = 0;
LONG WINAPI reportFault(EXCEPTION_POINTERS* fault)
{
    HANDLE process = GetCurrentProcess();
    SymInitialize(process, 0, TRUE);
    std::fprintf(stderr, "Fixture exception %08lx at %p\n", fault->ExceptionRecord->ExceptionCode,
                  fault->ExceptionRecord->ExceptionAddress);
    CONTEXT context = *fault->ContextRecord;
    STACKFRAME64 frame = {};
    frame.AddrPC.Offset = context.Eip; frame.AddrFrame.Offset = context.Ebp;
    frame.AddrStack.Offset = context.Esp;
    frame.AddrPC.Mode = frame.AddrFrame.Mode = frame.AddrStack.Mode = AddrModeFlat;
    for (int i = 0; i < 16; ++i)
    {
        char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
        SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(storage);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFO); symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 displacement = 0;
        if (SymFromAddr(process, frame.AddrPC.Offset, &displacement, symbol))
            std::fprintf(stderr, "  %08llx %s+%llu\n", frame.AddrPC.Offset, symbol->Name, displacement);
        else
            std::fprintf(stderr, "  %08llx\n", frame.AddrPC.Offset);
        IMAGEHLP_LINE64 line = {}; line.SizeOfStruct = sizeof(line);
        DWORD lineDisplacement = 0;
        if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &lineDisplacement, &line))
            std::fprintf(stderr, "    %s:%lu\n", line.FileName, line.LineNumber);
        if (!StackWalk64(IMAGE_FILE_MACHINE_I386, process, GetCurrentThread(), &frame, &context,
                         0, SymFunctionTableAccess64, SymGetModuleBase64, 0)) break;
    }
    std::fflush(stderr);
    return EXCEPTION_EXECUTE_HANDLER;
}
void check(bool ok, const char* label)
{
    ++checks;
    if (!ok) ++failures;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", label);
    std::fflush(stdout);
}


class FixtureClasses : public ClassManager
{
public:
    std::vector<ClassRegInfo> entries;
    FixtureClasses()
    {
        GameLogicObjTemplBFModule().getClassList(entries);
        GameLogicObjTemplBaseModule().getClassList(entries);
        ObjTemplBaseModule().getClassList(entries);
        ModuleNetworkables().getClassList(entries);
    }
    IBase* createInstance(unsigned int cid, unsigned int iid, IBase* outer) override
    {
        if (cid == CID_BinTreeCuller) return BinTreeCuller::create(iid, outer);
        if (cid == CID_QuadTreeCuller) return QuadTreeCuller::create(iid, outer);
        for (const auto& e : entries) if (e.classId == cid) return e.create(iid, outer);
        return 0; // deliberately absent class used to verify the retail failure branch
    }
    bool getClassName(unsigned int cid, std::string& name) override
    {
        for (const auto& e : entries) if (e.classId == cid) { name = e.name; return true; }
        return false;
    }
};
class FixtureTemplates : public ObjectTemplateManager
{
public:
    mutable unsigned int lookups = 0;
    IObjectTemplate* effect = 0;
    mutable std::string requested;
    IObjectTemplate* getTemplate(const std::string& name) const override
    {
        if (name.find("e_destroyableWindow_") == 0 || name.find("custom_") == 0)
        { ++lookups; requested = name; return effect; }
        return ObjectTemplateManager::getTemplate(name);
    }
};
class FixtureObjects : public ObjectManager
{
public:
    int posted = 0;
    IObject* target = 0;
    TemplateMessage message;
    float delay = 0;
    void postMessage(IObject* to, TemplateMessage msg, float time) override
    { ++posted; target = to; message = msg; delay = time; }
};
class FixtureMesh : public CollisionMesh
{
public:
    FixtureMesh() : CollisionMesh(static_cast<CollisionMeshTemplate*>(0))
    { box.m_min = {0,0,0}; box.m_max = {2,3,0}; }
    LocalBoundingBox box;
    Vec3 vertices[3] = {{0,0,0},{2,0,0},{0,3,0}};
    unsigned short faces[3] = {0,1,2};
    std::vector<int> calls;
    bool hasLod(int lod) override { calls.push_back(10 + lod); return lod == 2; }
    const LocalBoundingBox* getBoundingBox(int lod) override { calls.push_back(20 + lod); return &box; }
    const void* getFaceVector(int lod) override { calls.push_back(30 + lod); return faces; }
    const void* getVertexVector(int lod) override { calls.push_back(40 + lod); return vertices; }
};
class FixtureEffect : public SimpleObject
{
public:
    explicit FixtureEffect(SimpleObjectTemplate* t) : SimpleObject(t) {}
    std::vector<int> calls;
    Mat4 transform = Mat4::getZero();
    void setAbsoluteTransformation(const Mat4& m) override { calls.push_back(1); transform = m; }
    void init() override { calls.push_back(2); }
    void handleMessage(TemplateMessage msg, IPlayer* player, bool flag) override
    { check(msg == 0 && !player && !flag, "effect activation message"); calls.push_back(3); }
};
class FixtureEffectTemplate : public SimpleObjectTemplate
{
public:
    FixtureEffect* effect = 0;
    IObject* createObject() const override { return effect; }
};
// [TEMP] Explicit PE carrier for only the renderer slots exercised by updateStates.
// This is not an IGeometry declaration or an engine factory. Unused entries remain null.
struct FixtureGeometry
{
    void** table;
    void* slots[52] = {};
    FixtureGeometry* secondary = 0;
    int gets = 0;
    std::vector<int> parts;
    std::vector<Mat4> transforms;
    FixtureGeometry() : table(slots)
    {
        slots[0x4C/4] = reinterpret_cast<void*>(&setPart);
        slots[0xCC/4] = reinterpret_cast<void*>(&getSecondary);
    }
    static void __fastcall setPart(FixtureGeometry* self, void*, const Mat4& transform, int part)
    { self->parts.push_back(part); self->transforms.push_back(transform); }
    static void* __fastcall getSecondary(FixtureGeometry* self, void*)
    { ++self->gets; return self->secondary; }
};
std::string script(SimpleObjectTemplate* t)
{
    io::MemStream stream;
    check(t->makeScript(static_cast<io::IMemStream*>(&stream)), "template script writer succeeds");
    return std::string(static_cast<const char*>(stream.getPtr()), stream.getSize());
}
}
int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(reportFault);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    eventManager = new EventManager();
    responsePhysicsManager = new ResponsePhysicsManager();
    classManager = new FixtureClasses();
    io::mainConsole = new io::OldConsole();
    FixtureTemplates* templates = new FixtureTemplates();
    objectTemplateManager = templates;
    auto* before = templates->createTemplate(CID_SimpleObjectTemplate, "before_windows");
    auto* t = static_cast<DestroyableWindowsBundleTemplate*>(
        templates->createTemplate(CID_DestroyableWindowsBundleTemplate, "windows"));
    auto* paneTemplate = static_cast<DestroyableWindowTemplate*>(
        templates->createTemplate(CID_DestroyableWindowTemplate, "pane"));
    auto* after = templates->createTemplate(CID_SimpleObjectTemplate, "after_windows");
    check(before && t && paneTemplate && after, "real module factories create both window templates");
    if (!before || !t || !paneTemplate || !after) return 1;
    check(t->getId() == before->getId()+1 && paneTemplate->getId() == t->getId()+1 &&
          after->getId() == paneTemplate->getId()+1, "template ID continuity through the actual bundle");
    const unsigned int next = templates->m_objectTemplateIdCounter;
    check(!templates->createTemplate(0xffffffffu, "absent_class") &&
          templates->m_objectTemplateIdCounter == next, "failed factory no longer consumes a reserved ID");
    check(templates->getTemplate("windows") == t, "bundle is published in the template name index");
    check(t->respawnDelay == -1 && t->clientHitDetection && t->numWindows == 0 &&
          t->windowSizeVector.x == .2f && t->windowSizeVector.z == 1.8f, "bundle defaults");
    check(paneTemplate->getRef() == 1 && paneTemplate->_dword168 == 0 && paneTemplate->_dword16C == 1,
          "pane constructor initialises its own three words");
    auto* bundle = static_cast<DestroyableWindowsBundle*>(t->createObject());
    auto* pane = static_cast<DestroyableWindow*>(paneTemplate->createObject());
    auto* ignoredTemplate = new DestroyableWindowTemplate();
    auto* ignoredPane = static_cast<DestroyableWindow*>(ignoredTemplate->createObject());
    paneTemplate->m_geometryPart = 2;
    bundle->addChild(pane);
    bundle->addChild(ignoredPane);
    bundle->updateStates(~0u);
    check(!pane->getIsBroken() && !ignoredPane->getIsBroken(), "initial intact state");
    bundle->handleChildCollision(pane);
    check(pane->getIsBroken() && !ignoredPane->getIsBroken() &&
          bundle->m_states == 0xfffffffbu, "collision clears geometry part two, skips minus-one part");
    check(!(pane->m_flags & 0x100), "breaking removes collision flag");
    bundle->updateStates(~0u);
    check(!pane->getIsBroken() && (pane->m_flags & 0x100), "restoration restores collision flag");
    paneTemplate->m_geometryPart = 34;
    bundle->handleChildCollision(pane);
    check(bundle->m_states == 0xfffffffbu, "x86 shift count wraps at 32");
    paneTemplate->m_geometryPart = 2;
    bundle->m_states = 0x42;
    bundle->m_previousStates = 7;
    bundle->updateStates(7);
    check(bundle->m_states == 0x42 && bundle->m_previousStates == 0x42, "equal previous state skips assignment but refreshes previous");
    bundle->updateStates(~0u);
    Vec3 zero = {};
    check(!bundle->handleCollision(0,zero,zero,zero,0,0,0,0), "retail root collision declines");
    pane->setIsBroken(true);
    check(!pane->handleCollision(0,zero,zero,zero,0,0,0,0), "already-broken pane collision declines");
    check(bundle->queryInterface(IID_IObject) == static_cast<IObject*>(bundle) &&
          bundle->queryInterface(io::IID_IConsoleSaveable) &&
          !bundle->queryInterface(0xffffffffu), "bundle interface identities");
    bundle->addRef(); check(bundle->release() == 1, "bundle reference count");
    pane->initNetworkableComp(); check(!pane->getNetworkable(), "pane leaves replication to its parent");

    FixtureEffectTemplate* effectTemplate = new FixtureEffectTemplate();
    FixtureEffect* effect = new FixtureEffect(effectTemplate);
    effectTemplate->effect = effect;
    templates->effect = effectTemplate;
    LocalBoundingBox bounds;
    bounds.m_min = {0,0,0}; bounds.m_max = {1.8f,1.0f,0};
    check(t->getEffectTemplate(bounds) == effectTemplate &&
          templates->requested == "e_destroyableWindow_1X0", "effect thresholds use strict greater-than");
    unsigned int lookups = templates->lookups;
    check(t->getEffectTemplate(bounds) == effectTemplate && templates->lookups == lookups, "effect cache avoids a repeated lookup");
    bounds.m_max = {3,3,0}; bounds.m_derivedDataDirty = true;
    check(t->getEffectTemplate(bounds) == effectTemplate &&
          templates->requested == "e_destroyableWindow_2X2", "largest effect bucket and exact retail name");
    FixtureMesh* mesh = new FixtureMesh();
    pane->m_collisionMesh = mesh;
    pane->setIsBroken(false);
    Mat4 absolute = Mat4::getIdentity(); absolute.m[12] = 10;
    pane->setAbsoluteTransformation(absolute);
    effect->calls.clear();
    pane->setIsBroken(true);
    check(mesh->calls == std::vector<int>({12,22,32,42}), "LOD2 mesh queries keep retail order");
    check(effect->calls == std::vector<int>({1,2,3}), "break effect sets transform, initialises, activates");
    check(effect->transform.m[12] == 11 && effect->transform.m[13] == 1.5f &&
          effect->transform.m[0] == -1 && effect->transform.m[5] == 1, "effect uses transformed box centre and triangle normal");
    pane->setIsBroken(true);
    check(effect->calls.size() == 3, "unchanged broken state does not replay effect");
    pane->m_collisionMesh = 0;
    bundle->updateStates(0);
    FixtureGeometry primary, secondary;
    primary.secondary = &secondary;
    bundle->m_geometry = &primary;
    bundle->updateStates(~0u);
    bundle->updateStates(0);
    check(primary.parts == std::vector<int>({2,2}) && secondary.parts == primary.parts,
          "renderer seam receives geometry-part numbers");
    check(primary.gets == 4 && primary.transforms[0].m[15] == 1 && primary.transforms[1].m[15] == 0 &&
          secondary.transforms[0].m[15] == 0 && secondary.transforms[1].m[15] == 1,
          "intact and broken geometry swap transforms with repeated secondary lookup");
    bundle->m_geometry = 0;
    const Vec3 pos = {4,5,6}, up = {0,2,0}, normal = {0,0,1};
    Mat4 frame = GameLogicCollision::getCollisionEffectTransformation(pos, up, normal);
    check(frame.m[0] == -1 && frame.m[5] == 1 && frame.m[10] == -2 &&
          frame.m[12] == 4 && frame.m[15] == 1, "effect frame preserves unnormalised third-axis input");
    const Vec3 unitUp = {0,1,0};
    frame = GameLogicCollision::getCollisionEffectTransformation(pos, unitUp, unitUp);
    check(frame.m[0] == 1 && frame.m[5] == 1 && frame.m[10] == 1 && frame.m[14] == 6,
          "parallel effect vectors retain identity rotation and translation");

    std::string text = script(t);
    check(text.find("ObjectTemplate.respawnDelay -1\n") != std::string::npos &&
          text.find("ObjectTemplate.clientHitDetection 1\n") != std::string::npos &&
          text.find("ObjectTemplate.setNumWindows 0\n") != std::string::npos, "script mandatory bundle properties");
    check(text.find("ObjectTemplate.effectTemplateBaseName ") == std::string::npos &&
          text.find("ObjectTemplate.setWindowSizeVector ") == std::string::npos, "default optional properties omitted");
    t->effectTemplateBaseName = "custom_"; t->windowSizeVector = {.5f,2,4};
    text = script(t);
    check(text.find("ObjectTemplate.effectTemplateBaseName custom_\n") != std::string::npos &&
          text.find("ObjectTemplate.setWindowSizeVector 0.5/2/4\n") != std::string::npos, "script custom effect properties");
    script(paneTemplate);

    auto* wire = static_cast<DestroyableWindowsBundleNetworkable*>(
        static_cast<io::INetworkable*>(classManager->createInstance(
            CID_DestroyableWindowsBundleNetworkable, io::IID_INetworkable, 0)));
    t->numWindows = 5;
    bundle->m_child = 0; // bounded wire test has no collision/effect side effects
    bundle->updateStates(0x15);
    check(wire && wire->init(bundle) && wire->m_baselines[0].paneState == 0x15,
          "registered networkable factory and initial baseline");
    if (argc > 1 && std::string(argv[1]) == "--truncated")
    {
        std::signal(SIGABRT, [](int) { std::fflush(0); _exit(86); });
        io::BitStream truncated(1);
        truncated.setBitSize(4); truncated.setReadMode();
        wire->setNetUpdate(truncated,1,false,0,0);
        std::fprintf(stderr,"Truncated update returned unexpectedly\n"); return 2;
    }
    check(wire->getMinimumBitSize() == 32, "retail minimum-bit estimate remains 32");
    unsigned int mask = 8; wire->updateStateMask(&mask);
    check(mask == 8, "unchanged state preserves unrelated dirty bits");
    bundle->updateStates(0x0a); wire->updateStateMask(&mask);
    check(mask == 9, "pane change ORs dirty bit zero");
    unsigned char bytes[8] = {};
    io::BitStream stream(bytes,8);
    const unsigned int prefix = 3;
    stream.writeBits(&prefix,2);
    const int writeBefore = wire->m_stats.m_writeBits;
    check(wire->getNetUpdate(stream,0,0,0) && stream.getBitPosition() == 7 &&
          bytes[0] == 0x2b && wire->m_stats.m_writeBits == writeBefore+5,
          "wire writes five pane bits without a state-mask prefix");
    wire->predict(10);
    bundle->updateStates(0);
    stream.setReadMode();
    unsigned int readPrefix = 0; stream.readBits(&readPrefix,2);
    const int readBefore = wire->m_stats.m_readBits;
    wire->setNetUpdate(stream,3.5f,false,0,0);
    check(bundle->m_states == 0x0a && wire->m_baselines[wire->m_next].time == 3.5f &&
          wire->m_stats.m_readBits == readBefore+5, "wire round-trip applies pane mask and timestamp");
    io::BitStream tooSmall(1); tooSmall.m_bitLimit = 4;
    const int countBefore = wire->m_stats.m_writeBits;
    check(!wire->getNetUpdate(tooSmall,0,~0u,0) && !tooSmall.hasError() &&
          wire->m_stats.m_writeBits == countBefore, "short output reports failure and clears stream error");
    check(!DestroyableWindowsBundleNetworkable::create(io::IID_INetworkable,
          static_cast<io::INetworkable*>(wire)), "factory rejects aggregation");
    check(!DestroyableWindowsBundleNetworkable::create(0xffffffffu,0), "factory releases unsupported interface");
    check(wire->release() == 0, "networkable destroys its baseline ring");

    auto* objects = new FixtureObjects();
    objectManager = objects;
    g_system = new System();
    gameEventRegistry = new GameEventRegistry();
    game = new GameServer();
    t->respawnDelay = 4;
    bundle->updateStates(0); bundle->updateStates(~0u);
    check(objects->posted == 1 && objects->target == bundle && objects->message == 29 && objects->delay == 4,
          "restoration schedules message 29 with template delay");
    bundle->updateStates(~0u);
    check(objects->posted == 1, "unchanged intact state does not reschedule");
    bundle->updateStates(0);
    check(objects->posted == 1, "break-all does not schedule restoration");
    bundle->handleMessage(static_cast<TemplateMessage>(29),0,false);
    check(bundle->m_states == ~0u && objects->posted == 2, "message 29 restores state and reschedules");
    std::printf("Window bundle runtime: %d checks, %d failures\n",checks,failures);
    return failures ? 1 : 0;
}
