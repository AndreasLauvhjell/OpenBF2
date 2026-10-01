// [TEMP] [RETIRE-WHEN: HUD-INIT-FIXTURE] Native recovered init prefix and both
// registration helpers. Typed owner callbacks return observed failure values;
// font-file lookup intentionally throws after real System initialization.
// No production init/helper/constructor/descriptor is shadowed.
#include "font_runtime_support.h"
#include "../BF2/Menu/HudMessages.h"
#include "../BF2/Menu/HudSquad.h"
#include "../BF2/Menu/HudCommander.h"
#include "../BF2/Menu/HudScoreboard.h"
#include "../BF2/Menu/HudVoteMap.h"
#include "../BF2/Menu/HudInformationAdapter.h"
#include "../BF2/Menu/BfListBoxData.h"
#define private public
#define protected public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/Menu/Bf2HudBuilder.h"
#include "../BF2/Menu/BfEditData.h"
#include "../BF2/Menu/Ref2System.h"
#undef protected
#undef private
#include "../BF2/Menu/HudBlinker.h"
#include "../BF2/Menu/HudColors.h"
#include "../BF2/Game/HudInformationLayer.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#include "../BF2/External/BFMeme/DynamicIndexPointer.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include <functional>
// [TEMP] The test-only access macro changes MSVC's access decoration for these
// nonvirtual methods. Alias to the actual private production symbols; no body
// is substituted and the parameter/return ABI remains identical.
#pragma comment(linker,"/alternatename:?registerHudFunctions@BF2HudManager@hud@hfe@dice@@QAE_NXZ=?registerHudFunctions@BF2HudManager@hud@hfe@dice@@AAE_NXZ")
#pragma comment(linker,"/alternatename:?registerHudData@BF2HudManager@hud@hfe@dice@@QAE_NXZ=?registerHudData@BF2HudManager@hud@hfe@dice@@AAE_NXZ")
namespace dice { namespace hfe {
EventManager* eventManager=nullptr;
IBase* g_hudInformationLayer=nullptr;
hud::BF2HudManager* hudManager=nullptr;
} }
using namespace dice::hfe::hud;
static std::vector<std::string> calls;
static BF2HudManager* activeHud;
struct StopFile {};
struct PrefixFiles : Files {
    bool fileExists(const std::string& path) override {
        check(path=="BF1942.font","actual init reaches default font lookup");
        auto* system=activeHud->m_field0f4;
        check(system && system->getWidth()==800 && system->getHeight()==600,"HUD stores constructed800x600 System before initializing it");
        check(system->m_field020 && system->m_field020->getType()==dice::meme::Pointer::getClassType(),"actual base System init creates its real Pointer before font lookup");
        check(system->m_registry && system->m_eventQueue && system->m_field024,"real base System registry,queue and Style precede font lookup");
        check(!activeHud->m_field100&&!activeHud->m_field0fc&&!activeHud->m_field120,"owned HUD widgets follow root init");
        calls.push_back("font");
        std::fprintf(stderr,"[FRONTIER] fixture external font-file boundary reached after real HUD/System init prefix\n");
        std::fflush(stderr);throw StopFile{};
    }
};
struct Binding { char kind;std::string name;const void* address; };
static std::vector<Binding> bindings;
struct Information : HudInformationLayer {
    std::function<void()> callback;
    int count=0;
    int record(char kind,const std::string& name,const void* address) {
        ++count;
        bindings.push_back({kind,name,address});calls.push_back(name);
        if(callback)callback();return -1;
    }
    int registerInt(const std::string& n,int* p) override {return record('I',n,p);}
    int registerFloat(const std::string& n,float* p) override {return record('F',n,p);}
    int registerBool(const std::string& n,bool* p) override {return record('B',n,p);}
};
#define INIT_OWNER_OBSERVER(Type, Name) \
struct Name : Type { \
    Name():Type(nullptr){} \
    std::function<void()> callback; \
    bool registerFunctions() override {calls.push_back(#Name "F");if(callback)callback();return false;} \
    bool registerData() override {calls.push_back(#Name "D");if(callback)callback();return false;} \
};
INIT_OWNER_OBSERVER(HudScoreboard,Score)
INIT_OWNER_OBSERVER(HudVoteMap,Vote)
INIT_OWNER_OBSERVER(HudSquad,SquadOwner)
INIT_OWNER_OBSERVER(HudCommander,Commander)
#undef INIT_OWNER_OBSERVER
struct Adapter : HudInformationAdapter {
    std::function<void()> callback;
    bool registerFunctions() override {calls.push_back("AdapterF");if(callback)callback();return false;}
    bool registerData() override {calls.push_back("AdapterD");if(callback)callback();return false;}
};
struct Messages : HudMessages {
    bool registerData() override {calls.push_back("MessagesD");return false;}
};
static void clearBindings(BF2HudManager* h) {
    delete h->m_field0a8[0];delete h->m_field0a8[1];delete h->m_field0dc;
    delete h->m_field0b4[0];delete h->m_field0b4[1];delete h->m_field0e0;
    h->m_field0a8[0]=h->m_field0a8[1]=h->m_field0dc=nullptr;
    h->m_field0b4[0]=h->m_field0b4[1]=h->m_field0e0=nullptr;
}
static void destroyPreInit(BF2HudManager* h) {
    delete h->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;
    using SL=std::list<std::string>;using V=decltype(h->m_field32c);
    h->m_field010.~W();h->m_field038.~W();h->m_field060.~W();
    h->m_field08c.~S();h->m_field0c0.~S();h->m_field304.~S();
    h->m_field0e8.~L();h->m_field2e0.~SL();h->m_field33c.~SL();h->m_field32c.~V();
    ::operator delete(h);
}
int main() {
    setvbuf(stdout,nullptr,_IONBF,0);
    AddVectoredExceptionHandler(1,[](PEXCEPTION_POINTERS p)->LONG {
        if(p->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION){
            std::fprintf(stderr,"Fixture AV ip=%08lx base=%p esp=%08lx\n",p->ContextRecord->Eip,GetModuleHandleA(nullptr),p->ContextRecord->Esp);
            const auto* words=reinterpret_cast<const unsigned*>(p->ContextRecord->Esp);
            for(int i=0;i!=12;++i)std::fprintf(stderr,"stack[%d]=%08x\n",i,words[i]);
            std::fflush(stderr);
        }
        return EXCEPTION_CONTINUE_SEARCH;
    });
    EventManager manager;eventManager=&manager;
    auto* h=new BF2HudManager;hudManager=h;activeHud=h;
    check(sizeof(BF2HudManager)==824,"opaque14C pointer retains native HUD layout");
    Bf2HudBuilder builder;
    check(sizeof(builder)==140 && builder.m_field008==nullptr,"typed builder map field preserves lifetime/layout");
    check(categoryThreeChatColors[0].r==1&&categoryThreeChatColors[0].b==1&&
          categoryThreeChatColors[1].r==1&&categoryThreeChatColors[1].b==0&&
          categoryThreeChatColors[2].g==1&&categoryThreeChatColors[2].b==0&&
          categoryThreeChatColors[3].g==0&&categoryThreeChatColors[3].b==1,"new mutable palette has exact8655A0 initializer colors");
    const dice::meme::Color paletteExpected[]={{1,1,1,1},{1,1,0,1},{1,1,0,1},{1,0,1,1}};
    for(unsigned i=0;i!=4;++i)check(std::memcmp(&categoryThreeChatColors[i],&paletteExpected[i],sizeof(paletteExpected[i]))==0,"all16bytes of each palette entry match static initializer");
    PrefixFiles* files=new PrefixFiles;io::fileManager=files; // file manager wider teardown remains open
    const int pointers=dice::meme::Pointer::getClassType()->getInstanceCount();
    bool stopped=false;try {h->init();}catch(StopFile&){stopped=true;}
    std::puts("HUD prefix stopped; closing the real base System");
    check(stopped&&calls==std::vector<std::string>{"font"},"real main init reaches intentional external-file stop");
    h->m_field0f4->dice::meme::System::close();
    delete h->m_field0f4;h->m_field0f4=nullptr;
    check(dice::meme::Pointer::getClassType()->getInstanceCount()==pointers,"actual base close balances prefix Pointer");
    std::puts("Constructing typed registration observers");
    Adapter adapter;Score score;Vote vote;SquadOwner squad;Commander commander;Messages messages;
    Information info,next;g_hudInformationLayer=&info;
    std::puts("Testing HUD registration helpers");
    h->m_field104=&adapter;h->m_field120=&score;h->m_field128=&vote;
    h->m_field138=&squad;h->m_field13c=&commander;h->m_field148=&messages;
    calls.clear();check(h->registerHudFunctions(),"registration helper preserves retail true despite callback failures");
    check(calls==std::vector<std::string>{"AdapterF","ScoreF","VoteF","SquadOwnerF","CommanderF"},"function registration exact native owner order");
    adapter.callback=[&]{h->m_field120=nullptr;h->m_field138=nullptr;};
    calls.clear();check(h->registerHudFunctions(),"callback replacement keeps registration success");
    check(calls==std::vector<std::string>{"AdapterF","VoteF","CommanderF"},"function registration reloads and null-checks each later owner");
    std::puts("Function registration complete; testing data registration");
    adapter.callback={};h->m_field120=&score;h->m_field138=&squad;
    h->m_field08c="old chat";h->m_field0c0="old rename";
    calls.clear();bindings.clear();
    info.callback=[&]{g_hudInformationLayer=&next;};
    check(h->registerHudData(),"data registration discards every callback failure");
    const std::vector<std::string> expected={"AdapterD","ScoreD","VoteD","SquadOwnerD","CommanderD","MessagesD",
        "HudState","DisconnectMessageActive","PauseMessageActive","CommanderTracking","SetSpawnPoint",
        "CommanderRightNameColorRed","CommanderRightNameColorGreen","CommanderRightNameColorBlue","InvalidVehicleDropArea"};
    check(calls==expected,"data owners precede exact9binding order");
    check(h->m_field08c.empty()&&h->m_field0c0.empty(),"both source strings reset");
    const char* names[]={"ChatBoxString","CommanderChatBoxString","SquadRenameString"};
    dice::meme::Data* strings[]={h->m_field0a8[0],h->m_field0a8[1],h->m_field0dc};
    for(int i=0;i!=3;++i){
        check(std::strcmp(strings[i]->getName(),names[i])==0,"actual StringData constructor receives name");
        check(strings[i]->getCharacter(nullptr,0)==0,"actual StringData copies empty contents");
    }
    check(std::strcmp(h->m_field0b4[0]->getName(),"SayData")==0&&
        std::strcmp(h->m_field0b4[1]->getName(),"CommanderSayData")==0&&
        std::strcmp(h->m_field0e0->getName(),"SquadRenameData")==0,"three actual EditData constructors receive distinct names");
    const void* addresses[]={&h->m_field250,&h->m_field0bd,&h->m_field0be,&h->m_field2b0,&h->m_field228,
        &h->m_field350,&h->m_field358,&h->m_field354,&h->m_field35c};
    const char kinds[]="IBBBBFFFB";
    check(bindings.size()==9,"all nine data bindings observed across replacement globals");
    for(unsigned i=0;i<bindings.size();++i)check(bindings[i].address==addresses[i]&&bindings[i].kind==kinds[i],"binding points to actual native field with shipped color-order defect");
    check(g_hudInformationLayer==&next,"information global reloaded after callback");
    check(info.count==1&&next.count==8,"only first binding uses old information owner; all remaining callbacks use replacement");
    clearBindings(h);
    adapter.callback=[&]{h->m_field120=nullptr;h->m_field138=nullptr;};
    calls.clear();bindings.clear();
    check(h->registerHudData(),"changed data owners still return retail true");
    check(calls.size()==13&&calls[0]=="AdapterD"&&calls[1]=="VoteD"&&calls[2]=="CommanderD"&&calls[3]=="MessagesD", "data registration reloads and skips callback-cleared later owners");
    clearBindings(h);
    h->m_field104=nullptr;h->m_field120=nullptr;h->m_field128=nullptr;
    h->m_field138=nullptr;h->m_field13c=nullptr;h->m_field148=nullptr;
    calls.clear();check(h->registerHudFunctions()&&calls.empty(),"all-null functions return true without callbacks");
    calls.clear();bindings.clear();check(h->registerHudData()&&bindings.size()==9,"all-null owners still construct own data and bind scalars");
    clearBindings(h);destroyPreInit(h);hudManager=nullptr;
    std::printf("HUD init prefix and registration: %d/%d checks passed\n",checks-failures,checks);
    return failures?1:0;
}
