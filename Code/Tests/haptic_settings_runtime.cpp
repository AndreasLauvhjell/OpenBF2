// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: HAPTIC-SETTINGS-FIXTURE]
// Actual HapticSettings owner, lifetime, defaults, script, every accessor pair, the by-level and
// by-class dispatchers, the weapon-class table and the two profile loads' early returns. The
// class manager is a typed observer with no LocalProfileManager and the HUD pointer is a dummy.
// Nothing HapticSettings owns is shadowed.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>
#include "../BF2/Game/HapticSettings.h"
#include "../BF2/Game/Main/BF2ClientSeams.h"
#include "../BF2/Core/ClassManager.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Core/Ids.h"
using namespace dice::hfe;
static unsigned checks, failures;
static void check(bool value,const char* label)
{ ++checks; if(!value){++failures;std::printf("FAIL %s\n",label);} }

namespace dice { namespace hfe {
// [TEMP] Globals this isolated executable does not link from their owning units.
ClassManager* classManager=nullptr;
EventManager* eventManager=nullptr;
hud::BF2HudManager* hudManager=nullptr;
struct HapticSettingsFixture {
    static HapticSettings& owner(IBase* base){ return *static_cast<HapticSettings*>(base); }
    static int refs(HapticSettings& h){ return h.m_refCount; }
    static const std::vector<WeaponClassEntry>& table(HapticSettings& h){ return h.m_weaponClassMap; }
};
} }
// [TEMP] A class manager without a LocalProfileManager: both profile loads return before any path is built.
struct Registry : ClassManager {
    unsigned queries=0;
    IBase* getSingleton(const std::string& name) override { ++queries; check(name=="LocalProfileManager","the loads look the profile manager up by name"); return nullptr; }
};

static unsigned countLines(const std::string& s){ unsigned n=0; for(char c:s) if(c=='\n') ++n; return n; }
static bool hasLine(const std::string& s,const char* line){ return s.find(std::string(line)+"\n")!=std::string::npos; }

int main()
{
    check(sizeof(void*)==4,"x86 fixture");
    Registry registry; classManager=&registry;

    // lifetime and identity
    IBase* object=HapticSettings::create(IID_IHapticSettings,nullptr);
    check(object!=nullptr,"factory yields the owner for its interface id");
    IHapticSettings* api=static_cast<IHapticSettings*>(object);
    HapticSettings& owner=HapticSettingsFixture::owner(object);
    check(HapticSettingsFixture::refs(owner)==1 && api->getRef()==1,"refcount starts at one and queryInterface does not retain");
    check(api->queryInterface(IID_IBase)==object && api->queryInterface(IID_IHapticSettings)==object && !api->queryInterface(0xffffffff),
        "queryInterface accepts IBase and IHapticSettings only");
    check(!HapticSettings::create(IID_IHapticSettings,reinterpret_cast<IBase*>(1)),"aggregation is refused before allocation");
    check(!HapticSettings::create(0xffffffff,nullptr),"unsupported factory id releases the fresh owner");
    check(api->getFileName()=="Haptic.con","actual filename virtual");

    // the constructor's defaults (setDefaultValues through the table)
    check(api->getForceCapX()==50.0f && api->getForceCapY()==50.0f && api->getForceCapZ()==50.0f,"force caps 50");
    { Vec3 v=api->getControlBoxLengthSOLDIER(); check(v.x==0.03f && v.y==0.03f && v.z==0.01f,"soldier control box length"); }
    { Vec3 v=api->getControlBoxStiffnessSOLDIER(); check(v.x==760.0f && v.y==760.0f && v.z==760.0f,"soldier control box stiffness 760"); }
    { Vec3 v=api->getControlBoxStiffnessLAND(); check(v.x==400.0f && v.y==400.0f && v.z==800.0f,"land control box stiffness 400/400/800"); }
    { Vec3 v=api->getControlBoxLength(); check(v.x==0.03f && v.y==0.03f && v.z==0.01f,"the unscripted control box length takes the soldier values"); }
    { Vec3 v=api->getControlBoxStiffness(); check(v.x==800.0f,"the unscripted control box stiffness is 800"); }
    { Vec3 v=api->getAimSensitivity(); check(v.x==36.0f && v.y==36.0f && v.z==36.0f,"the unscripted aim sensitivity is 36"); }
    { Vec3 v=api->getTurnSensitivityHELI(); check(v.x==2.0f && v.y==2.0f && v.z==8.0f,"helicopter turn sensitivity 2/2/8"); }
    check(api->getPhysicsSOLDIERSmoothTimeXLow()==10 && api->getPhysicsSOLDIERSmoothTimeYLow()==20 && api->getPhysicsSOLDIERSmoothTimeYMed()==800,"soldier smooth times");
    check(api->getPhysicsSOLDIERScaleYLow()==0.035f && api->getPhysicsSOLDIERScaleZLow()==0.06f && api->getPhysicsSOLDIERTransitionValueMed()==900.0f,"soldier scales and transition");
    check(api->getPhysicsAIRSmoothTimeXMed()==150 && api->getPhysicsHELISmoothTimeXLow()==400 && api->getPhysicsSEASmoothTimeZHigh()==5,"air, helicopter and sea smooth times");
    check(api->getDamageScale()==0.4f,"damage scale 0.4");
    check(api->getRecoilPunchScaleRIFLE()==40.0f && api->getRecoilPunchTimeRIFLE()==2.0f && api->getRecoilYawScaleRIFLE()==0.0f,"rifle recoil row");
    check(api->getRecoilPunchScalePISTOL()==19.0f && api->getRecoilPunchTimePISTOL()==3.0f,"pistol recoil row");
    check(api->getRecoilPunchScaleV_COAXIALGUN()==35.0f && api->getRecoilPitchScaleV_COAXIALGUN()==8.0f,"coaxial recoil row");
    check(api->getUnresolved48C()==1.0f && api->getUnresolved490()==1.0f,"the two unscripted floats default to 1.0");
    check(api->getGeneralShakeScale()==14.0f && api->getExplosionShakeScale()==4.0f && api->getSpeedShakeScale()==6.0f,"shake scales 14/4/6");
    const std::vector<WeaponClassEntry>& table=HapticSettingsFixture::table(owner);
    check(table.size()==151 && table[0].name=="usrif_m16a2" && table[0].weaponClass==0 && table[150].name=="ahe_ah1z_gun" && table[150].weaponClass==15,
        "151 weapon-class rows in shipped order");
    check(api->getWeaponClass(std::string("usrif_m16a2"))==0 && api->getWeaponClass(std::string("defibrillator"))==12
        && api->getWeaponClass(std::string("ahe_ah1z_gun"))==15 && api->getWeaponClass(std::string("nothing"))==-1,"lookups by name, -1 when absent");
    check(api->getWeaponClass(std::string("USRIF_M16A2"))==-1,"lookups match the name as given");

    // the script
    const std::string script=api->getSettingsScript();
    check(countLines(script)==298+151,"298 fixed lines (81 comments, 217 values) plus one row per weapon class");
    check(script.find("rem -------------------------\nrem ---Force Cap Variables---\nHapticSettings.setForceCapX 50\nHapticSettings.setForceCapY 50\nHapticSettings.setForceCapZ 50\n")==0,"the script opens with the force caps");
    check(hasLine(script,"HapticSettings.setControlBoxLengthSOLDIER 0.03 0.03 0.01"),"vector rows write x y z");
    check(hasLine(script,"HapticSettings.setPhysicsSOLDIERSmoothTimeYMed 800"),"int rows");
    check(hasLine(script,"HapticSettings.setRecoilPitchScaleV_COAXIALGUN 8"),"the last recoil row");
    check(hasLine(script,"rem ***MOSTLY BASED OFF OF THE ''constants.py'' FILE***"),"the weapon-table comment lines");
    check(script.find("HapticSettings.addWeaponClassMap usrif_m16a2 0\n")!=std::string::npos && script.rfind("HapticSettings.addWeaponClassMap ahe_ah1z_gun 15\n")==script.size()-std::strlen("HapticSettings.addWeaponClassMap ahe_ah1z_gun 15\n"),
        "the weapon rows close the script");
    check(script.find("HapticSettings.setControlBoxLength ")==std::string::npos && script.find("HapticSettings.setAimSensitivity ")==std::string::npos,"the unscripted fields never appear");

    // apply and the profile loads
    check(!api->apply(),"apply is false without a HUD manager");
    hudManager=reinterpret_cast<hud::BF2HudManager*>(&registry); check(api->apply(),"apply is true with one"); hudManager=nullptr;
    registry.queries=0; api->loadCurrentProfileFile(); check(registry.queries==1,"loadCurrentProfileFile asks for the profile manager once");
    check(!api->reloadCurrentProfileFile() && registry.queries==2,"reloadCurrentProfileFile is false without it");

    // the weapon-class table
    api->addWeaponClassMap(std::string("USRIF_M16A2"),7);
    check(table.size()==151 && api->getWeaponClass(std::string("usrif_m16a2"))==7,"a known name, lowercased, updates its row in place");
    api->addWeaponClassMap(std::string("New_Weapon"),3);
    check(table.size()==152 && table[151].name=="new_weapon" && table[151].weaponClass==3 && api->getWeaponClass(std::string("new_weapon"))==3,"a new name is lowercased and appended");

    // every accessor pair and dispatcher
    api->setForceCapX(1.25f); check(api->getForceCapX() == 1.25f, "ForceCapX");
    api->setForceCapY(1.25f); check(api->getForceCapY() == 1.25f, "ForceCapY");
    api->setForceCapZ(1.25f); check(api->getForceCapZ() == 1.25f, "ForceCapZ");
    api->setControlBoxLength(1.5f, -2.0f, 3.25f); { Vec3 v = api->getControlBoxLength(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "ControlBoxLength"); }
    api->setControlBoxStiffness(1.5f, -2.0f, 3.25f); { Vec3 v = api->getControlBoxStiffness(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "ControlBoxStiffness"); }
    api->setControlBoxLengthSOLDIER(1.5f, -2.0f, 3.25f); { Vec3 v = api->getControlBoxLengthSOLDIER(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "ControlBoxLengthSOLDIER"); }
    api->setControlBoxStiffnessSOLDIER(1.5f, -2.0f, 3.25f); { Vec3 v = api->getControlBoxStiffnessSOLDIER(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "ControlBoxStiffnessSOLDIER"); }
    api->setControlBoxLengthAIR(1.5f, -2.0f, 3.25f); { Vec3 v = api->getControlBoxLengthAIR(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "ControlBoxLengthAIR"); }
    api->setControlBoxStiffnessAIR(1.5f, -2.0f, 3.25f); { Vec3 v = api->getControlBoxStiffnessAIR(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "ControlBoxStiffnessAIR"); }
    api->setControlBoxLengthHELI(1.5f, -2.0f, 3.25f); { Vec3 v = api->getControlBoxLengthHELI(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "ControlBoxLengthHELI"); }
    api->setControlBoxStiffnessHELI(1.5f, -2.0f, 3.25f); { Vec3 v = api->getControlBoxStiffnessHELI(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "ControlBoxStiffnessHELI"); }
    api->setControlBoxLengthLAND(1.5f, -2.0f, 3.25f); { Vec3 v = api->getControlBoxLengthLAND(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "ControlBoxLengthLAND"); }
    api->setControlBoxStiffnessLAND(1.5f, -2.0f, 3.25f); { Vec3 v = api->getControlBoxStiffnessLAND(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "ControlBoxStiffnessLAND"); }
    api->setControlBoxLengthSEA(1.5f, -2.0f, 3.25f); { Vec3 v = api->getControlBoxLengthSEA(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "ControlBoxLengthSEA"); }
    api->setControlBoxStiffnessSEA(1.5f, -2.0f, 3.25f); { Vec3 v = api->getControlBoxStiffnessSEA(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "ControlBoxStiffnessSEA"); }
    api->setAimSensitivity(1.5f, -2.0f, 3.25f); { Vec3 v = api->getAimSensitivity(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "AimSensitivity"); }
    api->setAimSensitivityLength(1.5f, -2.0f, 3.25f); { Vec3 v = api->getAimSensitivityLength(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "AimSensitivityLength"); }
    api->setTurnSensitivity(1.5f, -2.0f, 3.25f); { Vec3 v = api->getTurnSensitivity(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "TurnSensitivity"); }
    api->setAimSensitivitySOLDIER(1.5f, -2.0f, 3.25f); { Vec3 v = api->getAimSensitivitySOLDIER(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "AimSensitivitySOLDIER"); }
    api->setAimSensitivityLengthSOLDIER(1.5f, -2.0f, 3.25f); { Vec3 v = api->getAimSensitivityLengthSOLDIER(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "AimSensitivityLengthSOLDIER"); }
    api->setTurnSensitivitySOLDIER(1.5f, -2.0f, 3.25f); { Vec3 v = api->getTurnSensitivitySOLDIER(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "TurnSensitivitySOLDIER"); }
    api->setAimSensitivityAIR(1.5f, -2.0f, 3.25f); { Vec3 v = api->getAimSensitivityAIR(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "AimSensitivityAIR"); }
    api->setAimSensitivityLengthAIR(1.5f, -2.0f, 3.25f); { Vec3 v = api->getAimSensitivityLengthAIR(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "AimSensitivityLengthAIR"); }
    api->setTurnSensitivityAIR(1.5f, -2.0f, 3.25f); { Vec3 v = api->getTurnSensitivityAIR(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "TurnSensitivityAIR"); }
    api->setAimSensitivityHELI(1.5f, -2.0f, 3.25f); { Vec3 v = api->getAimSensitivityHELI(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "AimSensitivityHELI"); }
    api->setAimSensitivityLengthHELI(1.5f, -2.0f, 3.25f); { Vec3 v = api->getAimSensitivityLengthHELI(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "AimSensitivityLengthHELI"); }
    api->setTurnSensitivityHELI(1.5f, -2.0f, 3.25f); { Vec3 v = api->getTurnSensitivityHELI(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "TurnSensitivityHELI"); }
    api->setAimSensitivityLAND(1.5f, -2.0f, 3.25f); { Vec3 v = api->getAimSensitivityLAND(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "AimSensitivityLAND"); }
    api->setAimSensitivityLengthLAND(1.5f, -2.0f, 3.25f); { Vec3 v = api->getAimSensitivityLengthLAND(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "AimSensitivityLengthLAND"); }
    api->setTurnSensitivityLAND(1.5f, -2.0f, 3.25f); { Vec3 v = api->getTurnSensitivityLAND(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "TurnSensitivityLAND"); }
    api->setAimSensitivitySEA(1.5f, -2.0f, 3.25f); { Vec3 v = api->getAimSensitivitySEA(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "AimSensitivitySEA"); }
    api->setAimSensitivityLengthSEA(1.5f, -2.0f, 3.25f); { Vec3 v = api->getAimSensitivityLengthSEA(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "AimSensitivityLengthSEA"); }
    api->setTurnSensitivitySEA(1.5f, -2.0f, 3.25f); { Vec3 v = api->getTurnSensitivitySEA(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "TurnSensitivitySEA"); }
    api->setPhysicsSOLDIERScale(1.5f, -2.0f, 3.25f); { Vec3 v = api->getPhysicsSOLDIERScale(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "PhysicsSOLDIERScale"); }
    api->setPhysicsSOLDIERScaleXLow(1.25f); check(api->getPhysicsSOLDIERScaleXLow() == 1.25f, "PhysicsSOLDIERScaleXLow");
    api->setPhysicsSOLDIERScaleYLow(1.25f); check(api->getPhysicsSOLDIERScaleYLow() == 1.25f, "PhysicsSOLDIERScaleYLow");
    api->setPhysicsSOLDIERScaleZLow(1.25f); check(api->getPhysicsSOLDIERScaleZLow() == 1.25f, "PhysicsSOLDIERScaleZLow");
    api->setPhysicsSOLDIERScaleXMed(1.25f); check(api->getPhysicsSOLDIERScaleXMed() == 1.25f, "PhysicsSOLDIERScaleXMed");
    api->setPhysicsSOLDIERScaleYMed(1.25f); check(api->getPhysicsSOLDIERScaleYMed() == 1.25f, "PhysicsSOLDIERScaleYMed");
    api->setPhysicsSOLDIERScaleZMed(1.25f); check(api->getPhysicsSOLDIERScaleZMed() == 1.25f, "PhysicsSOLDIERScaleZMed");
    api->setPhysicsSOLDIERScaleXHigh(1.25f); check(api->getPhysicsSOLDIERScaleXHigh() == 1.25f, "PhysicsSOLDIERScaleXHigh");
    api->setPhysicsSOLDIERScaleYHigh(1.25f); check(api->getPhysicsSOLDIERScaleYHigh() == 1.25f, "PhysicsSOLDIERScaleYHigh");
    api->setPhysicsSOLDIERScaleZHigh(1.25f); check(api->getPhysicsSOLDIERScaleZHigh() == 1.25f, "PhysicsSOLDIERScaleZHigh");
    api->setPhysicsSOLDIERTransitionValue(0, 2.5f); check(api->getPhysicsSOLDIERTransitionValueMed() == 2.5f && api->getPhysicsSOLDIERTransitionValue(0) == 2.5f, "PhysicsSOLDIERTransitionValue level 0");
    api->setPhysicsSOLDIERTransitionValueMed(3.5f); check(api->getPhysicsSOLDIERTransitionValue(0) == 3.5f, "PhysicsSOLDIERTransitionValue level 0 reads back");
    api->setPhysicsSOLDIERTransitionValue(1, 2.5f); check(api->getPhysicsSOLDIERTransitionValueHigh() == 2.5f && api->getPhysicsSOLDIERTransitionValue(1) == 2.5f, "PhysicsSOLDIERTransitionValue level 1");
    api->setPhysicsSOLDIERTransitionValueHigh(3.5f); check(api->getPhysicsSOLDIERTransitionValue(1) == 3.5f, "PhysicsSOLDIERTransitionValue level 1 reads back");
    api->setPhysicsSOLDIERTransitionValue(9, 2.5f); check(api->getPhysicsSOLDIERTransitionValue(9) == 0.0f, "PhysicsSOLDIERTransitionValue level 9 ignored and reads 0.0f");
    api->setPhysicsSOLDIERTransitionValueMed(1.25f); check(api->getPhysicsSOLDIERTransitionValueMed() == 1.25f, "PhysicsSOLDIERTransitionValueMed");
    api->setPhysicsSOLDIERTransitionValueHigh(1.25f); check(api->getPhysicsSOLDIERTransitionValueHigh() == 1.25f, "PhysicsSOLDIERTransitionValueHigh");
    api->setPhysicsSOLDIERSmoothTimeX(0, 11); check(api->getPhysicsSOLDIERSmoothTimeXLow() == 11 && api->getPhysicsSOLDIERSmoothTimeX(0) == 11, "PhysicsSOLDIERSmoothTimeX level 0");
    api->setPhysicsSOLDIERSmoothTimeXLow(13); check(api->getPhysicsSOLDIERSmoothTimeX(0) == 13, "PhysicsSOLDIERSmoothTimeX level 0 reads back");
    api->setPhysicsSOLDIERSmoothTimeX(1, 11); check(api->getPhysicsSOLDIERSmoothTimeXMed() == 11 && api->getPhysicsSOLDIERSmoothTimeX(1) == 11, "PhysicsSOLDIERSmoothTimeX level 1");
    api->setPhysicsSOLDIERSmoothTimeXMed(13); check(api->getPhysicsSOLDIERSmoothTimeX(1) == 13, "PhysicsSOLDIERSmoothTimeX level 1 reads back");
    api->setPhysicsSOLDIERSmoothTimeX(2, 11); check(api->getPhysicsSOLDIERSmoothTimeXHigh() == 11 && api->getPhysicsSOLDIERSmoothTimeX(2) == 11, "PhysicsSOLDIERSmoothTimeX level 2");
    api->setPhysicsSOLDIERSmoothTimeXHigh(13); check(api->getPhysicsSOLDIERSmoothTimeX(2) == 13, "PhysicsSOLDIERSmoothTimeX level 2 reads back");
    api->setPhysicsSOLDIERSmoothTimeX(9, 11); check(api->getPhysicsSOLDIERSmoothTimeX(9) == 0, "PhysicsSOLDIERSmoothTimeX level 9 ignored and reads 0");
    api->setPhysicsSOLDIERSmoothTimeY(0, 11); check(api->getPhysicsSOLDIERSmoothTimeYLow() == 11 && api->getPhysicsSOLDIERSmoothTimeY(0) == 11, "PhysicsSOLDIERSmoothTimeY level 0");
    api->setPhysicsSOLDIERSmoothTimeYLow(13); check(api->getPhysicsSOLDIERSmoothTimeY(0) == 13, "PhysicsSOLDIERSmoothTimeY level 0 reads back");
    api->setPhysicsSOLDIERSmoothTimeY(1, 11); check(api->getPhysicsSOLDIERSmoothTimeYMed() == 11 && api->getPhysicsSOLDIERSmoothTimeY(1) == 11, "PhysicsSOLDIERSmoothTimeY level 1");
    api->setPhysicsSOLDIERSmoothTimeYMed(13); check(api->getPhysicsSOLDIERSmoothTimeY(1) == 13, "PhysicsSOLDIERSmoothTimeY level 1 reads back");
    api->setPhysicsSOLDIERSmoothTimeY(2, 11); check(api->getPhysicsSOLDIERSmoothTimeYHigh() == 11 && api->getPhysicsSOLDIERSmoothTimeY(2) == 11, "PhysicsSOLDIERSmoothTimeY level 2");
    api->setPhysicsSOLDIERSmoothTimeYHigh(13); check(api->getPhysicsSOLDIERSmoothTimeY(2) == 13, "PhysicsSOLDIERSmoothTimeY level 2 reads back");
    api->setPhysicsSOLDIERSmoothTimeY(9, 11); check(api->getPhysicsSOLDIERSmoothTimeY(9) == 0, "PhysicsSOLDIERSmoothTimeY level 9 ignored and reads 0");
    api->setPhysicsSOLDIERSmoothTimeZ(0, 11); check(api->getPhysicsSOLDIERSmoothTimeZLow() == 11 && api->getPhysicsSOLDIERSmoothTimeZ(0) == 11, "PhysicsSOLDIERSmoothTimeZ level 0");
    api->setPhysicsSOLDIERSmoothTimeZLow(13); check(api->getPhysicsSOLDIERSmoothTimeZ(0) == 13, "PhysicsSOLDIERSmoothTimeZ level 0 reads back");
    api->setPhysicsSOLDIERSmoothTimeZ(1, 11); check(api->getPhysicsSOLDIERSmoothTimeZMed() == 11 && api->getPhysicsSOLDIERSmoothTimeZ(1) == 11, "PhysicsSOLDIERSmoothTimeZ level 1");
    api->setPhysicsSOLDIERSmoothTimeZMed(13); check(api->getPhysicsSOLDIERSmoothTimeZ(1) == 13, "PhysicsSOLDIERSmoothTimeZ level 1 reads back");
    api->setPhysicsSOLDIERSmoothTimeZ(2, 11); check(api->getPhysicsSOLDIERSmoothTimeZHigh() == 11 && api->getPhysicsSOLDIERSmoothTimeZ(2) == 11, "PhysicsSOLDIERSmoothTimeZ level 2");
    api->setPhysicsSOLDIERSmoothTimeZHigh(13); check(api->getPhysicsSOLDIERSmoothTimeZ(2) == 13, "PhysicsSOLDIERSmoothTimeZ level 2 reads back");
    api->setPhysicsSOLDIERSmoothTimeZ(9, 11); check(api->getPhysicsSOLDIERSmoothTimeZ(9) == 0, "PhysicsSOLDIERSmoothTimeZ level 9 ignored and reads 0");
    api->setPhysicsSOLDIERSmoothTimeXLow(7); check(api->getPhysicsSOLDIERSmoothTimeXLow() == 7, "PhysicsSOLDIERSmoothTimeXLow");
    api->setPhysicsSOLDIERSmoothTimeYLow(7); check(api->getPhysicsSOLDIERSmoothTimeYLow() == 7, "PhysicsSOLDIERSmoothTimeYLow");
    api->setPhysicsSOLDIERSmoothTimeZLow(7); check(api->getPhysicsSOLDIERSmoothTimeZLow() == 7, "PhysicsSOLDIERSmoothTimeZLow");
    api->setPhysicsSOLDIERSmoothTimeXMed(7); check(api->getPhysicsSOLDIERSmoothTimeXMed() == 7, "PhysicsSOLDIERSmoothTimeXMed");
    api->setPhysicsSOLDIERSmoothTimeYMed(7); check(api->getPhysicsSOLDIERSmoothTimeYMed() == 7, "PhysicsSOLDIERSmoothTimeYMed");
    api->setPhysicsSOLDIERSmoothTimeZMed(7); check(api->getPhysicsSOLDIERSmoothTimeZMed() == 7, "PhysicsSOLDIERSmoothTimeZMed");
    api->setPhysicsSOLDIERSmoothTimeXHigh(7); check(api->getPhysicsSOLDIERSmoothTimeXHigh() == 7, "PhysicsSOLDIERSmoothTimeXHigh");
    api->setPhysicsSOLDIERSmoothTimeYHigh(7); check(api->getPhysicsSOLDIERSmoothTimeYHigh() == 7, "PhysicsSOLDIERSmoothTimeYHigh");
    api->setPhysicsSOLDIERSmoothTimeZHigh(7); check(api->getPhysicsSOLDIERSmoothTimeZHigh() == 7, "PhysicsSOLDIERSmoothTimeZHigh");
    api->setPhysicsLANDScale(1.5f, -2.0f, 3.25f); { Vec3 v = api->getPhysicsLANDScale(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "PhysicsLANDScale"); }
    api->setPhysicsLANDScaleXLow(1.25f); check(api->getPhysicsLANDScaleXLow() == 1.25f, "PhysicsLANDScaleXLow");
    api->setPhysicsLANDScaleYLow(1.25f); check(api->getPhysicsLANDScaleYLow() == 1.25f, "PhysicsLANDScaleYLow");
    api->setPhysicsLANDScaleZLow(1.25f); check(api->getPhysicsLANDScaleZLow() == 1.25f, "PhysicsLANDScaleZLow");
    api->setPhysicsLANDScaleXMed(1.25f); check(api->getPhysicsLANDScaleXMed() == 1.25f, "PhysicsLANDScaleXMed");
    api->setPhysicsLANDScaleYMed(1.25f); check(api->getPhysicsLANDScaleYMed() == 1.25f, "PhysicsLANDScaleYMed");
    api->setPhysicsLANDScaleZMed(1.25f); check(api->getPhysicsLANDScaleZMed() == 1.25f, "PhysicsLANDScaleZMed");
    api->setPhysicsLANDScaleXHigh(1.25f); check(api->getPhysicsLANDScaleXHigh() == 1.25f, "PhysicsLANDScaleXHigh");
    api->setPhysicsLANDScaleYHigh(1.25f); check(api->getPhysicsLANDScaleYHigh() == 1.25f, "PhysicsLANDScaleYHigh");
    api->setPhysicsLANDScaleZHigh(1.25f); check(api->getPhysicsLANDScaleZHigh() == 1.25f, "PhysicsLANDScaleZHigh");
    api->setPhysicsLANDTransitionValue(0, 2.5f); check(api->getPhysicsLANDTransitionValueMed() == 2.5f && api->getPhysicsLANDTransitionValue(0) == 2.5f, "PhysicsLANDTransitionValue level 0");
    api->setPhysicsLANDTransitionValueMed(3.5f); check(api->getPhysicsLANDTransitionValue(0) == 3.5f, "PhysicsLANDTransitionValue level 0 reads back");
    api->setPhysicsLANDTransitionValue(1, 2.5f); check(api->getPhysicsLANDTransitionValueHigh() == 2.5f && api->getPhysicsLANDTransitionValue(1) == 2.5f, "PhysicsLANDTransitionValue level 1");
    api->setPhysicsLANDTransitionValueHigh(3.5f); check(api->getPhysicsLANDTransitionValue(1) == 3.5f, "PhysicsLANDTransitionValue level 1 reads back");
    api->setPhysicsLANDTransitionValue(9, 2.5f); check(api->getPhysicsLANDTransitionValue(9) == 0.0f, "PhysicsLANDTransitionValue level 9 ignored and reads 0.0f");
    api->setPhysicsLANDTransitionValueMed(1.25f); check(api->getPhysicsLANDTransitionValueMed() == 1.25f, "PhysicsLANDTransitionValueMed");
    api->setPhysicsLANDTransitionValueHigh(1.25f); check(api->getPhysicsLANDTransitionValueHigh() == 1.25f, "PhysicsLANDTransitionValueHigh");
    api->setPhysicsLANDSmoothTimeX(0, 11); check(api->getPhysicsLANDSmoothTimeXLow() == 11 && api->getPhysicsLANDSmoothTimeX(0) == 11, "PhysicsLANDSmoothTimeX level 0");
    api->setPhysicsLANDSmoothTimeXLow(13); check(api->getPhysicsLANDSmoothTimeX(0) == 13, "PhysicsLANDSmoothTimeX level 0 reads back");
    api->setPhysicsLANDSmoothTimeX(1, 11); check(api->getPhysicsLANDSmoothTimeXMed() == 11 && api->getPhysicsLANDSmoothTimeX(1) == 11, "PhysicsLANDSmoothTimeX level 1");
    api->setPhysicsLANDSmoothTimeXMed(13); check(api->getPhysicsLANDSmoothTimeX(1) == 13, "PhysicsLANDSmoothTimeX level 1 reads back");
    api->setPhysicsLANDSmoothTimeX(2, 11); check(api->getPhysicsLANDSmoothTimeXHigh() == 11 && api->getPhysicsLANDSmoothTimeX(2) == 11, "PhysicsLANDSmoothTimeX level 2");
    api->setPhysicsLANDSmoothTimeXHigh(13); check(api->getPhysicsLANDSmoothTimeX(2) == 13, "PhysicsLANDSmoothTimeX level 2 reads back");
    api->setPhysicsLANDSmoothTimeX(9, 11); check(api->getPhysicsLANDSmoothTimeX(9) == 0, "PhysicsLANDSmoothTimeX level 9 ignored and reads 0");
    api->setPhysicsLANDSmoothTimeY(0, 11); check(api->getPhysicsLANDSmoothTimeYLow() == 11 && api->getPhysicsLANDSmoothTimeY(0) == 11, "PhysicsLANDSmoothTimeY level 0");
    api->setPhysicsLANDSmoothTimeYLow(13); check(api->getPhysicsLANDSmoothTimeY(0) == 13, "PhysicsLANDSmoothTimeY level 0 reads back");
    api->setPhysicsLANDSmoothTimeY(1, 11); check(api->getPhysicsLANDSmoothTimeYMed() == 11 && api->getPhysicsLANDSmoothTimeY(1) == 11, "PhysicsLANDSmoothTimeY level 1");
    api->setPhysicsLANDSmoothTimeYMed(13); check(api->getPhysicsLANDSmoothTimeY(1) == 13, "PhysicsLANDSmoothTimeY level 1 reads back");
    api->setPhysicsLANDSmoothTimeY(2, 11); check(api->getPhysicsLANDSmoothTimeYHigh() == 11 && api->getPhysicsLANDSmoothTimeY(2) == 11, "PhysicsLANDSmoothTimeY level 2");
    api->setPhysicsLANDSmoothTimeYHigh(13); check(api->getPhysicsLANDSmoothTimeY(2) == 13, "PhysicsLANDSmoothTimeY level 2 reads back");
    api->setPhysicsLANDSmoothTimeY(9, 11); check(api->getPhysicsLANDSmoothTimeY(9) == 0, "PhysicsLANDSmoothTimeY level 9 ignored and reads 0");
    api->setPhysicsLANDSmoothTimeZ(0, 11); check(api->getPhysicsLANDSmoothTimeZLow() == 11 && api->getPhysicsLANDSmoothTimeZ(0) == 11, "PhysicsLANDSmoothTimeZ level 0");
    api->setPhysicsLANDSmoothTimeZLow(13); check(api->getPhysicsLANDSmoothTimeZ(0) == 13, "PhysicsLANDSmoothTimeZ level 0 reads back");
    api->setPhysicsLANDSmoothTimeZ(1, 11); check(api->getPhysicsLANDSmoothTimeZMed() == 11 && api->getPhysicsLANDSmoothTimeZ(1) == 11, "PhysicsLANDSmoothTimeZ level 1");
    api->setPhysicsLANDSmoothTimeZMed(13); check(api->getPhysicsLANDSmoothTimeZ(1) == 13, "PhysicsLANDSmoothTimeZ level 1 reads back");
    api->setPhysicsLANDSmoothTimeZ(2, 11); check(api->getPhysicsLANDSmoothTimeZHigh() == 11 && api->getPhysicsLANDSmoothTimeZ(2) == 11, "PhysicsLANDSmoothTimeZ level 2");
    api->setPhysicsLANDSmoothTimeZHigh(13); check(api->getPhysicsLANDSmoothTimeZ(2) == 13, "PhysicsLANDSmoothTimeZ level 2 reads back");
    api->setPhysicsLANDSmoothTimeZ(9, 11); check(api->getPhysicsLANDSmoothTimeZ(9) == 0, "PhysicsLANDSmoothTimeZ level 9 ignored and reads 0");
    api->setPhysicsLANDSmoothTimeXLow(7); check(api->getPhysicsLANDSmoothTimeXLow() == 7, "PhysicsLANDSmoothTimeXLow");
    api->setPhysicsLANDSmoothTimeYLow(7); check(api->getPhysicsLANDSmoothTimeYLow() == 7, "PhysicsLANDSmoothTimeYLow");
    api->setPhysicsLANDSmoothTimeZLow(7); check(api->getPhysicsLANDSmoothTimeZLow() == 7, "PhysicsLANDSmoothTimeZLow");
    api->setPhysicsLANDSmoothTimeXMed(7); check(api->getPhysicsLANDSmoothTimeXMed() == 7, "PhysicsLANDSmoothTimeXMed");
    api->setPhysicsLANDSmoothTimeYMed(7); check(api->getPhysicsLANDSmoothTimeYMed() == 7, "PhysicsLANDSmoothTimeYMed");
    api->setPhysicsLANDSmoothTimeZMed(7); check(api->getPhysicsLANDSmoothTimeZMed() == 7, "PhysicsLANDSmoothTimeZMed");
    api->setPhysicsLANDSmoothTimeXHigh(7); check(api->getPhysicsLANDSmoothTimeXHigh() == 7, "PhysicsLANDSmoothTimeXHigh");
    api->setPhysicsLANDSmoothTimeYHigh(7); check(api->getPhysicsLANDSmoothTimeYHigh() == 7, "PhysicsLANDSmoothTimeYHigh");
    api->setPhysicsLANDSmoothTimeZHigh(7); check(api->getPhysicsLANDSmoothTimeZHigh() == 7, "PhysicsLANDSmoothTimeZHigh");
    api->setPhysicsAIRScale(1.5f, -2.0f, 3.25f); { Vec3 v = api->getPhysicsAIRScale(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "PhysicsAIRScale"); }
    api->setPhysicsAIRScaleXLow(1.25f); check(api->getPhysicsAIRScaleXLow() == 1.25f, "PhysicsAIRScaleXLow");
    api->setPhysicsAIRScaleYLow(1.25f); check(api->getPhysicsAIRScaleYLow() == 1.25f, "PhysicsAIRScaleYLow");
    api->setPhysicsAIRScaleZLow(1.25f); check(api->getPhysicsAIRScaleZLow() == 1.25f, "PhysicsAIRScaleZLow");
    api->setPhysicsAIRScaleXMed(1.25f); check(api->getPhysicsAIRScaleXMed() == 1.25f, "PhysicsAIRScaleXMed");
    api->setPhysicsAIRScaleYMed(1.25f); check(api->getPhysicsAIRScaleYMed() == 1.25f, "PhysicsAIRScaleYMed");
    api->setPhysicsAIRScaleZMed(1.25f); check(api->getPhysicsAIRScaleZMed() == 1.25f, "PhysicsAIRScaleZMed");
    api->setPhysicsAIRScaleXHigh(1.25f); check(api->getPhysicsAIRScaleXHigh() == 1.25f, "PhysicsAIRScaleXHigh");
    api->setPhysicsAIRScaleYHigh(1.25f); check(api->getPhysicsAIRScaleYHigh() == 1.25f, "PhysicsAIRScaleYHigh");
    api->setPhysicsAIRScaleZHigh(1.25f); check(api->getPhysicsAIRScaleZHigh() == 1.25f, "PhysicsAIRScaleZHigh");
    api->setPhysicsAIRTransitionValue(0, 2.5f); check(api->getPhysicsAIRTransitionValueMed() == 2.5f && api->getPhysicsAIRTransitionValue(0) == 2.5f, "PhysicsAIRTransitionValue level 0");
    api->setPhysicsAIRTransitionValueMed(3.5f); check(api->getPhysicsAIRTransitionValue(0) == 3.5f, "PhysicsAIRTransitionValue level 0 reads back");
    api->setPhysicsAIRTransitionValue(1, 2.5f); check(api->getPhysicsAIRTransitionValueHigh() == 2.5f && api->getPhysicsAIRTransitionValue(1) == 2.5f, "PhysicsAIRTransitionValue level 1");
    api->setPhysicsAIRTransitionValueHigh(3.5f); check(api->getPhysicsAIRTransitionValue(1) == 3.5f, "PhysicsAIRTransitionValue level 1 reads back");
    api->setPhysicsAIRTransitionValue(9, 2.5f); check(api->getPhysicsAIRTransitionValue(9) == 0.0f, "PhysicsAIRTransitionValue level 9 ignored and reads 0.0f");
    api->setPhysicsAIRTransitionValueMed(1.25f); check(api->getPhysicsAIRTransitionValueMed() == 1.25f, "PhysicsAIRTransitionValueMed");
    api->setPhysicsAIRTransitionValueHigh(1.25f); check(api->getPhysicsAIRTransitionValueHigh() == 1.25f, "PhysicsAIRTransitionValueHigh");
    api->setPhysicsAIRSmoothTimeX(0, 11); check(api->getPhysicsAIRSmoothTimeXLow() == 11 && api->getPhysicsAIRSmoothTimeX(0) == 11, "PhysicsAIRSmoothTimeX level 0");
    api->setPhysicsAIRSmoothTimeXLow(13); check(api->getPhysicsAIRSmoothTimeX(0) == 13, "PhysicsAIRSmoothTimeX level 0 reads back");
    api->setPhysicsAIRSmoothTimeX(1, 11); check(api->getPhysicsAIRSmoothTimeXMed() == 11 && api->getPhysicsAIRSmoothTimeX(1) == 11, "PhysicsAIRSmoothTimeX level 1");
    api->setPhysicsAIRSmoothTimeXMed(13); check(api->getPhysicsAIRSmoothTimeX(1) == 13, "PhysicsAIRSmoothTimeX level 1 reads back");
    api->setPhysicsAIRSmoothTimeX(2, 11); check(api->getPhysicsAIRSmoothTimeXHigh() == 11 && api->getPhysicsAIRSmoothTimeX(2) == 11, "PhysicsAIRSmoothTimeX level 2");
    api->setPhysicsAIRSmoothTimeXHigh(13); check(api->getPhysicsAIRSmoothTimeX(2) == 13, "PhysicsAIRSmoothTimeX level 2 reads back");
    api->setPhysicsAIRSmoothTimeX(9, 11); check(api->getPhysicsAIRSmoothTimeX(9) == 0, "PhysicsAIRSmoothTimeX level 9 ignored and reads 0");
    api->setPhysicsAIRSmoothTimeY(0, 11); check(api->getPhysicsAIRSmoothTimeYLow() == 11 && api->getPhysicsAIRSmoothTimeY(0) == 11, "PhysicsAIRSmoothTimeY level 0");
    api->setPhysicsAIRSmoothTimeYLow(13); check(api->getPhysicsAIRSmoothTimeY(0) == 13, "PhysicsAIRSmoothTimeY level 0 reads back");
    api->setPhysicsAIRSmoothTimeY(1, 11); check(api->getPhysicsAIRSmoothTimeYMed() == 11 && api->getPhysicsAIRSmoothTimeY(1) == 11, "PhysicsAIRSmoothTimeY level 1");
    api->setPhysicsAIRSmoothTimeYMed(13); check(api->getPhysicsAIRSmoothTimeY(1) == 13, "PhysicsAIRSmoothTimeY level 1 reads back");
    api->setPhysicsAIRSmoothTimeY(2, 11); check(api->getPhysicsAIRSmoothTimeYHigh() == 11 && api->getPhysicsAIRSmoothTimeY(2) == 11, "PhysicsAIRSmoothTimeY level 2");
    api->setPhysicsAIRSmoothTimeYHigh(13); check(api->getPhysicsAIRSmoothTimeY(2) == 13, "PhysicsAIRSmoothTimeY level 2 reads back");
    api->setPhysicsAIRSmoothTimeY(9, 11); check(api->getPhysicsAIRSmoothTimeY(9) == 0, "PhysicsAIRSmoothTimeY level 9 ignored and reads 0");
    api->setPhysicsAIRSmoothTimeZ(0, 11); check(api->getPhysicsAIRSmoothTimeZLow() == 11 && api->getPhysicsAIRSmoothTimeZ(0) == 11, "PhysicsAIRSmoothTimeZ level 0");
    api->setPhysicsAIRSmoothTimeZLow(13); check(api->getPhysicsAIRSmoothTimeZ(0) == 13, "PhysicsAIRSmoothTimeZ level 0 reads back");
    api->setPhysicsAIRSmoothTimeZ(1, 11); check(api->getPhysicsAIRSmoothTimeZMed() == 11 && api->getPhysicsAIRSmoothTimeZ(1) == 11, "PhysicsAIRSmoothTimeZ level 1");
    api->setPhysicsAIRSmoothTimeZMed(13); check(api->getPhysicsAIRSmoothTimeZ(1) == 13, "PhysicsAIRSmoothTimeZ level 1 reads back");
    api->setPhysicsAIRSmoothTimeZ(2, 11); check(api->getPhysicsAIRSmoothTimeZHigh() == 11 && api->getPhysicsAIRSmoothTimeZ(2) == 11, "PhysicsAIRSmoothTimeZ level 2");
    api->setPhysicsAIRSmoothTimeZHigh(13); check(api->getPhysicsAIRSmoothTimeZ(2) == 13, "PhysicsAIRSmoothTimeZ level 2 reads back");
    api->setPhysicsAIRSmoothTimeZ(9, 11); check(api->getPhysicsAIRSmoothTimeZ(9) == 0, "PhysicsAIRSmoothTimeZ level 9 ignored and reads 0");
    api->setPhysicsAIRSmoothTimeXLow(7); check(api->getPhysicsAIRSmoothTimeXLow() == 7, "PhysicsAIRSmoothTimeXLow");
    api->setPhysicsAIRSmoothTimeYLow(7); check(api->getPhysicsAIRSmoothTimeYLow() == 7, "PhysicsAIRSmoothTimeYLow");
    api->setPhysicsAIRSmoothTimeZLow(7); check(api->getPhysicsAIRSmoothTimeZLow() == 7, "PhysicsAIRSmoothTimeZLow");
    api->setPhysicsAIRSmoothTimeXMed(7); check(api->getPhysicsAIRSmoothTimeXMed() == 7, "PhysicsAIRSmoothTimeXMed");
    api->setPhysicsAIRSmoothTimeYMed(7); check(api->getPhysicsAIRSmoothTimeYMed() == 7, "PhysicsAIRSmoothTimeYMed");
    api->setPhysicsAIRSmoothTimeZMed(7); check(api->getPhysicsAIRSmoothTimeZMed() == 7, "PhysicsAIRSmoothTimeZMed");
    api->setPhysicsAIRSmoothTimeXHigh(7); check(api->getPhysicsAIRSmoothTimeXHigh() == 7, "PhysicsAIRSmoothTimeXHigh");
    api->setPhysicsAIRSmoothTimeYHigh(7); check(api->getPhysicsAIRSmoothTimeYHigh() == 7, "PhysicsAIRSmoothTimeYHigh");
    api->setPhysicsAIRSmoothTimeZHigh(7); check(api->getPhysicsAIRSmoothTimeZHigh() == 7, "PhysicsAIRSmoothTimeZHigh");
    api->setPhysicsHELIScale(1.5f, -2.0f, 3.25f); { Vec3 v = api->getPhysicsHELIScale(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "PhysicsHELIScale"); }
    api->setPhysicsHELIScaleXLow(1.25f); check(api->getPhysicsHELIScaleXLow() == 1.25f, "PhysicsHELIScaleXLow");
    api->setPhysicsHELIScaleYLow(1.25f); check(api->getPhysicsHELIScaleYLow() == 1.25f, "PhysicsHELIScaleYLow");
    api->setPhysicsHELIScaleZLow(1.25f); check(api->getPhysicsHELIScaleZLow() == 1.25f, "PhysicsHELIScaleZLow");
    api->setPhysicsHELIScaleXMed(1.25f); check(api->getPhysicsHELIScaleXMed() == 1.25f, "PhysicsHELIScaleXMed");
    api->setPhysicsHELIScaleYMed(1.25f); check(api->getPhysicsHELIScaleYMed() == 1.25f, "PhysicsHELIScaleYMed");
    api->setPhysicsHELIScaleZMed(1.25f); check(api->getPhysicsHELIScaleZMed() == 1.25f, "PhysicsHELIScaleZMed");
    api->setPhysicsHELIScaleXHigh(1.25f); check(api->getPhysicsHELIScaleXHigh() == 1.25f, "PhysicsHELIScaleXHigh");
    api->setPhysicsHELIScaleYHigh(1.25f); check(api->getPhysicsHELIScaleYHigh() == 1.25f, "PhysicsHELIScaleYHigh");
    api->setPhysicsHELIScaleZHigh(1.25f); check(api->getPhysicsHELIScaleZHigh() == 1.25f, "PhysicsHELIScaleZHigh");
    api->setPhysicsHELITransitionValue(0, 2.5f); check(api->getPhysicsHELITransitionValueMed() == 2.5f && api->getPhysicsHELITransitionValue(0) == 2.5f, "PhysicsHELITransitionValue level 0");
    api->setPhysicsHELITransitionValueMed(3.5f); check(api->getPhysicsHELITransitionValue(0) == 3.5f, "PhysicsHELITransitionValue level 0 reads back");
    api->setPhysicsHELITransitionValue(1, 2.5f); check(api->getPhysicsHELITransitionValueHigh() == 2.5f && api->getPhysicsHELITransitionValue(1) == 2.5f, "PhysicsHELITransitionValue level 1");
    api->setPhysicsHELITransitionValueHigh(3.5f); check(api->getPhysicsHELITransitionValue(1) == 3.5f, "PhysicsHELITransitionValue level 1 reads back");
    api->setPhysicsHELITransitionValue(9, 2.5f); check(api->getPhysicsHELITransitionValue(9) == 0.0f, "PhysicsHELITransitionValue level 9 ignored and reads 0.0f");
    api->setPhysicsHELITransitionValueMed(1.25f); check(api->getPhysicsHELITransitionValueMed() == 1.25f, "PhysicsHELITransitionValueMed");
    api->setPhysicsHELITransitionValueHigh(1.25f); check(api->getPhysicsHELITransitionValueHigh() == 1.25f, "PhysicsHELITransitionValueHigh");
    api->setPhysicsHELISmoothTimeX(0, 11); check(api->getPhysicsHELISmoothTimeXLow() == 11 && api->getPhysicsHELISmoothTimeX(0) == 11, "PhysicsHELISmoothTimeX level 0");
    api->setPhysicsHELISmoothTimeXLow(13); check(api->getPhysicsHELISmoothTimeX(0) == 13, "PhysicsHELISmoothTimeX level 0 reads back");
    api->setPhysicsHELISmoothTimeX(1, 11); check(api->getPhysicsHELISmoothTimeXMed() == 11 && api->getPhysicsHELISmoothTimeX(1) == 11, "PhysicsHELISmoothTimeX level 1");
    api->setPhysicsHELISmoothTimeXMed(13); check(api->getPhysicsHELISmoothTimeX(1) == 13, "PhysicsHELISmoothTimeX level 1 reads back");
    api->setPhysicsHELISmoothTimeX(2, 11); check(api->getPhysicsHELISmoothTimeXHigh() == 11 && api->getPhysicsHELISmoothTimeX(2) == 11, "PhysicsHELISmoothTimeX level 2");
    api->setPhysicsHELISmoothTimeXHigh(13); check(api->getPhysicsHELISmoothTimeX(2) == 13, "PhysicsHELISmoothTimeX level 2 reads back");
    api->setPhysicsHELISmoothTimeX(9, 11); check(api->getPhysicsHELISmoothTimeX(9) == 0, "PhysicsHELISmoothTimeX level 9 ignored and reads 0");
    api->setPhysicsHELISmoothTimeY(0, 11); check(api->getPhysicsHELISmoothTimeYLow() == 11 && api->getPhysicsHELISmoothTimeY(0) == 11, "PhysicsHELISmoothTimeY level 0");
    api->setPhysicsHELISmoothTimeYLow(13); check(api->getPhysicsHELISmoothTimeY(0) == 13, "PhysicsHELISmoothTimeY level 0 reads back");
    api->setPhysicsHELISmoothTimeY(1, 11); check(api->getPhysicsHELISmoothTimeYMed() == 11 && api->getPhysicsHELISmoothTimeY(1) == 11, "PhysicsHELISmoothTimeY level 1");
    api->setPhysicsHELISmoothTimeYMed(13); check(api->getPhysicsHELISmoothTimeY(1) == 13, "PhysicsHELISmoothTimeY level 1 reads back");
    api->setPhysicsHELISmoothTimeY(2, 11); check(api->getPhysicsHELISmoothTimeYHigh() == 11 && api->getPhysicsHELISmoothTimeY(2) == 11, "PhysicsHELISmoothTimeY level 2");
    api->setPhysicsHELISmoothTimeYHigh(13); check(api->getPhysicsHELISmoothTimeY(2) == 13, "PhysicsHELISmoothTimeY level 2 reads back");
    api->setPhysicsHELISmoothTimeY(9, 11); check(api->getPhysicsHELISmoothTimeY(9) == 0, "PhysicsHELISmoothTimeY level 9 ignored and reads 0");
    api->setPhysicsHELISmoothTimeZ(0, 11); check(api->getPhysicsHELISmoothTimeZLow() == 11 && api->getPhysicsHELISmoothTimeZ(0) == 11, "PhysicsHELISmoothTimeZ level 0");
    api->setPhysicsHELISmoothTimeZLow(13); check(api->getPhysicsHELISmoothTimeZ(0) == 13, "PhysicsHELISmoothTimeZ level 0 reads back");
    api->setPhysicsHELISmoothTimeZ(1, 11); check(api->getPhysicsHELISmoothTimeZMed() == 11 && api->getPhysicsHELISmoothTimeZ(1) == 11, "PhysicsHELISmoothTimeZ level 1");
    api->setPhysicsHELISmoothTimeZMed(13); check(api->getPhysicsHELISmoothTimeZ(1) == 13, "PhysicsHELISmoothTimeZ level 1 reads back");
    api->setPhysicsHELISmoothTimeZ(2, 11); check(api->getPhysicsHELISmoothTimeZHigh() == 11 && api->getPhysicsHELISmoothTimeZ(2) == 11, "PhysicsHELISmoothTimeZ level 2");
    api->setPhysicsHELISmoothTimeZHigh(13); check(api->getPhysicsHELISmoothTimeZ(2) == 13, "PhysicsHELISmoothTimeZ level 2 reads back");
    api->setPhysicsHELISmoothTimeZ(9, 11); check(api->getPhysicsHELISmoothTimeZ(9) == 0, "PhysicsHELISmoothTimeZ level 9 ignored and reads 0");
    api->setPhysicsHELISmoothTimeXLow(7); check(api->getPhysicsHELISmoothTimeXLow() == 7, "PhysicsHELISmoothTimeXLow");
    api->setPhysicsHELISmoothTimeYLow(7); check(api->getPhysicsHELISmoothTimeYLow() == 7, "PhysicsHELISmoothTimeYLow");
    api->setPhysicsHELISmoothTimeZLow(7); check(api->getPhysicsHELISmoothTimeZLow() == 7, "PhysicsHELISmoothTimeZLow");
    api->setPhysicsHELISmoothTimeXMed(7); check(api->getPhysicsHELISmoothTimeXMed() == 7, "PhysicsHELISmoothTimeXMed");
    api->setPhysicsHELISmoothTimeYMed(7); check(api->getPhysicsHELISmoothTimeYMed() == 7, "PhysicsHELISmoothTimeYMed");
    api->setPhysicsHELISmoothTimeZMed(7); check(api->getPhysicsHELISmoothTimeZMed() == 7, "PhysicsHELISmoothTimeZMed");
    api->setPhysicsHELISmoothTimeXHigh(7); check(api->getPhysicsHELISmoothTimeXHigh() == 7, "PhysicsHELISmoothTimeXHigh");
    api->setPhysicsHELISmoothTimeYHigh(7); check(api->getPhysicsHELISmoothTimeYHigh() == 7, "PhysicsHELISmoothTimeYHigh");
    api->setPhysicsHELISmoothTimeZHigh(7); check(api->getPhysicsHELISmoothTimeZHigh() == 7, "PhysicsHELISmoothTimeZHigh");
    api->setPhysicsSEAScale(1.5f, -2.0f, 3.25f); { Vec3 v = api->getPhysicsSEAScale(); check(v.x == 1.5f && v.y == -2.0f && v.z == 3.25f, "PhysicsSEAScale"); }
    api->setPhysicsSEAScaleXLow(1.25f); check(api->getPhysicsSEAScaleXLow() == 1.25f, "PhysicsSEAScaleXLow");
    api->setPhysicsSEAScaleYLow(1.25f); check(api->getPhysicsSEAScaleYLow() == 1.25f, "PhysicsSEAScaleYLow");
    api->setPhysicsSEAScaleZLow(1.25f); check(api->getPhysicsSEAScaleZLow() == 1.25f, "PhysicsSEAScaleZLow");
    api->setPhysicsSEAScaleXMed(1.25f); check(api->getPhysicsSEAScaleXMed() == 1.25f, "PhysicsSEAScaleXMed");
    api->setPhysicsSEAScaleYMed(1.25f); check(api->getPhysicsSEAScaleYMed() == 1.25f, "PhysicsSEAScaleYMed");
    api->setPhysicsSEAScaleZMed(1.25f); check(api->getPhysicsSEAScaleZMed() == 1.25f, "PhysicsSEAScaleZMed");
    api->setPhysicsSEAScaleXHigh(1.25f); check(api->getPhysicsSEAScaleXHigh() == 1.25f, "PhysicsSEAScaleXHigh");
    api->setPhysicsSEAScaleYHigh(1.25f); check(api->getPhysicsSEAScaleYHigh() == 1.25f, "PhysicsSEAScaleYHigh");
    api->setPhysicsSEAScaleZHigh(1.25f); check(api->getPhysicsSEAScaleZHigh() == 1.25f, "PhysicsSEAScaleZHigh");
    api->setPhysicsSEATransitionValue(0, 2.5f); check(api->getPhysicsSEATransitionValueMed() == 2.5f && api->getPhysicsSEATransitionValue(0) == 2.5f, "PhysicsSEATransitionValue level 0");
    api->setPhysicsSEATransitionValueMed(3.5f); check(api->getPhysicsSEATransitionValue(0) == 3.5f, "PhysicsSEATransitionValue level 0 reads back");
    api->setPhysicsSEATransitionValue(1, 2.5f); check(api->getPhysicsSEATransitionValueHigh() == 2.5f && api->getPhysicsSEATransitionValue(1) == 2.5f, "PhysicsSEATransitionValue level 1");
    api->setPhysicsSEATransitionValueHigh(3.5f); check(api->getPhysicsSEATransitionValue(1) == 3.5f, "PhysicsSEATransitionValue level 1 reads back");
    api->setPhysicsSEATransitionValue(9, 2.5f); check(api->getPhysicsSEATransitionValue(9) == 0.0f, "PhysicsSEATransitionValue level 9 ignored and reads 0.0f");
    api->setPhysicsSEATransitionValueMed(1.25f); check(api->getPhysicsSEATransitionValueMed() == 1.25f, "PhysicsSEATransitionValueMed");
    api->setPhysicsSEATransitionValueHigh(1.25f); check(api->getPhysicsSEATransitionValueHigh() == 1.25f, "PhysicsSEATransitionValueHigh");
    api->setPhysicsSEASmoothTimeX(0, 11); check(api->getPhysicsSEASmoothTimeXLow() == 11 && api->getPhysicsSEASmoothTimeX(0) == 11, "PhysicsSEASmoothTimeX level 0");
    api->setPhysicsSEASmoothTimeXLow(13); check(api->getPhysicsSEASmoothTimeX(0) == 13, "PhysicsSEASmoothTimeX level 0 reads back");
    api->setPhysicsSEASmoothTimeX(1, 11); check(api->getPhysicsSEASmoothTimeXMed() == 11 && api->getPhysicsSEASmoothTimeX(1) == 11, "PhysicsSEASmoothTimeX level 1");
    api->setPhysicsSEASmoothTimeXMed(13); check(api->getPhysicsSEASmoothTimeX(1) == 13, "PhysicsSEASmoothTimeX level 1 reads back");
    api->setPhysicsSEASmoothTimeX(2, 11); check(api->getPhysicsSEASmoothTimeXHigh() == 11 && api->getPhysicsSEASmoothTimeX(2) == 11, "PhysicsSEASmoothTimeX level 2");
    api->setPhysicsSEASmoothTimeXHigh(13); check(api->getPhysicsSEASmoothTimeX(2) == 13, "PhysicsSEASmoothTimeX level 2 reads back");
    api->setPhysicsSEASmoothTimeX(9, 11); check(api->getPhysicsSEASmoothTimeX(9) == 0, "PhysicsSEASmoothTimeX level 9 ignored and reads 0");
    api->setPhysicsSEASmoothTimeY(0, 11); check(api->getPhysicsSEASmoothTimeYLow() == 11 && api->getPhysicsSEASmoothTimeY(0) == 11, "PhysicsSEASmoothTimeY level 0");
    api->setPhysicsSEASmoothTimeYLow(13); check(api->getPhysicsSEASmoothTimeY(0) == 13, "PhysicsSEASmoothTimeY level 0 reads back");
    api->setPhysicsSEASmoothTimeY(1, 11); check(api->getPhysicsSEASmoothTimeYMed() == 11 && api->getPhysicsSEASmoothTimeY(1) == 11, "PhysicsSEASmoothTimeY level 1");
    api->setPhysicsSEASmoothTimeYMed(13); check(api->getPhysicsSEASmoothTimeY(1) == 13, "PhysicsSEASmoothTimeY level 1 reads back");
    api->setPhysicsSEASmoothTimeY(2, 11); check(api->getPhysicsSEASmoothTimeYHigh() == 11 && api->getPhysicsSEASmoothTimeY(2) == 11, "PhysicsSEASmoothTimeY level 2");
    api->setPhysicsSEASmoothTimeYHigh(13); check(api->getPhysicsSEASmoothTimeY(2) == 13, "PhysicsSEASmoothTimeY level 2 reads back");
    api->setPhysicsSEASmoothTimeY(9, 11); check(api->getPhysicsSEASmoothTimeY(9) == 0, "PhysicsSEASmoothTimeY level 9 ignored and reads 0");
    api->setPhysicsSEASmoothTimeZ(0, 11); check(api->getPhysicsSEASmoothTimeZLow() == 11 && api->getPhysicsSEASmoothTimeZ(0) == 11, "PhysicsSEASmoothTimeZ level 0");
    api->setPhysicsSEASmoothTimeZLow(13); check(api->getPhysicsSEASmoothTimeZ(0) == 13, "PhysicsSEASmoothTimeZ level 0 reads back");
    api->setPhysicsSEASmoothTimeZ(1, 11); check(api->getPhysicsSEASmoothTimeZMed() == 11 && api->getPhysicsSEASmoothTimeZ(1) == 11, "PhysicsSEASmoothTimeZ level 1");
    api->setPhysicsSEASmoothTimeZMed(13); check(api->getPhysicsSEASmoothTimeZ(1) == 13, "PhysicsSEASmoothTimeZ level 1 reads back");
    api->setPhysicsSEASmoothTimeZ(2, 11); check(api->getPhysicsSEASmoothTimeZHigh() == 11 && api->getPhysicsSEASmoothTimeZ(2) == 11, "PhysicsSEASmoothTimeZ level 2");
    api->setPhysicsSEASmoothTimeZHigh(13); check(api->getPhysicsSEASmoothTimeZ(2) == 13, "PhysicsSEASmoothTimeZ level 2 reads back");
    api->setPhysicsSEASmoothTimeZ(9, 11); check(api->getPhysicsSEASmoothTimeZ(9) == 0, "PhysicsSEASmoothTimeZ level 9 ignored and reads 0");
    api->setPhysicsSEASmoothTimeXLow(7); check(api->getPhysicsSEASmoothTimeXLow() == 7, "PhysicsSEASmoothTimeXLow");
    api->setPhysicsSEASmoothTimeYLow(7); check(api->getPhysicsSEASmoothTimeYLow() == 7, "PhysicsSEASmoothTimeYLow");
    api->setPhysicsSEASmoothTimeZLow(7); check(api->getPhysicsSEASmoothTimeZLow() == 7, "PhysicsSEASmoothTimeZLow");
    api->setPhysicsSEASmoothTimeXMed(7); check(api->getPhysicsSEASmoothTimeXMed() == 7, "PhysicsSEASmoothTimeXMed");
    api->setPhysicsSEASmoothTimeYMed(7); check(api->getPhysicsSEASmoothTimeYMed() == 7, "PhysicsSEASmoothTimeYMed");
    api->setPhysicsSEASmoothTimeZMed(7); check(api->getPhysicsSEASmoothTimeZMed() == 7, "PhysicsSEASmoothTimeZMed");
    api->setPhysicsSEASmoothTimeXHigh(7); check(api->getPhysicsSEASmoothTimeXHigh() == 7, "PhysicsSEASmoothTimeXHigh");
    api->setPhysicsSEASmoothTimeYHigh(7); check(api->getPhysicsSEASmoothTimeYHigh() == 7, "PhysicsSEASmoothTimeYHigh");
    api->setPhysicsSEASmoothTimeZHigh(7); check(api->getPhysicsSEASmoothTimeZHigh() == 7, "PhysicsSEASmoothTimeZHigh");
    api->setDamageScale(1.25f); check(api->getDamageScale() == 1.25f, "DamageScale");
    api->setRecoilPunchScaleRIFLE(1.25f); check(api->getRecoilPunchScaleRIFLE() == 1.25f, "RecoilPunchScaleRIFLE");
    api->setRecoilPunchTimeRIFLE(1.25f); check(api->getRecoilPunchTimeRIFLE() == 1.25f, "RecoilPunchTimeRIFLE");
    api->setRecoilYawScaleRIFLE(1.25f); check(api->getRecoilYawScaleRIFLE() == 1.25f, "RecoilYawScaleRIFLE");
    api->setRecoilPitchScaleRIFLE(1.25f); check(api->getRecoilPitchScaleRIFLE() == 1.25f, "RecoilPitchScaleRIFLE");
    api->setRecoilPunchScaleRIFLELAUNCHER(1.25f); check(api->getRecoilPunchScaleRIFLELAUNCHER() == 1.25f, "RecoilPunchScaleRIFLELAUNCHER");
    api->setRecoilPunchTimeRIFLELAUNCHER(1.25f); check(api->getRecoilPunchTimeRIFLELAUNCHER() == 1.25f, "RecoilPunchTimeRIFLELAUNCHER");
    api->setRecoilYawScaleRIFLELAUNCHER(1.25f); check(api->getRecoilYawScaleRIFLELAUNCHER() == 1.25f, "RecoilYawScaleRIFLELAUNCHER");
    api->setRecoilPitchScaleRIFLELAUNCHER(1.25f); check(api->getRecoilPitchScaleRIFLELAUNCHER() == 1.25f, "RecoilPitchScaleRIFLELAUNCHER");
    api->setRecoilPunchScaleCARBINE(1.25f); check(api->getRecoilPunchScaleCARBINE() == 1.25f, "RecoilPunchScaleCARBINE");
    api->setRecoilPunchTimeCARBINE(1.25f); check(api->getRecoilPunchTimeCARBINE() == 1.25f, "RecoilPunchTimeCARBINE");
    api->setRecoilYawScaleCARBINE(1.25f); check(api->getRecoilYawScaleCARBINE() == 1.25f, "RecoilYawScaleCARBINE");
    api->setRecoilPitchScaleCARBINE(1.25f); check(api->getRecoilPitchScaleCARBINE() == 1.25f, "RecoilPitchScaleCARBINE");
    api->setRecoilPunchScaleLMG(1.25f); check(api->getRecoilPunchScaleLMG() == 1.25f, "RecoilPunchScaleLMG");
    api->setRecoilPunchTimeLMG(1.25f); check(api->getRecoilPunchTimeLMG() == 1.25f, "RecoilPunchTimeLMG");
    api->setRecoilYawScaleLMG(1.25f); check(api->getRecoilYawScaleLMG() == 1.25f, "RecoilYawScaleLMG");
    api->setRecoilPitchScaleLMG(1.25f); check(api->getRecoilPitchScaleLMG() == 1.25f, "RecoilPitchScaleLMG");
    api->setRecoilPunchScaleSNIPER(1.25f); check(api->getRecoilPunchScaleSNIPER() == 1.25f, "RecoilPunchScaleSNIPER");
    api->setRecoilPunchTimeSNIPER(1.25f); check(api->getRecoilPunchTimeSNIPER() == 1.25f, "RecoilPunchTimeSNIPER");
    api->setRecoilYawScaleSNIPER(1.25f); check(api->getRecoilYawScaleSNIPER() == 1.25f, "RecoilYawScaleSNIPER");
    api->setRecoilPitchScaleSNIPER(1.25f); check(api->getRecoilPitchScaleSNIPER() == 1.25f, "RecoilPitchScaleSNIPER");
    api->setRecoilPunchScalePISTOL(1.25f); check(api->getRecoilPunchScalePISTOL() == 1.25f, "RecoilPunchScalePISTOL");
    api->setRecoilPunchTimePISTOL(1.25f); check(api->getRecoilPunchTimePISTOL() == 1.25f, "RecoilPunchTimePISTOL");
    api->setRecoilYawScalePISTOL(1.25f); check(api->getRecoilYawScalePISTOL() == 1.25f, "RecoilYawScalePISTOL");
    api->setRecoilPitchScalePISTOL(1.25f); check(api->getRecoilPitchScalePISTOL() == 1.25f, "RecoilPitchScalePISTOL");
    api->setRecoilPunchScaleLAUNCHER(1.25f); check(api->getRecoilPunchScaleLAUNCHER() == 1.25f, "RecoilPunchScaleLAUNCHER");
    api->setRecoilPunchTimeLAUNCHER(1.25f); check(api->getRecoilPunchTimeLAUNCHER() == 1.25f, "RecoilPunchTimeLAUNCHER");
    api->setRecoilYawScaleLAUNCHER(1.25f); check(api->getRecoilYawScaleLAUNCHER() == 1.25f, "RecoilYawScaleLAUNCHER");
    api->setRecoilPitchScaleLAUNCHER(1.25f); check(api->getRecoilPitchScaleLAUNCHER() == 1.25f, "RecoilPitchScaleLAUNCHER");
    api->setRecoilPunchScaleSMG(1.25f); check(api->getRecoilPunchScaleSMG() == 1.25f, "RecoilPunchScaleSMG");
    api->setRecoilPunchTimeSMG(1.25f); check(api->getRecoilPunchTimeSMG() == 1.25f, "RecoilPunchTimeSMG");
    api->setRecoilYawScaleSMG(1.25f); check(api->getRecoilYawScaleSMG() == 1.25f, "RecoilYawScaleSMG");
    api->setRecoilPitchScaleSMG(1.25f); check(api->getRecoilPitchScaleSMG() == 1.25f, "RecoilPitchScaleSMG");
    api->setRecoilPunchScaleSHOTGUN(1.25f); check(api->getRecoilPunchScaleSHOTGUN() == 1.25f, "RecoilPunchScaleSHOTGUN");
    api->setRecoilPunchTimeSHOTGUN(1.25f); check(api->getRecoilPunchTimeSHOTGUN() == 1.25f, "RecoilPunchTimeSHOTGUN");
    api->setRecoilYawScaleSHOTGUN(1.25f); check(api->getRecoilYawScaleSHOTGUN() == 1.25f, "RecoilYawScaleSHOTGUN");
    api->setRecoilPitchScaleSHOTGUN(1.25f); check(api->getRecoilPitchScaleSHOTGUN() == 1.25f, "RecoilPitchScaleSHOTGUN");
    api->setRecoilPunchScaleKNIFE(1.25f); check(api->getRecoilPunchScaleKNIFE() == 1.25f, "RecoilPunchScaleKNIFE");
    api->setRecoilPunchTimeKNIFE(1.25f); check(api->getRecoilPunchTimeKNIFE() == 1.25f, "RecoilPunchTimeKNIFE");
    api->setRecoilYawScaleKNIFE(1.25f); check(api->getRecoilYawScaleKNIFE() == 1.25f, "RecoilYawScaleKNIFE");
    api->setRecoilPitchScaleKNIFE(1.25f); check(api->getRecoilPitchScaleKNIFE() == 1.25f, "RecoilPitchScaleKNIFE");
    api->setRecoilPunchScaleDEFIB(1.25f); check(api->getRecoilPunchScaleDEFIB() == 1.25f, "RecoilPunchScaleDEFIB");
    api->setRecoilPunchTimeDEFIB(1.25f); check(api->getRecoilPunchTimeDEFIB() == 1.25f, "RecoilPunchTimeDEFIB");
    api->setRecoilYawScaleDEFIB(1.25f); check(api->getRecoilYawScaleDEFIB() == 1.25f, "RecoilYawScaleDEFIB");
    api->setRecoilPitchScaleDEFIB(1.25f); check(api->getRecoilPitchScaleDEFIB() == 1.25f, "RecoilPitchScaleDEFIB");
    api->setRecoilPunchScaleDROP(1.25f); check(api->getRecoilPunchScaleDROP() == 1.25f, "RecoilPunchScaleDROP");
    api->setRecoilPunchTimeDROP(1.25f); check(api->getRecoilPunchTimeDROP() == 1.25f, "RecoilPunchTimeDROP");
    api->setRecoilYawScaleDROP(1.25f); check(api->getRecoilYawScaleDROP() == 1.25f, "RecoilYawScaleDROP");
    api->setRecoilPitchScaleDROP(1.25f); check(api->getRecoilPitchScaleDROP() == 1.25f, "RecoilPitchScaleDROP");
    api->setRecoilPunchScaleTHROWN(1.25f); check(api->getRecoilPunchScaleTHROWN() == 1.25f, "RecoilPunchScaleTHROWN");
    api->setRecoilPunchTimeTHROWN(1.25f); check(api->getRecoilPunchTimeTHROWN() == 1.25f, "RecoilPunchTimeTHROWN");
    api->setRecoilYawScaleTHROWN(1.25f); check(api->getRecoilYawScaleTHROWN() == 1.25f, "RecoilYawScaleTHROWN");
    api->setRecoilPitchScaleTHROWN(1.25f); check(api->getRecoilPitchScaleTHROWN() == 1.25f, "RecoilPitchScaleTHROWN");
    api->setRecoilPunchScaleV_LMG(1.25f); check(api->getRecoilPunchScaleV_LMG() == 1.25f, "RecoilPunchScaleV_LMG");
    api->setRecoilPunchTimeV_LMG(1.25f); check(api->getRecoilPunchTimeV_LMG() == 1.25f, "RecoilPunchTimeV_LMG");
    api->setRecoilYawScaleV_LMG(1.25f); check(api->getRecoilYawScaleV_LMG() == 1.25f, "RecoilYawScaleV_LMG");
    api->setRecoilPitchScaleV_LMG(1.25f); check(api->getRecoilPitchScaleV_LMG() == 1.25f, "RecoilPitchScaleV_LMG");
    api->setRecoilPunchScaleV_HMG(1.25f); check(api->getRecoilPunchScaleV_HMG() == 1.25f, "RecoilPunchScaleV_HMG");
    api->setRecoilPunchTimeV_HMG(1.25f); check(api->getRecoilPunchTimeV_HMG() == 1.25f, "RecoilPunchTimeV_HMG");
    api->setRecoilYawScaleV_HMG(1.25f); check(api->getRecoilYawScaleV_HMG() == 1.25f, "RecoilYawScaleV_HMG");
    api->setRecoilPitchScaleV_HMG(1.25f); check(api->getRecoilPitchScaleV_HMG() == 1.25f, "RecoilPitchScaleV_HMG");
    api->setRecoilPunchScaleV_HELIGUN(1.25f); check(api->getRecoilPunchScaleV_HELIGUN() == 1.25f, "RecoilPunchScaleV_HELIGUN");
    api->setRecoilPunchTimeV_HELIGUN(1.25f); check(api->getRecoilPunchTimeV_HELIGUN() == 1.25f, "RecoilPunchTimeV_HELIGUN");
    api->setRecoilYawScaleV_HELIGUN(1.25f); check(api->getRecoilYawScaleV_HELIGUN() == 1.25f, "RecoilYawScaleV_HELIGUN");
    api->setRecoilPitchScaleV_HELIGUN(1.25f); check(api->getRecoilPitchScaleV_HELIGUN() == 1.25f, "RecoilPitchScaleV_HELIGUN");
    api->setRecoilPunchScaleV_AIRGUN(1.25f); check(api->getRecoilPunchScaleV_AIRGUN() == 1.25f, "RecoilPunchScaleV_AIRGUN");
    api->setRecoilPunchTimeV_AIRGUN(1.25f); check(api->getRecoilPunchTimeV_AIRGUN() == 1.25f, "RecoilPunchTimeV_AIRGUN");
    api->setRecoilYawScaleV_AIRGUN(1.25f); check(api->getRecoilYawScaleV_AIRGUN() == 1.25f, "RecoilYawScaleV_AIRGUN");
    api->setRecoilPitchScaleV_AIRGUN(1.25f); check(api->getRecoilPitchScaleV_AIRGUN() == 1.25f, "RecoilPitchScaleV_AIRGUN");
    api->setRecoilPunchScaleV_MISSILE(1.25f); check(api->getRecoilPunchScaleV_MISSILE() == 1.25f, "RecoilPunchScaleV_MISSILE");
    api->setRecoilPunchTimeV_MISSILE(1.25f); check(api->getRecoilPunchTimeV_MISSILE() == 1.25f, "RecoilPunchTimeV_MISSILE");
    api->setRecoilYawScaleV_MISSILE(1.25f); check(api->getRecoilYawScaleV_MISSILE() == 1.25f, "RecoilYawScaleV_MISSILE");
    api->setRecoilPitchScaleV_MISSILE(1.25f); check(api->getRecoilPitchScaleV_MISSILE() == 1.25f, "RecoilPitchScaleV_MISSILE");
    api->setRecoilPunchScaleV_ARMORCANNON(1.25f); check(api->getRecoilPunchScaleV_ARMORCANNON() == 1.25f, "RecoilPunchScaleV_ARMORCANNON");
    api->setRecoilPunchTimeV_ARMORCANNON(1.25f); check(api->getRecoilPunchTimeV_ARMORCANNON() == 1.25f, "RecoilPunchTimeV_ARMORCANNON");
    api->setRecoilYawScaleV_ARMORCANNON(1.25f); check(api->getRecoilYawScaleV_ARMORCANNON() == 1.25f, "RecoilYawScaleV_ARMORCANNON");
    api->setRecoilPitchScaleV_ARMORCANNON(1.25f); check(api->getRecoilPitchScaleV_ARMORCANNON() == 1.25f, "RecoilPitchScaleV_ARMORCANNON");
    api->setRecoilPunchScaleV_COAXIALGUN(1.25f); check(api->getRecoilPunchScaleV_COAXIALGUN() == 1.25f, "RecoilPunchScaleV_COAXIALGUN");
    api->setRecoilPunchTimeV_COAXIALGUN(1.25f); check(api->getRecoilPunchTimeV_COAXIALGUN() == 1.25f, "RecoilPunchTimeV_COAXIALGUN");
    api->setRecoilYawScaleV_COAXIALGUN(1.25f); check(api->getRecoilYawScaleV_COAXIALGUN() == 1.25f, "RecoilYawScaleV_COAXIALGUN");
    api->setRecoilPitchScaleV_COAXIALGUN(1.25f); check(api->getRecoilPitchScaleV_COAXIALGUN() == 1.25f, "RecoilPitchScaleV_COAXIALGUN");
    api->setUnresolved48C(1.25f); check(api->getUnresolved48C() == 1.25f, "Unresolved48C");
    api->setUnresolved490(1.25f); check(api->getUnresolved490() == 1.25f, "Unresolved490");
    api->setGeneralShakeScale(1.25f); check(api->getGeneralShakeScale() == 1.25f, "GeneralShakeScale");
    api->setExplosionShakeScale(1.25f); check(api->getExplosionShakeScale() == 1.25f, "ExplosionShakeScale");
    api->setSpeedShakeScale(1.25f); check(api->getSpeedShakeScale() == 1.25f, "SpeedShakeScale");

    // by-class dispatch
    api->setRecoilPunchScale(0,1.0f); api->setRecoilPunchScale(5,2.0f); api->setRecoilPunchScale(10,3.0f); api->setRecoilPunchScale(106,4.0f); api->setRecoilPunchScale(50,5.0f);
    check(api->getRecoilPunchScaleRIFLE()==1.0f && api->getRecoilPunchScalePISTOL()==3.0f && api->getRecoilPunchScaleV_COAXIALGUN()==4.0f,
        "class setters: 0 rifle, 5 and 10 the pistol row, 106 the coaxial row, 50 ignored");
    check(api->getRecoilPunchScale(0)==1.0f && api->getRecoilPunchScale(5)==3.0f && api->getRecoilPunchScale(50)==3.0f && api->getRecoilPunchScale(106)==4.0f,
        "class getters: unlisted ids read the pistol row");
    api->setRecoilPitchScale(12,9.0f); check(api->getRecoilPitchScaleTHROWN()==9.0f && api->getRecoilPitchScale(12)==9.0f,"class 12 is the THROWN row");
    api->setRecoilYawScale(11,8.0f); check(api->getRecoilYawScaleDROP()==8.0f,"class 11 is the DROP row");
    api->setRecoilPunchTime(9,7.0f); check(api->getRecoilPunchTimeKNIFE()==7.0f && api->getRecoilPunchTime(9)==7.0f,"class 9 is the KNIFE row");

    // setDefaults restores everything
    api->setDefaults();
    check(api->getForceCapX()==50.0f && api->getRecoilPunchScaleRIFLE()==40.0f && api->getPhysicsSEASmoothTimeZHigh()==5 && api->getGeneralShakeScale()==14.0f,"setDefaults rewrites the values");
    check(table.size()==151 && api->getWeaponClass(std::string("usrif_m16a2"))==0 && api->getWeaponClass(std::string("new_weapon"))==-1,"setDefaults rebuilds the weapon table");
    check(api->getSettingsScript()==script,"the script is the constructor's again");

    api->addRef();
    check(api->release()==1,"non-final release keeps the owner");
    check(api->release()==0,"final release reports zero");
    std::printf("HapticSettings: %u/%u passed; native owner=%zu\n",checks-failures,checks,sizeof(HapticSettings));
    return failures?1:0;
}
