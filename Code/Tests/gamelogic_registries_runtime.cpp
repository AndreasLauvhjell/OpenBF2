// [TEMP] [CHOSEN, not recovered] component fixture. Real constructors and production bodies;
// observer overrides capture only collaborators. No boot, socket, map, or client acceptance.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <DbgHelp.h>
#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <fstream>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>
#define private public
#define protected public
#include "../BF2/Game/GameLogic/ServerGameLogic.h"
#include "../BF2/Game/GameLogic/Player.h"
#include "../BF2/Game/Objects/Object.h"
#include "../BF2/Game/Objects/StrategicObjectComp.h"
#include "../BF2/Game/GameEvents/PythonCommandEvent.h"
#include "../BF2/Game/Common/ClientConnection.h"
#undef protected
#undef private
#include "../BF2/Game/Objects/IStrategicObjectComp.h"
#include "../BF2/Game/Objects/StrategicObjectCompTemplate.h"
#include "../BF2/Game/Objects/ObjectLayoutAccess.h"
#include "../BF2/Game/GameServer/GameServer.h"
#include "../BF2/Game/GameLogic/ScoreSystem/ScoreManager.h"
#include "../BF2/Game/SingletonServer.h"
#include "../BF2/Core/ClassManager.h"
#include "../BF2/Core/Debug.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/Ids.h"
#include "../BF2/Core/System.h"
#include "../BF2/Core/SettingsRepostitory.h"
#include "../BF2/Scene/Object/ObjectManager.h"
#include "../BF2/Scene/Object/PlayerManager.h"
#include "../BF2/Scene/Object/BinTreeCuller.h"
#include "../BF2/Scene/Object/QuadTreeCuller.h"
#include "../BF2/IO/Network/NetworkManager.h"
#include "../BF2/Game/Python/PythonHost.h"
#include "../BF2/Game/Python/PyRef.h"
#include "../BF2/Game/Persistence/PlayerStats.h"

using namespace dice::hfe;
using namespace dice::hfe::world;
namespace dice { namespace hfe {
extern ClassManager* classManager;
namespace world { extern IBase* scoreManager; }
} }
namespace {
int checks = 0, failures = 0;
std::vector<int> calls;
void check(bool ok, const char* name)
{
    ++checks; if (!ok) ++failures;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", name); std::fflush(stdout);
}
[[noreturn]] void unused() { std::fputs("Unexpected fixture operation\n", stderr); std::abort(); }
LONG WINAPI reportFault(EXCEPTION_POINTERS* fault)
{
    HANDLE process = GetCurrentProcess(); SymInitialize(process, 0, TRUE);
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
        if (!StackWalk64(IMAGE_FILE_MACHINE_I386, process, GetCurrentThread(), &frame, &context,
                        0, SymFunctionTableAccess64, SymGetModuleBase64, 0)) break;
    }
    std::fflush(stderr); return EXCEPTION_EXECUTE_HANDLER;
}
#define UNUSED(signature) signature override { unused(); }
#define BASE_OBSERVER \
    UNUSED(void addRef()) \
    UNUSED(int getRef()) \
    UNUSED(int release()) \
    UNUSED(void* queryInterface(unsigned int) const)
struct Names : INametagManager
{
    BASE_OBSERVER
    UNUSED(void init(const std::string&, const std::string&))
    UNUSED(void update()) UNUSED(void draw())
    bool addTouchItem(IPlayer*) override { return false; }
    UNUSED(void addTouchItem(ControlPoint*))
    void addTouchItem(IObject*) override { calls.push_back(30); }
    UNUSED(void addTouchItem(SquadOrder*, int, const Vec3&))
    UNUSED(void addTouchItem(SquadOrder*, int, IObject*))
    UNUSED(void addTouchItem(IObject*, int))
    UNUSED(void* getColors(int)) UNUSED(void* getTransformations(int)) UNUSED(void* getAlphaArray(int))
    UNUSED(void rebufferAll()) UNUSED(void enableBuffering()) UNUSED(void clearAll())
    void removeItem(int type, void*) override { check(type == 2, "strategic nametag removal type"); calls.push_back(31); }
    UNUSED(void removeItem(int, void*, int)) UNUSED(const void* getItems() const)
    UNUSED(bool get3dMapOn() const) UNUSED(void set3dMapOn(bool))
    UNUSED(void setSpectatedPlayer(IPlayer*)) UNUSED(IPlayer* getSpectatedPlayer() const)
    UNUSED(bool isDemoPlayback() const)
};
struct Strategic : StrategicObjectComp
{
    // Real component storage; overrides below observe GameLogic's call ordering.
    Strategic() : StrategicObjectComp(nullptr), holder(m_holder) {}
    StrategicObjectHolder*& holder;
    std::string name = "bridge";
    int index = -1;
    bool onePiece = false, icon = false;
    BASE_OBSERVER
    UNUSED(void setObject(IObject*)) UNUSED(IObject* getObject() const)
    const std::string& getStrategicObjectName() override { calls.push_back(10); return name; }
    UNUSED(const std::string& getIntactIconName()) UNUSED(const std::string& getDestroyedIconName())
    UNUSED(int getIntactIconPtr()) UNUSED(int getDestroyedIconPtr()) UNUSED(int getPart())
    int getIndex() override { calls.push_back(11); return index; }
    void setIndex(int i) override { calls.push_back(20); index = i; }
    UNUSED(void* getHudInfo()) UNUSED(void* getHudIcons())
    bool has3dMapIcon() const override { calls.push_back(21); return icon; }
    UNUSED(int get3dMapIcon() const)
    bool isOnePiece() const override { calls.push_back(12); return onePiece; }
    UNUSED(bool isPartOfDestroyedHolder() const) UNUSED(const Vec3& getHolderPosition() const)
};
// Deliberately limited ABI collaborator: only IBase and the observed +0x34 call are used.
struct Particles : IBase
{
    BASE_OBSERVER
    virtual void slot4() { unused(); } virtual void slot5() { unused(); }
    virtual void slot6() { unused(); } virtual void slot7() { unused(); }
    virtual void slot8() { unused(); } virtual void slot9() { unused(); }
    virtual void slot10() { unused(); } virtual void slot11() { unused(); }
    virtual void slot12() { unused(); }
    virtual void pause() { calls.push_back(403); }
};
#undef BASE_OBSERVER
#undef UNUSED
struct Classes : ClassManager
{
    IBase* particles = 0;
    IBase* getSingleton(const std::string& name) override
    { if (name == "ParticleSystemManager") { calls.push_back(402); return particles; } return ClassManager::getSingleton(name); }
    IBase* createInstance(unsigned int cid, unsigned int iid, IBase* outer) override
    {
        if (cid == CID_BinTreeCuller) return BinTreeCuller::create(iid, outer);
        if (cid == CID_QuadTreeCuller) return QuadTreeCuller::create(iid, outer);
        unused();
    }
};
struct Events : EventManager
{
    int status = -1;
    unsigned int postEvent(EventCategory category, unsigned int id, void* data, float delay) override
    {
        if (category == ECGameLogic && id == 1)
        { check(delay == 0, "pause status has zero local delay"); status = *static_cast<int*>(data); calls.push_back(401); }
        return 0;
    }
};
struct Players : PlayerManager
{
    IPlayer* local = 0;
    int reads = 0;
    std::vector<int> ids;
    IPlayer* getLocalHumanPlayer() override { ++reads; return local; }
    IPlayer* registered = 0;
    IPlayer* getPlayerFromId(int id) override
    { ids.push_back(id); return registered && registered->getId() == id ? registered : 0; }
};
struct Server : GameServer
{
    GameEvent* sent = 0;
    int target = 0, status = -1;
    void sendGameEvent(unsigned int id, GameEvent* event) override { target = static_cast<int>(id); sent = event; calls.push_back(300); }
    void sendGameEventToAllPlaying(GameEvent* event) override { delete event; }
    void sendGameEventToAllConnected(GameEvent* event) override
    {
        PostRemoteEvent* p = static_cast<PostRemoteEvent*>(event);
        check(p->m_eventCategory == ECGameLogic && p->m_eventId == 1 && p->m_payloadBitSize == 4,
              "status broadcast uses four-bit ECGameLogic event");
        std::memcpy(&status, p->m_payload, sizeof(status)); calls.push_back(400); delete event;
    }
};
struct Logic : ServerGameLogic
{
    int toggleCalls = 0, killCalls = 0;
    void togglePause() override { ++toggleCalls; ServerGameLogic::togglePause(); }
    bool killPlayer(IPlayer* player, bool a, bool b) override
    { check(player && a && b, "suicide forwards player and both kill flags"); ++killCalls; calls.push_back(602); return false; }
};
struct ObjectProbe : Object
{
    int label;
    float radius = 1;
    Vec3 position = {};
    Mat4 matrix = {};
    explicit ObjectProbe(int id) : Object(0), label(id) { m_objectId = static_cast<unsigned int>(id); }
    float getPhysicBoundingRadius() const override { calls.push_back(100 + label); return radius; }
    const Vec3& getAbsolutePosition() const override { calls.push_back(200 + label); return position; }
    const Mat4& getAbsoluteTransformation() const override { calls.push_back(503); return matrix; }
    void setAbsoluteTransformation(const Mat4& m) override { calls.push_back(504); matrix = m; }
};
struct Actor : Player
{
    IObject* vehicle = 0; IObject* camera = 0;
    bool alive = false, setValue = false;
    int setCalls = 0, readCalls = 0;
    Actor() : Player(-1, 0) {}
    IObject* getVehicle() const override { calls.push_back(501); return vehicle; }
    IObject* getFreeCamera() const override { calls.push_back(502); return camera; }
    void toggleFreeCameraEnabled() override { calls.push_back(505); Player::toggleFreeCameraEnabled(); }
    bool getFreeCameraEnabled() const override { ++const_cast<Actor*>(this)->readCalls; return false; }
    void setFreeCameraEnabled(bool value) override { ++setCalls; setValue = value; }
    void setSuicide(bool value) override { check(value, "suicide marks true before alive query"); calls.push_back(600); }
    bool getIsAlive() const override { calls.push_back(601); return alive; }
};
struct DebugConnection : ClientConnection
{
    int debugCalls = 0;
    DebugConnection() : ClientConnection(static_cast<ConnectionDemoMode>(0)) {}
    void debugShowActiveGhosts() override { ++debugCalls; }
};
}
int main()
{
    static_assert(sizeof(void*) == 4, "x86 only");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _CALL_REPORTFAULT); SetUnhandledExceptionFilter(reportFault);
    g_system = new System; settings = new SettingsRepostitory;
    auto* events = new Events; eventManager = events;
    auto* classes = new Classes; classManager = classes;
    objectManager = new ObjectManager;
    auto* players = new Players; playerManager = players;
    gameEventRegistry = new GameEventRegistry;
    auto* server = new Server; game = server;
    io::networkManager = new io::NetworkManager;
    scoreManager = reinterpret_cast<IBase*>(new ScoreManager);
    auto* logic = new Logic; gameLogic = logic;
    IGameLogic* api = logic;
    check(!api->close() && !logic->GameLogic::close(), "both recovered close implementations return false");

    auto* a = new ObjectProbe(1); auto* b = new ObjectProbe(2); auto* c = new ObjectProbe(3);
    b->m_objectId = a->m_objectId;
    api->addFlag(a); api->addFlag(b);
    auto* flags = static_cast<std::map<unsigned int,IObject*>*>(api->getFlagMap());
    check(flags->size() == 1 && flags->at(1) == a, "duplicate flag id keeps original pointer");
    api->removeFlag(b); api->removeFlag(c);
    check(flags->empty(), "removal uses id without pointer equality; absent id is inert");
    a->m_objectId = 0xffffffffu; api->addFlag(a);
    check(flags->at(0xffffffffu) == a, "flag keys retain unsigned high bits"); api->removeFlag(a);
    WorldPref::resetTickCount(100); logic->m_nextStateChangeAtTick = 130;
    check(std::fabs(api->getTimeToNextStateChange()-1.0f) < 1e-6f, "future state counts thirty ticks per second");
    logic->m_nextStateChangeAtTick = 100; check(api->getTimeToNextStateChange() == 0, "state at current tick returns zero");
    logic->m_nextStateChangeAtTick = 99; check(api->getTimeToNextStateChange() == 0, "past state returns zero");
    WorldPref::resetTickCount(0); logic->m_nextStateChangeAtTick = 0x80000000u;
    check(api->getTimeToNextStateChange() > 7e7f, "state countdown uses unsigned delta");
    WorldPref::resetTickCount(0xfffffff0u); logic->m_nextStateChangeAtTick = 14;
    check(api->getTimeToNextStateChange() == 0, "state comparison does not treat wrapped tick as future");
    api->refreshTriggerables(); check(true, "null triggerable manager wrapper returns");

    auto* names = new Names; SingletonServer::m_iNametagManager = names;
    auto* sa = new Strategic; auto* sb = new Strategic; auto* sc = new Strategic;
    check(reinterpret_cast<char*>(&sa->holder)-reinterpret_cast<char*>(sa) == 0x14, "fixture models only attested holder field offset");
    object_detail::LayoutAccess::strategicObjectComponent(a) = sa;
    object_detail::LayoutAccess::strategicObjectComponent(b) = sb;
    object_detail::LayoutAccess::strategicObjectComponent(c) = sc;
    logic->m_isPrecaching = true; calls.clear(); api->addStrategicObject(0);
    check(calls.empty() && logic->m_strategicObjectMap.empty(), "precaching returns before reading object");
    logic->m_isPrecaching = false; sa->icon = true;
    calls.clear(); api->addStrategicObject(a);
    check(sa->index == 0 && sa->holder == logic->m_strategicObjectMap.at("bridge0") &&
          calls == std::vector<int>({10,20,21,30}), "first strategic piece publishes holder before nametag");
    b->position.x = 4; calls.clear(); api->addStrategicObject(b);
    check(sb->index == 1 && calls == std::vector<int>({10,12,102,101,201,202,20,21}),
          "exact twice-radius boundary starts new holder with PE query order");
    c->position.x = 3.999f; api->addStrategicObject(c);
    check(sc->holder == sa->holder && sa->holder->m_nrObjects == 2, "piece strictly inside boundary shares holder");
    calls.clear(); api->removeStrategicObject(a);
    check(calls == std::vector<int>({31,10,11}) && sa->holder->getObjectByIndex(0) == 0,
          "removal precedes name lookup and leaves first slot hole");
    a->position.x = 10000; api->addStrategicObject(a);
    check(sa->index == 0 && sa->holder == sc->holder, "first-slot hole permits distant reuse");
    api->removeStrategicObject(a); auto* former = sc->holder; api->removeStrategicObject(c);
    check(!logic->m_strategicObjectMap.count("bridge0") && sc->holder == former,
          "last removal erases holder without clearing component pointer");
    api->removeStrategicObject(b);
    sc->name = "absent"; calls.clear(); api->removeStrategicObject(c);
    check(calls == std::vector<int>({31,10,11}), "absent holder still removes nametag");
    std::vector<ObjectProbe*> pieces;
    for (int i=0; i<11; ++i)
    {
        auto* piece = new ObjectProbe(20+i); auto* component = new Strategic;
        component->name = "single"; component->onePiece = true;
        object_detail::LayoutAccess::strategicObjectComponent(piece) = component;
        api->addStrategicObject(piece); pieces.push_back(piece);
    }
    check(logic->m_strategicObjectMap.count("single:") == 1 && !logic->m_strategicObjectMap.count("single10"),
          "one-piece groups append ASCII punctuation after index nine");
    for (auto* piece : pieces) api->removeStrategicObject(piece);
    check(logic->m_strategicObjectMap.empty(), "all strategic holders removed");

    // Exercise the recovered runtime factory/init/destroy with the real GameLogic registry.
    // Object position and nametag collaborators remain the explicit observers above.
    StrategicObjectCompTemplate actualTemplate;
    actualTemplate.setStrategicObjectName("actual");
    auto* actualA=static_cast<StrategicObjectComp*>(actualTemplate.createComponent());
    auto* actualB=static_cast<StrategicObjectComp*>(actualTemplate.createComponent());
    object_detail::LayoutAccess::strategicObjectComponent(a)=actualA;
    object_detail::LayoutAccess::strategicObjectComponent(b)=actualB;
    a->position=Vec3{0,0,0}; b->position=Vec3{2,0,0};
    actualA->init(a); actualB->init(b);
    auto* actualHolder=logic->m_strategicObjectMap.at("actual0");
    check(actualA->getObject()==a && actualB->getObject()==b && actualA->m_holder==actualHolder
          && actualB->m_holder==actualHolder && actualHolder->m_nrObjects==2,
          "real component init registers and joins the holder through GameLogic");
    check(actualA->getHolderPosition().x==1 && &actualA->getHolderPosition()==&actualB->getHolderPosition(),
          "real runtime returns shared live-piece average reference");
    actualHolder->setNrDestroyedObjects(1);
    check(actualA->isPartOfDestroyedHolder() && actualB->isPartOfDestroyedHolder(),
          "destroyed holder count propagates to both runtime components");
    actualHolder->setNrDestroyedObjects(0);
    check(!actualA->isPartOfDestroyedHolder(), "repaired holder count clears runtime query");
    actualHolder->setNrDestroyedObjects(-1);
    check(!actualB->isPartOfDestroyedHolder(), "negative destroyed count retains retail greater-than-zero test");
    actualA->destroy();
    check(actualHolder->getObjectByIndex(0)==nullptr && actualB->getHolderPosition().x==2,
          "real component destroy leaves hole and excludes it from the average");
    actualB->destroy();
    check(logic->m_strategicObjectMap.empty() && actualB->m_holder==actualHolder && actualB->getObject()==b,
          "real last-component destroy erases holder and preserves stale fields");
    object_detail::LayoutAccess::strategicObjectComponent(a)=nullptr;
    object_detail::LayoutAccess::strategicObjectComponent(b)=nullptr;
    check(actualA->release()==0 && actualB->release()==0 && actualTemplate.getRef()==1,
          "real runtime release leaves its non-owned template alive");

    calls.clear(); players->ids.clear();
    check(api->remoteCommand(-7, -1, 0x12345678, 0, 0), "remote command reports true");
    auto* command = static_cast<PythonCommandEvent*>(server->sent);
    check(server->target == -1 && command->m_playerId == 255 && command->m_commandId == -7 &&
          command->m_args[0] == -1 && command->m_args[1] == 0x12345678 && command->m_argCount == 2 &&
          players->ids == std::vector<int>({255}), "remote command executes locally then sends exact absent-player payload");
    delete command;
    auto* actor = new Actor; players->local = actor;
    players->ids.clear(); api->remoteCommand(6, 1, 2, 3, 4);
    command = static_cast<PythonCommandEvent*>(server->sent);
    check(command->m_playerId == actor->getId() && command->m_argCount == 4 &&
          players->ids == std::vector<int>({actor->getId()}), "local command sends player id after server event's missing-id arm");
    delete command;
    players->registered = actor;
    players->ids.clear(); api->remoteCommand(6, 1, 2, 3, 4);
    check(players->ids == std::vector<int>({actor->getId(), 2, 3}),
          "registered local command reaches real uninitialized buildArgs and both discarded lookups");
    delete server->sent;
    players->local = 0; api->toggleLocalFreeCamera(); check(true, "missing local player skips free camera work");
    players->local = actor; actor->vehicle = a; actor->camera = b;
    a->matrix.m[12] = 37.0f; calls.clear(); api->toggleLocalFreeCamera();
    check(calls == std::vector<int>({501,502,503,504,505}) && b->matrix.m[12] == 37.0f,
          "free camera receives vehicle transform before toggle in PE order");
    check(actor->setCalls == 1 && !actor->setValue && actor->readCalls == 0,
          "Player toggle reads stored flag and dispatches virtual setter only");
    actor->m_freeCameraEnabled = false; actor->Player::toggleFreeCameraEnabled();
    check(actor->setValue && actor->setCalls == 2, "cleared free-camera flag toggles on");
    calls.clear(); api->suicide();
    check(calls == std::vector<int>({600,601,602}) && logic->killCalls == 1, "dead local player still reaches virtual kill");
    actor->alive = true; calls.clear(); api->suicide();
    check(calls == std::vector<int>({600,601,602}) && logic->killCalls == 2, "living suicide runs uninitialized-Python arm then kill");
    players->local = 0; turnOffAllDebug(DEBUGTYPE_6); api->suicide();
    check(logic->killCalls == 2, "missing local player does not kill when fixture disables Debug termination");

    logic->m_gameStatus = 1; calls.clear(); api->setPause(false);
    check(calls.empty() && logic->toggleCalls == 0, "matching pause request has no side effects");
    classes->particles = new Particles; calls.clear(); api->setPause(true);
    check(api->getPause() && logic->m_statusBeforePause == 1 &&
          server->status == 4 && events->status == 4 && calls == std::vector<int>({400,401,402,403}),
          "pause directly broadcasts status before optional particle dispatch");
    calls.clear(); api->setPause(true);
    check(calls.empty() && logic->toggleCalls == 1, "repeated pause request does not retoggle");
    classes->particles = 0; calls.clear(); api->setPause(false);
    check(!api->getPause() && logic->m_gameStatus == 1 && calls == std::vector<int>({400,401,402}),
          "unpause restores saved status with null particle manager");
    logic->m_gameStatus = -123; api->togglePause(); api->togglePause();
    check(logic->m_gameStatus == -123, "pause preserves arbitrary prior status without sanitizing");
    logic->setOutputPlayerStats(true); check(logic->getOutputPlayerStats(), "output-player-stats set");
    logic->setOutputPlayerStats(false); check(!logic->getOutputPlayerStats(), "output-player-stats clear");
    IGameServer* serverApi = server;
    calls.clear(); serverApi->toggleFreeCamera(actor);
    check(calls == std::vector<int>({501,502,503,504,505}), "GameServer free-camera wrapper reaches real server logic");
    check(!serverApi->useContentChecking() && !serverApi->getStatsManager() &&
          !serverApi->getCurrentPersistenceObject(), "server accessors expose constructor seeds");
    server->m_useContentChecking = true;
    server->m_statsManager = reinterpret_cast<IStatsManager*>(scoreManager);
    check(serverApi->useContentChecking() && serverApi->getStatsManager() == scoreManager,
          "content and stats queries read stored state");
    auto* persist = new PlayerStats;
    HTTPRequest* request = static_cast<HTTPRequest*>(persist);
    check(reinterpret_cast<char*>(request) - reinterpret_cast<char*>(static_cast<IBase*>(persist)) == 4,
          "real persistence secondary base matches both x86 images");
    serverApi->setCurrentPersistenceObject(persist);
    check(serverApi->getCurrentPersistenceObject() == persist, "persistence setter/getter leave reference counts alone");
    check(!serverApi->removePersistenceConnection(request) && !serverApi->getCurrentPersistenceObject(),
          "absent request still clears matching current object before failed lookup");
    check(serverApi->addPersistenceConnection(request) && serverApi->addPersistenceConnection(request) &&
          server->m_httpRequests.size() == 1, "duplicate insert reports success below capacity");
    serverApi->setCurrentPersistenceObject(persist);
    check(!serverApi->removePersistenceConnection(0) && serverApi->getCurrentPersistenceObject() == persist,
          "different missing request preserves current object");
    check(serverApi->removePersistenceConnection(request) && serverApi->removePersistenceConnection(request) &&
          server->m_httpRequests.size() == 1 && server->m_httpRequestsToRemove.size() == 2 &&
          server->m_httpRequestsToRemove.front() == request && server->m_httpRequestsToRemove.back() == request,
          "removal queues duplicate pointers and leaves the live set intact");
    serverApi->setCurrentPersistenceObject(0);
    check(serverApi->addPersistenceConnection(0) && serverApi->removePersistenceConnection(0),
          "null requests retain retail set and queue behavior");
    server->m_httpRequests.clear(); server->m_httpRequestsToRemove.clear();
    std::vector<std::array<unsigned char,24>> requestBytes(257);
    bool accepted = true;
    for (int i = 0; i < 256; ++i)
        accepted = serverApi->addPersistenceConnection(reinterpret_cast<HTTPRequest*>(requestBytes[i].data())) && accepted;
    check(accepted && server->m_httpRequests.size() == 256, "request capacity accepts the 256th entry");
    // Error diagnostics are disabled above for suicide; inspect the recoverable return arm only.
    check(!serverApi->addPersistenceConnection(reinterpret_cast<HTTPRequest*>(requestBytes[256].data())) &&
          !serverApi->addPersistenceConnection(reinterpret_cast<HTTPRequest*>(requestBytes[0].data())) &&
          server->m_httpRequests.size() == 256, "full set rejects both new and duplicate entries before insertion");
    server->m_httpRequests.clear();
    delete persist;
    auto* debugConnection = new DebugConnection;
    server->m_connections[42] = debugConnection; server->m_connections[13] = 0;
    serverApi->debugShowActiveGhosts(-1);
    check(debugConnection->debugCalls == 0, "all-connections ghost debug preserves empty retail loop");
    serverApi->debugShowActiveGhosts(298);
    static_cast<IGame*>(server)->debugShowActiveGhosts(42);
    serverApi->debugShowActiveGhosts(13); serverApi->debugShowActiveGhosts(99);
    check(debugConnection->debugCalls == 2, "both interfaces dispatch ghost debug with byte-key alias and null guards");
    serverApi->debugShowStreamManagerSizes(); serverApi->debugShowGhostPriorities(-1);
    check(debugConnection->debugCalls == 2, "stream-stat sweep and empty priorities body do not dispatch ghost debug");

    world::IPlayer* nullPlayer = 0;
    check(!python::buildArgs(-19, nullPlayer, std::vector<int>()).get(), "uninitialized command arguments return empty PyRef");
    Py_Initialize();
    // [TEMP] A tiny in-memory bf2 module supplies only a player converter. Production initialize,
    // registerHandler, argument conversion and dispatch remain real; no stock-map acceptance.
    PyObject* module = PyImport_AddModule(const_cast<char*>("bf2"));
    PyObject* globals = PyModule_GetDict(module);
    PyDict_SetItemString(globals, "__builtins__", PyEval_GetBuiltins());
    PyObject* setup = PyRun_String("def playerConvFunc(id):\n return ('player', id)\ndef init_module():\n pass\n",
                                  Py_file_input, globals, globals);
    check(setup != 0, "fixture bf2 module created"); Py_XDECREF(setup);
    check(python::pythonHost()->initialize(false), "real PythonHost initialized with fixture converter");
    {
        python::PyRef args = python::buildArgs(-19, nullPlayer, std::vector<int>());
        check(PyTuple_Check(args.get()) && PyTuple_Size(args.get()) == 3 &&
              PyInt_AsLong(PyTuple_GetItem(args.get(),0)) == -19 && PyTuple_GetItem(args.get(),1) == Py_None &&
              PyTuple_Check(PyTuple_GetItem(args.get(),2)) && PyTuple_Size(PyTuple_GetItem(args.get(),2)) == 0,
              "initialized helper preserves negative command, null player and empty nested tuple");
        check(args.get()->ob_refcnt == 1, "argument tuple transfers exactly one owned reference");
    }
    setup = PyRun_String("import host\nseen = []\ndef receive(command, player, args):\n seen.append((command, player, args))\nhost.registerHandler('ClientCommand', receive, 1)\n",
                         Py_file_input, globals, globals);
    check(setup != 0, "real host registers fixture command observer");
    if (!setup) PyErr_Print();
    Py_XDECREF(setup);
    actor->m_playerId = 27; players->local = actor; players->registered = actor;
    players->ids.clear(); api->remoteCommand(-9, -2147483647-1, 2147483647, -7, 0);
    PyObject* seen = PyDict_GetItemString(globals, "seen");
    check(seen && PyList_Size(seen) == 1, "registered local command reaches initialized Python callback");
    if (seen && PyList_Size(seen) == 1)
    {
        PyObject* record = PyList_GetItem(seen,0);
        PyObject* player = PyTuple_GetItem(record,1);
        PyObject* values = PyTuple_GetItem(record,2);
        check(PyInt_AsLong(PyTuple_GetItem(record,0)) == -9 &&
              std::string(PyString_AsString(PyTuple_GetItem(player,0))) == "player" &&
              PyInt_AsLong(PyTuple_GetItem(player,1)) == 27,
              "command callback carries exact command and converted player");
        check(PyTuple_Check(values) && PyTuple_Size(values) == 4 &&
              PyInt_AsLong(PyTuple_GetItem(values,0)) == (-2147483647-1) &&
              PyInt_AsLong(PyTuple_GetItem(values,1)) == 2147483647 &&
              PyInt_AsLong(PyTuple_GetItem(values,2)) == -7 && PyInt_AsLong(PyTuple_GetItem(values,3)) == 0,
              "command callback keeps all four signed arguments including trimmed wire tail");
    }
    check(players->ids == std::vector<int>({27,2147483647,-7}) &&
          static_cast<PythonCommandEvent*>(server->sent)->m_argCount == 3,
          "initialized dispatch retains discarded lookups and separate wire count");
    delete server->sent;
    check(!PyErr_Occurred(), "Python command path leaves no exception");
    std::printf("%d checks, %d failures\n", checks, failures);
    // Real objects intentionally live until process exit; teardown is outside this component fixture.
    return failures ? 1 : 0;
}
