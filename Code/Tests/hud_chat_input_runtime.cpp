// [TEMP] Real HUD chat/edit bodies, real Data/EditData/Node storage and input ring.
// Typed observers record adapter/player and unresolved createSquad interactions.
// No HUD init/destructor, descriptor publication or network send is exercised.
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <clocale>
#include <cstdint>
#include <functional>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <new>
#define private public
#define protected public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/HudBlinker.h"
#include "../BF2/Menu/BfEditData.h"
#include "../BF2/Menu/BfEditNode.h"
#undef protected
#undef private
#include "../BF2/Menu/HudInformationAdapter.h"
#include "../BF2/Menu/Ref2System.h"
#include "../BF2/Game/Main/BF2.h"
#include "../BF2/Game/Objects/IPlayer.h"
#include "../BF2/Game/IGameLogic.h"
#include "../BF2/Scene/Object/PlayerManager.h"
#include "../BF2/Core/EventManager.h"
[[noreturn]] static void fixtureUnsupported(const char* name)
{ std::fprintf(stderr,"[FRONTIER] chat input fixture unexpected %s\n",name);std::fflush(stderr);std::abort(); }
#include "hud_kit_observers.h" // complete typed Player interface; all unused calls abort
namespace dice { namespace hfe {
// [TEMP] Actual ring's named storage, normally defined beside the WM_CHAR writer.
char consoleCharBuffer[256]; int consoleCharCount,consoleCharRead,consoleCharWrite;
EventManager* eventManager = nullptr;
namespace world {
PlayerManager* playerManager = nullptr;
class CPersistentExplosionEffectManager;
CPersistentExplosionEffectManager* explosionEffectManager = nullptr;
}
} }
using namespace dice::hfe;
using namespace dice::hfe::hud;
using namespace dice::meme;
static unsigned checks, failures;
static std::vector<std::string> trace;
static void check(bool ok,const char* name)
{ ++checks; if(!ok){++failures;std::printf("FAIL %s\n",name);for(auto& s:trace)std::printf("  %s\n",s.c_str());} }
static bool sequence(std::initializer_list<const char*> expected)
{ if(trace.size()!=expected.size())return false;unsigned i=0;for(auto s:expected)if(trace[i++]!=s)return false;return true; }
struct Hud : BF2HudManager {
    bool closeChat() override { trace.push_back("close");return BF2HudManager::closeChat(); }
    void getString(Data* d,std::string& s) override { trace.push_back("getString");BF2HudManager::getString(d,s); }
    void setSquadEditText(std::string s) override {
        trace.push_back("setEdit");BF2HudManager::setSquadEditText(s);
        m_field0e6=true; // lets processChatInput prove its post-call flag clear
    }
    std::string getSquadEditText() override { trace.push_back("getEdit");return BF2HudManager::getSquadEditText(); }
    bool isSquadCreateActive() override { trace.push_back("active?");return BF2HudManager::isSquadCreateActive(); }
    void setSquadCreateActive(bool b) override { trace.push_back("active=0");BF2HudManager::setSquadCreateActive(b); }
    void createSquad(std::string s) override { trace.push_back("create:"+s);m_field0c0="changed during create"; }
};
struct Adapter : HudInformationAdapter {
    const char* name;
    std::function<void()> callback;
    explicit Adapter(const char* n):name(n){}
    bool setBool(std::string key,bool value) override {
        trace.push_back(std::string(name)+":"+key+(value?":1":":0"));
        if(callback){auto f=callback;callback={};f();}return false;
    }
};
struct Text : StringData {
    const char* name;
    std::function<void()> callback;
    Text(const char* n):StringData("old"),name(n){}
    bool setString(System* s,const char* text) override {
        trace.push_back(std::string(name)+":"+text);
        StringData::setString(s,text);
        if(callback){auto f=callback;callback={};f();}return false;
    }
};
struct Player : world::PlayerBoundary {
    int squad=0;mutable int calls=0;
    int getSquadId() const override { ++calls;return squad; }
    bool getIsAlive() const override { fixtureUnsupported("getIsAlive"); }
    int getTeam() const override { fixtureUnsupported("getTeam"); }
    bool getIsSquadLeader() const override { fixtureUnsupported("getIsSquadLeader"); }
    int getKit() const override { fixtureUnsupported("getKit"); }
    int getChosenUnlockLevel(int,int) const override { fixtureUnsupported("getChosenUnlockLevel"); }
    void setChosenUnlockLevel(int,int,int) override { fixtureUnsupported("setChosenUnlockLevel"); }
    bool isUnlocked(int,int,int) override { fixtureUnsupported("isUnlocked"); }
};
struct Players : world::PlayerManager {
    world::IPlayer* local=nullptr;int calls=0;
    world::IPlayer* getLocalHumanPlayer() override { ++calls;return local; }
};
static void ring(std::initializer_list<unsigned char> bytes,int start=0)
{
    std::memset(consoleCharBuffer,0,sizeof consoleCharBuffer);
    int i=start;for(unsigned char b:bytes){if(i>255)i=0;consoleCharBuffer[i++]=static_cast<char>(b);}
    consoleCharRead=start;consoleCharWrite=i;consoleCharCount=static_cast<int>(bytes.size());
}
static void attach(BfEditData& edit,BfEditNode& node,Data& text)
{ edit.m_activeNode=&node;node.m_editData=&edit;node.m_string=&text; }
static void destroyConstructedHud(Hud* h)
{
    // [TEMP] Constructor-only cleanup; the unpublished HUD destructor is not called.
    delete h->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<std::string>;
    using I=std::list<int>;using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    h->m_field33c.~L();h->m_field32c.~V();h->m_field304.~S();h->m_field2e0.~L();
    h->m_field0e8.~I();h->m_field0c0.~S();h->m_field08c.~S();
    h->m_field060.~W();h->m_field038.~W();h->m_field010.~W();::operator delete(h);
}
int main()
{
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    std::setvbuf(stdout,nullptr,_IONBF,0);
    static_assert(sizeof(void*)==4,"x86 only");
    check(std::setlocale(LC_ALL,".1252")!=nullptr,"CP1252 for literal one-byte input cases");
    Hud* h=new Hud; BF2HudManager* api=h;
    menu::Ref2System system(800,600);
    EventManager events; eventManager=&events;
    Adapter a("A"),b("B");
    BfEditData normal,alternate,squad,replacement;
    BfEditNode normalNode,alternateNode,squadNode,replacementNode;
    Text normalText("N"),alternateText("A"),squadText("S"),replacementText("R");
    attach(normal,normalNode,normalText);attach(alternate,alternateNode,alternateText);
    attach(squad,squadNode,squadText);attach(replacement,replacementNode,replacementText);
    h->m_field0f4=&system;h->m_field104=&a;h->m_field0b4[0]=&normal;
    h->m_field0b4[1]=&alternate;h->m_field0e0=&squad;h->m_field0dc=&squadText;h->m_field081=true;
    normalNode.m_focused=alternateNode.m_focused=squadNode.m_focused=true;
    h->m_field0b0=h->m_field0b1=h->m_field0b2=h->m_field0e4=h->m_field0e5=true;
    a.callback=[&]{h->m_field104=&b;};
    bool flagsDuringClear=false;
    normalText.callback=[&]{flagsDuringClear=h->m_field0b0&&h->m_field0e4&&h->m_field081;};
    trace.clear();check(api->closeChat(),"close returns true despite all adapter/setString failures");
    check(sequence({"close","A:SayAllShow:0","B:SayTeamShow:0","B:SaySquadShow:0","B:RenameSquadShow:0","N:","A:","S:"}),
          "close order and adapter reloads");
    check(flagsDuringClear,"clear callbacks precede category and creation flag stores");
    check(!h->m_field0b0&&!h->m_field0b1&&!h->m_field0b2&&!h->m_field0e4&&!h->m_field0e5&&!h->m_field081,
          "close resets all six flags");
    check(!normalNode.m_focused&&!alternateNode.m_focused&&!squadNode.m_focused,"close clears all focus states");
    h->m_field104=&a;trace.clear();api->toggleSayAll(false);
    check(trace.size()==9&&trace.front()=="close"&&trace.back()=="A:SayAllShow:1","toggle closes before enable");
    check(h->m_field0b0&&h->m_field081&&normalNode.m_focused&&!alternateNode.m_focused,"normal toggle selects normal node");
    trace.clear();api->toggleSayAll(true);
    check(trace.size()==8&&!h->m_field0b0&&!h->m_field081,"already-open toggle only closes even if alternate changes");
    h->m_field0b1=true;h->m_field081=false;trace.clear();api->toggleSayTeam(true);
    check(trace.size()==9&&trace.back()=="A:SayTeamShow:1"&&alternateNode.m_focused&&!normalNode.m_focused,
          "category flag alone does not close toggle; alternate focus selected");
    alternate.m_focusLocked=true;api->closeChat();api->toggleSayAll(true);
    check(alternateNode.m_focused&&h->m_field0b0,"locked focus retained while category toggles");
    alternate.m_focusLocked=false;api->closeChat();
    Players players;world::playerManager=&players;Player player;
    trace.clear();api->toggleSaySquad(true);
    check(players.calls==1&&trace.empty(),"alternate squad still requires local player");
    players.local=&player;api->toggleSaySquad(false);
    check(player.calls==1&&trace.empty(),"squad zero rejects normal toggle");
    api->toggleSaySquad(true);
    check(player.calls==2&&h->m_field0b2&&alternateNode.m_focused,"alternate checks squad before bypassing zero");
    int managerCalls=players.calls;trace.clear();api->toggleSaySquad(false);
    check(players.calls==managerCalls&&trace.size()==8,"already-open squad closes before any player query");
    player.squad=-1;api->toggleSaySquad(false);
    check(h->m_field0b2&&normalNode.m_focused,"negative squad word accepted by nonzero test");
    api->closeChat();

    io::GameInput input{};ring({'x'});trace.clear();
    check(!api->processChatInput(input,-1,nullptr)&&consoleCharRead==0&&consoleCharCount==1,
          "null edit preserves ring");
    ring({});input.mappingFlags=0x60;input.inputs[io::GILeft]=1;input.inputs[io::GIRight]=1;
    normalNode.m_pendingInput=0;check(!api->processChatInput(input,-1,&normal),"movement alone is not enter");
    check(normalNode.m_pendingInput==1&&normalNode.m_pendingKey==5,"left input has precedence");
    input.inputs[io::GILeft]=0.5f;api->processChatInput(input,-1,&normal);
    check(normalNode.m_pendingKey==7,"left threshold is strict and falls through to right");
    input.mappingFlags=0;normalNode.m_pendingInput=0;api->processChatInput(input,-1,&normal);
    check(normalNode.m_pendingInput==0,"unmapped axis values ignored");
    ring({'x',13,'y'});check(api->processChatInput(input,-1,&normal),"enter survives later queued text");
    check(normalNode.m_pendingInput==2&&normalNode.m_pendingCharacter=='y'&&consoleCharCount==0,"last text overwrites queued input");
    ring({'x',0,'z'});check(!api->processChatInput(input,-1,&normal),"NUL ends drain without enter");
    check(consoleCharCount==1&&consoleCharRead==2&&normalNode.m_pendingCharacter=='x',"NUL consumed but tail retained");
    api->processChatInput(input,-1,&normal);check(normalNode.m_pendingCharacter=='z',"next call drains retained tail");
    ring({'q'},256);api->processChatInput(input,-1,&normal);
    check(consoleCharRead==1&&normalNode.m_pendingCharacter=='q',"read index above255 wraps before load");
    ring({'r',0});consoleCharCount=0;api->processChatInput(input,-1,&normal);
    check(normalNode.m_pendingCharacter=='r'&&consoleCharCount==0&&consoleCharRead==2,"index mismatch drains with zero count and clamps decrement");
    ring({'s'});consoleCharWrite=consoleCharRead;api->processChatInput(input,-1,&normal);
    check(normalNode.m_pendingCharacter=='s'&&consoleCharCount==0,"nonzero count drains despite equal indices");
    h->m_field0bc=false;normalNode.m_pendingInput=0;ring({9,27,31,128,255});
    api->processChatInput(input,-1,&normal);check(normalNode.m_pendingInput==0,"default filter rejects controls and high bytes after conversion");
    ring({127});api->processChatInput(input,-1,&normal);check(normalNode.m_pendingCharacter==127,"default filter includes DEL127");
    h->m_field0bc=true;ring({128,255});api->processChatInput(input,-1,&normal);
    check(normalNode.m_pendingCharacter==255,"extended flag forwards unsigned high byte");
    normalNode.m_pendingInput=0;ring({9});api->processChatInput(input,-1,&normal);
    check(normalNode.m_pendingInput==0,"tab remains ignored even with extended flag");
    ring({27});api->processChatInput(input,-1,&normal);check(normalNode.m_pendingCharacter==27,"extended flag forwards ESC to node");
    h->m_field0bc=false;h->m_field0e6=true;normalNode.m_pendingInput=0;trace.clear();ring({'x',13});
    check(api->processChatInput(input,'x',&normal)&&h->m_field0e6&&trace.empty(),"ignored character and Enter do not clear selection");
    ring({8});trace.clear();api->processChatInput(input,-1,&normal);
    check(sequence({"setEdit","S:","S:"})&&!h->m_field0e6,"backspace clears selection through virtual setter then resets callback-written flag");
    check(normalNode.m_pendingInput==1&&normalNode.m_pendingKey==15,"backspace queues internal key15");
    h->m_field0e6=true;ring({'a','b'});trace.clear();api->processChatInput(input,-1,&normal);
    check(trace.size()==3&&normalNode.m_pendingCharacter=='b'&&!h->m_field0e6,"first accepted character clears selection once");

    trace.clear();squadText.callback=[&]{h->m_field0e0=&replacement;};
    h->BF2HudManager::setSquadEditText(std::string("ab\0tail",7));
    check(sequence({"S:","S:ab"})&&replacementNode.m_cursor==2&&squadNode.m_cursor==0,
          "squad setter reloads edit owner after callbacks and cursor uses C-string length");
    h->m_field0e0=&squad;h->m_field0c0="stale";trace.clear();
    std::string copy=api->getSquadEditText();
    check(copy=="ab"&&h->m_field0c0=="ab"&&sequence({"getEdit","getString"}),"getter uses virtual extraction into member then returns copy");
    h->m_field0c0="later";check(copy=="ab","returned squad string is independent");
    squadNode.m_focused=true;h->m_field0e4=true;trace.clear();api->prepareSquadCreate(false);
    check(sequence({"active?","getEdit","getString","active=0"})&&!squadNode.m_focused&&!h->m_field0e4,
          "prepare false performs discarded getter before focus and active reset");
    trace.clear();api->prepareSquadCreate(false);
    check(sequence({"active?","active=0"}),"inactive prepare skips getter");
    squadNode.m_focused=true;h->m_field0e4=true;trace.clear();api->prepareSquadCreate(true);
    check(sequence({"S:","S:"})&&squadNode.m_focused&&h->m_field0e4,"prepare clear branch leaves focus and active flag untouched");
    h->m_field0c0="new squad";trace.clear();api->submitSquadCreate();
    check(trace.size()==9&&trace[0]=="create:new squad"&&trace[1]=="close"&&h->m_field0c0.empty(),
          "submit passes by-value text, closes after callback then clears changed member");
    destroyConstructedHud(h);world::playerManager=nullptr;
    std::printf("HUD chat input runtime: %u/%u checks passed\n",checks-failures,checks);
    return failures?1:0;
}
