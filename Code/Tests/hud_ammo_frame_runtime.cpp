// [TEMP] [CHOSEN, not recovered] Native Ammo ability-frame observers.
// [RETIRE-WHEN: HUD-AMMO-RUNTIME-FIXTURE] Replace external player/sound/PCO
// observers when complete world lifetimes can exercise these same branches.
// Ammo, Kit, GenericFireArm, component templates, and weak storage are real.
// World owners are retained; no recovered Ammo helper is shadowed.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define main ammoOldRegressionMain
#include "hud_ammo_runtime.cpp"
#undef main
#include "../BF2/Core/System.h"
#include "../BF2/Core/WeakPtr.h"
#include "../BF2/Core/Ids.h"
#include "../BF2/Game/Objects/Object.h"
#include "../BF2/Game/Objects/ObjectLayoutAccess.h"
#include "../BF2/Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "../BF2/Game/Objects/IAbilityHudComp.h"
#include "../BF2/Game/Objects/Game/Kit.h"
#include "../BF2/Game/Objects/Game/KitTemplate.h"
#include "../BF2/Game/Objects/AbilityGLCompTemplate.h"
#include "../BF2/Game/Objects/GameLogicComp/AbilityGLComp.h"
#include "../BF2/Game/Objects/GenericFireArmTemplate.h"
#include "../BF2/Game/Objects/Weapons/GenericFireArm.h"
#include "../BF2/Game/Objects/Weapons/AmmoComponents/ReplenishingAmmoComp.h"
#define actionUnsupported fixtureUnsupported
#include "hud_radio_action_observers.h"
#undef actionUnsupported
#include <limits>
namespace dice { namespace hfe { namespace world {
ObjectManager* objectManager=nullptr;
} } }
using namespace dice::hfe::world;
struct FrameAbility : IAbilityHudComp {
    int stops=0; std::vector<std::pair<char,bool>> active;
    std::function<void(char)> after;
    void addRef() override {fixtureUnsupported("ability addRef");}
    int getRef() override {fixtureUnsupported("ability getRef");}
    int release() override {fixtureUnsupported("ability release");}
    void* queryInterface(unsigned) const override {fixtureUnsupported("ability query");}
    void setObject(IObject*) override {fixtureUnsupported("ability setObject");}
    IObject* getObject() const override {fixtureUnsupported("ability getObject");}
    AbilityHudInfo* getHudInfo() override {fixtureUnsupported("ability info");}
    void record(char kind,bool value){trace+=kind;active.push_back({kind,value});if(after)after(kind);}
    void setHealingActive(bool b) override {record('H',b);}
    void setRepairingActive(bool b) override {record('R',b);}
    void setAmmoActive(bool b) override {record('A',b);}
    void setGetHealActive(bool) override {fixtureUnsupported("receive healing");}
    void setGetRepairActive(bool) override {fixtureUnsupported("receive repair");}
    void setGetAmmoActive(bool) override {fixtureUnsupported("receive ammo");}
    bool isPlayingHealSound() const override {fixtureUnsupported("playing healing");}
    bool isPlayingRepairSound() const override {fixtureUnsupported("playing repair");}
    bool isPlayingAmmoSound() const override {fixtureUnsupported("playing ammo");}
    void forceStopAllSound() override {trace+='s';++stops;}
};
struct FrameKit : Kit {
    FrameKit(const KitTemplate* t):Kit(t){}
    void components(IAbilityGLComp* gl,IAbilityHudComp* hud){m_abilityGLComponent=gl;m_abilityHudComponent=hud;}
};
struct FrameHud : Hud {
    Kit* kit=nullptr;std::function<void()> afterKit;
    Kit* getPlayerKit(IPlayer*) override {trace+='k';Kit* result=kit;if(afterKit)afterKit();return result;}
};
struct FramePlayer : ::Player {
    IObject* vehicle=nullptr;IObject* fallback=nullptr;
    bool giving=false,repairing=false,replenishing=false;
    unsigned repaired=0,ammoAt=0;
    std::function<void()> afterVehicle;
    IObject* getVehicle() const override {trace+='v';auto* result=vehicle;if(afterVehicle)afterVehicle();return result;}
    IObject* getDefaultVehicle() override {trace+='f';return fallback;}
    bool getIsGivingAbility() const override {trace+='g';return giving;}
    bool getIsRepairing() const override {trace+='r';return repairing;}
    bool getIsGivingReplenish() const override {trace+='n';return replenishing;}
    unsigned getHealRepairedAtTick() const override {trace+='t';return repaired;}
    unsigned getGaveAmmoAtTick() const override {trace+='u';return ammoAt;}
};
struct FrameControl : RadioPcoBoundary {
    world::IPlayer* getOccupingPlayer() const override {fixtureUnsupported("PCO occupant");}
    std::map<int,IPlayerControlObject*>* getPcos() override {fixtureUnsupported("PCO children");}
    IObject* weapon=nullptr;bool child=false;int ammo=15;
    std::function<void()> afterAmmo;
    IObject* getActiveWeapon(int index) const override {trace+='w';check(index==0,"ability selects weapon channel zero");return weapon;}
    bool isChildSoldierActive() const override {trace+='c';return child;}
    int getAmmoStatus() override {trace+='m';const int result=ammo;if(afterAmmo)afterAmmo();return result;}
};
struct FrameObject : world::Object {
    FrameObject():world::Object(nullptr){}
    IPlayerControlObject* control=nullptr;
    std::function<void()> afterQuery;
    void parent(IObject* p){m_parent=p;}
    void* queryInterface(unsigned id) const override {
        if(id==IID_IPlayerControlObject){trace+='q';auto* result=control;if(afterQuery)afterQuery();return result;}
        return world::Object::queryInterface(id);
    }
};
struct FrameWeaponTemplate : GenericFireArmTemplate {
    // This fixture observes a disabled world weapon; the real constructor's
    // updateFlags therefore does not require an active world scheduling manager.
    void objectCreated(IObject* object) override {object_detail::LayoutAccess::flags(object)|=1;}
    IObjectTemplateComponent* component=nullptr;
    IObjectTemplateComponent* queryComponent(unsigned id) override {
        trace+='C';check(id==IID_IWeaponAmmoCompTemplate,"replenish predicate uses ammo interface id");return component;
    }
};
struct FrameWeapon : GenericFireArm {
    FrameWeapon(const GenericFireArmTemplate* t):GenericFireArm(t){}
    mutable std::vector<WeakPtr<IObject>> targets;
    bool working=true,listPresent=true;float amount=0.375f;
    std::function<void()> afterAmount;
    void* getAbilityObjectList() const override {trace+='l';return listPresent?&targets:nullptr;}
    bool getAbilityWorking() const override {trace+='b';return working;}
    float getAbility() const override {trace+='x';const float result=amount;if(afterAmount)afterAmount();return result;}
};
static WeakPtr<IObject> weak(IObject* p){WeakPtr<IObject> w;w.m_ctrl=new WeakPtr<IObject>::Ctrl{1,p};return w;}
namespace dice { namespace hfe { namespace hud {
struct HudAmmoFrameFixture {
static void run(){
    Events events;eventManager=&events;
    RegistrySystem system;HudAmmo ammo(&system);
    auto* hud=new FrameHud;hudManager=hud;
    Players players;playerManager=&players;
    FramePlayer player;players.local=&player;
    auto* glTemplate=new AbilityGLCompTemplate;
    auto* gl=new AbilityGLComp(glTemplate);
    FrameAbility sounds,otherSounds;
    auto* kitTemplate=new KitTemplate;
    auto* kit=new FrameKit(kitTemplate);kit->components(gl,&sounds);hud->kit=kit;
    auto* otherKit=new FrameKit(kitTemplate);otherKit->components(gl,&otherSounds);
    auto* object=new FrameObject;auto* parent=new FrameObject;FrameControl control;
    object->parent(parent);parent->control=&control;player.vehicle=object;
    auto* weaponTemplate=new FrameWeaponTemplate;
    auto* replenishing=new ReplenishingAmmoCompTemplate;weaponTemplate->component=replenishing;
    auto* weapon=new FrameWeapon(weaponTemplate);control.weapon=weapon;
    auto* target=new FrameObject;FrameControl targetControl;target->control=&targetControl;
    auto* secondTarget=new FrameObject;FrameControl secondControl;secondTarget->control=&secondControl;
    auto reset=[&]{
        trace.clear();sounds.active.clear();otherSounds.active.clear();sounds.stops=otherSounds.stops=0;
        sounds.after=nullptr;otherSounds.after=nullptr;hud->afterKit=nullptr;hud->kit=kit;
        kit->components(gl,&sounds);players.local=&player;player.alive=true;player.down=false;
        player.giving=false;player.repairing=false;player.replenishing=false;player.afterVehicle=nullptr;
        player.vehicle=object;player.fallback=nullptr;control.weapon=weapon;control.child=false;
        object->parent(parent);parent->control=&control;target->control=&targetControl;target->afterQuery=nullptr;
        targetControl.afterAmmo=nullptr;targetControl.ammo=15;secondControl.ammo=3;
        weaponTemplate->component=replenishing;weapon->targets.clear();weapon->working=true;
        weapon->listPresent=true;weapon->amount=.375f;weapon->afterAmount=nullptr;
        glTemplate->setHasAmmoAbility(true);glTemplate->setHasHealingAbility(false);glTemplate->setHasRepairingAbility(false);
        ammo.m_field1c2=false;ammo.m_field1b0=ammo.m_field0ee=ammo.m_field1b1=true;
        ammo.m_field1ac=.75f;ammo.m_field1b4="stale";ammo.m_field1b8=2;ammo.m_field0b4=3;
        WorldPref::mTickCount=100;player.repaired=player.ammoAt=90;
    };
    reset();players.local=nullptr;ammo.updateAbility(0);
    check(trace=="P"&&!ammo.m_field1b0&&!ammo.m_field0ee&&!ammo.m_field1b1,"missing local clears three flags only");
    check(ammo.m_field1ac==.75f&&ammo.m_field1b8==2,"missing local preserves stale floats");
    reset();player.vehicle=nullptr;ammo.updateAbility(1);
    check(trace=="Pkvvs"&&sounds.stops==1,"missing vehicle stops captured kit sounds after repeated vehicle lookup");
    reset();kit->components(nullptr,&sounds);ammo.updateAbility(2);
    check(sounds.stops==0&&!ammo.m_field1b1,"missing GL with present weapon does not stop sounds");
    reset();weaponTemplate->component=nullptr;ammo.updateAbility(3);
    check(sounds.stops==1&&!ammo.m_field1b1,"non-replenishing weapon stops sounds before target calls");
    reset();control.weapon=nullptr;control.child=false;ammo.updateAbility(4);
    check(sounds.stops==1&&trace.find("wc")!=std::string::npos,"no active weapon queries child-soldier flag");
    reset();control.weapon=nullptr;control.child=true;player.fallback=secondTarget;secondControl.weapon=weapon;
    weapon->targets.push_back(weak(target));ammo.updateAbility(5);
    check(trace.find("wcfqw")!=std::string::npos&&ammo.m_field1b0,"child soldier fallback searches default vehicle control and weapon");
    reset();weapon->targets.push_back(weak(target));ammo.updateAbility(6);
    check(trace=="PkvvqqwClbqqmadAxad","ammo callback sequence includes both target queries and repeated alive/down reads");
    check(ammo.m_field1b0&&!ammo.m_field0ee&&ammo.m_field1b1&&ammo.m_field1ac==1,"ammo target shows bar and value without wrench");
    check(ammo.m_field1b8==.375f&&ammo.m_field0b4==.375f&&sounds.active==std::vector<std::pair<char,bool>>{{'A',true}},"single amount sample populates both fields and active sound");
    check(std::strcmp(ammo.m_field1b4,"Ingame/Player/Bars/abilitybar_smallAmmoIcon.tga")==0,"exact ammo icon path");
    reset();weapon->targets.push_back(WeakPtr<IObject>());weapon->targets.push_back(weak(target));ammo.updateAbility(7);
    check(ammo.m_field1ac==.5f,"skipped initial weak slot still counts toward averaging index");
    reset();weapon->targets.push_back(weak(nullptr));weapon->targets.push_back(weak(target));ammo.updateAbility(8);
    check(ammo.m_field1ac==.5f,"expired native weak control is skipped without removing its ordinal");
    reset();weapon->targets.push_back(weak(target));target->afterQuery=[&]{target->control=&secondControl;};ammo.updateAbility(9);
    check(ammo.m_field1ac==.20000001788139343f,"second query supplies the actual ammo receiver");
    reset();weapon->targets.push_back(weak(target));targetControl.afterAmmo=[&]{weapon->targets.push_back(weak(secondTarget));};ammo.updateAbility(10);
    check(ammo.m_field1ac==.60000002384185791f,"target callback growth reloads native vector end and blends in x87");
    reset();weapon->listPresent=false;ammo.updateAbility(11);
    check(!ammo.m_field1b0&&ammo.m_field1b1&&ammo.m_field1ac==.75f&&sounds.active==std::vector<std::pair<char,bool>>{{'A',false}},"null target vector preserves old bar but still samples ability and disables sound");
    reset();weapon->working=false;weapon->targets.push_back(weak(target));ammo.updateAbility(12);
    check(ammo.m_field1b0&&sounds.active==std::vector<std::pair<char,bool>>{{'A',false}},"non-working target still displays bar");
    reset();player.alive=false;weapon->targets.push_back(weak(target));ammo.updateAbility(13);
    check(sounds.stops==1&&sounds.active.empty()&&!ammo.m_field1b0&&ammo.m_field1b1&&ammo.m_field1b8==.375f,"dead player stops sound and hides bar while still sampling amount");
    check(trace.find('d')==std::string::npos,"dead short-circuit omits both man-down callbacks");
    reset();weapon->targets.push_back(weak(target));weapon->afterAmount=[&]{player.down=true;};ammo.updateAbility(14);
    check(!ammo.m_field1b0&&sounds.active==std::vector<std::pair<char,bool>>{{'A',true}},"post-amount down transition hides bar without replaying sound callback");
    reset();weapon->amount=std::numeric_limits<float>::quiet_NaN();ammo.updateAbility(15);
    check(ammo.m_field1b8!=ammo.m_field1b8&&ammo.m_field0b4!=ammo.m_field0b4,"NaN ability amount propagates to both native floats");
    reset();ammo.m_field1c2=true;player.giving=true;ammo.updateAbility(16);
    check(trace=="PkvvqqgtrkgrRgrHnA","vehicle healing exact repeated-player callback sequence");
    check(ammo.m_field1b0&&!ammo.m_field1b1&&ammo.m_field1ac==1&&sounds.active==std::vector<std::pair<char,bool>>{{'R',false},{'H',true},{'A',false}},"vehicle medic branch has ordered repair/heal/ammo sound updates");
    check(std::strcmp(ammo.m_field1b4,"Ingame/Player/Bars/abilitybar_smallMedicIcon.tga")==0,"exact medic icon path");
    reset();ammo.m_field1c2=true;player.replenishing=true;ammo.updateAbility(17);
    check(ammo.m_field1b0&&sounds.active==std::vector<std::pair<char,bool>>{{'R',false},{'H',false},{'A',true}},"vehicle replenish activates ammo only");
    reset();ammo.m_field1c2=true;player.giving=true;player.repaired=85;ammo.updateAbility(18);
    check(!ammo.m_field1b0&&sounds.active==std::vector<std::pair<char,bool>>{{'R',false},{'H',false},{'A',false}},"exact 15-tick expiry is excluded");
    reset();ammo.m_field1c2=true;player.giving=true;player.repaired=0xfffffff8u;ammo.updateAbility(19);
    check(!ammo.m_field1b0,"tick addition wraps unsigned before current tick comparison");
    reset();ammo.m_field1c2=true;player.giving=true;hud->afterKit=[&]{hud->kit=otherKit;};ammo.updateAbility(20);
    check(sounds.active.empty()&&otherSounds.active.size()==3,"vehicle sound owner comes from fresh second HUD kit query");
    reset();ammo.m_field1c2=true;player.giving=true;sounds.after=[&](char kind){if(kind=='R'){kit->components(gl,&otherSounds);player.giving=false;player.replenishing=true;}};ammo.updateAbility(21);
    check(sounds.active==std::vector<std::pair<char,bool>>{{'R',false},{'H',false},{'A',true}}&&otherSounds.active.empty(),"sound receiver is captured while player flags reload between setters");
    reset();ammo.m_field1c2=true;player.giving=true;hud->afterKit=[&]{hud->kit=nullptr;};ammo.updateAbility(22);
    check(ammo.m_field1b0&&sounds.active.empty(),"fresh missing kit returns after vehicle bar update");
    reset();ammo.m_field1c2=true;player.giving=true;otherKit->components(gl,nullptr);hud->afterKit=[&]{hud->kit=otherKit;};ammo.updateAbility(23);
    check(ammo.m_field1b0&&sounds.active.empty(),"fresh kit without HUD component returns without sound calls");
    reset();parent->control=nullptr;ammo.updateAbility(24);
    check(sounds.stops==1&&trace=="Pkvvqqs","exhausted raw parent chain stops captured kit sound");
    reset();glTemplate->setHasAmmoAbility(false);glTemplate->setHasRepairingAbility(true);ammo.updateAbility(25);
    check(sounds.active==std::vector<std::pair<char,bool>>{{'R',false}}&&ammo.m_field1ac==.75f,"repair type with no targets disables only repair sound and preserves bar value");
    reset();glTemplate->setHasAmmoAbility(false);glTemplate->setHasHealingAbility(true);ammo.updateAbility(26);
    check(sounds.active==std::vector<std::pair<char,bool>>{{'H',false}}&&ammo.m_field1b1,"healing type with no targets disables healing and still samples amount");
    reset();glTemplate->setHasAmmoAbility(false);ammo.updateAbility(27);
    check(sounds.active.empty()&&ammo.m_field1b1&&ammo.m_field1b8==.375f,"no ability flags retains sound state while sampling amount");
    // Do not free retained world owners through unexercised full world teardown.
    // Native weak vector ownership is exercised explicitly before their retention.
    weapon->targets.clear();
}
};
} } }
static LONG WINAPI frameException(EXCEPTION_POINTERS* info){
    auto* context=info->ContextRecord;
    std::fprintf(stderr,"[TEMP] exception %08lx at %p, trace=%s, return=%p\n",info->ExceptionRecord->ExceptionCode,
        info->ExceptionRecord->ExceptionAddress,trace.c_str(),*reinterpret_cast<void**>(context->Esp));
    std::fprintf(stderr,"[TEMP] module=%p\n",GetModuleHandle(nullptr));
    std::fflush(stderr);return EXCEPTION_EXECUTE_HANDLER;
}
int main(){SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);SetUnhandledExceptionFilter(frameException);
    if(ammoOldRegressionMain())return 1;
    HudAmmoFrameFixture::run();std::printf("HUD Ammo ability frame: %d/%d checks passed\n",checks-failures,checks);return failures?1:0;}
