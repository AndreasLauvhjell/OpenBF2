// [TEMP] Whole production AIInformationGrid/InformationManager, exact extracted
// environment wrappers/selector, native objects and chosen environment/record lifetimes.
// The raw calls below ONLY test MSVC virtual slot placement on genuine native objects.
#include "information_query_runtime.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>

using namespace dice::hfe;
using namespace dice::hfe::ai;
static_assert(sizeof(void*) == 4, "MSVC x86 fixture only");
[[noreturn]] static void unexpected(const char* message)
{
    std::fprintf(stderr, "UNEXPECTED %s\n", message);
    std::fflush(stderr);
    std::abort();
}
static void require(bool condition, const char* message) { if (!condition) unexpected(message); }
static IAIInformationGrid* selectedGrids[6];
static std::string predicateCalls;

#include "query_mocks.inl"

namespace dice { namespace hfe { namespace ai {
// [CHOSEN, not recovered] Bypass unrelated registry/construction/destruction work.
IAIInformationEnvironment::IAIInformationEnvironment() {}
IAIInformationEnvironment::~IAIInformationEnvironment() {}
IAIInformationEnvironmentModifiers::IAIInformationEnvironmentModifiers() {}
IAIInformationEnvironmentModifiers::~IAIInformationEnvironmentModifiers() {}
BFInformationEnvironment::BFInformationEnvironment()
{
    m_gridsA.assign(selectedGrids, selectedGrids + 3);
    m_gridsB.assign(selectedGrids + 3, selectedGrids + 6);
}
BFInformationEnvironment::~BFInformationEnvironment() {} // fixture owns the six actual grids
Information::Information(IAIObject*, double) : m_physical(nullptr) {}
Information::~Information() {}
int Information::getInformationSort() { unexpected("base Information::getInformationSort"); }
} } }

class Predicate : public IAIInformationGridPredicate
{
public:
    char kind;
    int selector; // 0 = all, 1 = identities 1/2, 2 = identity 2 only
    Predicate(char k, int s) : kind(k), selector(s) {}
    bool includeInformation(Information* info) const override
    {
        auto record = static_cast<ObservedInformation*>(info);
        predicateCalls += kind;
        predicateCalls += std::to_string(record->identity);
        return selector == 0 || (selector == 1 ? record->identity <= 2 : record->identity == 2);
    }
};

static void expect(const std::vector<unsigned>& result, std::initializer_list<unsigned> values)
{
    require(result == std::vector<unsigned>(values), "exact output order and append behavior");
}

int main(int argc, char** argv)
{
    require(argc == 2, "one mode required");
    const std::string mode = argv[1];
    InformationManager manager;
    informationManager = &manager;
    Vec2 minimum = {}, maximum = {}; maximum.x = maximum.y = 10.0f;
    std::unique_ptr<AIInformationGrid> owned[6];
    for (unsigned i = 0; i < 6; ++i)
    {
        owned[i].reset(new AIInformationGrid(i % 3, 1, 1, minimum, maximum));
        selectedGrids[i] = owned[i].get();
    }
    BFInformationEnvironment environment;
    IAIInformationEnvironment* reader = &environment;
    const IAIInformationEnvironment* constReader = reader;
    ObservedInformation one(1), two(2), three(3), four(4), five(5), six(6);
    Information* records[] = {&one, &two, &three, &four, &five, &six};
    unsigned handles[6] = {};
    bool released[6] = {};
    Predicate keep('k', 1), cull('c', 2), all('a', 0);
    std::vector<unsigned> output{999};

    if (mode == "empty")
    {
        reader->getAllInformation(output, 2, true, &keep, &cull);
        constReader->getAllInformation(output, 1, false, &keep);
        expect(output, {999});
        require(predicateCalls.empty(), "empty grids invoke no predicates");
    }
    else if (mode == "selection_append")
    {
        for (unsigned i = 0; i < 6; ++i)
        {
            handles[i] = manager.getPool()->acquire(records[i]);
            owned[i]->addInfo(handles[i]);
        }
        for (unsigned i = 0; i < 6; ++i)
        {
            output.assign(1, 999);
            reader->getAllInformation(output, i % 3, i >= 3, nullptr, nullptr);
            expect(output, {999, handles[i]});
            owned[i]->removeInfo(handles[i]);
        }
    }
    else if (mode == "filter_cull" || mode == "null_fast_path")
    {
        auto grid = owned[4].get();
        const unsigned count = mode == "filter_cull" ? 4 : 3;
        for (unsigned i = 0; i < count; ++i)
        {
            handles[i] = manager.getPool()->acquire(records[i]);
            grid->addInfo(handles[i]);
        }
        unsigned stale = mode == "filter_cull" ? 3 : 1;
        manager.getPool()->release(handles[stale]); released[stale] = true;
        if (mode == "filter_cull")
        {
            reader->getAllInformation(output, 1, true, &keep, &cull);
            expect(output, {999, handles[0]});
            require(predicateCalls == "c1k1c2c3k3", "cull precedes keep, stale skips predicates");
        }
        else
        {
            reader->getAllInformation(output, 1, true, nullptr, nullptr);
            expect(output, {999, handles[0], handles[1], handles[2]});
            require(predicateCalls.empty(), "null/null fast path invokes no predicates");
            output.assign(1, 999);
            constReader->getAllInformation(output, 1, true, &all);
            expect(output, {999, handles[0], handles[2]});
            require(predicateCalls == "a1a3", "const environment forwards sweep with null cull");
        }
        output.clear();
        grid->getAllInformation(output, nullptr, nullptr);
        expect(output, {handles[0], handles[2]}); // verifies removals, including stale record
        grid->removeInfo(handles[0]); grid->removeInfo(handles[2]);
    }
    else if (mode == "pe_slot_sweep" || mode == "pe_slot_const")
    {
        auto grid = owned[2].get();
        handles[0] = manager.getPool()->acquire(&one);
        grid->addInfo(handles[0]);
        void** table = *reinterpret_cast<void***>(reader);
        if (mode == "pe_slot_sweep")
        {
            // PE address point 0x10282890 +0x30, five arguments.
            typedef void (__thiscall* Sweep)(IAIInformationEnvironment*, std::vector<unsigned>&,
                                             int, bool, IAIInformationGridPredicate*, IAIInformationGridPredicate*);
            reinterpret_cast<Sweep>(table[0x30 / 4])(reader, output, 2, false, nullptr, &all);
            require(predicateCalls == "a1", "slot +30 dispatches cull predicate");
        }
        else
        {
            manager.getPool()->release(handles[0]); released[0] = true;
            // PE address point 0x10282890 +0x34, const four-argument wrapper.
            typedef void (__thiscall* Query)(const IAIInformationEnvironment*, std::vector<unsigned>&,
                                             int, bool, IAIInformationGridPredicate*);
            reinterpret_cast<Query>(table[0x34 / 4])(constReader, output, 2, false, &all);
            require(predicateCalls.empty(), "stale record never reaches predicate");
        }
        expect(output, {999});
        output.clear();
        // Actual native grid sweep itself occupies PE +0x28 (ELF also +0x28).
        IAIInformationGrid* interface = grid;
        void** gridTable = *reinterpret_cast<void***>(interface);
        typedef void (__thiscall* GridSweep)(IAIInformationGrid*, std::vector<unsigned>&,
                                             IAIInformationGridPredicate*, IAIInformationGridPredicate*);
        reinterpret_cast<GridSweep>(gridTable[0x28 / 4])(interface, output, nullptr, nullptr);
        expect(output, {});
    }
    else unexpected("unknown mode");

    for (unsigned i = 0; i < 6; ++i)
        if (handles[i] && !released[i]) manager.getPool()->release(handles[i]);
    informationManager = nullptr;
    std::printf("PASS %s\n", mode.c_str());
    return 0;
}
