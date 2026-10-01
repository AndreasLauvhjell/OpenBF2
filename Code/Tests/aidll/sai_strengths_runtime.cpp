// [TEMP] [CHOSEN, not recovered] Isolated production updateStrengths fixture.
// Query, Information forwarding, occupancy and object lifetimes are chosen observers.
// Real headers, native objects, UnitPredicate and InformationManager/pool remain real.
// This does not pass the production nonempty Information/IPIUnit frontier bodies.
#include "sai_strengths_runtime.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <string>

using namespace dice::hfe;
using namespace dice::hfe::ai;
static_assert(sizeof(void*) == 4, "MSVC x86 fixture only");
[[noreturn]] static void unexpected(const char* text)
{
    std::fprintf(stderr, "UNEXPECTED %s\n", text); std::fflush(stderr); std::abort();
}
static void require(bool condition, const char* text) { if (!condition) unexpected(text); }
struct Event { const char* name; int id; };
static Event events[128];
static unsigned eventCount;
static void trace(const char* name, int id = -1)
{
    require(eventCount < 128, "event capacity"); events[eventCount++] = {name, id};
}
static std::string mode;
static float seeds[4][6], wanted[4][6];
static float* totals[4]; // enemy strengths/counts, friendly strengths/counts
static int* saiSide;
static bool sameFloat(float actual, float expected)
{
    return std::isnan(expected) ? std::isnan(actual) : std::memcmp(&actual, &expected, 4) == 0;
}
static void checkTotals()
{
    for (int array = 0; array < 4; ++array) for (int category = 0; category < 6; ++category)
        if (!sameFloat(totals[array][category], wanted[array][category]))
        {
            std::fprintf(stderr, "FAIL array %d category %d: %.9g != %.9g\n", array, category,
                         totals[array][category], wanted[array][category]); std::exit(1);
        }
}
// Matching chosen allocator pair; only the live query predicate is observed.
static void* livePredicate;
static unsigned predicateDeletes;
void* operator new(std::size_t size)
{
    if (void* value = std::malloc(size ? size : 1)) return value;
    throw std::bad_alloc();
}
void operator delete(void* value) noexcept
{
    if (value && value == livePredicate)
    {
        livePredicate = nullptr; ++predicateDeletes;
        checkTotals(); // The original predicate is released only after the entire decay sweep.
        trace("delete");
    }
    std::free(value);
}
void operator delete(void* value, std::size_t) noexcept { ::operator delete(value); }

struct Record
{
    Information* information;
    IPIUnit* unit;
    IPIPhysical* physical;
    IPIUnit** unitSlot;
    IPIPhysical** physicalSlot;
    int side = 1, category = 0;
    bool occupied = true;
    float security = 1.0f;
    float strengths[6] = {};
    unsigned int children[3] = {};
    unsigned childCount = 0;
};
static Record records[5];
static int constructing;
static unsigned int rootHandles[3];
static unsigned rootCount;
static InformationManager* alternateManager;
static bool replaceUnit, changeSide, changeManager;
static int categoryArray = -1;
static float beforeCategory[6];

static int recordOf(const Information* information)
{
    for (int i = 0; i < 5; ++i) if (records[i].information == information) return i;
    unexpected("Information observer identity");
}
static int observeSide(int id)
{
    trace("side", id);
    if (changeSide) { *saiSide = 2; changeSide = false; }
    return records[id].side;
}
static float observeSecurity(int id)
{
    trace("security", id);
    if (replaceUnit)
    {
        require(id == 0, "replacement on the selected Information");
        *records[0].unitSlot = records[2].unit;
        *records[0].physicalSlot = records[2].physical;
        replaceUnit = false;
    }
    return records[id].security;
}
static int observeCategory(int id)
{
    trace("category", id);
    if (categoryArray >= 0)
        for (int i = 0; i < 6; ++i)
            require(sameFloat(totals[categoryArray][i], beforeCategory[i]), "all six strengths precede category callback");
    return records[id].category;
}
static void observeQuery(std::vector<unsigned int>& out, int side, bool which,
                         IAIInformationGridPredicate* keep, IAIInformationGridPredicate* cull)
{
    trace("query");
    require(out.empty() && side == *saiSide && which && !cull && keep, "two-predicate query arguments");
    require(dynamic_cast<UnitPredicate*>(keep) != nullptr, "actual UnitPredicate is supplied");
    require(!livePredicate, "one live predicate per update"); livePredicate = keep;
    for (int i = 0; i < 5; ++i)
        require(keep->includeInformation(records[i].information), "real UnitPredicate sees genuine typed unit member");
    IPIUnit* const savedUnit = *records[4].unitSlot;
    *records[4].unitSlot = nullptr;
    require(!keep->includeInformation(records[4].information), "real UnitPredicate excludes null unit");
    *records[4].unitSlot = savedUnit;
    for (unsigned i = 0; i < rootCount; ++i) out.push_back(rootHandles[i]);
}

#include "sai_strengths_mocks.inl"

namespace dice { namespace hfe { namespace ai {
// Chosen fixture lifetimes. Production Information forwarding and IPIUnit occupancy
// stay frontiers; these observers test only the recovered caller's contract.
ISAI::~ISAI() {}
SAI::SAI(int side, int, int, Vec2, Vec2) : m_side(side)
{
    totals[0] = m_enemyStrengths; totals[1] = m_enemyCounts;
    totals[2] = m_friendlyStrengths; totals[3] = m_friendlyCounts;
    saiSide = &m_side;
    for (int a = 0; a < 4; ++a) for (int i = 0; i < 6; ++i) totals[a][i] = seeds[a][i];
}
SAI::~SAI() {}
Information::Information(IAIObject* object, double time)
    : m_time(time), m_object(object), m_physical(nullptr), m_unit(nullptr)
{
    records[constructing].unitSlot = &m_unit;
    records[constructing].physicalSlot = &m_physical;
}
Information::~Information() {}
int Information::getInformationSort() { unexpected("Information::getInformationSort"); }
int Information::getFirstSecondaryObjectIterator() const
{
    const int id = recordOf(this); trace("first", id); return 41;
}
unsigned int Information::getSecondaryObject(int& iterator, int side)
{
    const int id = recordOf(this); trace("secondary", id);
    require(side == *saiSide, "secondary uses current SAI side");
    require(iterator >= 41 && iterator <= 44, "iterator passed by reference without reset");
    const unsigned index = static_cast<unsigned>(iterator++ - 41);
    if (changeManager) { informationManager = alternateManager; changeManager = false; }
    return index < records[id].childCount ? records[id].children[index] : 0;
}

IPIUnit::IPIUnit(Information* information)
    : m_battleStrengths(records[constructing].strengths), m_strategicStrengths(nullptr),
      m_strengthMod(0.0f), m_ammo(0.0f), m_ammoTime(0.0), m_ownsPointer24(false), m_pointer24(nullptr)
{
    m_information = information;
}
IPIUnit::~IPIUnit() {}
int IPIUnit::getType() { unexpected("IPIUnit::getType"); }
void IPIUnit::setTime() { unexpected("IPIUnit::setTime"); }
void IPIUnit::setTime(double) { unexpected("IPIUnit::setTime(double)"); }
bool IPIUnit::isOccupied()
{
    for (int i = 0; i < 5; ++i) if (records[i].unit == this)
    { trace("occupied", i); return records[i].occupied; }
    unexpected("IPIUnit identity");
}

int IPIPhysical::getType() { unexpected("IPIPhysical::getType"); }
IAIInformationEnvironment::IAIInformationEnvironment() {}
IAIInformationEnvironment::~IAIInformationEnvironment() {}
void* AISingletonServer::m_iaiInformationEnvironment = nullptr;
} } }

static void checkTrace(const std::string& expected)
{
    std::string actual;
    for (unsigned i = 0; i < eventCount; ++i)
    {
        if (i) actual += ',';
        actual += events[i].name;
        if (events[i].id >= 0) actual += std::to_string(events[i].id);
    }
    if (actual != expected)
    {
        std::fprintf(stderr, "FAIL trace\nexpected: %s\nactual:   %s\n", expected.c_str(), actual.c_str());
        std::exit(1);
    }
}

int main(int argc, char** argv)
{
    require(argc == 2, "one fixture mode"); mode = argv[1];
    // Both managers use actual production lifetimes and native handle pools.
    InformationManager firstManager, secondManager;
    alternateManager = &secondManager; informationManager = &firstManager;
    ObservedInformation* info[5]; IPIUnit* unit[5]; ObservedPhysical physical[5];
    for (int i = 0; i < 5; ++i)
    {
        constructing = i;
        info[i] = new ObservedInformation; info[i]->identity = i; records[i].information = info[i];
        unit[i] = new IPIUnit(info[i]); records[i].unit = unit[i]; *records[i].unitSlot = unit[i];
        physical[i].identity = i; records[i].physical = &physical[i]; *records[i].physicalSlot = &physical[i];
    }
    unsigned handles[5];
    for (int i = 0; i < 5; ++i)
    {
        handles[i] = firstManager.getPool()->acquire(info[i]);
        require(secondManager.getPool()->acquire(info[i]) == handles[i], "matching native handles across managers");
    }
    rootHandles[0] = handles[0]; rootCount = 1;
    const std::string single = "query,first0,side0,occupied0,security0,category0,secondary0,delete";
    std::string expected = single;
    if (mode == "empty_decay")
    {
        rootCount = 0; expected = "query,delete";
        const float values[] = {0.001f, std::nextafter(0.001f, 1.0f), -2.0f,
                               std::numeric_limits<float>::quiet_NaN(), -0.0f, 8.0f};
        for (int a = 0; a < 4; ++a) for (int i = 0; i < 6; ++i)
        {
            seeds[a][i] = values[(i + a) % 6]; wanted[a][i] = seeds[a][i];
            if (wanted[a][i] > 0.001f) wanted[a][i] *= 0.5f;
        }
    }
    else if (mode == "mixed_chain")
    {
        rootCount = 2; rootHandles[1] = handles[3];
        records[0].children[0] = handles[1]; records[0].children[1] = handles[2]; records[0].childCount = 2;
        records[0].security = 0.5f; records[0].category = 1;
        records[1].side = 2; records[1].security = 0.25f; records[1].category = 2;
        records[2].occupied = false;
        records[3].side = 2; records[3].security = 2.0f; records[3].category = 5;
        for (int i = 0; i < 6; ++i)
        {
            records[0].strengths[i] = static_cast<float>(i + 1);
            records[1].strengths[i] = static_cast<float>(8 - i);
            records[3].strengths[i] = 0.5f;
            seeds[0][i] = 1.0f; seeds[2][i] = 2.0f;
            wanted[0][i] = 1.0f;
            wanted[2][i] = (2.0f + 0.5f * records[0].strengths[i] + 0.25f * records[1].strengths[i]) * 0.5f;
        }
        wanted[3][1] = 0.25f; wanted[3][2] = 0.125f; wanted[1][5] = 1.0f;
        expected = "query,first0,side0,occupied0,security0,category0,secondary0,occupied1,security1,category1,secondary0,occupied2,secondary0,first3,side3,occupied3,security3,category3,secondary3,delete";
    }
    else if (mode == "unoccupied")
    {
        records[0].occupied = false;
        expected = "query,first0,side0,occupied0,secondary0,delete";
    }
    else if (mode == "wide_product")
    {
        records[0].security = 1.0f + std::ldexp(1.0f, -23);
        records[0].strengths[0] = 1.0f - std::ldexp(1.0f, -23);
        seeds[2][0] = -1.0f; wanted[2][0] = -std::ldexp(1.0f, -46);
        wanted[3][0] = records[0].security * 0.5f;
        categoryArray = 2; beforeCategory[0] = wanted[2][0];
    }
    else if (mode == "unit_capture")
    {
        replaceUnit = true; records[0].category = 1; records[2].category = 4;
        records[0].security = 2.0f;
        for (int i = 0; i < 6; ++i)
        {
            records[0].strengths[i] = static_cast<float>(i + 1); records[2].strengths[i] = 100.0f;
            beforeCategory[i] = 2.0f * records[0].strengths[i]; wanted[2][i] = records[0].strengths[i];
        }
        wanted[3][4] = 1.0f; categoryArray = 2;
        expected = "query,first0,side0,occupied0,security0,category2,secondary0,delete";
    }
    else if (mode == "side_after_callback")
    {
        changeSide = true; records[0].side = 2; records[0].strengths[0] = 8.0f;
        wanted[2][0] = 4.0f; wanted[3][0] = 0.5f;
    }
    else if (mode == "manager_after_secondary")
    {
        changeManager = true; rootCount = 2; rootHandles[1] = handles[3];
        records[0].occupied = false; records[0].children[0] = handles[1]; records[0].childCount = 1;
        // The same secondary/root handles resolve to distinct objects after the callback.
        secondManager.getPool()->set(handles[1], info[2]); secondManager.getPool()->set(handles[3], info[4]);
        records[2].strengths[0] = 4.0f; records[2].category = 2;
        records[4].side = 2; records[4].strengths[1] = 8.0f; records[4].category = 4;
        wanted[2][0] = 2.0f; wanted[3][2] = 0.5f; wanted[0][1] = 4.0f; wanted[1][4] = 0.5f;
        expected = "query,first0,side0,occupied0,secondary0,occupied2,security2,category2,secondary0,first4,side4,occupied4,security4,category4,secondary4,delete";
    }
    else if (mode == "repeated_accumulation")
    {
        records[0].strengths[0] = 4.0f; seeds[2][0] = 8.0f;
        wanted[2][0] = 6.0f; wanted[3][0] = 0.5f;
    }
    else unexpected("unknown fixture mode");

    ObservedEnvironment environment;
    AISingletonServer::m_iaiInformationEnvironment = &environment;
    SAI subject(1, 0, 0, Vec2(), Vec2());
    subject.updateStrengths();
    require(predicateDeletes == 1 && !livePredicate, "predicate freed exactly once");
    checkTotals(); checkTrace(expected);
    if (mode == "repeated_accumulation")
    {
        eventCount = 0; wanted[2][0] = 5.0f; wanted[3][0] = 0.75f;
        subject.updateStrengths(); checkTotals(); checkTrace(single);
        require(predicateDeletes == 2 && !livePredicate, "new predicate lifetime for second update");
    }
    for (int i = 0; i < 5; ++i) { delete unit[i]; delete info[i]; }
    std::printf("PASS %s\n", mode.c_str());
    return 0;
}
