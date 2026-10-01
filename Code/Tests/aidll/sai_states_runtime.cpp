// [TEMP] Exact updateStates and four small accessors with chosen native lifetimes.
// Game/timer/object virtuals are typed observers. Real native containers remain real;
// no complete SAI, strategic object/area, manager construction or engine tick is claimed.
#include "sai_states_runtime.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <windows.h>
#include <crtdbg.h>

using namespace dice::hfe;
using namespace dice::hfe::ai;
static_assert(sizeof(void*) == 4, "MSVC x86 fixture only");
static_assert(sizeof(StrategyContainer) == 20, "paired strategy record size");
static_assert(offsetof(StrategyContainer, m_startTime) == 8, "paired strategy timestamp offset");
[[noreturn]] static void unexpected(const char* message)
{
    std::fprintf(stderr, "UNEXPECTED %s\n", message); std::fflush(stderr); std::abort();
}
static void require(bool condition, const char* message) { if (!condition) unexpected(message); }
static std::string mode, calls;
static void trace(const std::string& event) { if (!calls.empty()) calls += ','; calls += event; }
static float storage[2][44], expectedStorage[2][44];
static float* outputA = storage[0];
static float* outputB = storage[1];
static float* expectedA = expectedStorage[0];
static float* expectedB = expectedStorage[1];
static int* saiSide;
static bool activeStrategy;
static double timeValue = 10.25;
static int ticketCalls, controlCalls, timeCalls;
static IAIGame* otherGame;
static IBFTimer* otherTimer;
static std::vector<AIStrategicObject*>* roster;
static AIStrategicObject* objects[8];
static bool mutateGame, appendObject, replaceAfterType, replaceAfterFlag;
struct ObjectConfig
{
    int type = 0;
    bool border = false;
    bool neighbourObjects[2] = {};
    bool flags[21] = {};
    int flagCalls[21] = {};
};
static ObjectConfig configs[8];
static const int flagOrder[] = {1,3,4,2,5,15,12,15,12,17,18,19,20};
static const int flagIndex[] = {22,23,24,25,26,27,28,27,28,29,30,31,32};

static int observeTickets(int identity, int side)
{
    require(identity == 0 && ticketCalls < 2, "captured game serves both ticket calls");
    const int call = ticketCalls++;
    trace("tickets" + std::to_string(side));
    require(side == (call == 0 ? 1 : 2), "enemy side captured before callback");
    if (call == 1) require(outputA[13] == -123.0f, "first ticket result stored before second callback");
    if (call == 0 && mutateGame)
    {
        *saiSide = 2;
        CommonSingletonServer::m_iaiGame = otherGame;
    }
    return call == 0 ? -123 : 234;
}
static int observeControlPoints(int identity, int side)
{
    require(identity == 0 && ticketCalls == 2 && controlCalls < 2, "same game after ticket calls");
    const int call = controlCalls++;
    trace("cps" + std::to_string(side));
    require(side == (call == 0 ? *saiSide : 2), "own control-point side reloaded; enemy captured");
    require(outputB[13] == 234.0f, "enemy tickets stored before control-point call");
    if (call == 1)
    {
        require(outputA[14] == 5.0f, "own control-point result stored before enemy call");
        if (mutateGame) CommonSingletonServer::m_iBFTimer = otherTimer;
    }
    return call == 0 ? 5 : 7;
}
static double observeTime(int identity)
{
    require(identity == (mutateGame ? 1 : 0), "timer loaded after all game callbacks");
    require(controlCalls == 2 && timeCalls++ == 0 && outputB[14] == 7.0f, "one timer after stored counters");
    trace("time" + std::to_string(identity));
    return timeValue;
}
static int observeType(int id)
{
    trace("type" + std::to_string(id));
    if (appendObject && id == 0) { roster->push_back(objects[1]); appendObject = false; }
    if (replaceAfterType && id == 0) { (*roster)[0] = objects[1]; replaceAfterType = false; }
    return configs[id].type;
}
static bool observeNeighbour(int id, StatusMap::Status status, int side)
{
    trace("neighbour" + std::to_string(id) + ":" + std::to_string(static_cast<int>(status)));
    require(side == *saiSide, "neighbour call uses current side");
    const StatusMap::Status own = objects[id]->getStatus(side);
    require(static_cast<int>(status) == (own == StatusMap::Owned ? 1 : 0), "opposite status passed to border query");
    return configs[id].border;
}
static bool observeNeighbourObject(int id, StatusMap::Status status, int side)
{
    trace("near" + std::to_string(id) + ":" + std::to_string(static_cast<int>(status)));
    require(side == *saiSide, "neutral neighbour-object side");
    require(status == StatusMap::Owned || status == StatusMap::Hostile, "neutral query status");
    return configs[id].neighbourObjects[static_cast<int>(status)];
}
static bool observeFlag(int id, AIStrategicObject::StrategicObjectType flag)
{
    const int value = static_cast<int>(flag);
    trace("flag" + std::to_string(id) + ":" + std::to_string(value));
    require(value >= 1 && value <= 20, "flag range");
    const int call = configs[id].flagCalls[value]++;
    if (replaceAfterFlag && id == 1 && value == 1)
    { (*roster)[0] = objects[2]; replaceAfterFlag = false; }
    if (mode == "repeated_flags" && value == 15) return call == 0;
    if (mode == "repeated_flags" && value == 12) return call == 1;
    return configs[id].flags[value];
}

#include "sai_states_mocks.inl"

namespace dice { namespace hfe { namespace ai {
// Chosen lifetimes use only real declared members and native containers.
ISAI::~ISAI() {}
SAI::SAI(int side, int, int, Vec2, Vec2) : m_side(side)
{
    saiSide = &m_side;
    m_list1BC.assign(2, 71); m_list1B8.assign(3, 72);
    m_strategies.resize(1);
    m_strategies[0].m_startTime = 16777216.0f;
    m_activeStrategy = m_strategies.data() + (activeStrategy ? 0 : 1);
}
SAI::~SAI() {}
IAIGame::IAIGame() {}
IAIGame::~IAIGame() {}
IBFTimer::IBFTimer() {}
IBFTimer::~IBFTimer() {}
void* CommonSingletonServer::m_iaiGame = nullptr;
void* CommonSingletonServer::m_iBFTimer = nullptr;
AIStrategicObject::AIStrategicObject(float, float)
{
    for (int side = 0; side < 4; ++side) m_status[side] = StatusMap::Unknown;
    m_attack[0] = m_attack[1] = false;
    m_index = -1;
}
AIStrategicObject::~AIStrategicObject() {}
AIStrategicObjectManager* AIStrategicObjectManager::m_instance = nullptr;
AIStrategicObjectManager::AIStrategicObjectManager()
{
    m_instance = this; roster = &m_strategicObjects; roster->reserve(10);
}
AIStrategicObjectManager::~AIStrategicObjectManager() { if (m_instance == this) m_instance = nullptr; }
} } }

static void prefixExpectation()
{
    expectedA[13] = -123.0f; expectedB[13] = 234.0f;
    expectedA[14] = 5.0f; expectedB[14] = 7.0f;
    expectedA[15] = expectedB[15] = static_cast<float>(timeValue);
    expectedA[16] = activeStrategy ? 1.0f : 0.0f; expectedB[16] = 0.0f;
    expectedA[17] = 2.0f; expectedA[18] = 3.0f;
    const int cleared[] = {19,20,21,39,38,41,40,42};
    for (int index : cleared) { expectedA[index] = 0.0f; expectedB[index] = 0.0f; }
    expectedB[17] = 0.0f; expectedB[18] = 0.0f;
}
static void addFlags(std::string& expected, int id, float* destination, bool border,
                     bool count = true, int first = 0)
{
    for (int i = first; i < 13; ++i)
    {
        expected += ",flag" + std::to_string(id) + ":" + std::to_string(flagOrder[i]);
        if (count)
        {
            destination[flagIndex[i]] += 1.0f;
            if (border && i < 5) destination[33 + i] += 1.0f;
        }
    }
}
static void setAllFlags(int id) { for (int flag : flagOrder) configs[id].flags[flag] = true; }
static void addObject(int id, StatusMap::Status status)
{
    objects[id]->m_status[0] = objects[id]->m_status[1] = status;
    roster->push_back(objects[id]);
}
static void checkResult(const std::string& expected)
{
    if (calls != expected)
    {
        std::fprintf(stderr, "FAIL trace\nexpected: %s\nactual:   %s\n", expected.c_str(), calls.c_str());
        std::exit(1);
    }
    for (int array = 0; array < 2; ++array) for (int index = 0; index < 44; ++index)
        if (storage[array][index] != expectedStorage[array][index])
        {
            std::fprintf(stderr, "FAIL output%d[%d]: %.9g != %.9g\n", array, index,
                         storage[array][index], expectedStorage[array][index]); std::exit(1);
        }
    require(ticketCalls == 2 && controlCalls == 2 && timeCalls == 1, "exact game/timer call counts");
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    require(argc == 2, "one fixture mode"); mode = argv[1];
    for (int array = 0; array < 2; ++array) for (int i = 0; i < 44; ++i)
        storage[array][i] = expectedStorage[array][i] = static_cast<float>(1000 * (array + 1) + i);
    if (mode == "alias_outputs") { outputB = outputA; expectedB = expectedA; }
    if (mode == "active_time") { activeStrategy = true; timeValue = 16777217.0; }
    mutateGame = mode == "cached_game";
    prefixExpectation();
    std::string expected = mutateGame ? "tickets1,tickets2,cps2,cps2,time1" : "tickets1,tickets2,cps1,cps2,time0";
    ObservedGame game, replacementGame; replacementGame.identity = 1; otherGame = &replacementGame;
    ObservedTimer timer, replacementTimer; replacementTimer.identity = 1; otherTimer = &replacementTimer;
    CommonSingletonServer::m_iaiGame = &game; CommonSingletonServer::m_iBFTimer = &timer;
    AIStrategicObjectManager manager;
    ObservedObject nativeObjects[8];
    for (int i = 0; i < 8; ++i) { nativeObjects[i].identity = i; objects[i] = &nativeObjects[i]; objects[i]->m_index = i + 100; }
    if (mode == "empty" || mode == "active_time" || mode == "cached_game") {}
    else if (mode == "owned_hostile")
    {
        for (int i = 0; i < 4; ++i)
        {
            const bool owned = i < 2, border = (i % 2) == 0;
            addObject(i, owned ? StatusMap::Owned : StatusMap::Hostile); configs[i].border = border;
            setAllFlags(i);
            expected += ",type" + std::to_string(i) + ",neighbour" + std::to_string(i) + (owned ? ":1" : ":0");
            addFlags(expected, i, owned ? expectedA : expectedB, border);
        }
        expectedA[19] = expectedA[21] = expectedB[19] = expectedB[21] = 2.0f;
        expectedA[39] = expectedA[40] = expectedA[41] = expectedA[42] = 1.0f;
        expectedB[39] = expectedB[40] = expectedB[41] = expectedB[42] = 1.0f;
    }
    else if (mode == "neutral")
    {
        addObject(0, StatusMap::Neutral); addObject(1, StatusMap::Neutral);
        configs[0].neighbourObjects[0] = true; configs[1].neighbourObjects[1] = true;
        expected += ",type0,near0:0,near0:1,type1,near1:0,near1:1";
        expectedA[20] = expectedB[20] = 2.0f; expectedA[38] = expectedB[38] = 1.0f;
    }
    else if (mode == "retained_selection")
    {
        configs[0].type = configs[2].type = configs[4].type = 7;
        addObject(0, StatusMap::Unknown); addObject(1, StatusMap::Owned);
        addObject(2, StatusMap::Hostile); addObject(3, StatusMap::Unknown); addObject(4, StatusMap::Owned);
        configs[1].border = true; setAllFlags(1); setAllFlags(2);
        expected += ",type0,type1,neighbour1:1"; addFlags(expected, 1, expectedA, true);
        expected += ",type2"; addFlags(expected, 2, expectedA, true); expected += ",type3,type4";
        expectedA[19] = expectedB[21] = expectedA[39] = expectedB[41] = 1.0f;
    }
    else if (mode == "assignment_sets")
    {
        for (int i = 0; i < 5; ++i)
        {
            addObject(i, StatusMap::Unknown); expected += ",type" + std::to_string(i);
            if (i < 4) objects[i]->m_set68[0].insert(90);
        }
        objects[0]->m_attack[0] = objects[1]->m_attack[0] = objects[2]->m_attack[0] = true;
        objects[0]->m_index = objects[1]->m_index = objects[3]->m_index = 5;
        objects[2]->m_index = 7;
        expectedB[17] = 2.0f; expectedB[18] = 1.0f;
    }
    else if (mode == "repeated_flags")
    {
        addObject(0, StatusMap::Owned);
        expected += ",type0,neighbour0:1"; addFlags(expected, 0, expectedA, false, false);
        expectedA[27] += 1.0f; expectedA[28] += 1.0f;
        expectedA[19] = expectedB[21] = expectedA[40] = expectedB[42] = 1.0f;
    }
    else if (mode == "alias_outputs")
    {
        addObject(0, StatusMap::Owned); configs[0].border = true; setAllFlags(0);
        objects[0]->m_set68[0].insert(1); objects[0]->m_attack[0] = true;
        expected += ",type0,neighbour0:1"; addFlags(expected, 0, expectedA, true);
        expectedA[19] = expectedA[21] = expectedA[39] = expectedA[41] = 1.0f;
        expectedA[17] = 1.0f; expectedA[18] = 0.0f;
    }
    else if (mode == "live_end")
    {
        addObject(0, StatusMap::Unknown); objects[1]->m_status[0] = StatusMap::Owned; appendObject = true;
        expected += ",type0,type1,neighbour1:1"; addFlags(expected, 1, expectedA, false, false);
        expectedA[19] = expectedB[21] = expectedA[40] = expectedB[42] = 1.0f;
    }
    else if (mode == "entry_reload")
    {
        addObject(0, StatusMap::Owned); objects[1]->m_status[0] = StatusMap::Hostile;
        objects[1]->m_set68[0].insert(2); objects[1]->m_attack[0] = true;
        replaceAfterType = replaceAfterFlag = true; setAllFlags(2);
        expected += ",type0,neighbour1:0,flag1:1"; addFlags(expected, 2, expectedB, false, true, 1);
        expectedA[21] = expectedB[19] = expectedB[40] = expectedA[42] = 1.0f;
        expectedB[17] = 1.0f;
    }
    else unexpected("unknown fixture mode");

    SAI subject(1, 0, 0, Vec2(), Vec2());
    subject.updateStates(outputA, outputB);
    checkResult(expected);
    std::printf("PASS %s\n", mode.c_str());
    return 0;
}
