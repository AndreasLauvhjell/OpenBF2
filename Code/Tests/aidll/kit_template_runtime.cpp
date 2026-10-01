// [TEMP] Whole production KitTemplate.cpp and real header; no production body
// substitutions. The only collaborator is a test allocation/deallocation observer.
// Raw slot dispatch below validates this compiler's genuine native x86 vtable;
// it is not a raw bridge in production and does not instantiate retail bytes.
#include "../../BF2/AI/AIInterface/BFKnowledge/KitTemplate.h"
#include <windows.h>
#include <crtdbg.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <climits>

using namespace dice::hfe;
using namespace dice::hfe::ai;
static_assert(sizeof(void*) == 4, "strict x86 fixture only");
static_assert(sizeof(KitTemplate) == 20, "native pointer-only layout");

static bool observe;
static void* allocations[8];
static size_t allocationSizes[8];
static void* releases[8];
static int allocationCount;
static int releaseCount;
static int checks;

static void check(bool ok, const char* message)
{
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAIL %s\n", message); std::exit(1); }
}

void* operator new[](size_t size)
{
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    if (observe)
    {
        if (allocationCount >= 8) std::abort();
        allocations[allocationCount] = p;
        allocationSizes[allocationCount++] = size;
    }
    return p;
}

void operator delete[](void* p) noexcept
{
    if (observe)
    {
        if (releaseCount >= 8) std::abort();
        releases[releaseCount++] = p;
    }
    std::free(p);
}

void operator delete[](void* p, size_t) noexcept { ::operator delete[](p); }

static void beginObservation()
{
    allocationCount = releaseCount = 0;
    observe = true;
}

static void constructorCase()
{
    beginObservation();
    {
        KitTemplate kit(static_cast<KitType>(-19));
        const IKitTemplate& view = kit;
        check(allocationCount == 2, "two allocations");
        check(allocationSizes[0] == 8 && allocationSizes[1] == 24,
              "strategic then battle allocation sizes");
        check(view.getStrategicStrengths() == allocations[0], "strategic borrowed allocation");
        check(view.getBattleStrengths() == allocations[1], "battle borrowed allocation");
        check(static_cast<int>(view.getType()) == -19, "full signed kit type");
        for (int i = 0; i < 2; ++i) check(view.getStrategicStrength(i) == 0, "strategic zero");
        for (int i = 0; i < 6; ++i) check(view.getBattleStrength(i) == 0, "battle zero");
    }
    check(releaseCount == 2, "two owned arrays released");
    check(releases[0] == allocations[0] && releases[1] == allocations[1], "release order");
    observe = false;
}

static void strengthCase()
{
    KitTemplate kit(static_cast<KitType>(12));
    IKitTemplate* view = &kit;
    const int* strategic = view->getStrategicStrengths();
    const int* battle = view->getBattleStrengths();
    const int values[6] = { INT_MIN, -1, 0, 1, 16777217, INT_MAX };
    view->setStrategicStrength(0, INT_MIN);
    view->setStrategicStrength(1, INT_MAX);
    for (int i = 0; i < 6; ++i) view->setBattleStrength(i, values[i]);
    check(view->getStrategicStrengths() == strategic && view->getBattleStrengths() == battle,
          "stable borrowed addresses after setters");
    check(strategic[0] == INT_MIN && strategic[1] == INT_MAX, "strategic integer writes");
    check(view->getStrategicStrength(0) == INT_MIN && view->getStrategicStrength(1) == INT_MAX,
          "strategic indexed reads");
    for (int i = 0; i < 6; ++i)
    {
        check(battle[i] == values[i], "battle borrowed integer writes");
        check(view->getBattleStrength(i) == values[i], "battle indexed integer reads");
    }
    check(static_cast<int>(view->getType()) == 12, "strength stores preserve type");
}

static void independentCase()
{
    KitTemplate first(static_cast<KitType>(2));
    KitTemplate second(static_cast<KitType>(3));
    first.setBattleStrength(5, 101);
    second.setStrategicStrength(1, -303);
    check(first.getStrategicStrengths() != second.getStrategicStrengths(), "strategic ownership distinct");
    check(first.getBattleStrengths() != second.getBattleStrengths(), "battle ownership distinct");
    check(first.getBattleStrength(5) == 101 && second.getBattleStrength(5) == 0, "battle state isolated");
    check(first.getStrategicStrength(1) == 0 && second.getStrategicStrength(1) == -303,
          "strategic state isolated");
}

static int derivedDestructions;
class DerivedKit : public KitTemplate
{
public:
    DerivedKit() : KitTemplate(static_cast<KitType>(7)) {}
    ~DerivedKit() override { ++derivedDestructions; }
};

static void destructionCase()
{
    beginObservation();
    IKitTemplate* kit = new DerivedKit;
    const int* strategic = kit->getStrategicStrengths();
    const int* battle = kit->getBattleStrengths();
    delete kit;
    check(derivedDestructions == 1, "virtual destructor reaches derived lifetime");
    check(allocationCount == 2 && releaseCount == 2, "virtual deletion releases both arrays");
    check(releases[0] == strategic && releases[1] == battle, "virtual destruction array order");
    observe = false;
}

static void slotsCase()
{
    KitTemplate kit(static_cast<KitType>(0x12345678));
    IKitTemplate* view = &kit;
    void** slots = *reinterpret_cast<void***>(view);
    typedef void (__thiscall *Setter)(IKitTemplate*, int, int);
    typedef const int* (__thiscall *ArrayGetter)(const IKitTemplate*);
    typedef int (__thiscall *IndexGetter)(const IKitTemplate*, int);
    typedef KitType (__thiscall *TypeGetter)(const IKitTemplate*);
    reinterpret_cast<Setter>(slots[0])(view, 1, -51);
    check(reinterpret_cast<ArrayGetter>(slots[1])(view) == view->getStrategicStrengths(), "+04 array");
    check(reinterpret_cast<IndexGetter>(slots[2])(view, 1) == -51, "+00 setter and +08 getter");
    reinterpret_cast<Setter>(slots[3])(view, 5, 16777217);
    check(reinterpret_cast<ArrayGetter>(slots[4])(view) == view->getBattleStrengths(), "+10 array");
    check(reinterpret_cast<IndexGetter>(slots[5])(view, 5) == 16777217, "+0C setter and +14 getter");
    check(static_cast<int>(reinterpret_cast<TypeGetter>(slots[6])(view)) == 0x12345678, "+18 full KitType");
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
    if (argc != 2) return 2;
    if (!std::strcmp(argv[1], "constructor")) constructorCase();
    else if (!std::strcmp(argv[1], "strengths")) strengthCase();
    else if (!std::strcmp(argv[1], "independent")) independentCase();
    else if (!std::strcmp(argv[1], "destruction")) destructionCase();
    else if (!std::strcmp(argv[1], "slots")) slotsCase();
    // KitTemplate::initFromObject is outside this fixture's coverage.
    else return 2;
    std::printf("PASS %s checks=%d\n", argv[1], checks);
    return 0;
}
