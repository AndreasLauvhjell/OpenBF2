// [TEMP] [CHOSEN, not recovered]
// Offline courier tests with real AIEventCourier, SquadManager, and ClassManager.
// Typed collaborators record forwarding; unused virtuals and unrelated level/admission dependencies abort.
#include "../../BF2/AI/AIInterface/IAIEventCourier.h"
#include "../../BF2/AI/AIDLL/AI/Main/AIEventCourier.h"
#include "../../BF2/AI/AIDLL/AI/Main/AIMain.h"
#include "../../BF2/AI/AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../BF2/AI/AIDLL/AI/AIBot/SquadManager.h"
#include "../../BF2/AI/AIDLL/AI/AIBot/SquadCoordinator.h"
#include "../../BF2/AI/AIDLL/AI/AIBot/BotSquad.h"
#include "../../BF2/AI/AIDLL/AI/Main/IBotManager.h"
#include "../../BF2/AI/AIInterface/BFKnowledge/BFGame.h"
#include "../../BF2/AI/AIInterface/BFKnowledge/AIPlayerFactory.h"
#include "../../BF2/AI/AIDLL/AI/Main/IAIPathfinding.h"
#include "../../BF2/AI/AIDLL/AI/AIStrategicEntity/AIStrategicObjectManager.h"
#include "../../BF2/Core/ClassManager.h"
#include "../../BF2/Core/Debug.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <vector>
#include <string>
#include <windows.h>
#include <crtdbg.h>

static_assert(sizeof(void*) == 4, "MSVC x86 fixture only");
static_assert(sizeof(dice::hfe::ai::IAIEventCourier) == 4, "courier interface owns only its vptr");
static_assert(sizeof(dice::hfe::ai::AIEventCourier) == 4, "courier owns only its vptr");
[[noreturn]] static void unexpected(const char* name)
{
    std::fprintf(stderr, "UNEXPECTED courier collaborator: %s\n", name);
    std::fflush(stderr);
    std::abort();
}
static void require(bool value, const char* description)
{
    if (!value) { std::fprintf(stderr, "FAIL %s\n", description); std::exit(1); }
}
static std::vector<int> received;
static std::string calls;
static bool fatalMode = false;
// Every observed forwarding call is an error in fatal-Debug mode.
static void trace(const std::string& item)
{
    if (fatalMode) { std::printf("CALLED %s\n", item.c_str()); std::fflush(stdout); }
    if (!calls.empty()) calls += ',';
    calls += item;
}
static void expectCalls(const char* wanted)
{
    if (calls != wanted) { std::fprintf(stderr, "Expected %s; got %s\n", wanted, calls.c_str()); std::exit(1); }
    calls.clear();
}
static std::vector<dice::hfe::ai::SquadCoordinator*> coordinators[2];
// These are pointer-identity tokens only. No IAIPlayer or SquadMember object is fabricated/dereferenced.
alignas(void*) static unsigned char playerToken, memberTokenA, memberTokenB;
static dice::hfe::ai::IAIPlayer* expectedPlayer = reinterpret_cast<dice::hfe::ai::IAIPlayer*>(&playerToken);
static dice::hfe::ai::SquadMember* matchA = reinterpret_cast<dice::hfe::ai::SquadMember*>(&memberTokenA);
static dice::hfe::ai::SquadMember* matchB = reinterpret_cast<dice::hfe::ai::SquadMember*>(&memberTokenB);
static dice::hfe::ai::SquadMember* matches[2][3];
static int wantedPlayer;
// Identity tokens and references must reach pathfinding unchanged; none is dereferenced.
alignas(void*) static unsigned char managerToken, objectTokenA, objectTokenB;
static dice::hfe::ai::IAIObject* const objectA = reinterpret_cast<dice::hfe::ai::IAIObject*>(&objectTokenA);
static dice::hfe::ai::IAIObject* const objectB = reinterpret_cast<dice::hfe::ai::IAIObject*>(&objectTokenB);
static const dice::hfe::Vec3* expectedMin = nullptr;
static const dice::hfe::Vec3* expectedMax = nullptr;
static const dice::hfe::Mat4* expectedTransformation = nullptr;
static std::string objectName(const void* object)
{
    return object == &objectTokenA ? "A" : object == &objectTokenB ? "B" : object ? "?" : "null";
}

#include "courier_runtime_mocks.inl"

namespace dice { namespace hfe {
ClassManager* classManager = nullptr;
class SettingsRepostitory;
SettingsRepostitory* settings = nullptr;
namespace ai {
void* CommonSingletonServer::m_iaiGame = nullptr;
void* CommonSingletonServer::m_iaiPlayerFactory = nullptr;
void* CommonSingletonServer::m_iBFSquad = nullptr;
IBotManager* IBotManager::instance = nullptr;
// [TEMP] Only collaborator lifetimes. This gate does not validate concrete AIMain behavior.
IAIMain* IAIMain::m_instance = nullptr;
IAIMain::IAIMain() { m_instance = this; }
IAIMain::~IAIMain() { m_instance = nullptr; }
IAIGame::IAIGame() {}
IAIGame::~IAIGame() {}
IAIPlayerFactory::IAIPlayerFactory() {}
IAIPlayerFactory::~IAIPlayerFactory() {}
// Real native coordinator/container lifetimes with just the fixture's side/index observations.
// No manager or courier body is replaced. Formation/admission behavior is outside this fixture.
SquadCoordinator::SquadCoordinator(int side, int index) : m_side(side), m_squadIndex(index)
{
    require(side >= 0 && side < 2 && index == static_cast<int>(coordinators[side].size()), "coordinator construction order");
    coordinators[side].push_back(this);
}
SquadMember* SquadCoordinator::getSquadMember(int player, int& index)
{
    trace("find:" + std::to_string(m_side) + ":" + std::to_string(m_squadIndex) + ":" + std::to_string(player));
    require(player == wantedPlayer, "lookup ID");
    SquadMember* result = matches[m_side][m_squadIndex];
    if (result) index = 91; // Manager must not treat this member index as a coordinator index.
    return result;
}
void SquadCoordinator::addSquadMember(IAIPlayer* player)
{
    require(player == expectedPlayer, "factory result forwarded unchanged");
    trace("add:" + std::to_string(m_side) + ":" + std::to_string(m_squadIndex));
}
void SquadCoordinator::updateSquadLeader()
{ trace("leader:" + std::to_string(m_side) + ":" + std::to_string(m_squadIndex)); }
// The separate automatic-admission path is not exercised and cannot silently succeed.
int SquadCoordinator::getNumberOfSquadMembers() { unexpected("SquadCoordinator::getNumberOfSquadMembers"); }
int SquadCoordinator::getSquadId() { unexpected("SquadCoordinator::getSquadId"); }
SquadMember* SquadCoordinator::getSquadMember(int) { unexpected("SquadCoordinator::getSquadMember(index)"); }
bool BotSquad::joinSquad(int) { unexpected("BotSquad::joinSquad"); }
// [TEMP]
// Unused coordinator dependencies abort if reached.
void SquadCoordinator::update(double) { unexpected("SquadCoordinator::update"); }
void SquadCoordinator::updateSquadPaths() { unexpected("SquadCoordinator::updateSquadPaths"); }
int SquadCoordinator::getMissionStatus() { unexpected("SquadCoordinator::getMissionStatus"); }
bool SquadCoordinator::isSquadAssignedToArea(int) { unexpected("SquadCoordinator::isSquadAssignedToArea"); }
int SquadCoordinator::getStrategicStrength(StrategicStrengthType) { unexpected("SquadCoordinator::getStrategicStrength"); }
void* CommonSingletonServer::m_iBFCommander = nullptr;
void* CommonSingletonServer::m_iBFTimer = nullptr;
// Static storage only; no corresponding owner is constructed or exercised by this fixture.
IAIPathfindingSetup* IAIPathfindingSetup::instance = nullptr;
// [TEMP]
// Pathfinder fixture lifetime: last construction wins; destruction unconditionally clears the singleton.
IAIPathfinding* IAIPathfinding::instance = nullptr;
IAIPathfinding::IAIPathfinding() { instance = this; }
IAIPathfinding::~IAIPathfinding() { instance = nullptr; }
// [TEMP] The manager singleton is an identity token: the four bridge members observe `this` and
// their argument and touch no field. Never call event_levelLoaded here: no map/area behavior is
// modeled, and postLoadInitAreas still fails loudly.
AIStrategicObjectManager* AIStrategicObjectManager::getInstance()
{ return reinterpret_cast<AIStrategicObjectManager*>(&managerToken); }
void AIStrategicObjectManager::postLoadInitAreas()
{ unexpected("AIStrategicObjectManager::postLoadInitAreas"); }
void AIStrategicObjectManager::createBridgeSegment(IAIObject* object)
{ require(this == getInstance(), "bridge manager singleton"); trace("create:" + objectName(object)); }
void AIStrategicObjectManager::removeBridgeSegment(IAIObject* object)
{ require(this == getInstance(), "bridge manager singleton"); trace("remove:" + objectName(object)); }
void AIStrategicObjectManager::bridgeSegmentDestroyed(IAIObject* object, int value)
{
    require(this == getInstance(), "bridge manager singleton");
    trace("destroyed:" + objectName(object) + ":" + std::to_string(value));
}
void AIStrategicObjectManager::bridgeSegmentRepaired(IAIObject* object)
{ require(this == getInstance(), "bridge manager singleton"); trace("repaired:" + objectName(object)); }
} } }

void courierFromEngine(dice::hfe::ai::IAIEventCourier*, int);
void courierFromDll(dice::hfe::ai::IAIEventCourier*, int);
void leaderFromEngine(dice::hfe::ai::IAIEventCourier*, int, int);
void leaderFromDll(dice::hfe::ai::IAIEventCourier*, int, int);
void joinedFromEngine(dice::hfe::ai::IAIEventCourier*, int, int, int);
void joinedFromDll(dice::hfe::ai::IAIEventCourier*, int, int, int);
void mapPartFromEngine(dice::hfe::ai::IAIEventCourier*, int&, bool, const dice::hfe::Vec3&,
                       const dice::hfe::Vec3&, const dice::hfe::Mat4&);
void mapPartFromDll(dice::hfe::ai::IAIEventCourier*, int&, bool, const dice::hfe::Vec3&,
                    const dice::hfe::Vec3&, const dice::hfe::Mat4&);
void mapPartDestroyedFromEngine(dice::hfe::ai::IAIEventCourier*, int, dice::hfe::ai::IAIObject*);
void mapPartDestroyedFromDll(dice::hfe::ai::IAIEventCourier*, int, dice::hfe::ai::IAIObject*);
void mapPartRestoredFromEngine(dice::hfe::ai::IAIEventCourier*, int);
void mapPartRestoredFromDll(dice::hfe::ai::IAIEventCourier*, int);
void mapPartOriginalFromEngine(dice::hfe::ai::IAIEventCourier*, int, dice::hfe::ai::IAIObject*);
void mapPartOriginalFromDll(dice::hfe::ai::IAIEventCourier*, int, dice::hfe::ai::IAIObject*);
void bridgeEventFromEngine(dice::hfe::ai::IAIEventCourier*, int, dice::hfe::ai::IAIObject*, int);
void bridgeEventFromDll(dice::hfe::ai::IAIEventCourier*, int, dice::hfe::ai::IAIObject*, int);

// Print the Error text, then return: ~Debug then aborts, as after any shipped callback.
static void errorCallback(dice::hfe::DebugType, const std::string&, const std::string&, int line,
                          const std::string&, const std::string& text, const std::string& fullFileName,
                          void*)
{
    std::printf("DEBUG-ERROR %s(%d): %s\n", fullFileName.c_str(), line, text.c_str());
    std::fflush(stdout);
}

// Negative map-part IDs must abort before forwarding. called or reached-after indicates failure.
static int fatal(const char* mode)
{
    using namespace dice::hfe;
    using namespace dice::hfe::ai;
    ClassManager registry;
    classManager = &registry;
    AIEventCourier courier;
    ObservedPathfinding pathfinding;
    setDebugCallback(DebugTypeError, &errorCallback, 0);
    fatalMode = true;
    if (std::strcmp(mode, "fatal-destroyed") == 0)
        mapPartDestroyedFromEngine(&courier, -1, objectA);
    else if (std::strcmp(mode, "fatal-original") == 0)
        mapPartOriginalFromDll(&courier, INT_MIN, objectB);
    else
        unexpected("unknown fatal mode");
    std::printf("REACHED-AFTER %s\n", mode);
    std::fflush(stdout);
    return 1;
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT, 0);
    if (argc == 2)
        return fatal(argv[1]);
    using namespace dice::hfe;
    using namespace dice::hfe::ai;
    ClassManager registry;
    classManager = &registry;
    {
        AIEventCourier courier;
        require(registry.getSimpleSingleton("IAIEventCourier") == &courier, "real courier registration");
        require(IAIMain::m_instance == nullptr, "initially no main");
        const int ids[] = {0, -1, INT_MIN, INT_MAX};
        for (int id : ids)
        {
            courierFromEngine(&courier, id);
            courierFromDll(&courier, id);
        }
        require(received.empty(), "null main does not forward");
        {
            ObservedMain receiver;
            require(IAIMain::m_instance == &receiver, "observer publishes receiver");
            for (int id : ids)
            {
                courierFromEngine(&courier, id);
                courierFromDll(&courier, id);
            }
            require(received.size() == 8, "exactly one forward per invocation");
            for (unsigned i = 0; i < 8; ++i)
                require(received[i] == ids[i / 2], "ID forwarded unchanged in both include orders");
        }
        require(IAIMain::m_instance == nullptr, "observer clears receiver");
        courierFromEngine(&courier, 17);
        require(received.size() == 8, "no forward after receiver destruction");
        require(SquadManager::m_instance == nullptr, "initially no squad manager");
        leaderFromEngine(&courier, INT_MIN, INT_MAX);
        leaderFromDll(&courier, INT_MAX, INT_MIN);
        joinedFromEngine(&courier, 101, 9, INT_MIN);
        joinedFromDll(&courier, 102, -3, INT_MAX);
        expectCalls(""); // The null singleton guard must precede all side/squad accesses.
        ObservedGame game;
        ObservedFactory factory;
        CommonSingletonServer::m_iaiGame = &game;
        CommonSingletonServer::m_iaiPlayerFactory = &factory;
        {
            SquadManager manager;
            require(SquadManager::m_instance == &manager, "real manager singleton");
            require(coordinators[0].size() == 3 && coordinators[1].size() == 3, "both native coordinator vectors");
            expectCalls("capacity");
            leaderFromEngine(&courier, 1, 3);
            leaderFromDll(&courier, 2, 2);
            expectCalls("leader:0:2,leader:1:1");
            joinedFromEngine(&courier, 5, 0, INT_MIN);
            joinedFromDll(&courier, 6, 0, INT_MAX);
            expectCalls(""); // Squad zero must return before even selecting a side.
            wantedPlayer = 71;
            matches[0][1] = matchA;
            matches[0][2] = matchB;
            require(manager.getSquadMember(71, 1) == matchA, "first match wins");
            expectCalls("find:0:0:71,find:0:1:71");
            joinedFromEngine(&courier, 71, 3, 1);
            expectCalls("find:0:0:71,find:0:1:71"); // Duplicate: no factory/add/leader call.
            require(manager.getSquadMember(71, 2) == nullptr, "other-side match ignored");
            expectCalls("find:1:0:71,find:1:1:71,find:1:2:71");
            joinedFromDll(&courier, 71, 2, 2);
            expectCalls("find:1:0:71,find:1:1:71,find:1:2:71,factory:71,add:1:1,leader:1:1");
            matches[1][0] = matchB;
            joinedFromDll(&courier, 71, 3, 2);
            expectCalls("find:1:0:71");
            matches[0][1] = matches[0][2] = nullptr;
            wantedPlayer = -9;
            joinedFromEngine(&courier, -9, 3, 1);
            expectCalls("find:0:0:-9,find:0:1:-9,find:0:2:-9,factory:-9,add:0:2,leader:0:2");
            expectedPlayer = nullptr;
            wantedPlayer = 88;
            joinedFromDll(&courier, 88, 1, 1);
            expectCalls("find:0:0:88,find:0:1:88,find:0:2:88,factory:88,add:0:0,leader:0:0");
            // Manager teardown is outside this gate; the fixture owns its collaborator allocations.
            SquadManager::m_instance = nullptr;
            for (auto& side : coordinators) { for (auto* coordinator : side) delete coordinator; side.clear(); }
        }
        CommonSingletonServer::m_iaiGame = nullptr;
        CommonSingletonServer::m_iaiPlayerFactory = nullptr;

        // Checks map-part and bridge events through both header include orders.
        {
            ObservedPathfinding pathfinding;
            require(IAIPathfinding::instance == &pathfinding, "observer publishes the pathfinder");
            static Vec3 minCorner;
            static Vec3 maxCorner;
            static Mat4 transformation;
            expectedMin = &minCorner;
            expectedMax = &maxCorner;
            expectedTransformation = &transformation;
            int mapPartId = -1;
            mapPartFromEngine(&courier, mapPartId, true, minCorner, maxCorner, transformation);
            require(mapPartId == 4242, "the id comes back through the caller's own int&");
            mapPartId = 7;
            mapPartFromDll(&courier, mapPartId, false, minCorner, maxCorner, transformation);
            require(mapPartId == 4242, "the id comes back through the caller's own int& (DLL order)");
            expectCalls("mapPart:-1:walkable,mapPart:7:blocked");
            // Zero is a valid id; the bridge record goes only when an object is given.
            mapPartDestroyedFromEngine(&courier, 0, objectA);
            mapPartDestroyedFromDll(&courier, INT_MAX, nullptr);
            expectCalls("destroy:0,remove:A,destroy:2147483647");
            // Restored has NO negative-id check in either image - the id goes straight through.
            mapPartRestoredFromEngine(&courier, 5);
            mapPartRestoredFromDll(&courier, -5);
            expectCalls("restore:5,restore:-5");
            mapPartOriginalFromEngine(&courier, 3, objectB);
            mapPartOriginalFromDll(&courier, 0, nullptr);
            expectCalls("original:3,remove:B,original:0");
            bridgeEventFromEngine(&courier, 0, objectA, 0);
            bridgeEventFromDll(&courier, 1, objectA, -77);
            bridgeEventFromEngine(&courier, 2, objectB, 0);
            bridgeEventFromDll(&courier, 3, nullptr, 0);   // no null test: the manager gets the null
            expectCalls("create:A,destroyed:A:-77,repaired:B,remove:null");
        }
        require(IAIPathfinding::instance == nullptr, "observer clears the pathfinder");
    }
    require(registry.getSimpleSingleton("IAIEventCourier") == nullptr, "real courier unregisters");
    classManager = nullptr;
    std::puts("PASS courier/manager: 9 null-main calls, 8 unchanged-ID forwards, 4 null-manager calls, 2 leader routes, 2 zero-squad calls, first-match/side-local lookup, 2 duplicate guards, 3 ordered insertions including null player; 14 map-part/bridge forwards in both include orders (id through the caller's int&, bridge removal only with an object, no id check on restore, null bridge object forwarded); strict x86");
    return 0;
}
