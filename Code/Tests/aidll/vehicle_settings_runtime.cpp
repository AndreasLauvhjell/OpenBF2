// [TEMP] Real AISettings construction/roster bodies and MaterialManager lifecycle.
// AISettings is intentionally left alive until each child process exits: its real destructor
// is an explicit stop, tested in a separate child. No substitute settings destructor is linked.
#include "vehicle_settings_runtime.h"
#include <windows.h>
#include <crtdbg.h>
#include <new>
#include <cmath>
#include <sstream>

using namespace dice::hfe;
using namespace dice::hfe::ai;
static int checks;
static void require(bool condition, const char* message)
{
    ++checks;
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::exit(1); }
}
[[noreturn]] void unexpected(const char* method)
{
    std::fprintf(stderr, "UNEXPECTED %s\n", method); std::abort();
}

// [TEMP] Typed access to one real native container, used only to supply the group table that
// belongs to the still-unrecovered pool builder. No raw offsets, fabricated object, or ABI claim.
struct GroupsTag { using type = std::vector<int> AISettings::*; friend type field(GroupsTag); };
template<class Tag, typename Tag::type Member> struct Access
{ friend typename Tag::type field(Tag) { return Member; } };
template struct Access<GroupsTag, &AISettings::m_vehicleGroups>;
static std::vector<int>& groups(AISettings& settings) { return settings.*field(GroupsTag{}); }

static const void* watched[16];
static int watchCount, deletionCount;
static int deletionOrder[16];
static void watch(const void* pointer)
{
    require(pointer != nullptr && watchCount < 16, "bounded actual allocation watch");
    watched[watchCount++] = pointer;
}
static void observeDelete(void* pointer)
{
    if (!pointer) return;
    for (int i = 0; i < watchCount; ++i)
        if (watched[i] == pointer)
        {
            if (deletionCount >= 16) std::abort();
            deletionOrder[deletionCount++] = i;
            watched[i] = nullptr;
            return;
        }
}
void* operator new(size_t size) { if (void* p = std::malloc(size ? size : 1)) return p; throw std::bad_alloc(); }
void* operator new[](size_t size) { if (void* p = std::malloc(size ? size : 1)) return p; throw std::bad_alloc(); }
void operator delete(void* pointer) noexcept { observeDelete(pointer); std::free(pointer); }
void operator delete[](void* pointer) noexcept { observeDelete(pointer); std::free(pointer); }
void operator delete(void* pointer, size_t) noexcept { observeDelete(pointer); std::free(pointer); }
void operator delete[](void* pointer, size_t) noexcept { observeDelete(pointer); std::free(pointer); }

static int registrations;
static std::string registryTrace;
static void* registeredRead;
static void* registeredWrite;
static unsigned char* parserResult;
namespace dice { namespace hfe {
ClassManager* classManager = nullptr;
// [TEMP] Only the external registry has a chosen native lifetime.
ClassManager::ClassManager() {}
ClassManager::~ClassManager() {}
void ClassManager::registerSimpleSingleton(const std::string& name, void* pointer)
{
    ++registrations;
    if (name == "IAISettings") registeredRead = pointer;
    else if (name == "IAISettingsModifier") registeredWrite = pointer;
    else unexpected("unexpected registration name");
    registryTrace += name + ",";
}
void ClassManager::unregisterSimpleSingleton(const std::string&) { unexpected("settings base destruction"); }
namespace ai {
void* CommonSingletonServer::m_iaiSettings = nullptr;
void* CommonSingletonServer::m_iaiSettingsModifier = nullptr;
} } }
#include "unused.inl"
#include "stock_data.inl"

static void debugOutput(DebugType type, const std::string&, const std::string&, int,
                        const std::string&, const std::string& text, const std::string&, void*)
{
    require(type == DebugTypeError, "actual error severity");
    if (parserResult) require(*parserResult == 77, "unknown material does not assign a fallback");
    std::printf("DEBUG %s\n", text.c_str()); std::fflush(stdout);
}

static AISettings* newSettings()
{
    AISettings* settings = new AISettings;
    require(settings->getNumberOfVehicles() == 0 && settings->getNumberOfVehicleGroups() == 0,
            "constructor keeps independent roster/group containers empty");
    return settings;
}
static void populate(AISettings& settings)
{
    for (const char* name : stockVehicles) settings.setVehicle(name);
}
static float defaultCost(int material)
{
    return material == 0 ? -1.0f : (material == 5 || material == 7 ? 1000.0f : 1.0f);
}

class GroupCapture : public AISettings
{
public:
    mutable std::vector<int> oldStorage;
    mutable int calls = 0;
    int getVehicleGroup(const std::string& name) const override
    {
        require(name == "after-callback", "group name passed unchanged");
        ++calls;
        groups(*const_cast<GroupCapture*>(this)).swap(oldStorage);
        return 9;
    }
};
class MaterialDispatch : public AISettings
{
public:
    mutable int calls = 0;
    int getVehicle(const std::string& name) const override
    {
        require(name == "logical", "named material call uses virtual vehicle lookup");
        ++calls;
        return 1;
    }
};

int main(int argc, char** argv)
{
    static_assert(sizeof(void*) == 4, "x86 fixture only");
    static_assert(std::is_same<decltype(std::declval<const IAISettings&>().getVehicle(0)), const std::string&>::value,
                  "borrowed vehicle name return");
    static_assert(std::is_same<decltype(std::declval<const IAISettings&>().getVehicleClusterMapping(std::declval<const std::string&>())), const std::string&>::value,
                  "borrowed cluster name return");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    require(argc == 2, "mode argument");
    const std::string mode(argv[1]);
    ClassManager registry;
    classManager = &registry;
    setDebugCallback(DebugTypeError, debugOutput, nullptr);
    if (mode == "material_parser")
    {
        const char* const tokens[] = {"Invalid", "gRoUnD", "ROAD", "shallows", "DeepWater",
            "dangerOBJECT", "OtherDangerObject", "BOTHdangerobject", "NumberOfMaterials"};
        const unsigned char expected[] = {0,1,2,3,4,5,6,7,9};
        for (int i = 0; i < 9; ++i)
        {
            std::istringstream stream(std::string("  ") + tokens[i] + " tail");
            materialId material = 77;
            require(&dice::hfe::ai::operator>>(stream, material) == &stream, "extractor returns original stream");
            require(material == expected[i] && !stream.fail(), "named material including mixed case and sentinel gap");
            std::string rest; stream >> rest;
            require(rest == "tail", "one token consumed");
        }
        std::istringstream eofStream("Ground");
        materialId atEof = 77;
        dice::hfe::ai::operator>>(eofStream, atEof);
        require(atEof == 1 && eofStream.eof() && !eofStream.fail(), "final valid token retains normal stream EOF state");
    }
    else if (mode == "numeric_material" || mode == "empty_material")
    {
        materialId material = 77;
        parserResult = &material;
        std::istringstream stream(mode == "numeric_material" ? "1" : "");
        dice::hfe::ai::operator>>(stream, material);
        unexpected("unknown material parser returned");
    }
    else if (mode == "registration")
    {
        AISettings* first = newSettings();
        require(registeredRead == static_cast<IAISettings*>(first), "primary registration pointer");
        require(registeredWrite == static_cast<IAISettingsModifier*>(first), "secondary registration pointer");
        require(registryTrace == "IAISettings,IAISettingsModifier,", "registration order");
        AISettings* second = newSettings();
        require(registrations == 4 && registeredRead == static_cast<IAISettings*>(second), "unrefreshed statics permit second registration");
        CommonSingletonServer::m_iaiSettings = static_cast<IAISettings*>(first);
        CommonSingletonServer::m_iaiSettingsModifier = static_cast<IAISettingsModifier*>(first);
        newSettings();
        require(registrations == 4, "cached nonnull statics suppress both registrations");
    }
    else if (mode == "material_defaults" || mode == "material_values" || mode == "material_destruction")
    {
        MaterialManager* manager = new MaterialManager;
        require(manager->m_materials.size() == 9, "nine material cells");
        for (int i = 0; i < 9; ++i)
        {
            require(manager->getMaterialCost(static_cast<unsigned char>(i)) == defaultCost(i), "exact initial material cost");
            require(manager->isValidMaterial(static_cast<unsigned char>(i)) == (i != 0), "initial validity follows cost sign");
        }
        if (mode == "material_values")
        {
            const float values[] = {-3.5f, -0.0f, 0.0f, 2.75f, std::numeric_limits<float>::infinity(),
                                    -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()};
            for (float value : values)
            {
                manager->setMaterialCost(8, value);
                const float actual = manager->getMaterialCost(8);
                require(std::memcmp(&actual, &value, sizeof(float)) == 0, "cost bits retained including signed zero and NaN");
                require(manager->isValidMaterial(8) == (value >= 0.0f), "ordered nonnegative validity");
            }
        }
        if (mode == "material_destruction")
        {
            for (auto cell : manager->m_materials) watch(cell);
            watch(manager->m_materials.data());
            watch(manager);
        }
        delete manager;
        if (mode == "material_destruction")
        {
            require(deletionCount == 11, "nine cells then vector storage and object all deleted once");
            for (int i = 0; i < 11; ++i) require(deletionOrder[i] == i, "actual material ownership deletion order");
        }
    }
    else if (mode == "group_target_capture")
    {
        GroupCapture* settings = new GroupCapture;
        groups(*settings) = {3, 4};
        settings->oldStorage = {7, 8};
        settings->addVehicleToVehicleGroup(1, "after-callback");
        require(settings->calls == 1, "one virtual group lookup");
        require(groups(*settings)[0] == 7 && groups(*settings)[1] == 8, "PE keeps target from before callback");
        require(settings->oldStorage[0] == 3 && settings->oldStorage[1] == 9, "captured native storage receives group result");
    }
    else if (mode == "material_dispatch")
    {
        MaterialDispatch* settings = new MaterialDispatch;
        settings->setVehicle("one"); settings->setVehicle("two");
        settings->setVehicleMaterialCost("logical", 8, 3.125f);
        require(settings->calls == 1, "setter invokes virtual vehicle name lookup once");
        require(settings->getVehicleMaterialCost("logical", 8) == 3.125f && settings->calls == 2, "named getter invokes virtual lookup once");
        require(settings->getVehicleMaterialCost(0, 8) == 1.0f && settings->getVehicleMaterialCost(1, 8) == 3.125f,
                "numeric getter selects independent manager without virtual name lookup");
        require(settings->calls == 2, "numeric getters skip name dispatch");
    }
    else
    {
        AISettings* settings = newSettings();
        if (mode == "settings_destructor") { delete settings; unexpected("settings destructor returned"); }
        else if (mode == "known_vehicle_group")
        {
            settings->setVehicle("Tank"); settings->createVehicleGroup("ground");
            settings->addVehicleToVehicleGroup(std::string("Tank"), "ground");
            unexpected("inverted known-name error returned");
        }
        else if (mode == "negative_vehicle" || mode == "past_vehicle")
        {
            settings->setVehicle("Tank");
            settings->getVehicle(mode == "negative_vehicle" ? -1 : 1);
            unexpected("invalid vehicle error returned");
        }
        else if (mode == "duplicates")
        {
            settings->setVehicle("Alpha"); settings->setVehicle("Alpha");
            settings->setVehicle("Beta"); settings->setVehicle("alpha");
            require(settings->getVehicle("Alpha") == 0 && settings->getVehicle("Beta") == 1
                    && settings->getVehicle("alpha") == 2, "name map uses pre-insert map size and first-wins names");
            require(settings->getVehicle(0) == "Alpha" && settings->getVehicle(1) == "Alpha"
                    && settings->getVehicle(2) == "Beta" && settings->getVehicle(3) == "alpha", "duplicate still appends name");
            require(settings->getVehicleMaterialManager(0) != settings->getVehicleMaterialManager(1), "duplicate owns new material manager");
            require(settings->getClusterCostTypes().size() == 4, "duplicate still appends cluster slot");
            require(settings->getNumberOfVehicles() == 0, "roster insertion does not build behavior pools");
        }
        else if (mode == "groups")
        {
            settings->createVehicleGroup("ground"); settings->createVehicleGroup("ground"); settings->createVehicleGroup("air");
            require(settings->getNumberOfVehicleGroups() == 3 && settings->getVehicleGroup("ground") == 0
                    && settings->getVehicleGroup("air") == 2, "groups append names and keep first map entry");
            require(settings->getVehicleGroupName(0) == "ground" && settings->getVehicleGroupName(1) == "ground"
                    && settings->getVehicleGroupName(2) == "air", "group name slots preserve insertion order");
            require(groups(*settings).empty(), "creating groups does not size vehicle membership table");
            groups(*settings) = {-1, -1, -1};
            settings->addVehicleToVehicleGroup(1, "air");
            require(settings->getVehicleGroup(0) == -1 && settings->getVehicleGroup(1) == 2
                    && settings->getVehicleGroup(2) == -1, "integer overload writes only selected native slot");
        }
        else if (mode == "behaviour_count")
        {
            const int values[] = {13, 257, -1, 0, 2147483647};
            for (int value : values)
            {
                settings->setMaxNumberOfBehaviours(value);
                require(settings->getNumberOfBehaviours() == value, "complete signed settings behavior count retained");
                require(BotMain::getNBehaviours() == static_cast<unsigned char>(value), "shared bot behavior byte truncation");
                require(settings->getNumberOfVehicles() == 0, "count setter does not provision behavior pools");
            }
        }
        else
        {
            populate(*settings);
            require(sizeof(stockVehicles) / sizeof(*stockVehicles) == 15, "stock roster input count");
            if (mode == "stock_roster")
            {
                for (int i = 0; i < 15; ++i)
                {
                    require(settings->getVehicle(stockVehicles[i]) == i, "stock names have ordered ids");
                    require(settings->getVehicle(i) == stockVehicles[i], "stock id returns corresponding name");
                    require(&settings->getVehicle(i) == &settings->getVehicle(i), "vehicle name is borrowed");
                    require(settings->getClusterCostType(i) == -1, "each unmapped roster entry starts at minus one");
                    for (int m = 0; m < 9; ++m)
                        require(settings->getVehicleMaterialCost(i, static_cast<unsigned char>(m)) == defaultCost(m), "per-vehicle default table");
                    for (int j = 0; j < i; ++j)
                        require(settings->getVehicleMaterialManager(i) != settings->getVehicleMaterialManager(j), "separate material owner per roster entry");
                }
                require(settings->getVehicle("infantery") == -1 && settings->getVehicle("missing") == -1, "vehicle lookup case sensitivity and miss sentinel");
                require(settings->getNumberOfVehicles() == 0 && groups(*settings).empty(), "setVehicle leaves behavior and membership pools alone");
            }
            else if (mode == "clusters")
            {
                const auto& costs = settings->getClusterCostTypes();
                require(costs.size() == 15, "borrowed cluster roster size");
                for (const auto& mapping : stockMappings)
                {
                    settings->setVehicleClusterMapping(mapping.source, mapping.target);
                    const int from = settings->getVehicle(mapping.source), to = settings->getVehicle(mapping.target);
                    require(costs[from] == to, "borrowed cluster vector sees store");
                    require(&settings->getVehicleClusterMapping(mapping.source) == &settings->getVehicle(to), "mapping returns borrowed target name");
                }
                require(costs[1] == -1 && costs[4] == -1, "unmapped slots stay unchanged");
                settings->setVehicleClusterMapping("Tank", "Boat");
                require(settings->getVehicleClusterMapping("Tank") == "Boat" && costs[0] == 2, "mapping overwrite");
            }
            else if (mode == "stock_costs")
            {
                for (const auto& cost : stockCosts) settings->setVehicleMaterialCost(cost.vehicle, cost.material, cost.cost);
                const int vehicles[] = {0, 2, 3, 5, 6, 12};
                const float expected[][4] = {{1,1,1.5f,-1},{-1,-1,-1,1},{1,1,1.5f,6},{1,1,1.8f,-1},{10,10,2,1},{1,1,1,2}};
                for (int row = 0; row < 6; ++row)
                    for (unsigned char m = 1; m <= 4; ++m)
                    {
                        require(settings->getVehicleMaterialCost(vehicles[row], m) == expected[row][m-1], "stock named-material cost mapping");
                        require(settings->isValidMaterial(vehicles[row], m) == (expected[row][m-1] >= 0), "stock material validity");
                    }
                require(settings->getVehicleMaterialCost(1, 4) == 1 && settings->getVehicleMaterialCost(0, 5) == 1000,
                        "stock edits preserve other vehicles and other material slots");
            }
            else if (mode == "man_down")
            {
                settings->setManDownEquipment("ManDown");
                require(settings->getManDownEquipmentType() == 13, "stock man-down vehicle id");
                settings->setManDownEquipment("missing");
                require(settings->getManDownEquipmentType() == -1, "man-down name uses ordinary virtual vehicle lookup");
            }
            else unexpected("unknown fixture mode");
        }
    }
    std::printf("PASS %s %d checks\n", mode.c_str(), checks);
    return 0;
}
