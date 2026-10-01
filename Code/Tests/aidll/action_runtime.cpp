// [TEMP] [CHOSEN, not recovered] Isolated production AIMain::action fixture.
// The generated TU contains the exact current body and uses this same real header.
// Lifetimes below only initialize the tested state; no production AIMain constructor,
// destructor, strategic work, bot work, physics, mine logic or server tick is claimed.
#include "action_runtime.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <limits>
#include <string>
#include <windows.h>
#include <crtdbg.h>

using namespace dice::hfe;
using namespace dice::hfe::ai;
static_assert(sizeof(void*) == 4, "MSVC x86 fixture only");
[[noreturn]] static void unexpected(const char* name)
{
    std::fprintf(stderr, "UNEXPECTED action collaborator: %s\n", name);
    std::fflush(stderr);
    std::abort();
}
static void require(bool condition, const char* message)
{
    if (!condition) { std::fprintf(stderr, "FAIL %s\n", message); std::exit(1); }
}
static std::string calls;
static void trace(const std::string& name) { if (!calls.empty()) calls += ','; calls += name; }
static double fromBits(std::uint64_t bits) { double value; std::memcpy(&value, &bits, 8); return value; }
static std::uint64_t bitsOf(double value) { std::uint64_t bits; std::memcpy(&bits, &value, 8); return bits; }
static double systemQuotient = 0.25, componentQuotient = 0.125, frameIncrease = 0.5;
static double updateTimes[] = {10.0, 10.0625, std::numeric_limits<double>::quiet_NaN()};
static double gameTime = 42.0, wantedRemainder = 0.0625;
static double wantedSaiBudget;
static bool enableSai, changeTimer, changeManager, changeCursor, appendDuringDrain;
static unsigned initialCursor, wantedCursor;
static unsigned* cursor;
static std::list<Bot*>* pending;
static BotManager* selectedBotManager;
static SAI* selectedSai[3];
static IBFTimer* otherTimer;
static SquadManager* firstManager;
static SquadManager* secondManager;
static Bot* appendedBot;
static Bot* originalBot;
static unsigned updates, adds, botCalls, squadCalls, saiCalls;

static double observeFrame(int identity)
{
    require(identity == 0, "initial global timer");
    trace("A.frame");
    if (changeTimer) CommonSingletonServer::m_iBFTimer = otherTimer;
    return frameIncrease;
}
static double observeUpdate(int identity)
{
    require(identity == 0 && updates < 3, "cached timer used for exactly three update reads");
    trace("A.update" + std::to_string(updates));
    return updateTimes[updates++];
}
static double observeGame(int identity)
{
    require(identity == (changeTimer ? 1 : 0), "game time uses freshly loaded global timer");
    trace(identity ? "B.game" : "A.game");
    require(pending->empty(), "pending list cleared before squad update");
    if (changeManager) SquadManager::m_instance = secondManager;
    return gameTime;
}
static int observeSides()
{
    trace("sides");
    require(enableSai && saiCalls == 1, "side count queried after SAI update only");
    if (changeCursor) *cursor = 1;
    return 2;
}
static void observeCollision()
{
    trace("collision");
    require(squadCalls == 1 && *cursor == wantedCursor, "squad update and cursor advance precede collision");
}

#include "action_runtime_mocks.inl"

namespace dice { namespace hfe { namespace ai {
// All following lifetimes/callee bodies are chosen fixture scaffolding, not recovery.
IAIMain* IAIMain::m_instance = nullptr;
IAIMain::IAIMain() { m_instance = this; }
IAIMain::~IAIMain() { m_instance = nullptr; }
AIMain::AIMain()
    : m_aiSystemQuotient(systemQuotient), m_componentQuotient(componentQuotient),
      m_initialised(false), m_saiEnabled(enableSai), m_componentCursor(initialCursor),
      m_botManager(selectedBotManager)
{
    for (unsigned i = 0; i < 3; ++i) m_sai[i] = selectedSai[i];
    cursor = &m_componentCursor;
    pending = &m_pendingBots;
}
AIMain::~AIMain() {}
IBFTimer::IBFTimer() {}
IBFTimer::~IBFTimer() {}
IAISettings::IAISettings() {}
IAISettings::~IAISettings() {}
IBFCollisionHandler::IBFCollisionHandler() {}
IBFCollisionHandler::~IBFCollisionHandler() {}
IAIMineManager::IAIMineManager() {}
IAIMineManager::~IAIMineManager() {}
IAIObjectEnvironment::IAIObjectEnvironment() {}
IAIObjectEnvironment::~IAIObjectEnvironment() {}
Bot::~Bot() {}
int Bot::getBotType() const { unexpected("Bot::getBotType"); }
BAPContext* Bot::getScriptContext() { unexpected("Bot::getScriptContext"); }
IBotManager::IBotManager() {}
IBotManager::~IBotManager() {}
IBotManagerModifier::IBotManagerModifier() {}
IBotManagerModifier::~IBotManagerModifier() {}
BotManager::BotManager() {}
BotManager::~BotManager() {}
void BotManager::action(const double& budget)
{
    trace("bots");
    require(this == selectedBotManager && botCalls++ == 0, "AIMain-owned BotManager receives one slice");
    require(budget == wantedRemainder, "remaining budget is deadline minus cached timer, without clamping");
    require(updates == 2, "bot work follows second update-time read");
}
ISAI::~ISAI() {}
SAI::SAI(int side, int, int, Vec2, Vec2) : m_side(side) {}
SAI::~SAI() {}
void SAI::update(double slice)
{
    trace("sai" + std::to_string(m_side));
    require(enableSai && saiCalls++ == 0 && m_side == static_cast<int>(initialCursor + 1), "one selected strategic side");
    require(pending->empty(), "queue clears before strategic update");
    require(bitsOf(slice) == bitsOf(wantedSaiBudget), "PE strategic multiplication order and budget");
}
void SAI::addBot(Bot* bot)
{
    trace("add" + std::to_string(m_side));
    require(m_side == 1 && (bot == originalBot || bot == appendedBot), "queue side-index dispatch");
    require(pending->size() == (adds == 0 ? 3u : 4u), "entries stay queued until entire drain completes");
    ++adds;
    if (appendDuringDrain) { pending->push_back(appendedBot); appendDuringDrain = false; }
}
SquadManager* SquadManager::m_instance = nullptr;
SquadManager::SquadManager() {}
void SquadManager::update(double time)
{
    trace(this == firstManager ? "squad1" : "squad2");
    require(this == firstManager && squadCalls++ == 0, "PE captures squad manager before game-time callback");
    require(time == gameTime, "game time forwarded unchanged");
}
void* CommonSingletonServer::m_iBFTimer = nullptr;
void* CommonSingletonServer::m_iaiSettings = nullptr;
void* CommonSingletonServer::m_iBFCollisionHandler = nullptr;
void* CommonSingletonServer::m_iAIMineManager = nullptr;
void* CommonSingletonServer::m_iaiObjectEnvironment = nullptr;
} } }

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _CrtSetReportMode(_CRT_ASSERT, 0);
    require(argc == 2, "one fixture mode");
    const std::string mode = argv[1];
    require(mode == "empty_disabled" || mode == "queue_disabled" || mode == "sai_wrap" ||
            mode == "cursor_after_count" || mode == "cached_dependencies" ||
            mode == "negative_remainder" || mode == "sai_multiply_order", "known mode");
    enableSai = mode == "sai_wrap" || mode == "cursor_after_count" || mode == "sai_multiply_order";
    initialCursor = mode == "sai_wrap" ? 1 : 0;
    changeCursor = mode == "cursor_after_count";
    wantedCursor = enableSai ? (changeCursor ? 0 : (initialCursor + 1) % 2) : initialCursor;
    changeTimer = changeManager = mode == "cached_dependencies";
    if (mode == "negative_remainder") { updateTimes[1] = 10.5; wantedRemainder = -0.375; }
    if (mode == "sai_multiply_order")
    {
        frameIncrease = fromBits(0x3fce4fdb63168238ULL);
        componentQuotient = fromBits(0x3fbaa3dd37a1c1b4ULL);
        systemQuotient = 1.0;
        wantedSaiBudget = fromBits(0x3f97f9193e1c348fULL);
        wantedRemainder = (frameIncrease + updateTimes[0]) - updateTimes[1];
    }
    else wantedSaiBudget = 0.01484375; // .5 * .25 * .125 * .95, in PE order.
    ObservedTimer timerA, timerB;
    timerB.identity = 1; otherTimer = &timerB;
    ObservedSettings settings;
    ObservedCollision collision;
    ObservedMines mines;
    ObservedEnvironment environment;
    BotManager bots;
    selectedBotManager = &bots;
    SquadManager squad1, squad2;
    firstManager = &squad1; secondManager = &squad2;
    SAI sai1(1, 0, 0, Vec2{}, Vec2{}), sai2(2, 0, 0, Vec2{}, Vec2{});
    if (enableSai) { selectedSai[1] = &sai1; selectedSai[2] = &sai2; }
    if (mode == "queue_disabled") selectedSai[1] = &sai1;
    CommonSingletonServer::m_iBFTimer = &timerA;
    CommonSingletonServer::m_iaiSettings = &settings;
    CommonSingletonServer::m_iBFCollisionHandler = &collision;
    CommonSingletonServer::m_iAIMineManager = &mines;
    CommonSingletonServer::m_iaiObjectEnvironment = &environment;
    SquadManager::m_instance = &squad1;
    ObservedBot bot1, bot2, bot0, bot3;
    bot1.side = bot3.side = 1; bot2.side = 2;
    bot1.identity = 1; bot2.identity = 2; bot0.identity = 0; bot3.identity = 3;
    originalBot = &bot1; appendedBot = &bot3;
    AIMain main;
    if (mode == "queue_disabled")
    {
        pending->push_back(&bot1); pending->push_back(&bot2); pending->push_back(&bot0);
        appendDuringDrain = true;
    }
    require(main.action(), "action returns true even while fixture initialised flag is false");
    std::string expected = "A.frame,A.update0,";
    if (mode == "queue_disabled") expected += "side1,add1,side2,side0,side3,add1,";
    if (enableSai) expected += "sai" + std::to_string(initialCursor + 1) + ",sides,";
    expected += changeTimer ? "B.game," : "A.game,";
    expected += "squad1,collision,A.update1,bots,mines,damage,A.update2";
    if (calls != expected) { std::fprintf(stderr, "Expected %s\nObserved %s\n", expected.c_str(), calls.c_str()); return 1; }
    require(updates == 3 && botCalls == 1 && squadCalls == 1 && pending->empty(), "complete tick and final timer read");
    require(*cursor == wantedCursor && saiCalls == (enableSai ? 1u : 0u), "SAI gate and round-robin state");
    require(adds == (mode == "queue_disabled" ? 2u : 0u), "all queued bots visited, only nonnull side dispatched");
    std::printf("PASS %s\n", argv[1]);
    return 0;
}
