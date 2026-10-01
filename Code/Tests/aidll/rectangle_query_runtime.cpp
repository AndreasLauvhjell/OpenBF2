// [TEMP] Verbatim rectangle wrapper/selector plus whole actual grid/manager TUs.
// Environment and Information lifetimes are chosen native fixtures. Grid observers
// isolate forwarding; separate real-grid modes exercise bounded spatial behavior.
#include "rectangle_query_runtime.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>
#include <windows.h>
#include <crtdbg.h>

using namespace dice::hfe;
using namespace dice::hfe::ai;
static_assert(sizeof(void*) == 4, "MSVC x86 only");
using EnvironmentQuery = void (IAIInformationEnvironment::*)(std::vector<unsigned>&,
    const Vec2&, const Vec2&, int, bool, IAIInformationGridPredicate*) const;
using GridQuery = void (IAIInformationGrid::*)(std::vector<unsigned>&,
    const Vec2&, const Vec2&, IAIInformationGridPredicate*) const;
static_assert(std::is_same<decltype(&IAIInformationEnvironment::getObjectsWithinRectangle), EnvironmentQuery>::value,
              "exact const environment rectangle contract");
static_assert(std::is_same<decltype(&IAIInformationGrid::getInformationWithinRectangle), GridQuery>::value,
              "exact const grid rectangle contract");

static unsigned checks;
[[noreturn]] static void unexpected(const char* message)
{
    std::fprintf(stderr, "FAIL %s\n", message); std::fflush(stderr); std::abort();
}
static void require(bool condition, const char* message) { ++checks; if (!condition) unexpected(message); }
static unsigned bits(float value) { unsigned result; std::memcpy(&result, &value, 4); return result; }
static float fromBits(unsigned value) { float result; std::memcpy(&result, &value, 4); return result; }
static std::string mode;
static IAIInformationGrid* initial[6];
static IAIInformationGrid* replacement;
static std::vector<IAIInformationGrid*> *worldA, *worldB;
static Vec2 *inputMin, *inputMax;
struct Call {
    int identity;
    std::vector<unsigned>* output;
    const Vec2 *minAddress, *maxAddress;
    unsigned minX, minY, maxX, maxY;
    IAIInformationGridPredicate* predicate;
};
static std::vector<Call> calls;
static void observeGrid(int identity, std::vector<unsigned>& out, const Vec2& min,
                        const Vec2& max, IAIInformationGridPredicate* predicate)
{
    calls.push_back({identity, &out, &min, &max, bits(min.x), bits(min.y), bits(max.x), bits(max.y), predicate});
    out.push_back(static_cast<unsigned>(identity + 100));
    if (calls.size() == 1) {
        if (mode == "reload_grid") {
            auto prior = worldA->data();
            worldA->reserve(worldA->capacity() + 17);
            require(worldA->data() != prior, "genuine native grid-vector relocation");
            (*worldA)[2] = replacement;
        }
        if (mode == "borrowed_corners") { *inputMin = {21,22}; *inputMax = {-11,-12}; }
    }
}
#include "rectangle_query_mocks.inl"

namespace dice { namespace hfe { namespace ai {
// [CHOSEN, not recovered] Real native STL lifetimes, without registry or grid ownership.
IAIInformationEnvironment::IAIInformationEnvironment() {}
IAIInformationEnvironment::~IAIInformationEnvironment() {}
IAIInformationEnvironmentModifiers::IAIInformationEnvironmentModifiers() {}
IAIInformationEnvironmentModifiers::~IAIInformationEnvironmentModifiers() {}
BFInformationEnvironment::BFInformationEnvironment()
{
    m_gridsA.assign(initial, initial + 3); m_gridsB.assign(initial + 3, initial + 6);
    worldA = &m_gridsA; worldB = &m_gridsB;
}
BFInformationEnvironment::~BFInformationEnvironment() {}
Information::Information(IAIObject*, double) : m_physical(nullptr) {}
Information::~Information() {}
int Information::getInformationSort() { unexpected("Information::getInformationSort"); }
} } }

static std::vector<int> predicateTrace;
class Predicate : public IAIInformationGridPredicate {
public:
    bool includeInformation(Information* value) const override
    {
        if (mode.find("real_") != 0) unexpected("wrapper invoked predicate directly");
        const int id = static_cast<ObservedInformation*>(value)->identity;
        predicateTrace.push_back(id);
        return id != 2;
    }
};

static void forwarding()
{
    ObservedGrid a0(0), a1(1), a2(2), b0(3), b1(4), b2(5), changed(6);
    IAIInformationGrid* setup[] = {&a0,&a1,&a2,&b0,&b1,&b2};
    for (int i=0;i<6;++i) initial[i] = setup[i];
    replacement = &changed;
    BFInformationEnvironment environment;
    const IAIInformationEnvironment& view = environment;
    Predicate predicate;
    Vec2 min{9,8}, max{-3,-4}; inputMin=&min; inputMax=&max;
    if (mode == "nonfinite") { min = {fromBits(0x80000000),fromBits(0x7fc12345)}; max={fromBits(0xff800000),fromBits(0x7f800000)}; }
    const Vec2 beforeMin=min, beforeMax=max;
    const bool which = mode == "world_b" || mode == "side_zero";
    const int side = mode == "side_zero" ? 0 : 2;
    IAIInformationGridPredicate* keep = mode == "null_predicate" ? nullptr : &predicate;
    std::vector<unsigned> out{777};
    view.getObjectsWithinRectangle(out,min,max,side,which,keep);
    const int first = which ? 3 : 0;
    const int last = mode == "reload_grid" ? 6 : first + side;
    require(calls.size() == 2, "exactly two grid queries");
    require(calls[0].identity == first && calls[1].identity == last, "neutral then fresh requested grid");
    for (const Call& call : calls) {
        require(call.output == &out && call.predicate == keep, "output/predicate identity retained");
        require(call.minAddress == &min && call.maxAddress == &max, "both corners borrowed without swapping");
    }
    require(calls[0].minX == bits(beforeMin.x) && calls[0].minY == bits(beforeMin.y) &&
            calls[0].maxX == bits(beforeMax.x) && calls[0].maxY == bits(beforeMax.y), "first corner bits preserved");
    require(calls[1].minX == bits(min.x) && calls[1].minY == bits(min.y) &&
            calls[1].maxX == bits(max.x) && calls[1].maxY == bits(max.y), "second query sees current borrowed values");
    require(out == std::vector<unsigned>({777,static_cast<unsigned>(first+100),static_cast<unsigned>(last+100)}),
            "output appended in order including side-zero duplicate");
}

static void realGrid()
{
    InformationManager manager; informationManager = &manager;
    std::unique_ptr<AIInformationGrid> owned[6];
    for (int i=0;i<6;++i) {
        owned[i].reset(new AIInformationGrid(i%3,4,4,Vec2{0,0},Vec2{20,20}));
        initial[i]=owned[i].get();
    }
    BFInformationEnvironment environment;
    const IAIInformationEnvironment& view=environment;
    ObservedInformation one(1,Vec3{5,500,5}), two(2,Vec3{10,999,10}), outside(3,Vec3{10.25f,0,10});
    Information* records[]={&one,&two,&outside};
    unsigned handles[3]={};
    std::vector<std::pair<int,unsigned>> installed;
    bool released[3]={};
    auto add=[&](int grid,int index) {
        if (!handles[index]) handles[index]=manager.getPool()->acquire(records[index]);
        owned[grid]->addInfo(handles[index]); installed.emplace_back(grid,handles[index]);
    };
    Predicate predicate;
    std::vector<unsigned> out{777};
    const Vec2 min{4,4}, max{10,10};
    if (mode == "real_empty") {
        view.getObjectsWithinRectangle(out,min,max,2,false,&predicate);
        require(out == std::vector<unsigned>({777}), "real empty grids preserve output");
        require(predicateTrace.empty(), "real empty grids skip predicate");
    } else if (mode == "real_bounds") {
        add(0,0); add(2,1); add(2,2); add(5,0);
        view.getObjectsWithinRectangle(out,min,max,2,false,nullptr);
        require(out == std::vector<unsigned>({777,handles[0],handles[1]}), "inclusive boundary and outside rejection");
        out.assign(1,777);
        view.getObjectsWithinRectangle(out,min,max,2,true,nullptr);
        require(out == std::vector<unsigned>({777,handles[0]}), "real second-world selection");
    } else if (mode == "real_filter_stale") {
        add(0,0); add(2,0); add(2,1); add(2,2);
        manager.getPool()->release(handles[2]); released[2]=true;
        view.getObjectsWithinRectangle(out,min,max,2,false,&predicate);
        require(out == std::vector<unsigned>({777,handles[0],handles[0]}), "same handle retained across two grids");
        require(predicateTrace == std::vector<int>({1,1,2}), "predicate order and stale handle rejection");
        out.assign(1,777); predicateTrace.clear();
        view.getObjectsWithinRectangle(out,min,max,0,false,&predicate);
        require(out == std::vector<unsigned>({777,handles[0],handles[0]}), "real neutral query repeated for side zero");
        require(predicateTrace == std::vector<int>({1,1}), "visited flags reset between grid queries");
    } else unexpected("unknown real mode");
    for (const auto& item : installed) owned[item.first]->removeInfo(item.second);
    for (int i=0;i<3;++i) if (handles[i] && !released[i]) manager.getPool()->release(handles[i]);
    informationManager=nullptr;
}

int main(int argc,char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    if (argc != 2) unexpected("mode required");
    mode=argv[1];
    if (mode.find("real_") == 0) realGrid();
    else forwarding();
    std::printf("PASS %s %u checks\n",mode.c_str(),checks);
    return 0;
}
