// [TEMP] [CHOSEN, not recovered]
// Tests BFObjectEnvironment lifecycle, object queries, and damage with generated production bodies.
// Chosen lifetimes seed state and record calls; this does not exercise world loading, spawning, caching, or an AI frame.
#include "object_environment_runtime.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <new>
#include <type_traits>
#include <windows.h>
#include <crtdbg.h>

using namespace dice::hfe;
using namespace dice::hfe::ai;
static_assert(sizeof(void*) == 4, "MSVC x86 fixture only");

[[noreturn]] void unexpected(const char* name)
{
    std::fprintf(stderr, "UNEXPECTED collaborator: %s\n", name);
    std::fflush(stderr);
    std::abort();
}

static std::string mode;
static int checks = 0;
static std::string calls;
static std::function<void(const std::string&)> hook;

static void require(bool condition, const char* message)
{
    ++checks;
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s: %s [calls=%s]\n", mode.c_str(), message, calls.c_str());
        std::fflush(stderr);
        std::_Exit(1);
    }
}

static void trace(const std::string& item)
{
    if (!calls.empty())
        calls += ',';
    calls += item;
    if (hook)
        hook(item);
}

static std::string tag(char kind, int identity) { return std::string(1, kind) + std::to_string(identity); }
static std::uint32_t bits(float value) { std::uint32_t out; std::memcpy(&out, &value, 4); return out; }

// scripted collaborator state, indexed by observer identity
struct PlayerState { bool alive = true; bool down = false; world::IObject* vehicle = nullptr; int id = 0; };
struct TemplateState { unsigned int classId = 0; world::IObjectTemplateComponent* component = nullptr; };
struct ArmorState { float hitPoints = 0.0f; float maxHitPoints = 0.0f; };
struct ObjectState { bool answersPco = true; world::IObject* occupying = nullptr; bool occupied = false; };
static PlayerState playerStates[24];
static TemplateState templateStates[24];
static ArmorState armorStates[24];
static ObjectState objectStates[24];
static bool canBeDestroyed[24];
static double gameTime = 0.0;
static std::map<unsigned int, world::IObject*> pcoMap;
static std::list<world::IPlayer*>* managedPlayers = nullptr;
static char aiTokens[32];
static IAIObject* aiObject(int n) { return reinterpret_cast<IAIObject*>(&aiTokens[n]); }
static int aiLabel(IAIObject* object) { return static_cast<int>(reinterpret_cast<char*>(object) - aiTokens); }

struct DamageCall
{
    world::IObject* object; float damage; world::IPlayer* player;
    std::list<WeakPtr<world::IPlayer> >* assists; world::IObject* source; int team;
    world::IObject* weapon; world::IObject* vehicle; Vec3 position; Vec3 direction;
    int material; bool a13; bool canTK; bool noFriendlyFireMod;
};
static std::vector<DamageCall> damages;

// hooks the generated observers forward to
static double timerGameTime() { trace("G"); return gameTime; }
static std::map<unsigned int, world::IObject*>* logicPcoMap() { trace("L"); return &pcoMap; }
static bool logicGiveDamage(world::IObject* object, float damage, world::IPlayer* player,
                            std::list<WeakPtr<world::IPlayer> >* assists, world::IObject* source, int team,
                            world::IObject* weapon, world::IObject* vehicle, Vec3 position, Vec3 direction,
                            int material, bool a13, bool canTK, bool noFriendlyFireMod);
static bool playerAlive(int n) { trace(tag('A', n)); return playerStates[n].alive; }
static bool playerDown(int n) { trace(tag('M', n)); return playerStates[n].down; }
static world::IObject* playerVehicle(int n) { trace(tag('V', n)); return playerStates[n].vehicle; }
static int playerId(int n) { trace(tag('I', n)); return playerStates[n].id; }
static unsigned int templateClass(int n) { trace(tag('C', n)); return templateStates[n].classId; }
static world::IObjectTemplateComponent* templateComponent(int n, unsigned int classId)
{
    trace(tag('Q', n));
    require(classId == world::CID_ArmorGLCompTemplate, "only the armor template component is asked for");
    return templateStates[n].component;
}
static float armorHitPoints(int n) { trace(tag('H', n)); return armorStates[n].hitPoints; }
static float armorMaxHitPoints(int n) { trace(tag('X', n)); return armorStates[n].maxHitPoints; }

#include "collaborators.inl"

// chosen lifetimes: no registry, template, roster or world work
namespace dice { namespace hfe {
IGameLogic* gameLogic = nullptr;
namespace ai {
void* CommonSingletonServer::m_iBFTimer = nullptr;
IAIObjectEnvironment::IAIObjectEnvironment() {}
IAIObjectEnvironment::~IAIObjectEnvironment() {}
IBFTimer::IBFTimer() {}
IBFTimer::~IBFTimer() {}
}
namespace world {
PlayerManager* playerManager = nullptr;
PlayerManager::PlayerManager() { managedPlayers = &m_players; }
PlayerManager::~PlayerManager() {}
IPlayer::IPlayer() : m_weakControl(nullptr) {}
IPlayer::~IPlayer() {}
IObject::IObject() {}
IObject::~IObject() {}
Object::Object(const IObjectTemplate*) {}
Object::~Object() {}
SimpleObject::SimpleObject(const SimpleObjectTemplate*) : Object(nullptr) {}
SimpleObject::~SimpleObject() {}
Bundle::Bundle(const BundleTemplate*) : SimpleObject(nullptr) {}
Bundle::~Bundle() {}
PlayerControlObject::PlayerControlObject(const PlayerControlObjectTemplate*) : Bundle(nullptr) {}
PlayerControlObject::~PlayerControlObject() {}
IObjectTemplate::IObjectTemplate() : m_weakRef(nullptr) {}
IObjectTemplate::~IObjectTemplate() {}
ArmorGLCompTemplate::ArmorGLCompTemplate() {}
ArmorGLCompTemplate::~ArmorGLCompTemplate() {}

// The ridden objects, carried objects and PCO-map entries: real PlayerControlObjects, so the
// production static_cast and both images' IPlayerControlObject dispatch land on a real subobject.
class ObservedObject : public PlayerControlObject
{
public:
    int identity;
    ObservedObject(int id, const IObjectTemplate* objectTemplate, void* ai)
        : PlayerControlObject(nullptr), identity(id)
    {
        IObject::m_template = objectTemplate;      // PlayerControlObject's own m_template hides it
        IObject::m_aiObject = ai;
        IObject::m_armorComponent = nullptr;
        IObject::m_parent = nullptr;
    }
    void setArmor(IObjectComponent* armor) { IObject::m_armorComponent = armor; }
    void setParent(IObject* parent) { IObject::m_parent = parent; }
    void* queryInterface(unsigned int iid) const override
    {
        trace(tag('q', identity));
        require(iid == IID_IPlayerControlObject, "only IID_IPlayerControlObject is queried");
        if (!objectStates[identity].answersPco)
            return nullptr;
        return static_cast<IPlayerControlObject*>(const_cast<ObservedObject*>(this));
    }
    IObject* getOccupingObject() const override
    {
        trace(tag('O', identity));
        return objectStates[identity].occupying;
    }
    bool isOccupied() const override
    {
        trace(tag('U', identity));
        return objectStates[identity].occupied;
    }
};

class ObservedArmorTemplate : public ArmorGLCompTemplate
{
public:
    int identity;
    explicit ObservedArmorTemplate(int id) : identity(id) {}
    bool getCanBeDestroyed() override { trace(tag('D', identity)); return canBeDestroyed[identity]; }
};
} // namespace world
} }

using world::ObservedObject;

static bool logicGiveDamage(world::IObject* object, float damage, world::IPlayer* player,
                            std::list<WeakPtr<world::IPlayer> >* assists, world::IObject* source, int team,
                            world::IObject* weapon, world::IObject* vehicle, Vec3 position, Vec3 direction,
                            int material, bool a13, bool canTK, bool noFriendlyFireMod)
{
    trace(tag('Z', static_cast<ObservedObject*>(object)->identity));
    DamageCall call = { object, damage, player, assists, source, team, weapon, vehicle, position, direction,
                        material, a13, canTK, noFriendlyFireMod };
    damages.push_back(call);
    return false;
}

class ObservedPredicate : public AIObjectPredicate
{
public:
    std::vector<int> rejected;
    bool predicate(IAIObject* object) override
    {
        const int label = aiLabel(object);
        trace(tag('P', label));
        return std::find(rejected.begin(), rejected.end(), label) == rejected.end();
    }
};

static world::ObservedPlayer players[24];
static world::ObservedTemplate templates[24];
static world::ObservedArmor armors[24];

static world::IObjectTemplateComponent* armorTemplate(int n)
{
    world::ObservedArmorTemplate* const created = new world::ObservedArmorTemplate(n);   // fixture-lifetime
    return static_cast<world::IObjectTemplateComponent*>(created);
}

static void setPlayer(int n, bool alive, bool down, world::IObject* vehicle, int id)
{
    playerStates[n].alive = alive;
    playerStates[n].down = down;
    playerStates[n].vehicle = vehicle;
    playerStates[n].id = id;
}

static void setArmor(ObservedObject& object, int n, float hitPoints, float maxHitPoints)
{
    armorStates[n].hitPoints = hitPoints;
    armorStates[n].maxHitPoints = maxHitPoints;
    object.setArmor(static_cast<world::IObjectComponent*>(&armors[n]));
}

// access violations the shipped bodies are expected to take
static DWORD faultCode = 0;
static ULONG_PTR faultKind = ~static_cast<ULONG_PTR>(0), faultAddress = ~static_cast<ULONG_PTR>(0);
static IAIObjectEnvironment* faultEnvironment = nullptr;
static std::vector<std::pair<int, IAIObject*> >* faultOut = nullptr;

static int recordFault(EXCEPTION_POINTERS* info)
{
    faultCode = info->ExceptionRecord->ExceptionCode;
    if (info->ExceptionRecord->NumberParameters >= 2)
    {
        faultKind = info->ExceptionRecord->ExceptionInformation[0];
        faultAddress = info->ExceptionRecord->ExceptionInformation[1];
    }
    return EXCEPTION_EXECUTE_HANDLER;
}

static bool faults(void (*body)())
{
    __try
    {
        body();
    }
    __except (recordFault(GetExceptionInformation()))
    {
        return true;
    }
    return false;
}

static void occupiedBody() { faultEnvironment->getWorldPlayerOccupiedObjectsWithIds(*faultOut, nullptr); }
static void damageBody() { faultEnvironment->damageSeverelyDamagedVehicles(); }

typedef std::vector<std::pair<int, IAIObject*> > Pairs;
static int counter = 0;

// getWorldPlayerOccupiedObjectsWithIds
static void containerCase(IAIObjectEnvironment& env, unsigned int classId, bool occupant, bool occupantAi,
                          const char* expected, bool pushed)
{
    ObservedObject carried(8, &templates[8], occupantAi ? aiObject(8) : nullptr);
    ObservedObject vehicle(7, &templates[7], nullptr);
    templateStates[7].classId = classId;
    objectStates[7].occupying = occupant ? &carried : nullptr;
    setPlayer(7, true, false, &vehicle, 107);
    managedPlayers->push_back(&players[7]);
    Pairs out;
    env.getWorldPlayerOccupiedObjectsWithIds(out, nullptr);
    require(calls == expected, "exact call order through the container arm");
    if (pushed)
        require(out.size() == 1 && out[0] == std::make_pair(107, aiObject(8)), "the carried object's AI object");
    else
        require(out.empty(), "nothing appended");
}

static void runOccupied(IAIObjectEnvironment& env)
{
    if (mode == "wo_empty")
    {
        Pairs out(1, std::make_pair(-7, aiObject(1)));
        env.getWorldPlayerOccupiedObjectsWithIds(out, nullptr);
        require(out.size() == 1 && out[0] == std::make_pair(-7, aiObject(1)), "existing entry kept, nothing added");
        require(calls.empty(), "no player asked anything");
    }
    else if (mode == "wo_filters")
    {
        ObservedObject vehicle(4, &templates[4], aiObject(4));
        setPlayer(1, false, false, &vehicle, 101);
        setPlayer(2, true, true, &vehicle, 102);
        setPlayer(3, true, false, nullptr, 103);
        setPlayer(4, true, false, &vehicle, 104);
        managedPlayers->push_back(nullptr);
        for (int n = 1; n <= 4; ++n)
            managedPlayers->push_back(&players[n]);
        Pairs out(1, std::make_pair(-7, aiObject(1)));
        env.getWorldPlayerOccupiedObjectsWithIds(out, nullptr);
        require(calls == "A1,A2,M2,A3,M3,V3,A4,M4,V4,V4,I4", "null, dead, down and on-foot players drop out in order");
        require(out.size() == 2 && out[0] == std::make_pair(-7, aiObject(1)) &&
                out[1] == std::make_pair(104, aiObject(4)), "appended after the existing entry, never cleared");
    }
    else if (mode == "wo_predicate")
    {
        ObservedObject first(5, &templates[5], aiObject(5)), second(6, &templates[6], aiObject(6));
        setPlayer(5, true, false, &first, 105);
        setPlayer(6, true, false, &second, 106);
        managedPlayers->push_back(&players[5]);
        managedPlayers->push_back(&players[6]);
        ObservedPredicate filter;
        filter.rejected.push_back(5);
        Pairs out;
        env.getWorldPlayerOccupiedObjectsWithIds(out, &filter);
        require(calls == "A5,M5,V5,V5,P5,A6,M6,V6,V6,P6,I6", "a rejected object asks no id");
        require(out.size() == 1 && out[0] == std::make_pair(106, aiObject(6)), "only the accepted pair");
    }
    else if (mode == "wo_ladder")
        containerCase(env, world::CID_LadderContainerTemplate, true, true, "A7,M7,V7,V7,V7,C7,V7,O7,I7", true);
    else if (mode == "wo_ladder_empty")
        containerCase(env, world::CID_LadderContainerTemplate, false, false, "A7,M7,V7,V7,V7,C7,V7,O7", false);
    else if (mode == "wo_ladder_no_ai")
        containerCase(env, world::CID_LadderContainerTemplate, true, false, "A7,M7,V7,V7,V7,C7,V7,O7", false);
    else if (mode == "wo_parachute")
        containerCase(env, world::CID_ParachuteTemplate, true, true,
                      "A7,M7,V7,V7,V7,C7,V7,C7,V7,q7,O7,I7", true);
    else if (mode == "wo_zipline")
        containerCase(env, world::CID_ZiplineContainerTemplate, true, true,
                      "A7,M7,V7,V7,V7,C7,V7,C7,V7,C7,V7,O7,I7", true);
    else if (mode == "wo_rope")
        containerCase(env, world::CID_GrapplingHookRopeContainerTemplate, true, true,
                      "A7,M7,V7,V7,V7,C7,V7,C7,V7,C7,V7,C7,V7,O7,I7", true);
    else if (mode == "wo_other")
        containerCase(env, 0x7777u, true, true, "A7,M7,V7,V7,V7,C7,V7,C7,V7,C7,V7,C7", false);
    else if (mode == "wo_container_predicate")
    {
        ObservedObject carried(8, &templates[8], aiObject(8));
        ObservedObject vehicle(7, &templates[7], nullptr);
        templateStates[7].classId = world::CID_LadderContainerTemplate;
        objectStates[7].occupying = &carried;
        setPlayer(7, true, false, &vehicle, 107);
        managedPlayers->push_back(&players[7]);
        ObservedPredicate filter;
        filter.rejected.push_back(8);
        Pairs out;
        env.getWorldPlayerOccupiedObjectsWithIds(out, &filter);
        require(calls == "A7,M7,V7,V7,V7,C7,V7,O7,P8", "the filter sees the CARRIED object's AI object");
        require(out.empty(), "rejected");
        calls.clear();
        filter.rejected.clear();
        env.getWorldPlayerOccupiedObjectsWithIds(out, &filter);
        require(calls == "A7,M7,V7,V7,V7,C7,V7,O7,P8,I7", "accepted on the second pass");
        require(out.size() == 1 && out[0] == std::make_pair(107, aiObject(8)), "the carried pair");
    }
    else if (mode == "wo_reread_id")
    {
        ObservedObject vehicle(5, &templates[5], aiObject(5));
        setPlayer(5, true, false, &vehicle, 105);
        setPlayer(6, true, false, nullptr, 106);
        managedPlayers->push_back(&players[5]);
        hook = [](const std::string& event) { if (event == "P5") managedPlayers->front() = &players[6]; };
        ObservedPredicate filter;
        Pairs out;
        env.getWorldPlayerOccupiedObjectsWithIds(out, &filter);
        require(calls == "A5,M5,V5,V5,P5,I6", "getId re-reads the list node after the filter");
        require(out.size() == 1 && out[0] == std::make_pair(106, aiObject(5)), "node's id, first player's object");
    }
    else if (mode == "wo_reread_class")
    {
        ObservedObject carried(12, &templates[12], aiObject(12));
        ObservedObject first(10, &templates[10], nullptr), second(11, &templates[11], nullptr);
        templateStates[10].classId = 0x7777u;
        templateStates[11].classId = world::CID_LadderContainerTemplate;
        objectStates[10].occupying = &carried;
        setPlayer(10, true, false, &first, 110);
        setPlayer(11, true, false, &second, 111);
        managedPlayers->push_back(&players[10]);
        hook = [](const std::string& event)
        {
            if (event == "V10" && ++counter == 2)
                managedPlayers->front() = &players[11];
        };
        Pairs out;
        env.getWorldPlayerOccupiedObjectsWithIds(out, nullptr);
        require(calls == "A10,M10,V10,V10,V11,C11,V10,O10,I11",
                "the class chain reads the list node; the ladder arm reads the local player");
        require(out.size() == 1 && out[0] == std::make_pair(111, aiObject(12)), "node id, local player's carried object");
    }
    else if (mode == "wo_live_list")
    {
        ObservedObject first(4, &templates[4], aiObject(4)), second(6, &templates[6], aiObject(6));
        setPlayer(4, true, false, &first, 104);
        setPlayer(6, true, false, &second, 106);
        managedPlayers->push_back(&players[4]);
        hook = [](const std::string& event) { if (event == "I4") managedPlayers->push_back(&players[6]); };
        Pairs out;
        env.getWorldPlayerOccupiedObjectsWithIds(out, nullptr);
        require(calls == "A4,M4,V4,V4,I4,A6,M6,V6,V6,I6", "a player appended mid-walk is visited");
        require(out.size() == 2 && out[1] == std::make_pair(106, aiObject(6)), "both pairs");
    }
    else if (mode == "wo_parachute_no_occupant" || mode == "wo_parachute_no_interface")
    {
        const bool noInterface = mode == "wo_parachute_no_interface";
        ObservedObject vehicle(7, &templates[7], nullptr);
        templateStates[7].classId = world::CID_ParachuteTemplate;
        objectStates[7].answersPco = !noInterface;
        objectStates[7].occupying = nullptr;
        setPlayer(7, true, false, &vehicle, 107);
        managedPlayers->push_back(&players[7]);
        Pairs out;
        faultEnvironment = &env;
        faultOut = &out;
        require(faults(occupiedBody), "the parachute arm tests nothing and faults");
        require(faultCode == EXCEPTION_ACCESS_VIOLATION && faultKind == 0, "a read access violation");
        require(faultAddress == (noInterface ? 0x0u : 0x34u),
                noInterface ? "the null interface's vptr is read" : "the null carried object's +0x34 is read");
        require(calls == (noInterface ? "A7,M7,V7,V7,V7,C7,V7,C7,V7,q7" : "A7,M7,V7,V7,V7,C7,V7,C7,V7,q7,O7"),
                "exact calls up to the fault");
    }
    else
        unexpected("unknown wo_ mode");
}

// damageSeverelyDamagedVehicles
static void sweepAt(IAIObjectEnvironment& env, double now, const char* expected, const char* message)
{
    calls.clear();
    gameTime = now;
    env.damageSeverelyDamagedVehicles();
    require(calls == expected, message);
}

static void runDamage(IAIObjectEnvironment& env)
{
    if (mode == "dv_fresh_ctor")
    {
        // The corrected constructor writes all eight bytes of the clock: over dirty memory the first
        // sweep still fires at 4.5 s. The old 4-byte store left a NaN here and never swept.
        alignas(8) static unsigned char storage[sizeof(BFObjectEnvironment)];
        std::memset(storage, 0xFF, sizeof storage);
        BFObjectEnvironment* const fresh = new (storage) BFObjectEnvironment;
        IAIObjectEnvironment& environment = *fresh;
        sweepAt(environment, 4.5, "G,L", "a fresh clock is 0.0 over dirty memory");
        sweepAt(environment, 8.5, "G", "exactly four seconds later does not sweep");
        sweepAt(environment, std::nextafter(8.5, 9.0), "G,L", "just over four seconds does");
        fresh->~BFObjectEnvironment();
    }
    else if (mode == "dv_interval")
    {
        sweepAt(env, 3.0, "G", "three seconds from zero");
        sweepAt(env, 4.0, "G", "exactly four from zero");
        sweepAt(env, 4.25, "G,L", "past four from zero");
        sweepAt(env, 8.25, "G", "exactly four from the last sweep");
        sweepAt(env, 8.2500001, "G,L", "past four from the last sweep");
        sweepAt(env, 12.0, "G", "the clock moved to the sweep time");
    }
    else if (mode == "dv_backwards")
    {
        sweepAt(env, 100.0, "G,L", "first sweep");
        sweepAt(env, 50.0, "G,L", "a clock behind the stored time resets it to ZERO, not to now");
        sweepAt(env, 2.0, "G", "reset to zero again, and two seconds is too soon");
        sweepAt(env, 4.5, "G,L", "the reset persisted");
    }
    else if (mode == "dv_nan")
    {
        const double nan = std::numeric_limits<double>::quiet_NaN();
        sweepAt(env, nan, "G", "a NaN clock neither resets nor sweeps");
        sweepAt(env, 4.5, "G,L", "the stored zero survived");
        sweepAt(env, nan, "G", "again");
        sweepAt(env, 8.6, "G,L", "the stored 4.5 survived");
    }
    else if (mode == "dv_skips")
    {
        ObservedObject* objects[9] = {};
        for (int n = 1; n <= 8; ++n)
        {
            objects[n] = new ObservedObject(n, &templates[n], nullptr);
            templateStates[n].classId = 0x7777u;
            pcoMap[static_cast<unsigned int>(n)] = objects[n];
        }
        templateStates[1].classId = world::CID_SoldierTemplate;
        objectStates[2].occupied = true;
        templateStates[3].component = armorTemplate(3);                               // no armor component
        setArmor(*objects[4], 4, 10.0f, 100.0f);                                      // no armor template
        setArmor(*objects[5], 5, 10.0f, 100.0f); templateStates[5].component = armorTemplate(5);
        setArmor(*objects[6], 6, 60.0f, 100.0f); templateStates[6].component = armorTemplate(6);
        setArmor(*objects[7], 7, 50.0f, 100.0f); templateStates[7].component = armorTemplate(7);
        setArmor(*objects[8], 8, 49.5f, 100.0f); templateStates[8].component = armorTemplate(8);
        canBeDestroyed[6] = canBeDestroyed[7] = canBeDestroyed[8] = true;             // 5 cannot be
        sweepAt(env, 10.0,
                "G,L,C1,C2,q2,U2,C3,q3,U3,Q3,C4,q4,U4,Q4,C5,q5,U5,Q5,X5,D5,C6,q6,U6,Q6,X6,D6,H6,"
                "C7,q7,U7,Q7,X7,D7,H7,C8,q8,U8,Q8,X8,D8,H8,Z8",
                "soldier, occupied, armorless, templateless, indestructible, healthy, exactly-half, damaged");
        require(damages.size() == 1 && damages[0].object == objects[8], "only the damaged vehicle");
        require(bits(damages[0].damage) == bits(100.0f * 0.1f), "a tenth of the MAXIMUM");
    }
    else if (mode == "dv_parent_walk")
    {
        ObservedObject child(1, &templates[1], nullptr), parent(2, &templates[2], nullptr);
        templateStates[1].classId = 0x7777u;
        objectStates[1].answersPco = false;
        child.setParent(&parent);
        setArmor(child, 1, 10.0f, 100.0f);
        templateStates[1].component = armorTemplate(1);
        canBeDestroyed[1] = true;
        pcoMap[1] = &child;
        sweepAt(env, 10.0, "G,L,C1,q1,q2,U2,Q1,X1,D1,H1,Z1",
                "the PCO walk climbs to the parent; armor and template are the object's own");
        require(damages.size() == 1 && damages[0].object == &child, "the map object is damaged");
    }
    else if (mode == "dv_damage_args")
    {
        ObservedObject vehicle(1, &templates[1], nullptr);
        templateStates[1].classId = 0x7777u;
        setArmor(vehicle, 1, 1.0f, 13.0f);
        templateStates[1].component = armorTemplate(1);
        canBeDestroyed[1] = true;
        pcoMap[1] = &vehicle;
        sweepAt(env, 10.0, "G,L,C1,q1,U1,Q1,X1,D1,H1,Z1", "one damaged vehicle");
        require(damages.size() == 1, "one giveDamage");
        const DamageCall& d = damages[0];
        require(d.object == &vehicle, "the object itself");
        require(bits(d.damage) == 0x3FA66667u, "13 * 0.1f - the FLOAT tenth (a double 0.1 gives 0x3FA66666)");
        require(d.player == nullptr && d.assists == nullptr && d.source == nullptr && d.weapon == nullptr &&
                d.vehicle == nullptr, "no attacker, assist list, source, weapon or vehicle");
        require(d.team == -1 && d.material == -1, "team and material -1");
        require(d.position.x == 0.0f && d.position.y == 0.0f && d.position.z == 0.0f, "position (0,0,0)");
        require(d.direction.x == 0.0f && d.direction.y == 1.0f && d.direction.z == 0.0f, "direction (0,1,0)");
        require(!d.a13 && d.canTK && !d.noFriendlyFireMod, "false, true, false");
    }
    else if (mode == "dv_zero_max")
    {
        ObservedObject negative(1, &templates[1], nullptr), empty(2, &templates[2], nullptr);
        templateStates[1].classId = templateStates[2].classId = 0x7777u;
        setArmor(negative, 1, -1.0f, 0.0f);
        setArmor(empty, 2, 0.0f, 0.0f);
        templateStates[1].component = armorTemplate(1);
        templateStates[2].component = armorTemplate(2);
        canBeDestroyed[1] = canBeDestroyed[2] = true;
        pcoMap[1] = &negative;
        pcoMap[2] = &empty;
        sweepAt(env, 10.0, "G,L,C1,q1,U1,Q1,X1,D1,H1,Z1,C2,q2,U2,Q2,X2,D2,H2",
                "no guard on a zero maximum: -1/0 is -inf (damaged), 0/0 is NaN (not)");
        require(damages.size() == 1 && damages[0].object == &negative && bits(damages[0].damage) == 0u,
                "a zero-point tenth");
    }
    else if (mode == "dv_live_map")
    {
        ObservedObject first(1, &templates[1], nullptr), later(5, &templates[5], nullptr);
        templateStates[1].classId = templateStates[5].classId = 0x7777u;
        setArmor(first, 1, 10.0f, 100.0f);
        setArmor(later, 5, 10.0f, 100.0f);
        templateStates[1].component = armorTemplate(1);
        templateStates[5].component = armorTemplate(5);
        canBeDestroyed[1] = canBeDestroyed[5] = true;
        pcoMap[1] = &first;
        static ObservedObject* inserted = nullptr;
        inserted = &later;
        hook = [](const std::string& event) { if (event == "Z1") pcoMap[5] = inserted; };
        sweepAt(env, 10.0, "G,L,C1,q1,U1,Q1,X1,D1,H1,Z1,C5,q5,U5,Q5,X5,D5,H5,Z5",
                "an entry inserted mid-sweep is visited");
        require(damages.size() == 2, "both damaged");
    }
    else if (mode == "dv_no_pco")
    {
        ObservedObject orphan(1, &templates[1], nullptr);
        templateStates[1].classId = 0x7777u;
        objectStates[1].answersPco = false;
        pcoMap[1] = &orphan;
        faultEnvironment = &env;
        gameTime = 10.0;
        require(faults(damageBody), "no PCO anywhere up the chain faults");
        require(faultCode == EXCEPTION_ACCESS_VIOLATION && faultKind == 0 && faultAddress == 0,
                "isOccupied through a null interface reads its vptr at 0");
        require(calls == "G,L,C1,q1", "exact calls up to the fault");
    }
    else
        unexpected("unknown dv_ mode");
}

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    if (argc != 2)
    {
        std::fprintf(stderr, "usage: object_environment_runtime MODE\n");
        return 2;
    }
    mode = argv[1];
    static_assert(std::is_same<decltype(&AIObjectPredicate::predicate), bool (AIObjectPredicate::*)(IAIObject*)>::value,
                  "the one predicate slot");

    world::PlayerManager manager;
    world::playerManager = &manager;
    ObservedTimer timer;
    CommonSingletonServer::m_iBFTimer = static_cast<IBFTimer*>(&timer);
    ObservedLogic logic;
    gameLogic = &logic;
    for (int n = 0; n < 24; ++n)
    {
        players[n].identity = n;
        templates[n].identity = n;
        armors[n].identity = n;
    }

    BFObjectEnvironment environment;
    IAIObjectEnvironment& env = environment;
    if (mode.compare(0, 3, "wo_") == 0)
        runOccupied(env);
    else if (mode.compare(0, 3, "dv_") == 0)
        runDamage(env);
    else
        unexpected("unknown mode");

    hook = nullptr;
    std::printf("PASS %s %d checks\n", mode.c_str(), checks);
    std::fflush(stdout);
    return 0;
}
