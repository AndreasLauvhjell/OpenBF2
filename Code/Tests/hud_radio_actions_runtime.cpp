// [TEMP] Actual radio opener bodies. Typed observers model ray-cast results;
// squad-prefix tests stop by throwing from the real inverse helper's first
// settings callback, before an unavailable native map would be required.
#define main cameraRegressionMain
#include "hud_camera_runtime.cpp"
#undef main
#include "../BF2/Menu/HudRadio.h"
#include "../BF2/Game/Objects/IVehicleHudComp.h"
#include "../BF2/Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "../BF2/IO/Network/INetworkable.h"
#include "../BF2/Core/SettingsRepostitory.h"
#include "hud_radio_action_observers.h"
namespace dice { namespace hfe { SettingsRepostitory* settings=nullptr; } }
struct RadioPlayer : Player {
    world::IObject* vehicle=nullptr,*defaultVehicle=nullptr;
    std::function<void()> onVehicle;
    mutable int vehicleReads=0;
    mutable std::size_t teamRead=0;
    std::vector<int> teamScript;
    int getTeam() const override {trace+='T';return teamRead<teamScript.size()?teamScript[teamRead++]:team;}
    float getViewDistance() const override {trace+='F';return 333.0f;}
    world::IObject* getVehicle() const override {trace+='V';++vehicleReads;if(onVehicle)onVehicle();return vehicle;}
    world::IObject* getDefaultVehicle() override {trace+='D';return defaultVehicle;}
};
struct RadioObject : CameraObject {
    world::IPlayerControlObject* pco=nullptr;
    io::INetworkable* net=nullptr;
    char tag='Q';
    void* queryInterface(unsigned iid) const override {
        trace+=tag;check(iid==world::IID_IPlayerControlObject,"parent walk asks actual IPCO identity");return pco;
    }
    io::INetworkable* getNetworkable() const override {trace+='N';return net;}
    void parent(world::IObject* value){m_parent=value;}
    void component(world::IObjectComponent* value){m_vehicleHudComponent=value;}
};
struct RadioPco : world::RadioPcoBoundary {
    std::map<int,world::IPlayerControlObject*> entries;
    world::IPlayer* occupant=nullptr;
    char tag='O';
    std::function<void()> onOccupant;
    std::map<int,world::IPlayerControlObject*>* getPcos() override {trace+='C';return &entries;}
    world::IPlayer* getOccupingPlayer() const override {trace+=tag;if(onOccupant)onOccupant();return occupant;}
};
struct VehicleComponent : world::IVehicleHudComp {
    world::VehicleHudInfo info{};
    std::function<void()> onInfo;
    world::VehicleHudInfo* getHudInfo() override {trace+='I';if(onInfo)onInfo();return &info;}
    void addRef() override {actionUnsupported("vehicle addRef");}
    int getRef() override {actionUnsupported("vehicle getRef");}
    int release() override {actionUnsupported("vehicle release");}
    void* queryInterface(unsigned) const override {actionUnsupported("vehicle queryInterface");}
    void setObject(world::IObject*) override {actionUnsupported("vehicle setObject");}
    world::IObject* getObject() const override {actionUnsupported("vehicle getObject");}
    int getGuiIndex(bool) override {actionUnsupported("vehicle gui");}
    world::IPostProcessComp* getEnablePostProcessing(int) override {actionUnsupported("vehicle postprocessing");}
    std::string getVehicleIcon(int) override {actionUnsupported("vehicle icon");}
    int getVehicleIconPtr(int) override {actionUnsupported("vehicle icon ptr");}
    std::string getAbilityIcon(int) override {actionUnsupported("vehicle ability icon");}
    int getAbilityIconPtr(int) override {actionUnsupported("vehicle ability ptr");}
    bool getShowOnCamMode(int) override {actionUnsupported("vehicle camera mode");}
    void playingPantingSound(bool) override {actionUnsupported("vehicle pant");}
    bool getIsPlayingPantingSound() override {actionUnsupported("vehicle pant state");}
    void playInjurySound() override {actionUnsupported("vehicle injury");}
    world::VehicleHudIcons* getHudIcons() override {actionUnsupported("vehicle icons");}
};
struct RadioOwner : HudRadio {
    char enabledTag='E',activeTag='A';
    bool isEnabled() override {trace+=enabledTag;return HudRadio::isEnabled();}
    bool isActive() override {trace+=activeTag;return HudRadio::isActive();}
};
struct RadioHud : BF2HudManager {
    bool hit=false;
    world::IObject* target=nullptr;
    float distance=0;
    bool vehiclePosition=false;
    Vec3 point{10,20,30};
    std::function<void()> onRay;
    std::vector<int> modes,help;
    bool findSpottedTarget(world::IObject*& object,Vec3& position,float limit,bool vehicle) override {
        trace+='R';distance=limit;vehiclePosition=vehicle;object=target;position=point;
        if(onRay)onRay();return hit;
    }
    void setMode(int value,bool save) override {trace+='M';check(save,"radio modes save previous");modes.push_back(value);}
    void enableVOHelpMessage(int id) override {trace+='H';help.push_back(id);}
    void updateOrderPositionColor(const Vec3& position) override {
        trace+='U';check(&position==&m_field1e8&&m_order.orderId==4,"color callback sees stored default order and actual spotted position");
    }
};
struct RadioLogic : Logic {
    std::list<std::pair<std::string,bool>> invalid;
    std::list<std::pair<std::string,bool>>* GetInvalidVehicleDropObjectList() override {trace+='L';return &invalid;}
};
struct RadioTemplate : CameraTemplate {
    std::string name="Target";
    mutable int reads=0;
    const std::string& getName() const override {trace+='G';++reads;return name;}
};
struct RadioNetworkable : io::INetworkable {
    void addRef() override {actionUnsupported("network addRef");}
    int getRef() override {actionUnsupported("network getRef");}
    int release() override {actionUnsupported("network release");}
    void* queryInterface(unsigned) const override {actionUnsupported("network queryInterface");}
    bool init(void*) override {actionUnsupported("network init");}
    unsigned getGhostStateMask() const override {actionUnsupported("network ghost mask");}
    void updateStateMask(unsigned*) override {actionUnsupported("network state mask");}
    bool getNetUpdate(io::BitStream&,io::BaseLineData*,unsigned,int) override {actionUnsupported("network update");}
    void setNetUpdate(io::BitStream&,float,bool,io::BaseLineData*,int) override {actionUnsupported("network set update");}
    bool getBaseLine(io::BaseLineData&) override {actionUnsupported("network baseline");}
};
struct MapBoundaryReached {};
struct RadioSettings : SettingsRepostitory {
    const int& intGet(const std::string& key,const int& fallback) override {
        trace+='X';check(key=="GLSWorldSizeX"&&fallback==0,"squad prefix reaches real inverse helper before map storage");
        throw MapBoundaryReached{};
    }
};
static void squadPrefix(RadioHud* hud) {
    try {hud->openSquadRadio(false);check(false,"expected map boundary callback");}
    catch(const MapBoundaryReached&){}
}
int main() {
    if(cameraRegressionMain())return 1;
    checks=failures=0;
    Players players;RadioPlayer local,enemy,friendly;
    world::playerManager=&players;enemy.team=1;friendly.team=2;
    RadioLogic logic;gameLogic=&logic;RadioSettings observedSettings;settings=&observedSettings;
    auto* hud=new RadioHud;hudManager=hud;
    RadioOwner infantry,vehicleRadio,squad,leader,commander;
    infantry.enabledTag='i';vehicleRadio.enabledTag='v';
    hud->m_field108=&infantry;hud->m_field10c=&vehicleRadio;hud->m_field110=&squad;
    hud->m_field114=&leader;hud->m_field118=&commander;hud->m_field0f8=nullptr;
    hud->m_field1e4=777;trace.clear();hud->openRadio(false);
    check(trace=="P"&&hud->m_field1e4==0&&hud->modes.empty(),"no-player ordinary radio only clears spotted target");
    trace.clear();hud->openRadio(true);
    check(trace=="PM"&&hud->modes.back()==31&&hud->help.empty(),"alternate ordinary radio opens mode31 even without player");
    players.local=&local;trace.clear();hud->openRadio(true);
    check(trace=="PFRM"&&hud->distance==333&&hud->vehiclePosition,"alternate ordinary radio still ray-casts first");
    check(hud->m_field1e8.x==10&&hud->m_field1e8.z==30,"ray cast writes actual spotted vector");
    trace.clear();hud->openRadio(false);
    check(trace=="PFRVDViMH"&&hud->modes.back()==4&&hud->help.back()==50,"infantry radio reads vehicle twice around default vehicle then enables help");
    RadioObject vehicle,target,parent,defaultObject;VehicleComponent component;
    local.vehicle=&vehicle;local.defaultVehicle=&defaultObject;
    trace.clear();hud->openRadio(false);
    check(trace=="PFRVDVvMH"&&hud->modes.back()==5,"different vehicle defaults to vehicle comm rose without component");
    vehicle.component(&component);component.info.useVehicleCommRose=false;
    trace.clear();hud->openRadio(false);
    check(trace=="PFRVIDVviMH"&&hud->modes.back()==4,"vehicle enabled getter precedes authored comm-rose flag test");
    component.info.useVehicleCommRose=true;vehicleRadio.setEnabled(false);
    trace.clear();hud->openRadio(false);
    check(trace=="PFRVIDVviMH"&&hud->modes.back()==4,"disabled vehicle radio falls back to infantry");
    infantry.setEnabled(false);const auto modeCount=hud->modes.size();trace.clear();hud->openRadio(false);
    check(trace=="PFRVIDVviH"&&hud->modes.size()==modeCount&&hud->help.back()==50,"both disabled radios still enable help");
    infantry.setEnabled(true);vehicleRadio.setEnabled(true);
    hud->hit=true;hud->target=&target;target.parent(&parent);parent.tag='q';
    RadioPco pco,first,second;parent.pco=&pco;first.tag='a';second.tag='b';
    pco.entries[1]=nullptr;pco.entries[2]=&first;pco.entries[3]=&second;
    first.occupant=&enemy;second.occupant=&friendly;
    trace.clear();hud->openRadio(true);
    check(trace=="PFRQqCabTTM"&&hud->m_field1e4==0,"last nonnull occupant wins across null map entries and parent walk");
    RadioNetworkable network;network.m_networkableId=65000;target.net=&network;
    second.occupant=nullptr;trace.clear();hud->openRadio(true);
    check(trace=="PFRQqCabTTNM"&&hud->m_field1e4==65000,"later unoccupied seat preserves earlier enemy and copies unsigned network id");
    target.component(&component);component.info.usePlayerIcon=true;
    enemy.defaultVehicle=&defaultObject;defaultObject.net=&network;target.net=nullptr;
    trace.clear();hud->openRadio(true);
    check(trace=="PFRQqCabTTIDNM"&&hud->m_field1e4==65000,"player icon redirects target to occupant default vehicle root");
    component.info.usePlayerIcon=false;trace.clear();hud->openRadio(true);
    check(trace=="PFRQqCabTTINM"&&hud->m_field1e4==0,"nonnull HUD info without player icon keeps original target and missing network id zero");
    first.occupant=nullptr;second.occupant=nullptr;trace.clear();hud->openRadio(true);
    check(trace=="PFRQqCabM"&&hud->m_field1e4==0,"all-empty seats skip team and network callbacks");
    hud->target=nullptr;trace.clear();hud->openRadio(true);
    check(trace=="PFRM","successful ray with null object skips parent walk");
    // Alternate squad mode precedes ray casting, target clears and map access.
    hud->m_order.targetNetId=99;hud->m_field1e4=88;players.local=nullptr;trace.clear();hud->openSquadRadio(true);
    check(trace=="PM"&&hud->modes.back()==30&&hud->m_order.targetNetId==99&&hud->m_field1e4==88,
          "alternate squad opens mode30 after player query without clearing either target");
    hud->hit=false;hud->m_order.orderId=17;hud->onRay=[&]{check(hud->m_order.targetNetId==0,"squad order target clears before ray callback");};
    trace.clear();squadPrefix(hud);
    check(trace=="PRX"&&hud->distance==1000&&!hud->vehiclePosition,"ordinary squad prepares coordinates even without local player");
    check(hud->m_order.orderId==17&&hud->m_order.points[0].x==10&&hud->m_order.points[0].z==30,"failed ray preserves order type but copies its output position");
    players.local=&local;hud->hit=true;hud->target=&target;target.component(nullptr);target.net=&network;
    RadioTemplate objectTemplate;target.use(&objectTemplate);
    logic.invalid={{"Other",true},{"Target",false},{"Target",true},{"Target",true}};
    pco.entries.clear();pco.entries[1]=&first;pco.entries[2]=&second;
    first.occupant=&friendly;second.occupant=&enemy;
    first.onOccupant=[&]{pco.entries.clear();};
    hud->m_field35c=false;hud->m_field350=1;hud->m_field354=2;hud->m_field358=3;
    trace.clear();squadPrefix(hud);
    check(hud->m_order.orderId==5&&hud->m_order.targetNetId==65000,"squad classifier uses last differing-team occupant in copied map");
    check(trace=="PRULGGGGNPQqCaTTbTTTTX","squad classification preserves repeated team reads and copied map callback order");
    check(objectTemplate.reads==4&&hud->m_field35c&&hud->m_field350==0.6f&&hud->m_field354==0.6f&&hud->m_field358==0.6f,
          "all invalid-list entries visited and true matching rows set order color");
    first.onOccupant=nullptr;pco.entries[1]=&first;pco.entries[2]=&second;
    first.occupant=nullptr;second.occupant=&friendly;trace.clear();squadPrefix(hud);
    check(hud->m_order.orderId==6,"classifier skips empty seats and selects allied order6");
    second.occupant=&enemy;enemy.teamScript={1,2};enemy.teamRead=0;trace.clear();squadPrefix(hud);
    check(hud->m_order.orderId==4&&enemy.teamRead==2,"team changing between repeated comparisons preserves previous order4");
    enemy.teamScript.clear();pco.entries.clear();trace.clear();squadPrefix(hud);
    check(hud->m_order.orderId==4,"empty copied PCO map leaves default order4");
    target.parent(nullptr);logic.invalid={{"Target",false},{"Other",true}};
    hud->m_field35c=false;hud->m_field350=1;hud->m_field354=2;hud->m_field358=3;
    trace.clear();squadPrefix(hud);
    check(hud->m_order.orderId==4&&hud->m_order.targetNetId==65000,"target lacking PCO retains network id and order4");
    check(!hud->m_field35c&&hud->m_field350==1&&hud->m_field354==2&&hud->m_field358==3,
          "false matching and true nonmatching invalid rows leave color untouched");
    target.use(nullptr);vehicle.component(nullptr);
    hud->onRay=nullptr;
    delete hud->m_field2dc;
    hud->onRay.~function();hud->modes.~vector();hud->help.~vector();
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
    hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();::operator delete(hud);hudManager=nullptr;
    std::printf("HUD radio actions: %u/%u checks passed; squad normal path stopped at typed settings callback\n",checks-failures,checks);
    return failures?1:0;
}
