// [TEMP] Stock lifecycle observer and bounded registry/event checks. No production
// method is replaced. Run through tier3/run.py --suite managers --reload.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <vector>
#include "../../BF2/Core/Ids.h"
#include "../../BF2/Core/System.h"
#include "../../BF2/Game/IGameLogic.h"
#include "../../BF2/Game/Objects/Game/AmbientEffectArea.h"
#include "../../BF2/Game/Objects/Game/AmbientEffectAreaManager.h"
#include "../../BF2/Game/GameEvents/AmbientEffectAreaEvent.h"
#include "../../BF2/Scene/Object/ObjectManager.h"
#include "../../BF2/IO/BitStream.h"
#include "../../BF2/Game/Objects/Game/TriggerManager.h"
#include "../../BF2/Game/Objects/Game/TriggerableManager.h"
#include "../../BF2/Game/Objects/TemplateLeaves.h"
#include "../../BF2/Game/Objects/Weapons/GenericFireArm.h"
#include "../../BF2/Game/GameLogic/GameLogicExplosion.h"
#include "../../BF2/Game/GameLogic/Player.h"
#include "../../BF2/Physics/HeightmapCluster.h"
#include "../../BF2/IO/Network/INetworkable.h"
#include <cmath>
#include <limits>
using namespace dice::hfe;
using namespace dice::hfe::world;
namespace dice { namespace hfe { extern IGameLogic* gameLogic;
namespace world { extern IBase* heightmapCluster; } } }
namespace {
struct State {
    bool passed = true, destroyed = false, sawTickReset = false;
    unsigned checks = 0, adds = 0, removes = 0, clears = 0, initial = 0, previousTick = 0;
    AmbientEffectAreaManager* production = nullptr;
    std::map<int, AmbientEffectArea*> live;
};
State& state() { static State value; return value; }
struct TriggerState {
    unsigned adds=0, removes=0, areaChecks=0, checksAtSetup=0, initial=0;
    bool recording=false;
    std::set<Trigger*> live;
    std::vector<Trigger*> checked, events;
    TriggerManager* production=nullptr;
    TriggerableManager* triggerables=nullptr;
};
TriggerState& triggers() { static TriggerState value; return value; }
// [TEMP] Expose state only on independently constructed test objects. All methods
// execute production bodies; no method override or grid/Player fixture is used.
struct TestTrigger : Trigger {
    explicit TestTrigger(const TriggerTemplate* t) : Trigger(t) {}
    using Trigger::m_effectors;
    using Trigger::m_lastTestTick;
    using Trigger::m_lastTriggerTick;
};
void check(bool value, const char* label)
{
    State& s = state(); ++s.checks;
    if (!value) s.passed = false;
    std::printf("[tier3-managers] %s %s\n", value ? "PASS" : "FAIL", label);
    std::fflush(stdout);
}
// Exposes event payload only; constructors/serialization/decoding are real bodies.
struct Event : AmbientEffectAreaEvent {
    Event() {}
    Event(IObject* a, int t, Vec3 p, unsigned short n) : AmbientEffectAreaEvent(a,t,p,n) {}
    using AmbientEffectAreaEvent::m_area;
    using AmbientEffectAreaEvent::m_areaId;
    using AmbientEffectAreaEvent::m_team;
    using AmbientEffectAreaEvent::m_objectNetId;
    using AmbientEffectAreaEvent::m_position;
};
struct EffectPayload { unsigned areaId, index, objectId; Vec3 position; };
std::vector<EffectPayload> effectEvents;
bool recordEffects=false;
// [TEMP] Expose private scalar/cache state and count real position calls. The
// placement, random draw, terrain query and GameServer dispatch all remain production.
struct TestArea : AmbientEffectArea {
    explicit TestArea(const AmbientEffectAreaTemplate* t) : AmbientEffectArea(t) {}
    using AmbientEffectArea::m_lastRandom;
    using AmbientEffectArea::m_effectContainer;
    using AmbientEffectArea::m_effectTemplate;
    mutable unsigned positionReads=0;
    const Vec3& getAbsolutePosition() const override {
        ++positionReads; return AmbientEffectArea::getAbsolutePosition();
    }
};
// [TEMP] Models only the IPlayer::getVehicle response. No remote connection,
// spawn, player registration or replacement GameLogic is involved in this check.
struct EffectPlayer : dice::hfe::Player {
    EffectPlayer() : dice::hfe::Player(250,nullptr) {}
    IObject* vehicle=nullptr;
    mutable unsigned reads=0;
    IObject* getVehicle() const override { ++reads; return vehicle; }
};
void effectChecks()
{
    const int counter=ambientEffectAreaNextId;
    AmbientEffectAreaManager* owner=ambientEffectAreaManager;
    {
        AmbientEffectAreaManager local;
        ambientEffectAreaManager=&local;
        AmbientEffectAreaTemplate t;
        BundleTemplate containerTemplate;
        TestArea area(&t);
        local.addAmbientEffectArea(&area);
        area.setAbsolutePosition(Vec3{100,20,100});
        t.m_projectOnHeightmap=false; t.m_playAtEventPosition=true;
        t.m_numNetworkedEffects=7;
        recordEffects=true; effectEvents.clear();
        area.trigger(nullptr,Vec3{103,24,105});
        check(effectEvents.size()==1 && effectEvents.back().index==0
            && effectEvents.back().objectId==0 && effectEvents.back().position.x==3
            && effectEvents.back().position.y==4 && effectEvents.back().position.z==5,
            "dedicated activation sends one relative-position event even with seven configured effects");
        check(area.m_lastRandom==0x759DC150,"event placement leaves the area random seed untouched");
        t.m_numNetworkedEffects=0; area.trigger(nullptr,Vec3{103,24,105});
        check(effectEvents.size()==1,"non-networked dedicated activation sends no event or local effect");
        t.m_numNetworkedEffects=1; t.m_projectOnHeightmap=true;
        area.positionReads=0; area.trigger(nullptr,Vec3{103,24,105});
        const float height=static_cast<IHeightmapCluster*>(heightmapCluster)->getHeightInWorldCoords(103,105);
        check(area.positionReads==4 && effectEvents.back().position.y==height-20,
            "height projection queries stock terrain and preserves three additional area-position calls");
        {
            TriggerTemplate triggerTemplate;
            TestTrigger trigger(&triggerTemplate);
            trigger.m_effectors.push_back(&area);
            const auto count=effectEvents.size();
            trigger.executeTrigger(nullptr,Vec3{103,24,105},-1);
            check(effectEvents.size()==count+1 && effectEvents.back().position.y==height-20
                && unsigned(trigger.m_lastTriggerTick)==WorldPref::getTickCount(),
                "real Trigger execution reaches area placement and the GameServer event-send path");
            area.handleAmbientMessage(static_cast<TemplateMessage>(1),nullptr,Vec3{103,24,105});
            check(effectEvents.size()==count+1,"non-trigger ambient messages remain ignored");
            trigger.destroy(false);
        }
        t.m_projectOnHeightmap=false; t.m_playAtEventPosition=false; t.m_radius=10;
        area.trigger(nullptr,Vec3{900,900,900});
        const auto first=effectEvents.back().position;
        const unsigned firstSeed=unsigned(area.m_lastRandom);
        // Independently authored arithmetic model: modular products, not the production
        // 16-bit-halves implementation. Geometry uses the independent circle equation.
        auto advance=[](unsigned seed) {
            return unsigned(((static_cast<unsigned long long>(seed)*24112u)%2147483647u*26143u)%2147483647u);
        };
        const unsigned raw1=advance(0x759DC150), raw2=advance(raw1);
        const float expectedX=float((((raw1>>7)|1u)+1u)/8388608.0-1.0);
        const float chord=float(std::sqrt(1.0-double(expectedX)*expectedX));
        const float expectedZ=float(((((raw2>>7)|1u)+1u)/16777216.0*2.0*chord-chord)*10.0);
        check(firstSeed==raw2 && std::fabs(first.x-expectedX*10)<0.00001f
            && std::fabs(first.z-expectedZ)<0.00001f && first.y==0,
            "random disk placement matches an independent modular/chord oracle");
        area.trigger(nullptr,Vec3{900,900,900});
        check(unsigned(area.m_lastRandom)==advance(advance(firstSeed))
            && (effectEvents.back().position.x!=first.x || effectEvents.back().position.z!=first.z),
            "consecutive random placements continue persistent state without tick reseeding");
        area.m_effectContainer=&area; area.m_effectTemplate=&containerTemplate;
        t.m_playAtEventPosition=true; t.m_numNetworkedEffects=13;
        area.trigger(nullptr,Vec3{103,24,105});
        const unsigned selectionSeed=advance((WorldPref::getTickCount()&1023u)+92993u);
        const unsigned selected=unsigned(1.0+12.0*(((selectionSeed>>7)|1u)+1u)/16777216.0);
        check(unsigned(area.m_lastRandom)==selectionSeed && effectEvents.back().index==selected,
            "bundle selection reseeds from tick and uses configured effect count independently of child count");
        area.m_effectContainer=nullptr; area.m_effectTemplate=nullptr;
        {
            EffectPlayer player;
            area.trigger(&player,Vec3{900,900,900});
            check(player.reads==1 && effectEvents.back().position.x==0
                && effectEvents.back().position.y==0 && effectEvents.back().position.z==0
                && effectEvents.back().objectId==0,
                "player with no vehicle sends a zero attachment offset");
            for (const auto& row:objectManager->getAllRegisteredObjects()) {
                auto net=row.second->getNetworkable();
                if (net && net->m_networkableId && row.second->queryInterface(IID_IPlayerControlObject)) {
                    player.vehicle=row.second; break;
                }
            }
            check(player.vehicle!=nullptr,"stock world supplies a registered vehicle for attachment");
            if (player.vehicle) {
                area.trigger(&player,Vec3{900,900,900});
                check(effectEvents.back().objectId==player.vehicle->getNetworkable()->m_networkableId
                    && effectEvents.back().position.x==0 && effectEvents.back().position.y==0
                    && effectEvents.back().position.z==0,
                    "attachment sends the stock vehicle network id with zero offset");
                t.m_attachToEventObject=false;
                area.trigger(&player,Vec3{900,900,900});
                const auto p=player.vehicle->getAbsolutePosition();
                check(effectEvents.back().objectId==0 && effectEvents.back().position.x==p.x-100
                    && effectEvents.back().position.y==p.y-20 && effectEvents.back().position.z==p.z-100,
                    "detached player placement uses vehicle position rather than the passed event point");
            }
        }
        recordEffects=false;
        area.destroy(false);
    }
    ambientEffectAreaManager=owner;
    ambientEffectAreaNextId=counter;
}
void snapshot()
{
    State& s = state(); unsigned count = 0;
    bool indexed = ambientEffectAreaManager == s.production;
    for (const auto& row : objectManager->getAllRegisteredObjects()) {
        IObject* object = row.second;
        if (object->getTemplate()->getClassID() != CID_AmbientEffectAreaTemplate) continue;
        ++count;
        Event event(object,0,Vec3{0,0,0},0);
        indexed = indexed && ambientEffectAreaManager->getAreaFromId(event.m_areaId) == object;
    }
    std::printf("[tier3-area-census] objects=%u indexed=%u adds=%u removes=%u clears=%u\n",
        count,unsigned(s.live.size()),s.adds,s.removes,s.clears);
    check(indexed && count > 0 && count == s.live.size(), "every stock ambient area has its real id index");
}
void registryChecks()
{
    State& s = state();
    const auto first = *s.live.begin();
    const int savedCounter = ambientEffectAreaNextId;
    const int triggerId = first.second->getTriggerId();
    {
        AmbientEffectAreaManager local;
        check(ambientEffectAreaManager == s.production, "a second manager does not replace the singleton");
        std::vector<IObject*> objects;
        check(!local.getAreasWithTriggerId(triggerId,objects), "empty query is false");
        objects.push_back(nullptr);
        check(local.getAreasWithTriggerId(triggerId,objects) && objects.size()==1,
            "query preserves an existing output and reports its nonempty state");
        std::set<AmbientEffectArea*> expected;
        for (const auto& entry : s.live) {
            local.addAmbientEffectArea(entry.second);
            if (entry.second->getTriggerId()==triggerId) expected.insert(entry.second);
        }
        local.addAmbientEffectArea(first.second);
        objects.clear();
        local.getAreasWithTriggerId(triggerId,objects);
        std::vector<IObject*> ordered(expected.begin(),expected.end());
        check(objects==ordered, "duplicate registration stays unique and queries follow pointer-set order");
        check(local.getAreaFromId(first.first)==first.second && !local.getAreaFromId(-123),
            "signed id lookup returns the object and does not insert misses");
        local.removeAmbientEffectArea(first.second);
        check(!local.getAreaFromId(first.first) && ambientEffectAreaManager->getAreaFromId(first.first)==first.second,
            "local removal clears its id while preserving the production index");
        local.removeAll();
        check(!local.getAreaFromId(first.first) && ambientEffectAreaNextId==0,
            "removeAll clears indexes and resets the shared id counter");
        ambientEffectAreaNextId=savedCounter; // restore the test's deliberate counter mutation
    }
    check(ambientEffectAreaManager==s.production && ambientEffectAreaNextId==savedCounter,
        "secondary destruction neither clears the singleton nor resets ids");

    // Isolated empty-manager lifecycle: no world object is destroyed while the alias is changed.
    ambientEffectAreaManager=nullptr;
    {
        AmbientEffectAreaManager owner;
        check(ambientEffectAreaManager==&owner, "first manager publishes itself");
        { AmbientEffectAreaManager later; }
        check(ambientEffectAreaManager==&owner, "later manager destruction preserves the owner");
    }
    check(!ambientEffectAreaManager, "owning manager destruction unpublishes itself");
    ambientEffectAreaManager=s.production;

    Event outgoing(first.second,0xA3,Vec3{1.0f,-2.0f,3.5f},0x1234);
    check(outgoing.m_areaId==unsigned(first.first), "event constructor reads the native area id member");
    auto objectTemplate=static_cast<const AmbientEffectAreaTemplate*>(first.second->getTemplate());
    check(outgoing.m_objectNetId==(objectTemplate->m_attachToEventObject ? 0x1234 : 0),
        "event constructor preserves the template-controlled attachment id");
    io::BitStream encoded(64);
    check(outgoing.serialize(&encoded), "real event serialization succeeds");
    int type=0; encoded.readBits(&type,gameEventTypeIdBitCount());
    Event roundtrip;
    check(type==34 && roundtrip.deSerialize(&encoded) && roundtrip.m_area==first.second
        && roundtrip.m_areaId==unsigned(first.first) && roundtrip.m_team==0xA3
        && roundtrip.m_objectNetId==outgoing.m_objectNetId
        && roundtrip.m_position.x==1.0f && roundtrip.m_position.y==-2.0f && roundtrip.m_position.z==3.5f,
        "event roundtrip resolves the indexed area and preserves all payload fields");

    // Independently authored little-endian payload: id16/team8/netid16/x32/y32/z32.
    unsigned char golden[17]={0,0,0xA3,0x34,0x12,0,0,0x80,0x3F,0,0,0,0xC0,0,0,0x60,0x40};
    golden[0]=static_cast<unsigned char>(first.first);
    golden[1]=static_cast<unsigned char>(first.first>>8);
    io::BitStream known(golden,sizeof(golden)); known.setBuffer(golden,sizeof(golden));
    Event decoded;
    check(decoded.deSerialize(&known) && decoded.m_area==first.second && decoded.m_team==0xA3
        && decoded.m_objectNetId==0x1234 && decoded.m_position.x==1.0f
        && decoded.m_position.y==-2.0f && decoded.m_position.z==3.5f,
        "independent event payload resolves the area between ids and position");
    golden[0]=0xFE; golden[1]=0xFF; known.setBuffer(golden,sizeof(golden));
    check(decoded.deSerialize(&known) && !decoded.m_area && decoded.m_position.z==3.5f,
        "unknown area id leaves a null object and still consumes the position");
    known.setBuffer(golden,5);
    check(!decoded.deSerialize(&known) && !known.hasError(),
        "truncated event returns failure and preserves retail error clearing");
}
void triggerChecks()
{
    TriggerState& s=triggers();
    s.production=triggerManager; s.triggerables=triggerableManager;
    s.initial=s.adds; s.checksAtSetup=s.areaChecks;
    check(s.production && s.triggerables && !s.live.empty(),
        "GameLogic publishes both managers and stock init registers triggers");
    unsigned count=0, valid=0;
    for (const auto& row:objectManager->getAllRegisteredObjects()) {
        if (row.second->getTemplate()->getClassID()!=CID_TriggerTemplate) continue;
        ++count;
        Trigger* trigger=static_cast<Trigger*>(row.second);
        if (trigger->isValid()) ++valid;
        check(s.live.count(trigger)!=0,"stock trigger is registered");
    }
    check(count==s.live.size() && valid>0,"stock link setup finds ambient effectors");
    {
        TriggerableManager local;
        std::vector<IObject*> output;
        check(triggerableManager==s.triggerables && !local.getTriggerablesWithTriggerId(-1,output),
            "secondary triggerable manager preserves owner and has an empty query");
        output.push_back(nullptr);
        check(local.getTriggerablesWithTriggerId(-1,output) && output.size()==1,
            "triggerable query appends and reports the whole output");
        check(!local.getTriggerableFromId(-1) && !local.getTriggerableFromId(123)
            && !local.getTriggerableTargetByName("absent") && !local.alreadyActive(nullptr),
            "empty triggerable lookups do not insert missing entries");
        local.handleUpdate(0.01f,0); local.refresh(); local.reset();
        check(!local.getTriggerableFromId(-1),"empty update refresh and reset remain callable");
    }
    triggerableManager=nullptr;
    {
        TriggerableManager owner;
        {
            TriggerableManager second;
            check(triggerableManager==&owner,"triggerable publication is first-wins");
        }
        TriggerableTemplate t;
        check(t.m_types==3 && t.m_radius==5 && t.m_triggerId==-1
            && t.m_float190==100 && t.m_float194==10 && t.m_float198==1
            && !t.m_bool19C && !t.m_bool19D && !t.m_bool19E,
            "triggerable template constructor supplies recovered defaults");
        owner.reset(); // non-owning template index cleared before the stack template dies
    }
    check(!triggerableManager,"owning triggerable destructor clears its alias");
    triggerableManager=s.triggerables;

    {
        TriggerManager local;
        check(triggerManager==s.production,"secondary trigger manager preserves owner");
        TriggerTemplate t;
        t.m_radius=5; t.m_events=24; t.m_triggerId=123;
        t.m_chance=1; t.m_minimumTimeBetweenTriggering=0;
        TestTrigger a(&t), b(&t), c(&t), d(&t), e(&t);
        for (Trigger* trigger:{&a,&b,&c,&d,&e}) local.addTrigger(trigger);
        s.recording=true;
        local.handleUpdate(0.01f,0);
        check(s.checked==std::vector<Trigger*>({&a,&b,&c}),"update inspects only the first three candidates");
        s.checked.clear(); local.handleUpdate(0.01f,0);
        check(s.checked==std::vector<Trigger*>({&d,&e}),"update stops at the end without wrapping mid-pass");
        s.checked.clear(); local.handleUpdate(0.01f,0);
        check(s.checked==std::vector<Trigger*>({&a,&b,&c}),"the next update wraps its cursor at entry");
        local.addTrigger(&b); local.removeTrigger(&b);
        s.checked.clear(); local.handleUpdate(0.01f,0);
        check(s.checked==std::vector<Trigger*>({&a,&c,&d}),"remove erases every inactive duplicate and resets the cursor");
        local.removeAll(); s.checked.clear(); local.handleUpdate(0.01f,0);
        check(s.checked.empty(),"removeAll clears the candidate list");

        local.addTrigger(&a);
        const Vec3 origin=a.getAbsolutePosition();
        Vec3 boundary={origin.x+5,origin.y,origin.z};
        Vec3 inside={origin.x+4,origin.y,origin.z};
        check(a.isInArea(inside) && !a.isInArea(boundary),"trigger sphere excludes its exact radius boundary");
        b.setAbsolutePosition(boundary);
        s.events.clear(); local.handleEvent(TriggerType_Object,&b,123);
        check(s.events.empty(),"object event outside the sphere is filtered");
        local.handleEvent(TriggerType_ControlPoint,&b,123);
        check(s.events==std::vector<Trigger*>({&a}),"control-point event bypasses the distance filter");
        s.events.clear(); local.handleEvent(TriggerType_ControlPoint,&b,124);
        local.handleEvent(TriggerType_ControlPoint,nullptr,-1);
        local.handleEvent(TriggerType_Weapon,&b,-1);
        check(s.events.empty(),"mismatched ids null sources and inactive weapon candidates do not dispatch");
        b.setAbsolutePosition(origin);
        local.handleEvent(TriggerType_Object,&b,-1);
        check(s.events==std::vector<Trigger*>({&a}),"wildcard id dispatches an in-range object event");
        t.m_events=0; s.events.clear(); local.handleEvent(TriggerType_Object,&b,-1);
        check(s.events.empty(),"event type mask is required");

        const unsigned now=WorldPref::getTickCount();
        a.m_lastTriggerTick=int(now-1); t.m_minimumTimeBetweenTriggering=1000;
        a.updateOnEvent(TriggerType_Object,static_cast<IObject*>(&b));
        check(unsigned(a.m_lastTriggerTick)==now-1,"minimum interval prevents execution");
        t.m_minimumTimeBetweenTriggering=std::numeric_limits<float>::quiet_NaN();
        a.updateOnEvent(TriggerType_Object,static_cast<IObject*>(&b));
        check(unsigned(a.m_lastTriggerTick)==now,"NaN minimum permits the retail event path");
        t.m_minimumTimeBetweenTriggering=1;
        a.m_lastTriggerTick=int(now+1); a.updateOnEvent(TriggerType_Object,static_cast<IObject*>(&b));
        check(unsigned(a.m_lastTriggerTick)==now,"event interval subtraction wraps as unsigned");
        a.m_lastTriggerTick=int(now-1); a.m_lastTestTick=int(now+1);
        a.handleTrigger(nullptr,origin);
        check(unsigned(a.m_lastTriggerTick)==now-1,"time probability gate rejects a negative signed tick delta");
        a.m_lastTestTick=int(now-1); a.handleTrigger(nullptr,origin);
        check(unsigned(a.m_lastTriggerTick)==now,"positive elapsed time and certain chance execute");
        IObject* weapon=nullptr;
        for (const auto& row:objectManager->getAllRegisteredObjects())
            if (row.second->queryInterface(IID_IWeaponObject)) { weapon=row.second; break; }
        t.m_minimumTimeBetweenTriggering=0; a.m_lastTriggerTick=int(now-1);
        if (weapon) a.updateOnEvent(TriggerType_Weapon,weapon);
        check(weapon && unsigned(a.m_lastTriggerTick)==now,
            "type 2 reads the real stock weapon's player interface before execution");
        t.m_events=8; s.events.clear();
        triggerManager=&local;
        GameLogicExplosion::handleExplosion(origin,1,0,0,nullptr,nullptr,0,nullptr,nullptr,0,&b,nullptr,0,false,false);
        triggerManager=s.production;
        check(s.events==std::vector<Trigger*>({&a}),
            "real explosion callback forwards its source object to the trigger manager");
        a.m_lastTriggerTick=int(now-1); a.executeTrigger(nullptr,origin,0);
        check(unsigned(a.m_lastTriggerTick)==now,"an explicit effector index still stamps the trigger clock");

        // Actual indexed area; linking is tested without firing its remaining effect frontier.
        t.m_triggerId=state().live.begin()->second->getTriggerId();
        a.findEffectors(); const auto links=a.m_effectors;
        a.findEffectors();
        auto doubled=links; doubled.insert(doubled.end(),links.begin(),links.end());
        check(!links.empty() && a.m_effectors==doubled,"repeated link setup appends duplicate effectors");
        TriggerTemplate missing; missing.m_triggerId=-123456;
        TestTrigger invalid(&missing);
        local.removeAll(); local.addTrigger(&a); local.addTrigger(&invalid);
        local.setupTriggerLinks(); s.checked.clear(); local.handleUpdate(0.01f,0);
        check(s.checked==std::vector<Trigger*>({&a}),"link setup prunes invalid triggers before updating");
        local.removeAll(); s.recording=false;
        // Object's retail destructor requires destroy first, even for stack fixtures.
        for (Trigger* trigger:{&a,&b,&c,&d,&e,&invalid}) trigger->destroy(false);
    }
    triggerManager=nullptr;
    { TriggerManager owner; { TriggerManager later; }
      check(triggerManager==&owner,"secondary trigger destruction preserves first publication"); }
    check(!triggerManager,"owning trigger destructor clears its alias");
    triggerManager=s.production;
}
}
void ambientEventObserve(GameEvent* event)
{
    if (!recordEffects || event->getType()!=34) return;
    io::BitStream stream(64);
    check(event->serialize(&stream),"outgoing ambient event serializes at the real GameServer send entry");
    int type=0; stream.readBits(&type,gameEventTypeIdBitCount());
    Event decoded;
    check(type==34 && decoded.deSerialize(&stream) && decoded.m_area,
        "outgoing ambient event decodes through the active area index");
    effectEvents.push_back(EffectPayload{decoded.m_areaId,decoded.m_team,decoded.m_objectNetId,decoded.m_position});
}
void triggersObserve(const char* operation,Trigger* trigger,unsigned)
{
    TriggerState& s=triggers();
    if (!std::strcmp(operation,"add")) { ++s.adds; s.live.insert(trigger); }
    else if (!std::strcmp(operation,"remove")) {
        if (s.live.erase(trigger)) ++s.removes;
    } else if (!std::strcmp(operation,"check")) {
        if (s.live.count(trigger)) ++s.areaChecks;
        if (s.recording) s.checked.push_back(trigger);
    } else if (s.recording && !std::strcmp(operation,"event")) s.events.push_back(trigger);
}
void managersObserve(const char* operation, AmbientEffectAreaManager* manager,
    AmbientEffectArea* area, int id, unsigned size, unsigned ids)
{
    State& s=state();
    if (!std::strcmp(operation,"construct") && !s.production) {
        s.production=manager;
        check(ambientEffectAreaManager==manager && !size && !ids,
            "GameLogic constructs and publishes an empty ambient manager");
    }
    if (manager!=s.production) return;
    if (!std::strcmp(operation,"add")) {
        ++s.adds; s.live[id]=area;
        check(ambientEffectAreaManager->getAreaFromId(id)==area && size==s.live.size() && ids==size,
            "stock init registers the area in both indexes");
    } else if (!std::strcmp(operation,"remove")) {
        ++s.removes; s.live.erase(id);
        check(!ambientEffectAreaManager->getAreaFromId(id) && size==s.live.size() && ids==size,
            "stock area destructor removes both index entries");
    } else if (!std::strcmp(operation,"clear")) {
        ++s.clears;
        check(s.live.empty() && !size && !ids && id==0,
            "unload destroys areas before clearing indexes and resetting ids");
    } else if (!std::strcmp(operation,"destruct")) {
        s.destroyed=true;
        check(!ambientEffectAreaManager && s.live.empty() && !size && !ids,
            "GameLogic deletes the empty manager and its singleton is cleared");
    }
}
bool managersSetup()
{
    snapshot();
    State& s=state(); s.initial=s.adds;
    if (!s.live.empty()) registryChecks();
    if (!s.live.empty()) triggerChecks();
    if (!s.live.empty()) effectChecks();
    return s.passed;
}
void managersFrame(int frame)
{
    State& s=state(); const unsigned tick=WorldPref::getTickCount();
    if (frame>360 && tick<s.previousTick) s.sawTickReset=true;
    s.previousTick=tick;
}
bool managersResult()
{
    snapshot(); State& s=state();
    if (std::getenv("BF2_TIER3_RELOAD"))
        check(s.sawTickReset && s.removes>=s.initial && s.adds>=2*s.initial && s.clears
            && gameLogic->isLevelLoaded() && gameLogic->getGameStatus()==1,
            "same-map reload re-registers areas and returns to playing");
    TriggerState& t=triggers();
    check(t.areaChecks>t.checksAtSetup && triggerManager==t.production
        && triggerableManager==t.triggerables,"stock ticks drive the trigger area queries with both managers alive");
    if (std::getenv("BF2_TIER3_RELOAD"))
        check(t.removes>=t.initial && t.adds>=2*t.initial && t.live.size()==t.initial,
            "same-map reload balances old triggers and registers the new instances");
    return s.passed;
}
bool managersShutdown()
{
    State& s=state();
    check(s.destroyed && !ambientEffectAreaManager && s.adds==s.removes,
        "shutdown balances all registrations and destroys the manager");
    TriggerState& t=triggers();
    check(!triggerManager && t.live.empty() && t.adds==t.removes,
        "shutdown destroys the trigger manager after all trigger objects");
    check(triggerableManager==t.triggerables,"GameLogic preserves retail's omission of triggerable-manager deletion");
    std::printf("[tier3-triggers-result] adds=%u removes=%u areaChecks=%u\n",t.adds,t.removes,t.areaChecks);
    std::printf("[tier3-managers-result] checks=%u adds=%u removes=%u clears=%u PASS=%d\n",
        s.checks,s.adds,s.removes,s.clears,s.passed);
    return s.passed;
}
