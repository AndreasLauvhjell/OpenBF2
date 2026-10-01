// [TEMP] Adapter queries use exact production bodies, plus actual TeamScore.cpp
// and the exact existing ControlPoint::getTeam body. The observers below are
// native typed objects. Their chosen constructors/destructors intentionally omit
// registry/world/template work; this tests neither engine lifetime nor map load.
#include "bf_game_counts_runtime.h"
#include "counts_collaborators.inl"

namespace dice { namespace hfe {
IGameLogic* gameLogic = nullptr;
namespace ai {
IAIGame::IAIGame() {}
IAIGame::~IAIGame() {}
BFGame::BFGame() : m_isPrecaching(true) {}
BFGame::~BFGame() {}
}
namespace world {
IBase* scoreManager = nullptr;
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

class ObservedPoint : public ControlPoint
{
public:
    int identity;
    int reads = 0;
    void (*onRead)(ObservedPoint&) = nullptr;
    ObservedPoint(int id, int team) : ControlPoint(nullptr), identity(id) { m_team = team; }
    void setObservedTeam(int team) { m_team = team; }
    int getTeam() override
    {
        visits.push_back(identity);
        ++reads;
        if (onRead) onRead(*this);
        return ControlPoint::getTeam();
    }
};

static LogicObserver* originalLogic;
static LogicObserver* otherLogic;
static ObservedPoint* insertedPoint;
static void insertDuringRead(ObservedPoint& point)
{
    point.onRead = nullptr;
    originalLogic->points[20] = insertedPoint;
    gameLogic = otherLogic;
}
static void changeRepeatedTeam(ObservedPoint& point)
{
    point.setObservedTeam(point.reads == 1 ? 1 : 2);
}

int main(int argc, char** argv)
{
    static_assert(sizeof(void*) == 4, "MSVC x86 fixture required");
    require(argc == 2, "mode");
    const std::string mode(argv[1]);
    BFGame adapter;
    const IAIGame& game = adapter;
    if (mode == "tickets")
    {
        TeamScore first, second;
        first.setTickets(235);
        second.m_tickets = -81; // Intentional raw sentinel: adapter must not clamp.
        ScoreObserver before, after;
        before.selected = &first;
        before.replacement = &after;
        after.selected = &second;
        scoreManager = &before;
        require(game.getTickets(1) == 235, "first ticket read");
        require(game.getTickets(2) == -81, "fresh global and unmodified result");
        require(game.getTickets(-1) == -81, "adapter preserves argument bits");
        require(before.sides == std::vector<unsigned int>({1}), "first dispatch once");
        require(after.sides == std::vector<unsigned int>({2, 0xffffffffu}), "later dispatches");
        // The negative input goes only to this typed observer, not retail score arithmetic.
    }
    else if (mode == "null_logic")
    {
        gameLogic = nullptr;
        require(game.getNumberOfControlPoints(-1) == 0, "null logic guard");
    }
    else
    {
        LogicObserver logic;
        gameLogic = &logic;
        if (mode == "empty")
        {
            require(game.getNumberOfControlPoints(1) == 0, "empty map");
            require(logic.calls == 1 && visits.empty(), "one map read");
        }
        else if (mode == "teams")
        {
            ObservedPoint neutral(10, 0), first(20, 1), second(30, 2), negative(40, -1);
            logic.points[40] = &negative;
            logic.points[30] = &second;
            logic.points[10] = &neutral;
            logic.points[20] = &first;
            for (int team : {0, 1, 2, -1, 9})
            {
                visits.clear();
                require(game.getNumberOfControlPoints(team) == (team == 9 ? 0 : 1), "exact team count");
                require(visits == std::vector<int>({10, 20, 30, 40}), "map order and all entries");
            }
            require(logic.calls == 5 && first.reads == 5 && neutral.reads == 5 && second.reads == 5 && negative.reads == 5, "one live read per entry");
        }
        else if (mode == "live_map")
        {
            LogicObserver replacement;
            ObservedPoint first(10, 1), inserted(20, 2), last(30, 1);
            logic.points[10] = &first;
            logic.points[30] = &last;
            originalLogic = &logic;
            otherLogic = &replacement;
            insertedPoint = &inserted;
            first.onRead = insertDuringRead;
            require(game.getNumberOfControlPoints(1) == 2, "live map result");
            require(visits == std::vector<int>({10, 20, 30}), "inserted entry visited");
            require(logic.calls == 1 && replacement.calls == 0 && gameLogic == &replacement, "captured map survives global swap");
        }
        else if (mode == "repeated_object")
        {
            ObservedPoint repeated(10, 0);
            repeated.onRead = changeRepeatedTeam;
            logic.points[10] = &repeated;
            logic.points[20] = &repeated;
            require(game.getNumberOfControlPoints(2) == 1, "fresh team for repeated object");
            require(visits == std::vector<int>({10, 10}) && repeated.reads == 2, "no object deduplication");
        }
        else unexpected("unknown mode");
    }
    std::printf("PASS %s\n", argv[1]);
    return 0;
}
