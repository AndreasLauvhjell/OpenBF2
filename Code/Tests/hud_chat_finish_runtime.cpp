// [TEMP] [CHOSEN, not recovered] Native x86, bounded actual-body checks.
// HUD/Player/PlayerManager partial callback tables model only observed inputs;
// every unused entry aborts. No production method or descriptor is shadowed.
// Actual wrapMessage's System descriptor and optional resource+20 are NOT run.
#include <cstdint>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <functional>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>
#include <float.h>
#include <windows.h>
#define private public
#define protected public
#include "../BF2/Menu/HudChat.h"
#include "../BF2/Menu/BfListBoxData.h"
#include "../BF2/Menu/BfNewListBoxNode.h"
#include "../BF2/External/BFMeme/System.h"
#undef protected
#undef private
#include "../BF2/Menu/HudColors.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/External/BFMeme/Style.h"
namespace dice { namespace hfe {
namespace hud { class BF2HudManager; }
hud::BF2HudManager* hudManager = nullptr;
namespace world { class PlayerManager; PlayerManager* playerManager = nullptr; }
} }
using namespace dice::meme;
using namespace dice::hfe;
using namespace dice::hfe::hud;
static int checks, failures;
static std::vector<int> playerCallOrder;
static void check(bool value, const char* label) {
    ++checks; if (!value) { ++failures; std::printf("FAIL %s\n", label); }
    else std::printf("PASS %s\n",label);
    std::fflush(stdout);
}
static std::uint32_t bits(float value) { std::uint32_t word; std::memcpy(&word,&value,4); return word; }
static bool white(const Color& c) { return c.r==1 && c.g==1 && c.b==1 && c.a==1; }
static void cleanup(const std::vector<ListBoxColumn*>& columns) {
    for (ListBoxColumn* c : columns) if (c) { for (ListBoxCell* cell : c->cells) delete cell; delete c; }
}
static void __fastcall unexpectedChatCallback(void*, void*) {
    std::fprintf(stderr,"[TEMP] unexpected HUD chat fixture callback\n"); std::fflush(stderr); std::abort();
}
struct PlayerInput {
    void** table;
    std::string name = "Alice";
    int id = 42, team = 1, squad = 3;
    bool commander = false, leader = false;
    std::vector<int> calls;
    static void record(PlayerInput* p,int offset) { p->calls.push_back(offset);playerCallOrder.push_back(p->id*0x1000+offset); }
    static const std::string& __fastcall getName(PlayerInput* p, void*) { record(p,0x20); return p->name; }
    static int __fastcall getId(PlayerInput* p, void*) { record(p,0x50); return p->id; }
    static int __fastcall getTeam(PlayerInput* p, void*) { record(p,0xE4); return p->team; }
    static int __fastcall getSquad(PlayerInput* p, void*) { record(p,0x108); return p->squad; }
    static bool __fastcall isCommander(PlayerInput* p, void*) { record(p,0x110); return p->commander; }
    static bool __fastcall isLeader(PlayerInput* p, void*) { record(p,0x138); return p->leader; }
    world::IPlayer* asPlayer() { return reinterpret_cast<world::IPlayer*>(this); }
};
struct ManagerInput {
    void** table;
    PlayerInput* local = nullptr;
    int calls = 0;
    static world::IPlayer* __fastcall getLocal(ManagerInput* p, void*) { ++p->calls; return p->local ? p->local->asPlayer() : nullptr; }
};
struct HudInput {
    void** table;
    bool ignored = false, emptyWide = false;
    int rank = 765;
    std::vector<std::string> calls;
    std::function<void()> onLocalize;
    static bool __fastcall isIgnored(HudInput* p, void*, int id) {
        p->calls.push_back("ignored:"+std::to_string(id)); return p->ignored;
    }
    static std::wstring* __fastcall localize(HudInput* p, void*, std::wstring* out, std::string key) {
        p->calls.push_back(key);
        std::wstring result;
        if (key=="HUD_CHAT_DEADPREFIX") result=L"[dead]";
        else if(key=="HUD_TEXT_CHAT_COMMANDER") result=L"[commander]";
        else if(key=="HUD_TEXT_CHAT_TEAM") result=L"[team]";
        else if(key=="HUD_TEXT_CHAT_SQUAD") result=L"[squad]";
        if(p->onLocalize) p->onLocalize();
        return new(out) std::wstring(result);
    }
    static std::string* __fastcall narrow(HudInput* p, void*, std::string* out, const std::wstring& text) {
        p->calls.push_back("narrow");
        std::string result; for(wchar_t c:text) result += static_cast<char>(c);
        return new(out) std::string(result);
    }
    static std::wstring* __fastcall wide(HudInput* p, void*, std::wstring* out, const std::string& text) {
        p->calls.push_back("wide");
        std::wstring result; if(!p->emptyWide) for(unsigned char c:text) result += static_cast<wchar_t>(c);
        return new(out) std::wstring(result);
    }
    static int __fastcall rankIcon(HudInput* p, void*, world::IPlayer*) { p->calls.push_back("rank"); return p->rank; }
};
struct CaptureChat : HudChat {
    CaptureChat(System* system=nullptr) : HudChat(system) { for(int i=0;i<9;++i) {m_firstRow[i]=10+i;m_count[i]=0;m_capacity[i]=4;m_remaining[i]=-1;} }
    std::wstring text;
    int kind=-1, team=-1, first=-1, capacity=-1, history=-1, appends=0;
    int* count=nullptr; float* timer=nullptr; world::IPlayer* player=nullptr;
    std::vector<Color> colors;
    std::function<void(int)> onColor;
    void setColor(int k,int v,float r,float g,float b,float a) override {
        colors.push_back({r,g,b,a}); HudChat::setColor(k,v,r,g,b,a); if(onColor) onColor(v);
    }
    void appendMessage(std::wstring value,int k,int t,int f,int* c,int cap,float* rem,int h,world::IPlayer* p) override {
        ++appends; text=value;kind=k;team=t;first=f;count=c;capacity=cap;timer=rem;history=h;player=p;
    }
};
struct WrapCapture : HudChat {
    explicit WrapCapture(System* s) : HudChat(s) {}
    int wraps=0, kind=-1,team=-1,row=-1; std::wstring text;
    void wrapMessage(int k,std::wstring value,int t,int,int* count,int,int r) override {
        ++wraps; kind=k;team=t;row=r;text=value; ++*count;
    }
};
struct MetricSystem : System {
    MetricSystem() : System(800,600) {}
    std::vector<int> chars, fonts;
    float advance=4;
    bool success=false;
    int texture=876;
    std::string path;
    bool getGlyphMetrics(int character,int font,GlyphMetrics* out) override {
        chars.push_back(character); fonts.push_back(font); *out={0,advance,0,0,0}; return success;
    }
    int loadTexture(const char* name,bool menuPath) override { path=name;check(menuPath,"texture wrapper menuPath true");return texture; }
};
int main() {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    AddVectoredExceptionHandler(1, [](PEXCEPTION_POINTERS info)->LONG {
        std::fprintf(stderr,"[TEMP] exception %08lX at %08lX\n",info->ExceptionRecord->ExceptionCode, info->ContextRecord->Eip);
        std::fflush(stderr); return EXCEPTION_CONTINUE_SEARCH;
    });
    static_assert(sizeof(void*)==4,"x86 only");
    static_assert(sizeof(BfNewListBoxNode)==296,"native list reduces retail300 by4");
    static_assert(sizeof(BfListBoxData)==100 && sizeof(HudChat)==972,"native STL layout");
    void* playerTable[0x13C/4], *managerTable[0x34/4], *hudTable[0xAC/4];
    for(auto& p:playerTable)p=reinterpret_cast<void*>(&unexpectedChatCallback);
    for(auto& p:managerTable)p=reinterpret_cast<void*>(&unexpectedChatCallback);
    for(auto& p:hudTable)p=reinterpret_cast<void*>(&unexpectedChatCallback);
    playerTable[0x20/4]=reinterpret_cast<void*>(&PlayerInput::getName);
    playerTable[0x50/4]=reinterpret_cast<void*>(&PlayerInput::getId);
    playerTable[0xE4/4]=reinterpret_cast<void*>(&PlayerInput::getTeam);
    playerTable[0x108/4]=reinterpret_cast<void*>(&PlayerInput::getSquad);
    playerTable[0x110/4]=reinterpret_cast<void*>(&PlayerInput::isCommander);
    playerTable[0x138/4]=reinterpret_cast<void*>(&PlayerInput::isLeader);
    managerTable[0x30/4]=reinterpret_cast<void*>(&ManagerInput::getLocal);
    hudTable[0x38/4]=reinterpret_cast<void*>(&HudInput::localize);
    hudTable[0x50/4]=reinterpret_cast<void*>(&HudInput::narrow);
    hudTable[0x54/4]=reinterpret_cast<void*>(&HudInput::wide);
    hudTable[0x68/4]=reinterpret_cast<void*>(&HudInput::rankIcon);
    hudTable[0xA8/4]=reinterpret_cast<void*>(&HudInput::isIgnored);
    PlayerInput player, local; player.table=local.table=playerTable; local.id=43;
    ManagerInput manager; manager.table=managerTable; manager.local=&local;
    HudInput hud; hud.table=hudTable;
    world::playerManager=reinterpret_cast<world::PlayerManager*>(&manager);
    hudManager=reinterpret_cast<BF2HudManager*>(&hud);
    check(bits(defaultChatColors[0].r)==1061410570u && bits(defaultChatColors[0].b)==1059699294u,"exact default palette bits");
    check(bits(commanderChatColor.r)==1064826872u && bits(commanderChatColor.b)==1048740483u,"exact commander palette bits");
    check(white(squadChatColors[0]) && squadChatColors[1].g==squadChatColors[3].g,"squad palette copies");
    CaptureChat chat;
    chat.addRequest(L"aBc",2);
    check(chat.text==L"ABC" && chat.kind==8 && chat.team==2 && chat.history==8,"request uppercase and category forwarding");
    check(chat.count==&chat.m_count[8] && chat.timer==&chat.m_remaining[8] && *chat.timer==5,"request uses actual counter and refreshed timer");
    hud.emptyWide=true;chat.addRequest(L"fallback",1);hud.emptyWide=false;
    check(chat.text==L"fallback","request falls back when converted text empty");
    const int before=chat.appends; hud.ignored=true;
    player.calls.clear(); chat.m_remaining[0]=91; chat.addPlayerMessage(L"ignored",player.asPlayer(),44);
    check(chat.appends==before && chat.m_remaining[0]==91 && player.calls.size()==1 && player.calls[0]==0x50,"ignored player exits before timer/name/color changes");
    hud.ignored=false; hud.calls.clear(); chat.colors.clear();
    chat.addPlayerMessage(L"hello",player.asPlayer(),44);
    check(chat.text==L"Alice: hello" && chat.team==1 && chat.kind==0,"ordinary player prefix and actual player team");
    check(chat.player==nullptr && chat.count==&chat.m_count[0] && chat.colors.size()==6,"player formatter resets three default colors after append");
    hud.calls.clear(); chat.addPlayerMessage(L"HUD_CHAT_DEADPREFIXHUD_TEXT_CHAT_TEAMhello",player.asPlayer(),0);
    check(chat.text==L"[dead][team]Alice: hello","dead/team token formatting order");
    check(hud.calls.size()==4 && hud.calls[2]=="HUD_TEXT_CHAT_TEAM" && hud.calls[3]=="HUD_CHAT_DEADPREFIX","group localization precedes dead localization");
    chat.colors.clear(); chat.addPlayerMessage(L"HUD_TEXT_CHAT_SQUADhello",player.asPlayer(),0);
    check(chat.text==L"[squad]Alice: hello" && white(chat.colors[0]) && chat.colors[1].g==squadChatColors[1].g,"squad token selects squad palette");
    player.commander=true;
    chat.addPlayerMessage(L"HUD_CHAT_DEADPREFIXHUD_TEXT_CHAT_COMMANDERhello",player.asPlayer(),0);
    check(chat.text==L"[dead][commander]Alice: hello","commander prefix formatting");
    chat.addPlayerMessage(L"HUD_CHAT_DEADPREFIXhello",player.asPlayer(),0);
    check(chat.text==L"Alice: HUD_CHAT_DEADPREFIXhello","commander no-group preserves raw dead token defect");
    player.commander=false;
    chat.addPlayerMessage(L"XHUD_TEXT_CHAT_TEAMhello",player.asPlayer(),0);
    check(chat.text==L"[team]Alice: Mhello","matched token is removed by prefix length rather than found position");
    player.squad=local.squad=3; player.team=local.team=1; chat.colors.clear();playerCallOrder.clear();
    chat.addKillMessage(L"kill",player.asPlayer(),1);
    check(chat.colors.size()==6 && white(chat.colors[0]) && chat.colors[1].g==squadChatColors[1].g,"same-squad message palette");
    check(playerCallOrder==std::vector<int>({42*0x1000+0x110,42*0x1000+0x108,43*0x1000+0xE4,42*0x1000+0xE4,43*0x1000+0x108,42*0x1000+0x108}),"local team/squad queries precede sender queries");
    player.squad=5; local.commander=true; player.leader=true;chat.colors.clear();
    chat.addKillMessage(L"kill",player.asPlayer(),1);
    check(white(chat.colors[0]),"commander sees other squad leader in squad palette");
    local.commander=false;player.leader=false;chat.colors.clear();
    chat.addKillMessage(L"kill",player.asPlayer(),1);
    check(chat.colors[0].r==defaultChatColors[0].r,"different squad default palette");
    const Color saved=defaultChatColors[1];chat.onColor=[](int v){if(v==0)defaultChatColors[1].r=.125f;};
    chat.addKillMessage(L"palette",player.asPlayer(),1);
    check(chat.m_colors[1][0].r==.125f,"palette reread after virtual color callback");
    chat.onColor={};defaultChatColors[1]=saved;

    MetricSystem system;
    alignas(BfNewListBoxNode) unsigned char nodeStorage[sizeof(BfNewListBoxNode)];
    std::memset(nodeStorage,0xCD,sizeof(nodeStorage));
    BfNewListBoxNode* node=new(nodeStorage) BfNewListBoxNode;
    check(node->m_pendingEvents.empty() && node->m_firstRow==0 && node->m_border && node->m_right==2,"real node constructor/list/border defaults");
    check(bits(node->m_selection.b)==0x3F09FBE7u && node->m_field0e8[0]==0xCDCDCDCDu,"exact selection bits and untouched node words");
    BfListBoxData data("fixture");data.m_node=node;data.m_activeColumn=data.m_columns[0];
    data.selectRow(2,true);data.selectRow(2,true);data.selectRow(3,false);
    check(data.m_row==3 && node->m_pendingEvents.size()==1 && node->m_pendingEvents.front()==7,"row selection changed/notify filtering");
    data.m_field023=false;data.selectRow(3,true);
    check(node->m_pendingEvents.size()==2,"repeated row can notify when flag disabled");
    node->m_firstRow=7;check(data.getFirstVisibleRow()==7,"typed node first-visible query");
    data.m_node=nullptr;check(data.getFirstVisibleRow()==0,"no node first-visible result");data.m_node=node;
    node->m_width=30;node->m_fonts[0]=70;node->m_fonts[1]=71;node->m_fonts[3]=73;
    data.m_field021=false;data.appendWideCell(0,L"first",.2f,.3f,.4f,.5f,4);
    data.appendWideCell(0,L"second",1,1,1,1,9);
    ListBoxCell* cell=data.m_columns[0]->cells[0];
    cell->resourceIndex=51;cell->field090=21;cell->field068=55;
    data.setImageCell(0,0,91,12,13,1,2,.2f,.3f,.4f,.5f,77);
    check(cell->texture==91 && cell->isImage && !cell->isWide && cell->wide==L"first" && cell->field064==4,"image update retains old strings/alignment");
    check(cell->resourceIndex==-1 && cell->field090==77 && cell->imageY==2,"image update resets resource index and sets extra");
    data.setImageCell(0,500,92,14,15,3,4,.1f,.2f,.3f,.4f,99);
    check(data.m_columns[0]->cells.size()==3 && white(data.m_columns[0]->cells.back()->color) && data.m_columns[0]->cells.back()->field090==0,"image fallback appends white and drops extra/row");
    data.copyCell(0,1,cell);
    ListBoxCell* dest=data.m_columns[0]->cells[1];
    check(dest->texture==91 && dest->wide==L"first" && dest->field064==9 && dest->field068==1 && dest->field090==77,"copy keeps destination alignment and mode");
    data.moveRow(0,1);
    check(dest->texture==91 && !cell->isImage && cell->narrow.empty() && cell->wide.empty() && white(cell->color),"move copies row before clearing source");
    check(cell->field064==4 && cell->field068==55 && cell->resourceIndex==-1 && cell->field090==0,"clear preserves alignment/mode but resets index/extra");
    data.moveRow(1,1);check(dest->wide.empty() && dest->texture==0,"self move clears row");
    check(data.copyCell(-1,0,nullptr) && data.clearCell(0,-1) && data.copyRow(-1,0),"invalid coordinates preserve successful no-op branches");
    data.m_columns[0]->width=0;data.m_activeColumn->field020=-1;
    check(data.getColumnRightEdge(1)==22 && data.getAvailableWidth(0,0)==22,"right edge excludes border and fixed six pixels");
    data.m_field021=true;check(data.getAvailableWidth(0,0)==24,"scroll-width subtraction path");data.m_field021=false;
    int mode=0;system.chars.clear();system.fonts.clear();
    check(data.measureText(&system,L"abc",0,0,&mode)==3 && system.chars.size()==3 && system.fonts[0]==70,"false metric results still contribute written widths");
    system.chars.clear();system.fonts.clear();mode=0;
    check(data.measureText(&system,L"\xA7" L"1ab\xA7" L"3c",0,0,&mode)==7 && mode==3 && system.fonts[0]==71 && system.fonts[2]==73,"font control tags consume text and update mode");
    system.chars.clear();mode=0;
    check(data.measureText(&system,L"\xA7" L"CabcdZ",0,0,&mode)==7 && system.chars.size()==1 && system.chars[0]=='Z',"six-character color escape skips payload");
    system.chars.clear();mode=0;
    check(data.measureText(&system,L"abcdefg",0,0,&mode)==6,"measurement includes first overflowing character");
    system.chars.clear();mode=0;
    data.measureText(&system,std::wstring(1,static_cast<wchar_t>(0xFF80)),0,0,&mode);
    check(system.chars[0]==0xFF80,"wide character zero extends full16-bit value");
    node->m_width=10000;system.chars.clear();mode=0;
    check(data.measureText(&system,std::wstring(300,L'a'),0,0,&mode)==250 && system.chars.size()==250,"retail250-character scan cap");
    system.advance=std::numeric_limits<float>::quiet_NaN();mode=0;
    check(data.measureText(&system,L"nan",0,0,&mode)==3,"unordered width comparison continues");system.advance=4;
    const unsigned oldControl=_controlfp(0,0);_controlfp(_RC_UP,_MCW_RC);
    node->m_width=30.25f;check(data.getColumnRightEdge(1)==22,"floor under changed rounding mode");
    check((_controlfp(0,0)&_MCW_RC)==_RC_UP,"x87 width conversion restores rounding control");_controlfp(oldControl,_MCW_RC);
    TypeDescriptor* type=BfListBoxData::getClassType();
    check(std::strcmp(type->getName(),"dice::meme::BfListBoxData")==0 && type->getBaseType()==ListBoxData::getClassType() && type->getInstanceSize()==100,"complete34-slot descriptor identity/ancestry/size");
    BfListBoxData* created=static_cast<BfListBoxData*>(type->create());
    check(created && created->getType()==type && created->getFirstVisibleRow()==0 && created->m_node==nullptr,"real descriptor factory and complete virtual queries");
    const auto leakedCreated=created->m_columns;delete created;cleanup(leakedCreated);
    const auto leaked=data.m_columns;data.m_columns.clear();cleanup(leaked);
    node->~BfNewListBoxNode();
    WrapCapture append(&system);append.setCapacity(0,10);append.init("append",16,200);
    int count=0;float time=123;
    append.appendMessage(L"",6,0,0,&count,10,&time,9,nullptr);check(append.wraps==0 && count==0,"empty append no row allocation");
    append.appendMessage(L"ignored",0,0,0,&count,10,&time,9,nullptr);check(append.wraps==0 && count==0,"kind0 invalid team consumes no text");
    append.appendMessage(L"plain",3,2,0,&count,10,&time,9,nullptr);check(append.kind==3 && append.team==2 && append.text==L"plain" && count==1,"kind3 keeps team and plain text");
    append.appendMessage(L"rank",4,0,0,&count,10,&time,9,player.asPlayer());
    check(append.text==L"\xA7" L"1rank\xA7" L"0" && append.m_data->m_columns[0]->cells[1]->texture==hud.rank,"rank icon and decorated wrapping");
    append.appendMessage(L"personal",5,0,0,&count,10,&time,9,nullptr);
    check(system.path=="Ingame/GeneralIcons/empty.dds" && append.m_data->m_columns[0]->cells[2]->texture==system.texture && time==123,"personal image wrapper and untouched timer pointer");
    append.appendMessage(L"notice",7,0,0,&count,10,&time,9,nullptr);
    check(append.text==L"\xA7" L"1notice\xA7" L"0" && append.kind==7,"notice tag wrapping");
    const auto leakedAppend=append.m_data->m_columns;append.m_data->m_columns.clear();cleanup(leakedAppend);
    std::printf("Chat finish diagnostic: %d/%d checks passed\n",checks-failures,checks);
    return failures ? 1 : 0;
}
