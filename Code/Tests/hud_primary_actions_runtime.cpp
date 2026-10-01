// [TEMP] Primary HUD actions against real Commander/Squad storage and typed observers.
// The included independent regression keeps its original scope; no recovered action
// tested below is overridden. Wider /FORCE dependencies remain uncalled.
#pragma push_macro("main")
#undef main
#define main chatSendRegressionMain
#include "hud_chat_send_runtime.cpp"
#undef main
#pragma pop_macro("main")
#include <bitset>
#define private public
#include "../BF2/Menu/HudCommander.h"
#include "../BF2/Menu/HudSquad.h"
#undef private
#include "../BF2/Game/GameLogic/SquadManager/Squad.h"
namespace dice { namespace hfe {
IBase* g_hudInformationLayer = nullptr;
IBase* spawnManager = nullptr;
} }
struct CommandGame : Game {
    std::vector<int> flagValues;
    std::string notice;
    TargetChannel getCurrentChannel() override { return channel; }
    void setCurrentFlags(int value) override {flagValues.push_back(value);if(onFlags)onFlags();}
    bool localGameInfoMessage(TargetChannel c,const std::string& s) override {
        check(c==14,"commander notice channel");notice=s;return false;
    }
};
struct CommandHud : Hud {
    int creates=0,disabled=-1;
    std::vector<std::pair<int,bool>> transitions;
    bool soldierAlive=true;
    void setMode(int value,bool save) override {mode=value;transitions.push_back({value,save});}
    bool isPlayerAlive(world::IPlayer*) override {return soldierAlive;}
    void prepareSquadCreate(bool active) override {check(!active,"done closes create mode");++creates;}
    void disableVOHelpMessage(int id) override {disabled=id;}
};
struct CommandPlayer : Player {
    bool commanderFlag=false,leader=false;
    bool getIsCommander() const override {return commanderFlag;}
    bool getIsSquadLeader() const override {return leader;}
};
struct CommandSquads : ActionSquadBoundary {
    std::string name;int calls=0,lastTeam=-1,lastSquad=-1,lastId=-1;
    bool setSquadName(int team,int squad,int id,const std::string& value,bool flag) override {
        ++calls;lastTeam=team;lastSquad=squad;lastId=id;name=value;check(!flag,"create name flag false");return false;
    }
    void removeFromSquad(int,int,int,bool) override {actionUnsupported("remove squad");}
    void inviteToSquad(int,int,int,bool,bool) override {actionUnsupported("invite squad");}
    int getSquadLeader(int,int) override {actionUnsupported("squad leader");}
    int getCommander(int) override {actionUnsupported("commander id");}
};
int main() {
    setvbuf(stdout,nullptr,_IONBF,0);
    std::puts("Running existing chat regression");
    if(chatSendRegressionMain())return 1;
    checks=failures=0;
    EventManager events;eventManager=&events;
    Players players;world::playerManager=&players;
    CommandPlayer player;players.local=&player;
    CommandGame current;game=&current;
    CommandSquads squads;squadManager=&squads;
    void* storage=::operator new(sizeof(CommandHud));
    std::memset(storage,0x5a,sizeof(CommandHud));
    CommandHud* hud=new(storage)CommandHud;hudManager=hud;
    HudCommander commander(nullptr);HudSquad squad(nullptr);
    std::puts("Running squad sends");
    hud->m_field13c=&commander;hud->m_field138=&squad;
    dice::meme::StringData regular("hello"),command("orders");
    hud->m_field0a8[0]=&regular;hud->m_field0a8[1]=&command;hud->m_field0f4=nullptr;
    hud->commander=false;hud->sendSaySquad();
    check(current.channel==3&&current.sends==1&&current.flagValues.empty(),"ordinary squad chat chooses channel3 and preserves flags");
    check(current.text=="HUD_TEXT_CHAT_SQUADhello"&&hud->echoes==1,"squad literal prefix and local echo");
    hud->commander=true;commander.m_selectedSquads.set(2);commander.m_selectedSquads.set(8);
    current.sends=0;current.flagValues.clear();hud->sendSaySquad();
    check(current.channel==5&&current.sends==2&&current.flagValues==std::vector<int>{2,8},"commander sends only to selected squads in ascending order");
    check(current.text=="HUD_TEXT_CHAT_COMMANDERorders"&&hud->echoes==2,"commander prefix with one local echo");
    commander.m_selectedSquads.reset();current.sends=0;hud->sendSaySquad();
    check(current.sends==0&&hud->echoes==3,"no selected squads still gives a local echo");
    hud->m_field13c=nullptr;hud->sendSaySquad();
    check(current.sends==0&&hud->echoes==4,"null commander owner skips sends but preserves echo");
    command.setString(nullptr,"");auto queries=players.queries;hud->sendSaySquad();
    check(players.queries==queries+1&&current.channel==5&&current.types>=1,"empty squad send still commits channel/type before length gate");
    command.setString(nullptr,"orders");hud->m_field13c=&commander;
    std::puts("Running commander toggles");
    hud->commander=false;hud->mode=7;hud->toggleArtilleryState();hud->toggleSupplyState();hud->toggleVehicleDropState();
    check(hud->mode==7&&current.notice.empty(),"commander toggles no-op outside commander display");
    hud->commander=true;hud->mode=15;hud->setArtilleryEnabled(true);hud->setArtilleryAvailability(1);
    commander.m_artilleryReloading=false;hud->toggleArtilleryState();check(hud->mode==22,"ready artillery enters mode22");
    hud->toggleArtilleryState();check(hud->mode==15,"artillery toggle closes mode22");
    commander.m_artilleryReloading=true;hud->mode=19;hud->toggleArtilleryState();
    check(hud->mode==19&&current.notice=="artillery_in_use","busy artillery leaves current mode intact");
    hud->setArtilleryAvailability(0);hud->toggleArtilleryState();
    check(hud->mode==15&&current.notice=="artillery_unavailable","unavailable artillery returns mode15");
    hud->setArtilleryAvailability(std::numeric_limits<float>::quiet_NaN());commander.m_artilleryReloading=false;hud->toggleArtilleryState();
    check(hud->mode==22,"unordered artillery availability passes retail zero comparison");
    players.local=nullptr;hud->toggleArtilleryState();check(hud->mode==22,"artillery requires local player even when toggling closed");
    hud->mode=15;commander.m_supplyReloading=false;hud->toggleSupplyState();check(hud->mode==24,"supply toggle has no local-player guard");
    hud->toggleSupplyState();check(hud->mode==15,"supply closes mode24");
    commander.m_supplyReloading=true;hud->mode=19;hud->toggleSupplyState();
    check(hud->mode==15&&current.notice=="supplies_in_use","busy supplies explicitly restore mode15");
    commander.m_vehicleDropReloading=false;hud->toggleVehicleDropState();check(hud->mode==25,"drop enters mode25");
    hud->toggleVehicleDropState();check(hud->mode==15,"drop closes mode25");
    hud->m_field13c=nullptr;hud->toggleVehicleDropState();
    check(hud->mode==15&&current.notice=="vehicledrop_in_use","missing commander follows busy drop path");
    players.local=&player;hud->createSquad("Named");
    check(squads.calls==1&&squads.lastTeam==2&&squads.lastSquad==1&&squads.lastId==3&&squads.name=="Named","create invokes setSquadName with first empty squad and supplied name");
    hud->createSquad("");check(squads.calls==2&&squads.name=="Alpha","empty squad name uses the main HUD's retail default array");
    player.commanderFlag=true;hud->createSquad("ignored");check(squads.calls==2,"commander cannot create a squad");
    player.commanderFlag=false;players.local=nullptr;hud->createSquad("ignored");check(squads.calls==2,"missing local player cannot create a squad");
    std::puts("Running done and order actions");
    hud->mode=19;hud->setDone(false);check(!hud->m_field24a&&hud->mode==19&&hud->creates==0,"done false only stores done byte");
    hud->setDone(true);check(hud->m_field24a&&hud->mode==0&&hud->creates==1,"done true restores mode and closes actual squad create mode without local player");
    Logic logic;gameLogic=&logic;players.local=&player;HudKitSelection kit;hud->m_field11c=&kit;hud->m_field228=true;hud->m_field2ac=9.25f;
    hud->setDone(true);check(logic.requests==1&&logic.player==&player&&hud->m_field2a8==9.25f&&hud->disabled==60&&hud->creates==2,"done submits selected spawn group before help dismissal and squad closure");
    gameLogic=nullptr;
    SquadOrder order;std::memset(&order,0xa3,sizeof(order));order.orderId=-4;order.targetNetId=65530;order.flag2C=true;
    order.points[0]={1,2,3};order.points[1]={4,5,6};order.points[2]={7,8,9};
    hud->setSquadOrder(order);
    check(hud->m_order.orderId==-4&&hud->m_order.targetNetId==65530&&hud->m_order.flag2C&&hud->m_order.points[0].x==1&&hud->m_order.points[1].y==5&&hud->m_order.points[2].z==9,"squad order copies all typed fields");
    check(reinterpret_cast<unsigned char*>(&hud->m_order.targetNetId)[2]==0x5a&&reinterpret_cast<unsigned char*>(&hud->m_order.flag2C)[1]==0x5a,"squad order preserves destination padding");
    player.leader=true;hud->mode=18;hud->transitions.clear();hud->toggleSquadCommander();
    check(hud->transitions==std::vector<std::pair<int,bool>>{{17,false}},"leader closes commander panel without saving mode");
    player.leader=false;player.commanderFlag=true;hud->soldierAlive=false;hud->transitions.clear();hud->toggleSquadCommander();
    check(hud->transitions==std::vector<std::pair<int,bool>>{{15,false}},"alive commander without alive soldier selects mode15 without save");
    player.commanderFlag=false;hud->commander=true;hud->mode=19;hud->transitions.clear();hud->toggleSquadCommander();
    check(hud->transitions==std::vector<std::pair<int,bool>>{{15,true},{18,true}},"commander mode transition retains intermediate mode15");
    hud->commander=false;hud->mode=17;hud->transitions.clear();hud->toggleSquadCommander();
    check(hud->transitions==std::vector<std::pair<int,bool>>{{18,true}},"mode17 enters18 without commander query requirement");
    delete hud->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
    hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();::operator delete(storage);hudManager=nullptr;
    std::printf("HUD primary commander/squad actions: %u/%u checks passed\n",checks-failures,checks);return failures?1:0;
}
