// [TEMP] [CHOSEN, not recovered] Actual filter, Squad actions, native Radio and
// owner constructors. Typed collaborators observe callbacks. Encoded-value and
// question dispatch frontiers are never replaced or exercised.
#define main squadRegressionMain
#include "hud_squad_runtime.cpp"
#undef main
#include <cmath>
#include <limits>
#include "../BF2/Menu/HudAmmo.h"
#include "../BF2/Menu/HudInformationAdapter.h"
#include "../BF2/Menu/HudRadio.h"
#include "../BF2/Game/SingletonServer.h"
// Publish an explicit null voice client; unresolved data under /force could otherwise appear non-null.
namespace dice { namespace hfe { class BF2VoipClient; BF2VoipClient* bf2VoipClient = nullptr; } }
#include "../BF2/IO/Console/Console.h"
#include "../BF2/IO/Input/GameInput.h"
namespace dice { namespace hfe {
IGame* game=nullptr; IGameLogic* gameLogic=nullptr;
int consoleCharCount=0,consoleCharRead=0,consoleCharWrite=0;
INametagManager* SingletonServer::m_iNametagManager=nullptr;
namespace io { IBase* inputManager=nullptr; OldConsole* mainConsole=nullptr; }
} }
struct FilterPlayer : Player {
    int id=7,team=2,squad=0;bool commander=false,leader=false,alive=true;
    mutable int teamReads=0,squadReads=0;
    std::function<void()> onTeam;
    int getId() const override {trace+='I';return id;}
    int getTeam() const override {trace+='T';++teamReads;if(onTeam)onTeam();return team;}
    int getSquadId() const override {trace+='Q';++squadReads;return squad;}
    bool getIsCommander() const override {trace+='C';return commander;}
    bool getIsSquadLeader() const override {trace+='L';return leader;}
    bool getIsAlive() const override {trace+='A';return alive;}
    world::IObject* getVehicle() const override {trace+='V';return nullptr;}
};
struct FilterPlayers : Players {
    world::IPlayer* lookup=nullptr;
    HudSquad* owner=nullptr;
    int count=0,countCalls=0,lastLookup=0;
    world::IPlayer* getPlayerFromId(int id) override {
        trace+='K';lastLookup=id;
        if(owner)check(owner->m_field090==id,"player selection stored before lookup callback");
        return lookup;
    }
    int getNrOfPlayersInSquad(int team,int squad) override {
        trace+='N';++countCalls;check(team==2,"action forwards team");return count;
    }
};
struct FilterGame : ActionGameBoundary {
    bool demo=false;int reads=0;
    bool isPlayingDemo() override {trace+='D';++reads;return demo;}
    bool radioMessage(TargetChannel,const std::string&,world::IPlayer*,unsigned int,bool,bool) override {actionUnsupported("game radio");}
    void sendGameEvent(unsigned int,GameEvent*) override {actionUnsupported("game event");}
};
struct FilterLogic : ActionLogicBoundary {
    int status=1,reads=0;
    int getGameStatus() override {trace+='G';++reads;return status;}
    bool isServer() const override {actionUnsupported("logic isServer");}
};
struct MapBoundary {};
struct FilterHud : Hud {
    bool chat=false,rename=false,create=false,commander=false,order=false,playerState=false;
    bool process=false;
    int mode=0,previous=0,modeReads=0,previousReads=0,chatReads=0,renames=0,creates=0;
    int say=-1,radio=-1,processed=0;
    std::deque<int> modes;
    std::vector<std::pair<int,bool>> changes;
    std::vector<HudRadio*> resets;
    std::function<void()> onClose;
    bool isChatActive() override {trace+='c';++chatReads;return chat;}
    bool isSquadRenameActive() override {trace+='r';++renames;return rename;}
    bool isSquadCreateActive() override {trace+='n';++creates;return create;}
    int getMode() override {trace+='m';++modeReads;int v=mode;if(!modes.empty()){v=modes.front();modes.pop_front();}return v;}
    int getPreviousMode() override {trace+='p';++previousReads;return previous;}
    void setMode(int next,bool remember) override {trace+='M';changes.push_back({next,remember});mode=next;}
    bool closeChat() override {trace+='X';chat=false;if(onClose)onClose();return false;}
    bool isOrderMode() override {trace+='o';return order;}
    bool isCommanderShown() override {trace+='k';return commander;}
    bool isPlayerAlive(world::IPlayer*) override {trace+='a';return playerState;}
    bool updateRadioMode() override {trace+='u';return false;}
    void resetRadio(HudRadio* value) override {trace+='R';resets.push_back(value);}
    void openRadio(bool value) override {radio=value?11:10;trace+='O';}
    void openSquadRadio(bool value) override {radio=value?21:20;trace+='O';}
    void toggleSayAll(bool value) override {say=0;check(value==commander,"chat commander selector forwarded");}
    void toggleSayTeam(bool value) override {say=1;check(value==commander,"team commander selector forwarded");}
    void toggleSaySquad(bool value) override {say=2;check(value==commander,"squad commander selector forwarded");}
    bool processChatInput(io::GameInput&,int ignored,dice::meme::BfEditData* data) override {
        trace+='E';++processed;check(ignored==167&&data==nullptr,"chat process exact ignored key and chosen data");return process;
    }
    void prepareSquadRename(bool value) override {trace+='B';check(!value,"rename closes before submission");rename=value;}
    void submitSquadRename() override {trace+='J';}
    void setFlagE7(bool value) override {trace+='7';m_field0e7=value;}
    void disableVOHelpMessage(int id) override {trace+='H';check(id==12,"join disables help12");}
    std::wstring replaceWideTokenFromString(std::wstring text,std::string token,std::string value) override {
        trace+='W';check(text==L"localized"&&!token.empty()&&!value.empty(),"join warning replacement values");return text;
    }
    std::string getSquadEditText() override {trace+='g';return editText;}
    void submitSquadCreate() override {trace+='s';}
    HudMap* getMap() override {trace+='z';throw MapBoundary{};}
};
struct FilterInput : io::IInputManager {
    int delay=-1;
    void addRef() override {actionUnsupported("input addRef");}
    int getRef() override {actionUnsupported("input getRef");}
    int release() override {actionUnsupported("input release");}
    void* queryInterface(unsigned int) const override {actionUnsupported("input query");}
    bool init(unsigned int) override {actionUnsupported("input init");}
    bool shutdown() override {actionUnsupported("input shutdown");}
    bool setHasFocus(bool,unsigned int) override {actionUnsupported("input focus");}
    void update(int,float) override {actionUnsupported("input update");}
    bool registerInputFilter(io::IInputFilter*) override {actionUnsupported("input register");}
    bool unregisterInputFilter(io::IInputFilter*) override {actionUnsupported("input unregister");}
    bool setGameControlMap(std::string) override {actionUnsupported("input game map");}
    bool setPlayerControlMap(std::string) override {actionUnsupported("input player map");}
    std::string getPlayerControlMap() override {actionUnsupported("input map");}
    io::PlayerInput* getNewPlayerInputSample() override {actionUnsupported("input sample");}
    io::GameInput* getNewGameInputSample() override {actionUnsupported("input sample");}
    bool advanceNextUnread() override {actionUnsupported("input advance");}
    void catchUp() override {actionUnsupported("input catchup");}
    void setPlayerInputDelay(int value) override {trace+='d';delay=value;}
    int getNumEntries() override {actionUnsupported("input entries");}
};
struct FilterSquads : Squads {
    Info first,second;int queries=0,adds=0;
    ISquadInfo* getSquadInfo(int team,int squad) override {trace+='S';++queries;check(team==2,"join info team");return squad==1?&first:&second;}
    bool addToSquad(int team,int squad,int id) override {trace+='j';++adds;check(team==2&&squad==3&&id==7,"join exact identifiers");return true;}
};
static void destroyFilterHud(FilterHud* hud) {
    delete hud->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=decltype(hud->m_field32c);
    hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
    hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();::operator delete(hud);
}
int main() {
    if(squadRegressionMain())return 1;
    checks=failures=0;Events events;eventManager=&events;
    FilterPlayers players;world::playerManager=&players;FilterPlayer player;
    FilterGame currentGame;game=&currentGame;FilterLogic logic;gameLogic=&logic;
    FilterInput inputManager;io::inputManager=&inputManager;
    FilterSquads squads;squadManager=&squads;Sound sound;g_bfSound=&sound;
    auto* hud=new FilterHud;hudManager=hud;HudRadio radios[5];
    hud->m_field108=&radios[0];hud->m_field10c=&radios[1];hud->m_field110=&radios[2];hud->m_field114=&radios[3];hud->m_field118=&radios[4];
    hud->m_field100=nullptr;hud->m_field104=nullptr;hud->m_field138=nullptr;hud->m_field13c=nullptr;
    hud->m_field0b4[0]=hud->m_field0b4[1]=hud->m_field0e0=nullptr;
    hud->m_field0a8[0]=hud->m_field0a8[1]=nullptr;
    io::GameInput input{};
    auto reset=[&]() {
        input={};input.mappingFlags=~0ULL;input.dword0=0xabcdef01;input.dword1=0x12345678;input.flag=0x5a;input.byte3=0xa5;
        trace.clear();hud->mode=0;hud->previous=4;hud->modes.clear();hud->changes.clear();hud->resets.clear();
        hud->modeReads=hud->previousReads=hud->chatReads=hud->renames=hud->creates=hud->processed=0;
        hud->chat=hud->rename=hud->create=hud->commander=hud->order=hud->playerState=hud->process=false;
        hud->onClose={};hud->say=hud->radio=-1;hud->m_field0bf=false;hud->m_field248=true;hud->m_field0e7=false;
        hud->m_field0b0=hud->m_field0b1=hud->m_field0b2=false;hud->m_field242=false;hud->m_field2fc=false;
        game=&currentGame;currentGame.demo=false;currentGame.reads=0;logic.status=1;logic.reads=0;
        players.local=nullptr;players.queries=0;players.count=0;players.countCalls=0;player.commander=player.leader=false;player.squad=0;
        inputManager.delay=-1;emptySquad=1;for(auto& r:radios)r.setEnabled(true);
    };
    auto run=[&]() {check(hud->filterGameInput(input),"filter returns true");};
    auto preserved=[&]() {check(input.mappingFlags==~0ULL&&input.dword0==0xabcdef01&&input.dword1==0x12345678&&input.flag==0x5a&&input.byte3==0xa5,"input tail preserved");};
    reset();hud->m_field0bf=true;game=nullptr;run();check(trace.empty(),"first gate avoids all dependencies");
    reset();hud->m_field248=false;run();check(trace.empty(),"disabled gate avoids all dependencies");
    reset();game=nullptr;run();check(trace.empty(),"missing game avoids logic");
    reset();logic.status=4;run();check(trace=="G"&&players.queries==0,"not playing skips flags and radio tail");
    reset();input.inputs[71]=input.inputs[72]=1;run();check(hud->changes.empty(),"PE allshl suppresses high64-plus key getters");
    reset();input.inputs[12]=.5f;input.inputs[11]=std::numeric_limits<float>::quiet_NaN();input.inputs[9]=1;run();check(!hud->m_field229&&hud->m_field22a,"threshold strict and unordered false");
    reset();input.inputs[12]=std::numeric_limits<float>::infinity();input.inputs[10]=1;run();check(hud->m_field229&&hud->m_field22a,"positive infinity and right shift accepted");
    reset();input.inputs[2]=1;hud->mode=13;run();check(hud->modeReads==2&&hud->changes.empty()&&input.inputs[2]==1,"retail impossible stable Escape13-and14 condition");
    reset();input.inputs[2]=1;hud->modes={13,14};run();check(hud->changes==std::vector<std::pair<int,bool>>({{4,true}})&&input.inputs[2]==0,"changing virtual mode permits Escape restore");
    reset();input.inputs[2]=1;hud->chat=true;hud->onClose=[&](){input.mappingFlags&=~4ULL;};run();check(input.inputs[2]==1,"Escape clear rechecks mapping after close callback");
    reset();radios[0].setEnabled(false);radios[1].setEnabled(false);radios[2].setEnabled(false);radios[3].setEnabled(false);run();check(hud->resets==std::vector<HudRadio*>({&radios[0],&radios[2]})&&players.queries==1,"radio tail resets only first disabled owner in each chain");
    reset();radios[1].setEnabled(false);radios[4].setEnabled(false);run();check(hud->resets==std::vector<HudRadio*>({&radios[1],&radios[4]}),"radio tail reaches later owners");
    reset();input.inputs[37]=1;radios[0].setEnabled(false);run();check(players.queries==0&&hud->resets.empty()&&input.inputs[37]==1,"menu zero returns before radio and player tail");
    reset();input.inputs[55]=1;run();check(hud->changes==std::vector<std::pair<int,bool>>({{9,true}})&&hud->modeReads==6,"scoreboard tests all six independent modes");
    reset();currentGame.demo=true;run();check(hud->changes==std::vector<std::pair<int,bool>>({{0,false}}),"demo fallthrough resets mode");
    reset();input.inputs[33]=1;currentGame.demo=true;radios[2].setEnabled(false);run();check(hud->radio==21&&hud->resets.empty(),"demo squad radio opens alternate and held key suppresses its resets");
    reset();input.inputs[34]=1;currentGame.demo=true;run();check(hud->radio==11,"demo general radio alternate");
    reset();input.inputs[33]=1;players.local=&player;run();check(hud->radio==20,"alive regular squad radio opens");
    reset();input.inputs[34]=1;players.local=&player;hud->playerState=true;run();check(hud->radio==-1,"HUD player-state guard blocks radio");
    for(int key=22;key<=24;++key) {
        reset();input.inputs[key]=1;input.inputs[40]=3;consoleCharCount=8;consoleCharRead=1;consoleCharWrite=7;run();
        check(hud->say==key-22&&hud->m_field2fc&&consoleCharCount==0&&consoleCharRead==7,"chat hotkey selects correct channel and clears ring");
        bool zero=true;for(float f:input.inputs)zero=zero&&f==0;check(zero,"chat hotkey clears all74 input scalars");preserved();
    }
    reset();hud->chat=true;input.inputs[3]=1;run();check(hud->processed==1&&players.queries==0&&input.inputs[3]==0,"active chat processes then clears and returns before tail");preserved();
    reset();hud->rename=true;hud->process=true;input.inputs[4]=1;run();check(trace.find("ErBJd")!=std::string::npos&&inputManager.delay==5&&input.inputs[4]==0,"rename submit ordering and player input delay");preserved();
    reset();input.inputs[47]=1;input.inputs[48]=1;run();check(players.queries==1,"null question owner preserves common tail");
    reset();hud->mode=9;hud->previous=10;run();check(hud->changes==std::vector<std::pair<int,bool>>({{0,true}})&&hud->previousReads==1,"scoreboard previous10 normalizes to0");
    reset();hud->mode=12;hud->previous=7;run();check(hud->changes==std::vector<std::pair<int,bool>>({{7,true}})&&hud->previousReads==2,"scoreboard restore repeats previous-mode getter");
    reset();input.inputs[60]=1;players.local=&player;player.commander=true;run();check(hud->changes.empty(),"leader commander weapon guard rejects missing vehicle");
    check(!HudAmmo::canChangeWeapon(nullptr),"actual weapon gate missing object result");
    HudInformationAdapter adapter;trace.clear();try{adapter.getMapZoom();check(false,"adapter map boundary expected");}catch(MapBoundary&){check(trace=="z","adapter getter first callback uses current HUD map");}
    reset();HudSquad squad(nullptr);hud->m_field138=&squad;players.owner=&squad;
    squad.m_field08c=3;squad.m_expanded.set();check(squad.toggleSelectedSquad(3)==-1&&squad.m_expanded.none(),"same squad toggles selection off and clears every bit");
    check(squad.toggleSelectedSquad(99)==99&&squad.m_expanded.to_ulong()==512,"out-of-range selection retains original integer but clamps bit");
    check(squad.toggleSelectedSquad(-5)==-5&&squad.m_expanded.none(),"negative selection stores raw value without expanded bit");
    squad.m_expanded.set(15);squad.setSquadExpanded(-1,true);squad.setSquadExpanded(99,true);squad.setSquadExpanded(99,false);check(squad.m_expanded.to_ulong()==0x8001,"expanded clamp preserves unrelated upper bits");
    players.lookup=nullptr;squad.m_field08c=4;trace.clear();squad.selectPlayer(42);check(trace=="K"&&squad.m_field090==-1&&squad.m_field08c==4,"missing player clears only player selection");
    players.lookup=&player;player.squad=99;squad.selectPlayer(42);check(squad.m_field08c==-1&&squad.m_field090==42&&squad.m_expanded.test(9),"existing player clears squad selection and expands bounded squad");
    players.lookup=nullptr;players.count=1;squad.selectSquad(4);check(squad.m_field08c==4&&players.lastLookup==-1,"valid nonempty squad selects and clears player through lookup");
    players.count=0;squad.selectSquad(4);check(squad.m_field08c==-1,"empty squad selection rejected");
    int oldCount=players.countCalls;squad.selectSquad(0);squad.selectSquad(10);check(players.countCalls==oldCount,"invalid squad skips count query");
    players.local=nullptr;trace.clear();squad.joinSquad(3);check(trace=="P","join missing local guard");
    players.local=&player;player.commander=true;trace.clear();squad.joinSquad(3);check(trace=="PC","join commander guard");
    player.commander=false;player.squad=0;players.count=1;trace.clear();squad.joinSquad(3);check(trace=="PCQTNHITj"&&squads.adds==1,"join ordering and repeated team read");
    trace.clear();squad.joinSquad(-1);check(trace=="PCQ","negative join short circuits before team");
    player.squad=1;squad.m_squadId=1;trace.clear();squad.joinSquad(3);check(trace=="PCQSSLWWA","already-in-squad warning queries both squads then localizes and replaces twice");
    player.squad=0;squad.m_squadId=0;players.count=0;squad.m_createVisible=true;trace.clear();squad.finishSquadCreate();check(trace=="PgsFC"&&!squad.m_createVisible,"active create reads discarded edit text, submits, then closes");
    players.local=nullptr;trace.clear();squad.finishSquadCreate();check(trace=="PgsFC","missing player still submits and closes create");
    players.local=&player;int beforeTeamReads=player.teamReads;trace.clear();squad.finishSquadCreate();
    check(player.teamReads==beforeTeamReads+2&&editText=="Alpha"&&trace.substr(trace.size()-5)=="NgsFC","inactive create repeats player team then copies default name before submission");
    hud->m_field138=nullptr;destroyFilterHud(hud);hudManager=nullptr;
    std::printf("HUD game filter: %d/%d checks passed\n",checks-failures,checks);return failures?1:0;
}
