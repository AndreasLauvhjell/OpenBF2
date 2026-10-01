// [TEMP] [CHOSEN, not recovered] Real HudSquad owner, table and bodies.
// Collaborators observe callbacks only; no tested production method is replaced.
#include <windows.h>
#include <bitset>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <new>
#include <string>
#include <vector>
#define private public
#define protected public
#include "../BF2/Menu/HudSquad.h"
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#include "../BF2/Menu/BfListBoxData.h"
#undef protected
#undef private
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Game/HudInformationLayer.h"
#include "../BF2/Game/MapInfo.h"
#include "../BF2/Game/IGame.h"
#include "../BF2/Game/IGameLogic.h"
#include "../BF2/Game/Objects/IPlayer.h"
#include "../BF2/Scene/Object/PlayerManager.h"
#include "../BF2/Game/GameLogic/SquadManager/ISquadManager.h"
#include "../BF2/Sound/BfSound.h"
[[noreturn]] static void actionUnsupported(const char* name) {
    std::fprintf(stderr,"[FRONTIER] HudSquad fixture unexpected %s\n",name);
    std::fflush(stderr); std::abort();
}
#include "hud_action_observers.h"
namespace dice { namespace hfe {
EventManager* eventManager = nullptr;
IBase* g_hudInformationLayer = nullptr;
IBase* squadManager = nullptr;
IBase* spawnManager = nullptr;
hud::BF2HudManager* hudManager = nullptr;
IBfSound* g_bfSound = nullptr;
namespace world {
PlayerManager* playerManager = nullptr;
class CPersistentExplosionEffectManager;
CPersistentExplosionEffectManager* explosionEffectManager = nullptr;
}
} }
// The clean friend TU (hud_squad_private_access.cpp) calls the private row helpers - see there.
void hudSquadRefreshCountsForFixture(dice::hfe::hud::HudSquad* squad, int team);
void hudSquadScrollToSquadForFixture(dice::hfe::hud::HudSquad* squad);
void hudSquadRefreshRowsForFixture(dice::hfe::hud::HudSquad* squad, int team);
using namespace dice::hfe;
using namespace dice::hfe::hud;
using namespace dice::meme;
static int checks, failures;
static std::string trace;
static void check(bool value, const char* label) {
    ++checks; if (!value) { ++failures; std::fprintf(stderr,"FAIL %s (%s)\n",label,trace.c_str()); }
}
struct Player : world::ActionPlayerBoundary {
    int id = 7;
    int getId() const override { trace += 'I'; return id; }
    int getTeam() const override { actionUnsupported("player team"); }
    int getSquadId() const override { actionUnsupported("player squad"); }
    int getKit() const override { actionUnsupported("player kit"); }
};
// Row-player observer exposes only the getters used by painters.
static int invitedQueries;
struct RowPlayer : world::ActionPlayerBoundary {
    int id = 0, squad = 0;
    bool leader = false, commander = false, alive = true, voip = false, muted = false, talking = false;
    std::string name;
    int getId() const override { return id; }
    int getSquadId() const override { return squad; }
    bool getIsSquadLeader() const override { return leader; }
    bool getIsCommander() const override { return commander; }
    bool getIsAlive() const override { return alive; }
    const std::string& getName() const override { return name; }
    bool getIsInvitedToSquad(int) const override { ++invitedQueries; return false; }
    bool getVoipEnabled() override { return voip; }
    bool getIsMuted() override { return muted; }
    bool getIsTalking() override { return talking; }
    int getTeam() const override { actionUnsupported("row player team"); }
    int getKit() const override { actionUnsupported("row player kit"); }
    // E5: the commander's member cells read the vehicle's armour; no vehicle = health 0.
    world::IObject* getVehicle() const override { return nullptr; }
};
struct Players : world::PlayerManager {
    world::IPlayer* local = nullptr;
    int queries = 0, occupied = -1;
    std::vector<int> squads;
    world::IPlayer* getLocalHumanPlayer() override { ++queries; trace += 'P'; return local; }
    int getNrOfPlayersInSquad(int team, int squad) override {
        check(team == 2, "squad count team forwarded"); squads.push_back(squad); return squad == occupied ? 1 : 0;
    }
    // Counts for total players, players in squads, and active squads.
    int teamTotal = 0, inSquads = 0, activeSquads = 0;
    int getNrOfPlayerInTeam(int team) override { check(team == 2, "team-total team"); trace += 'T'; return teamTotal; }
    int getNrOfPlayerInSquads(int team) override { check(team == 2, "in-squads team"); trace += 'Q'; return inSquads; }
    int getNrOfActiveSquads(int team) override { check(team == 2, "active-squads team"); trace += 'V'; return activeSquads; }
    // E2: refreshRows' list sources. Squad 1 is the only populated squad; every call is counted by
    // overload so the test can see which sort the flag picked.
    std::list<world::IPlayer*> squadByScore, squadByRank, teamByScore, teamByRank, empty;
    std::vector<world::IPlayer*> byId;
    int squadScoreCalls = 0, squadRankCalls = 0, teamScoreCalls = 0, teamRankCalls = 0;
    const std::list<world::IPlayer*>& getPlayersInSquadSortedByScore(int team, int squad) override {
        check(team == 2, "squad-by-score team"); ++squadScoreCalls; return squad == 1 ? squadByScore : empty;
    }
    const std::list<world::IPlayer*>& getPlayersInSquadSortedByRank(int team, int squad) override {
        check(team == 2, "squad-by-rank team"); ++squadRankCalls; return squad == 1 ? squadByRank : empty;
    }
    const std::list<world::IPlayer*>& getPlayersSortedByScore(int team) override {
        check(team == 2, "team-by-score team"); ++teamScoreCalls; return teamByScore;
    }
    const std::list<world::IPlayer*>& getPlayersSortedByRank(int team) override {
        check(team == 2, "team-by-rank team"); ++teamRankCalls; return teamByRank;
    }
    world::IPlayer* getPlayerFromId(int id) override {
        for (world::IPlayer* player : byId) if (player->getId() == id) return player;
        return nullptr;
    }
};
struct Events : EventManager {
    HudSquad* constructing = nullptr;
    bool registerEventHandler(EventCategory category, IEventListener* listener, int priority) override {
        if (constructing) {
            check(category == ECHud && priority == 0 && listener == constructing, "primary event listener registration");
            check(constructing->m_columns[0] == static_cast<int>(0xa5a5a5a5), "column defaults follow registration");
            check(constructing->m_title == constructing->m_newName && constructing->m_title == constructing->m_createTag && *constructing->m_title == 0, "shared narrow sentinel before registration");
            check(constructing->m_unassignedText == constructing->m_squadCountText && *constructing->m_unassignedText == 0, "shared wide sentinel before registration");
            constructing->m_selectedPlayers.push_back(1);
            constructing->m_excludedPlayers.push_back(2);
            constructing->m_invitePlayers.push_back(nullptr);
            constructing->m_eventPlayers.push_back(3);
            constructing->m_expanded.set(15);
        }
        return EventManager::registerEventHandler(category, listener, priority);
    }
};
struct TextureSystem : System {
    TextureSystem() : System(640, 480) {}
    std::vector<std::string> textures;
    HudSquad* owner = nullptr;
    TextureSystem* replace = nullptr;
    int offset = 0, releases = 0;
    int loadTexture(const char* name, bool menuPath) override {
        check(menuPath, "every texture load requests menu path");
        textures.push_back(name);
        if (replace) { owner->setSystem(replace); replace = nullptr; }
        return offset + static_cast<int>(textures.size());
    }
    void releaseTexture(int) override { ++releases; }
};
struct Information : HudInformationLayer {
    std::vector<std::string> names;
    std::vector<const void*> addresses;
    int registerBool(const std::string& name, bool* p) override { names.push_back(name);addresses.push_back(p);return -1; }
    int registerString(const std::string& name, const char** p) override { names.push_back(name);addresses.push_back(p);return -1; }
    int registerWideString(const std::string& name, const wchar_t** p) override { names.push_back(name);addresses.push_back(p);return -1; }
};
static int emptySquad = 1;
static std::vector<std::string> messages;
static bool prepared, flag6, flag7;
static std::string editText;
// E2: while set, getLocalizedString echoes its key, so a cell shows which key it was built from.
static bool echoKeys;
static int voHelpCalls, voHelpId, rankIconCalls;
struct Hud : BF2HudManager {
    int getPlayerVehicleIcon(world::IPlayer*, bool typeIcon) override { check(typeIcon, "vehicle icon asks the type icon"); return 0; }
    int getPlayerKitIcon(world::IPlayer* player, bool mapIcon, bool spotted) override {
        check(!mapIcon && !spotted, "kit icon flags"); return 500 + player->getId();
    }
    int getPlayerRankIcon(world::IPlayer*) override { ++rankIconCalls; return 600; }
    bool isPlayerAlive(world::IPlayer*) override { return false; }
    void enableVOHelpMessage(int id) override { ++voHelpCalls; voHelpId = id; }
    int findEmptySquad(int team) override { check(team == 2, "HUD empty-squad team");trace += 'F';return emptySquad; }
    void prepareSquadCreate(bool value) override { trace += 'C';prepared = value; }
    void setFlagE6(bool value) override { trace += '6';flag6=value; }
    void setFlagE7(bool value) override { trace += '7';flag7=value; }
    void setSquadEditText(std::string value) override { trace += 'N';editText=value; }
    std::wstring getLocalizedString(std::string key) override {
        trace += 'L';messages.push_back(key);
        // E2: the two count templates carry their tokens; every other key stays "localized".
        if (key=="HUD_TEXT_MENU_SPAWN_SQUAD_NUMBER_OF_UNASSIGNED") return L"U #UNASSIGNED#/#TOTAL#";
        if (key=="HUD_TEXT_MENU_SPAWN_SQUAD_NUMBER_OF_SQUADS") return L"S #ACTIVE#/#TOTAL#";
        if (echoKeys) return std::wstring(key.begin(), key.end());
        return L"localized";
    }
    void addServerChat(std::wstring text, int team) override { trace += 'A';check(text==L"localized"&&team==0,"localized warning forwarded"); }
};
struct Sound : BfSoundNull {
    int calls=0;
    void playSound(const std::string& sound) override { trace+='S';++calls;check(sound=="warning","warning sound name"); }
};
struct Info : ISquadInfo {
    std::string name="Alpha";
    int privacy=-1;   // E2: -1 = not expected (the earlier tests), else the answer
    const std::string& getSquadName() override {return name;}
    bool getIsPrivate() override {if(privacy<0)actionUnsupported("privacy");return privacy!=0;}
    bool getIsAccepted(SquadOrderType,OrderType) override {actionUnsupported("accepted");}
    int getSquadId() override {actionUnsupported("squad id");}
    SquadOrder* getSquadOrders() override {actionUnsupported("orders");}
    SquadOrder* getCommanderOrders() override {actionUnsupported("commander orders");}
    SquadOrder* getRequest(SquadOrderType) override {actionUnsupported("request");}
    int getSpawnGroupId() override {actionUnsupported("spawn group");}
    void addSquadLeaderSpawnPoint(world::IPlayer*) override {actionUnsupported("add spawn");}
    void removeSquadLeaderSpawnPoint(world::IPlayer*) override {actionUnsupported("remove spawn");}
};
struct Squads : ActionSquadBoundary {
    Info info;
    int infoCalls=0, leaderCalls=0;
    // E5: the commander rows ask every squad 1..9 (and drop the answer); only squad 1 is populated.
    ISquadInfo* getSquadInfo(int team,int squad) override {check(team==2&&squad>=1&&squad<=9,"squad info query");++infoCalls;return &info;}
    bool setSquadName(int,int,int,const std::string&,bool) override {actionUnsupported("squad name");}
    void inviteToSquad(int,int,int,bool,bool) override {actionUnsupported("invite");}
    void removeFromSquad(int,int,int,bool) override {actionUnsupported("remove");}
    // E2: -2 = not expected (the earlier tests), else the answer.
    int leader=-2, commander=-2, commanderCalls=0;
    int getSquadLeader(int team,int squad) override {
        if(leader==-2)actionUnsupported("leader");check(team==2&&squad>=1&&squad<=9,"squad leader query");
        ++leaderCalls;return squad==1?leader:-1;
    }
    int getCommander(int team) override {
        if(commander==-2)actionUnsupported("commander");check(team==2,"commander team");++commanderCalls;return commander;
    }
};
static void cleanupData(BfListBoxData* data) {
    for (auto* column : data->m_columns) if(column) {for(auto* cell:column->cells)delete cell;delete column;}
    delete data;
}
int main() {
    static_assert(sizeof(void*)==4,"x86 required");
    static_assert(sizeof(HudSquad)==284,"native squad owner uses four8-byte lists");
    Events events; eventManager=&events;
    Players players;world::playerManager=&players;
    Information information;g_hudInformationLayer=&information;
    events.unregisterEventHandler(ECHud,&information);
    Sound sound;g_bfSound=&sound;
    Squads squads;squadManager=&squads;
    Player player;
    alignas(HudSquad) unsigned char memory[sizeof(HudSquad)];std::memset(memory,0xa5,sizeof(memory));
    auto* expected=reinterpret_cast<HudSquad*>(memory);events.constructing=expected;
    TextureSystem first,second;first.owner=expected;first.replace=&second;second.offset=100;
    HudSquad* squad=new(memory)HudSquad(&first);events.constructing=nullptr;
    check(first.textures.size()==1&&second.textures.size()==13,"constructor reloads system after texture callback");
    check(first.textures[0]=="Ingame/Kits/Icons/kit_Commander.tga"&&second.textures[0]=="Ingame/Kits/Icons/kit_Squad.tga"&&
          second.textures.back()=="Ingame/Respawn/icon_speakerMute.tga","exact texture ordering endpoints");
    check(squad->m_textures[12]==1&&squad->m_textures[13]==101&&squad->m_textures[0]==102&&squad->m_textures[11]==113,"texture field mapping");
    check(squad->m_field074==0xa5a5a5a5&&squad->m_field08c==0xa5a5a5a5&&squad->m_field090==0xa5a5a5a5,"untouched scalar storage");
    check(reinterpret_cast<std::uintptr_t>(squad->m_memberList)==0xa5a5a5a5&&reinterpret_cast<std::uintptr_t>(squad->m_inviteList)==0xa5a5a5a5,"constructor leaves data pointers untouched");
    check(squad->m_selectedPlayers.empty()&&squad->m_excludedPlayers.empty()&&squad->m_invitePlayers.empty(),"three late list clears");
    check(squad->m_eventPlayers==std::list<int>({3})&&squad->m_expanded.to_ulong()==0x8000,"fourth list and upper bit survive registration callback");
    check(squad->m_playerId==-1&&squad->m_visibleRows==21&&squad->m_team==2&&squad->m_squadId==0,"constructor IDs and rows");
    check(!squad->m_visible&&!squad->m_isCommander&&squad->m_showCommander&&squad->m_field09f,"constructor flags");
    check(squad->m_field0ac==19&&squad->m_field0b0==2&&squad->m_field0b4==0,"constructor floats");
    check(squad->m_columns[0]==5&&squad->m_columns[5]==196&&squad->m_columns[9]==216&&squad->m_columns[11]==181,"late column settings");
    for(int row:squad->m_squadIcons)check(row==0,"squad icon cache initialized");
    check(squad->registerFunctions(),"retail true virtual");
    check(squad->registerData(),"list registration succeeds despite rejected HUD bindings");
    check(information.names==std::vector<std::string>({"TeamTopLabelString","NrOfUnassigned","NrOfSquads","SquadMemberListShow","SquadRenameShow","SquadCreateShow","SquadCreateLockedShow","SquadCreateTagString","NewSquadName","InviteListShow","IsInSquad","IsSquadLeader"}),"exact binding order");
    check(information.addresses[0]==&squad->m_title&&information.addresses[4]==&squad->m_renameVisible&&information.addresses[11]==&squad->m_isSquadLeader,"live binding addresses");
    BfListBoxData* members=squad->m_memberList;BfListBoxData* invites=squad->m_inviteList;
    check(std::string(members->getName())=="SquadMemberList"&&std::string(invites->getName())=="SquadInviteList","list names");
    check(members->m_columnCount==15&&invites->m_columnCount==3,"default column plus14/2 added columns");
    check(members->m_columns[4]->field020==166&&members->m_columns[5]->field020==69&&members->m_columns[6]->field020==30&&members->m_columns[10]->field020==80,"icon column heights");
    check(members->m_visibleRows==21&&invites->m_visibleRows==21&&members->m_field021&&!members->m_field023,"data flags and limits");
    check(!members->m_columns[13]->enabled&&!members->m_columns[14]->enabled&&!invites->m_columns[2]->enabled,"hidden columns");
    int before=players.queries;squad->handleEvent(ECGame,59,nullptr,nullptr);check(players.queries==before,"other categories skip local-player query");
    squad->handleEvent(ECHud,13,nullptr,nullptr);check(players.queries==before+1,"HUD events always query player");
    players.local=&player;struct Payload{bool enabled;int id;} payload{true,8};
    events.postEvent(ECHud,59,&payload,0);events.postEvent(ECHud,59,&payload,0);
    check(squad->m_eventPlayers==std::list<int>({3,8,8}),"event59 appends duplicate IDs");
    payload.enabled=false;events.postEvent(ECHud,59,&payload,0);
    check(squad->m_eventPlayers==std::list<int>({3,8}),"event59 removes first duplicate only");
    payload.enabled=true;payload.id=7;events.postEvent(ECHud,59,&payload,0);check(squad->m_eventPlayers.size()==2,"self event ignored");
    payload.enabled=false;payload.id=99;events.postEvent(ECHud,59,&payload,0);check(squad->m_eventPlayers.size()==2,"missing removal leaves list");
    players.local=nullptr;before=players.queries;squad->setVisible(false);check(players.queries==before&&!squad->m_visible,"hide does not query or refresh");
    squad->setVisible(true);check(players.queries==before+1&&squad->m_visible,"show with null player still sets visibility");
    void* hudMemory=::operator new(sizeof(Hud));auto* hud=new(hudMemory)Hud;hudManager=hud;
    // Checks count/localization order: team total, in-squads, localization, active squads, localization.
    players.teamTotal=12;players.inSquads=7;players.activeSquads=3;trace.clear();messages.clear();
    hudSquadRefreshCountsForFixture(squad,2);
    check(trace=="TQLVL","counts ask team, in-squads, localise, then active, localise");
    check(messages==std::vector<std::string>({"HUD_TEXT_MENU_SPAWN_SQUAD_NUMBER_OF_UNASSIGNED","HUD_TEXT_MENU_SPAWN_SQUAD_NUMBER_OF_SQUADS"}),"both count keys");
    check(std::wstring(squad->m_unassignedText)==L"U 5/12","unassigned line: team minus squads, then the total");
    check(std::wstring(squad->m_squadCountText)==L"S 3/9","squads line: active, and the constant nine");
    {const wchar_t* keepUnassigned=squad->m_unassignedText;const wchar_t* keepSquads=squad->m_squadCountText;
     hudSquadRefreshCountsForFixture(squad,2);
     check(squad->m_unassignedText==keepUnassigned&&squad->m_squadCountText==keepSquads,"unchanged text keeps its allocation");
     players.activeSquads=4;hudSquadRefreshCountsForFixture(squad,2);
     check(squad->m_unassignedText==keepUnassigned&&std::wstring(squad->m_squadCountText)==L"S 4/9","a changed line is replaced, the other kept");}
    messages.clear();
    // scrollToSquad (78E2C0): a list that fits, or no squad, scrolls to the top without asking;
    // otherwise the overflow plus the local squad's head count (the list has no node here, so
    // setFirstVisibleRow stores nothing - only the query is observable).
    {const int keepTotal=squad->m_localPlayerRow;const int keepSquad=squad->m_squadId;
     squad->m_localPlayerRow=10;players.squads.clear();hudSquadScrollToSquadForFixture(squad);
     check(players.squads.empty(),"a list that fits never asks for the squad count");
     squad->m_localPlayerRow=30;squad->m_squadId=0;hudSquadScrollToSquadForFixture(squad);
     check(players.squads.empty(),"no squad scrolls to the top without asking");
     squad->m_squadId=4;hudSquadScrollToSquadForFixture(squad);
     check(players.squads==std::vector<int>({4}),"an overflowing list asks the local squad's count");
     squad->m_localPlayerRow=keepTotal;squad->m_squadId=keepSquad;players.squads.clear();}
    // Checks row cells and exact color values through real list storage.
    {
    auto cell=[](BfListBoxData* data,int column,int row)->ListBoxCell*{
        ListBoxColumn* col=data->m_columns[column];
        return static_cast<std::size_t>(row)<col->cells.size()?col->cells[row]:nullptr;};
    auto narrowAt=[&](BfListBoxData* d,int c,int r){ListBoxCell* x=cell(d,c,r);return x?x->narrow:std::string("<none>");};
    auto wideAt=[&](BfListBoxData* d,int c,int r){ListBoxCell* x=cell(d,c,r);return x?x->wide:std::wstring(L"<none>");};
    auto textureAt=[&](BfListBoxData* d,int c,int r){ListBoxCell* x=cell(d,c,r);return x?x->texture:-999;};
    auto same=[](const Color& c,float r,float g,float b,float a){return c.r==r&&c.g==g&&c.b==b&&c.a==a;};
    RowPlayer lead,mate,fresh,boss;
    lead.id=10;lead.squad=1;lead.leader=true;lead.name="Lead";
    mate.id=11;mate.squad=1;mate.voip=true;mate.name="Mate";
    fresh.id=12;fresh.name="New";
    boss.id=13;boss.commander=true;boss.name="Boss";
    players.byId={&lead,&mate,&fresh,&boss};
    players.squadByRank={&lead,&mate};players.squadByScore={&mate,&lead};
    players.teamByScore={&lead,&mate,&fresh,&boss};players.teamByRank={&boss,&fresh,&mate,&lead};
    squads.commander=-1;squads.leader=10;squads.info.privacy=1;
    echoKeys=true;voHelpCalls=0;rankIconCalls=0;invitedQueries=0;
    squad->m_squadId=0;squad->m_isCommander=false;squad->m_showCommander=true;squad->m_field09f=true;
    squad->m_field08c=-1;squad->m_field090=-5;squad->m_playerId=11;squad->m_sortByScore=false;
    squad->m_inviteVisible=false;squad->m_expanded.reset();squad->m_expanded.set(1);
    squad->m_rowCount=99;squad->m_localPlayerRow=0;
    // Pass 1: an unassigned local player (id 11 - but listed in squad 1 so its row is painted),
    // no commander, squad 1 expanded, the invite list hidden.
    hudSquadRefreshRowsForFixture(squad,2);
    check(members->m_columns[0]->cells.size()==5&&squad->m_rowCount==5,"member rows: commander, create, squad, leader, member");
    check(narrowAt(members,14,0)=="0"&&narrowAt(members,14,1)=="3"&&narrowAt(members,14,2)=="2"&&
          narrowAt(members,14,3)=="4"&&narrowAt(members,14,4)=="4","row types in hidden column 14");
    check(narrowAt(members,13,0)=="-1"&&narrowAt(members,13,1)==""&&narrowAt(members,13,2)=="1"&&
          narrowAt(members,13,3)=="10"&&narrowAt(members,13,4)=="11","ids in hidden column 13");
    check(wideAt(members,10,0)==L"HUD_TEXT_MENU_SPAWN_SQUAD_APPLY"&&textureAt(members,12,0)==squad->m_textures[2],
          "no commander: APPLY and the empty arrow");
    check(wideAt(members,5,0)==L"HUD_HUD_MENU_COMMANDER_HEADING"&&textureAt(members,1,0)==squad->m_textures[12],"commander band caption and kit");
    check(squads.commanderCalls==3,"getCommander: the fill once, the heading twice (no selection to compare)");
    check(wideAt(members,5,1)==L"HUD_SQUADTAB_SQUAD"&&wideAt(members,10,1)==L"HUD_SQUADTAB_CREATE"&&
          narrowAt(members,12,1)=="","create row captions, no arrow");
    check(wideAt(members,10,2)==L"HUD_TEXT_MENU_SPAWN_SQUAD_JOIN","an unassigned player sees JOIN");
    check(narrowAt(members,5,2)=="\xA7" "1ALPHA" "\xA7" "0"&&narrowAt(members,6,2)=="\xA7" "12/6" "\xA7" "0",
          "squad name upper-cased; the count against the literal 6");
    check(textureAt(members,8,2)==squad->m_textures[8]&&
          same(cell(members,8,2)->color,0.7490196228027344f,0.7490196228027344f,0.5607843399047852f,1),
          "a private squad's lock in the row tan");
    check(textureAt(members,12,2)==squad->m_textures[0],"an expanded squad shows the up arrow");
    check(voHelpCalls==1&&voHelpId==12,"one VO help message per squad row for an unassigned player");
    check(players.squadRankCalls==9&&players.squadScoreCalls==0,"the rank sort asks the rank lists of all nine squads");
    check(textureAt(members,9,3)==squad->m_squadIcons[1]&&squad->m_squadIcons[1]!=0&&
          second.textures.back()=="Ingame/Player/Icons/Minimap/mini_Squad_1_small.tga",
          "the leader row loads and caches its squad icon");
    check(textureAt(members,9,4)==squad->m_textures[2],"a member row's leader column is the empty image");
    check(textureAt(members,1,3)==squad->m_textures[2]&&textureAt(members,3,3)==510,"no vehicle falls back to empty; kit icon per player");
    check(rankIconCalls==2,"the rank icon is asked for per player row and discarded");
    check(narrowAt(members,12,3)==""&&textureAt(members,12,4)==squad->m_textures[10]&&
          same(cell(members,12,4)->color,0.5759999752044678f,0.5730000138282776f,0.45100000500679016f,1),
          "voip: none for the leader, a quiet olive speaker for the member");
    check(narrowAt(members,4,4)=="\xA7" "1Mate"&&same(cell(members,4,4)->color,0,0.3803921639919281f,1,1),
          "player name without the closing code, in the friendly name colour");
    check(squad->m_localPlayerRow==4&&cell(members,0,4)->field06c&&
          same(cell(members,0,4)->highlight,0.9960784316062927f,0.7529411911964417f,0.08235294371843338f,0.3499999940395355f),
          "the local row is remembered and highlighted gold");
    check(cell(members,0,3)->field06c&&
          same(cell(members,0,3)->highlight,0.7254902124404907f,0.7098039388656616f,0.5803921818733215f,1),
          "odd player rows get the alternate band");
    check(same(cell(members,0,2)->highlight,0.4862745106220245f,0.4784313738346100f,0.3882353007793427f,1),
          "a squad row carries the heading band");
    check(invites->m_columns[0]->cells.empty(),"a hidden invite list is left alone");
    // Pass 2: the local player in squad 1, the score sort, the invite list shown with one pending
    // invitee (12, squadless) and nobody excluded.
    squad->m_squadId=1;squad->m_sortByScore=true;squad->m_inviteVisible=true;
    squad->m_selectedPlayers={12};squad->m_excludedPlayers.clear();players.occupied=1;players.squads.clear();
    players.squadRankCalls=players.squadScoreCalls=0;voHelpCalls=0;invitedQueries=0;
    hudSquadRefreshRowsForFixture(squad,2);
    check(members->m_columns[0]->cells.size()==4&&squad->m_localPlayerRow==3,"a squad member gets no create row");
    check(wideAt(members,10,1)==L"HUD_TEXT_MENU_SPAWN_SQUAD_LEAVE"&&voHelpCalls==0,"own squad: LEAVE, no VO help");
    check(same(cell(members,0,1)->color,0.4470588266849518f,0.8901960849761963f,0.3450980484485626f,1),
          "the own squad's row in the squad-chat green");
    check(players.squadScoreCalls==10&&players.teamScoreCalls==1&&players.squadRankCalls==0&&players.teamRankCalls==0,
          "the score sort: nine squads, the invite squad, then the team");
    check(invites->m_columns[0]->cells.size()==9&&squad->m_rowCount==9,"invite rows: heading, squad, invitee, padding, heading, team");
    check(wideAt(invites,0,0)==L"HUD_SQUADTAB_SQUAD"&&wideAt(invites,1,0)==L"HUD_SQUADTAB_KICK"&&narrowAt(invites,2,0)=="-1",
          "the squad heading");
    check(cell(invites,0,0)->imageX==7&&
          same(cell(invites,0,0)->highlight,0.4862745106220245f,0.4784313738346100f,0.3882353007793427f,1),
          "the heading at column 10's offset, in the band");
    check(narrowAt(invites,0,1)=="\xA7" "1Lead"&&textureAt(invites,1,1)==squad->m_textures[3]&&narrowAt(invites,2,1)=="10",
          "a squadmate: empty checkbox; the local player skipped");
    check(narrowAt(invites,0,2)=="\xA7" "1New"&&textureAt(invites,1,2)==squad->m_textures[2]&&
          same(cell(invites,0,2)->color,1,0.9f,0.2f,0.5f),"a pending invitee of another squad: no box, half alpha");
    check(narrowAt(invites,0,3)==""&&narrowAt(invites,2,3)=="-1"&&narrowAt(invites,2,6)=="-1",
          "padding: 0 excluded - 1 selected - 1 in the squad + 6 = four blank rows");
    check(wideAt(invites,0,7)==L"HUD_SQUADTAB_TEAM"&&wideAt(invites,1,7)==L"HUD_SQUADTAB_INVITE","the team heading");
    check(narrowAt(invites,2,8)=="12"&&textureAt(invites,1,8)==squad->m_textures[5],
          "the team list skips squad 1 and the commander; a selected player is ASKed");
    check(invitedQueries==1,"only the unselected squadmate asks getIsInvitedToSquad");
    check(players.squads==std::vector<int>({1}),"the padding asks the local squad's head count");
    echoKeys=false;squads.commander=-2;squads.leader=-2;squads.info.privacy=-1;players.occupied=-1;players.squads.clear();
    squad->m_squadId=0;squad->m_inviteVisible=false;squad->m_sortByScore=false;squad->m_playerId=-1;
    squad->m_selectedPlayers.clear();squad->m_invitePlayers.clear();
    players.byId.clear();players.squadByRank.clear();players.squadByScore.clear();players.teamByScore.clear();players.teamByRank.clear();
    }
    trace.clear();squad->m_renameVisible=true;check(squad->setCreateMode(false)&&trace=="FC"&&!squad->m_renameVisible&&!prepared,"disable mode closes create and rename");
    trace.clear();check(squad->setCreateMode(true)&&trace=="FC6N7F"&&flag6&&flag7&&editText=="Alpha"&&squad->m_createVisible,"enable mode name and flags in order");
    check(std::string(squad->m_createTag)=="1","actual copied create tag");
    squad->m_createLocked=true;const char* old=squad->m_createTag;squad->refreshCreateTag(false);
    check(old==squad->m_createTag&&squad->m_createLocked,"same tag preserves pointer and false reset preserves lock");
    emptySquad=-1;trace.clear();check(!squad->setCreateMode(false)&&trace=="FLAS"&&!squad->m_createVisible,"full squads warns even when disabling");
    check(messages.back()=="HUD_HUD_SQUAD_MAX_REACHED","maximum warning key");
    squad->m_isCommander=true;trace.clear();check(!squad->setCreateMode(true)&&trace=="LAS","commander warning precedes empty-squad search");
    check(messages.back()=="HUD_HUD_SQUAD_COMMANDER_MUST_RESIGN","commander warning key");
    players.occupied=1;players.squads.clear();check(squad->getAvailableSquadName(2,1)=="Bravo","colliding default name recursively advances");
    check(players.squads==std::vector<int>({0,1,0,1,2,3,4,5,6,7,8,9}),"recursive name search repeats full squad walk");
    players.occupied=-1;check(squad->getAvailableSquadName(2,7)=="Gamma","retail Gamma label");
    before=players.queries;squad->m_squadId=0;squad->leaveAndReinvite();check(players.queries==before+1,"leave helper queries before zero-squad guard");
    squad->~HudSquad();before=players.queries;events.postEvent(ECHud,9,nullptr,0);check(players.queries==before,"destructor unregisters listener");
    check(!first.releases&&!second.releases&&members->m_columnCount==15,"destructor retains texture and data owners");
    cleanupData(members);cleanupData(invites);
    // Full HUD destructor remains outside this fixture; destroy only actual constructed native members.
    delete hud->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
    hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();::operator delete(hudMemory);hudManager=nullptr;
    // Clear globals before their local targets die. Keep eventManager alive for destructor unregistration.
    g_hudInformationLayer=nullptr;squadManager=nullptr;g_bfSound=nullptr;world::playerManager=nullptr;
    std::printf("HUD squad: %d/%d checks passed\n",checks-failures,checks);return failures?1:0;
}
