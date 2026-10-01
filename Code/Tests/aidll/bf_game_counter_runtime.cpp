// [TEMP] Exact BFGame counter and ControlPoint getter bodies, real native headers.
// Chosen native constructors/destructors omit registry, world and template work;
// typed observers model only map lookup/callback conditions. No engine lifetime,
// retail binary layout, missing-key safety or completed area update is claimed.
#include "bf_game_counter_runtime.h"
#include "counter_collaborators.inl"

namespace dice { namespace hfe {
IGameLogic* gameLogic = nullptr;
namespace ai {
IAIGame::IAIGame() {}
IAIGame::~IAIGame() {}
BFGame::BFGame() : m_isPrecaching(true) {}
BFGame::~BFGame() {}
}
namespace world {
IObject::IObject() {}
IObject::~IObject() {}
Object::Object(const IObjectTemplate*) {}
Object::~Object() {}
SimpleObject::SimpleObject(const SimpleObjectTemplate*) : Object(nullptr) {}
SimpleObject::~SimpleObject() {}
Bundle::Bundle(const BundleTemplate*) : SimpleObject(nullptr) {}
Bundle::~Bundle() {}
ControlPoint::ControlPoint(const ControlPointTemplate*) : Bundle(nullptr) {}
ControlPoint::~ControlPoint() {}
}
} }

using namespace dice::hfe;
using namespace dice::hfe::world;
using namespace dice::hfe::ai;
static std::vector<int> visits;
static unsigned int bits(float value)
{
    unsigned int result;
    std::memcpy(&result, &value, sizeof result);
    return result;
}
class ObservedPoint : public ControlPoint
{
public:
    int identity, reads = 0;
    void (*onRead)(ObservedPoint&) = nullptr;
    explicit ObservedPoint(int id) : ControlPoint(nullptr), identity(id) {}
    void store(unsigned int value) { std::memcpy(&m_takeOverCounter, &value, sizeof value); }
    float getTakeOverCounter() override
    {
        ++reads;
        visits.push_back(identity);
        if (onRead) onRead(*this);
        return ControlPoint::getTakeOverCounter();
    }
};
static LogicObserver* original;
static LogicObserver* replacement;
static ObservedPoint* changedEntry;
static void changeMap(const LogicObserver& owner)
{
    owner.points[17] = changedEntry;
    gameLogic = replacement;
}
static void changeAfterReceiver(ObservedPoint& point)
{
    original->points[17] = changedEntry;
    gameLogic = replacement;
    point.store(0xbf000000u);
}
int main(int argc, char** argv)
{
    static_assert(sizeof(void*) == 4, "MSVC x86 fixture required");
    require(argc == 2, "mode");
    BFGame adapter;
    const IAIGame& game = adapter;
    LogicObserver logic, other;
    ObservedPoint first(1), second(2), third(3);
    first.store(0x3e800000u);
    second.store(0x3f400000u);
    third.store(0x40000000u);
    logic.points[17] = &first;
    other.points[17] = &third;
    original = &logic;
    replacement = &other;
    changedEntry = &second;
    gameLogic = &logic;
    const std::string mode(argv[1]);
    if (mode == "values")
    {
        const unsigned int values[] = {0u, 0x80000000u, 0x3e800000u, 0x3f800000u,
            0xc0200000u, 0x7f7fffffu, 1u, 0x7f800000u, 0xff800000u, 0x7fc12345u};
        for (unsigned int value : values)
        {
            first.store(value);
            require(bits(game.getControlPointTakeOverCounter(17)) == value, "float bits forwarded");
        }
        require(logic.calls == 10 && first.reads == 10 && visits == std::vector<int>(10, 1), "one map and point read per call");
    }
    else if (mode == "unsigned_ids")
    {
        logic.points.clear();
        logic.points[0] = &first;
        logic.points[0x80000000u] = &second;
        logic.points[0xffffffffu] = &third;
        require(bits(game.getControlPointTakeOverCounter(0xffffffffu)) == 0x40000000u, "maximum unsigned key");
        require(bits(game.getControlPointTakeOverCounter(0x80000000u)) == 0x3f400000u, "high-bit unsigned key");
        require(bits(game.getControlPointTakeOverCounter(0)) == 0x3e800000u, "zero key");
        require(logic.calls == 3 && visits == std::vector<int>({3, 2, 1}), "exact key selects exact receiver");
    }
    else if (mode == "map_mutation")
    {
        logic.onMap = changeMap;
        require(bits(game.getControlPointTakeOverCounter(17)) == 0x3f400000u, "find sees map callback replacement");
        require(logic.calls == 1 && other.calls == 0 && visits == std::vector<int>({2}), "retained returned map after global change");
        require(bits(game.getControlPointTakeOverCounter(17)) == 0x40000000u, "next invocation reads fresh global");
        require(logic.calls == 1 && other.calls == 1 && visits == std::vector<int>({2, 3}), "exact callback sequence");
    }
    else if (mode == "receiver_capture")
    {
        first.onRead = changeAfterReceiver;
        require(bits(game.getControlPointTakeOverCounter(17)) == 0xbf000000u, "captured receiver returns current actual field");
        require(logic.points[17] == &second && first.reads == 1 && second.reads == 0, "no post-callback map entry reload");
        require(bits(game.getControlPointTakeOverCounter(17)) == 0x40000000u, "getter callback global change applies next call");
        require(logic.calls == 1 && other.calls == 1 && visits == std::vector<int>({1, 3}), "no extra dispatches");
    }
    else unexpected("unknown mode");
    std::printf("PASS %s checks=%d\n", argv[1], checks);
}
