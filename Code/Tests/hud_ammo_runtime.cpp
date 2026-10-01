// [TEMP] [CHOSEN, not recovered] Bounded Ammo/FloatRefData actual-body tests.
// Actual Ammo and Vehicle constructors now run with their native event subscriptions.
// Wider active gameplay updates remain uncalled; no whole HUD publication is claimed.
// [RETIRE-WHEN: HUD-AMMO-RUNTIME-FIXTURE] Replace typed external observers with
// actual game/UI collaborators when their complete lifetimes can be exercised.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <new>
#include <functional>
#include "../BF2/Menu/HudAmmo.h"
#include "../BF2/Menu/HudVehicleInformation.h"
#define private public
#include "../BF2/Menu/BF2HudManager.h"
#include "../BF2/External/BFMeme/Data.h"
#undef private
#include "../BF2/Menu/HudBlinker.h"
#include "../BF2/Menu/HudInformationAdapter.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Game/HudInformationLayer.h"
#include "../BF2/External/BFMeme/System.h"
#include "../BF2/External/BFMeme/NamedObjectRegistry.h"
#include "../BF2/External/BFMeme/TypeDescriptor.h"
#include "../BF2/Game/IGameLogic.h"
#include "../BF2/Game/Objects/IPlayer.h"
#include "../BF2/Scene/Object/PlayerManager.h"
[[noreturn]] static void fixtureUnsupported(const char* name) {
    std::fprintf(stderr,"[FRONTIER] Ammo fixture unexpected %s\n",name);
    std::fflush(stderr);std::abort();
}
#include "hud_kit_observers.h"
#include "hud_vehicle_storage_fixture.h"
namespace dice { namespace hfe {
EventManager* eventManager = nullptr;
IBase* g_hudInformationLayer = nullptr;
IGameLogic* gameLogic = nullptr;
hud::BF2HudManager* hudManager = nullptr;
namespace world {
PlayerManager* playerManager = nullptr;
class CPersistentExplosionEffectManager;
CPersistentExplosionEffectManager* explosionEffectManager = nullptr;
}
} }
using namespace dice::hfe;
using namespace dice::hfe::hud;
using namespace dice::meme;
static int checks, failures;
static std::string trace;
static std::wstring lastMessage,lastDeath;
static std::function<void()> onMessage,onDeath,onStatus;
static std::function<void(int)> onMode;
static std::vector<int> modes;
static void check(bool good, const char* label) {
    ++checks;
    if (!good) { ++failures; std::fprintf(stderr, "FAIL %s (trace=%s)\n",label,trace.c_str()); }
}
static const void* watched[4];
static int freed[4];
static bool destructionActive;
static bool unregistered;
void* operator new[](std::size_t bytes) { return ::operator new(bytes); }
void operator delete[](void* value) noexcept {
    if (destructionActive) for (int i=0;i<4;++i) if(value && value==watched[i]) {
        ++freed[i]; check(unregistered,"both listener removals precede owned string deletion");
    }
    ::operator delete(value);
}
struct Events : EventManager {
    IEventListener* watchedListener=nullptr;
    std::vector<int> removals;
    bool unregisterEventHandler(EventCategory category,IEventListener* listener) override {
        if (listener==watchedListener) {
            removals.push_back(category); unregistered=removals.size()==2;
        }
        return EventManager::unregisterEventHandler(category,listener);
    }
};
struct Binding { char kind; std::string name; const void* address; };
static std::vector<Binding> bindings;
struct Information : HudInformationLayer {
    Information* next=nullptr;
    int calls=0;
    int record(char kind,const std::string& name,const void* value) {
        ++calls;bindings.push_back({kind,name,value});
        if(next)g_hudInformationLayer=next;
        return -1; // The actual binding body must ignore a failure return.
    }
    int registerBool(const std::string& n,bool* p) override {return record('B',n,p);}
    int registerInt(const std::string& n,int* p) override {return record('I',n,p);}
    int registerFloat(const std::string& n,float* p) override {return record('F',n,p);}
    int registerString(const std::string& n,const char** p) override {return record('S',n,p);}
    int registerWideString(const std::string& n,const wchar_t** p) override {return record('W',n,p);}
    void setDisconnectMessage(std::wstring value) override {
        trace+='X';lastMessage=value;if(onMessage)onMessage();
    }
    void setDeathMessage(std::wstring value) override {
        trace+='D';lastDeath=value;if(onDeath)onDeath();
    }
};
struct RegistrySystem : System {
    RegistrySystem() : System(800,600) {m_registry=&root;}
    NameTreeRoot root;
};
struct Hud : BF2HudManager {
    bool blocked=false,enabled=false;
    char tag='A';
    std::function<void()> afterBlocked,afterEnabled;
    bool commander=false;
    int mode=0;
    HudInformationAdapter* adapter=nullptr;
    bool unknown1E8() override {
        trace+=tag;
        const bool result=blocked;
        if(afterBlocked)afterBlocked();
        return result;
    }
    bool isPointerEnabled() override {
        trace+=static_cast<char>(tag+1);
        const bool result=enabled;
        if(afterEnabled)afterEnabled();
        return result;
    }
    void disablePointer() override {trace+=static_cast<char>(tag+2);}
    std::wstring getLocalizedString(std::string key) override {
        trace+='L';check(key=="HUD_TEXT_MENU_YOU_ARE_IN_MAN_DOWN","man-down exact localization key");return L"localized";
    }
    std::wstring replaceWideInputKeys(std::wstring text) override {
        trace+='W';check(text==L"localized","man-down replaces keys after localization");return L"rendered";
    }
    bool isCommanderShown() override {trace+='C';return commander;}
    int getMode() override {trace+='M';return mode;}
    void setMode(int value,bool save) override {
        trace+='S';check(save,"man-down saves previous mode");modes.push_back(value);mode=value;if(onMode)onMode(value);
    }
    HudInformationAdapter* getInformationAdapter() override {trace+='A';return adapter;}
};
struct Players : world::PlayerManager {
    world::IPlayer* local=nullptr;
    std::function<void()> afterQuery;
    world::IPlayer* getLocalHumanPlayer() override {trace+='P';auto* result=local;if(afterQuery)afterQuery();return result;}
};
struct Player : world::PlayerBoundary {
    bool alive=true,down=false;
    bool getIsAlive() const override {trace+='a';return alive;}
    bool getIsManDown() override {trace+='d';return down;}
    int getTeam() const override {fixtureUnsupported("getTeam");}
    int getSquadId() const override {fixtureUnsupported("getSquadId");}
    bool getIsSquadLeader() const override {fixtureUnsupported("getIsSquadLeader");}
    int getKit() const override {fixtureUnsupported("getKit");}
    int getChosenUnlockLevel(int,int) const override {fixtureUnsupported("getChosenUnlockLevel");}
    void setChosenUnlockLevel(int,int,int) override {fixtureUnsupported("setChosenUnlockLevel");}
    bool isUnlocked(int,int,int) override {fixtureUnsupported("isUnlocked");}
};
struct Logic : LogicBoundary {
    int status=4;
    int getGameStatus() override {trace+='T';if(onStatus)onStatus();return status;}
    void suicide() override {fixtureUnsupported("suicide");}
    bool selectTeam(world::IPlayer*,int) override {fixtureUnsupported("selectTeam");}
    bool selectKitAndUnlockLevel(world::IPlayer*,int,int) override {fixtureUnsupported("selectKit");}
};
struct Adapter : HudInformationAdapter {
    bool setBool(std::string name,bool value) override {
        trace+='B';check(name=="CloseToMine"&&!value,"real reset clears mine proximity");return false;
    }
};
static Hud* makeHud() {
    void* memory=::operator new(sizeof(Hud));std::memset(memory,0xa5,sizeof(Hud));
    return new(memory) Hud;
}
static void cleanHud(Hud* value) {
    // Fixture cleanup of the real constructor's native owners; HUD dtor stays open.
    delete value->m_field2dc;
    using W=std::wstring;using S=std::string;using L=std::list<int>;
    using SL=std::list<std::string>;using V=std::vector<dice::hfe::hud::HudPendingRequest>;
    value->m_field010.~W();value->m_field038.~W();value->m_field060.~W();
    value->m_field08c.~S();value->m_field0c0.~S();value->m_field304.~S();
    value->m_field0e8.~L();value->m_field2e0.~SL();value->m_field33c.~SL();value->m_field32c.~V();
    using F=std::function<void()>;value->afterBlocked.~F();value->afterEnabled.~F();
    ::operator delete(value);
}
namespace dice { namespace hfe { namespace hud {
struct HudVehicleInformationFixture {
    static bool& killed(HudVehicleInformation& value) {return value.m_killedState;}
    static void occupied(HudVehicleInformation& value,meme::BfOccupiedVehicleData* data) {value.m_occupiedData=data;}
    static void setup(HudVehicleInformation& value) {
        value.m_field170=20;value.m_field294=true;value.m_playerHealth=100;value.m_vehicleArmor=90;
        value.m_guiIndex=7;value.m_field0a4=5;value.m_vehiclePassengersShow=true;
        value.m_turretIconShow=true;value.m_usingParachute=true;
    }
    static bool cleared(HudVehicleInformation& value) {
        return value.m_field170==0&&!value.m_field294&&value.m_playerHealth==0&&value.m_vehicleArmor==0
            &&value.m_guiIndex==0&&value.m_field0a4==0&&!value.m_vehiclePassengersShow
            &&!value.m_turretIconShow&&!value.m_usingParachute;
    }
};
struct HudAmmoFixture {
static void run() {
    static_assert(sizeof(void*)==4,"native x86 required");
    static_assert(sizeof(HudAmmo)==0x1c4,"non-STL Ammo storage must retain PE452 bytes");
    Events events;eventManager=&events;
    Information first,second;first.next=&second;g_hudInformationLayer=&first;
    RegistrySystem system;
    HudAmmo ammo(&system);
    check(reinterpret_cast<char*>(static_cast<IEventListener*>(&ammo))-reinterpret_cast<char*>(&ammo)==4,
          "secondary event listener adjustment");
    IHudAmmo* primary=&ammo;
    primary->setSystem(nullptr);check(!ammo.m_system,"primary slot0 writes system");
    primary->setSystem(&system);
    check(primary->registerFunctions(),"primary slot8 is recovered true");
    primary->unknown0C();primary->unknown10();primary->unknown14();
    check(primary->registerData(),"registerData ignores all failed information callbacks");
    check(bindings.size()==121 && first.calls==1 && second.calls==120,"all bindings reload information global");
    struct Golden {char kind;const char* name;std::size_t offset;};
    const Golden expected[] = {
        {'B', "BottomRightDirection", 0x18},
        {'F', "BottomRightAlpha", 0x10},
        {'F', "BottomRightFadedAlpha", 0x14},
        {'I', "MissileTargetLockId", 0x98},
        {'F', "TargetDirection", 0x178},
        {'F', "PrimaryAmmo", 0xb4},
        {'F', "SecondaryAmmo", 0xc4},
        {'F', "HitIndicatorIconAlpha", 0xfc},
        {'F', "BombFuel", 0xd0},
        {'I', "FiringIndex", 0xe4},
        {'I', "CrosshairUpPos", 0x10c},
        {'I', "CrosshairDownPos", 0x110},
        {'I', "CrosshairLeftPos", 0x114},
        {'I', "CrosshairRightPos", 0x118},
        {'B', "TVBlink", 0x12c},
        {'B', "SecondaryHeatShow", 0xec},
        {'F', "SecondaryHeatValue", 0xf0},
        {'B', "PrimaryHeatShow", 0xed},
        {'F', "PrimaryHeatValue", 0xf4},
        {'B', "PrimaryGrenadeLoadShow", 0xbf},
        {'B', "SecondaryGrenadeLoadShow", 0xc0},
        {'F', "SecondaryReloadTime", 0xc8},
        {'B', "SecondaryReloadTimeShow", 0xcc},
        {'B', "SecondaryEternalShow", 0xcd},
        {'B', "SecondaryEternalMagsShow", 0xce},
        {'B', "PrimaryEternalShow", 0xbd},
        {'B', "PrimaryEternalMagsShow", 0xbe},
        {'B', "PrimaryReloadTimeShow", 0xbc},
        {'F', "PrimaryReloadTime", 0xb8},
        {'B', "PrimarySingleFireShow", 0xc1},
        {'B', "SecondarySingleFireShow", 0xcf},
        {'B', "PrimaryAmmoBarShow", 0xac},
        {'B', "PrimaryClipsShow", 0xad},
        {'B', "PrimaryAmmoShow", 0x12d},
        {'B', "PrimaryAmmoBlink", 0x12e},
        {'B', "PrimaryHeatBlink", 0x12f},
        {'B', "PrimaryAmmoIconShow", 0x130},
        {'S', "PrimaryAmmoIconPathString", 0x150},
        {'S', "PrimaryAmmoString", 0x144},
        {'B', "PrimaryClipStringShow", 0x134},
        {'B', "PrimaryClipBlink", 0x135},
        {'S', "PrimaryClipPathString", 0x14c},
        {'S', "PrimaryClipString", 0x148},
        {'F', "PrimaryClips", 0x140},
        {'B', "PrimaryClip1Show", 0x131},
        {'B', "PrimaryClip2Show", 0x132},
        {'B', "PrimaryClip3Show", 0x133},
        {'S', "PrimaryAmmoBarFrontPathString", 0x154},
        {'S', "PrimaryAmmoBarBackPathString", 0x158},
        {'B', "SecondaryAmmoBarShow", 0xae},
        {'B', "SecondaryClipsShow", 0xaf},
        {'B', "SecondaryAmmoShow", 0x136},
        {'B', "SecondaryAmmoBlink", 0x137},
        {'B', "SecondaryHeatBlink", 0x138},
        {'B', "SecondaryAmmoIconShow", 0x139},
        {'S', "SecondaryAmmoIconPathString", 0x16c},
        {'S', "SecondaryAmmoString", 0x160},
        {'B', "SecondaryClipStringShow", 0x13e},
        {'B', "SecondaryClipBlink", 0x13f},
        {'S', "SecondaryClipPathString", 0x168},
        {'S', "SecondaryClipString", 0x164},
        {'F', "SecondaryClips", 0x15c},
        {'B', "SecondaryClip1Show", 0x13b},
        {'B', "SecondaryClip2Show", 0x13c},
        {'B', "SecondaryClip3Show", 0x13d},
        {'S', "SecondaryAmmoBarFrontPathString", 0x170},
        {'S', "SecondaryAmmoBarBackPathString", 0x174},
        {'S', "PrimaryWeaponNameString", 0x17c},
        {'S', "SecondaryWeaponNameString", 0x180},
        {'F', "LaserTargetRed", 0xd4},
        {'F', "LaserTargetGreen", 0xd8},
        {'F', "LaserTargetBlue", 0xdc},
        {'F', "LaserTargetAlpha", 0xe0},
        {'F', "PrimaryAmmoColorRed", 0x30},
        {'F', "PrimaryAmmoColorGreen", 0x34},
        {'F', "PrimaryAmmoColorBlue", 0x38},
        {'F', "PrimaryAmmoColorAlpha", 0x3c},
        {'F', "PrimaryClipColorRed", 0x40},
        {'F', "PrimaryClipColorGreen", 0x44},
        {'F', "PrimaryClipColorBlue", 0x48},
        {'F', "PrimaryClipColorAlpha", 0x4c},
        {'F', "SecondaryAmmoColorRed", 0x50},
        {'F', "SecondaryAmmoColorGreen", 0x54},
        {'F', "SecondaryAmmoColorBlue", 0x58},
        {'F', "SecondaryAmmoColorAlpha", 0x5c},
        {'F', "SecondaryClipColorRed", 0x60},
        {'F', "SecondaryClipColorGreen", 0x64},
        {'F', "SecondaryClipColorBlue", 0x68},
        {'F', "SecondaryClipColorAlpha", 0x6c},
        {'F', "PrimaryHeatColorRed", 0x70},
        {'F', "PrimaryHeatColorGreen", 0x74},
        {'F', "PrimaryHeatColorBlue", 0x78},
        {'F', "PrimaryHeatColorAlpha", 0x7c},
        {'F', "SecondaryHeatColorRed", 0x80},
        {'F', "SecondaryHeatColorGreen", 0x84},
        {'F', "SecondaryHeatColorBlue", 0x88},
        {'F', "SecondaryHeatColorAlpha", 0x8c},
        {'F', "FlaresReloadTime", 0x188},
        {'B', "FlaresReloadTimeShow", 0x184},
        {'B', "CrosshairIconShow", 0xe8},
        {'B', "CrosshairActive", 0xe9},
        {'B', "HitIndicatorIconShow", 0xeb},
        {'B', "WrenchRepairingShow", 0xee},
        {'W', "TargetDistanceString", 0x190},
        {'S', "TargetCoordXString", 0x194},
        {'S', "TargetCoordYString", 0x198},
        {'S', "TargetCoordZString", 0x19c},
        {'S', "CrosshairIconPathString", 0x1a0},
        {'B', "FireRateIconShow", 0x18c},
        {'S', "FireRateIconPathString", 0x1a4},
        {'S', "AbilityIconPath", 0x1b4},
        {'B', "AbilityBarShow", 0x1b0},
        {'F', "AbilityBar", 0x1ac},
        {'F', "AbilityValue", 0x1b8},
        {'B', "AbilityValueShow", 0x1b1},
        {'B', "VoiceOverIconShow", 0x1bc},
        {'B', "VoiceOverMuted", 0x1c0},
        {'B', "VoiceOverTalkIconShow", 0x1bd},
        {'B', "VoiceOverDistIconShow", 0x1be},
        {'B', "VoiceOverCommanderIconShow", 0x1bf},
        {'B', "HasMissileConnection", 0x1c1},
    };
    for (std::size_t i=0;i<121 && i<bindings.size();++i)
        check(bindings[i].kind==expected[i].kind && bindings[i].name==expected[i].name
            && bindings[i].address==reinterpret_cast<char*>(&ammo)+expected[i].offset,
            expected[i].name);
    const char* paths[]={"BottomRight/Alpha/BottomRight_alpha","BottomRight/BottomRight_oldXPos",
                        "BottomRight/BottomRight_newXPos","BottomRight/BottomRight_direction"};
    Data* references[4];
    for(int i=0;i<4;++i) {
        references[i]=static_cast<Data*>(system.root.findObject(&system,paths[i]));
        check(references[i]!=nullptr,paths[i]);
    }
    check(FloatRefData::getClassType()->getInstanceCount()==3,"three FloatRef descriptors registered");
    if(references[0]&&references[1]&&references[2]&&references[3]) {
        references[0]->setFloat(&system,.25f);references[1]->setString(&system,"44.5suffix");
        references[2]->setFloat(&system,-2.5f);references[3]->setFloat(&system,1);
        check(ammo.m_field010==.25f && ammo.m_field028==44.5f && ammo.m_field02c==-2.5f && ammo.m_field018,
              "actual registered references write native Ammo fields");
        check(references[0]->getType()==FloatRefData::getClassType()
            && references[3]->getType()==Data::getClassType(),"actual float/bool descriptor dispatch");
        for(int i=0;i<4;++i)check(system.root.findEntry(&system,references[i])->close(&system),"real registered entry cleanup");
    }
    check(FloatRefData::getClassType()->getInstanceCount()==0,"FloatRef descriptor count balanced");
    check(std::strcmp(ammo.m_field158,"Ingame/GeneralIcons/empty.tga")==0
        &&std::strcmp(ammo.m_field154,"Ingame/GeneralIcons/empty.tga")==0
        &&ammo.m_field158!=ammo.m_field154,"back/front empty-bar paths are separate owned clones");
    const char* previous=ammo.m_field158;
    HudAmmo::updateString(&ammo.m_field158,"Ingame/GeneralIcons/empty.tga");
    check(ammo.m_field158==previous,"equal replacement retains pointer");
    HudAmmo::updateString(&ammo.m_field158,nullptr);
    check(ammo.m_field158==previous,"null source preserves existing text");
    HudAmmo::updateString(&ammo.m_field158,"changed");
    check(std::strcmp(ammo.m_field158,"changed")==0,"different source is cloned");
    Hud* hud=makeHud();Hud* other=makeHud();other->tag='D';hudManager=hud;
    ammo.m_field01c=503;ammo.m_field02c=1;ammo.m_field018=true;ammo.m_field13a=true;
    ammo.m_field0e9=true;ammo.m_field0a4=true;trace.clear();
    ammo.hide(false);
    check(ammo.m_field02c==503&&!ammo.m_field018&&!ammo.m_field13a&&ammo.m_field0e9&&trace.empty(),
          "hide false writes position and flags without changing pointer state");
    ammo.m_field0a4=false;ammo.hide(true);
    check(!ammo.m_field0e9&&trace.empty(),"false pointer flag skips all HUD calls");
    ammo.m_field0a4=true;hud->blocked=true;trace.clear();ammo.hide(true);
    check(trace=="A","mode guard short circuits enabled query");
    hud->blocked=false;hud->enabled=false;trace.clear();ammo.hide(true);
    check(trace=="AB","disabled pointer skips disable");
    hud->enabled=true;trace.clear();ammo.hide(true);
    check(trace=="ABC","allowed enabled pointer is disabled after both queries");
    hud->afterBlocked=[&] {
        check(ammo.m_field02c==503&&!ammo.m_field0e9&&!ammo.m_field13a&&!ammo.m_field018,
              "all hide stores precede first HUD callback");
        hudManager=other;
    };
    other->enabled=true;other->afterEnabled=[&]{hudManager=hud;};
    trace.clear();ammo.hide(true);
    check(trace=="AEC","hide reloads HUD global between callbacks");
    hud->afterBlocked=nullptr;other->afterEnabled=nullptr;
    {
        HudVehicleInformation vehicle,otherVehicle;
        Players players;world::playerManager=&players;
        Player player;
        Logic logic;gameLogic=&logic;
        Adapter adapter;hud->adapter=&adapter;
        hudManager=hud;hud->m_field130=&vehicle;hud->m_field12c=&ammo;
        g_hudInformationLayer=&second;
        bool& killed=HudVehicleInformationFixture::killed(vehicle);
        bool& otherKilled=HudVehicleInformationFixture::killed(otherVehicle);
        auto* occupied=reinterpret_cast<BfOccupiedVehicleData*>(0x12345000); // [TEMP] identity, never dereferenced
        HudVehicleInformationFixture::occupied(vehicle,occupied);
        check(hud->getOccupiedVehicleData(1)==occupied,"occupied selector1 returns actual vehicle field");
        hud->m_field130=nullptr;
        check(hud->getOccupiedVehicleData(0)==nullptr&&hud->getOccupiedVehicleData(-1)==nullptr
            &&hud->getOccupiedVehicleData(2)==nullptr,"other occupied selectors do not dereference owner");
        hud->m_field130=&vehicle;hud->m_field0be=false;killed=true;
        onMessage=[&] {check(hud->m_field0be&&!killed,"first show stores connection flag and clears killed state before callback");};
        trace.clear();hud->showConnectionMessage(std::wstring(L"abc\0tail",8));
        check(trace=="X"&&lastMessage.size()==8&&lastMessage[4]==L't',"connection message preserves complete by-value wide string");
        onMessage=nullptr;killed=true;hud->m_field130=nullptr;trace.clear();hud->showConnectionMessage(L"again");
        check(trace=="X"&&killed&&lastMessage==L"again","already-shown message skips vehicle access and retains killed state");
        hud->m_field130=&vehicle;players.local=nullptr;
        onMessage=[&] {check(!hud->m_field0be&&lastMessage.empty(),"clear stores visibility false before empty-message callback");};
        trace.clear();hud->clearConnectionMessage();
        check(trace=="XP"&&killed,"clear with missing player leaves killed state untouched");
        onMessage=nullptr;trace.clear();hud->m_field130=nullptr;hud->clearConnectionMessage();
        check(trace.empty(),"already-clear message does not query any collaborator");
        hud->m_field130=&vehicle;players.local=&player;player.alive=false;player.down=true;killed=false;
        hud->m_field0be=true;trace.clear();hud->clearConnectionMessage();
        check(trace=="XPa"&&killed,"dead player sets killed state without man-down query");
        player.alive=true;player.down=false;hud->m_field0be=true;trace.clear();hud->clearConnectionMessage();
        check(trace=="XPad"&&!killed,"alive non-man-down player clears killed state");
        player.down=true;hud->m_field0be=true;trace.clear();hud->clearConnectionMessage();
        check(trace=="XPad"&&killed,"alive man-down player sets killed state");
        killed=false;otherKilled=false;players.afterQuery=[&]{hud->m_field130=&otherVehicle;};
        hud->m_field0be=true;hud->clearConnectionMessage();
        check(!killed&&otherKilled,"clear reloads vehicle owner after player callback");
        players.afterQuery=nullptr;players.local=nullptr;hud->m_field130=&vehicle;
        for(int branch=0;branch!=4;++branch) {
            HudVehicleInformationFixture::setup(vehicle);
            ammo.m_field01c=503;ammo.m_field02c=2;ammo.m_field018=true;ammo.m_field13a=true;ammo.m_field0e9=true;
            hud->m_field24c=false;hud->commander=branch>=2;hud->mode=branch==0?12:5;
            logic.status=branch==1?3:4;
            onDeath=[&] {check(!hud->m_field24c&&ammo.m_field02c==2,"death text precedes man-down state writes");};
            onStatus=[&] {
                check(hud->m_field24c&&ammo.m_field02c==503&&!ammo.m_field018&&!ammo.m_field13a&&ammo.m_field0e9,
                      "real Ammo hide(false) finishes before real Vehicle reset status callback");
            };
            onMode=branch==3?std::function<void(int)>([&](int value){if(value==15)hud->mode=12;}):std::function<void(int)>();
            trace.clear();modes.clear();hud->onManDown();
            check(lastDeath==L"rendered"&&HudVehicleInformationFixture::cleared(vehicle),"onManDown reaches real reset fields after rendered death text");
            check(killed==(logic.status!=4),"real reset killed-state status comparison");
            const char* expectedTrace[]={"LWDTPPPABCM","LWDTPPPABCMS","LWDTPPPABCSMS","LWDTPPPABCSM"};
            check(trace==expectedTrace[branch],"onManDown callback ordering and conditional modes");
            const std::vector<int> expectedModes=branch==0?std::vector<int>():branch==1?std::vector<int>{0}
                :branch==2?std::vector<int>{15,0}:std::vector<int>{15};
            check(modes==expectedModes,"onManDown preserves mode12 after commander callback when observed");
        }
        onDeath=nullptr;onStatus=nullptr;onMode=nullptr;
        hud->m_field130=nullptr;hud->m_field12c=nullptr;hud->adapter=nullptr;
        world::playerManager=nullptr;gameLogic=nullptr;
    }
    cleanHud(hud);cleanHud(other);hudManager=nullptr;
    auto* ownedEmpty=new char[1];ownedEmpty[0]=0;ammo.m_field144=ownedEmpty;
    auto* ownedWide=new wchar_t[1];ownedWide[0]=0;ammo.m_field190=ownedWide;
    auto* leakedAbility=new char[8];std::strcpy(leakedAbility,"ability");ammo.m_field1b4=leakedAbility;
    watched[0]=ammo.m_field154;watched[1]=ownedEmpty;watched[2]=ownedWide;watched[3]=leakedAbility;
    events.watchedListener=&ammo;
    // Destruct at scope end, with all collaborators still alive.
    destructionActive=true;
}
};
} } }
// A separate lifetime case observes destructor calls before returning from its scope.
static void floatReferenceTests() {
    static_assert(sizeof(FloatRefData)==20,"PE FloatRefData size20");
    FloatRefData fallback;
    check(fallback.m_value==&fallback.m_cachedValue&&fallback.m_text==nullptr&&fallback.getFloat(nullptr)==0,
          "default float reference aliases own cached storage");
    float source=.1f;FloatRefData data(&source,"BoundFloat");
    check(data.m_value==&source&&std::strcmp(data.getName(),"BoundFloat")==0&&data.getObjectSize()==20,
          "external reference and cloned name");
    char text[32];data.getString(nullptr,text,sizeof(text));
    check(std::strcmp(text,"0.100000001")==0,"nine decimal x87 truncation");
    source=3.1415927410125732421875f;data.getString(nullptr,text,sizeof(text));
    check(std::strcmp(text,"3.141592741")==0,"changed external value refreshes cache");
    char* cached=data.m_text;data.getString(nullptr,text,sizeof(text));
    check(data.m_text==cached,"unchanged external value reuses allocation");
    data.m_text[0]=static_cast<char>(0x80);
    check(data.getCharacter(nullptr,0)==-128,"character read sign extends cached byte");
    source=-2.5f;data.getString(nullptr,text,sizeof(text));
    check(std::strcmp(text,"-2.500000000")==0,"cache update replaces changed content");
    check(data.setString(nullptr,"  -1.25e2tail")&&source==-125,"atof prefix parsing writes borrowed storage");
    check(data.setString(nullptr,"invalid")&&source==0,"atof invalid text stores zero");
    fallback.getString(nullptr,text,sizeof(text));
    check(std::strcmp(text,"0.000000000")==0,"initial fallback conversion");
    fallback.setFloat(nullptr,2.5f);fallback.getString(nullptr,text,sizeof(text));
    check(fallback.getFloat(nullptr)==2.5f&&std::strcmp(text,"0.000000000")==0,
          "retail fallback alias preserves stale formatted text after setter");
    struct Setter : FloatRefData {
        int calls=0;System* receiver=nullptr;float value=0;
        bool setFloat(System* s,float v) override {++calls;receiver=s;value=v;return false;}
    } setter;
    auto* identity=reinterpret_cast<System*>(0x1234);
    check(!setter.setString(identity,"3.5suffix")&&setter.calls==1&&setter.receiver==identity&&setter.value==3.5f,
          "string setter dispatches virtual float setter and propagates false");
    auto* descriptor=FloatRefData::getClassType();
    check(data.getType()==descriptor&&descriptor->getBaseType()==Data::getClassType()
        &&descriptor->getInstanceSize()==20&&std::strcmp(descriptor->getName(),"dice::meme::FloatRefData")==0,
        "complete FloatRef descriptor identity, base and size");
    auto* created=static_cast<FloatRefData*>(descriptor->create());
    check(created&&created->getFloat(nullptr)==0&&created->getName()==nullptr,"real descriptor factory constructs fallback");
    delete created;
}
int main() {
    HudAmmoFixture::run();
    check(freed[0]==1&&freed[1]==1&&freed[2]==1&&freed[3]==0,
          "dtor frees normal/narrow-empty/wide-empty ownership but preserves leaked ability icon");
    destructionActive=false;delete[] static_cast<const char*>(watched[3]);
    floatReferenceTests();
    std::printf("HUD Ammo/FloatRefData: %d/%d checks passed\n",checks-failures,checks);
    return failures?1:0;
}
