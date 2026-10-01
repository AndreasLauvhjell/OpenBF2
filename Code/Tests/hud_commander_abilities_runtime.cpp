// [TEMP] [CHOSEN, not recovered] Exercise real cooldown helpers using native
// Commander storage and typed HUD/game observers. No helper is shadowed.
#define main primaryActionsRegressionMain
#include "hud_primary_actions_runtime.cpp"
#undef main
#include <cmath>
void refreshTestArtillery(HudCommander&);
void refreshTestSupplies(HudCommander&);
void refreshTestVehicleDrop(HudCommander&);
void refreshTestSatellite(HudCommander&);
void refreshTestUAV(HudCommander&);
struct AbilityHud : CommandHud {
    std::deque<float> artilleryValues;
    std::vector<int> help;
    std::function<void()> availabilityCallback;
    unsigned mapQueries=0;
    HudMap* getMap() override {++mapQueries;return nullptr;}
    float getArtilleryAvailability() override {
        float result = m_field2c8;
        if (!artilleryValues.empty()) { result=artilleryValues.front(); artilleryValues.pop_front(); }
        return result;
    }
    float getVehicleDropAvailability() override {
        if(availabilityCallback)availabilityCallback();return m_field2d0;
    }
    void enableVOHelpMessage(int id) override {help.push_back(id);}
};
struct AbilityGame : CommandGame {
    unsigned radios=0;
    HudCommander* owner=nullptr;
    bool localRadioMessage(TargetChannel channel,const std::string& message) override {
        check(channel==TCTeam&&message=="COMMSL_ARTILLERY_artillerycondred","artillery warning radio channel and text");
        check(!owner->m_warningActive,"warning callback precedes active flag");
        ++radios;return false;
    }
};
struct AbilityLogic : Logic {
    std::string dropVehicle="jeep";
    mutable unsigned reads=0;
    const std::string& getTeamDropVehicle(int team) const override {
        check(team==2,"drop vehicle query uses local team");++reads;return dropVehicle;
    }
};
int main() {
    if(primaryActionsRegressionMain())return 1;
    checks=failures=0;
    EventManager events;eventManager=&events;
    Players players;world::playerManager=&players;
    CommandPlayer player;players.local=&player;
    AbilityGame current;game=&current;
    AbilityLogic logic;gameLogic=&logic;
    void* storage=::operator new(sizeof(AbilityHud));
    AbilityHud* hud=new(storage)AbilityHud;hudManager=hud;
    hud->m_field0f8=nullptr;
    HudCommander commander(nullptr);current.owner=&commander;
    WorldPref::resetWorldTime(10);
    hud->mode=24;commander.m_supplyDuration=20;commander.m_supplyDeadline=15;
    commander.m_supplyRemaining=20;commander.m_supplyReloading=true;
    refreshTestSupplies(commander);
    check(commander.m_supplyState&&commander.m_supplyRemaining==5&&commander.m_supplyFraction==.25f&&commander.m_supplyReloading,"supply cooldown and mode");
    WorldPref::resetWorldTime(16);refreshTestSupplies(commander);
    check(commander.m_supplyRemaining==-1&&commander.m_supplyFraction==-.05f&&commander.m_supplyReloading,"expired supply stays negative and reloading for this call");
    refreshTestSupplies(commander);
    check(commander.m_supplyRemaining==-1&&commander.m_supplyFraction==0&&!commander.m_supplyReloading,"following call clears reload without clamping remaining");
    commander.m_supplyRemaining=std::numeric_limits<float>::quiet_NaN();commander.m_supplyReloading=true;hud->mode=0;
    refreshTestSupplies(commander);
    check(!commander.m_supplyState&&!commander.m_supplyReloading&&std::isnan(commander.m_supplyRemaining),"unordered supply remaining ends reload and preserves remaining");
    commander.m_supplyRemaining=1;commander.m_supplyReloading=true;commander.m_supplyDuration=3;
    commander.m_supplyDeadline=1;WorldPref::resetWorldTime(std::ldexp(1.0,-25));refreshTestSupplies(commander);
    check(commander.m_supplyRemaining==1.0f&&commander.m_supplyFraction==0.3333333134651184f,"fraction divides the unrounded x87 difference");
    hud->mode=22;hud->setArtilleryEnabled(false);commander.m_artilleryDuration=8;commander.m_artilleryFraction=.125f;
    WorldPref::resetWorldTime(30);refreshTestArtillery(commander);
    check(commander.m_artilleryState&&commander.m_noArtillery&&commander.m_artilleryReloading&&commander.m_artilleryDeadline==38&&commander.m_artilleryRemaining==8,"disabled artillery restarts cooldown");
    check(commander.m_artilleryFraction==.125f,"disabled artillery leaves fraction untouched");
    hud->setArtilleryEnabled(true);hud->artilleryValues={1,.75f};commander.m_previousArmor=1;
    refreshTestArtillery(commander);
    check(hud->artilleryValues.empty()&&commander.m_artilleryArmor==.75f&&!commander.m_noArtillery&&commander.m_previousArmor==.75f,"artillery makes two independent availability queries");
    check(hud->help==std::vector<int>{53}&&current.radios==1&&commander.m_warningActive&&commander.m_warningTime==30,"damaged artillery raises help and warning");
    hud->artilleryValues={1,.5f};refreshTestArtillery(commander);
    check(current.radios==1,"active warning suppresses another radio");
    hud->artilleryValues={1,.75f};commander.m_warningPending=true;refreshTestArtillery(commander);
    check(!commander.m_warningActive&&!commander.m_warningPending,"repair clears both warning flags");
    hud->artilleryValues={1,0};refreshTestArtillery(commander);
    check(commander.m_warningPending&&!commander.m_warningActive&&current.radios==1,"first fall to zero sets pending without radio");
    commander.m_previousArmor=.5f;hud->artilleryValues={1,0};refreshTestArtillery(commander);
    check(commander.m_warningActive&&current.radios==2,"repeated zero drop with pending warning sends radio");
    WorldPref::resetWorldTime(60);hud->artilleryValues={1,0};refreshTestArtillery(commander);
    check(commander.m_warningActive,"warning interval equality does not expire");
    WorldPref::resetWorldTime(60.5);hud->artilleryValues={1,0};refreshTestArtillery(commander);
    check(!commander.m_warningActive,"warning expires only after strict interval");
    hud->artilleryValues={std::numeric_limits<float>::quiet_NaN()};refreshTestArtillery(commander);
    check(commander.m_noArtillery&&commander.m_artilleryReloading,"unordered first artillery availability selects unavailable arm");
    hud->mode=25;hud->setVehicleDropAvailability(1);commander.m_vehicleDropDuration=4;
    commander.m_vehicleDropRemaining=4;commander.m_vehicleDropDeadline=64;commander.m_vehicleDropReloading=true;
    WorldPref::resetWorldTime(61);refreshTestVehicleDrop(commander);
    check(commander.m_vehicleDropState&&!commander.m_noVehicleDrop&&commander.m_vehicleDropRemaining==3&&commander.m_vehicleDropFraction==.75f,"available drop countdown");
    hud->setVehicleDropAvailability(0);refreshTestVehicleDrop(commander);
    check(commander.m_noVehicleDrop&&commander.m_vehicleDropDeadline==65&&commander.m_vehicleDropRemaining==4&&commander.m_vehicleDropFraction==1,"first unavailable drop restarts and computes fraction");
    commander.m_vehicleDropFraction=.375f;WorldPref::resetWorldTime(62);refreshTestVehicleDrop(commander);
    check(commander.m_vehicleDropDeadline==66&&commander.m_vehicleDropRemaining==4&&commander.m_vehicleDropFraction==.375f,"already unavailable drop updates deadline and preserves stale fraction");
    hud->setVehicleDropAvailability(std::numeric_limits<float>::quiet_NaN());refreshTestVehicleDrop(commander);
    check(!commander.m_noVehicleDrop&&std::isnan(commander.m_vehiclesAvailable),"unordered drop availability passes zero gate");
    logic.dropVehicle.clear();hud->availabilityCallback=[&]{logic.dropVehicle="replacement";};refreshTestVehicleDrop(commander);
    check(commander.m_noVehicleDrop,"empty vehicle decision is cached before availability callback");
    hud->availabilityCallback=nullptr;logic.dropVehicle=std::string("\0",1);refreshTestVehicleDrop(commander);
    check(!commander.m_noVehicleDrop,"native string length, not C string terminator, controls empty check");
    commander.m_vehicleDropRemaining=-1;commander.m_vehicleDropReloading=true;refreshTestVehicleDrop(commander);
    check(!commander.m_vehicleDropReloading&&commander.m_vehicleDropFraction==0&&commander.m_vehicleDropRemaining==-1,"drop completion preserves negative remaining");
    hud->mapQueries=0;hud->setSatelliteEnabled(false);commander.m_satelliteArmor=.25f;
    refreshTestSatellite(commander);
    check(commander.m_noSatellite&&commander.m_satelliteArmor==.25f&&hud->mapQueries==1,"disabled satellite queries map and preserves armor");
    hud->setSatelliteEnabled(true);hud->setSatelliteAvailability(.5f);hud->help.clear();refreshTestSatellite(commander);
    check(!commander.m_noSatellite&&commander.m_satelliteArmor==.5f&&hud->help==std::vector<int>{52},"damaged satellite help and available flag");
    hud->setSatelliteAvailability(std::numeric_limits<float>::quiet_NaN());hud->help.clear();refreshTestSatellite(commander);
    check(commander.m_noSatellite&&hud->help.empty()&&hud->mapQueries==2,"unordered satellite availability selects reset arm without help");
    hud->setUAVEnabled(false);commander.m_noUAV=false;commander.m_uavArmor=.75f;refreshTestUAV(commander);
    check(!commander.m_noUAV&&commander.m_uavArmor==.75f&&hud->mapQueries==2,"disabled UAV does not change state or query map");
    hud->setUAVEnabled(true);hud->setUAVAvailability(0);hud->help.clear();refreshTestUAV(commander);
    check(commander.m_noUAV&&hud->help==std::vector<int>{51}&&hud->mapQueries==3,"new unavailable UAV emits help and queries map");
    refreshTestUAV(commander);check(hud->mapQueries==3,"already unavailable UAV does not reset map again");
    hud->setUAVAvailability(std::numeric_limits<float>::quiet_NaN());hud->help.clear();refreshTestUAV(commander);
    check(!commander.m_noUAV&&hud->help.empty(),"unordered UAV availability clears unavailable state");
    hud->commander=false;hud->mode=19;current.notice.clear();hud->toggleUAVState();
    check(hud->mode==19&&current.notice.empty(),"UAV toggle ignores hidden commander");
    hud->commander=true;players.local=nullptr;hud->mode=23;hud->toggleUAVState();
    check(hud->mode==23,"UAV close still requires local player");
    players.local=&player;hud->toggleUAVState();check(hud->mode==15,"UAV closes mode23");
    hud->setUAVAvailability(1);hud->mode=19;hud->toggleUAVState();
    check(hud->mode==19&&current.notice=="uav_in_use","missing map reports busy without mode change");
    hud->setUAVAvailability(std::numeric_limits<float>::quiet_NaN());hud->toggleUAVState();
    check(hud->mode==15&&current.notice=="uav_unavailable","unordered UAV toggle restores commander mode and reports unavailable");
    delete hud->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;using SL=std::list<std::string>;using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    hud->m_field010.~W();hud->m_field038.~W();hud->m_field060.~W();hud->m_field08c.~S();hud->m_field0c0.~S();hud->m_field304.~S();
    hud->m_field0e8.~L();hud->m_field2e0.~SL();hud->m_field33c.~SL();hud->m_field32c.~V();::operator delete(storage);hudManager=nullptr;
    std::printf("HUD Commander ability helpers: %u/%u checks passed\n",checks-failures,checks);return failures?1:0;
}
