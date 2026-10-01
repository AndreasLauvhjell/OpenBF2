// [TEMP] Real HUD orders and native SquadOrder/Commander storage. Typed external
// observers exercise non-map paths; no map constructor or order body is replaced.
#define main primaryActionsRegressionMain
#include "hud_primary_actions_runtime.cpp"
#undef main
#include <cmath>
#include "../BF2/Menu/HudInformationAdapter.h"
namespace dice { namespace hfe { IBase* unresolvedGlobalA33C08=nullptr; } }
void testSquadReply(BF2HudManager&,int,std::string,bool);
void testNoPlayerResponse(BF2HudManager&);
struct OrderHud : CommandHud {
    std::function<void()> onMap;
    HudMap* getMap() override {if(onMap)onMap();std::abort();}
    std::wstring getLocalizedString(std::string key) override {
        localized.push_back(key);return std::wstring(key.begin(),key.end());
    }
};
struct ZoomBoundaryReached {};
struct OrderAdapter : HudInformationAdapter {
    std::function<void()> onSet;
    bool setInt(std::string key,int value) override {
        check(key=="MapZoom"&&value==-7,"adapter preserves signed zoom and owned key");
        trace.push_back("setZoom");if(onSet)onSet();return false;
    }
};
struct OrderPlayer : CommandPlayer {
    mutable unsigned commanderReads=0,aliveReads=0;
    mutable std::deque<bool> aliveResults;
    std::function<void()> aliveCallback;
    bool getIsCommander() const override {++commanderReads;trace.push_back("commander");return commanderFlag;}
    bool getIsAlive() const override {
        ++aliveReads;trace.push_back("alive");if(aliveCallback)aliveCallback();
        bool result=alive;if(!aliveResults.empty()){result=aliveResults.front();aliveResults.pop_front();}return result;
    }
    int getSquadId() const override {trace.push_back("squad");return 7;}
    int getTeam() const override {trace.push_back("team");return CommandPlayer::getTeam();}
};
struct OrderPlayers : Players {
    int counts[10]={};
    std::vector<int> checkedSquads;
    int getNrOfPlayersInSquad(int team,int squad) override {
        check(team==2,"order count uses local team");checkedSquads.push_back(squad);return counts[squad];
    }
};
struct OrderSquads : ActionSquadBoundary {
    bool setSquadName(int,int,int,const std::string&,bool) override {std::abort();}
    void inviteToSquad(int,int,int,bool,bool) override {std::abort();}
    void removeFromSquad(int,int,int,bool) override {std::abort();}
    struct Sent {int team,squad;SquadOrder value;bool flag;int kind;};
    std::vector<Sent> sent;
    SquadOrder* expected=nullptr;
    int commanderId=9,commanderQueries=0,leaderQueries=0;
    std::function<void()> onSend,onCommander;
    void record(int team,int squad,SquadOrder& order,bool flag,int kind) {
        check(&order==expected,"order callback receives the actual embedded member");
        check(!flag,"order wire-local flag false");sent.push_back({team,squad,order,flag,kind});
        if(onSend)onSend();
    }
    void issueSquadOrder(int team,int squad,SquadOrder& order,bool flag) override {record(team,squad,order,flag,0);}
    void issueCommanderOrder(int team,int squad,SquadOrder& order,bool flag) override {record(team,squad,order,flag,1);}
    void issueRequest(int team,int squad,SquadOrder& order,bool flag) override {record(team,squad,order,flag,2);}
    int getCommander(int team) override {check(team==2,"commander query team");++commanderQueries;if(onCommander)onCommander();return commanderId;}
    int getSquadLeader(int team,int squad) override {check(team==2&&squad==-3,"reply forwards signed squad");++leaderQueries;return -1;}
};
struct OrderGame : CommandGame {
    std::string gameMode="gpm_cq";
    std::function<void()> onMode;
    unsigned radios=0;
    bool lastAnnounce=false;
    std::string radioText;
    const std::string& getCurrentMapGameMode() const override {trace.push_back("gameMode");if(onMode)onMode();return gameMode;}
    bool radioMessage(TargetChannel channel,const std::string& message,world::IPlayer* player,unsigned leader,bool announce,bool local) override {
        check(channel==14&&player!=nullptr&&leader==0xffffffffu&&!local,"reply radio arguments and signed leader conversion");
        ++radios;lastAnnounce=announce;radioText=message;return false;
    }
};
struct OrderEvents : EventManager {
    std::vector<unsigned> squads,middle;
    unsigned int postEvent(EventCategory category,unsigned int id,void* data,float delay) override {
        check(category==ECHud&&id==25&&delay==0,"commander order synchronous HUD notification");
        auto* words=static_cast<unsigned*>(data);squads.push_back(words[0]);middle.push_back(words[1]);
        check(words[2]==0,"commander order event resets type word");words[1]=0x12345678;words[2]=99;return 0;
    }
};
int main() {
    if(primaryActionsRegressionMain())return 1;
    checks=failures=0;trace.clear();
    OrderEvents events;eventManager=&events;
    OrderPlayers players;world::playerManager=&players;
    OrderPlayer player;players.local=&player;
    OrderGame current;game=&current;
    OrderSquads squads;squadManager=&squads;
    void* storage=::operator new(sizeof(OrderHud));std::memset(storage,0x5a,sizeof(OrderHud));
    OrderHud* hud=new(storage)OrderHud;hudManager=hud;squads.expected=&hud->m_order;
    HudCommander commander(nullptr);hud->m_field13c=&commander;hud->m_field0f8=nullptr;
    check(sizeof(SquadOrder)==48&&sizeof(BF2HudManager)==824,"embedded order preserves both native sizes");
    check(hud->m_order.orderId==0x5a5a5a5a&&hud->m_order.targetNetId==0x5a5a,"constructor preserves untouched order scalars");
    check(reinterpret_cast<unsigned char*>(&hud->m_order)[44]==0x5a&&hud->m_order.points[0].x==0&&hud->m_order.points[2].z==0,"constructor initializes only the three order vectors");
    hud->m_order.points[0]={1,2,3};hud->m_order.points[1]={4,5,6};hud->m_order.points[2]={7,8,9};
    players.local=nullptr;hud->sendOrder(2,true);
    check(squads.sent.empty()&&hud->localized.back()=="HUD_HUD_SQUAD_ALIVE_TO_ORDER","missing player order notice precedes map path");
    players.local=&player;player.commanderFlag=false;trace.clear();hud->m_order.targetNetId=60000;
    hud->sendOrder(-8,false);
    check(squads.sent.size()==1&&squads.sent.back().kind==0&&squads.sent.back().team==2&&squads.sent.back().squad==7,"ordinary squad order recipient and dispatch");
    check(hud->m_order.orderId==-8&&hud->m_order.targetNetId==0&&hud->m_order.flag2C,"negative type retained and ordinary order accepted");
    check(trace==std::vector<std::string>{"player","commander","commander","squad","team"},"discarded commander query and squad-before-team ordering");
    check(hud->m_order.points[1].y==5&&hud->m_order.points[2].z==9,"only first order vector participates in validity");
    check(reinterpret_cast<unsigned char*>(&hud->m_order.targetNetId)[2]==0x5a,"order target clear preserves padding");
    auto reads=player.commanderReads;hud->sendOrder(10,false);
    check(squads.sent.size()==1&&hud->m_order.orderId==4&&!hud->m_order.flag2C&&hud->m_order.points[0].x==-4000,"reset order writes invalid sentinel and sends nothing");
    check(player.commanderReads==reads+1,"invalid order stops before second commander query");
    hud->m_order.points[0]={-4000,-4000,std::nextafter(-4000.0f,0.0f)};hud->sendOrder(9,false);
    check(squads.sent.size()==2,"one float ULP from sentinel is valid");
    hud->m_order.points[0]={std::numeric_limits<float>::quiet_NaN(),-4000,-4000};hud->sendOrder(9,false);
    check(squads.sent.size()==3,"unordered position does not match invalid sentinel");
    player.commanderFlag=true;hud->m_order.points[0]={1,2,3};squads.sent.clear();
    commander.m_selectedSquads.reset();commander.m_selectedSquads.set(2);commander.m_selectedSquads.set(4);commander.m_selectedSquads.set(8);
    players.counts[2]=2;players.counts[4]=0;players.counts[8]=-1;players.counts[9]=1;
    squads.onSend=[&]{commander.m_selectedSquads.set(9);hud->m_order.orderId=6;};
    hud->sendOrder(3,false);
    check(squads.sent.size()==2&&squads.sent[0].squad==2&&squads.sent[1].squad==9,"commander skips empty/negative counts and reloads live selection");
    check(!squads.sent[0].value.flag2C&&squads.sent[1].value.orderId==6,"commander flag false and callback order mutation reaches later squad");
    check(events.squads==std::vector<unsigned>{2,9}&&events.middle==std::vector<unsigned>{0xC57A0000u,0x12345678u},"notification retains stale comparison word and callback mutation");
    squads.onSend=nullptr;commander.m_selectedSquads.reset();auto notifications=events.squads.size();hud->sendOrder(3,false);
    check(hud->localized.back()=="HUD_HUD_SQUAD_NO_SQUAD_SELECTED"&&events.squads.size()==notifications,"no selected squad gives localized notice");
    hud->m_field13c=nullptr;hud->sendOrder(2,false);
    check(squads.sent.back().kind==0&&hud->m_order.flag2C,"commander without panel falls back to ordinary squad order");
    player.commanderFlag=false;squads.sent.clear();players.local=nullptr;hud->sendRequest(1,true);
    check(hud->localized.back()=="HUD_HUD_SQUAD_ALIVE_TO_REQUEST"&&squads.sent.empty(),"missing player request notice");
    players.local=&player;player.alive=false;hud->sendRequest(1,false);
    check(squads.sent.empty(),"dead player request stops");
    player.alive=true;current.gameMode="GPM_COOP";squads.commanderId=-1;auto queries=squads.commanderQueries;
    hud->m_order.points[0]={1,2,3};hud->sendRequest(-4,false);
    check(squads.commanderQueries==queries&&squads.sent.back().kind==2&&hud->m_order.orderId==-4&&hud->m_order.flag2C,"case-insensitive coop bypasses commander query");
    current.gameMode="gpm_cq";squads.sent.clear();hud->sendRequest(1,false);
    check(squads.sent.empty()&&current.notice=="no_commander_available","noncoop requires commander or captured AI pointer");
    current.onMode=[&]{unresolvedGlobalA33C08=&squads;};hud->sendRequest(1,false);
    check(squads.sent.empty(),"AI pointer captured before game-mode callback");
    current.onMode=[&]{unresolvedGlobalA33C08=nullptr;};hud->sendRequest(1,false);
    check(squads.sent.size()==1,"captured AI pointer survives callback clear");current.onMode=nullptr;
    squads.commanderId=4;player.aliveResults={true,false};squads.sent.clear();hud->m_order.targetNetId=1234;hud->sendRequest(2,false);
    check(squads.sent.empty()&&hud->m_order.targetNetId==1234,"second alive query prevents state writes");
    hud->m_order.flag2C=true;hud->sendRequest(99,false);
    check(hud->m_order.targetNetId==0&&hud->m_order.orderId==0&&hud->m_order.flag2C&&squads.sent.empty(),"request reset preserves accepted byte while invalidating position");
    hud->m_order.points[0]={1,2,3};trace.clear();hud->sendRequest(8,false);
    check(squads.sent.size()==1&&squads.sent[0].squad==7&&squads.sent[0].kind==2,"ordinary live request dispatch");
    check(trace==std::vector<std::string>{"player","alive","gameMode","team","alive","squad","team"},"request repeated alive and query ordering");
    queries=players.queries;hud->accept(true);check(players.queries==queries,"accept with missing map skips local player query");
    players.local=nullptr;testNoPlayerResponse(*hud);check(players.queries==queries+1,"response helper missing player stops before map use");
    testSquadReply(*hud,-3,"message",true);check(current.radios==0,"reply missing player stops");
    players.local=&player;testSquadReply(*hud,-3,"",true);check(squads.leaderQueries==0,"empty reply still queries player but skips leader");
    testSquadReply(*hud,-3,std::string("\0body",5),true);
    check(current.radios==1&&current.radioText.size()==5&&current.lastAnnounce,"reply tests native string length and forwards announce");
    testSquadReply(*hud,-3,"deny",false);check(current.radios==2&&!current.lastAnnounce,"denial reply announce false");
    {
        OrderAdapter adapter;
        trace.clear();hudManager=nullptr;
        adapter.onSet=[&]{hudManager=hud;};
        hud->onMap=[&]{trace.push_back("getMap");throw ZoomBoundaryReached{};};
        bool reached=false;
        try {adapter.setMapZoom(-7);} catch(const ZoomBoundaryReached&) {reached=true;}
        check(reached&&trace==std::vector<std::string>{"setZoom","getMap"},"adapter ignores failed setInt and reloads HUD after callback");
        hud->onMap=nullptr;
        // The real body is deliberately stopped by a typed external observer
        // before it needs a map lifetime. No map object or state is fabricated.
    }
    delete hud->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
    hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();::operator delete(storage);hudManager=nullptr;
    std::printf("HUD order/request bodies: %u/%u checks passed\n",checks-failures,checks);return failures?1:0;
}
