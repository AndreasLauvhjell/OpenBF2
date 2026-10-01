// [TEMP] Exact production primary caller and small helpers; chosen native lifetimes.
// Typed callbacks observe the caller. The sibling spawn/kit selectors remain production stops.
#include "bot_spawner_runtime.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <windows.h>
#include <crtdbg.h>

using namespace dice::hfe;
using namespace dice::hfe::ai;
static_assert(sizeof(void*) == 4, "MSVC x86 only");
[[noreturn]] static void unexpected(const char* message)
{
    std::fprintf(stderr, "UNEXPECTED %s\n", message); std::fflush(stderr); std::abort();
}
static void require(bool value, const char* message) { if (!value) unexpected(message); }
static std::string mode, calls;
static void trace(const std::string& event) { if (!calls.empty()) calls += ','; calls += event; }
static std::string number(int value) { return std::to_string(value); }
static float fromBits(unsigned bits) { float value; std::memcpy(&value, &bits, 4); return value; }
static unsigned undoRight(unsigned value, unsigned shift)
{
    unsigned result = value; for (int i = 0; i < 8; ++i) result = value ^ (result >> shift); return result;
}
static unsigned undoLeft(unsigned value, unsigned shift, unsigned mask)
{
    unsigned result = value; for (int i = 0; i < 8; ++i) result = value ^ ((result << shift) & mask); return result;
}
static unsigned stateFor(unsigned value)
{
    return undoRight(undoLeft(undoLeft(undoRight(value,18),15,0xDB8B0000u),7,0x2B5B2500u),11);
}
static unsigned rawWords[2];
static int *randomIndex[2], randomConstruction;
static AIRandom* alternateRandom;
static IAIPlayer* player;
static Bot* bot;
static IAIGame *games[3];
static IAIMain* alternateMain;
static SAI* sais[2];
static std::map<int,int>* counts[2];
static int saiConstruction, managerConstruction;
static AIStrategicObjectManager* managers[2];
static std::vector<AIStrategicObject*>* objects[2];
static AIStrategicObject* strategic[5];
static std::list<int> groups;
static int groupValue, sideValue = 2, priorCalls;
static int selectedGroup = -1, selectedKit = 6, selectorGroup = 37;
static AIStrategicObject* selectorObject;
static AIStrategicObject* expectedKitObject;
static Vec3 expectedClosest = {};
static bool expectClosest, directClosest, mutateClosest;
static int expectedClosestSide = 2;
static float gravityValues[5] = {0, 2, 3, 4, 5};
static float arrays[4][6];
struct Point { bool eligible = false; Vec3 first = {}; Vec3 second = {}; int reads = 0; };
static Point points[64];
static int targetArity, targetObject, targetGroup, targetKit;
static int observedSelectorCalls, observedKitCalls, addedCount;

static int observeSide()
{
    trace("side");
    if (mode == "cached_context") CommonSingletonServer::m_iaiGame = games[1];
    return sideValue;
}
static IAIPlayer* observePlayer()
{
    trace("player");
    if (mode == "cached_context") { CommonSingletonServer::m_iaiGame = games[2]; sideValue = 1; }
    return player;
}
static int observePlayerGroup() { trace("group"); return groupValue; }
static const std::vector<IKitTemplate*>& observeTemplates()
{
    trace("templates"); static std::vector<IKitTemplate*> unused;
    if (mode == "cached_context") IAIMain::m_instance = alternateMain;
    return unused;
}
static SAI* observeSAI(int identity, int side)
{
    trace("main" + number(identity)); require(side == 2, "initial bot side passed to getSAI");
    if (mode == "cached_context") AIRandom::m_instance = alternateRandom;
    return sais[identity];
}
static int observePriorPoint()
{
    trace("prior"); ++priorCalls;
    if (priorCalls == 2 && (mode == "prior_eligible" || mode == "cached_context")) return 19;
    return 9;
}
static void checkGame(int identity, int side)
{
    require(identity == (mode == "cached_context" ? 1 : 0), "game captured after side, before player callbacks");
    require(side == 2, "cached side reused");
    require(*randomIndex[mode == "cached_context" ? 1 : 0] == 1, "exactly one actual random draw precedes world query");
}
static bool observeEligible(int identity, int side, int group)
{
    trace("eligible" + number(identity) + ":" + number(side) + ":" + number(group)); checkGame(identity, side);
    require(group >= 0 && group < 64, "fixture group range");
    if (mode == "live_list" && group == 10) groups.push_back(11);
    return points[group].eligible;
}
static const std::list<int>& observeList(int identity, int side)
{
    trace("list" + number(identity) + ":" + number(side)); checkGame(identity, side); return groups;
}
static const Vec3& observeSpawnPosition(int identity, int group)
{
    trace("position" + number(identity) + ":" + number(group)); checkGame(identity, 2);
    require(group >= 0 && group < 64, "position group range");
    Point& point = points[group];
    if (mode == "target_reference" && group == 10 && point.reads == 0)
        strategic[0]->m_position[1] = Vec2{10.0f, 0.0f};
    ++point.reads;
    return point.reads == 1 ? point.first : point.second;
}
static const Vec2& observeObjectPosition(int id, int side, const Vec2& value)
{
    trace("object" + number(id) + ":" + number(side));
    require(id == 0 && side == 2, "prior object gets captured side once"); return value;
}
static void observeAdd(int identity, int group)
{
    trace("add" + number(identity) + ":" + number(group)); ++addedCount;
    require(identity == (mode == "cached_context" ? 1 : 0), "captured SAI receiver");
    require(group == selectedGroup, "counter uses selected/current prior group");
}
static void observeAfterAdd(int identity, int group)
{
    require(counts[identity]->at(group) == 4, "actual counter increments pre-existing entry exactly once");
    if (mode == "second_position") AIStrategicObjectManager::m_instance = managers[1];
}
static float observeGravity(int id, const Vec3& position, int side)
{
    trace("gravity" + number(id));
    require(side == expectedClosestSide && std::memcmp(&position, &expectedClosest, sizeof(Vec3)) == 0,
            "closest receives copied second-query XYZ and captured side");
    if (!directClosest) require(addedCount == 1 && expectClosest, "spawn counter precedes closest scan");
    if (mutateClosest && id == 0) {
        (*objects[0])[0] = strategic[2]; objects[0]->push_back(strategic[3]); mutateClosest = false;
    }
    return gravityValues[id];
}
static void observeTarget(int object, int group, int kit, int arity)
{
    trace("target" + number(arity) + ":" + number(object) + ":" + number(group) + ":" + number(kit));
    require(targetArity == 0, "only one target mutation");
    targetArity = arity; targetObject = object; targetGroup = group; targetKit = kit;
}

#include "bot_spawner_mocks.inl"

namespace dice { namespace hfe { namespace ai {
// [TEMP] Chosen genuine C++ lifetimes; all declared containers are constructed normally.
Bot::~Bot() {}
int Bot::getBotType() const { unexpected("Bot::getBotType"); }
BAPContext* Bot::getScriptContext() { unexpected("Bot::getScriptContext"); }
IAIPlayer::~IAIPlayer() {}
IAIGame::IAIGame() {}
IAIGame::~IAIGame() {}
IKitTemplateManager::IKitTemplateManager() {}
IKitTemplateManager::~IKitTemplateManager() {}
IAIMain* IAIMain::m_instance = nullptr;
IAIMain::IAIMain() {}
IAIMain::~IAIMain() {}
ISAI::~ISAI() {}
SAI::SAI(int side, int, int, Vec2, Vec2) : m_side(side) { counts[saiConstruction++] = &m_map1D8; }
SAI::~SAI() {}
void* CommonSingletonServer::m_iaiGame = nullptr;
void* CommonSingletonServer::m_iKitTemplateManager = nullptr;
AIRandom* AIRandom::m_instance = nullptr;
AIRandom::AIRandom() : m_seed(0), m_index(0)
{
    const int id = randomConstruction++; randomIndex[id] = &m_index;
    for (unsigned& value : m_state) value = 0;
    m_state[0] = stateFor(rawWords[id]);
}
AIRandom::~AIRandom() {}
unsigned int AIRandom::generateNumbers() const { unexpected("random refill outside one-draw fixture"); }
AIStrategicObject::AIStrategicObject(float, float)
{
    m_position[0] = m_position[1] = Vec2{0,0}; m_index = -1;
}
AIStrategicObject::~AIStrategicObject() {}
AIStrategicObjectManager* AIStrategicObjectManager::m_instance = nullptr;
AIStrategicObjectManager::AIStrategicObjectManager()
{
    const int id = managerConstruction++; objects[id] = &m_strategicObjects;
    objects[id]->reserve(8);
}
AIStrategicObjectManager::~AIStrategicObjectManager() {}

// Chosen receiving contracts for the two still-unrecovered static helper bodies.
int BotSpawner::findSuitableSpawnGroup(int side, AIStrategicObject*& object, Bot* argument)
{
    trace("sibling"); ++observedSelectorCalls;
    require(side == 2 && argument == bot && object == nullptr, "sibling selector arguments and initial null reference");
    object = selectorObject;
    return selectorGroup;
}
int BotSpawner::findSuitableKit(const float* a, const float* b, const float* c, const float* d,
                              AIStrategicObject* object, int side, Bot* argument)
{
    trace("kit"); ++observedKitCalls;
    require(a == arrays[0] && b == arrays[1] && c == arrays[2] && d == arrays[3] &&
            object == expectedKitObject && side == 2 && argument == bot, "kit array/object/side/Bot argument order");
    if (mode == "kit_index_mutation") object->m_index = 88;
    return selectedKit;
}
} } }

static void point(int group, Vec3 position)
{
    groups.push_back(group); points[group].eligible = true;
    points[group].first = points[group].second = position;
}
static void expectTrace(const std::string& expected)
{
    if (calls != expected) {
        std::fprintf(stderr, "FAIL trace\nexpected: %s\nactual:   %s\n", expected.c_str(), calls.c_str()); std::exit(2);
    }
}
static std::string candidate(int group, bool eligible, bool improves)
{
    const std::string suffix = ":" + number(group);
    return ",eligible0:2" + suffix + (eligible ? ",position0" + suffix : "") +
           (improves ? ",position0" + suffix : "");
}

int main(int argc, char** argv)
{
    require(argc == 2, "one mode"); mode = argv[1];
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    const bool skipPrior = mode == "zero_empty" || mode == "negative_empty" || mode == "fallback_null" || mode == "fallback_object";
    rawWords[0] = skipPrior || mode == "cached_context" ? 0 : mode == "random_equal" ? 0xCCCCCD00u : mode == "random_above" ? 0xCCCCCD01u : 0xffffffffu;
    rawWords[1] = 0xffffffffu;
    AIRandom random, otherRandom; AIRandom::m_instance = &random; alternateRandom = &otherRandom;
    ObservedBot botObject; ObservedPlayer playerObject; bot = &botObject; player = &playerObject;
    ObservedGame gameObjects[3];
    for (int i = 0; i < 3; ++i) { gameObjects[i].identity = i; games[i] = &gameObjects[i]; }
    CommonSingletonServer::m_iaiGame = games[0];
    ObservedKitManager kitManager; CommonSingletonServer::m_iKitTemplateManager = &kitManager;
    ObservedMain mainObject, otherMain; otherMain.identity = 1;
    IAIMain::m_instance = &mainObject; alternateMain = &otherMain;
    ObservedSAI firstSAI, secondSAI; secondSAI.identity = 1; sais[0] = &firstSAI; sais[1] = &secondSAI;
    AIStrategicObjectManager firstManager, secondManager; managers[0] = &firstManager; managers[1] = &secondManager;
    AIStrategicObjectManager::m_instance = &firstManager;
    ObservedObject objectInstances[5];
    for (int i = 0; i < 5; ++i) { objectInstances[i].identity = i; objectInstances[i].m_index = 20 + i; strategic[i] = &objectInstances[i]; }
    objects[0]->assign({strategic[0],strategic[1]}); objects[1]->assign({strategic[2],strategic[3]});
    for (int a = 0; a < 4; ++a) for (int c = 0; c < 6; ++c) arrays[a][c] = float(10*a+c);
    const std::string prefix = "side,player,group,templates,main0";
    const std::string prior = ",prior,eligible0:2:9";
    const std::string list = ",list0:2";
    const std::string object = ",object0:2";
    const std::string refusal = ",target2:-1:-1:13";
    std::string expected;
    bool expectedReturn = true;
    int wantedArity = 0, wantedObject = -1, wantedGroup = -1, wantedKit = 13;
    int priorObjectId = 0;

    if (mode == "counter_insert") {
        firstSAI.SAI::addSpawnToPoint(-7);
        firstSAI.SAI::addSpawnToPoint(0);
        firstSAI.SAI::addSpawnToPoint(-7);
        require(counts[0]->size() == 2 && counts[0]->at(-7) == 2 && counts[0]->at(0) == 1 && counts[1]->empty(),
                "actual counter inserts one, increments existing entry, preserves signed keys and receiver");
        expectTrace("");
        std::printf("PASS %s\n", mode.c_str()); return 0;
    }
    if (mode == "closest_thresholds" || mode == "closest_mutation") {
        directClosest = true; expectedClosestSide = 2; expectedClosest = Vec3{2,3,4};
        if (mode == "closest_thresholds") {
            objects[0]->clear();
            require(firstManager.getClosestObject(expectedClosest,2) == nullptr, "empty closest roster returns null without callbacks");
            expectTrace("");
            objects[0]->assign({strategic[0],strategic[1],strategic[2],strategic[3]});
            gravityValues[0] = -1.0f; gravityValues[1] = -2.0f;
            gravityValues[2] = std::numeric_limits<float>::quiet_NaN(); gravityValues[3] = -1.0f;
            require(firstManager.getClosestObject(expectedClosest,2) == nullptr, "ordered strict gravity threshold excludes equal/below/NaN");
            expectTrace("gravity0,gravity1,gravity2,gravity3"); calls.clear();
            gravityValues[0] = gravityValues[1] = 0.0f; gravityValues[3] = -0.0f;
            require(firstManager.getClosestObject(expectedClosest,2) == strategic[0], "first exact gravity tie wins");
            expectTrace("gravity0,gravity1,gravity2,gravity3");
        } else {
            objects[0]->assign({strategic[0],strategic[1]});
            gravityValues[0] = 10; gravityValues[1] = gravityValues[3] = 1; mutateClosest = true;
            require(firstManager.getClosestObject(expectedClosest,2) == strategic[2], "post-gravity current entry reload picks replacement");
            expectTrace("gravity0,gravity1,gravity3");
        }
        std::printf("PASS %s\n", mode.c_str()); return 0;
    }
    if (mode == "positive_group") {
        groupValue = 7; expected = "side,player,group";
        CommonSingletonServer::m_iKitTemplateManager = nullptr; IAIMain::m_instance = nullptr; AIRandom::m_instance = nullptr;
    } else if (mode == "zero_empty" || mode == "negative_empty" || mode == "random_equal") {
        groupValue = mode == "negative_empty" ? -7 : 0;
        expectedReturn = false; wantedArity = 2; expected = prefix + list + refusal;
    } else if (mode == "random_above" || mode == "prior_empty") {
        expectedReturn = false; wantedArity = 2; expected = prefix + prior + list + refusal;
    } else if (mode == "prior_eligible") {
        points[9].eligible = true; selectedGroup = 19;
        expected = prefix + prior + ",prior,add0:19";
    } else if (mode == "cached_context") {
        points[9].eligible = true; selectedGroup = 19;
        expected = "side,player,group,templates,main1,prior,eligible1:2:9,prior,add1:19";
    } else if (mode == "fallback_null" || mode == "fallback_object") {
        groups.push_back(10); selectorGroup = mode == "fallback_null" ? -5 : 0;
        selectorObject = mode == "fallback_object" ? strategic[1] : nullptr;
        expectedKitObject = selectorObject; wantedArity = selectorObject ? 3 : 2;
        wantedObject = selectorObject ? 21 : -1; wantedGroup = selectorGroup; wantedKit = selectedKit;
        expected = prefix + list + ",sibling,kit,target" + number(wantedArity) + ":" + number(wantedObject) + ":" + number(wantedGroup) + ":6";
    } else {
        expected = prefix + prior + list + object;
        point(10, Vec3{1, 900, 0}); selectedGroup = 10;
        expectedClosest = points[10].second; expectClosest = true; expectedKitObject = strategic[1];
        if (mode == "no_eligible") {
            points[10].eligible = false; groups.push_back(11);
            expected += candidate(10,false,false) + candidate(11,false,false);
            selectedGroup = -1; expectClosest = false;
        } else if (mode == "nearest") {
            points[10].first = points[10].second = Vec3{3,999,0};
            point(11,Vec3{1,-999,0}); point(12,Vec3{2,0,0});
            selectedGroup = 11; expectedClosest = points[11].second;
            expected += candidate(10,true,true) + candidate(11,true,true) + candidate(12,true,false);
        } else if (mode == "tie") {
            point(11,Vec3{-1,0,0}); expected += candidate(10,true,true) + candidate(11,true,false);
        } else if (mode == "nan") {
            points[10].first.x = std::numeric_limits<float>::quiet_NaN();
            point(11,Vec3{0,0,std::numeric_limits<float>::infinity()});
            expected += candidate(10,true,false) + candidate(11,true,false);
            selectedGroup = -1; expectClosest = false;
        } else if (mode == "initial_limit") {
            points[10].first.x = 33554432.0f; // 2^50 distance exceeds exact initial float ceiling.
            expected += candidate(10,true,false); selectedGroup = -1; expectClosest = false;
        } else if (mode == "wide_distance") {
            point(11,Vec3{1.0f-0.000000059604644775390625f,0,fromBits(0x39B504F3u)});
            const double x = points[11].first.x, z = points[11].first.z, d = x*x+z*z;
            require(d < 1.0 && static_cast<float>(d) == 1.0f, "independent compare-before-narrowing witness");
            selectedGroup = 11; expectedClosest = points[11].second;
            expected += candidate(10,true,true) + candidate(11,true,true);
        } else if (mode == "live_list") {
            points[11].eligible = true; points[11].first = points[11].second = Vec3{0,7,0};
            selectedGroup = 11; expectedClosest = points[11].second;
            expected += candidate(10,true,true) + candidate(11,true,true);
        } else if (mode == "target_reference") {
            point(11,Vec3{2,0,0}); selectedGroup = 11; expectedClosest = points[11].second;
            expected += candidate(10,true,true) + candidate(11,true,true);
        } else if (mode == "second_position") {
            points[10].second = Vec3{41,42,43}; expectedClosest = points[10].second;
            expectedKitObject = strategic[3]; expected += candidate(10,true,true);
        } else if (mode == "kit_index_mutation") {
            expected += candidate(10,true,true);
        } else unexpected("unknown mode");
        if (expectClosest) {
            wantedArity = 3; wantedObject = mode == "second_position" ? 23 : mode == "kit_index_mutation" ? 88 : 21;
            wantedGroup = selectedGroup; wantedKit = selectedKit;
            expected += ",add0:" + number(selectedGroup) + (mode == "second_position" ? ",gravity2,gravity3" : ",gravity0,gravity1") +
                        ",kit,target3:" + number(wantedObject) + ":" + number(selectedGroup) + ":6";
        } else {
            expectedReturn = false; wantedArity = 2; expected += refusal;
        }
    }
    if (selectedGroup >= 0) { (*counts[0])[selectedGroup] = 3; (*counts[1])[selectedGroup] = 3; }
    const bool result = BotSpawner::findSuitableSpawnGroup(arrays[0], arrays[1], arrays[2], arrays[3], priorObjectId, bot);
    require(result == expectedReturn, "actual Boolean caller result"); expectTrace(expected);
    require(targetArity == wantedArity, "correct target overload or no target call");
    if (wantedArity) require(targetObject == wantedObject && targetGroup == wantedGroup && targetKit == wantedKit, "exact target arguments");
    if (mode == "positive_group") require(*randomIndex[0] == 0 && *randomIndex[1] == 0, "positive group does not draw random");
    else require(*randomIndex[mode == "cached_context" ? 1 : 0] == 1 && *randomIndex[mode == "cached_context" ? 0 : 1] == 0, "one draw from current random singleton");
    for (int a = 0; a < 4; ++a) for (int c = 0; c < 6; ++c) require(arrays[a][c] == float(10*a+c), "array identity forwarding does not alter values");
    require(observedSelectorCalls == ((mode == "fallback_null" || mode == "fallback_object") ? 1 : 0), "sibling selector branch");
    require(observedKitCalls == (wantedKit == 6 && wantedArity ? 1 : 0), "kit selector branch");
    if (mode == "fallback_null" || mode == "fallback_object") require(addedCount == 0, "primary does not increment around sibling selector");
    std::printf("PASS %s\n", mode.c_str()); return 0;
}
