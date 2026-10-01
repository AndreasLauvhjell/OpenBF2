// [TEMP] Native fixture lifetimes replace only untested constructors/destructors.
// Exact production getter/setter definitions live in separately compiled extraction.
// Constructors expose typed member pointers; no byte offsets, header edits or raw objects.
#include "spawn_metadata_runtime.h"
#include <windows.h>
#include <crtdbg.h>
using namespace dice::hfe;
using namespace dice::hfe::ai;
static IAIPlayer** playerSlot;
static unsigned int* idSlot;
static int* sideSlot;
static int* areaSlot;
static std::vector<IKitTemplate*>* kitVector;
static IAIPlayer* nextPlayer;
static IAIMain* nextMain;
static SAI* chosenSAI;
static bool mutation;
static int expectedGroup;
static int expectedKit;
static std::string trace;
static void observeGroup(int identity, int value)
{
    trace += "group,";
    require(identity == 1 && value == expectedGroup, "group receiver and argument");
    require(*areaSlot == 901, "area unchanged at first callback");
    if (mutation) *playerSlot = nextPlayer;
}
static void observeKit(int identity, int value)
{
    trace += "kit,";
    require(identity == (mutation ? 2 : 1) && value == expectedKit, "fresh player after group callback");
    require(*areaSlot == 901, "area unchanged at kit callback");
    if (mutation) { *sideSlot = 7; IAIMain::m_instance = nextMain; }
}
static SAI* observeSAI(int identity, int side)
{
    trace += "sai,";
    require(identity == (mutation ? 2 : 1), "fresh IAIMain after kit callback");
    require(side == (mutation ? 7 : 2), "fresh side after kit callback");
    require(*areaSlot == 901, "area unchanged at SAI lookup");
    if (mutation) { *idSlot = 501; *sideSlot = 9; }
    return chosenSAI;
}
#include "metadata_collaborators.inl"

namespace dice { namespace hfe { namespace ai {
Bot::~Bot() {}
BotMain::BotMain(IAIPlayer* player, unsigned int id, float*, float, int, BotOrder*,
    BotMemory*, PlanInterpreter*, int side, UrgencyGenerator*, UrgencyGenerator**)
    : m_botId(id), m_side(side), m_spawnToStrategicAreaId(901), m_player(player)
{
    playerSlot = &m_player; idSlot = &m_botId;
    sideSlot = &m_side; areaSlot = &m_spawnToStrategicAreaId;
}
BotMain::~BotMain() {}
IAIPlayer::~IAIPlayer() {}
IAIMain* IAIMain::m_instance = nullptr;
IAIMain::IAIMain() {}
IAIMain::~IAIMain() {}
ISAI::~ISAI() {}
SAI::SAI(int side, int, int, Vec2, Vec2) : m_side(side) {}
SAI::~SAI() {}
void SAI::setNewBotKit(int id, int kit)
{
    trace += "notify,";
    require(this == chosenSAI, "retained SAI returned by lookup");
    require(id == (mutation ? 501 : 37) && kit == expectedKit, "bot ID read after lookup and kit preserved");
    require(*areaSlot == 901, "area stored only after notification");
    *areaSlot = 902;
}
IKitTemplateManager::IKitTemplateManager() {}
IKitTemplateManager::~IKitTemplateManager() {}
KitTemplateManager::KitTemplateManager() : m_active(m_byName.end()), m_byKitType(13, nullptr) { kitVector = &m_byKitType; }
KitTemplateManager::~KitTemplateManager() {}
} } }

int main(int argc, char** argv)
{
    static_assert(sizeof(void*) == 4, "x86 only");
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    require(argc == 2, "mode");
    const std::string mode(argv[1]);
    if (mode == "kit_reference")
    {
        KitTemplateManager manager;
        const IKitTemplateManager& query = manager;
        const std::vector<IKitTemplate*>& result = query.getAllTemplates();
        require(&result == kitVector && result.size() == 13, "borrowed original vector identity");
        kitVector->clear();
        require(result.empty() && &query.getAllTemplates() == kitVector, "live empty borrowed vector");
        kitVector->resize(19, nullptr);
        require(result.size() == 19 && &query.getAllTemplates() == &result, "no copied or filtered result");
    }
    else
    {
        PlayerObserver first, second;
        first.identity = 1; second.identity = 2;
        BotMain bot(&first, 37, nullptr, 0, 0, nullptr, nullptr, nullptr, 2, nullptr, nullptr);
        Bot& dispatch = bot;
        if (mode == "spawn_point")
        {
            for (int value : {INT_MIN, -1, 0, 1, 255, INT_MAX})
            {
                first.group = value;
                require(dispatch.getSpawnPoint() == value, "unchanged signed group getter");
            }
            *playerSlot = &second; second.group = -83;
            require(dispatch.getSpawnPoint() == -83 && first.getCalls == 6 && second.getCalls == 1,
                    "fresh player and exactly one query per call");
        }
        else
        {
            require(mode == "two" || mode == "three" || mode == "two_mutation" || mode == "three_mutation", "known setter mode");
            mutation = mode.find("mutation") != std::string::npos;
            const bool three = mode.find("three") == 0;
            MainObserver original, replacement;
            original.identity = 1; replacement.identity = 2;
            SAI sai(2, 0, 0, Vec2(), Vec2());
            IAIMain::m_instance = &original;
            nextMain = &replacement; nextPlayer = &second; chosenSAI = &sai;
            expectedGroup = mutation ? INT_MIN : 17;
            expectedKit = mutation ? -13 : 6;
            if (three) dispatch.setSpawnTarget(-313, expectedGroup, expectedKit);
            else dispatch.setSpawnTarget(expectedGroup, expectedKit);
            require(trace == "group,kit,sai,notify,", "exact callback sequence");
            require(*areaSlot == (three ? -313 : -1), "final area store overwrites callback mutation");
            require(first.getCalls == 0 && second.getCalls == 0, "setter adds no getter calls");
            IAIMain::m_instance = nullptr;
        }
    }
    std::printf("PASS %s\n", mode.c_str());
    return 0;
}
