// [TEMP] [RETIRE-WHEN: HUD-VEHICLE-VIRTUALS-FIXTURE] Typed observations of the
// actual owner virtual wrappers. Named, unrecovered leaf bodies below model only
// the callback edges exercised here; they do not enable production publication.
#define main vehicle_construction_regression_main
#include "hud_vehicle_construct_runtime.cpp"
#undef main
#include "../BF2/Game/IGame.h"
#include <limits>
// The included storage fixture opens private fields. MSVC also encodes that
// access specifier in member symbol names; these aliases bridge only that name
// difference. Both sides remain the same native thiscall signature.
#pragma comment(linker,"/alternatename:?updateLayout@HudVehicleInformation@hud@hfe@dice@@AAEXXZ=?updateLayout@HudVehicleInformation@hud@hfe@dice@@QAEXXZ")
#pragma comment(linker,"/alternatename:?updateProximity@HudVehicleInformation@hud@hfe@dice@@AAEXM@Z=?updateProximity@HudVehicleInformation@hud@hfe@dice@@QAEXM@Z")
#pragma comment(linker,"/alternatename:?updatePlayerInformation@HudVehicleInformation@hud@hfe@dice@@AAEXPAVIObject@world@34@@Z=?updatePlayerInformation@HudVehicleInformation@hud@hfe@dice@@QAEXPAVIObject@world@34@@Z")
#pragma comment(linker,"/alternatename:?updatePlayerAngles@HudVehicleInformation@hud@hfe@dice@@AAEXPAVIPlayer@world@34@@Z=?updatePlayerAngles@HudVehicleInformation@hud@hfe@dice@@QAEXPAVIPlayer@world@34@@Z")
#pragma comment(linker,"/alternatename:?updateVehicleInformation@HudVehicleInformation@hud@hfe@dice@@AAEXPAVIObject@world@34@M@Z=?updateVehicleInformation@HudVehicleInformation@hud@hfe@dice@@QAEXPAVIObject@world@34@M@Z")
#pragma comment(linker,"/alternatename:?updateVehicleAngles@HudVehicleInformation@hud@hfe@dice@@AAEXPAVIObject@world@34@@Z=?updateVehicleAngles@HudVehicleInformation@hud@hfe@dice@@QAEXPAVIObject@world@34@@Z")
#pragma comment(linker,"/alternatename:?setupVehicle@HudVehicleInformation@hud@hfe@dice@@AAEXPAVIObject@world@34@@Z=?setupVehicle@HudVehicleInformation@hud@hfe@dice@@QAEXPAVIObject@world@34@@Z")
// Uses production event routing; only unresolved per-case dependencies are modeled below.
#pragma comment(linker,"/alternatename:?setupOccupiedVehicle@HudVehicleInformation@hud@hfe@dice@@AAEXPAVIObject@world@34@_N@Z=?setupOccupiedVehicle@HudVehicleInformation@hud@hfe@dice@@QAEXPAVIObject@world@34@_N@Z")
#pragma comment(linker,"/alternatename:?handleMissileLockEvent@HudVehicleInformation@hud@hfe@dice@@AAEXPAX@Z=?handleMissileLockEvent@HudVehicleInformation@hud@hfe@dice@@QAEXPAX@Z")
#pragma comment(linker,"/alternatename:?handleCrossHairEvent@HudVehicleInformation@hud@hfe@dice@@AAEXIPAX@Z=?handleCrossHairEvent@HudVehicleInformation@hud@hfe@dice@@QAEXIPAX@Z")
#pragma comment(linker,"/alternatename:?assignString@HudVehicleInformation@hud@hfe@dice@@QAEXAAPADPBD@Z=?assignString@HudVehicleInformation@hud@hfe@dice@@AAEXAAPADPBD@Z")
namespace dice { namespace hfe { IGame* game=nullptr; } }
static float observedDt;
static unsigned observedId;
static EventNode* observedNode;
static void* observedData;
namespace dice { namespace hfe { namespace hud {
void HudVehicleInformation::updateLayout(){trace+='L';}
void HudVehicleInformation::updateProximity(float dt){trace+='Q';observedDt=dt;}
void HudVehicleInformation::updatePlayerInformation(world::IObject*){fixtureUnsupported("player information leaf");}
void HudVehicleInformation::updatePlayerAngles(world::IPlayer*){fixtureUnsupported("player angles leaf");}
void HudVehicleInformation::updateVehicleInformation(world::IObject*,float){fixtureUnsupported("vehicle information leaf");}
void HudVehicleInformation::updateVehicleAngles(world::IObject*){fixtureUnsupported("vehicle angles leaf");}
void HudVehicleInformation::setupVehicle(world::IObject*){fixtureUnsupported("vehicle setup leaf");}
void HudVehicleInformation::setupOccupiedVehicle(world::IObject*,bool){fixtureUnsupported("occupied vehicle setup leaf");}
void HudVehicleInformation::handleMissileLockEvent(void*){fixtureUnsupported("missile lock case");}
void HudVehicleInformation::handleCrossHairEvent(unsigned,void*){fixtureUnsupported("crosshair cases");}
void dispatchWarningHudLockState(world::IObjectComponent*,int){fixtureUnsupported("warning lock seam");}
void dispatchHudMapApplyVehicleZoom(meme::Bf2MenuMapNode*){fixtureUnsupported("map vehicle zoom seam");}
} } }
struct VirtualPlayer : HudVehicleInformationFixture::Player {
    bool alive=false;
    int id=1;
    int getId() const override {trace+='#';return id;}
    bool getIsAlive() const override {trace+='A';return alive;}
    world::IObject* getDefaultVehicle() override {trace+='D';return nullptr;}
    world::IObject* getVehicle() const override {trace+='V';return nullptr;}
    world::ICameraObject* getCamera() const override {trace+='C';return nullptr;}
    bool getOverheadCameraEnabled() const override {trace+='O';return false;}
};
struct VirtualAdapter : HudInformationAdapter {
    std::vector<float> values;
    bool setFloat(std::string name,float value) override {
        trace+='F';check(name=="CrossHairColorAlpha","crosshair exact key");values.push_back(value);return false;
    }
};
struct VirtualHud : BF2HudManager {
    VirtualAdapter adapter;bool down=false;int mode=0;
    int getMode() override {trace+='M';return mode;}
    bool isCommanderShown() override {trace+='S';return false;}
    bool isPlayerAlive(world::IPlayer*) override {trace+='I';return down;}
    world::Kit* getPlayerKit(world::IPlayer*) override {trace+='K';return nullptr;}
    HudInformationAdapter* getInformationAdapter() override {trace+='G';return &adapter;}
};
static void virtualChecks() {
    std::fprintf(stderr,"[TEMP] virtual events construction\n");
    ConstructionEvents events;eventManager=&events;
    std::fprintf(stderr,"[TEMP] virtual player manager construction\n");
    HudVehicleInformationFixture::Players players;world::playerManager=&players;
    std::fprintf(stderr,"[TEMP] virtual HUD construction\n");
    auto* hud=new VirtualHud;hudManager=hud;
    {
        std::fprintf(stderr,"[TEMP] virtual owner construction\n");
        HudVehicleInformation v;
        trace.clear();v.handleEvent(static_cast<EventCategory>(99),5,nullptr,nullptr);
        check(trace.empty(),"unrelated event category makes no player read");
        trace.clear();v.handleEvent(ECGameLogic,3,nullptr,nullptr);
        check(trace=="P","every game-logic event performs discarded player lookup");
        trace.clear();v.m_field018=8;v.m_field01c=9;v.handleEvent(ECGameLogic,5,nullptr,nullptr);
        check(trace=="PP"&&v.m_field020&&v.m_field01c==5&&v.m_field018==0,"game-logic5 refresh then exact timer stores");
        int payload=17;
        // Both leaves are recovered now. Their unconditional local-player lookup runs for every
        // id, including ids inside the jump table that do nothing and ids outside it.
        trace.clear();v.handleEvent(ECGame,31,nullptr,&payload);
        check(trace=="P","game event outside ids6..11 still performs the player lookup");
        trace.clear();v.handleEvent(ECGame,7,nullptr,&payload);
        check(trace=="P","game event id7 falls through after the player lookup");
        trace.clear();v.handleEvent(ECHud,33,nullptr,&payload);
        check(trace=="P","HUD event inside the table but unhandled performs only the player lookup");
        trace.clear();v.handleEvent(ECHud,82,nullptr,&payload);
        check(trace=="P","HUD event past the table performs only the player lookup");
        // ECHud2 reads the HUD mode before applyHudState, which then evaluates the commander
        // predicate before testing for a local player. With no local player it stops there.
        hud->mode=0;trace.clear();v.handleEvent(ECHud,2,nullptr,nullptr);
        check(trace=="PMPS","HUD state change reads mode, repeats the player lookup, then stops");
        // ECHud57/58 resolve the payload id through the player manager first; an unknown id stops.
        int unknownId=4242;hud->adapter.values.clear();
        trace.clear();v.handleEvent(ECHud,57,nullptr,&unknownId);
        check(trace=="P"&&hud->adapter.values.empty(),"close-to-mine stops on an unresolved player id");
        trace.clear();check(!v.updatePostProcessing()&&trace=="P","postprocessing false with no player");
        trace.clear();check(!v.canSelectWeapon(false)&&trace=="P","weapon selection false with no player");
        v.m_squadInfoBarShow=true;v.m_field0b4=0;v.m_crossHairMode=0;
        trace.clear();v.update(.25f);
        check(trace=="PL"&&v.m_squadInfoBarShow,"null player still updates layout and preserves player fields");
        v.m_field0b4=.25f;v.m_guiIndex=7;v.m_field0a8=7;v.m_field0b0=3;
        v.update(.5f);check(v.m_field0b4==-.25f&&v.m_guiIndex==3,"GUI timer subtracts then restores matching index");
        v.m_field0b4=.1f;v.m_guiIndex=8;v.m_field0a8=7;v.update(.2f);
        check(v.m_guiIndex==8,"expired GUI timer preserves different current index");
        v.m_field0b4=-2;v.update(3);check(v.m_field0b4==-2,"inactive GUI timer remains untouched");
        const float nan=(std::numeric_limits<float>::quiet_NaN)();
        v.m_field0b4=nan;v.update(1);check(v.m_field0b4!=v.m_field0b4,"unordered GUI timer remains inactive");
        v.m_field0b4=0;v.m_crossHairMode=1;v.m_field2a4=.25f;v.m_field2a8=0;hud->adapter.values.clear();
        trace.clear();v.update(.125f);
        check(v.m_field2a8==1&&v.m_field2a4==.125f&&hud->adapter.values.size()==1&&hud->adapter.values[0]==.25f,"zero fade divisor becomes one before alpha and subtraction");
        check(trace=="PLGF","crosshair adapter getter/callback follow layout");
        v.m_crossHairMode=2;v.m_field2a4=.25f;v.m_field2a8=.5f;v.m_guiIndex=9;hud->adapter.values.clear();
        trace.clear();v.update(.5f);
        check(trace=="PLGFGF"&&hud->adapter.values.size()==2&&hud->adapter.values[0]==.5f&&hud->adapter.values[1]==1,"expired fade repeats getter then restores full alpha");
        check(v.m_crossHairMode==0&&v.m_guiIndex==3&&v.m_field2a4==-.25f,"expired fade restores GUI and clears mode after callback");
        v.m_crossHairMode=1;v.m_field2a4=nan;v.m_field2a8=nan;hud->adapter.values.clear();
        v.update(.5f);check(v.m_field2a8!=v.m_field2a8&&v.m_crossHairMode==1&&hud->adapter.values.size()==1,"NaN divisor is not replaced and NaN timer does not expire");
        v.m_crossHairMode=3;hud->adapter.values.clear();v.update(1);check(hud->adapter.values.empty(),"only crosshair modes one and two fade");
        VirtualPlayer player;players.local=&player;v.m_crossHairMode=0;
        // ECHud69 tests the sentinel before it dispatches on the local player, and that local
        // dispatch has no null guard in retail - the sentinel arm is what keeps it reachable.
        int sentinel=-1;trace.clear();v.handleEvent(ECHud,69,nullptr,&sentinel);
        check(trace=="P","dropped-kit sentinel returns before the unguarded local dispatch");
        int otherId=77;v.m_field294=false;trace.clear();v.handleEvent(ECHud,69,nullptr,&otherId);
        check(trace=="P#"&&!v.m_field294,"dropped-kit id mismatch reads the local id then stops");
        // ECGame6 reads the local id first, then the payload player's.
        // The payload player IS the local player here, so the id gate opens and the whole
        // refresh runs. updateGuiIndex appears twice on purpose: updateHudMode tail-calls it
        // (retail jmp sub_78B340) and case6 then calls it again.
        trace.clear();v.handleEvent(ECGame,6,nullptr,&player);
        check(trace=="P##SPVPAPA","spawn event reads both ids, then refreshes the mode and the index twice");
        trace.clear();check(!v.updatePostProcessing()&&trace=="PKDVO","postprocessing preserves kit/default/vehicle/overhead callback order");
        trace.clear();check(!v.canSelectWeapon(true)&&trace=="PV","selection stops after null vehicle");
        v.m_squadInfoBarShow=v.m_showCommanderIcon=v.m_showSquadIcon=true;
        v.m_usingParachute=v.m_playerHealthShow=v.m_playerStaminaShow=true;
        v.m_squadInfoText=static_cast<wchar_t*>(::operator new(8));std::wcscpy(v.m_squadInfoText,L"abc");
        v.assignString(v.m_squadInfoIconPath,"icon");
        trace.clear();v.update(.2f);
        check(trace=="PLA"&&!v.m_squadInfoBarShow&&!v.m_showCommanderIcon&&!v.m_showSquadIcon,"dead-player path skips HUD alive callback and clears squad flags");
        check(!v.m_usingParachute&&!v.m_playerHealthShow&&!v.m_playerStaminaShow&&v.m_field0bc==-1,"dead-player path clears exact visibility fields");
        check(!*v.m_squadInfoText&&!*v.m_squadInfoIconPath,"dead-player path clears owned text and icon strings");
        auto* empty=v.m_squadInfoText;v.update(1);check(v.m_squadInfoText==empty,"equal empty text keeps its allocation");
        player.alive=true;hud->down=true;trace.clear();v.update(.3f);
        check(trace=="PLAI","man-down HUD predicate enters the same hidden path");
        hud->down=false;v.m_field294=false;v.m_bottomLeftVehicleAlpha=.2f;trace.clear();v.update(.4f);
        check(trace=="PLAIQDDVC"&&observedDt==.4f,"living path preserves repeated default/vehicle calls and proximity argument");
        check(v.m_playerHealthShow&&v.m_vehicleIconShow&&v.m_vehicleArmorShow&&!v.m_turretIconShow,"living path sets visibility and clears turret for equal vehicles");
        v.m_field294=true;v.m_bottomLeftVehicleAlpha=nan;trace.clear();v.update(.4f);
        check(trace=="PLAIDDVC"&&!v.m_vehicleIconShow&&!v.m_vehicleArmorShow,"proximity gate and unordered alpha are preserved");
        players.local=nullptr;
    }
    hudManager=nullptr;hud->adapter.~VirtualAdapter();destroyHudStorage(hud);
}
static LONG WINAPI reportException(EXCEPTION_POINTERS* info) {
    std::fprintf(stderr,"[TEMP] exception %08lx at %p, access %p, trace=%s\n",
        info->ExceptionRecord->ExceptionCode,info->ExceptionRecord->ExceptionAddress,
        reinterpret_cast<void*>(info->ExceptionRecord->ExceptionInformation[1]),trace.c_str());
    std::fflush(stderr);return EXCEPTION_EXECUTE_HANDLER;
}
int main() {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter(reportException);
    if(vehicle_construction_regression_main())return 1;
    std::fflush(stdout);std::fprintf(stderr,"[TEMP] virtual wrapper checks\n");
    checks=failures=0;trace.clear();virtualChecks();
    std::printf("Vehicle virtual wrappers: %u/%u checks passed\n",checks-failures,checks);
    return failures?1:0;
}
