// [TEMP] Focused execution of the exact production MapContainer::init body.
//
// The MapContainer and interface lifetimes below are fixture-only. Every unexercised virtual
// reports and terminates. No server is launched and the retail missing-key undefined behavior is
// deliberately not executed.
#include "mapcontainer_init_runtime.h"

#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <map>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <Windows.h>
#include <crtdbg.h>
#endif

[[noreturn]] static void unexpected(const char* what)
{
    std::fprintf(stderr, "UNEXPECTED %s\n", what);
    std::fflush(stderr);
    std::exit(91);
}

namespace dice {
namespace hfe {
namespace ai {

// [TEMP] Chosen fixture lifetimes. Only the two STL members read/written by init are populated.
IAIPathfindingSetup::IMapContainer::IMapContainer()
{
}

IAIPathfindingSetup::IMapContainer::~IMapContainer()
{
}

AIPathfinding::MapContainer::MapContainer(const std::string& name, int id)
    : m_id(id)
    , m_name(name)
    , m_properties(0)
    , m_minPolygonsInLeaf(4)
    , m_maxDepthInTree(8)
    , m_quadTreeManager(0)
{
}

AIPathfinding::MapContainer::~MapContainer()
{
}

IAISettings::IAISettings()
{
}

IAISettings::~IAISettings()
{
}

void* CommonSingletonServer::m_iaiSettings = 0;

#include "unused.inl"
#include "settings.inl"

int InitSettings::getVehicle(const std::string& name) const
{
    m_calls.push_back(std::string("vehicle:") + name);
    std::map<std::string, int>::const_iterator found = m_vehicleIds.find(name);
    if (found == m_vehicleIds.end())
        unexpected("InitSettings::getVehicle unknown fixture name");
    return found->second;
}

const std::vector<int>& InitSettings::getClusterCostTypes() const
{
    m_calls.push_back("types");
    ++m_clusterGetterCalls;
    return m_clusterTypes;
}

} // namespace ai
} // namespace hfe
} // namespace dice

using namespace dice::hfe::ai;

static unsigned int g_checks = 0;

#define CHECK(expression)                                                                    \
    do {                                                                                     \
        ++g_checks;                                                                          \
        if (!(expression)) {                                                                 \
            std::fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #expression);       \
            std::fflush(stderr);                                                             \
            std::exit(90);                                                                   \
        }                                                                                    \
    } while (0)

static std::vector<std::string> strings(std::initializer_list<const char*> values)
{
    std::vector<std::string> result;
    for (std::initializer_list<const char*>::const_iterator i = values.begin(); i != values.end(); ++i)
        result.push_back(*i);
    return result;
}

static void expectCalls(const InitSettings& settings, std::initializer_list<const char*> expected)
{
    CHECK(settings.m_calls == strings(expected));
    CHECK(settings.m_clusterGetterCalls == 1);
}

static void publish(InitSettings& settings)
{
    CommonSingletonServer::m_iaiSettings = static_cast<IAISettings*>(&settings);
}

static void runOrderedTranslation()
{
    InitSettings settings;
    settings.m_vehicleIds["thirty"] = 30;
    settings.m_vehicleIds["ten"] = 10;
    settings.m_vehicleIds["twenty"] = 20;
    settings.m_clusterTypes = {20, 30, 10, 20};
    publish(settings);

    AIPathfinding::MapContainer map("ordered", 7);
    map.m_clusterCostVehicles = {"thirty", "ten", "twenty"};
    const std::vector<std::string> namesBefore = map.m_clusterCostVehicles;
    map.init();

    CHECK(map.m_clusterCostTypes == std::vector<int>({2, 0, 1, 2}));
    CHECK(map.m_clusterCostVehicles == namesBefore);
    expectCalls(settings, {"vehicle:thirty", "vehicle:ten", "vehicle:twenty", "types"});
}

static void runDuplicateFirstWins()
{
    InitSettings settings;
    settings.m_vehicleIds["first"] = 10;
    settings.m_vehicleIds["duplicate"] = 10;
    settings.m_vehicleIds["twenty"] = 20;
    settings.m_clusterTypes = {10, 20};
    publish(settings);

    AIPathfinding::MapContainer map("duplicates", 0);
    map.m_clusterCostVehicles = {"first", "duplicate", "twenty"};
    map.init();

    CHECK(map.m_clusterCostTypes == std::vector<int>({0, 2}));
    expectCalls(settings, {"vehicle:first", "vehicle:duplicate", "vehicle:twenty", "types"});
}

static void runMinusOneKey()
{
    InitSettings settings;
    settings.m_vehicleIds["unknown"] = -1;
    settings.m_vehicleIds["valid"] = 42;
    settings.m_clusterTypes = {-1, 42, -1};
    publish(settings);

    AIPathfinding::MapContainer map("minus-one", 0);
    map.m_clusterCostVehicles = {"unknown", "valid"};
    map.init();

    CHECK(map.m_clusterCostTypes == std::vector<int>({0, 1, 0}));
    expectCalls(settings, {"vehicle:unknown", "vehicle:valid", "types"});
}

static void runGrowOverwrite()
{
    InitSettings settings;
    settings.m_vehicleIds["thirty"] = 30;
    settings.m_vehicleIds["ten"] = 10;
    settings.m_vehicleIds["twenty"] = 20;
    settings.m_clusterTypes = {20, 30, 10, 20};
    publish(settings);

    AIPathfinding::MapContainer map("grow", 0);
    map.m_clusterCostVehicles = {"thirty", "ten", "twenty"};
    map.m_clusterCostTypes = {999};
    map.init();

    CHECK(map.m_clusterCostTypes.size() == 4);
    CHECK(map.m_clusterCostTypes == std::vector<int>({2, 0, 1, 2}));
    expectCalls(settings, {"vehicle:thirty", "vehicle:ten", "vehicle:twenty", "types"});
}

static void runShrinkOverwrite()
{
    InitSettings settings;
    settings.m_vehicleIds["thirty"] = 30;
    settings.m_vehicleIds["ten"] = 10;
    settings.m_vehicleIds["twenty"] = 20;
    settings.m_clusterTypes = {20};
    publish(settings);

    AIPathfinding::MapContainer map("shrink", 0);
    map.m_clusterCostVehicles = {"thirty", "ten", "twenty"};
    map.m_clusterCostTypes = {99, 98, 97, 96};
    map.init();

    CHECK(map.m_clusterCostTypes.size() == 1);
    CHECK(map.m_clusterCostTypes == std::vector<int>({2}));
    expectCalls(settings, {"vehicle:thirty", "vehicle:ten", "vehicle:twenty", "types"});
}

static void runRepeatAfterReorder()
{
    InitSettings settings;
    settings.m_vehicleIds["a"] = 10;
    settings.m_vehicleIds["b"] = 20;
    settings.m_vehicleIds["c"] = 30;
    settings.m_clusterTypes = {30, 10};
    publish(settings);

    AIPathfinding::MapContainer map("repeat", 0);
    map.m_clusterCostVehicles = {"a", "b", "c"};
    map.init();
    CHECK(map.m_clusterCostTypes == std::vector<int>({2, 0}));
    expectCalls(settings, {"vehicle:a", "vehicle:b", "vehicle:c", "types"});

    settings.m_calls.clear();
    settings.m_clusterGetterCalls = 0;
    map.m_clusterCostVehicles = {"c", "a", "b"};
    map.m_clusterCostTypes = {777, 778, 779};
    map.init();
    CHECK(map.m_clusterCostTypes == std::vector<int>({0, 1}));
    expectCalls(settings, {"vehicle:c", "vehicle:a", "vehicle:b", "types"});
}

static void runEmptyEmpty()
{
    InitSettings settings;
    publish(settings);

    AIPathfinding::MapContainer map("empty", 0);
    map.m_clusterCostTypes = {88, 89};
    map.init();

    CHECK(map.m_clusterCostVehicles.empty());
    CHECK(map.m_clusterCostTypes.empty());
    expectCalls(settings, {"types"});
}

int main(int argc, char** argv)
{
#if defined(_WIN32)
    SetErrorMode(SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
#endif
    if (argc != 2)
        unexpected("usage: mapcontainer_init_runtime CASE");

    const std::string mode = argv[1];
    if (mode == "ordered_translation")
        runOrderedTranslation();
    else if (mode == "duplicate_first_wins")
        runDuplicateFirstWins();
    else if (mode == "minus_one_key")
        runMinusOneKey();
    else if (mode == "grow_overwrite")
        runGrowOverwrite();
    else if (mode == "shrink_overwrite")
        runShrinkOverwrite();
    else if (mode == "repeat_after_reorder")
        runRepeatAfterReorder();
    else if (mode == "empty_empty")
        runEmptyEmpty();
    else
        unexpected("unknown case");

    CommonSingletonServer::m_iaiSettings = 0;
    std::printf("PASS %s %u checks\n", mode.c_str(), g_checks);
    return 0;
}

