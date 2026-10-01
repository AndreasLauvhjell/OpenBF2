// [TEMP] Actual filterPlayerInput, native input setters and bounded widget calls.
// [RETIRE-WHEN: HUD-PLAYER-FILTER-FIXTURE] Replace observers/retained pre-init HUD
// and world objects when their real client lifetimes cover the same branches.
#include "../BF2/Menu/HudMessages.h"
#include "../BF2/Menu/HudVehicleInformation.h"
#define main unusedChatRegression
#include "hud_chat_send_runtime.cpp"
#undef main
#include "../BF2/Menu/HudInformationAdapter.h"
#include "../BF2/Menu/HudRadio.h"
#include "../BF2/Menu/Ref2System.h"
#include "../BF2/Game/Objects/Object.h"
#include "../BF2/Game/Objects/ICameraObject.h"
#include "../BF2/Game/Objects/IVehicleHudComp.h"
#include "../BF2/Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "../BF2/IO/Input/PlayerInput.h"
#include "../BF2/Core/Ids.h"
#include "../BF2/Core/Debug.h"
#include "hud_radio_action_observers.h"
#include "hud_vehicle_storage_fixture.h"
#include <cmath>
#include <climits>
#include <cfloat>
struct FilterFrontier {};
namespace dice { namespace hfe {
IBase* g_hudInformationLayer=nullptr;
namespace world { class ObjectManager; ObjectManager* objectManager=nullptr; }
namespace hud {
struct HudPlayerFilterFixture {
    static void setup(HudMessages& m,int selected,int count,int id) {
        m.m_selectedItem=selected;m.m_itemCount=count;m.m_items[selected].id=id;
        m.m_duration=10;m.m_field020=0.3;m.m_field028=0;m.m_visible=true;
    }
    static float duration(const HudMessages& m){return m.m_duration;}
    static double deadline(const HudMessages& m){return m.m_field028;}
    static void deadline(HudMessages& m,double v){m.m_field028=v;}
    static void cooldown(HudMessages& m,double v){m.m_field020=v;}
    static bool visible(const HudMessages& m){return m.m_visible;}
};
struct HudVehicleInformationFixture {
    static void setup(HudVehicleInformation& h,int gui,int saved,float first,float second){
        h.m_guiIndex=gui;h.m_field0a4=saved;h.m_playerAngle=first;h.m_vehicleAngle=second;
        h.m_unknown28c=123;
    }
    static int saved(const HudVehicleInformation& h){return h.m_field0a4;}
    static float input(const HudVehicleInformation& h){return h.m_unknown28c;}
};
} } }
struct FilterLogic : Logic {
    int status=1;unsigned reads=0;
    int getGameStatus() override {++reads;trace.push_back("status");return status;}
};
struct FilterGame : Game {
    bool demo=false;
    bool isPlayingDemo() override {trace.push_back("demo");return demo;}
};
struct FilterPlayer : Player {
    world::IObject* vehicle=nullptr,*defaultVehicle=nullptr;
    mutable std::deque<world::ICameraObject*> cameras;
    world::ICameraObject* camera=nullptr;
    std::function<void()> onVehicle;
    world::IObject* getVehicle() const override {trace.push_back("vehicle");if(onVehicle)onVehicle();return vehicle;}
    world::IObject* getDefaultVehicle() override {trace.push_back("default");return defaultVehicle;}
    world::ICameraObject* getCamera() const override {
        trace.push_back("camera");if(cameras.empty())return camera;
        auto* p=cameras.front();cameras.pop_front();return p;
    }
};
struct FilterHud : BF2HudManager {
    int mode=0;bool commander=false,chat=false,create=false,pointer=false;
    std::deque<int> modes;unsigned modeReads=0;
    std::vector<int> disabled,enabled;
    std::vector<std::pair<int,bool>> transitions;
    std::function<void()> onChat,onCreate,onPointer,onTransition;
    std::function<void(int)> onHelp;
    bool isChatActive() override {trace.push_back("chat");if(onChat)onChat();return chat;}
    bool isSquadCreateActive() override {trace.push_back("create");if(onCreate)onCreate();return create;}
    bool isPointerEnabled() override {trace.push_back("pointer");if(onPointer)onPointer();return pointer;}
    int getMode() override {trace.push_back("mode");++modeReads;if(modes.empty())return mode;int v=modes.front();modes.pop_front();return v;}
    bool isCommanderShown() override {trace.push_back("commander");return commander;}
    void setMode(int m,bool save) override {transitions.push_back({m,save});mode=m;trace.push_back("transition");if(onTransition)onTransition();}
    void enableVOHelpMessage(int id) override {enabled.push_back(id);trace.push_back("enable"+std::to_string(id));if(onHelp)onHelp(id);}
    void disableVOHelpMessage(int id) override {disabled.push_back(id);trace.push_back("disable"+std::to_string(id));if(onHelp)onHelp(-id);}
};
struct FilterRadio : HudRadio {
    int id;bool enabled=true;std::function<void()> callback;
    explicit FilterRadio(int i):id(i){}
    bool isEnabled() override {trace.push_back("radio"+std::to_string(id));if(callback)callback();return enabled;}
};
struct FilterAdapter : HudInformationAdapter {
    int zoom=1;unsigned reads=0;
    std::function<void()> callback;
    int getInt(std::string name) override {check(name=="MapZoom","zoom binding name");++reads;trace.push_back("zoom");if(callback)callback();return zoom;}
    bool setInt(std::string name,int) override {
        std::fprintf(stderr,"[FRONTIER] fixture map setup beyond setInt reached\n");std::fflush(stderr);
        check(name=="MapZoom","real adapter reaches map zoom write");throw FilterFrontier{};
    }
};
struct FilterSystem : menu::Ref2System {
    unsigned owners=0;dice::meme::Node* owner=nullptr;
    FilterSystem():Ref2System(800,600){}
    dice::meme::Node* getPointerOwner() override {++owners;trace.push_back("owner");return owner;}
};
struct FilterObject : world::Object {
    world::IPlayerControlObject* control=nullptr;
    FilterObject():Object(nullptr){}
    void* queryInterface(unsigned iid) const override {check(iid==world::IID_IPlayerControlObject,"ancestor asks IPCO");trace.push_back("query");return control;}
    void parent(world::IObject* p){m_parent=p;}
    void flags(unsigned f){m_flags=f;}
    void component(world::IVehicleHudComp* c){m_vehicleHudComponent=c;}
};
struct FilterPco : world::RadioPcoBoundary {
    world::IObject* weapon=nullptr;
    std::map<int,world::IPlayerControlObject*>* getPcos() override {actionUnsupported("unused pcos");}
    world::IPlayer* getOccupingPlayer() const override {actionUnsupported("unused occupying player");}
    world::IObject* getActiveWeapon(int slot) const override {check(slot==0,"actual active weapon slot zero");trace.push_back("weapon");return weapon;}
};
struct FilterVehicleComp : world::IVehicleHudComp {
    world::VehicleHudInfo info{};std::function<void()> onInfo;
    world::VehicleHudInfo* getHudInfo() override {trace.push_back("info");if(onInfo)onInfo();return &info;}
    void addRef() override {actionUnsupported("component addRef");}
    int getRef() override {actionUnsupported("component getRef");}
    int release() override {actionUnsupported("component release");}
    void* queryInterface(unsigned) const override {actionUnsupported("component query");}
    void setObject(world::IObject*) override {actionUnsupported("component setObject");}
    world::IObject* getObject() const override {actionUnsupported("component getObject");}
    int getGuiIndex(bool) override {actionUnsupported("component gui");}
    world::IPostProcessComp* getEnablePostProcessing(int) override {actionUnsupported("component pp");}
    std::string getVehicleIcon(int) override {actionUnsupported("component icon");}
    int getVehicleIconPtr(int) override {actionUnsupported("component icon ptr");}
    std::string getAbilityIcon(int) override {actionUnsupported("component ability");}
    int getAbilityIconPtr(int) override {actionUnsupported("component ability ptr");}
    bool getShowOnCamMode(int) override {actionUnsupported("component mode");}
    void playingPantingSound(bool) override {actionUnsupported("component pant");}
    bool getIsPlayingPantingSound() override {actionUnsupported("component pant query");}
    void playInjurySound() override {actionUnsupported("component injury");}
    world::VehicleHudIcons* getHudIcons() override {actionUnsupported("component icons");}
};
struct FilterCamera : world::ICameraObject {
    bool look=false;unsigned reads=0;std::function<void()> callback;
    bool getToggleMouseLook() override {++reads;trace.push_back("look");if(callback)callback();return look;}
    void addRef() override {actionUnsupported("camera addRef");}
    int getRef() override {actionUnsupported("camera getRef");}
    int release() override {actionUnsupported("camera release");}
    void* queryInterface(unsigned) const override {actionUnsupported("camera query");}
    const Mat4& getTransformation(float) override {actionUnsupported("camera transformation");}
    void setInterpolatedMode(bool) override {actionUnsupported("camera interpolate");}
    bool setViewMode(world::CameraViewMode,world::IObject*,bool) override {actionUnsupported("camera view");}
    world::CameraViewMode getViewMode() const override {actionUnsupported("camera view query");}
    bool isActive() const override {actionUnsupported("camera active");}
    void setIsActive(bool) override {actionUnsupported("camera activate");}
    void setCameraId(int) override {actionUnsupported("camera id");}
    int getCameraId() override {actionUnsupported("camera id query");}
    void prevViewMode() override {actionUnsupported("camera previous");}
    void setTraceObjectId(unsigned) override {actionUnsupported("camera trace");}
    unsigned getTraceObjectId() override {actionUnsupported("camera trace query");}
    void setExternCameraTrans(Mat4) override {actionUnsupported("camera external");}
    Mat4* getExternCameraTrans() override {actionUnsupported("camera external query");}
    int getPcoId() override {actionUnsupported("camera pco");}
    void handleExplosion(float) override {actionUnsupported("camera explosion");}
    void resetCameraView(bool) override {actionUnsupported("camera reset");}
    void updateCameraView(bool) override {actionUnsupported("camera update");}
    float getActiveFOV() const override {actionUnsupported("camera fov");}
    bool getAllowDynamicLights() const override {actionUnsupported("camera lights");}
    bool getAllowDynamicSun() const override {actionUnsupported("camera sun");}
    const Vec3& getStaticSunDirection() const override {actionUnsupported("camera direction");}
    const Vec3& getStaticSunColor() const override {actionUnsupported("camera color");}
    const Vec3& getStaticSunDamageColor() const override {actionUnsupported("camera damage");}
    float getInsideDamageFrequency() const override {actionUnsupported("camera frequency");}
    float getInsideDamageThreshold() const override {actionUnsupported("camera threshold");}
    const Vec3& getStaticSunAmbientColor() const override {actionUnsupported("camera ambient");}
};
#ifndef HUD_PLAYER_FILTER_NO_MAIN
int main(int argc,char** argv) {
    setvbuf(stdout,nullptr,_IONBF,0);
    if(argc>1) {
        setDebugCallback(DebugTypeError,[](DebugType,const std::string&,const std::string&,
            int,const std::string&,const std::string& message,const std::string&,void*) {
            std::fprintf(stderr,"Observed real input diagnostic: %s\n",message.c_str());
            std::fflush(stderr);
        },nullptr);
        io::PlayerInput negative{};negative.mappingFlags=~0ULL;
        if(argv[1][0]=='b')negative.setInput(static_cast<io::PlayerInputMap>(-1),true);
        else negative.setInput(static_cast<io::PlayerInputMap>(-1),1.0f);
        return 42; // A returned negative setter would fail the separate death check.
    }
    static_assert(sizeof(io::PlayerInput)==280,"native x86 input stride");
    static_assert(sizeof(BF2HudManager)==824,"HUD typed scalars preserve native layout");
    EventManager events;eventManager=&events;
    Players players;world::playerManager=&players;
    FilterPlayer player;FilterLogic logic;FilterGame current;
    game=&current;gameLogic=&logic;
    auto* h=new FilterHud;hudManager=h;
    HudMessages messages,replacement;
    HudVehicleInformation vehicle;
    FilterRadio r0(0),r1(1),r2(2),r3(3),r4(4);
    FilterAdapter adapter;FilterSystem system;
    FilterCamera c1,c2;
    auto* object=new FilterObject;auto* parent=new FilterObject;auto* weapon=new FilterObject;
    FilterPco control;FilterVehicleComp component;
    io::PlayerInput input;
    auto reset=[&] {
        input={};input.mappingFlags=~0ULL;input.dword0=0x12345678;input.dword1=0xABCDEF01;
        input.flag=0xA5;input.byte1=0x39;input.byte2=0x18;input.byte3=0x27;
        h->m_field0bf=false;h->m_field248=true;h->hudVehicleInformation=nullptr;
        h->m_field148=&messages;h->ref2System=&system;h->m_field104=&adapter;h->hudMap=nullptr;
        h->m_field108=&r0;h->m_field10c=&r1;h->m_field110=&r2;h->m_field114=&r3;h->m_field118=&r4;
        h->chat=h->create=h->pointer=h->commander=false;h->mode=0;h->modes.clear();h->modeReads=0;
        h->m_field2fc=false;h->m_field300=7;h->m_field2f8=0;h->m_field2f5=false;h->m_field328=64;
        h->m_field2b4=42;h->m_field2b8=-2;
        h->disabled.clear();h->enabled.clear();h->transitions.clear();
        h->onChat={};h->onCreate={};h->onPointer={};h->onTransition={};h->onHelp={};
        logic.status=1;logic.reads=0;gameLogic=&logic;game=&current;current.demo=false;
        players.local=nullptr;players.onQuery={};players.queries=0;players.replies.clear();
        player.vehicle=player.defaultVehicle=nullptr;player.camera=nullptr;player.cameras.clear();player.onVehicle={};
        object->control=parent->control=nullptr;object->parent(nullptr);object->component(nullptr);weapon->flags(0);
        control.weapon=nullptr;component.info.useSelectionIcons=false;component.onInfo={};
        c1.look=c2.look=false;c1.reads=c2.reads=0;c1.callback={};c2.callback={};
        HudPlayerFilterFixture::setup(messages,0,6,1);HudPlayerFilterFixture::setup(replacement,0,6,2);
        r0.enabled=r1.enabled=r2.enabled=r3.enabled=r4.enabled=true;
        r0.callback={};r1.callback={};r2.callback={};r3.callback={};r4.callback={};
        adapter.zoom=1;adapter.reads=0;adapter.callback={};system.owner=nullptr;system.owners=0;
        WorldPref::resetWorldTime(10);trace.clear();
    };
    auto run=[&] {return h->filterPlayerInput(input);};
    auto stopped=[&] {try{run();return false;}catch(FilterFrontier&){return true;}};
    std::puts("Player filter guards and chat masks");
    for(int guard=0;guard!=4;++guard){reset();input.inputs[8]=1;auto before=input;
        if(guard==0){h->m_field0bf=true;gameLogic=nullptr;}
        if(guard==1){h->m_field248=false;gameLogic=nullptr;}
        if(guard==2){game=nullptr;gameLogic=nullptr;}
        if(guard==3)logic.status=2;
        check(run()&&std::memcmp(&input,&before,sizeof input)==0,"early guard leaves entire input snapshot unchanged");
        check(players.queries==0&&logic.reads==(guard==3?1u:0u),"guard order avoids unavailable managers");
    }
    reset();h->chat=true;h->m_field2fc=true;
    for(int i=0;i!=64;++i)input.inputs[i]=float(i+1);auto before=input;
    check(run(),"chat returns true");
    for(int i=0;i!=64;++i)check(input.inputs[i]==(i>=3&&i<=7?before.inputs[i]:0),"chat preserves selected movement axes only");
    check(!h->m_field2fc&&h->m_field300==4,"first chat tick captures throttle and clears capture latch");
    check(std::memcmp(reinterpret_cast<char*>(&input)+256,reinterpret_cast<char*>(&before)+256,sizeof input-256)==0,"chat preserves masks, state and native padding");
    check(trace==std::vector<std::string>{"status","chat","pointer","create"},"chat short-circuits first create query then reloads after clear");
    reset();h->create=true;h->m_field2fc=true;input.inputs[3]=0.75f;input.inputs[4]=0.5f;
    check(run()&&input.inputs[3]==0.75f&&input.inputs[4]==0,"create keeps throttle but drops the other axes");
    reset();h->chat=true;h->pointer=true;h->m_field2fc=true;input.inputs[3]=2;
    check(run()&&input.inputs[3]==0&&h->m_field300==2,"pointer suppresses restoration after capture");
    reset();h->chat=true;h->m_field2fc=true;input.mappingFlags&=~8ULL;input.inputs[3]=99;
    check(run()&&h->m_field300==0&&input.inputs[3]==0,"unmapped throttle captures positive zero");
    reset();h->chat=true;input.mappingFlags=0x10;std::uint32_t payload=0x7FC12345;
    std::memcpy(&input.inputs[4],&payload,4);h->onPointer=[&]{input.mappingFlags=0x28;};
    run();std::uint32_t actual=0;std::memcpy(&actual,&input.inputs[5],4);
    check(input.inputs[3]==7&&input.inputs[4]==0&&actual==0,"restoration observes callback-modified mapping mask");
    reset();h->chat=true;std::memcpy(&input.inputs[4],&payload,4);run();std::memcpy(&actual,&input.inputs[4],4);
    check(actual==payload,"chat raw copy preserves NaN payload");
    reset();h->chat=true;h->onPointer=[&]{h->create=true;};input.inputs[4]=3;run();
    check(input.inputs[4]==0,"create state is re-read after pointer callback");
    std::puts("Player filter mode masks and latch");
    for(int mode: {7,4,6,5,30,31,18,1}) {
        reset();h->mode=mode;input.mappingFlags=0x5555555555555555ULL;
        for(int i=0;i!=64;++i)input.inputs[i]=0.25f;
        const bool allow=mode!=18&&mode!=1;check(run()==allow,"mode category returns the retail boolean");
        for(int i=0;i!=64;++i){bool keep=i==0||i==3||i==13||i==38||(allow&&(i==9||i==37));
            check(input.inputs[i]==((keep||!(input.mappingFlags&(1ULL<<i)))?0.25f:0),"suppression respects exact allowlist and mapping bits");}
    }
    reset();h->modes={0,7,6};input.inputs[2]=0.25f;check(run()&&h->modeReads==3&&input.inputs[2]==0,"repeated mode callbacks select a later matching branch");
    reset();h->m_field328=63;input.inputs[63]=0.75f;run();check(input.inputs[63]==0&&h->m_field328==63,"active latch input is suppressed without resetting latch");
    for(float value:{0.5f,-1.0f,std::numeric_limits<float>::quiet_NaN()}) {reset();h->m_field328=63;input.inputs[63]=value;run();check(h->m_field328==64,"digital threshold or NaN releases latch");}
    reset();h->m_field328=-1;run();check(h->m_field328==64,"negative -1 shift produces zero without an array read");
    reset();h->commander=true;input.inputs[13]=std::numeric_limits<float>::quiet_NaN();run();check(h->m_field2b8==42,"commander NaN movement counts as nonzero");
    reset();h->commander=true;input.mappingFlags&=~0x2000ULL;input.inputs[13]=1;run();check(h->m_field2b8==-2,"unmapped commander movement leaves timer alone");
    std::puts("Player filter help callbacks and weapon confirmation");
    reset();players.local=&player;input.inputs[32]=input.inputs[3]=input.inputs[4]=input.inputs[9]=input.inputs[38]=input.inputs[37]=input.inputs[0]=1;h->m_field2f8=6;
    run();check(h->disabled==std::vector<int>{3,1,0,5,6,7,71,8}&&h->enabled==std::vector<int>{70},"help callbacks preserve complete retail ordering");
    check(h->m_field2f5&&h->m_field2f8==0,"held input triggers and resets help timer");
    reset();players.local=&player;input.inputs[3]=1;h->onHelp=[&](int id){if(id==-1)input.inputs[3]=0;};h->m_field2f8=9;run();
    check(!h->m_field2f5&&h->m_field2f8==0&&h->enabled.empty(),"help callback changes the repeated input3 read");
    reset();players.local=&player;input.inputs[3]=1;h->m_field2f8=std::numeric_limits<float>::quiet_NaN();run();check(h->enabled.empty(),"unordered help timer does not pass greater-than-five");
    for(int id:{-100,1,6,9,INT_MAX}) {reset();h->mode=10;input.inputs[8]=1;HudPlayerFilterFixture::setup(messages,0,1,id);check(run(),"selection confirmation returns true");
        int expected=id==INT_MAX?14:(id+13<14?14:(id+13>22?22:id+13));
        check(input.inputs[expected]==1&&input.inputs[8]==0,"confirm clamps the selected action after signed wrap");
        check(!HudPlayerFilterFixture::visible(messages)&&h->transitions==std::vector<std::pair<int,bool>>{{0,false}},"confirm hides real messages before mode change");
        check(HudPlayerFilterFixture::deadline(messages)==10.3,"confirm stores actual world-time cooldown");
    }
    reset();h->mode=10;input.inputs[8]=1;HudPlayerFilterFixture::cooldown(replacement,2.5);
    h->onTransition=[&]{h->m_field148=&replacement;WorldPref::resetWorldTime(20);};run();
    check(HudPlayerFilterFixture::deadline(messages)==0&&HudPlayerFilterFixture::deadline(replacement)==22.5,"confirm reloads owner and clock after mode callback");
    for(int blocker=0;blocker!=7;++blocker){reset();input.inputs[8]=1;
        if(blocker<5){FilterRadio* radios[]={&r0,&r1,&r2,&r3,&r4};radios[blocker]->enabled=false;}
        if(blocker==5)HudPlayerFilterFixture::deadline(messages,11);
        if(blocker==6)HudPlayerFilterFixture::deadline(messages,std::numeric_limits<double>::quiet_NaN());
        run();check(input.inputs[8]==(blocker==6?1:0),"cooldown/radio chain consumes blocked action and accepts unordered deadline");
        if(blocker<5)check(std::find(trace.begin(),trace.end(),"radio"+std::to_string(blocker))!=trace.end(),"radio blocking query is reached in order");
    }
    reset();input.inputs[8]=1;r0.callback=[&]{h->m_field10c=&r4;};run();
    auto a=std::find(trace.begin(),trace.end(),"radio0");check(a!=trace.end()&&a+1!=trace.end()&&*(a+1)=="radio4","radio pointer reload follows callback");
    std::puts("Player filter selection and map dependency boundaries");
    // An empty item list prevents the real wheel steppers from reaching sound selection.
    for(int key:{60,61}){reset();players.local=&player;input.inputs[key]=1;HudPlayerFilterFixture::setup(messages,0,0,1);
        check(!stopped()&&HudPlayerFilterFixture::duration(messages)==2&&h->mode==10,"wheel selection commits duration/mode, then steps the real item list");}
    reset();players.local=&player;input.inputs[14]=1;HudPlayerFilterFixture::setup(messages,0,6,2);run();
    check(h->mode==10&&HudPlayerFilterFixture::duration(messages)==0.4f,"direct slot opens timed selection when item differs");
    reset();players.local=&player;input.inputs[14]=1;run();check(h->transitions.empty(),"same direct item leaves mode unchanged");
    reset();players.local=&player;input.inputs[19]=1;HudPlayerFilterFixture::setup(messages,0,5,1);run();check(h->transitions.empty(),"direct item above count is ignored");
    reset();players.local=&player;input.inputs[14]=1;HudPlayerFilterFixture::setup(messages,0,0,1);run();
    check(h->transitions.empty()&&std::find(trace.begin(),trace.end(),"vehicle")!=trace.end(),"empty real refresh with no vehicle leaves selection closed");
    reset();players.local=&player;input.inputs[20]=input.inputs[21]=1;run();check(h->transitions.empty(),"selectable direct gate excludes slot20 and21");
    for(int key=14;key<=21;++key){reset();h->commander=true;input.inputs[key]=1;run();check(input.inputs[key]==(key==21?1:0),"commander clears first selected key with retail slot21 fallthrough");}
    for(int zoom:{0,1,2,-2,INT_MAX}){reset();h->mode=18;input.inputs[60]=1;adapter.zoom=zoom;
        // A different address is compared only; no fabricated map is dereferenced.
        h->hudMap=reinterpret_cast<HudMap*>(0x1234);run();
        check(adapter.reads==1&&system.owners==((zoom==-2||zoom==0||zoom==1)?1u:0u),"zoom clamp preserves negative/wrap and unchanged-level guard");}
    reset();h->mode=18;input.inputs[60]=1;adapter.callback=[&]{input.inputs[60]=0;input.inputs[61]=1;};
    check(stopped()&&system.owners==1,"zoom direction is re-read after adapter callback before actual map setter boundary");
    std::puts("Player filter real vehicle mouse-look and component precedence");
    reset();h->hudVehicleInformation=&vehicle;HudVehicleInformationFixture::setup(vehicle,8,3,1,1);run();
    check(vehicle.getGuiIndex()==8&&HudVehicleInformationFixture::input(vehicle)==0&&players.queries==2,"null local player leaves gui but still captures axis after second lookup");
    reset();players.local=&player;h->hudVehicleInformation=&vehicle;player.cameras={&c1,&c2};c2.look=true;input.inputs[11]=1;input.inputs[3]=0.25f;
    HudVehicleInformationFixture::setup(vehicle,8,3,1,1);run();
    check(vehicle.getGuiIndex()==0&&HudVehicleInformationFixture::saved(vehicle)==8&&c1.reads==0&&c2.reads==1,"mouse-look observes second camera and saves real gui index");
    check(HudVehicleInformationFixture::input(vehicle)==0.25f,"vehicle captures mapped axis3");
    for(float delta:{0.0f,0.01f,0.02f,std::numeric_limits<float>::quiet_NaN()}){reset();players.local=&player;player.camera=&c2;c2.look=true;h->hudVehicleInformation=&vehicle;
        HudVehicleInformationFixture::setup(vehicle,0,9,delta,0);run();check(vehicle.getGuiIndex()==((delta<=0.01f)?9:0),"mouse-look restores gui only within exact angle tolerance");}
    reset();players.local=&player;player.vehicle=object;object->parent(parent);parent->control=&control;control.weapon=weapon;weapon->flags(1);input.inputs[60]=1;
    check(run()&&h->transitions.empty(),"ancestor active weapon flag suppresses selection");
    reset();players.local=&player;player.vehicle=object;object->control=&control;control.weapon=weapon;weapon->flags(1);object->component(&component);component.info.useSelectionIcons=true;input.inputs[60]=1;
    HudPlayerFilterFixture::setup(messages,0,0,1);
    check(!stopped()&&h->mode==10&&HudPlayerFilterFixture::duration(messages)==2,"authored component flag overrides active weapon suppression");
    reset();players.local=&player;player.vehicle=object;object->component(&component);component.info.useSelectionIcons=true;current.demo=true;input.inputs[60]=1;
    check(run()&&h->transitions.empty(),"demo mode overrides component selection permission");
    reset();players.local=&player;player.vehicle=nullptr;current.demo=true;input.inputs[60]=1;
    HudPlayerFilterFixture::setup(messages,0,0,1);
    check(!stopped()&&h->mode==10&&HudPlayerFilterFixture::duration(messages)==2,"null vehicle preserves initial permission and skips demo check");
    // Retain actual Object/HUD lifetimes: constructor-owned members are cleaned
    // explicitly; no full HUD teardown or live world removal is claimed.
    delete h->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=decltype(h->m_field32c);
    h->m_field010.~W();h->m_field038.~W();h->m_field060.~W();h->m_field08c.~S();h->m_field0c0.~S();h->m_field304.~S();
    h->m_field0e8.~L();h->m_field2e0.~SL();h->m_field33c.~SL();h->m_field32c.~V();::operator delete(h);hudManager=nullptr;
    std::printf("HUD player filter: %u/%u checks passed\n",checks-failures,checks);
    return failures?1:0;
}
#endif
