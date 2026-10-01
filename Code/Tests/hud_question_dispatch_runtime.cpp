// [TEMP]
// Tests question dispatch and votes with typed collaborators.
// The settings fixture supplies only the out-of-voting byte read by the non-virtual getter.
#define main cameraRegressionMain
#include "hud_camera_runtime.cpp"
#undef main
#include "../BF2/Menu/HudQuestionManager.h"
#include "../BF2/Game/GeneralSettings.h"
#include "../BF2/Core/ClassManager.h"
#include "../BF2/Game/Common/ClientVoteManager.h"
#include "../BF2/Game/Common/VoteEvent.h"
#include "../BF2/Game/MapList.h"
#include <float.h>
#include <limits>
using world::PlayerVotingStatus;
// Keep the information layer explicitly null so PlayerManager construction skips HUD-value registration.
namespace dice { namespace hfe { ClassManager* classManager=nullptr; IBase* g_hudInformationLayer=nullptr;
namespace hud {
struct HudQuestionFixture {
    static void add(HudQuestionManager& m,HudQuestion& q) {m.m_questions[q.category].push_back(&q);}
};
} // hud
// The retail byte +0x81 the +0x80 getter reads, written into zeroed storage.
struct GeneralSettingsFixture {
    static void setOutOfVoting(GeneralSettings& settings,bool value) {settings.m_outOfVoting=value;}
};
} }
struct QPlayer : Player {
    int squad=4;bool leader=false,bot=false,participates=true;PlayerVotingStatus vote=static_cast<PlayerVotingStatus>(0);
    mutable std::string name="Player";
    std::function<void()> onParticipate;
    int getSquadId() const override {trace+='S';return squad;}
    bool getIsSquadLeader() const override {trace+='H';return leader;}
    bool getIsAIPlayer() const override {trace+='B';return bot;}
    const std::string& getName() const override {trace+='N';return name;}
    PlayerVotingStatus getVotingStatus() override {trace+='v';return vote;}
    bool getParticipateInVoting() override {trace+='p';if(onParticipate)onParticipate();return participates;}
};
struct QPlayers : Players {
    world::IPlayer* found=nullptr;int requested=-999;
    world::IPlayer* getPlayerFromId(int id) override {trace+='F';requested=id;return found;}
};
struct QSquadInfo : ISquadInfo {
    std::string name="Squad";SquadOrder order;bool hasOrder=true;
    const std::string& getSquadName() override {trace+='n';return name;}
    SquadOrder* getCommanderOrders() override {trace+='O';return hasOrder?&order:nullptr;}
    bool getIsPrivate() override {actionUnsupported("squad private");}
    bool getIsAccepted(SquadOrderType,OrderType) override {actionUnsupported("squad accepted");}
    int getSquadId() override {actionUnsupported("squad id");}
    SquadOrder* getSquadOrders() override {actionUnsupported("squad own orders");}
    SquadOrder* getRequest(SquadOrderType) override {actionUnsupported("squad request");}
    int getSpawnGroupId() override {actionUnsupported("squad spawn");}
    void addSquadLeaderSpawnPoint(world::IPlayer*) override {actionUnsupported("squad spawn add");}
    void removeSquadLeaderSpawnPoint(world::IPlayer*) override {actionUnsupported("squad spawn remove");}
};
struct QSquads : Squads {
    ISquadInfo* info=nullptr;int leaderId=29,commander=31;
    std::vector<int> invitation,accepted,leadership,application;
    SquadOrder* issued=nullptr;
    ISquadInfo* getSquadInfo(int team,int squad) override {trace+='I';queriedTeam=team;check(squad==4||squad==7,"squad id forwarded");return info;}
    int getSquadLeader(int team,int squad) override {trace+='l';return leaderId;}
    int getCommander(int team) override {trace+='c';return commander;}
    void inviteToSquad(int team,int squad,int id,bool yes,bool last) override {trace+='i';invitation={team,squad,id,yes,last};}
    bool setIsAccepted(int team,int squad,bool yes,OrderType order,SquadOrderType type,bool last) override {
        trace+='a';accepted={team,squad,yes,static_cast<int>(order),static_cast<int>(type),last};return false;
    }
    void issueSquadOrder(int team,int squad,SquadOrder& order,bool force) override {trace+='o';issued=&order;check(order.flag2C&&!force,"commander order accepted before issue");}
    void acceptCommander(int team,int id) override {trace+='A';application={team,id,1};}
    void declineCommander(int team,int id) override {trace+='d';application={team,id,0};}
    void setSquadLeader(int team,int squad,int id,bool force) override {trace+='h';leadership={team,squad,id,force};}
};
struct QLogic : Logic {
    std::string teamName="uSMC";int command=-1;
    const std::string& getTeamName(int) override {trace+='t';return teamName;}
    bool remoteCommand(int value,int b,int c,int d,int e) override {trace+='r';command=value;check(!b&&!c&&!d&&!e,"teamkill remote payload zero");return false;}
};
struct QGame : Game {
    std::string radio;int channel=-1;world::IPlayer* speaker=nullptr;unsigned recipient=0;
    std::function<void()> onRadio,onSend;
    bool radioMessage(TargetChannel ch,const std::string& value,world::IPlayer* who,unsigned to,bool a,bool b) override {
        trace+='R';radio=value;channel=static_cast<int>(ch);speaker=who;recipient=to;
        check(a&&!b,"radio true false flags");if(onRadio)onRadio();return false;
    }
    void sendGameEventIfConnected(unsigned id,GameEvent* event) override {trace+='E';sent=event;target=id;if(onSend)onSend();}
    void sendGameEvent(unsigned id,GameEvent* event) override {trace+='G';sent=event;target=id;if(onSend)onSend();}
};
struct QEvents : EventManager {
    GameEvent* posted=nullptr;
    unsigned postEventNextUpdate(EventCategory cat,unsigned id,void* event) override {
        trace+='e';check(cat==ECGameLogic&&id==15,"vote queued category and id");posted=static_cast<GameEvent*>(event);return 99;
    }
};
struct QClasses : ClassManager {
    IBase* returned=nullptr;
    IBase* getSingleton(const std::string& name) override {trace+='K';check(name=="GeneralSettings","settings singleton exact name");return returned;}
};
struct QHud : BF2HudManager {
    std::vector<std::string> keys;std::vector<std::wstring> texts;std::vector<int> teams;
    std::vector<int> request;int minutes=-99,squadReads=0;std::function<void()> onLocalize,onRefresh;
    void refreshChatCategory(int category) override {trace+='C';check(category==8,"question chat category8");if(onRefresh)onRefresh();}
    std::wstring getLocalizedString(std::string key) override {
        trace+='L';keys.push_back(key);if(onLocalize)onLocalize();
        return std::wstring(key.begin(),key.end());
    }
    std::wstring replaceWideInputKeys(std::wstring text) override {trace+='k';return text+L"|keys";}
    std::wstring replaceWideTokenFromString(std::wstring text,std::string token,std::string value) override {
        trace+='s';return text+L"|"+std::wstring(token.begin(),token.end())+L"="+std::wstring(value.begin(),value.end());
    }
    std::wstring replaceWideToken(std::wstring text,std::string token,std::wstring value) override {
        trace+='w';return text+L"|"+std::wstring(token.begin(),token.end())+L"="+value;
    }
    std::wstring replaceWideTokenFromInt(std::wstring text,std::string token,int value) override {trace+='m';minutes=value;return text;}
    void addServerTeamChat(std::wstring text,int team) override {trace+='T';texts.push_back(text);teams.push_back(team);}
    void addServerChat(std::wstring text,int team) override {trace+='U';texts.push_back(text);teams.push_back(team);}
    void addTeamChat(std::wstring text,world::IPlayer* player,int team) override {trace+='J';check(!player&&team==0,"reply team chat null player and zero team");texts.push_back(text);}
    HudSquad* getSquad() override {trace+='q';++squadReads;return nullptr;}
    void sendRequestEvent(int team,int squad,RequestEvent::RequestTypes type) override {trace+='Q';request={team,squad,static_cast<int>(type)};}
    void leaveSquad() override {trace+='x';}
    void toggleSquadCommander() override {trace+='z';}
    void reset() {keys.clear();texts.clear();teams.clear();request.clear();trace.clear();}
};
static void disposeHud(QHud* hud) {
    delete hud->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=std::vector<HudPendingRequest>;
    hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
    hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();
    hud->keys.~vector();hud->texts.~vector();hud->teams.~vector();hud->request.~vector();
    hud->onLocalize.~function();hud->onRefresh.~function();::operator delete(hud);
}
int main() {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);setvbuf(stdout,nullptr,_IONBF,0);
    QPlayers players;QPlayer player,remote;QSquads squads;QSquadInfo info;QGame observedGame;QLogic logic;QEvents events;QClasses classes;
    world::playerManager=&players;squadManager=&squads;game=&observedGame;gameLogic=&logic;eventManager=&events;classManager=&classes;
    QHud* hud=new QHud;hudManager=hud;HudQuestionManager manager;HudQuestion q{0,7,0,10};
    auto put=[&](int category,int id=7){manager.clear();q.category=category;q.id=id;HudQuestionFixture::add(manager,q);hud->reset();};
    manager.displayQuestion(nullptr);q.category=-1;manager.displayQuestion(&q);manager.expireQuestion(nullptr);manager.expireQuestion(&q);
    check(trace.empty(),"null and out-of-range display/expire do nothing");
    players.local=&player;players.found=&remote;remote.name=std::string("Before\0ignored",14);
    put(0);hud->onLocalize=[&]{remote.name="After";};manager.displayQuestion(&q);hud->onLocalize=nullptr;
    check(trace=="FCNLksTLkT","join request callback order");check(hud->texts[0].find(L"=Before")!=std::wstring::npos,"name captured and c-string truncated before localization");
    check(hud->teams==std::vector<int>({3,3}),"join request display teams");
    put(0);players.found=nullptr;manager.displayQuestion(&q);check(trace=="F","missing requester leaves current chat intact");players.found=&remote;
    put(1);players.local=nullptr;manager.displayQuestion(&q);check(trace=="P","missing invitee leaves chat intact");players.local=&player;
    put(1);squads.info=nullptr;manager.displayQuestion(&q);check(trace=="PCTI","missing squad clears question chat then stops");squads.info=&info;
    put(1);manager.displayQuestion(&q);check(trace=="PCTInLksTLkT","invitation display lookup and token order");
    for(int category:{2,4,5}) {put(category);manager.displayQuestion(&q);check(trace=="CLkTLkT","simple display two localized key substitutions");check(hud->teams==std::vector<int>({category==4?1:2,category==4?1:2}),"simple display team values");}
    put(3);gameLogic=nullptr;manager.displayQuestion(&q);check(trace.empty(),"commander display no logic gate");gameLogic=&logic;
    put(3);manager.displayQuestion(&q);check(trace=="PCTtLLkwTLkT","commander team localization precedes main localization");check(hud->keys[0]=="HUD_TEAMNAME_usmc","existing lowercase and format helper used");
    ClientVoteManager* votes=ClientVoteManager::getInstance();votes->m_string14="Issuer";votes->m_string0C="Target";votes->m_voteTimeRemaining=61;
    check(sizeof(ClientVoteManager)==88,"refined vote float preserves native x86 size88");
    std::string copied=votes->getVoteIssuerName();copied="Changed";check(votes->m_string14=="Issuer"&&votes->getVoteTargetName()=="Target","vote name getters return independent native values");
    put(6,2);manager.displayQuestion(&q);check(trace=="KCLssmkT"&&hud->minutes==2,"vote display names minutes and substitution order");
    check(hud->texts[0].find(L"#PLAYERNAME1#=Issuer")!=std::wstring::npos&&hud->texts[0].find(L"#PLAYERNAME2#=Target")!=std::wstring::npos,"vote native string fields identified");
    put(6,3);hud->onRefresh=[&]{q.id=2;votes->m_string14="Mutated";};manager.displayQuestion(&q);hud->onRefresh=nullptr;
    check(hud->keys[0]=="HUD_HUD_KICKVOTE_COMMANDER_START_REMOTE"&&hud->texts[0].find(L"=Issuer")!=std::wstring::npos,"vote case and copied names retained across chat callback");
    mapList->m_entries.clear();mapList->m_entries.push_back(MapList::Entry{"Daqing","gpm_cq",16});votes->m_dword08=0;
    put(6,1);manager.displayQuestion(&q);check(trace=="KCLLswmkT","map localized before vote template then token order");check(hud->keys[0]=="HUD_LEVELNAME_Daqing","native MapList entry name consumed");
    votes->m_dword08=-1;put(6,1);manager.displayQuestion(&q);check(hud->keys[0]=="HUD_LEVELNAME_","missing map entry retains empty map name");
    for(float seconds:{0.0f,-61.0f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity()}) {
        votes->m_voteTimeRemaining=seconds;put(6,2);manager.displayQuestion(&q);
        const int expected=seconds==0?0:seconds==-61?-1:INT_MIN;check(hud->minutes==expected,"vote ceil and x87 invalid conversion");
    }
    unsigned oldControl;_controlfp_s(&oldControl,0,0);for(unsigned mode:{_RC_NEAR,_RC_DOWN,_RC_UP,_RC_CHOP}) {
        unsigned current;_controlfp_s(&current,mode,_MCW_RC);votes->m_voteTimeRemaining=60;put(6,2);manager.displayQuestion(&q);_controlfp_s(&current,0,0);check((current&_MCW_RC)==mode,"vote integer conversion restores rounding control");
        check(hud->minutes==(mode==_RC_UP?2:1),"vote preserves float product rounding before ceil");
    }unsigned ignored;_controlfp_s(&ignored,oldControl,_MCW_RC);
    put(6,9);manager.displayQuestion(&q);check(manager.getCount(6)==0&&trace=="K","unknown vote id removed with real null redisplay");
    {
    // Out-of-voting returns before vote copying/chat and retains the question.
        alignas(GeneralSettings) unsigned char storage[sizeof(GeneralSettings)]={};
        GeneralSettings& settings=*reinterpret_cast<GeneralSettings*>(storage);
        GeneralSettingsFixture::setOutOfVoting(settings,true);
        put(6,2);classes.returned=&settings;manager.displayQuestion(&q);
        check(trace=="K"&&manager.getCount(6)==1&&hud->keys.empty(),"out of voting returns after the singleton read, question kept, no chat");
        GeneralSettingsFixture::setOutOfVoting(settings,false);
        put(6,2);manager.displayQuestion(&q);const std::string inVoting=trace;const auto inKeys=hud->keys;
        classes.returned=nullptr;put(6,2);manager.displayQuestion(&q);
        check(inVoting==trace&&inKeys==hud->keys&&!inKeys.empty(),"in voting runs exactly the missing-singleton vote path");
    }
    game=nullptr;hud->reset();manager.replyQuestion(nullptr,true,false,true);check(trace.empty(),"no-question reply requires game");game=&observedGame;players.local=nullptr;
    hud->reset();manager.replyQuestion(nullptr,true,false,true);check(trace=="P","no-question reply requires local human");players.local=&player;
    hud->reset();manager.replyQuestion(nullptr,false,false,true);check(trace=="PR"&&observedGame.radio=="negative"&&observedGame.channel==1&&observedGame.recipient==0,"default negative radio routing");
    hud->reset();manager.replyQuestion(nullptr,true,true,false);check(trace=="PR"&&observedGame.radio=="roger_that","no-question reply ignores silent flag");
    q.category=99;hud->reset();manager.replyQuestion(&q,false,true,false);check(trace=="CPPR","unknown question category re-queries local player before default reply");
    put(5);players.local=nullptr;manager.replyQuestion(&q,true,false,false);check(trace=="CP"&&manager.getCount(5)==1,"non-null reply clears chat but retains question when local absent");players.local=&player;
    for(bool yes:{false,true}) {put(5,255);manager.replyQuestion(&q,yes,true,false);check(manager.getCount(5)==0&&logic.command==(yes?101:100),"teamkill reply removed before real remote command");check(trace=="CPrLU","teamkill ignores silent flag");check(hud->keys[0]==(yes?"HUD_TEXT_TEAMKILL_FORGIVE":"HUD_ERRORMESSAGE_TKPUNISHONLOCALHOST"),"teamkill localhost key");}
    put(5,8);manager.replyQuestion(&q,false,false,false);check(hud->keys[0]=="HUD_TEXT_TEAMKILL_PUNISH","ordinary teamkill punish key");
    put(5);manager.expireQuestion(&q);check(logic.command==101&&manager.getCount(5)==0,"teamkill expiry forgives");
    for(int category=0;category<4;++category) {put(category);players.local=nullptr;manager.expireQuestion(&q);check(trace=="CPLU"&&manager.getCount(category)==1,"expiry still posts timeout after no-player reply refuses removal");}players.local=&player;
    for(int category:{4,6}) {put(category);players.local=nullptr;manager.expireQuestion(&q);check(trace=="CP"&&manager.getCount(category)==1,"expiry category4/6 uses silent negative reply without timeout message");}players.local=&player;
    put(0);players.found=nullptr;manager.replyQuestion(&q,true,false,false);check(trace=="CPF"&&manager.getCount(0)==0,"absent requester removed before no further work");players.found=&remote;player.leader=true;
    for(bool yes:{false,true}) {for(bool server:{false,true}) {
        put(0,42);logic.server=server;manager.replyQuestion(&q,yes,true,false);
        auto* event=static_cast<PostRemoteEvent*>(observedGame.sent);check(event&&event->m_eventCategory==ECHud&&event->m_eventId==(yes?19:18)&&event->m_payloadBitSize==4,"real postremote invitation event fields");
        int payload;std::memcpy(&payload,event->m_payload,4);check(payload==42&&event->m_field4C==0,"postremote actual payload and delay");
        check(observedGame.target==(server?static_cast<unsigned>(remote.id):static_cast<unsigned>(-1)),"invitation uses sendIfConnected target");event->release();observedGame.sent=nullptr;
        check(manager.getCount(0)==0,"request removed before invitation event");
    }}
    put(0,42);logic.server=false;manager.replyQuestion(&q,true,false,false);check(hud->keys[0]=="HUD_HUD_SQUAD_GRANTED_TO_ACCESS_SQUAD"&&squads.invitation==std::vector<int>({2,4,42,1,0}),"affirmative invitation chat then squad manager payload");observedGame.sent->release();observedGame.sent=nullptr;
    put(1);manager.replyQuestion(&q,true,false,false);check(trace=="CPqTlR"&&hud->squadReads==1&&observedGame.recipient==29,"invitation accepts missing HUD squad then leader radio");
    put(1);manager.replyQuestion(&q,false,true,false);check(trace=="CPDTi"&&squads.invitation==std::vector<int>({2,7,17,0,0}),"decline invitation exact callback and payload");
    put(2);manager.replyQuestion(&q,true,false,false);check(trace=="CPSTIOSToTcRSTQSTa","order acceptance callback and radio lifetime order");check(squads.issued==&info.order&&hud->request==std::vector<int>({2,4,2})&&squads.accepted==std::vector<int>({2,4,0,1,5,0}),"order issue request and accepted values");
    put(2);squads.info=nullptr;manager.replyQuestion(&q,true,true,false);check(hud->request[2]==0&&trace=="CPSTISTQSTa","missing commander order still sends state0 and acceptance update");squads.info=&info;
    put(2);manager.replyQuestion(&q,false,false,false);check(hud->request[2]==3&&observedGame.radio=="negative","order refusal request state3");
    put(3);manager.replyQuestion(&q,true,true,false);check(trace=="CPSxDTA"&&squads.application==std::vector<int>({2,17,1}),"commander acceptance leaves squad then id/team");
    put(3);manager.replyQuestion(&q,false,false,false);check(trace=="CPLUDTd"&&squads.application==std::vector<int>({2,17,0}),"commander decline message precedes id/team callbacks");
    put(4);player.leader=false;remote.bot=true;manager.replyQuestion(&q,true,true,false);check(trace=="CPHTSlFBDhz"&&squads.leadership==std::vector<int>({2,4,17,0}),"AI squad leader replacement and HUD toggle");
    put(6,0);manager.replyQuestion(&q,true,false,false);check(trace=="CP","zero vote id removed without voting-status callback");
    put(6,2);player.vote=static_cast<PlayerVotingStatus>(1);manager.replyQuestion(&q,true,false,false);check(trace=="CPv","already voted reply stops before vote singleton");player.vote=static_cast<PlayerVotingStatus>(0);
    votes->m_dword04=1;votes->m_dword08=300;logic.server=false;put(6,2);manager.replyQuestion(&q,false,false,false);
    auto* vote=static_cast<VoteEvent*>(observedGame.sent);check(vote&&vote->m_voteOption==Vote::VOAgainst&&vote->m_voteType==1&&vote->m_voteStatus==4&&vote->m_voteCaster==254&&vote->m_voteItem==44,"question sends real vote event preserving byte truncation and caster sentinel");
    check(trace=="CPvPpLPDG"&&votes->m_messageDisplayed,"vote reply callback order and post-send latch");vote->release();observedGame.sent=nullptr;
    for(int kind:{2,3}) {votes->m_dword04=kind;votes->m_dword08=player.id;hud->reset();check(!votes->sendVote(Vote::VOFor)&&trace=="PpPD","self-target kick/mutiny vote rejected");}
    votes->m_dword04=0;votes->m_dword08=-1;votes->m_messageDisplayed=false;logic.server=true;hud->reset();check(votes->sendVote(Vote::VOFor)&&trace=="PpLe","zero active vote type still sends server ballot");
    vote=static_cast<VoteEvent*>(events.posted);check(vote->m_voteType==0&&vote->m_voteItem==255&&votes->m_messageDisplayed,"server vote -1 item sentinel and post-queue latch");vote->release();events.posted=nullptr;
    player.participates=false;hud->reset();check(!votes->sendVote(Vote::VOFor)&&trace=="Pp","nonparticipant ballot rejected");
    player.participates=true;player.onParticipate=[] {world::playerManager=nullptr;};hud->reset();check(!votes->sendVote(Vote::VOFor)&&trace=="Pp","vote rechecks manager after participation callback");player.onParticipate=nullptr;world::playerManager=&players;
    votes->m_dword04=1;votes->m_dword08=3;votes->m_messageDisplayed=false;logic.server=false;
    observedGame.onSend=[&] {check(!votes->m_messageDisplayed,"vote displayed latch remains false during send callback");};hud->reset();check(votes->sendVote(Vote::VOFor)&&votes->m_messageDisplayed,"vote displayed latch written after send callback");observedGame.sent->release();observedGame.sent=nullptr;observedGame.onSend=nullptr;
    world::playerManager=nullptr;hud->reset();check(!votes->sendVote(Vote::VOFor)&&trace.empty(),"missing manager ballot rejected");world::playerManager=&players;
    manager.clear();ClientVoteManager::release();disposeHud(hud);hudManager=nullptr;
    std::printf("HUD question dispatch: %u/%u checks passed\n",checks-failures,checks);return failures?1:0;
}
