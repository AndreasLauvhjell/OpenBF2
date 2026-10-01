// [TEMP] Native launchArtillery/event constructors with typed callback observers.
// Only the explicit undefined AI courier seam is implemented by this fixture;
// it observes an undereferenced identity token, not an invented courier object.
#pragma push_macro("main")
#undef main
#define main unusedAbilityRegression
#include "hud_ability_actions_runtime.cpp"
#undef main
#pragma pop_macro("main")
#include "../BF2/Menu/HudArtillerySeams.h"
#include "../BF2/Game/GameEvents/ArtilleryEvent.h"
#include "../BF2/AI/AIInterface/AIInstance.h"
#include "../BF2/AI/AIInterface/SingletonServer.h"

static ai::IAIEventCourier* expectedCourier;
static std::vector<std::pair<int,Vec2>> courierCalls;
namespace dice { namespace hfe { namespace ai {
IBase* g_aiInstance = nullptr;
// [TEMP] [CHOSEN, not recovered] observer for HUD-ARTILLERY-COURIER only.
void dispatchArtilleryRequest(IAIEventCourier* receiver,int team,Vec2 target) {
    check(receiver==expectedCourier,"courier receiver captured before callbacks");
    trace.push_back("courier");courierCalls.emplace_back(team,target);
}
} } }
struct ArtilleryAI : ai::IAIInstance {
    bool initialized=false;
    std::function<void()> onRead;
    ~ArtilleryAI() override {}
    bool init() override {actionUnsupported("AI init");}
    bool clear() override {actionUnsupported("AI clear");}
    bool shutdown() override {actionUnsupported("AI shutdown");}
    int getSettingsFromMenu(bool) override {actionUnsupported("AI settings");}
    int update(double) override {actionUnsupported("AI update");}
    bool isInitialized() const override {trace.push_back("AI ready");if(onRead)onRead();return initialized;}
    bool isShuttingDown() const override {actionUnsupported("AI shutdown query");}
};
struct ArtilleryHud : CommandHud {
    bool enabled=true;
    float availability=1;
    std::deque<Vec2> positions;
    std::function<void()> onPosition,onEnabled,onAvailability;
    bool getArtilleryEnabled() override {trace.push_back("enabled");if(onEnabled)onEnabled();return enabled;}
    float getArtilleryAvailability() override {trace.push_back("availability");if(onAvailability)onAvailability();return availability;}
    Vec2 getCommanderPosition(bool relative) override {
        check(!relative,"artillery position is absolute");trace.push_back("position");
        if(positions.empty())actionUnsupported("artillery position reply missing");
        const Vec2 result=positions.front();positions.pop_front();if(onPosition)onPosition();return result;
    }
};
struct ArtilleryGame : AbilityActionGame {
    std::vector<std::string> messages;
    bool localGameInfoMessage(TargetChannel c,const std::string& message) override {
        check(c==2,"artillery failure channel two");messages.push_back(message);trace.push_back(message);return false;
    }
};
struct ArtilleryLogic : AbilityActionLogic {
    bool isServer() const override {trace.push_back("server");return server;}
};
struct ArtilleryEvents : AbilityActionEvents {
    std::function<void()> onPost;
    unsigned postEventNextUpdate(EventCategory category,unsigned id,void* data) override {
        auto result=AbilityActionEvents::postEventNextUpdate(category,id,data);if(onPost)onPost();return result;
    }
};
struct ArtillerySound : Sound {
    std::function<void()> onPlay;
    void playSound(const std::string& value) override {
        check(value=="mapAction","artillery success sound");trace.push_back("sound");
        Sound::playSound(value);if(onPlay)onPlay();
    }
};
static bool samePosition(const Vec2& a,const Vec2& b) {return std::memcmp(&a,&b,sizeof a)==0;}
static Vec2 position(float x,float y) {Vec2 value;value.x=x;value.y=y;return value;}

int main() {
    setvbuf(stdout,nullptr,_IONBF,0);checks=failures=0;
    std::puts("Artillery fixture: real constructor and native HUD callbacks");
    static_assert(sizeof(ArtilleryEvent)==36,"native event allocation");
    alignas(ArtilleryEvent) unsigned char bytes[sizeof(ArtilleryEvent)];
    std::memset(bytes,0xa5,sizeof bytes);
    Vec2 target;target.x=-0.0f;target.y=std::numeric_limits<float>::quiet_NaN();
    auto* direct=new(bytes)ArtilleryEvent(0x123,target,0xfedcba98u);
    check(direct->getType()==30&&direct->m_team==0x23&&direct->m_artilleryType==1,"request constructor type and low-byte team");
    check(samePosition(direct->m_target,target),"request constructor copies raw Vec2 bits");
    bool untouched=true;for(unsigned i=0x15;i<0x1c;++i)untouched&=bytes[i]==0xa5;
    for(unsigned i=0x20;i<sizeof bytes;++i)untouched&=bytes[i]==0xa5;
    check(untouched,"request constructor preserves fire tick net id and padding");direct->~ArtilleryEvent();
    std::memset(bytes,0x6b,sizeof bytes);direct=new(bytes)ArtilleryEvent;
    check(direct->m_target.x==0&&direct->m_target.y==0,"default constructor still zeros only target payload");
    untouched=true;for(unsigned i=0x14;i<sizeof bytes;++i)untouched&=bytes[i]==0x6b;
    check(untouched,"default constructor preserves remaining payload");direct->~ArtilleryEvent();

    ArtilleryEvents events;eventManager=&events;
    Players players;world::playerManager=&players;
    CommandPlayer player;players.local=&player;
    ArtilleryGame current;game=&current;
    ArtilleryLogic logic;gameLogic=&logic;
    ArtillerySound sound;g_bfSound=&sound;
    auto* hud=new ArtilleryHud;hudManager=hud; // retained: wider HUD teardown is outside this fixture
    HudCommander commander(nullptr);hud->m_field13c=&commander;commander.m_artilleryReloading=false;
    ArtilleryAI ai;
    int courierToken=0,changedCourierToken=0;
    expectedCourier=reinterpret_cast<ai::IAIEventCourier*>(&courierToken);
    ai::BFSingletonServer::m_iaiEventCourier=expectedCourier;
    auto reset=[&] {
        trace.clear();hud->positions.clear();current.messages.clear();sound.played.clear();
        current.clearEvents();events.clearEvents();courierCalls.clear();
        players.local=&player;player.team=2;player.onTeam=nullptr;
        hud->enabled=true;hud->availability=1;hud->m_field13c=&commander;
        commander.m_artilleryReloading=false;hud->onEnabled=nullptr;hud->onAvailability=nullptr;hud->onPosition=nullptr;
        sound.onPlay=nullptr;events.onPost=nullptr;logic.server=false;ai::g_aiInstance=nullptr;
    };
    reset();players.local=nullptr;
    check(!hud->launchArtillery(false)&&trace==std::vector<std::string>{"player"},"null player stops before all readiness calls");
    for(bool silent:{false,true}) {
        reset();hud->enabled=false;
        check(!hud->launchArtillery(silent)&&current.messages.size()==(silent?0:1),"disabled failure honors silent");
        check(std::find(trace.begin(),trace.end(),"availability")==trace.end(),"disabled short circuits availability");
    }
    for(float value:{-1.0f,0.0f,-0.0f,std::numeric_limits<float>::quiet_NaN()}) {
        reset();hud->availability=value;
        check(!hud->launchArtillery(false)&&current.messages==std::vector<std::string>{"artillery_unavailable"},"nonpositive and unordered availability reject");
    }
    for(bool silent:{false,true}) {
        reset();hud->m_field13c=nullptr;
        check(!hud->launchArtillery(silent)&&current.messages.size()==(silent?0:1),"missing Commander busy failure honors silent");
        reset();commander.m_artilleryReloading=true;
        check(!hud->launchArtillery(silent)&&hud->positions.empty()&&sound.played.empty(),"reloading stops before position and sound");
        reset();hud->positions.push_back(position(-4000,-4000));
        check(!hud->launchArtillery(silent)&&current.messages.size()==(silent?0:1),"sentinel failure honors silent");
    }
    reset();hud->positions={position(1,2),position(3,4)};
    player.onTeam=[&]{trace.push_back("team");player.team=0x102;};
    check(hud->launchArtillery(true),"client launch succeeds");
    check(trace==std::vector<std::string>{"player","enabled","availability","position","sound","position","team","server","send"},"exact client callback sequence");
    auto* event=static_cast<ArtilleryEvent*>(current.events.back());
    check(event->m_team==2&&samePosition(event->m_target,position(3,4))&&event->m_artilleryType==1,"client event uses second position and captured player's team");
    check(commander.m_artilleryReloading==false,"launch does not invent a cooldown store");
    reset();hud->positions={position(std::numeric_limits<float>::quiet_NaN(),-4000),position(9,10)};
    check(hud->launchArtillery(false)&&current.events.size()==1,"unordered sentinel comparison accepts position");

    reset();logic.server=true;hud->positions={position(1,2),position(3,4)};
    check(hud->launchArtillery(false)&&events.posted.size()==1&&current.events.empty()&&courierCalls.empty(),"server posts without an AI singleton");
    reset();logic.server=true;ai.initialized=false;ai::g_aiInstance=reinterpret_cast<IBase*>(&ai);
    hud->positions={position(1,2),position(3,4)};
    check(hud->launchArtillery(false)&&events.posted.size()==1&&courierCalls.empty()&&trace.back()=="AI ready","server skips courier when AI uninitialized");
    reset();logic.server=true;ai.initialized=true;ai::g_aiInstance=reinterpret_cast<IBase*>(&ai);
    hud->positions={position(1,2),position(3,4),position(5,6)};
    player.onTeam=[&]{trace.push_back("team");++player.team;};
    check(hud->launchArtillery(false),"server AI branch succeeds with explicit courier observer");
    check(trace==std::vector<std::string>{"player","enabled","availability","position","sound","position","team","server","post","AI ready","position","team","courier"},"exact server AI callback sequence");
    event=static_cast<ArtilleryEvent*>(events.posted.back());
    check(event->m_team==3&&courierCalls.size()==1&&courierCalls[0].first==4&&samePosition(courierCalls[0].second,position(5,6)),"AI uses third position and repeated team after post");
    reset();logic.server=true;hud->positions={position(1,2),position(3,4),position(5,6)};
    events.onPost=[&]{ai::g_aiInstance=reinterpret_cast<IBase*>(&ai);players.local=nullptr;};
    unsigned positionCount=0;
    hud->onPosition=[&]{if(++positionCount==3)ai::BFSingletonServer::m_iaiEventCourier=reinterpret_cast<ai::IAIEventCourier*>(&changedCourierToken);};
    check(hud->launchArtillery(false)&&courierCalls.size()==1,"post callback installs AI and local player remains captured");
    check(ai::BFSingletonServer::m_iaiEventCourier!=expectedCourier,"third position callback changes global courier after receiver capture");
    reset();hud->positions={position(1,2),position(3,4)};
    sound.onPlay=[&]{hud->positions.front()=position(11,12);player.team=-1;};
    check(hud->launchArtillery(false),"sound callback can change later event inputs");
    event=static_cast<ArtilleryEvent*>(current.events.back());
    check(event->m_team==255&&samePosition(event->m_target,position(11,12)),"event reloads changed position and preserves negative team truncation");
    reset();ai::BFSingletonServer::m_iaiEventCourier=nullptr;hudManager=nullptr;
    std::printf("Artillery runtime: %u/%u checks passed\n",checks-failures,checks);
    return failures?1:0;
}
