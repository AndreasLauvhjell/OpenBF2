// [TEMP] Exact strategic-object caller and nine other bounded actual definitions.
// SAI/object/registry/main lifetimes and virtual receivers are chosen native fixtures.
// No complete AI construction, engine tick, or retail STL layout is claimed.
#include "sai_objects_runtime.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <windows.h>
#include <crtdbg.h>

using namespace dice::hfe;
using namespace dice::hfe::ai;
static_assert(sizeof(void*) == 4, "strict x86 fixture");
[[noreturn]] static void unexpected(const char* message)
{
    std::fprintf(stderr, "UNEXPECTED %s\n", message); std::fflush(stderr); std::abort();
}
static void require(bool condition, const char* message) { if (!condition) unexpected(message); }
static std::string mode, calls;
static void trace(const std::string& event) { if (!calls.empty()) calls += ','; calls += event; }
static SAI* tested;
static int* currentSide;
static bool* restartFlag;
static bool* changedFlag;
static bool* commanderFlag;
static std::vector<AIStrategicObject*>::const_iterator* currentObject;
static EnemyStatistics** firstDestination;
static EnemyStatistics** secondDestination;
static EnemyStatistics* destinations[3];
static AIStrategicObject* objects[4];
static std::vector<AIStrategicObject*>* rosters[2];
static AIStrategicObjectManager* otherManager;
static IAIMain* otherMain;
static int managerCount, updateCalls, dirtyCalls, mainCalls;
static bool changedByObject[4];
static int expectedUpdateSide = 1;

struct ExpectedStatistics { float strength[6] = {}; float count[6] = {}; int samples = 0; };
static ExpectedStatistics expected[3], beforeUpdate[3];
static void compareStatistics(const ExpectedStatistics* values)
{
    for (int d = 0; d < 3; ++d)
    {
        require(destinations[d]->m_samples == values[d].samples, "statistics sample total");
        for (int i = 0; i < 6; ++i)
        {
            require(destinations[d]->m_strength[i] == values[d].strength[i], "all six strength buckets");
            require(destinations[d]->m_count[i] == values[d].count[i], "all six count buckets");
        }
    }
}
static void sentinel(int d)
{
    destinations[d]->m_samples = expected[d].samples = 100 + d;
    for (int i = 0; i < 6; ++i)
    {
        destinations[d]->m_strength[i] = expected[d].strength[i] = 500.0f + d * 10.0f + i;
        destinations[d]->m_count[i] = expected[d].count[i] = -100.0f - d * 10.0f - i;
    }
}
static void observeUpdate(int id, int side, ISAI* sai, bool& changed)
{
    trace("u" + std::to_string(id) + ":" + std::to_string(side));
    require(sai == tested && side == expectedUpdateSide, "update receiver and current side");
    require(!changed && !*restartFlag, "fresh local changed bool and restart cleared before update");
    compareStatistics(beforeUpdate);
    ++updateCalls;
    if (mode == "entry_replace") (*rosters[0])[0] = objects[1];
    if (mode == "side_change") *currentSide = 2;
    if (mode == "cursor_move" && id == 0) *currentObject = rosters[0]->cbegin() + 1;
    if (mode == "live_end" && id == 0) rosters[0]->push_back(objects[1]);
    if (mode == "manager_swap" && id == 0) AIStrategicObjectManager::m_instance = otherManager;
    if (mode == "destination_swap") *firstDestination = destinations[2];
    if (mode == "callback_flag") *changedFlag = true;
    if (mode == "cleared_flag") *changedFlag = false;
    changed = changedByObject[id];
}
static void observeDirty(int id, int side)
{
    trace("d" + std::to_string(id) + ":" + std::to_string(side));
    require(*changedFlag && !*restartFlag && side == *currentSide,
            "dirty callbacks precede completed-pass flag store and use current side");
    compareStatistics(expected);
    ++dirtyCalls;
    if (mode == "dirty_replace_side" && id == 0)
    {
        (*rosters[0])[0] = objects[2];
        *currentSide = 2;
    }
    if (mode == "dirty_live_end" && id == 0) rosters[0]->push_back(objects[1]);
}
static SAI* observeMain(int id, int side)
{
    trace("m" + std::to_string(id) + ":" + std::to_string(side));
    require(side == *currentSide && !*restartFlag, "release forwards side before pass completion");
    ++mainCalls;
    if (mode == "release_main_swap" && id == 0) IAIMain::m_instance = otherMain;
    if (mode == "release_late_index" && mainCalls == 2) objects[0]->m_index = 777;
    return tested;
}

#include "sai_objects_mocks.inl"

namespace dice { namespace hfe { namespace ai {
// Only these declared lifetimes are chosen; extracted tested bodies remain unshadowed.
ISAI::~ISAI() {}
SAI::SAI(int side, int, int, Vec2, Vec2) : m_side(side), m_flag1CF(false),
    m_flag1D0(true), m_flag1D1(false), m_strategicObjectIterator()
{
    currentSide = &m_side; restartFlag = &m_flag1D0; changedFlag = &m_flag1CF;
    commanderFlag = &m_flag1D1;
    currentObject = &m_strategicObjectIterator;
    firstDestination = &m_enemyStats; secondDestination = &m_friendlyStats;
    m_enemyStats = destinations[0] = new EnemyStatistics(6);
    m_friendlyStats = destinations[1] = new EnemyStatistics(6);
}
SAI::~SAI() { delete destinations[0]; delete destinations[1]; }
int SAI::releaseSquadsFromObject(int index, int side)
{
    require(this == tested && side == *currentSide, "release current side and receiving SAI");
    trace("release" + std::to_string(index) + ":" + std::to_string(side));
    return -17; // [TEMP] Deliberately nonzero; actual caller must discard it.
}
IAIMain* IAIMain::m_instance = nullptr;
IAIMain::IAIMain() {}
IAIMain::~IAIMain() {}
AIStrategicObject::AIStrategicObject(float, float)
{
    for (int i = 0; i < 4; ++i) { m_statistics[i] = new EnemyStatistics(6); m_status[i] = StatusMap::Unknown; }
    m_index = -1;
}
AIStrategicObject::~AIStrategicObject() { for (int i = 0; i < 4; ++i) delete m_statistics[i]; }
AIStrategicObjectManager* AIStrategicObjectManager::m_instance = nullptr;
AIStrategicObjectManager::AIStrategicObjectManager()
{
    require(managerCount < 2, "two native registries only");
    rosters[managerCount++] = &m_strategicObjects; m_strategicObjects.reserve(8);
}
AIStrategicObjectManager::~AIStrategicObjectManager() { if (m_instance == this) m_instance = nullptr; }
} } }

static std::string releaseTrace(int object, int side, int firstMain = 0, int secondMain = 0)
{
    const std::string s = std::to_string(side);
    std::string result = "d" + std::to_string(object) + ":" + s + ",m" + std::to_string(firstMain) + ":" + s;
    if (mode != "commander") result += ",m" + std::to_string(secondMain) + ":" + s + ",release" + std::to_string(100 + object) + ":" + s;
    return result;
}
static void invoke(bool result, int cursor, const std::string& exactTrace,
                   int selectedObject, int selectedSide, int destination,
                   bool changed, int manager = 0)
{
    if (*restartFlag)
    {
        // Initial fixture destinations are distinct; later destination swaps are only single calls.
        expected[0] = ExpectedStatistics(); expected[1] = ExpectedStatistics();
    }
    for (int i = 0; i < 3; ++i) beforeUpdate[i] = expected[i];
    EnemyStatistics* contribution = objects[selectedObject]->m_statistics[selectedSide - 1];
    for (int i = 0; i < 6; ++i)
    {
        expected[destination].strength[i] += contribution->m_strength[i];
        expected[destination].count[i] += contribution->m_count[i];
    }
    expected[destination].samples += contribution->m_samples;
    calls.clear();
    const int priorCalls = updateCalls;
    require(tested->updateStrategicObjects() == result, "one-object completion return");
    require(updateCalls == priorCalls + 1, "exactly one update per invocation");
    if (calls != exactTrace) { std::fprintf(stderr, "TRACE got=%s expected=%s\n", calls.c_str(), exactTrace.c_str()); unexpected("full callback order"); }
    compareStatistics(expected);
    require(*restartFlag == result && *changedFlag == changed, "restart and sticky flags after return");
    require(*currentObject == rosters[manager]->cbegin() + cursor, "retained native cursor after update");
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE); _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    require(argc == 2, "one mode"); mode = argv[1];
    ObservedObject nativeObjects[4];
    for (int id = 0; id < 4; ++id)
    {
        objects[id] = &nativeObjects[id]; nativeObjects[id].identity = id;
        nativeObjects[id].m_index = 100 + id;
        for (int side = 0; side < 4; ++side)
        {
            nativeObjects[id].m_status[side] = StatusMap::Owned;
            EnemyStatistics* stats = nativeObjects[id].m_statistics[side];
            stats->m_samples = 3 + id * 4 + side;
            for (int i = 0; i < 6; ++i)
            {
                stats->m_strength[i] = 1.25f + id * 16.0f + side * 4.0f + i;
                stats->m_count[i] = -0.5f - id * 16.0f - side * 4.0f - i;
            }
        }
    }
    AIStrategicObjectManager manager, replacementManager;
    otherManager = &replacementManager; AIStrategicObjectManager::m_instance = &manager;
    rosters[0]->push_back(objects[0]); rosters[1]->push_back(objects[3]);
    ObservedMain main, replacementMain;
    replacementMain.identity = 1; otherMain = &replacementMain; IAIMain::m_instance = &main;
    SAI sai(1, 0, 0, Vec2{}, Vec2{}); tested = &sai;
    EnemyStatistics spare(6); destinations[2] = &spare;
    for (int d = 0; d < 3; ++d) sentinel(d);
    *commanderFlag = mode == "commander" || mode == "release_refusal";

    if (mode == "single_owned") invoke(true, 1, "u0:1", 0, 1, 0, false);
    else if (mode == "single_hostile" || mode == "single_neutral")
    {
        objects[0]->m_status[0] = mode == "single_hostile" ? StatusMap::Hostile : StatusMap::Neutral;
        invoke(true, 1, "u0:1", 0, 1, 1, false);
    }
    else if (mode == "side_two")
    {
        *currentSide = expectedUpdateSide = 2; objects[0]->m_status[1] = StatusMap::Unknown;
        invoke(true, 1, "u0:2", 0, 2, 1, false);
    }
    else if (mode == "resume" || mode == "sticky_change")
    {
        rosters[0]->push_back(objects[1]); rosters[0]->push_back(objects[2]);
        objects[1]->m_status[0] = StatusMap::Hostile;
        changedByObject[0] = mode == "sticky_change";
        const bool changed = changedByObject[0];
        invoke(false, 1, "u0:1", 0, 1, 0, changed);
        invoke(false, 2, "u1:1", 1, 1, 1, changed);
        std::string trace = "u2:1";
        if (changed) for (int id = 0; id < 3; ++id) trace += "," + releaseTrace(id, 1);
        invoke(true, 3, trace, 2, 1, 0, changed);
    }
    else if (mode == "restart")
    {
        *changedFlag = true;
        invoke(true, 1, "u0:1", 0, 1, 0, false);
        *changedFlag = true; for (int d = 0; d < 2; ++d) sentinel(d);
        invoke(true, 1, "u0:1", 0, 1, 0, false);
    }
    else if (mode == "changed_output" || mode == "callback_flag" || mode == "commander" || mode == "release_main_swap")
    {
        changedByObject[0] = mode != "callback_flag";
        invoke(true, 1, "u0:1," + releaseTrace(0, 1, 0, mode == "release_main_swap" ? 1 : 0), 0, 1, 0, true);
    }
    else if (mode == "cleared_flag")
    {
        *restartFlag = false; *changedFlag = true; *currentObject = rosters[0]->cbegin();
        invoke(true, 1, "u0:1", 0, 1, 0, false);
    }
    else if (mode == "entry_replace") invoke(true, 1, "u0:1", 1, 1, 0, false);
    else if (mode == "side_change")
    {
        objects[0]->m_status[1] = StatusMap::Neutral;
        invoke(true, 1, "u0:1", 0, 2, 1, false);
    }
    else if (mode == "cursor_move")
    {
        rosters[0]->push_back(objects[1]); rosters[0]->push_back(objects[2]);
        invoke(false, 2, "u0:1", 1, 1, 0, false);
        invoke(true, 3, "u2:1", 2, 1, 0, false);
    }
    else if (mode == "live_end")
    {
        invoke(false, 1, "u0:1", 0, 1, 0, false);
        invoke(true, 2, "u1:1", 1, 1, 0, false);
    }
    else if (mode == "manager_swap")
    {
        changedByObject[0] = true;
        invoke(true, 1, "u0:1," + releaseTrace(0, 1), 0, 1, 0, true);
        invoke(true, 1, "u3:1", 3, 1, 0, false, 1);
    }
    else if (mode == "dirty_replace_side")
    {
        rosters[0]->push_back(objects[1]); changedByObject[0] = true;
        invoke(false, 1, "u0:1", 0, 1, 0, true);
        invoke(true, 2, "u1:1,d0:1,m0:2,m0:2,release102:2," + releaseTrace(1, 2), 1, 1, 0, true);
    }
    else if (mode == "dirty_live_end")
    {
        changedByObject[0] = true;
        invoke(true, 1, "u0:1," + releaseTrace(0, 1) + "," + releaseTrace(1, 1), 0, 1, 0, true);
        require(rosters[0]->size() == 2, "dirty traversal observes appended end without updating appended object");
    }
    else if (mode == "destination_swap") invoke(true, 1, "u0:1", 0, 1, 2, false);
    else if (mode == "float_accumulation")
    {
        rosters[0]->push_back(objects[1]);
        for (int i = 0; i < 6; ++i)
        {
            objects[0]->m_statistics[0]->m_strength[i] = 16777216.0f;
            objects[1]->m_statistics[0]->m_strength[i] = 1.0f;
        }
        invoke(false, 1, "u0:1", 0, 1, 0, false);
        invoke(true, 2, "u1:1", 1, 1, 0, false);
        require(destinations[0]->m_strength[0] == 16777216.0f, "per-add float storage");
    }
    else if (mode == "release_returns" || mode == "release_refusal" || mode == "release_late_index")
    {
        *restartFlag = false;
        const int result = objects[0]->releaseAssignedResources(1);
        require(result == (mode == "release_refusal" ? 0 : -17), "full signed release result or actual commander refusal");
        const std::string exact = mode == "release_refusal" ? "m0:1" :
            (mode == "release_late_index" ? "m0:1,m0:1,release777:1" : "m0:1,m0:1,release100:1");
        require(calls == exact, "direct actual release callback order and late index read");
        compareStatistics(expected);
        require(updateCalls == 0 && dirtyCalls == 0, "direct helper isolation");
    }
    else unexpected("unknown mode");
    IAIMain::m_instance = nullptr;
    std::printf("PASS %s\n", mode.c_str());
}
