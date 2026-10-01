// [TEMP] [CHOSEN, not recovered] Real Scoreboard lifetime/primary/event and main
// HUD list selector. Collaborators only observe external callbacks. No owner
// constructor or tested method is replaced; no Map/VoIP object is fabricated.
// [RETIRE-WHEN: HUD-SCOREBOARD-LIST-FIXTURE] Replace dependency observers and
// manual pre-init HUD cleanup when the complete HUD lifecycle is executable.
#define main unusedSquadFixtureMain
#include "hud_squad_runtime.cpp"
#undef main
#define private public
#include "../BF2/Menu/HudScoreboard.h"
#include "../BF2/Menu/HudCommander.h"
#include "../BF2/Menu/HudChat.h"
#include "../BF2/Menu/HudVoteMap.h"
#undef private
#include <limits>

struct ScoreEvents : EventManager {
    HudScoreboard* constructing=nullptr;
    IEventListener* last=nullptr;
    unsigned removals=0;
    bool registerEventHandler(EventCategory category,IEventListener* listener,int priority) override {
        if(constructing) {
            check(category==ECHud&&priority==0,"Scoreboard category and priority");
            check(static_cast<HudScoreboard*>(listener)==constructing&&reinterpret_cast<char*>(listener)-reinterpret_cast<char*>(constructing)==4,"Scoreboard actual secondary listener adjustment");
            check(constructing->m_mutedPlayers.empty()&&!constructing->m_toggleSquads&&constructing->m_toggleScore&&!constructing->m_toggleManage&&constructing->m_teamVoteOnly,"Scoreboard list and flags precede registration");
            unsigned word=0;std::memcpy(&word,&constructing->m_columns[0],4);
            check(word==0xa5a5a5a5,"Scoreboard columns untouched at registration");
            std::memcpy(&word,&constructing->m_counters[0][0][0],4);
            check(word==0xa5a5a5a5,"Scoreboard counters untouched at registration");
            constructing->m_counters[0][0][0]=77;
            constructing->m_mutedPlayers.push_back(99);
            listener->handleEvent(ECHud,100,nullptr,nullptr);
        }
        last=listener;return EventManager::registerEventHandler(category,listener,priority);
    }
    bool unregisterEventHandler(EventCategory category,IEventListener* listener) override {
        ++removals;return EventManager::unregisterEventHandler(category,listener);
    }
};
struct ScoreTextures : System {
    ScoreTextures():System(640,480){}
    HudScoreboard* owner=nullptr;
    ScoreTextures* next=nullptr;
    std::vector<std::string> names;
    int base=0,releases=0;
    int loadTexture(const char* name,bool menuPath) override {
        check(menuPath,"Scoreboard texture menu-path flag");names.push_back(name);
        if(next){owner->setSystem(next);next=nullptr;}
        return base+static_cast<int>(names.size());
    }
    void releaseTexture(int) override {++releases;}
};
struct ScoreInformation : Information {
    ScoreInformation* next=nullptr;
    int registerBool(const std::string& name,bool* value) override {
        names.push_back(name);addresses.push_back(value);
        if(next){g_hudInformationLayer=next;next=nullptr;}
        return -1;
    }
};
struct ListChat : HudChat {
    ListChat():HudChat(nullptr){}
    unsigned reads=0;
    BfListBoxData* answer=nullptr;
    BfListBoxData* getData() override {++reads;return answer;}
};
static void destroyListFixture(BfListBoxData* data) {
    for(auto* column:data->m_columns)if(column){for(auto* cell:column->cells)delete cell;delete column;}
    delete data;
}
static void checkColumns(BfListBoxData* data,const std::vector<int>& widths,bool totals) {
    check(data->m_columns.size()==widths.size()&&data->m_columnCount==static_cast<int>(widths.size()),"Scoreboard native column count");
    for(unsigned i=0;i<widths.size();++i) {
        const auto* column=data->m_columns[i];
        check(column&&column->index==static_cast<int>(i)&&column->width==widths[i]&&column->cells.empty(),"Scoreboard column order/width/native empty cells");
        check(column->field019==!(totals&&i==3)&&!column->field01a,"Scoreboard column parameter flags");
    }
}
int main() {
    checks=failures=0;
    ScoreEvents events;eventManager=&events;
    Players players;world::playerManager=&players;
    Player player;players.local=&player;
    ScoreInformation information,nextInformation;g_hudInformationLayer=&information;
    events.unregisterEventHandler(ECHud,&information);events.unregisterEventHandler(ECHud,&nextInformation);
    alignas(HudScoreboard) unsigned char memory[sizeof(HudScoreboard)];std::memset(memory,0xa5,sizeof(memory));
    events.constructing=reinterpret_cast<HudScoreboard*>(memory);
    HudScoreboard* scoreboard=new(memory)HudScoreboard(nullptr);events.constructing=nullptr;
    check(sizeof(HudScoreboard)==888&&sizeof(std::list<int>)==8,"native x86 Scoreboard size888 versus retail892");
    check(reinterpret_cast<char*>(&scoreboard->m_counters)-reinterpret_cast<char*>(scoreboard)==0x6c&&reinterpret_cast<char*>(&scoreboard->m_mutedPlayers)-reinterpret_cast<char*>(scoreboard)==0x358,"non-STL prefix/counter/list offsets");
    check(reinterpret_cast<char*>(&scoreboard->m_friendlyList)-reinterpret_cast<char*>(scoreboard)==0x364,"native list shifts trailing pointers by four bytes");
    check(scoreboard->m_rowCount==0&&scoreboard->m_visibleRows==21&&scoreboard->m_squad==0&&scoreboard->m_player==-1&&scoreboard->m_team==2&&scoreboard->m_selectedPlayer==-1,"Scoreboard integer defaults");
    check(scoreboard->m_rowHeight==19&&scoreboard->m_rowOffset==2&&!scoreboard->m_visible&&!scoreboard->m_isCommander&&!scoreboard->m_isSquadLeader,"Scoreboard scalar defaults");
    bool zeros=true;for(auto& group:scoreboard->m_counters)for(auto& team:group)for(int counter:team)zeros=zeros&&counter==0;
    check(zeros&&scoreboard->m_mutedPlayers==std::list<int>{99},"post-registration counter reset preserves callback list mutation");
    unsigned untouched=0;std::memcpy(&untouched,&scoreboard->m_friendlyList,4);
    check(untouched==0xa5a5a5a5,"Scoreboard constructor leaves data pointers untouched");
    bool texturesZero=true;for(int texture:scoreboard->m_textures)texturesZero=texturesZero&&texture==0;
    check(texturesZero,"null System initializes texture handles to zero");
    IHudScoreboard* primary=scoreboard;check(primary->registerFunctions(),"primary slot8 is actual retail true");
    ScoreTextures first,second;first.base=100;second.base=200;first.owner=scoreboard;
    primary->setSystem(&first);check(scoreboard->m_system==&first,"primary slot0 stores borrowed System");

    information.next=&nextInformation;check(primary->registerData(),"Scoreboard registration ignores negative binding results");
    check(information.names==std::vector<std::string>{"ToggleSquads"}&&nextInformation.names==std::vector<std::string>{"ToggleScore","ToggleManage","TeamVoteOnly"},"binding names/order and global reload after callback");
    check(information.addresses[0]==&scoreboard->m_toggleSquads&&nextInformation.addresses==std::vector<const void*>{&scoreboard->m_toggleScore,&scoreboard->m_toggleManage,&scoreboard->m_teamVoteOnly},"bindings retain actual native bool addresses");
    check(std::string(scoreboard->m_friendlyList->getName())=="friendlyScoreboardList"&&std::string(scoreboard->m_enemyList->getName())=="enemyScoreboardList"&&std::string(scoreboard->m_finalList->getName())=="FinalScoreboardList"&&std::string(scoreboard->m_friendlyTotalList->getName())=="friendlyTotalScoreboardList"&&std::string(scoreboard->m_enemyTotalList->getName())=="enemyTotalScoreboardList","all five exact list names");
    const std::vector<int> scoreWidths={0,5,21,44,212,244,276,308,340,340,0};
    const std::vector<int> totalWidths={0,5,21,44,212,244,276,308,340,340};
    checkColumns(scoreboard->m_friendlyList,scoreWidths,false);checkColumns(scoreboard->m_enemyList,scoreWidths,false);
    checkColumns(scoreboard->m_friendlyTotalList,totalWidths,true);checkColumns(scoreboard->m_enemyTotalList,totalWidths,true);
    checkColumns(scoreboard->m_finalList,{0,148,179,212,244},false);
    for(auto* list:{scoreboard->m_friendlyList,scoreboard->m_enemyList})
        check(list->m_visibleRows==21&&list->m_field021&&list->m_field022&&!list->m_field023&&!list->m_columns[10]->enabled,"player list visibility and hidden tenth column");
    for(auto* list:{scoreboard->m_friendlyTotalList,scoreboard->m_enemyTotalList})
        check(list->m_visibleRows==1&&!list->m_field021&&list->m_field022&&list->m_field023,"totals preserve default selection byte23");
    check(scoreboard->m_finalList->m_visibleRows==20&&!scoreboard->m_finalList->m_field021&&!scoreboard->m_finalList->m_field022&&scoreboard->m_finalList->m_field023,"final list flags");
    check(scoreboard->getScoreList(1)==scoreboard->m_enemyList&&scoreboard->getScoreList(0)==scoreboard->m_friendlyList&&scoreboard->getScoreList(-1)==scoreboard->m_friendlyList,"score side1 selects enemy and other values friendly");
    check(scoreboard->getTotalList(1)==scoreboard->m_enemyTotalList&&scoreboard->getTotalList(2)==scoreboard->m_friendlyTotalList,"total selector mapping");
    auto* selectedCell=new ListBoxCell(L"",1,1,1,1,0);selectedCell->narrow=std::string(" -42\0ignored",12);
    scoreboard->m_enemyList->m_columns[10]->cells.push_back(selectedCell);scoreboard->m_enemyList->setRow(0);
    check(scoreboard->getSelectedPlayer(1)==-42&&scoreboard->getSelectedPlayer(2)==0,"selected hidden player ID follows atoi and empty string behavior");

    struct MutePayload{unsigned char muted;unsigned char padding[3];int id;};
    MutePayload mute={2,{0,0,0},-7};
    unsigned before=players.queries;scoreboard->handleEvent(ECGame,59,nullptr,&mute);
    check(players.queries==before,"non-HUD event does not query local player");
    scoreboard->handleEvent(ECHud,10,nullptr,nullptr);check(players.queries==before+1,"unrelated HUD event still queries player");
    events.postEvent(ECHud,59,&mute,0);events.postEvent(ECHud,59,&mute,0);
    check(scoreboard->m_mutedPlayers==std::list<int>{99,-7,-7},"native event dispatch stores duplicate signed player IDs");
    mute.muted=0;events.postEvent(ECHud,59,&mute,0);check(scoreboard->m_mutedPlayers==std::list<int>{99,-7},"unmute removes only first matching entry");
    mute.id=400;events.postEvent(ECHud,59,&mute,0);check(scoreboard->m_mutedPlayers==std::list<int>{99,-7},"unmute missing ID leaves list untouched");
    mute.id=player.id;mute.muted=1;events.postEvent(ECHud,59,&mute,0);check(scoreboard->m_mutedPlayers==std::list<int>{99,-7},"local player's mute notification is ignored");

    // Real HUD construction and real typed owners for every supported selector.
    void* hudStorage=::operator new(sizeof(BF2HudManager));auto* hud=new(hudStorage)BF2HudManager;hudManager=hud;
    hud->m_field120=nullptr;hud->m_field138=nullptr;hud->m_field13c=nullptr;hud->m_field144=nullptr;hud->m_field140=nullptr;hud->m_field128=nullptr;
    for(int selector=-1;selector<14;++selector)check(hud->getListData(selector)==nullptr,"null-owner/default list selector");
    check(hud->getListData((std::numeric_limits<int>::min)())==nullptr&&hud->getListData((std::numeric_limits<int>::max)())==nullptr,"extreme signed selectors take default arm");
    HudSquad squad(nullptr);HudCommander commander(nullptr);HudVoteMap vote(nullptr);ListChat gameChat,chat;
    BfListBoxData members("members"),invites("invites"),orders("orders"),votes("votes"),gameMessages("game"),messages("messages");
    squad.m_memberList=&members;squad.m_inviteList=&invites;commander.m_data=&orders;vote.m_data=&votes;
    gameChat.answer=&gameMessages;chat.answer=&messages;
    hud->m_field120=scoreboard;hud->m_field138=&squad;hud->m_field13c=&commander;hud->m_field128=&vote;hud->m_field144=&gameChat;hud->m_field140=&chat;
    const std::vector<BfListBoxData*> answers={scoreboard->m_enemyList,scoreboard->m_friendlyList,&gameMessages,scoreboard->m_finalList,&members,&orders,&messages,&invites,scoreboard->m_enemyTotalList,scoreboard->m_friendlyTotalList,nullptr,&votes};
    for(int selector=1;selector<=12;++selector)check(hud->getListData(selector)==answers[selector-1],"twelve native list selector mappings including optional voice omission");
    check(gameChat.reads==1&&chat.reads==1,"chat list selectors use typed virtual callbacks");
    const auto pointers=std::vector<BfListBoxData*>{scoreboard->m_friendlyList,scoreboard->m_enemyList,scoreboard->m_finalList,scoreboard->m_friendlyTotalList,scoreboard->m_enemyTotalList};
    const auto removals=events.removals;scoreboard->~HudScoreboard();
    check(events.removals==removals+1&&first.releases==0&&second.releases==0,"Scoreboard destructor unregisters without releasing borrowed textures");
    check(pointers[0]->m_columns.size()==11&&pointers[1]->m_columns.size()==11,"Scoreboard destructor leaves all five lists alive");
    for(auto* list:pointers)destroyListFixture(list);

    // Separate real constructor exercises System replacement between textures.
    std::memset(memory,0xa5,sizeof(memory));first.owner=reinterpret_cast<HudScoreboard*>(memory);first.next=&second;
    scoreboard=new(memory)HudScoreboard(&first);
    check(first.names==std::vector<std::string>{"Ingame/Respawn/iconframe.tga"}&&second.names.size()==12,"constructor reloads System after first texture callback");
    check(second.names.front()=="Ingame/Scoreboard/Icons/checkbox_empty.tga"&&second.names.back()=="Ingame/Scoreboard/Icons/kickvote_no.tga"&&scoreboard->m_textures[0]==101&&scoreboard->m_textures[12]==212,"texture names and handle assignment order");
    scoreboard->~HudScoreboard();check(first.releases==0&&second.releases==0,"loaded texture handles also remain unreleased");

    // The remaining main HUD destructor is a frontier; clean its actual native
    // pre-init members without calling the unavailable global lifetime path.
    delete hud->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
    hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();::operator delete(hudStorage);hudManager=nullptr;
    // List-box destruction preserves its retail column leak; explicitly clean
    // only these six stack-owned fixtures' columns before their real destructors.
    for(auto* data:{&members,&invites,&orders,&votes,&gameMessages,&messages})for(auto* column:data->m_columns)delete column;
    std::printf("HUD Scoreboard/list selector: %d/%d checks passed\n",checks-failures,checks);return failures?1:0;
}
