// [TEMP][CHOSEN, not recovered] Real Ammo constructor/event/hidden-update tests.
// [RETIRE-WHEN: HUD-AMMO-RUNTIME-FIXTURE] Replace typed external observers with
// complete game/UI collaborators. No weapon/ability frontier is overridden.
#include <vector>
#define private public
#define protected public
#include "../BF2/Menu/HudBlinker.h"
#undef protected
#undef private
#define main ammoRegressionMain
#include "hud_ammo_runtime.cpp"
#undef main
#include "../BF2/External/BFMeme/Pointer.h"
#include "../BF2/Game/Objects/Object.h"
#include "../BF2/Game/Objects/IVehicleHudComp.h"
#include "../BF2/Game/Objects/IAbilityHudComp.h"
#include "../BF2/Game/IGame.h"
#include <limits>
namespace dice { namespace hfe { namespace world {
class ObjectManager; ObjectManager* objectManager=nullptr;
} } }
struct AmmoObject : world::Object {
    AmmoObject():Object(nullptr) {}
    void vehicleHud(world::IVehicleHudComp* p){m_vehicleHudComponent=p;}
    void abilityHud(world::IAbilityHudComp* p){m_abilityHudComponent=p;}
    void warningHud(world::IObjectComponent* p){m_warningHudComponent=p;}
};
#define COMPONENT_OBSERVER \
void addRef() override {fixtureUnsupported("component addRef");} \
int getRef() override {fixtureUnsupported("component getRef");} \
int release() override {fixtureUnsupported("component release");} \
void* queryInterface(unsigned) const override {fixtureUnsupported("component query");} \
void setObject(world::IObject*) override {fixtureUnsupported("component setObject");} \
world::IObject* getObject() const override {fixtureUnsupported("component getObject");}
struct AmmoVehicleHud : world::IVehicleHudComp {
    COMPONENT_OBSERVER
    world::VehicleHudInfo info{};
    int getGuiIndex(bool) override {fixtureUnsupported("gui");}
    world::IPostProcessComp* getEnablePostProcessing(int) override {fixtureUnsupported("post");}
    std::string getVehicleIcon(int) override {fixtureUnsupported("icon");}
    int getVehicleIconPtr(int) override {fixtureUnsupported("iconptr");}
    std::string getAbilityIcon(int) override {fixtureUnsupported("abilityIcon");}
    int getAbilityIconPtr(int) override {fixtureUnsupported("abilityIconPtr");}
    bool getShowOnCamMode(int) override {fixtureUnsupported("cam");}
    void playingPantingSound(bool) override {fixtureUnsupported("pant");}
    bool getIsPlayingPantingSound() override {fixtureUnsupported("panting");}
    void playInjurySound() override {fixtureUnsupported("injury");}
    world::VehicleHudIcons* getHudIcons() override {fixtureUnsupported("icons");}
    world::VehicleHudInfo* getHudInfo() override {trace+='v';return &info;}
};
struct AmmoAbility : world::IAbilityHudComp {
    COMPONENT_OBSERVER
    int stops=0;
    world::AbilityHudInfo* getHudInfo() override {fixtureUnsupported("abilityInfo");}
    void setHealingActive(bool) override {fixtureUnsupported("heal");}
    void setRepairingActive(bool) override {fixtureUnsupported("repair");}
    void setAmmoActive(bool) override {fixtureUnsupported("ammo");}
    void setGetHealActive(bool) override {fixtureUnsupported("getHeal");}
    void setGetRepairActive(bool) override {fixtureUnsupported("getRepair");}
    void setGetAmmoActive(bool) override {fixtureUnsupported("getAmmo");}
    bool isPlayingHealSound() const override {fixtureUnsupported("healSound");}
    bool isPlayingRepairSound() const override {fixtureUnsupported("repairSound");}
    bool isPlayingAmmoSound() const override {fixtureUnsupported("ammoSound");}
    void forceStopAllSound() override {trace+='s';++stops;}
};
#undef COMPONENT_OBSERVER
struct AmmoPlayer : Player {
    int id=42;
    world::IObject* vehicle=nullptr;
    world::IObject* defaultVehicle=nullptr;
    bool freeCamera=false,commander=false;
    float fade=0;
    int getId() const override {trace+='i';return id;}
    world::IObject* getVehicle() const override {trace+='V';return vehicle;}
    world::IObject* getDefaultVehicle() override {trace+='D';return defaultVehicle;}
    bool getFreeCameraEnabled() const override {trace+='f';return freeCamera;}
    bool getIsCommander() const override {trace+='c';return commander;}
    float getCrossHairFadeValue() const override {trace+='x';return fade;}
};
struct AmmoPlayers : Players {
    world::IPlayer* found=nullptr; int lookedUp=0;
    world::IPlayer* getPlayerFromId(int id) override {trace+='I';lookedUp=id;return found;}
};
struct AmmoAdapter : HudInformationAdapter {
    bool value=false;
    bool setBool(std::string key,bool v) override {
        trace+='b';check(key=="CloseToMine","mine event exact information key");value=v;return false;
    }
};
struct AmmoHud : Hud {
    std::function<void()> afterMode;
    bool paint=true; int texture=0, pointerMode=0; world::IPlayer* kitPlayer=nullptr;
    world::Kit* getPlayerKit(world::IPlayer* p) override {trace+='k';kitPlayer=p;return nullptr;}
    bool getPaint() override {trace+='p';return paint;}
    int getMode() override {trace+='m';const int result=mode;if(afterMode)afterMode();return result;}
    void enablePointer(int t,int m) override {trace+='E';texture=t;pointerMode=m;}
};
struct AmmoPointer : Pointer {
    System* receiver=nullptr; float x=0,y=0;
    bool setPosition(System* s,float a,float b) override {trace+='z';receiver=s;x=a;y=b;return false;}
};
struct AmmoSystem : System {
    std::function<void()> afterTexture;
    AmmoSystem():System(800,600){}
    int loadTexture(const char* path,bool menu) override {
        trace+='t';check(menu&&std::strcmp(path,"pointer")==0,"pointer texture path and menu flag");
        if(afterTexture)afterTexture();return -19;
    }
};
namespace dice { namespace hfe { namespace hud {
struct HudAmmoConstructionFixture {
struct Golden {unsigned offset,width,kind,value;};
static void initialBytes(HudAmmo* ammo,System* system) {
    // Mechanically transcribed PE MOV stores up to the first event registration.
    // Source evidence ctor_initial_stores.json; unspecified bytes remain poison.
    static const Golden golden[]={
    {0x8,4,1,0x0u},
    {0x10,4,0,0x3f800000u},
    {0x14,4,0,0x3f800000u},
    {0x24,4,0,0x43250000u},
    {0x28,4,0,0x43250000u},
    {0x18,1,0,0x0u},
    {0x1c,4,0,0x43fb8000u},
    {0x20,4,0,0x43a88000u},
    {0x2c,4,0,0x43fb8000u},
    {0x90,4,0,0x40400000u},
    {0x94,4,0,0x41200000u},
    {0x11c,4,0,0x12cu},
    {0x120,4,0,0x12cu},
    {0x98,4,0,0xffffffffu},
    {0x9c,4,0,0x0u},
    {0xa0,4,0,0x20u},
    {0xa4,1,0,0x0u},
    {0xac,1,0,0x1u},
    {0xad,1,0,0x1u},
    {0xae,1,0,0x1u},
    {0xaf,1,0,0x1u},
    {0xb0,1,0,0x0u},
    {0xb4,4,0,0x0u},
    {0xb8,4,0,0x0u},
    {0xbc,1,0,0x0u},
    {0xbd,1,0,0x0u},
    {0xbe,1,0,0x0u},
    {0xbf,1,0,0x0u},
    {0xc0,1,0,0x0u},
    {0xc1,1,0,0x0u},
    {0xc4,4,0,0x0u},
    {0xc8,4,0,0x0u},
    {0xcc,1,0,0x0u},
    {0xcd,1,0,0x0u},
    {0xce,1,0,0x0u},
    {0xcf,1,0,0x0u},
    {0xd0,4,0,0x0u},
    {0xe4,4,0,0x0u},
    {0xe8,1,0,0x0u},
    {0xe9,1,0,0x0u},
    {0xea,1,0,0x0u},
    {0xeb,1,0,0x0u},
    {0xec,1,0,0x0u},
    {0xed,1,0,0x0u},
    {0xee,1,0,0x0u},
    {0xf0,4,0,0x0u},
    {0xf4,4,0,0x0u},
    {0xf8,4,0,0x41000000u},
    {0xfc,4,0,0x0u},
    {0x100,4,0,0xau},
    {0x104,4,0,0x3e8u},
    {0x108,4,0,0x0u},
    {0x10c,4,0,0x0u},
    {0x110,4,0,0x0u},
    {0x114,4,0,0x0u},
    {0x118,4,0,0x0u},
    {0x124,4,0,0x190u},
    {0x128,4,0,0x190u},
    {0x12c,1,0,0x0u},
    {0x12d,1,0,0x0u},
    {0x12e,1,0,0x0u},
    {0x12f,1,0,0x0u},
    {0x130,1,0,0x0u},
    {0x131,1,0,0x0u},
    {0x132,1,0,0x0u},
    {0x133,1,0,0x0u},
    {0x134,1,0,0x0u},
    {0x135,1,0,0x0u},
    {0x136,1,0,0x0u},
    {0x137,1,0,0x0u},
    {0x138,1,0,0x0u},
    {0x139,1,0,0x0u},
    {0x13a,1,0,0x0u},
    {0x13b,1,0,0x0u},
    {0x13c,1,0,0x0u},
    {0x13d,1,0,0x0u},
    {0x13e,1,0,0x0u},
    {0x13f,1,0,0x0u},
    {0x140,4,0,0x0u},
    {0x144,4,2,0x0u},
    {0x148,4,2,0x0u},
    {0x14c,4,2,0x0u},
    {0x150,4,2,0x0u},
    {0x154,4,2,0x0u},
    {0x158,4,2,0x0u},
    {0x15c,4,0,0x0u},
    {0x160,4,2,0x0u},
    {0x164,4,2,0x0u},
    {0x168,4,2,0x0u},
    {0x16c,4,2,0x0u},
    {0x170,4,2,0x0u},
    {0x174,4,2,0x0u},
    {0x178,4,0,0x0u},
    {0x17c,4,2,0x0u},
    {0x180,4,2,0x0u},
    {0x18c,1,0,0x0u},
    {0x190,4,3,0x0u},
    {0x194,4,2,0x0u},
    {0x198,4,2,0x0u},
    {0x19c,4,2,0x0u},
    {0x1a0,4,2,0x0u},
    {0x1a4,4,2,0x0u},
    {0x1a8,4,0,0xbf800000u},
    {0x1ac,4,0,0x0u},
    {0x1b0,1,0,0x0u},
    {0x1b1,1,0,0x0u},
    {0x1b4,4,2,0x0u},
    {0x1b8,4,0,0x0u},
    {0x1bc,1,0,0x0u},
    {0x1bd,1,0,0x0u},
    {0x1be,1,0,0x0u},
    {0x1bf,1,0,0x0u},
    {0x1c0,1,0,0x0u},
    {0x1c1,1,0,0x0u},
    {0x1c2,1,0,0x0u},
    };
    unsigned char expected[sizeof(HudAmmo)];std::memset(expected,0xa5,sizeof expected);
    for(const auto& g:golden) {
        unsigned value=g.kind==1?reinterpret_cast<unsigned>(system)
            :g.kind==2?reinterpret_cast<unsigned>(&HudAmmo::s_emptyString)
            :g.kind==3?reinterpret_cast<unsigned>(&HudAmmo::s_emptyWideString):g.value;
        std::memcpy(expected+g.offset,&value,g.width);
    }
    for(unsigned i=8;i<sizeof(HudAmmo);++i)
        check(reinterpret_cast<unsigned char*>(ammo)[i]==expected[i],"exact constructor pre-subscription byte/untouched padding");
}
struct Subscriptions : EventManager {
    std::function<void(EventCategory,IEventListener*)> observer;
    std::vector<int> added,removed;
    bool registerEventHandler(EventCategory c,IEventListener* l,int priority) override {
        added.push_back(c);check(priority==0,"constructor handler priority is zero");
        if(observer)observer(c,l);
        return EventManager::registerEventHandler(c,l,priority);
    }
    bool unregisterEventHandler(EventCategory c,IEventListener* l) override {
        removed.push_back(c);return EventManager::unregisterEventHandler(c,l);
    }
};
static void run() {
    Subscriptions events;eventManager=&events;
    AmmoSystem system,replacementSystem;
    AmmoPlayers players;world::playerManager=&players;
    AmmoPlayer local,other;players.local=&local;players.found=&local;
    AmmoHud* hud=new AmmoHud;AmmoHud* replacementHud=new AmmoHud;
    AmmoAdapter adapter;hud->adapter=&adapter;hudManager=hud;
    // Isolate Ammo delivery; this observer adapter is used only for setBool.
    events.unregisterEventHandler(ECGame,&adapter);
    events.unregisterEventHandler(ECHud,&adapter);
    events.added.clear();events.removed.clear();
    void* storage=::operator new(sizeof(HudAmmo));std::memset(storage,0xa5,sizeof(HudAmmo));
    events.observer=[&](EventCategory c,IEventListener* listener) {
        check(c==(events.added.size()==1?ECGame:ECHud),"constructor ECGame then ECHud");
        check(listener==reinterpret_cast<IEventListener*>(static_cast<char*>(storage)+4),"constructor secondary listener+4");
        initialBytes(static_cast<HudAmmo*>(listener),&system);
    };
    HudAmmo* ammo=new(storage) HudAmmo(&system);events.observer=nullptr;
    check(events.added==std::vector<int>{ECGame,ECHud},"actual native registrations completed");
    check(ammo->m_blinkManager->m_blinkers.size()==7,"seven owned native blink controllers");
    const unsigned sourceOffsets[]={0xb4,0xf4,0xc4,0x140,0x15c,0xf0,0};
    const unsigned outputOffsets[]={0x12e,0x12f,0x137,0x135,0x13f,0x138,0x12c};
    const float maximums[]={.3f,.7f,.3f,1.5f,.7f,.7f,0};
    const float periods[]={.7f,.3f,.7f,.3f,.3f,.3f,.7f};
    for(unsigned i=0;i<7;++i) {
        auto* b=ammo->m_blinkManager->m_blinkers[i];
        check(b->m_source==(sourceOffsets[i]?reinterpret_cast<float*>(static_cast<char*>(storage)+sourceOffsets[i]):nullptr)
            &&b->m_output==reinterpret_cast<bool*>(static_cast<char*>(storage)+outputOffsets[i]),"blink insertion order and actual bound members");
        check(b->m_maximum==maximums[i]&&b->m_minimum==((i==1||i==5)?1.0f:0.0f)
            &&b->m_period==periods[i]&&b->m_otherPeriod==(i==6?.7f:.15f)
            &&b->m_adaptive==(i!=6)&&b->m_elapsed==0,"blink constructor thresholds and timing");
    }
    for(unsigned i=0x30;i<0x90;i+=4)check(*reinterpret_cast<float*>(static_cast<char*>(storage)+i)==1,"late RGBA initialization");
    for(unsigned i=0xd4;i<=0xe0;i+=4)check(*reinterpret_cast<float*>(static_cast<char*>(storage)+i)==1,"late laser RGBA initialization");
    ammo->m_field0b4=.2f;ammo->m_field0f4=.8f;ammo->m_field0c4=.2f;
    ammo->m_field140=1;ammo->m_field15c=.5f;ammo->m_field0f0=.8f;
    ammo->m_blinkManager->update(1.0f);
    check(ammo->m_field12e&&ammo->m_field12f&&ammo->m_field137&&ammo->m_field135
        &&ammo->m_field13f&&ammo->m_field138&&ammo->m_field12c,"all seven actual controller updates reach bound flags");
    for(int mode=-1;mode<=32;++mode) {
        ammo->m_field0e9=false;ammo->m_field02c=1;ammo->m_field018=true;ammo->m_field13a=true;
        local.alive=true;hud->enabled=false;trace.clear();ammo->setHudMode(mode);
        const bool enabled=mode==-1||mode==0||mode==2||mode==6||mode==9||mode==10||mode==13||mode==14||mode==25||mode>=28;
        check(ammo->m_field0e9==enabled,"mode table active flag including signed out-of-range default");
        check((mode==15)==(ammo->m_field02c==503&&!ammo->m_field018&&!ammo->m_field13a),"only mode15 resets bottom-right state");
        check(trace==(mode==0?"PaB":"P"),"every mode queries player; only alive mode0 queries pointer");
    }
    ammo->m_field0e9=true;local.alive=false;trace.clear();ammo->setHudMode(0);
    check(ammo->m_field0e9&&trace=="Pa","dead mode0 leaves active flag unchanged");
    players.local=nullptr;trace.clear();ammo->setHudMode(0);
    check(ammo->m_field0e9&&trace=="P","null mode0 leaves active flag unchanged");
    players.local=&local;local.alive=true;
    trace.clear();events.postEvent(ECGame,6,nullptr,0);
    check(trace=="P","actual event subscription dispatches ignored game id after local query");
    trace.clear();ammo->handleEvent(ECNetwork,8,nullptr,nullptr);
    check(trace.empty(),"unhandled category does not query player");
    int id=42;trace.clear();events.postEvent(ECGame,8,&id,0);
    check(trace=="PIiifpck","death event ID comparison, free-camera, paint, commander, kit order");
    check(hud->kitPlayer==&local&&players.lookedUp==42,"death lookup payload and captured local receiver");
    players.found=nullptr;trace.clear();ammo->handleEvent(ECGame,8,nullptr,&id);
    check(trace=="PI","missing death player short circuits");
    players.found=&other;other.id=43;trace.clear();ammo->handleEvent(ECGame,8,nullptr,&id);
    check(trace=="PIii","different death ID short circuits");
    other.id=42;local.freeCamera=true;trace.clear();ammo->handleEvent(ECGame,8,nullptr,&id);
    check(trace=="PIiifk","matching IDs are accepted even for different player identities; free camera skips hide");
    local.freeCamera=false;hud->paint=false;trace.clear();ammo->handleEvent(ECGame,8,nullptr,&id);
    check(trace=="PIiifpk","false paint skips commander and hide");
    hud->paint=true;players.found=&local;
    for(unsigned eventId:{57u,58u}) {
        trace.clear();events.postEvent(ECHud,eventId,&id,0);
        check(trace=="PIAb"&&adapter.value==(eventId==57),"mine events preserve identity guard and false adapter result");
    }
    players.found=&other;trace.clear();ammo->handleEvent(ECHud,57,nullptr,&id);
    check(trace=="PI","mine events require identity despite matching numeric IDs");
    players.local=nullptr;trace.clear();ammo->handleEvent(ECHud,57,nullptr,&id);
    check(trace=="PI","mine event performs lookup before null-local guard");
    players.local=&local;players.found=&local;
    AmmoObject* object=new AmmoObject; // Retained world lifetime; Ammo never owns it.
    AmmoAbility ability;AmmoVehicleHud vehicleHud;
    object->warningHud(&ability);local.vehicle=object;
    world::MissileLockMessage message={0x0203, -77,0,0};
    trace.clear();events.postEvent(ECHud,44,&message,0);
    check(trace=="PVV"&&ammo->m_field098==-77,"missile event repeats vehicle query and accepts noncanonical nonzero flag bytes");
    for(unsigned flags:{0x0003u,0x0200u,0x030000u}) {
        message.field00=flags;ammo->m_field098=81;trace.clear();ammo->handleEvent(ECHud,44,nullptr,&message);
        check(trace=="PVV"&&ammo->m_field098==81,"both exact low message bytes are required");
    }
    trace.clear();ammo->handleEvent(ECHud,44,nullptr,nullptr);
    check(trace=="P","null missile payload avoids vehicle calls");
    object->warningHud(nullptr);message.field00=0x0101;ammo->handleEvent(ECHud,44,nullptr,&message);
    check(ammo->m_field098==81,"missing warning component leaves target unchanged");
    object->abilityHud(&ability);trace.clear();ammo->stopAbilitySounds(object);
    check(trace=="s"&&ability.stops==1,"actual ability interface final slot stops sound");
    object->abilityHud(nullptr);trace.clear();ammo->stopAbilitySounds(object);ammo->stopAbilitySounds(nullptr);
    check(trace.empty(),"ability stop null guards");
    object->vehicleHud(&vehicleHud);vehicleHud.info.showCrossHair=true;ammo->m_field0e9=true;
    for(float value:{0.0f,-0.0f,1.0f,-1.0f,std::numeric_limits<float>::quiet_NaN()}) {
        local.fade=value;trace.clear();ammo->updateCrosshair(object);
        check(trace=="vPx"&&ammo->m_field0e8&&ammo->m_field0eb==(value!=0),"crosshair actual typed callback and zero/unordered parity");
    }
    players.local=nullptr;ammo->m_field0fc=-2;trace.clear();ammo->updateCrosshair(object);
    check(trace=="vP"&&ammo->m_field0eb,"missing player preserves prior fade");
    ammo->m_field0e9=false;trace.clear();ammo->updateCrosshair(object);
    check(trace.empty()&&!ammo->m_field0e8&&ammo->m_field0eb,"inactive crosshair clears icon only");
    ammo->m_field0e9=true;vehicleHud.info.showCrossHair=false;trace.clear();ammo->updateCrosshair(object);
    check(trace=="v"&&!ammo->m_field0e8&&ammo->m_field0eb,"hidden authored crosshair leaves stale hit indicator");
    object->vehicleHud(nullptr);ammo->m_field0e8=true;trace.clear();ammo->updateCrosshair(object);ammo->updateCrosshair(nullptr);
    check(trace.empty()&&ammo->m_field0e8,"null crosshair dependencies leave icon state");
    for(int branch=0;branch<5;++branch) {
        players.local=branch==0?nullptr:&local;local.vehicle=branch==1?nullptr:object;
        local.defaultVehicle=branch==2?nullptr:object;local.alive=branch!=3;hud->commander=branch==4;
        ammo->m_field0a4=false;ammo->m_field0e9=true;ammo->m_field0b4=5;ammo->m_field0c4=6;
        ammo->m_field0e4=9;ammo->m_field1b0=true;ammo->m_field1b1=true;ammo->m_field184=true;
        ammo->m_field0ec=true;trace.clear();static_cast<IHudAmmo*>(ammo)->update(.25f);
        const char* expected[]={"PkC","PVkC","PVDkC","PVDakC","PVDaCkC"};
        check(trace==expected[branch],"hidden update guard callbacks and commander requery");
        check(ammo->m_field0b4==0&&ammo->m_field0c4==0&&ammo->m_field0e4==0&&!ammo->m_field1b0,
            "hidden update clears precise ammo/scalar fields");
        check(ammo->m_field1b1&&ammo->m_field184&&ammo->m_field0ec&&ammo->m_field0e9==(branch!=4),
            "hidden update preserves omitted flags and conditionally changes pointer active");
    }
    AmmoPointer pointer;pointer.setEnabled(true);system.getPointerObject()=&pointer;
    ammo->m_system=&system;ammo->m_field1a0="pointer";hud->enabled=false;hud->mode=17;
    trace.clear();ammo->updatePointer(true);
    check(trace=="BtmEz"&&hud->texture==-19&&hud->pointerMode==17
        &&pointer.receiver==&system&&pointer.x==3&&pointer.y==10,"pointer texture/mode/enable and actual position wrapper");
    hud->enabled=true;trace.clear();ammo->updatePointer(true);
    check(trace=="B","matching pointer state does not reload texture");
    trace.clear();ammo->updatePointer(false);check(trace=="BC","pointer disable branch");
    hud->enabled=false;replacementHud->mode=9;
    hud->afterMode=[&]{hudManager=replacementHud;ammo->m_system=&replacementSystem;replacementSystem.getPointerObject()=&pointer;};
    trace.clear();ammo->updatePointer(true);
    check(trace=="BtmEz"&&replacementHud->pointerMode==17&&pointer.receiver==&replacementSystem,
        "pointer mode callback changes live HUD and System before subsequent calls");
    hud->afterMode=nullptr;hudManager=hud;
    ammo->m_field1a0=&HudAmmo::s_emptyString;
    events.removed.clear();delete ammo;
    check(events.removed==std::vector<int>{ECGame,ECHud},"actual owner destruction unregisters both native listeners");
    object->warningHud(nullptr);object->abilityHud(nullptr);object->vehicleHud(nullptr);
    // Retained HUD/world owners are fixture-only; all Ammo and pointer lifetimes are real.
    world::playerManager=nullptr;hudManager=nullptr;
}
};
} } }
int main() {
    const int regression=ammoRegressionMain();destructionActive=false;
    HudAmmoConstructionFixture::run();
    std::printf("HUD Ammo construction/events: %d/%d checks passed\n",checks-failures,checks);
    return regression||failures?1:0;
}
