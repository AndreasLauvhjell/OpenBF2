// [TEMP] Native Vehicle constructor/init and occupied-data proof with typed observers.
// [RETIRE-WHEN: HUD-VEHICLE-CONSTRUCT-FIXTURE] Replace observers with real HUD/UI
// initialization and full owner shutdown. No unresolved handler/update is called.
#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <list>
#include <map>
#include <new>
#include <set>
#include <string>
#include <vector>
#define private public
#define protected public
#include "../BF2/Menu/HudVehicleInformation.h"
#include "../BF2/Menu/HudBlinker.h"
#include "../BF2/Menu/BfOccupiedVehicleData.h"
#undef protected
#undef private
#pragma push_macro("main")
#undef main
#define main vehicle_regression_main
#include "hud_vehicle_information_runtime.cpp"
#pragma pop_macro("main")
#include "../BF2/Menu/Ref2System.h"
#include "../BF2/Game/HudInformationLayer.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
namespace dice { namespace hfe { IBase* g_hudInformationLayer=nullptr; } }
using namespace dice::meme;
using dice::hfe::menu::Ref2System;

static std::uint32_t rawWord(const void* p) {std::uint32_t v;std::memcpy(&v,p,4);return v;}
static bool poison(const void* p,std::size_t size) {
    const auto* bytes=static_cast<const unsigned char*>(p);
    for(std::size_t i=0;i<size;++i) if(bytes[i]!=0xa5)return false;
    return true;
}
struct ConstructionEvents : EventManager {
    ConstructionEvents* next=nullptr;
    std::vector<EventCategory> registrations;
    bool inspect=false;
    bool registerEventHandler(EventCategory category,IEventListener* listener,int priority) override {
        registrations.push_back(category);
        if(inspect) {
            auto* v=static_cast<HudVehicleInformation*>(listener);
            check(priority==0,"constructor listener priority zero");
            check(v->m_system==nullptr&&v->m_maxClimbSpeed==10,"scalar initialization before registration");
            check(v->m_warningSources.empty()||v->m_warningSources.count(17),"warning tree already constructed");
            check(poison(&v->m_blinkManager,sizeof(v->m_blinkManager)),"blink pointer untouched until after registrations");
            check(poison(v->m_occupantNames,sizeof(v->m_occupantNames)),"occupant names untouched at registration");
            check(poison(v->m_playerHealthColor,sizeof(v->m_playerHealthColor)),"health colors initialize after registrations");
            v->m_warningSources.insert(17);
        }
        if(next)eventManager=next;
        return false; // Real constructor discards registration result.
    }
    bool unregisterEventHandler(EventCategory,IEventListener*) override {return false;}
};
struct BindingRecord {char kind;std::string name;const void* value;};
static std::vector<BindingRecord> registered;
struct InformationCapture : HudInformationLayer {
    InformationCapture* next=nullptr;int calls=0;
    int record(char kind,const std::string& name,const void* value) {
        ++calls;registered.push_back({kind,name,value});
        if(next)g_hudInformationLayer=next;
        return -1;
    }
    int registerBool(const std::string& n,bool* p) override{return record('B',n,p);}
    int registerInt(const std::string& n,int* p) override{return record('I',n,p);}
    int registerFloat(const std::string& n,float* p) override{return record('F',n,p);}
    int registerString(const std::string& n,const char** p) override{return record('S',n,p);}
    int registerWideString(const std::string& n,const wchar_t** p) override{return record('W',n,p);}
};
struct RegistrySystem : System {
    RegistrySystem():System(800,600){m_registry=&root;}
    NameTreeRoot root;
};
struct TextureSystem : Ref2System {
    TextureSystem():Ref2System(800,600){}
    std::vector<std::string> paths;
    BF2HudManager* next=nullptr;
    int loadTexture(const char* path,bool menuPath) override {
        paths.emplace_back(path);check(menuPath,"occupied texture flag remains true");
        if(next)hudManager=next;
        return 100+int(paths.size());
    }
    void releaseTexture(int) override {check(false,"occupied destruction must not release textures");}
};
struct TextureHud : BF2HudManager {
    Ref2System* system=nullptr;int reads=0;
    Ref2System* getSystem() override {++reads;return system;}
};
static void destroyHudStorage(BF2HudManager* hud) {
    delete hud->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;
    using V=decltype(hud->m_field32c);
    hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();
    hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
    hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();
    ::operator delete(hud);
}
static std::size_t nativeOffset(std::size_t retail) {
    return retail>=0x264?retail-8:retail>=0x44?retail-4:retail;
}
static void checkBinding(std::size_t i,char kind,const char* name,std::size_t retail,HudVehicleInformation& v) {
    check(i<registered.size()&&registered[i].kind==kind&&registered[i].name==name
        &&registered[i].value==reinterpret_cast<char*>(&v)+nativeOffset(retail),"PE binding type/name/native address/order");
}
static int constructorChecks() {
    ConstructionEvents first,second;first.next=&second;first.inspect=second.inspect=true;eventManager=&first;
    alignas(HudVehicleInformation) unsigned char storage[sizeof(HudVehicleInformation)];
    std::memset(storage,0xa5,sizeof(storage));
    auto* v=new(storage)HudVehicleInformation;
    check(first.registrations==std::vector<EventCategory>{ECGame}
        &&second.registrations==std::vector<EventCategory>({ECGameLogic,ECHud}),"three registrations reload manager and retain order");
    check(v->m_blinkManager&&v->m_blinkManager->m_blinkers.empty(),"native blink manager constructed empty");
    check(v->m_warningSources.empty(),"post-registration clear removes callback inserted key");
    check(v->m_connectionMessage.empty()&&v->m_field024==1&&v->m_field0bc==-1,"string/unit/negative defaults");
    check(v->m_field178==-295&&v->m_field17c==-137&&v->m_field180==54
        &&v->m_field184==-295&&v->m_field188==-295,"all signed coordinate defaults");
    check(!v->m_playerHealthShow&&!v->m_killedState&&v->m_field2ac,"visibility and terminal byte defaults");
    check(poison(&v->m_occupiedData,4)&&poison(&v->m_unknown00c,4)&&poison(&v->m_field01c,4),"constructor leaves occupied pointer and unrelated words untouched");
    check(poison(v->m_occupantX,sizeof(v->m_occupantX))&&poison(v->m_occupantY,sizeof(v->m_occupantY)),"positions untouched");
    check(poison(v->m_playerSprintColor,sizeof(v->m_playerSprintColor))
        &&poison(v->m_vehicleSprintColor,sizeof(v->m_vehicleSprintColor))
        &&poison(v->m_nightVisionColor,sizeof(v->m_nightVisionColor)),"three non-health color arrays untouched");
    check(poison(&v->m_bottomLeftHealthFadedAlpha,8)&&poison(&v->m_unknown24d,1),"faded alphas and unused flag untouched");
    for(int i=0;i<10;++i) {
        check(v->m_occupantNames[i]==v->m_occupantNames[0]&&*v->m_occupantNames[i]==0&&!v->m_occupantShow[i]
            &&v->m_occupantColors[i][0]==0&&v->m_occupantColors[i][1]==0&&v->m_occupantColors[i][2]==0,"all occupant defaults");
    }
    for(int i=0;i<4;++i)check(v->m_playerHealthColor[i]==1&&v->m_vehicleArmorColor[i]==1,"two white color arrays");
    v->~HudVehicleInformation();
    return 0;
}
static void initChecks() {
    ConstructionEvents events;eventManager=&events;hudManager=nullptr;
    InformationCapture first,second;first.next=&second;g_hudInformationLayer=&first;
    RegistrySystem system;
    HudVehicleInformation v;v.setSystem(&system);registered.clear();first.calls=second.calls=0;
    HudVehicleInformationPrimary* primary=&v;
    check(primary->init(),"real vehicle init completes with native registry and occupied data");
    check(v.m_occupiedData&&std::strcmp(v.m_occupiedData->getName(),"OccupiedData")==0,"occupied object assigned and named");
    check(registered.size()==146&&first.calls==1&&second.calls==145,"all146 bindings ignore failures and reload global");
    struct Golden{char kind;const char* name;std::size_t retail;};
    // Generated from complete789480 instruction string/field/virtual-slot operands.
    const Golden golden[]={
        {'F',"BottomLeftHealthAlpha",0x18c},
        {'F',"BottomLeftVehicleAlpha",0x190},
        {'F',"BottomLeftHealthFadedAlpha",0x194},
        {'F',"BottomLeftVehicleFadedAlpha",0x198},
        {'B',"UsingParachute",0xb8},
        {'F',"PlayerStamina",0x1ac},
        {'F',"NightVisionGaugeValue",0x1b0},
        {'F',"VehicleStamina",0x1b4},
        {'F',"PlayerHealth",0x1a8},
        {'F',"VehicleArmor",0x1b8},
        {'S',"PlayerHealthString",0x270},
        {'S',"VehicleArmorString",0x26c},
        {'F',"VehicleAngle",0x1c8},
        {'F',"VehicleTurretAngle",0x1cc},
        {'F',"VehicleBanking",0x1d0},
        {'F',"VehicleElevation",0x1d4},
        {'F',"VehicleElevationSpeedAngle",0x1d8},
        {'F',"PlayerAngle",0x1bc},
        {'F',"PlayerBanking",0x1c0},
        {'F',"PlayerElevation",0x1c4},
        {'S',"AltitudeString",0x284},
        {'S',"SpeedString",0x280},
        {'S',"TorqueString",0x288},
        {'I',"AngleOfAttack",0x1f0},
        {'F',"Torque",0x1e4},
        {'F',"TorqueAngle",0x1ec},
        {'B',"VehiclePassengersShow",0x24e},
        {'B',"VehicleIconShow",0x24f},
        {'S',"VehicleIconPathString",0x27c},
        {'F',"GunnerAngle",0x1e0},
        {'B',"WarningIconShow",0x254},
        {'S',"WarningIconPath",0x268},
        {'B',"PlayerStaminaShow",0x245},
        {'B',"VehicleStaminaShow",0x249},
        {'B',"PlayerHealthShow",0x24b},
        {'B',"PlayerStaminaBlink",0x246},
        {'B',"NightVisionGaugeBlink",0x248},
        {'B',"VehicleStaminaBlink",0x24a},
        {'B',"PlayerHealthBlink",0x24c},
        {'B',"VehicleArmorShow",0x250},
        {'B',"VehicleArmorBlink",0x251},
        {'B',"TurretIconShow",0x252},
        {'B',"NightVisionGaugeShow",0x247},
        {'B',"ReferenceCrossShow",0x264},
        {'S',"LocalKitNameString",0x274},
        {'S',"VehicleNameString",0x278},
        {'F',"PlayerHealthColorRed",0x1f4},
        {'F',"PlayerHealthColorGreen",0x1f8},
        {'F',"PlayerHealthColorBlue",0x1fc},
        {'F',"PlayerHealthColorAlpha",0x200},
        {'F',"PlayerSprintColorRed",0x214},
        {'F',"PlayerSprintColorGreen",0x218},
        {'F',"PlayerSprintColorBlue",0x21c},
        {'F',"PlayerSprintColorAlpha",0x220},
        {'F',"NightVisionColorRed",0x234},
        {'F',"NightVisionColorGreen",0x238},
        {'F',"NightVisionColorBlue",0x23c},
        {'F',"NightVisionColorAlpha",0x240},
        {'F',"VehicleSprintColorRed",0x224},
        {'F',"VehicleSprintColorGreen",0x228},
        {'F',"VehicleSprintColorBlue",0x22c},
        {'F',"VehicleSprintColorAlpha",0x230},
        {'F',"VehicleArmorColorRed",0x204},
        {'F',"VehicleArmorColorGreen",0x208},
        {'F',"VehicleArmorColorBlue",0x20c},
        {'F',"VehicleArmorColorAlpha",0x210},
        {'B',"CloseToMedic",0x290},
        {'B',"CloseToRepair",0x291},
        {'B',"CloseToAmmo",0x292},
        {'B',"SquadInfoBarShow",0x295},
        {'B',"ShowCommanderIcon",0x296},
        {'B',"ShowSquadIcon",0x297},
        {'W',"SquadInfoText",0x298},
        {'S',"SquadInfoIconPath",0x29c},
        {'B',"KilledState",0x253},
        {'I',"GuiIndex",0xa0},
    };
    for(std::size_t i=0;i<76;++i)checkBinding(i,golden[i].kind,golden[i].name,golden[i].retail,v);
    const char* names[]={"OccupyingPlayerName%iShow","OccupyingPlayerName%iString","OccupyingPlayerPos%iX",
        "OccupyingPlayerPos%iY","OccupyingPlayer%iColorRed","OccupyingPlayer%iColorGreen","OccupyingPlayer%iColorBlue"};
    const char kinds[]={'B','S','I','I','F','F','F'};
    const std::size_t bases[]={0xec,0xc4,0x4c,0x74,0xf8,0xfc,0x100};
    const std::size_t steps[]={1,4,4,4,12,12,12};
    for(int i=0;i<10;++i)for(int j=0;j<7;++j) {
        char name[80];std::sprintf(name,names[j],i+1);
        checkBinding(76+i*7+j,kinds[j],name,bases[j]+i*steps[j],v);
    }
    const char* referencePaths[]={"BottomLeft/Alpha/BottomLeft_alpha1","BottomLeft/Alpha/BottomLeft_alpha2",
        "BottomLeft/Alpha/BottomLeft_nextAlpha1","BottomLeft/Alpha/BottomLeft_nextAlpha2",
        "BottomLeft/BottomLeft_nextXPos","BottomLeft/BottomLeft_XPos"};
    float* sources[]={&v.m_bottomLeftHealthAlpha,&v.m_bottomLeftVehicleAlpha,&v.m_field19c,&v.m_field1a0,&v.m_field188,&v.m_field184};
    for(int i=0;i<6;++i) {
        auto* data=static_cast<Data*>(system.root.findObject(&system,referencePaths[i]));
        check(data&&data->getType()==FloatRefData::getClassType(),"six actual float reference registrations");
        if(data){*sources[i]=float(10+i);check(data->getFloat(&system)==float(10+i),"registered reference retains member alias");}
    }
    check(v.m_blinkManager->m_blinkers.size()==5,"five blink controllers inserted");
    float* blinkSources[]={&v.m_playerHealth,&v.m_vehicleArmor,&v.m_playerStamina,&v.m_nightVisionGauge,&v.m_vehicleStamina};
    bool* blinkOutputs[]={&v.m_playerHealthBlink,&v.m_vehicleArmorBlink,&v.m_playerStaminaBlink,&v.m_nightVisionGaugeBlink,&v.m_vehicleStaminaBlink};
    for(int i=0;i<5;++i) {
        HudBlinker* b=v.m_blinkManager->m_blinkers[i];
        check(b&&b->m_source==blinkSources[i]&&b->m_output==blinkOutputs[i]&&b->m_maximum==.3f
            &&b->m_minimum==0&&b->m_period==.7f&&b->m_otherPeriod==.15f&&b->m_adaptive&&b->m_elapsed==0,"blink order, pointers and exact parameters");
    }
    check(system.root.close(&system),"reference registrations close through real registry traversal");
    check(FloatRefData::getClassType()->getInstanceCount()==0,"reference descriptor counts balance on cleanup");
    delete v.m_occupiedData; // Fixture cleanup: production Vehicle destructor deliberately borrows this data.
    v.m_occupiedData=nullptr;
}
static void occupiedChecks() {
    ConstructionEvents events;eventManager=&events;hudManager=nullptr;
    auto* descriptor=BfOccupiedVehicleData::getClassType();
    check(descriptor->getBaseType()==Data::getClassType()&&descriptor->getInstanceSize()==44
        &&std::strcmp(descriptor->getName(),"dice::meme::BfOccupiedVehicleData")==0,"complete occupied descriptor ancestry/name/native size");
    auto* created=static_cast<BfOccupiedVehicleData*>(descriptor->create());
    check(created&&created->getName()==nullptr&&created->getObjectSize()==44,"descriptor factory uses native constructor");
    check(created->streamProperties(nullptr,nullptr),"occupied stream delegates real true base");
    check(created->getOccupantTexture(nullptr,4)==0,"absent signed map key returns zero");
    created->setOccupantIcon(-9,-1);
    check(created->getOccupantTexture(nullptr,-9)==0,"negative icon clamps to zero");
    delete created;
    TextureSystem system;
    auto* first=new TextureHud;auto* second=new TextureHud;
    first->system=second->system=&system;system.next=second;hudManager=first;
    // Extra bytes belong to fixture allocation only, so the actual one-past store
    // can be observed without corrupting an unrelated heap allocation.
    alignas(BfOccupiedVehicleData) unsigned char storage[sizeof(BfOccupiedVehicleData)+8];
    std::memset(storage,0xa5,sizeof(storage));
    auto* data=new(storage)BfOccupiedVehicleData("Occupied");
    check(first->reads==2&&second->reads==7,"HUD getter repeats/reloads across texture callbacks");
    check(system.paths.size()==8&&system.paths.front()=="Ingame/GeneralIcons/empty.tga"
        &&system.paths.back()=="Ingame/Vehicles/Icons/Hud/Positions/vehiclePositionSquad.tga","eight texture paths in native constructor");
    check(rawWord(storage+sizeof(BfOccupiedVehicleData))==108
        &&rawWord(storage+sizeof(BfOccupiedVehicleData)+4)==0xa5a5a5a5,"retail one-word overflow preserved exactly");
    for(int i=0;i<8;++i) {data->setOccupantIcon(i,i);check(data->getOccupantTexture(nullptr,i)==101+i,"map selects all eight handles including out-of-range seventh index");}
    data->setOccupantIcon(3,-7);check(data->getOccupantTexture(nullptr,3)==101,"existing key updates and low clamp");
    data->setOccupantIcon(3,99);check(data->getOccupantTexture(nullptr,3)==108,"high clamp remains seven");
    data->~BfOccupiedVehicleData();
    hudManager=nullptr;destroyHudStorage(first);destroyHudStorage(second);
}
int main() {
    if(vehicle_regression_main())return 1;
    checks=failures=0;trace.clear();
    std::fprintf(stderr,"[TEMP] constructor checks\n");constructorChecks();
    std::fprintf(stderr,"[TEMP] init checks\n");initChecks();
    std::fprintf(stderr,"[TEMP] occupied checks\n");occupiedChecks();
    std::printf("Vehicle construction: %u/%u checks passed\n",checks-failures,checks);
    return failures?1:0;
}
