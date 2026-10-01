// [TEMP] [CHOSEN, not recovered] Real System/Ref2System init, empty-root update,
// and close. Pointer enable/input snapshots below arrange recorded conditions.
// Only file bytes and external renderer/texture factories are modeled. No
// System/HUD method or descriptor is overridden. No rendering or device polling.
#include "font_runtime_support.h"
#define private public
#define protected public
#include "../BF2/Menu/Ref2System.h"
#include "../BF2/External/BFMeme/EventQueue.h"
#include "../BF2/External/BFMeme/DynamicIndexPointer.h"
#undef protected
#undef private
#include "../BF2/External/BFMeme/Node.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/Core/SettingsRepostitory.h"
namespace dice { namespace hfe { SettingsRepostitory* settings=nullptr; } }
using namespace dice::meme;
static std::vector<std::string> events;
static Ref2System* activeRoot;
struct Ref : IBase {
    std::string name;
    int releases=0;
    explicit Ref(const char* n) : name(n) {}
    void addRef() override {}
    int getRef() override { return 1; }
    int release() override {
        ++releases;events.push_back(name+":release");
        if(activeRoot) check(!activeRoot->m_registry && !activeRoot->m_eventQueue &&
            !activeRoot->m_field020 && !activeRoot->m_field024,"base close before renderer release");
        return 0;
    }
    void* queryInterface(unsigned int) const override { return nullptr; }
};
struct LifecycleTexture : Texture {
    std::string id;
    int flags=-1;
    void* link=nullptr;
    explicit LifecycleTexture(const std::string& n) : id(n) {}
    void setTexture(const char* n,const char* p,int f) override {
        Texture::setTexture(n,p,f);flags=f;events.push_back(id+":setTexture");
    }
    int release() override { ++releases;events.push_back(id+":release");return 0; }
    void setUserObject(void* p) override { link=p; }
    void* getUserObject() const override { return link; }
};
struct LifecycleClasses : Classes {
    Ref view{"view"},renderer{"renderer"};
    std::vector<LifecycleTexture*> textures;
    IBase* createInstance(unsigned int cid,unsigned int iid,IBase* outer) override {
        if(cid==io::CID_MemStream) return Classes::createInstance(cid,iid,outer);
        if(cid==820301) {
            check(iid==820201 && !outer,"render view IDs");
            events.push_back("view:create");
            check(activeRoot->m_field020->getType()==Pointer::getClassType(),"base pointer exists before view");
            check(activeRoot->m_fonts.size()==1,"font loaded before view");
            return &view;
        }
        check(cid==geom::CID_MemeTexture && iid==geom::IID_IMemeTexture && !outer,"texture IDs");
        auto* result=new LifecycleTexture(std::to_string(textures.size()));
        textures.push_back(result);events.push_back(result->id+":create");
        if(activeRoot && textures.size()==2) {
            check(activeRoot->m_field020->getType()==DynamicIndexPointer::getClassType(),"dynamic pointer before blank texture");
            bool clear=true;
            const auto* bytes=reinterpret_cast<const unsigned char*>(activeRoot->m_inputStates);
            for(unsigned int i=0;i<sizeof(activeRoot->m_inputStates);++i) clear=clear && bytes[i]==0;
            check(clear,"input states cleared before blank texture");
        }
        return result;
    }
    IBase* getSingleton(const std::string& name) override {
        check(name=="Renderer","renderer singleton name");
        events.push_back("renderer:get");
        check(activeRoot->m_renderView==&view,"view stored before singleton lookup");
        const auto* bytes=reinterpret_cast<const unsigned char*>(activeRoot->m_inputStates);
        check(bytes[0]==0xA5 && bytes[255]==0xA5,"input-state clear follows singleton lookup");
        return &renderer;
    }
};
struct LifecycleFiles : Files {
    int mode=-1,access=-1;
    unsigned int argument=99;
    io::IStream* open(const std::string& path,FileMode m,FileAccess a,unsigned int value) override {
        mode=int(m);access=int(a);argument=value;
        return Files::open(path,m,a,value);
    }
};
struct LifetimeFont : Font {
    int& destroyed;
    int* finalCount;
    LifetimeFont(const char* name,int refs,int& count,int* final) : Font(name),destroyed(count),finalCount(final) { referenceCount()=refs; }
    ~LifetimeFont() override { ++destroyed;*finalCount=referenceCount();events.push_back(std::string(getName())+":delete"); }
};
int main() {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    LifecycleFiles* files=new LifecycleFiles; // FileManager full teardown remains outside fixture.
    LifecycleClasses classes;io::fileManager=files;classManager=&classes;
    const int initialStyle=Style::getClassType()->getInstanceCount();
    const int initialPointer=Pointer::getClassType()->getInstanceCount();
    const int initialDynamic=DynamicIndexPointer::getClassType()->getInstanceCount();
    {
        System base(640,480);
        check(base.init(),"real System init returns true");
        check(base.m_registry && base.m_eventQueue && base.m_field024 && base.m_field020,"base collaborators created");
        check(base.m_registry->findObject(&base,"System")==base.m_field024,"real named style registered");
        check(base.m_field024->getType()==Style::getClassType(),"style descriptor published safely");
        check(Style::getClassType()->getInstanceCount()==initialStyle+1,"style registration increments counter");
        check(Pointer::getClassType()->getInstanceCount()==initialPointer+1,"pointer creation increments counter");
        int nodes=0;for(auto* node=base.m_eventQueue->m_free;node;node=node->next)++nodes;
        check(nodes==11 && !base.m_eventQueue->m_first && !base.m_eventQueue->m_last,"real eleven-node event queue");
        check(!base.getRootNode() && base.getNodeState()->target==-3 && base.getNodeState()->current==2,
            "base initial empty root and encoded state");
        check(base.update(0.125f),"real initialized System empty-root update succeeds");
        check(base.getNodeState()->target==-3 && base.getNodeState()->current==-3,
            "real NodeState update sets current inside flag");
        for(int type : {1,2,3,4,5,99}) base.enqueueEvent(type,7,1.5f,-2.5f,0);
        check(base.m_eventQueue->m_first && base.m_eventQueue->m_last,"base queues six actual events");
        auto* basePointer=static_cast<Pointer*>(base.m_field020);
        basePointer->m_enabled=true; // [TEMP] Enable the real allocated Pointer, no override.
        base.m_pointerX=4; base.m_pointerY=-5;
        check(base.update(0.25f),"real System drains queue and updates enabled real Pointer");
        nodes=0;for(auto* node=base.m_eventQueue->m_free;node;node=node->next)++nodes;
        check(nodes==11 && !base.m_eventQueue->m_first && !base.m_eventQueue->m_last,
            "base update recycles six events into original pool");
        check(base.m_previousPointerX==4 && base.m_previousPointerY==-5,
            "base Pointer update runs after state and queue work");
        check(base.m_field020==basePointer && !base.getRootNode(),"update preserves actual pointer and empty root");
        check(base.close(),"real System close succeeds");
        check(!base.m_registry && !base.m_eventQueue && !base.m_field020 && !base.m_field024,"base close clears owned fields");
        check(Style::getClassType()->getInstanceCount()==initialStyle && Pointer::getClassType()->getInstanceCount()==initialPointer,"base close balances descriptors");
    }
    files->data["BF1942.font"]="existence only";
    files->data["Fonts/BF1942.font"]="Version = 1\nTexture = atlas\nCharacterHeight = 12\n0x41 16 32 8\n";
    Ref2System root(800,600);activeRoot=&root;
    std::memset(root.m_inputStates,0xA5,sizeof(root.m_inputStates));
    check(root.init(),"real Ref2System init returns true");
    check(events==std::vector<std::string>({"0:create","0:setTexture","view:create","renderer:get","1:create","1:setTexture"}),"exact font/view/renderer/blank init order");
    check(root.m_blankTexture==classes.textures[1] && classes.textures[1]->name.empty() &&
        classes.textures[1]->prefix.empty() && classes.textures[1]->flags==0,"blank texture exact initialization");
    check(root.m_renderer==&classes.renderer && !root.m_textureList,"renderer borrowed and font texture outside list");
    check(Pointer::getClassType()->getInstanceCount()==initialPointer &&
        DynamicIndexPointer::getClassType()->getInstanceCount()==initialDynamic+1,"dynamic replacement destroys base pointer");
    check(root.m_fonts["BF1942.font"]->referenceCount()==1,"default font owned once");
    check(!root.getRootNode() && root.update(0.125f),"real Ref2System update after init succeeds with empty root");
    check(root.getNodeState()->target==-3 && root.getNodeState()->current==-3,
        "Ref2 update reaches real shared state transition");
    auto* dynamic=static_cast<DynamicIndexPointer*>(root.m_field020);
    dynamic->m_enabled=true; dynamic->m_timer=1.0f; // [TEMP] Existing real object, bounded enabled/timer state.
    root.m_pointerX=7; root.m_pointerY=-8;
    root.m_inputs[0].mappingFlags=1ULL<<io::GIUp;
    root.m_inputs[0].inputs[io::GIUp]=1.0f;
    check(root.update(0.125f),"Ref2 press traverses real preUpdate, queue, and DynamicIndexPointer");
    check(root.m_inputStates[0][8]==1 && dynamic->m_lastInputIndex==0 && dynamic->m_indexOrder[0]==1,
        "actual input snapshot press reaches dynamic index mapping");
    check(closeFloat(dynamic->m_timer,0.875f) && root.m_previousPointerX==7 && root.m_previousPointerY==-8,
        "enabled DynamicIndexPointer timer and base coordinate update execute");
    check(!root.m_eventQueue->m_first && !root.m_eventQueue->m_last,"preUpdate press is drained in same update");
    check(root.update(0.5f) && root.m_inputStates[0][8]==6 && closeFloat(dynamic->m_timer,0.375f),
        "held input repeat and pointer timer share actual half-second update");
    check(!root.m_eventQueue->m_first && !root.m_eventQueue->m_last,"repeat event drains before return");
    check(root.update(1.5f) && root.m_inputStates[0][8]==6 && closeFloat(dynamic->m_timer,0.375f),
        "Ref2 greater-than-one delta becomes zero across whole update pipeline");
    check(root.update(1.0f) && root.m_inputStates[0][8]==16 && closeFloat(dynamic->m_timer,-0.625f),
        "Ref2 exactly-one delta remains unchanged");
    root.m_inputs[0].inputs[io::GIUp]=0.0f;
    check(root.update(0.125f) && root.m_inputStates[0][8]==0,"real Ref2 release clears input state");
    int rootNodes=0;for(auto* node=root.m_eventQueue->m_free;node;node=node->next)++rootNodes;
    check(rootNodes==11 && !root.m_eventQueue->m_first && !root.m_eventQueue->m_last,
        "press repeat and release preserve original eleven-node queue pool");
    check(!root.getRootNode() && root.m_field020==dynamic && root.m_fonts.size()==1,
        "component updates preserve empty root and initialized resources");
    check(events==std::vector<std::string>({"0:create","0:setTexture","view:create","renderer:get","1:create","1:setTexture"}),
        "update does not invoke external renderer or resource factories");
    files->data["menu/output.ui"]="000000";
    files->opens.clear();files->exists.clear();
    OStream* output=root.createOutputStream("output.ui");
    check(files->opens==std::vector<std::string>({"menu/output.ui"}) && files->exists.empty(),"output prefixes menu without existence query");
    check(files->mode==3 && files->access==2 && files->argument==0,"output FileManager mode/access");
    check(output->write("ABC",3)==3 && output->getPosition()==3 && output->setPosition(1),"real FileOStream write/seek");
    root.deleteOutputStream(output);
    LifecycleTexture head("head"),tail("tail");head.link=&tail;root.m_textureList=&head;
    int destroyed[4]={},finalCounts[4]={};
    auto* zero=new LifetimeFont("a-zero",0,destroyed[0],&finalCounts[0]);
    auto* one=new LifetimeFont("b-one",1,destroyed[1],&finalCounts[1]);
    auto* two=new LifetimeFont("c-two",2,destroyed[2],&finalCounts[2]);
    auto* three=new LifetimeFont("d-three",3,destroyed[3],&finalCounts[3]);
    root.m_fonts.emplace("a-zero",zero);root.m_fonts.emplace("b-one",one);
    root.m_fonts.emplace("c-two",two);root.m_fonts.emplace("d-three",three);
    const auto beforeMap=root.m_fonts;
    events.clear();
    check(root.close(),"real Ref2System close returns true");
    check(events==std::vector<std::string>({"view:release","head:release","1:release","0:release","b-one:delete","c-two:delete"}),"close exact renderer/head/blank/font order and ordered keys");
    check(head.releases==1 && tail.releases==0,"close releases only texture-list head");
    check(root.m_renderView==&classes.view && root.m_blankTexture==classes.textures[1] &&
        root.m_textureList==&head && root.m_fonts==beforeMap,"retail close preserves released fields and local cache entries");
    check(zero->referenceCount()==-1 && three->referenceCount()==2,"zero and larger refcounts decremented once");
    check(!destroyed[0] && destroyed[1]==1 && destroyed[2]==1 && !destroyed[3] &&
        finalCounts[1]==0 && finalCounts[2]==0,"one/two refcounts delete at zero");
    check(Style::getClassType()->getInstanceCount()==initialStyle &&
        DynamicIndexPointer::getClassType()->getInstanceCount()==initialDynamic,"real root close balances style/dynamic descriptors");
    check(!classes.renderer.releases,"borrowed renderer never released");
    activeRoot=nullptr;delete zero;delete three; // fixture-only surviving payload cleanup

    // Shared count2 branch must remove global entry before the second decrement.
    files->data["Fonts/shared-close.dif"]="header 1 Shared Font\n64 12\nglyphs 1\n65 1 2 3 0 1 2 3 4\n";
    Ref2System sharedA(1,1),sharedB(1,1),reload(1,1);
    const int shared=sharedA.loadFont("Fonts/shared-close.dif");
    check(sharedB.loadFont("Fonts/shared-close.dif")==shared &&
        reinterpret_cast<Font*>(shared)->referenceCount()==2,"shared cache has two owners");
    Ref isolatedView("isolated");LifecycleTexture isolatedBlank("isolated");
    sharedA.m_renderView=&isolatedView;sharedA.m_blankTexture=&isolatedBlank;
    const auto oldTextureCount=classes.textures.size();
    check(sharedA.close(),"count2 shared close returns");
    check(sharedA.m_fonts["Fonts/shared-close.dif"]==reinterpret_cast<Font*>(shared) &&
        sharedB.m_fonts["Fonts/shared-close.dif"]==reinterpret_cast<Font*>(shared),"both local maps retain old identity without dereference");
    const auto oldOpens=files->opens.size();
    check(reload.loadFont("Fonts/shared-close.dif")!=0 && files->opens.size()>oldOpens &&
        classes.textures.size()==oldTextureCount+1,"global key removed so reload performs real IO");
    // Surviving reload cache deliberately process-lifetime; repeating close on
    // the now-dangling shared/local entries is the preserved shipped defect.
    std::printf("Ref2 lifecycle runtime: %d checks, %d failures\n",checks,failures);
    return failures?1:0;
}
