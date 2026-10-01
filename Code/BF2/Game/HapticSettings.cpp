// [INFERRED] path. Client-only class read from BF2.exe (table 8A7D50, 510 slots, factory 410700);
// the dedicated ELF carries only the IID/CID values (which collide with GSServerSideNameValidation's,
// see Core/Ids.h).
#include "HapticSettings.h"
#if !DEDICATED
#include "LocalProfileManager.h"          // ILocalProfileManager / LocalProfile - the profile file loads
#include "Main/BF2ClientSeams.h"          // hudManager - apply's only test
#include "../Core/Ids.h"
#include "../Core/SmartItf.h"             // classManager - the LocalProfileManager lookup
#include <algorithm>
#include <cctype>
#include <new>
#include <sstream>

namespace dice { namespace hfe {

// (45 B): table, refcount 1, the weapon-class vector zeroed, then setDefaultValues through
// the table - every other member is written there.
HapticSettings::HapticSettings() : m_refCount(1)
{
    setDefaultValues();
}

//: the weapon-class vector's rows and storage, then the free.
HapticSettings::~HapticSettings() = default;

// (66 B): the settings-factory shape - aggregation refused, 1200 retail bytes, construction
// only on a non-null allocation, unconditional queryInterface (the shipped null defect is kept).
IBase* HapticSettings::create(unsigned int iid, IBase* outer)
{
    if (outer) return nullptr;
    void* allocation = ::operator new(sizeof(HapticSettings));
    HapticSettings* owner = allocation ? new (allocation) HapticSettings : nullptr;
    IBase* result = static_cast<IBase*>(owner->queryInterface(iid));
    if (!result) owner->release();
    return result;
}
void HapticSettings::addRef() { ++m_refCount; }
int HapticSettings::getRef() { return m_refCount; }
// (26 B): decrement, destroy through the (withheld) +7F4 deleting destructor at zero.
int HapticSettings::release()
{
    const int references = --m_refCount;
    if (!references) delete this;
    return references;
}
// (36 B): IBase (dword_90D358 = 1), then IHapticSettings (dword_892CA8 = 121075).
void* HapticSettings::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IHapticSettings)
        return const_cast<HapticSettings*>(this);
    return nullptr;
}

std::string HapticSettings::getFileName() { return std::string("Haptic.con"); }

// (13,511 B): the annotated Haptic.con text - 81 rem lines, 217 value rows (vectors as
// "x y z"), then one addWeaponClassMap row per table entry. Row order and literals are the shipped
// ones; every value inserts through the stream's default float/int formatting.
std::string HapticSettings::getSettingsScript()
{
    std::stringstream stream;
    stream << "rem -------------------------" << std::endl;
    stream << "rem ---Force Cap Variables---" << std::endl;
    stream << "HapticSettings.setForceCapX " << m_forceCapX << std::endl;
    stream << "HapticSettings.setForceCapY " << m_forceCapY << std::endl;
    stream << "HapticSettings.setForceCapZ " << m_forceCapZ << std::endl;
    stream << "rem -------------------------------------" << std::endl;
    stream << "rem ---Control Box Settings (Infantry)---" << std::endl;
    stream << "HapticSettings.setControlBoxLengthSOLDIER " << m_controlBoxLengthSOLDIER.x << " " << m_controlBoxLengthSOLDIER.y << " " << m_controlBoxLengthSOLDIER.z << std::endl;
    stream << "HapticSettings.setControlBoxStiffnessSOLDIER " << m_controlBoxStiffnessSOLDIER.x << " " << m_controlBoxStiffnessSOLDIER.y << " " << m_controlBoxStiffnessSOLDIER.z << std::endl;
    stream << "rem -----------------------------------------" << std::endl;
    stream << "rem ---Control Box (Land Vehicle) Settings---" << std::endl;
    stream << "HapticSettings.setControlBoxLengthLAND " << m_controlBoxLengthLAND.x << " " << m_controlBoxLengthLAND.y << " " << m_controlBoxLengthLAND.z << std::endl;
    stream << "HapticSettings.setControlBoxStiffnessLAND " << m_controlBoxStiffnessLAND.x << " " << m_controlBoxStiffnessLAND.y << " " << m_controlBoxStiffnessLAND.z << std::endl;
    stream << "rem -------------------------------------" << std::endl;
    stream << "rem ---Control Box (Airplane) Settings---" << std::endl;
    stream << "HapticSettings.setControlBoxLengthAIR " << m_controlBoxLengthAIR.x << " " << m_controlBoxLengthAIR.y << " " << m_controlBoxLengthAIR.z << std::endl;
    stream << "HapticSettings.setControlBoxStiffnessAIR " << m_controlBoxStiffnessAIR.x << " " << m_controlBoxStiffnessAIR.y << " " << m_controlBoxStiffnessAIR.z << std::endl;
    stream << "rem ---------------------------------------" << std::endl;
    stream << "rem ---Control Box (Helicopter) Settings---" << std::endl;
    stream << "HapticSettings.setControlBoxLengthHELI " << m_controlBoxLengthHELI.x << " " << m_controlBoxLengthHELI.y << " " << m_controlBoxLengthHELI.z << std::endl;
    stream << "HapticSettings.setControlBoxStiffnessHELI " << m_controlBoxStiffnessHELI.x << " " << m_controlBoxStiffnessHELI.y << " " << m_controlBoxStiffnessHELI.z << std::endl;
    stream << "rem -------------------------------------" << std::endl;
    stream << "rem ---Control Box (Seacraft) Settings---" << std::endl;
    stream << "HapticSettings.setControlBoxLengthSEA " << m_controlBoxLengthSEA.x << " " << m_controlBoxLengthSEA.y << " " << m_controlBoxLengthSEA.z << std::endl;
    stream << "HapticSettings.setControlBoxStiffnessSEA " << m_controlBoxStiffnessSEA.x << " " << m_controlBoxStiffnessSEA.y << " " << m_controlBoxStiffnessSEA.z << std::endl;
    stream << "rem ---------------------------------------------" << std::endl;
    stream << "rem ---Control Sensitivity Settings (Infantry)---" << std::endl;
    stream << "HapticSettings.setAimSensitivitySOLDIER " << m_aimSensitivitySOLDIER.x << " " << m_aimSensitivitySOLDIER.y << " " << m_aimSensitivitySOLDIER.z << std::endl;
    stream << "HapticSettings.setAimSensitivityLengthSOLDIER " << m_aimSensitivityLengthSOLDIER.x << " " << m_aimSensitivityLengthSOLDIER.y << " " << m_aimSensitivityLengthSOLDIER.z << std::endl;
    stream << "HapticSettings.setTurnSensitivitySOLDIER " << m_turnSensitivitySOLDIER.x << " " << m_turnSensitivitySOLDIER.y << " " << m_turnSensitivitySOLDIER.z << std::endl;
    stream << "rem -------------------------------------------------" << std::endl;
    stream << "rem ---Control Sensitivity (Land Vehicle) Settings---" << std::endl;
    stream << "HapticSettings.setAimSensitivityLAND " << m_aimSensitivityLAND.x << " " << m_aimSensitivityLAND.y << " " << m_aimSensitivityLAND.z << std::endl;
    stream << "HapticSettings.setAimSensitivityLengthLAND " << m_aimSensitivityLengthLAND.x << " " << m_aimSensitivityLengthLAND.y << " " << m_aimSensitivityLengthLAND.z << std::endl;
    stream << "HapticSettings.setTurnSensitivityLAND " << m_turnSensitivityLAND.x << " " << m_turnSensitivityLAND.y << " " << m_turnSensitivityLAND.z << std::endl;
    stream << "rem ---------------------------------------------" << std::endl;
    stream << "rem ---Control Sensitivity (Airplane) Settings---" << std::endl;
    stream << "HapticSettings.setAimSensitivityAIR " << m_aimSensitivityAIR.x << " " << m_aimSensitivityAIR.y << " " << m_aimSensitivityAIR.z << std::endl;
    stream << "HapticSettings.setAimSensitivityLengthAIR " << m_aimSensitivityLengthAIR.x << " " << m_aimSensitivityLengthAIR.y << " " << m_aimSensitivityLengthAIR.z << std::endl;
    stream << "HapticSettings.setTurnSensitivityAIR " << m_turnSensitivityAIR.x << " " << m_turnSensitivityAIR.y << " " << m_turnSensitivityAIR.z << std::endl;
    stream << "rem -----------------------------------------------" << std::endl;
    stream << "rem ---Control Sensitivity (Helicopter) Settings---" << std::endl;
    stream << "HapticSettings.setAimSensitivityHELI " << m_aimSensitivityHELI.x << " " << m_aimSensitivityHELI.y << " " << m_aimSensitivityHELI.z << std::endl;
    stream << "HapticSettings.setAimSensitivityLengthHELI " << m_aimSensitivityLengthHELI.x << " " << m_aimSensitivityLengthHELI.y << " " << m_aimSensitivityLengthHELI.z << std::endl;
    stream << "HapticSettings.setTurnSensitivityHELI " << m_turnSensitivityHELI.x << " " << m_turnSensitivityHELI.y << " " << m_turnSensitivityHELI.z << std::endl;
    stream << "rem ---------------------------------------------" << std::endl;
    stream << "rem ---Control Sensitivity (Seacraft) Settings---" << std::endl;
    stream << "HapticSettings.setAimSensitivitySEA " << m_aimSensitivitySEA.x << " " << m_aimSensitivitySEA.y << " " << m_aimSensitivitySEA.z << std::endl;
    stream << "HapticSettings.setAimSensitivityLengthSEA " << m_aimSensitivityLengthSEA.x << " " << m_aimSensitivityLengthSEA.y << " " << m_aimSensitivityLengthSEA.z << std::endl;
    stream << "HapticSettings.setTurnSensitivitySEA " << m_turnSensitivitySEA.x << " " << m_turnSensitivitySEA.y << " " << m_turnSensitivitySEA.z << std::endl;
    stream << "rem -----------------------------" << std::endl;
    stream << "rem ---SOLDIER Physics Settings---" << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERScale " << m_physicsSOLDIERScale.x << " " << m_physicsSOLDIERScale.y << " " << m_physicsSOLDIERScale.z << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERScaleXLow " << m_physicsSOLDIERScaleXLow << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERScaleYLow " << m_physicsSOLDIERScaleYLow << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERScaleZLow " << m_physicsSOLDIERScaleZLow << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERSmoothTimeXLow " << m_physicsSOLDIERSmoothTimeXLow << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERSmoothTimeYLow " << m_physicsSOLDIERSmoothTimeYLow << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERSmoothTimeZLow " << m_physicsSOLDIERSmoothTimeZLow << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERTransitionValueMed " << m_physicsSOLDIERTransitionValueMed << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERScaleXMed " << m_physicsSOLDIERScaleXMed << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERScaleYMed " << m_physicsSOLDIERScaleYMed << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERScaleZMed " << m_physicsSOLDIERScaleZMed << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERSmoothTimeXMed " << m_physicsSOLDIERSmoothTimeXMed << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERSmoothTimeYMed " << m_physicsSOLDIERSmoothTimeYMed << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERSmoothTimeZMed " << m_physicsSOLDIERSmoothTimeZMed << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERTransitionValueHigh " << m_physicsSOLDIERTransitionValueHigh << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERScaleXHigh " << m_physicsSOLDIERScaleXHigh << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERScaleYHigh " << m_physicsSOLDIERScaleYHigh << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERScaleZHigh " << m_physicsSOLDIERScaleZHigh << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERSmoothTimeXHigh " << m_physicsSOLDIERSmoothTimeXHigh << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERSmoothTimeYHigh " << m_physicsSOLDIERSmoothTimeYHigh << std::endl;
    stream << "HapticSettings.setPhysicsSOLDIERSmoothTimeZHigh " << m_physicsSOLDIERSmoothTimeZHigh << std::endl;
    stream << "rem -----------------------------------" << std::endl;
    stream << "rem ---Land Vehicle Physics Settings---" << std::endl;
    stream << "HapticSettings.setPhysicsLANDScale " << m_physicsLANDScale.x << " " << m_physicsLANDScale.y << " " << m_physicsLANDScale.z << std::endl;
    stream << "HapticSettings.setPhysicsLANDScaleXLow " << m_physicsLANDScaleXLow << std::endl;
    stream << "HapticSettings.setPhysicsLANDScaleYLow " << m_physicsLANDScaleYLow << std::endl;
    stream << "HapticSettings.setPhysicsLANDScaleZLow " << m_physicsLANDScaleZLow << std::endl;
    stream << "HapticSettings.setPhysicsLANDSmoothTimeXLow " << m_physicsLANDSmoothTimeXLow << std::endl;
    stream << "HapticSettings.setPhysicsLANDSmoothTimeYLow " << m_physicsLANDSmoothTimeYLow << std::endl;
    stream << "HapticSettings.setPhysicsLANDSmoothTimeZLow " << m_physicsLANDSmoothTimeZLow << std::endl;
    stream << "HapticSettings.setPhysicsLANDTransitionValueMed " << m_physicsLANDTransitionValueMed << std::endl;
    stream << "HapticSettings.setPhysicsLANDScaleXMed " << m_physicsLANDScaleXMed << std::endl;
    stream << "HapticSettings.setPhysicsLANDScaleYMed " << m_physicsLANDScaleYMed << std::endl;
    stream << "HapticSettings.setPhysicsLANDScaleZMed " << m_physicsLANDScaleZMed << std::endl;
    stream << "HapticSettings.setPhysicsLANDSmoothTimeXMed " << m_physicsLANDSmoothTimeXMed << std::endl;
    stream << "HapticSettings.setPhysicsLANDSmoothTimeYMed " << m_physicsLANDSmoothTimeYMed << std::endl;
    stream << "HapticSettings.setPhysicsLANDSmoothTimeZMed " << m_physicsLANDSmoothTimeZMed << std::endl;
    stream << "HapticSettings.setPhysicsLANDTransitionValueHigh " << m_physicsLANDTransitionValueHigh << std::endl;
    stream << "HapticSettings.setPhysicsLANDScaleXHigh " << m_physicsLANDScaleXHigh << std::endl;
    stream << "HapticSettings.setPhysicsLANDScaleYHigh " << m_physicsLANDScaleYHigh << std::endl;
    stream << "HapticSettings.setPhysicsLANDScaleZHigh " << m_physicsLANDScaleZHigh << std::endl;
    stream << "HapticSettings.setPhysicsLANDSmoothTimeXHigh " << m_physicsLANDSmoothTimeXHigh << std::endl;
    stream << "HapticSettings.setPhysicsLANDSmoothTimeYHigh " << m_physicsLANDSmoothTimeYHigh << std::endl;
    stream << "HapticSettings.setPhysicsLANDSmoothTimeZHigh " << m_physicsLANDSmoothTimeZHigh << std::endl;
    stream << "rem -------------------------------" << std::endl;
    stream << "rem ---Airplane Physics Settings---" << std::endl;
    stream << "HapticSettings.setPhysicsAIRScale " << m_physicsAIRScale.x << " " << m_physicsAIRScale.y << " " << m_physicsAIRScale.z << std::endl;
    stream << "HapticSettings.setPhysicsAIRScaleXLow " << m_physicsAIRScaleXLow << std::endl;
    stream << "HapticSettings.setPhysicsAIRScaleYLow " << m_physicsAIRScaleYLow << std::endl;
    stream << "HapticSettings.setPhysicsAIRScaleZLow " << m_physicsAIRScaleZLow << std::endl;
    stream << "HapticSettings.setPhysicsAIRSmoothTimeXLow " << m_physicsAIRSmoothTimeXLow << std::endl;
    stream << "HapticSettings.setPhysicsAIRSmoothTimeYLow " << m_physicsAIRSmoothTimeYLow << std::endl;
    stream << "HapticSettings.setPhysicsAIRSmoothTimeZLow " << m_physicsAIRSmoothTimeZLow << std::endl;
    stream << "HapticSettings.setPhysicsAIRTransitionValueMed " << m_physicsAIRTransitionValueMed << std::endl;
    stream << "HapticSettings.setPhysicsAIRScaleXMed " << m_physicsAIRScaleXMed << std::endl;
    stream << "HapticSettings.setPhysicsAIRScaleYMed " << m_physicsAIRScaleYMed << std::endl;
    stream << "HapticSettings.setPhysicsAIRScaleZMed " << m_physicsAIRScaleZMed << std::endl;
    stream << "HapticSettings.setPhysicsAIRSmoothTimeXMed " << m_physicsAIRSmoothTimeXMed << std::endl;
    stream << "HapticSettings.setPhysicsAIRSmoothTimeYMed " << m_physicsAIRSmoothTimeYMed << std::endl;
    stream << "HapticSettings.setPhysicsAIRSmoothTimeZMed " << m_physicsAIRSmoothTimeZMed << std::endl;
    stream << "HapticSettings.setPhysicsAIRTransitionValueHigh " << m_physicsAIRTransitionValueHigh << std::endl;
    stream << "HapticSettings.setPhysicsAIRScaleXHigh " << m_physicsAIRScaleXHigh << std::endl;
    stream << "HapticSettings.setPhysicsAIRScaleYHigh " << m_physicsAIRScaleYHigh << std::endl;
    stream << "HapticSettings.setPhysicsAIRScaleZHigh " << m_physicsAIRScaleZHigh << std::endl;
    stream << "HapticSettings.setPhysicsAIRSmoothTimeXHigh " << m_physicsAIRSmoothTimeXHigh << std::endl;
    stream << "HapticSettings.setPhysicsAIRSmoothTimeYHigh " << m_physicsAIRSmoothTimeYHigh << std::endl;
    stream << "HapticSettings.setPhysicsAIRSmoothTimeZHigh " << m_physicsAIRSmoothTimeZHigh << std::endl;
    stream << "rem ----------------------------------" << std::endl;
    stream << "rem ---Helicopter Physics Settings----" << std::endl;
    stream << "HapticSettings.setPhysicsHELIScale " << m_physicsHELIScale.x << " " << m_physicsHELIScale.y << " " << m_physicsHELIScale.z << std::endl;
    stream << "HapticSettings.setPhysicsHELIScaleXLow " << m_physicsHELIScaleXLow << std::endl;
    stream << "HapticSettings.setPhysicsHELIScaleYLow " << m_physicsHELIScaleYLow << std::endl;
    stream << "HapticSettings.setPhysicsHELIScaleZLow " << m_physicsHELIScaleZLow << std::endl;
    stream << "HapticSettings.setPhysicsHELISmoothTimeXLow " << m_physicsHELISmoothTimeXLow << std::endl;
    stream << "HapticSettings.setPhysicsHELISmoothTimeYLow " << m_physicsHELISmoothTimeYLow << std::endl;
    stream << "HapticSettings.setPhysicsHELISmoothTimeZLow " << m_physicsHELISmoothTimeZLow << std::endl;
    stream << "HapticSettings.setPhysicsHELITransitionValueMed " << m_physicsHELITransitionValueMed << std::endl;
    stream << "HapticSettings.setPhysicsHELIScaleXMed " << m_physicsHELIScaleXMed << std::endl;
    stream << "HapticSettings.setPhysicsHELIScaleYMed " << m_physicsHELIScaleYMed << std::endl;
    stream << "HapticSettings.setPhysicsHELIScaleZMed " << m_physicsHELIScaleZMed << std::endl;
    stream << "HapticSettings.setPhysicsHELISmoothTimeXMed " << m_physicsHELISmoothTimeXMed << std::endl;
    stream << "HapticSettings.setPhysicsHELISmoothTimeYMed " << m_physicsHELISmoothTimeYMed << std::endl;
    stream << "HapticSettings.setPhysicsHELISmoothTimeZMed " << m_physicsHELISmoothTimeZMed << std::endl;
    stream << "HapticSettings.setPhysicsHELITransitionValueHigh " << m_physicsHELITransitionValueHigh << std::endl;
    stream << "HapticSettings.setPhysicsHELIScaleXHigh " << m_physicsHELIScaleXHigh << std::endl;
    stream << "HapticSettings.setPhysicsHELIScaleYHigh " << m_physicsHELIScaleYHigh << std::endl;
    stream << "HapticSettings.setPhysicsHELIScaleZHigh " << m_physicsHELIScaleZHigh << std::endl;
    stream << "HapticSettings.setPhysicsHELISmoothTimeXHigh " << m_physicsHELISmoothTimeXHigh << std::endl;
    stream << "HapticSettings.setPhysicsHELISmoothTimeYHigh " << m_physicsHELISmoothTimeYHigh << std::endl;
    stream << "HapticSettings.setPhysicsHELISmoothTimeZHigh " << m_physicsHELISmoothTimeZHigh << std::endl;
    stream << "rem ----------------------------------" << std::endl;
    stream << "rem ---Seacraft Physics Settings------" << std::endl;
    stream << "HapticSettings.setPhysicsSEAScale " << m_physicsSEAScale.x << " " << m_physicsSEAScale.y << " " << m_physicsSEAScale.z << std::endl;
    stream << "HapticSettings.setPhysicsSEAScaleXLow " << m_physicsSEAScaleXLow << std::endl;
    stream << "HapticSettings.setPhysicsSEAScaleYLow " << m_physicsSEAScaleYLow << std::endl;
    stream << "HapticSettings.setPhysicsSEAScaleZLow " << m_physicsSEAScaleZLow << std::endl;
    stream << "HapticSettings.setPhysicsSEASmoothTimeXLow " << m_physicsSEASmoothTimeXLow << std::endl;
    stream << "HapticSettings.setPhysicsSEASmoothTimeYLow " << m_physicsSEASmoothTimeYLow << std::endl;
    stream << "HapticSettings.setPhysicsSEASmoothTimeZLow " << m_physicsSEASmoothTimeZLow << std::endl;
    stream << "HapticSettings.setPhysicsSEATransitionValueMed " << m_physicsSEATransitionValueMed << std::endl;
    stream << "HapticSettings.setPhysicsSEAScaleXMed " << m_physicsSEAScaleXMed << std::endl;
    stream << "HapticSettings.setPhysicsSEAScaleYMed " << m_physicsSEAScaleYMed << std::endl;
    stream << "HapticSettings.setPhysicsSEAScaleZMed " << m_physicsSEAScaleZMed << std::endl;
    stream << "HapticSettings.setPhysicsSEASmoothTimeXMed " << m_physicsSEASmoothTimeXMed << std::endl;
    stream << "HapticSettings.setPhysicsSEASmoothTimeYMed " << m_physicsSEASmoothTimeYMed << std::endl;
    stream << "HapticSettings.setPhysicsSEASmoothTimeZMed " << m_physicsSEASmoothTimeZMed << std::endl;
    stream << "HapticSettings.setPhysicsSEATransitionValueHigh " << m_physicsSEATransitionValueHigh << std::endl;
    stream << "HapticSettings.setPhysicsSEAScaleXHigh " << m_physicsSEAScaleXHigh << std::endl;
    stream << "HapticSettings.setPhysicsSEAScaleYHigh " << m_physicsSEAScaleYHigh << std::endl;
    stream << "HapticSettings.setPhysicsSEAScaleZHigh " << m_physicsSEAScaleZHigh << std::endl;
    stream << "HapticSettings.setPhysicsSEASmoothTimeXHigh " << m_physicsSEASmoothTimeXHigh << std::endl;
    stream << "HapticSettings.setPhysicsSEASmoothTimeYHigh " << m_physicsSEASmoothTimeYHigh << std::endl;
    stream << "HapticSettings.setPhysicsSEASmoothTimeZHigh " << m_physicsSEASmoothTimeZHigh << std::endl;
    stream << "rem ---------------------" << std::endl;
    stream << "rem ---Damage Settings---" << std::endl;
    stream << "HapticSettings.setDamageScale " << m_damageScale << std::endl;
    stream << "rem ------------------------------------" << std::endl;
    stream << "rem ---Recoil Settings (Pistol Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScalePISTOL " << m_recoilPunchScalePISTOL << std::endl;
    stream << "HapticSettings.setRecoilPunchTimePISTOL " << m_recoilPunchTimePISTOL << std::endl;
    stream << "HapticSettings.setRecoilYawScalePISTOL " << m_recoilYawScalePISTOL << std::endl;
    stream << "HapticSettings.setRecoilPitchScalePISTOL " << m_recoilPitchScalePISTOL << std::endl;
    stream << "rem -----------------------------------" << std::endl;
    stream << "rem ---Recoil Settings (Rifle Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleRIFLE " << m_recoilPunchScaleRIFLE << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeRIFLE " << m_recoilPunchTimeRIFLE << std::endl;
    stream << "HapticSettings.setRecoilYawScaleRIFLE " << m_recoilYawScaleRIFLE << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleRIFLE " << m_recoilPitchScaleRIFLE << std::endl;
    stream << "rem -------------------------------------" << std::endl;
    stream << "rem ---Recoil Settings (Carbine Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleCARBINE " << m_recoilPunchScaleCARBINE << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeCARBINE " << m_recoilPunchTimeCARBINE << std::endl;
    stream << "HapticSettings.setRecoilYawScaleCARBINE " << m_recoilYawScaleCARBINE << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleCARBINE " << m_recoilPitchScaleCARBINE << std::endl;
    stream << "rem -------------------------------------" << std::endl;
    stream << "rem ---Recoil Settings (Shotgun Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleSHOTGUN " << m_recoilPunchScaleSHOTGUN << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeSHOTGUN " << m_recoilPunchTimeSHOTGUN << std::endl;
    stream << "HapticSettings.setRecoilYawScaleSHOTGUN " << m_recoilYawScaleSHOTGUN << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleSHOTGUN " << m_recoilPitchScaleSHOTGUN << std::endl;
    stream << "rem ------------------------------------------" << std::endl;
    stream << "rem ---Recoil Settings (Sniper Rifle Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleSNIPER " << m_recoilPunchScaleSNIPER << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeSNIPER " << m_recoilPunchTimeSNIPER << std::endl;
    stream << "HapticSettings.setRecoilYawScaleSNIPER " << m_recoilYawScaleSNIPER << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleSNIPER " << m_recoilPitchScaleSNIPER << std::endl;
    stream << "rem ----------------------------------------------------" << std::endl;
    stream << "rem ---Recoil Settings (Rifle Grenade Launcher Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleRIFLELAUNCHER " << m_recoilPunchScaleRIFLELAUNCHER << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeRIFLELAUNCHER " << m_recoilPunchTimeRIFLELAUNCHER << std::endl;
    stream << "HapticSettings.setRecoilYawScaleRIFLELAUNCHER " << m_recoilYawScaleRIFLELAUNCHER << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleRIFLELAUNCHER " << m_recoilPitchScaleRIFLELAUNCHER << std::endl;
    stream << "rem ----------------------------------------------" << std::endl;
    stream << "rem ---Recoil Settings (Rocket Launcher Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleLAUNCHER " << m_recoilPunchScaleLAUNCHER << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeLAUNCHER " << m_recoilPunchTimeLAUNCHER << std::endl;
    stream << "HapticSettings.setRecoilYawScaleLAUNCHER " << m_recoilYawScaleLAUNCHER << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleLAUNCHER " << m_recoilPitchScaleLAUNCHER << std::endl;
    stream << "rem -----------------------------------------------" << std::endl;
    stream << "rem ---Recoil Settings (Light Machine Gun Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleLMG " << m_recoilPunchScaleLMG << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeLMG " << m_recoilPunchTimeLMG << std::endl;
    stream << "HapticSettings.setRecoilYawScaleLMG " << m_recoilYawScaleLMG << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleLMG " << m_recoilPitchScaleLMG << std::endl;
    stream << "rem ---------------------------------------------" << std::endl;
    stream << "rem ---Recoil Settings (Sub Machine Gun Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleSMG " << m_recoilPunchScaleSMG << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeSMG " << m_recoilPunchTimeSMG << std::endl;
    stream << "HapticSettings.setRecoilYawScaleSMG " << m_recoilYawScaleSMG << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleSMG " << m_recoilPitchScaleSMG << std::endl;
    stream << "rem ------------------------------------------" << std::endl;
    stream << "rem ---Recoil Settings (Melee Weapon Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleKNIFE " << m_recoilPunchScaleKNIFE << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeKNIFE " << m_recoilPunchTimeKNIFE << std::endl;
    stream << "HapticSettings.setRecoilYawScaleKNIFE " << m_recoilYawScaleKNIFE << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleKNIFE " << m_recoilPitchScaleKNIFE << std::endl;
    stream << "rem ---------------------------------------------" << std::endl;
    stream << "rem ---Recoil Settings (Dropped Item Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleDROP " << m_recoilPunchScaleDROP << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeDROP " << m_recoilPunchTimeDROP << std::endl;
    stream << "HapticSettings.setRecoilYawScaleDROP " << m_recoilYawScaleDROP << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleDROP " << m_recoilPitchScaleDROP << std::endl;
    stream << "rem ---------------------------------------------" << std::endl;
    stream << "rem ---Recoil Settings (Thrown Item Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleTHROWN " << m_recoilPunchScaleTHROWN << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeTHROWN " << m_recoilPunchTimeTHROWN << std::endl;
    stream << "HapticSettings.setRecoilYawScaleTHROWN " << m_recoilYawScaleTHROWN << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleTHROWN " << m_recoilPitchScaleTHROWN << std::endl;
    stream << "rem ---------------------------------------------" << std::endl;
    stream << "rem ---Recoil Settings (Defibrillator Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleDEFIB " << m_recoilPunchScaleDEFIB << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeDEFIB " << m_recoilPunchTimeDEFIB << std::endl;
    stream << "HapticSettings.setRecoilYawScaleDEFIB " << m_recoilYawScaleDEFIB << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleDEFIB " << m_recoilPitchScaleDEFIB << std::endl;
    stream << "rem -------------------------------------------------------" << std::endl;
    stream << "rem ---Vehicle Recoil Settings (Light Machine Gun Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleV_LMG " << m_recoilPunchScaleV_LMG << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeV_LMG " << m_recoilPunchTimeV_LMG << std::endl;
    stream << "HapticSettings.setRecoilYawScaleV_LMG " << m_recoilYawScaleV_LMG << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleV_LMG " << m_recoilPitchScaleV_LMG << std::endl;
    stream << "rem -------------------------------------------------------" << std::endl;
    stream << "rem ---Vehicle Recoil Settings (Heavy Machine Gun Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleV_HMG " << m_recoilPunchScaleV_HMG << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeV_HMG " << m_recoilPunchTimeV_HMG << std::endl;
    stream << "HapticSettings.setRecoilYawScaleV_HMG " << m_recoilYawScaleV_HMG << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleV_HMG " << m_recoilPitchScaleV_HMG << std::endl;
    stream << "rem -------------------------------------------------------" << std::endl;
    stream << "rem ---Vehicle Recoil Settings (Helicopter Cannon Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleV_HELIGUN " << m_recoilPunchScaleV_HELIGUN << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeV_HELIGUN " << m_recoilPunchTimeV_HELIGUN << std::endl;
    stream << "HapticSettings.setRecoilYawScaleV_HELIGUN " << m_recoilYawScaleV_HELIGUN << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleV_HELIGUN " << m_recoilPitchScaleV_HELIGUN << std::endl;
    stream << "rem -----------------------------------------------------" << std::endl;
    stream << "rem ---Vehicle Recoil Settings (Airplane Cannon Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleV_AIRGUN " << m_recoilPunchScaleV_AIRGUN << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeV_AIRGUN " << m_recoilPunchTimeV_AIRGUN << std::endl;
    stream << "HapticSettings.setRecoilYawScaleV_AIRGUN " << m_recoilYawScaleV_AIRGUN << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleV_AIRGUN " << m_recoilPitchScaleV_AIRGUN << std::endl;
    stream << "rem -------------------------------------------------------" << std::endl;
    stream << "rem ---Vehicle Recoil Settings (Missile Launcher Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleV_MISSILE " << m_recoilPunchScaleV_MISSILE << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeV_MISSILE " << m_recoilPunchTimeV_MISSILE << std::endl;
    stream << "HapticSettings.setRecoilYawScaleV_MISSILE " << m_recoilYawScaleV_MISSILE << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleV_MISSILE " << m_recoilPitchScaleV_MISSILE << std::endl;
    stream << "rem -------------------------------------------------" << std::endl;
    stream << "rem ---Vehicle Recoil Settings (Armor Cannon Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleV_ARMORCANNON " << m_recoilPunchScaleV_ARMORCANNON << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeV_ARMORCANNON " << m_recoilPunchTimeV_ARMORCANNON << std::endl;
    stream << "HapticSettings.setRecoilYawScaleV_ARMORCANNON " << m_recoilYawScaleV_ARMORCANNON << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleV_ARMORCANNON " << m_recoilPitchScaleV_ARMORCANNON << std::endl;
    stream << "rem ---------------------------------------------------------" << std::endl;
    stream << "rem ---Vehicle Recoil Settings (Coaxial Machine Gun Class)---" << std::endl;
    stream << "HapticSettings.setRecoilPunchScaleV_COAXIALGUN " << m_recoilPunchScaleV_COAXIALGUN << std::endl;
    stream << "HapticSettings.setRecoilPunchTimeV_COAXIALGUN " << m_recoilPunchTimeV_COAXIALGUN << std::endl;
    stream << "HapticSettings.setRecoilYawScaleV_COAXIALGUN " << m_recoilYawScaleV_COAXIALGUN << std::endl;
    stream << "HapticSettings.setRecoilPitchScaleV_COAXIALGUN " << m_recoilPitchScaleV_COAXIALGUN << std::endl;
    stream << "rem -------------------" << std::endl;
    stream << "rem ---Shake Effects---" << std::endl;
    stream << "HapticSettings.setGeneralShakeScale " << m_generalShakeScale << std::endl;
    stream << "HapticSettings.setExplosionShakeScale " << m_explosionShakeScale << std::endl;
    stream << "HapticSettings.setSpeedShakeScale " << m_speedShakeScale << std::endl;
    stream << "rem ------------------------------" << std::endl;
    stream << "rem ---Weapon Type Template Map---" << std::endl;
    stream << "rem ***NO NEED TO EDIT THESE FOR EFFECT TUNING*********" << std::endl;
    stream << "rem ***THIS JUST MAPS WEAPON TYPES TO TEMPLATE NAME****" << std::endl;
    stream << "rem ***MOSTLY BASED OFF OF THE ''constants.py'' FILE***" << std::endl;
    for (std::vector<WeaponClassEntry>::const_iterator it = m_weaponClassMap.begin(); it != m_weaponClassMap.end(); ++it)
        stream << "HapticSettings.addWeaponClassMap " << it->name << " " << it->weaponClass << std::endl;
    return stream.str();
}

// (11 B)
bool HapticSettings::apply() { return hudManager != nullptr; }

// (5 B): a jump through the table.
void HapticSettings::setDefaults() { setDefaultValues(); }

// (8,158 B): the vector defaults go through the table (the setter slots), the scalars are
// stored directly in the shipped order, the weapon-class table is emptied (595EC0 frees its
// storage) and rebuilt through addWeaponClassMap with the shipped names and ids.
void HapticSettings::setDefaultValues()
{
    m_forceCapX = 50.0f;
    m_forceCapY = 50.0f;
    m_forceCapZ = 50.0f;
    setControlBoxLengthSOLDIER(0.03f, 0.03f, 0.01f);
    setControlBoxStiffnessSOLDIER(760.0f, 760.0f, 760.0f);
    setControlBoxLengthLAND(0.015f, 0.015f, 0.01f);
    setControlBoxStiffnessLAND(400.0f, 400.0f, 800.0f);
    setControlBoxLengthAIR(0.001f, 0.001f, 0.001f);
    setControlBoxStiffnessAIR(700.0f, 700.0f, 700.0f);
    setControlBoxLengthHELI(0.001f, 0.001f, 0.001f);
    setControlBoxStiffnessHELI(500.0f, 500.0f, 500.0f);
    setControlBoxLengthSEA(0.015f, 0.015f, 0.01f);
    setControlBoxStiffnessSEA(400.0f, 400.0f, 400.0f);
    setControlBoxLength(0.03f, 0.03f, 0.01f);
    setControlBoxStiffness(800.0f, 800.0f, 800.0f);
    setAimSensitivitySOLDIER(37.0f, 37.0f, 37.0f);
    setAimSensitivityLengthSOLDIER(0.03f, 0.03f, 0.01f);
    setTurnSensitivitySOLDIER(5.8f, 5.8f, 5.8f);
    setAimSensitivityLAND(20.0f, 20.0f, 20.0f);
    setAimSensitivityLengthLAND(0.02f, 0.02f, 0.01f);
    setTurnSensitivityLAND(4.0f, 4.0f, 4.0f);
    setAimSensitivityAIR(0.0f, 0.0f, 0.0f);
    setAimSensitivityLengthAIR(0.01f, 0.01f, 0.01f);
    setTurnSensitivityAIR(4.0f, 4.0f, 3.0f);
    setAimSensitivityHELI(0.0f, 0.0f, 0.0f);
    setAimSensitivityLengthHELI(0.01f, 0.01f, 0.02f);
    setTurnSensitivityHELI(2.0f, 2.0f, 8.0f);
    setAimSensitivitySEA(3.0f, 3.0f, 3.0f);
    setAimSensitivityLengthSEA(0.015f, 0.015f, 0.015f);
    setTurnSensitivitySEA(3.0f, 3.0f, 3.0f);
    setAimSensitivity(36.0f, 36.0f, 36.0f);
    setAimSensitivityLength(0.03f, 0.03f, 0.01f);
    setTurnSensitivity(12.0f, 12.0f, 12.0f);
    setPhysicsSOLDIERScale(1.0f, 1.0f, 1.0f);
    m_physicsSOLDIERSmoothTimeXLow = 10;
    m_physicsSOLDIERSmoothTimeZLow = 10;
    m_physicsSOLDIERSmoothTimeXMed = 200;
    m_physicsSOLDIERSmoothTimeYMed = 800;
    m_physicsSOLDIERSmoothTimeXHigh = 200;
    m_physicsSOLDIERSmoothTimeYHigh = 800;
    m_physicsSOLDIERScaleXLow = 0.03f;
    m_physicsSOLDIERScaleYLow = 0.035f;
    m_physicsSOLDIERScaleZLow = 0.06f;
    m_physicsSOLDIERSmoothTimeYLow = 20;
    m_physicsSOLDIERTransitionValueMed = 900.0f;
    m_physicsSOLDIERScaleXMed = 0.0f;
    m_physicsSOLDIERScaleYMed = 0.0f;
    m_physicsSOLDIERScaleZMed = 0.0f;
    m_physicsSOLDIERSmoothTimeZMed = 400;
    m_physicsSOLDIERTransitionValueHigh = 1000.0f;
    m_physicsSOLDIERScaleXHigh = 0.0f;
    m_physicsSOLDIERScaleYHigh = 0.0f;
    m_physicsSOLDIERScaleZHigh = 0.0f;
    m_physicsSOLDIERSmoothTimeZHigh = 400;
    setPhysicsLANDScale(1.0f, 1.0f, 1.0f);
    m_physicsLANDScaleXLow = 0.1f;
    m_physicsLANDScaleXMed = 0.1f;
    m_physicsLANDSmoothTimeXMed = 19;
    m_physicsLANDSmoothTimeZMed = 19;
    m_physicsLANDScaleYLow = 0.2f;
    m_physicsLANDScaleZLow = 0.3f;
    m_physicsLANDSmoothTimeXLow = 50;
    m_physicsLANDSmoothTimeYLow = 25;
    m_physicsLANDSmoothTimeZLow = 10;
    m_physicsLANDTransitionValueMed = 10.0f;
    m_physicsLANDScaleYMed = 0.2f;
    m_physicsLANDScaleZMed = 0.2f;
    m_physicsLANDSmoothTimeYMed = 30;
    m_physicsLANDTransitionValueHigh = 90.0f;
    m_physicsLANDScaleXHigh = 0.4f;
    m_physicsLANDScaleYHigh = 0.4f;
    m_physicsLANDScaleZHigh = 0.4f;
    m_physicsLANDSmoothTimeXHigh = 5;
    m_physicsLANDSmoothTimeYHigh = 5;
    m_physicsLANDSmoothTimeZHigh = 5;
    setPhysicsAIRScale(1.0f, 1.0f, 1.0f);
    m_physicsAIRScaleXLow = 0.2f;
    m_physicsAIRScaleYLow = 0.2f;
    m_physicsAIRScaleZLow = 0.2f;
    m_physicsAIRSmoothTimeXLow = 400;
    m_physicsAIRSmoothTimeYLow = 150;
    m_physicsAIRSmoothTimeZLow = 150;
    m_physicsAIRTransitionValueMed = 100.0f;
    m_physicsAIRScaleXMed = 0.0f;
    m_physicsAIRScaleYMed = 0.0f;
    m_physicsAIRScaleZMed = 0.0f;
    m_physicsAIRSmoothTimeXMed = 150;
    m_physicsAIRSmoothTimeYMed = 150;
    m_physicsAIRSmoothTimeZMed = 150;
    m_physicsAIRTransitionValueHigh = 200.0f;
    m_physicsAIRScaleXHigh = 0.0f;
    m_physicsAIRScaleYHigh = 0.0f;
    m_physicsAIRScaleZHigh = 0.0f;
    m_physicsAIRSmoothTimeXHigh = 150;
    m_physicsAIRSmoothTimeYHigh = 150;
    m_physicsAIRSmoothTimeZHigh = 150;
    setPhysicsHELIScale(1.0f, 1.0f, 1.0f);
    m_physicsHELIScaleXLow = 0.3f;
    m_physicsHELIScaleYLow = 0.3f;
    m_physicsHELIScaleZLow = 0.3f;
    m_physicsHELISmoothTimeXLow = 400;
    m_physicsHELISmoothTimeYLow = 400;
    m_physicsHELISmoothTimeZLow = 400;
    m_physicsHELITransitionValueMed = 100.0f;
    m_physicsHELIScaleXMed = 0.0f;
    m_physicsHELIScaleYMed = 0.0f;
    m_physicsHELIScaleZMed = 0.0f;
    m_physicsHELISmoothTimeXMed = 150;
    m_physicsHELISmoothTimeYMed = 150;
    m_physicsHELISmoothTimeZMed = 150;
    m_physicsHELITransitionValueHigh = 200.0f;
    m_physicsHELIScaleXHigh = 0.0f;
    m_physicsHELIScaleYHigh = 0.0f;
    m_physicsHELIScaleZHigh = 0.0f;
    m_physicsHELISmoothTimeXHigh = 150;
    m_physicsHELISmoothTimeYHigh = 150;
    m_physicsHELISmoothTimeZHigh = 150;
    setPhysicsSEAScale(1.0f, 1.0f, 1.0f);
    m_physicsSEASmoothTimeXLow = 2;
    m_physicsSEASmoothTimeYLow = 2;
    m_physicsSEASmoothTimeZLow = 2;
    m_physicsSEAScaleXLow = 0.5f;
    m_physicsSEAScaleYLow = 0.9f;
    m_physicsSEAScaleZLow = 0.4f;
    m_physicsSEATransitionValueMed = 10.0f;
    m_physicsSEAScaleXMed = 0.3f;
    m_physicsSEAScaleYMed = 0.6f;
    m_physicsSEAScaleZMed = 0.4f;
    m_physicsSEASmoothTimeXMed = 4;
    m_physicsSEASmoothTimeYMed = 4;
    m_physicsSEASmoothTimeZMed = 4;
    m_physicsSEATransitionValueHigh = 40.0f;
    m_physicsSEAScaleXHigh = 0.0006f;
    m_physicsSEAScaleYHigh = 0.0006f;
    m_physicsSEAScaleZHigh = 0.0006f;
    m_physicsSEASmoothTimeXHigh = 5;
    m_physicsSEASmoothTimeYHigh = 5;
    m_physicsSEASmoothTimeZHigh = 5;
    m_damageScale = 0.4f;
    m_recoilPunchScalePISTOL = 19.0f;
    m_recoilPunchTimePISTOL = 3.0f;
    m_recoilYawScalePISTOL = 0.0f;
    m_recoilPitchScalePISTOL = 0.0f;
    m_recoilPunchScaleRIFLE = 40.0f;
    m_recoilPunchTimeRIFLE = 2.0f;
    m_recoilYawScaleRIFLE = 0.0f;
    m_recoilPitchScaleRIFLE = 0.0f;
    m_recoilPunchScaleCARBINE = 40.0f;
    m_recoilPunchTimeCARBINE = 2.0f;
    m_recoilYawScaleCARBINE = 0.0f;
    m_recoilPitchScaleCARBINE = 0.0f;
    m_recoilPunchScaleSHOTGUN = 11.0f;
    m_recoilPunchTimeSHOTGUN = 5.0f;
    m_recoilYawScaleSHOTGUN = 0.0f;
    m_recoilPitchScaleSHOTGUN = 0.0f;
    m_recoilPunchScaleSNIPER = 16.0f;
    m_recoilPunchTimeSNIPER = 6.0f;
    m_recoilYawScaleSNIPER = 0.0f;
    m_recoilPitchScaleSNIPER = 0.0f;
    m_recoilPunchScaleRIFLELAUNCHER = 10.0f;
    m_recoilPunchTimeRIFLELAUNCHER = 9.0f;
    m_recoilYawScaleRIFLELAUNCHER = 0.0f;
    m_recoilPitchScaleRIFLELAUNCHER = 0.0f;
    m_recoilPunchScaleLAUNCHER = 24.0f;
    m_recoilPunchTimeLAUNCHER = 40.0f;
    m_recoilYawScaleLAUNCHER = 0.0f;
    m_recoilPitchScaleLAUNCHER = 0.0f;
    m_recoilPunchScaleLMG = 42.0f;
    m_recoilPunchTimeLMG = 6.0f;
    m_recoilYawScaleLMG = 0.0f;
    m_recoilPitchScaleLMG = 0.0f;
    m_recoilPunchScaleSMG = 30.0f;
    m_recoilPunchTimeSMG = 2.0f;
    m_recoilYawScaleSMG = 0.0f;
    m_recoilPitchScaleSMG = 0.0f;
    m_recoilPunchScaleKNIFE = 10.0f;
    m_recoilPunchTimeKNIFE = 50.0f;
    m_recoilYawScaleKNIFE = 0.0f;
    m_recoilPitchScaleKNIFE = 0.0f;
    m_recoilPunchScaleDROP = 10.0f;
    m_recoilPunchTimeDROP = 75.0f;
    m_recoilYawScaleDROP = 0.0f;
    m_recoilPitchScaleDROP = 0.0f;
    m_recoilPunchScaleTHROWN = 15.0f;
    m_recoilPunchTimeTHROWN = 80.0f;
    m_recoilYawScaleTHROWN = 0.0f;
    m_recoilPitchScaleTHROWN = 0.0f;
    m_recoilPunchScaleDEFIB = 40.0f;
    m_recoilPunchTimeDEFIB = 50.0f;
    m_recoilYawScaleDEFIB = 0.0f;
    m_recoilPitchScaleDEFIB = 0.0f;
    m_recoilPunchScaleV_LMG = 10.0f;
    m_recoilPunchTimeV_LMG = 2.0f;
    m_recoilYawScaleV_LMG = 0.0f;
    m_recoilPitchScaleV_LMG = 2.0f;
    m_recoilPunchScaleV_HMG = 12.0f;
    m_recoilPunchTimeV_HMG = 4.0f;
    m_recoilYawScaleV_HMG = 0.0f;
    m_recoilPitchScaleV_HMG = -2.0f;
    m_recoilPunchScaleV_HELIGUN = 12.0f;
    m_recoilPunchTimeV_HELIGUN = 5.0f;
    m_recoilYawScaleV_HELIGUN = 2.0f;
    m_recoilPitchScaleV_HELIGUN = 6.0f;
    m_recoilPunchScaleV_AIRGUN = 12.0f;
    m_recoilPunchTimeV_AIRGUN = 10.0f;
    m_recoilYawScaleV_AIRGUN = 2.0f;
    m_recoilPitchScaleV_AIRGUN = 6.0f;
    m_recoilPunchScaleV_MISSILE = 8.0f;
    m_recoilPunchTimeV_MISSILE = 4.0f;
    m_recoilYawScaleV_MISSILE = 2.0f;
    m_recoilPitchScaleV_MISSILE = 2.0f;
    m_recoilPunchScaleV_ARMORCANNON = 30.0f;
    m_recoilPunchTimeV_ARMORCANNON = 8.0f;
    m_recoilYawScaleV_ARMORCANNON = 10.0f;
    m_recoilPitchScaleV_ARMORCANNON = 10.0f;
    m_recoilPunchScaleV_COAXIALGUN = 35.0f;
    m_recoilPunchTimeV_COAXIALGUN = 10.0f;
    m_recoilYawScaleV_COAXIALGUN = 2.0f;
    m_recoilPitchScaleV_COAXIALGUN = 8.0f;
    m_unresolved48C = 1.0f;
    m_unresolved490 = 1.0f;
    m_generalShakeScale = 14.0f;
    m_explosionShakeScale = 4.0f;
    m_speedShakeScale = 6.0f;
    std::vector<WeaponClassEntry>().swap(m_weaponClassMap);   //  at +4A0: the entries destroyed and the storage freed
    addWeaponClassMap(std::string("usrif_m16a2"), 0);
    addWeaponClassMap(std::string("rurif_ak101"), 0);
    addWeaponClassMap(std::string("rurif_ak47"), 0);
    addWeaponClassMap(std::string("usrif_sa80"), 0);
    addWeaponClassMap(std::string("usrif_g3a3"), 0);
    addWeaponClassMap(std::string("usrif_m203"), 0);
    addWeaponClassMap(std::string("rurif_gp30"), 0);
    addWeaponClassMap(std::string("rurif_gp25"), 0);
    addWeaponClassMap(std::string("rurif_oc14"), 0);
    addWeaponClassMap(std::string("sasrif_fn2000"), 0);
    addWeaponClassMap(std::string("sasrif_g36e"), 0);
    addWeaponClassMap(std::string("sasrif_g36k"), 0);
    addWeaponClassMap(std::string("spzrif_aps"), 0);
    addWeaponClassMap(std::string("usrif_fnscarh"), 0);
    addWeaponClassMap(std::string("eurif_famas"), 0);
    addWeaponClassMap(std::string("gbrif_sa80a2_l85"), 0);
    addWeaponClassMap(std::string("usrgl_m203"), 1);
    addWeaponClassMap(std::string("rurgl_gp30"), 1);
    addWeaponClassMap(std::string("rurgl_gp25"), 1);
    addWeaponClassMap(std::string("sasgr_fn2000"), 1);
    addWeaponClassMap(std::string("gbgr_sa80a2_l85"), 1);
    addWeaponClassMap(std::string("rurrif_ak74u"), 2);
    addWeaponClassMap(std::string("usrif_m4"), 2);
    addWeaponClassMap(std::string("rurif_ak74u"), 2);
    addWeaponClassMap(std::string("chrif_type95"), 2);
    addWeaponClassMap(std::string("usrif_g36c"), 2);
    addWeaponClassMap(std::string("usrif_fnscarl"), 2);
    addWeaponClassMap(std::string("eurif_hk53a3"), 2);
    addWeaponClassMap(std::string("nsrif_crossbow"), 2);
    addWeaponClassMap(std::string("uslmg_m249saw"), 3);
    addWeaponClassMap(std::string("rulmg_rpk74"), 3);
    addWeaponClassMap(std::string("chlmg_type95"), 3);
    addWeaponClassMap(std::string("rulmg_pkm"), 3);
    addWeaponClassMap(std::string("sasrif_mg36"), 3);
    addWeaponClassMap(std::string("gbrif_hk21"), 3);
    addWeaponClassMap(std::string("usrif_m24"), 4);
    addWeaponClassMap(std::string("rurif_dragunov"), 4);
    addWeaponClassMap(std::string("chsni_type88"), 4);
    addWeaponClassMap(std::string("ussni_m82a1"), 4);
    addWeaponClassMap(std::string("ussni_m95_barret"), 4);
    addWeaponClassMap(std::string("gbrif_l96a1"), 4);
    addWeaponClassMap(std::string("uspis_92fs"), 5);
    addWeaponClassMap(std::string("uspis_92fs_silencer"), 5);
    addWeaponClassMap(std::string("rupis_baghira"), 5);
    addWeaponClassMap(std::string("rupis_baghira_silencer"), 5);
    addWeaponClassMap(std::string("chpis_qsz92"), 5);
    addWeaponClassMap(std::string("chpis_qsz92_silencer"), 5);
    addWeaponClassMap(std::string("usatp_predator"), 6);
    addWeaponClassMap(std::string("chat_eryx"), 6);
    addWeaponClassMap(std::string("insgr_rpg"), 6);
    addWeaponClassMap(std::string("usrif_mp5_a3"), 7);
    addWeaponClassMap(std::string("rurif_bizon"), 7);
    addWeaponClassMap(std::string("chrif_type85"), 7);
    addWeaponClassMap(std::string("sasrif_mp7"), 7);
    addWeaponClassMap(std::string("eurif_fnp90"), 7);
    addWeaponClassMap(std::string("usrif_remington11-87"), 8);
    addWeaponClassMap(std::string("rusht_saiga12"), 8);
    addWeaponClassMap(std::string("chsht_norinco982"), 8);
    addWeaponClassMap(std::string("chsht_protecta"), 8);
    addWeaponClassMap(std::string("ussht_jackhammer"), 8);
    addWeaponClassMap(std::string("gbrif_benelli_m4"), 8);
    addWeaponClassMap(std::string("kni_knife"), 9);
    addWeaponClassMap(std::string("c4_explosives"), 10);
    addWeaponClassMap(std::string("usmin_claymore"), 10);
    addWeaponClassMap(std::string("at_mine"), 10);
    addWeaponClassMap(std::string("ushgr_m67"), 11);
    addWeaponClassMap(std::string("hgr_smoke"), 11);
    addWeaponClassMap(std::string("nshgr_flashbang"), 11);
    addWeaponClassMap(std::string("sasrif_teargas"), 11);
    addWeaponClassMap(std::string("defibrillator"), 12);
    addWeaponClassMap(std::string("uslmg_m249saw_stationary"), 13);
    addWeaponClassMap(std::string("m249saw_ammo"), 13);
    addWeaponClassMap(std::string("chlmg_type95_stationary"), 13);
    addWeaponClassMap(std::string("rulmg_rpk74_stationary"), 13);
    addWeaponClassMap(std::string("chhmg_kord"), 14);
    addWeaponClassMap(std::string("hmg_h2mg"), 14);
    addWeaponClassMap(std::string("hmg_m2hb"), 14);
    addWeaponClassMap(std::string("hmg_m2hb_ammo"), 14);
    addWeaponClassMap(std::string("hmg_m134"), 14);
    addWeaponClassMap(std::string("hmg_m134_gun"), 14);
    addWeaponClassMap(std::string("chhmg_type85"), 14);
    addWeaponClassMap(std::string("ustnk_m1a2_barrel"), 18);
    addWeaponClassMap(std::string("tnk_type98_barrel"), 18);
    addWeaponClassMap(std::string("rutnk_t90_barrel"), 18);
    addWeaponClassMap(std::string("usapc_lav25_barrel"), 18);
    addWeaponClassMap(std::string("apc_wz551_barrel"), 18);
    addWeaponClassMap(std::string("apc_btr90__barrel"), 18);
    addWeaponClassMap(std::string("usaav_m6_barrel"), 18);
    addWeaponClassMap(std::string("aav_type95guns"), 18);
    addWeaponClassMap(std::string("aav_tunguska_gun"), 18);
    addWeaponClassMap(std::string("coaxial_browning"), 19);
    addWeaponClassMap(std::string("coaxial_mg_china"), 19);
    addWeaponClassMap(std::string("coaxial_mg_mec"), 19);
    addWeaponClassMap(std::string("rutnk_t90_smokelauncher"), 17);
    addWeaponClassMap(std::string("usapc_lav25_smokelauncher"), 17);
    addWeaponClassMap(std::string("usapc_lav25_towlauncher"), 17);
    addWeaponClassMap(std::string("apc_wz551_hj8launcher"), 17);
    addWeaponClassMap(std::string("apc_wz551_smokelauncher"), 17);
    addWeaponClassMap(std::string("apc_btr90_hj8launcher"), 17);
    addWeaponClassMap(std::string("apc_btr90_smokeLauncher"), 17);
    addWeaponClassMap(std::string("usaav_m6_stinger_launcher"), 17);
    addWeaponClassMap(std::string("aav_type95_qw2launcher"), 17);
    addWeaponClassMap(std::string("aav_tunguska_sa19launcher"), 17);
    addWeaponClassMap(std::string("usthe_uh60_flarelauncher"), 17);
    addWeaponClassMap(std::string("the_mi17_flarelauncher"), 17);
    addWeaponClassMap(std::string("chthe_z8_flarelauncher"), 17);
    addWeaponClassMap(std::string("ats_tow_launcher"), 17);
    addWeaponClassMap(std::string("igla_djigit_launcher"), 17);
    addWeaponClassMap(std::string("ats_hj8_launcher"), 17);
    addWeaponClassMap(std::string("usaas_stinger_launcher"), 17);
    addWeaponClassMap(std::string("aas_seasparrow"), 17);
    addWeaponClassMap(std::string("aas_phalanx"), 17);
    addWeaponClassMap(std::string("f18_sidewinderlauncher"), 17);
    addWeaponClassMap(std::string("usair_f18_bomblauncher"), 17);
    addWeaponClassMap(std::string("usair_f15_mavericklauncherLaser"), 17);
    addWeaponClassMap(std::string("usair_f15_mavericklaunchertv"), 17);
    addWeaponClassMap(std::string("usair_f15_sidewinderlauncher"), 17);
    addWeaponClassMap(std::string("usair_f15_250kgbomblauncher"), 17);
    addWeaponClassMap(std::string("ruair_su34_kedgelauncherLaser"), 17);
    addWeaponClassMap(std::string("ruair_su34_kedgelaunchertv"), 17);
    addWeaponClassMap(std::string("ruair_su34_archerlauncher"), 17);
    addWeaponClassMap(std::string("ruair_su34_250kgbomblauncher"), 17);
    addWeaponClassMap(std::string("ruair_archerlauncher"), 17);
    addWeaponClassMap(std::string("ruair_mig29_bomblauncher_1"), 17);
    addWeaponClassMap(std::string("air_su30mkk_archerlauncher"), 17);
    addWeaponClassMap(std::string("air_su34mkk_bomblauncher"), 17);
    addWeaponClassMap(std::string("air_su30mkk_kedgelauncher_Laser"), 17);
    addWeaponClassMap(std::string("air_su30mkk_kedgelauncher_Wire"), 17);
    addWeaponClassMap(std::string("air_j10_archerlauncher"), 17);
    addWeaponClassMap(std::string("air_j10_bomblauncher"), 17);
    addWeaponClassMap(std::string("air_f35b_sidewinderlauncher"), 17);
    addWeaponClassMap(std::string("air_f35b_bomblauncher"), 17);
    addWeaponClassMap(std::string("ahe_z10_hj8launcher_tv"), 17);
    addWeaponClassMap(std::string("ahe_z10_s8launcher"), 17);
    addWeaponClassMap(std::string("ahe_z10_flarelauncher"), 17);
    addWeaponClassMap(std::string("ahe_havoc_atakalauncher_tv"), 17);
    addWeaponClassMap(std::string("ahe_havoc_flarelauncher"), 17);
    addWeaponClassMap(std::string("ahe_havoc_s8launcher"), 17);
    addWeaponClassMap(std::string("ahe_ah1z_cogunner_hellfirelaunchertv"), 17);
    addWeaponClassMap(std::string("ahe_ah1z_hydralauncher"), 17);
    addWeaponClassMap(std::string("ahe_ah1z_flarelauncher"), 17);
    addWeaponClassMap(std::string("f18_autocannon"), 16);
    addWeaponClassMap(std::string("usair_f15_autocannon"), 16);
    addWeaponClassMap(std::string("ruair_su34_30mmcannon"), 16);
    addWeaponClassMap(std::string("ruair_mig29_30mmcannon"), 16);
    addWeaponClassMap(std::string("air_su30mkk_30mmcannon"), 16);
    addWeaponClassMap(std::string("air_j10_cannon"), 16);
    addWeaponClassMap(std::string("air_f35b_autocannon"), 16);
    addWeaponClassMap(std::string("ahe_z10_gun"), 15);
    addWeaponClassMap(std::string("ahe_havoc_gun"), 15);
    addWeaponClassMap(std::string("ahe_ah1z_gun"), 15);
}

// (170 B): with a LocalProfileManager singleton that has a current profile, that profile's
// name, "/" and this file name are joined and handed to loadProfileFile; the result is discarded.
// getCurrentProfile is asked twice, as shipped.
void HapticSettings::loadCurrentProfileFile()
{
    IBase* singleton = classManager->getSingleton(std::string("LocalProfileManager"));
    if (!singleton)
        return;
    ILocalProfileManager* profiles = static_cast<ILocalProfileManager*>(singleton);
    if (!profiles->getCurrentProfile())
        return;
    std::string path(std::string(profiles->getCurrentProfile()->name) += "/");
    path += getFileName();
    profiles->loadProfileFile(path);
}

// (180 B): the same body; false without the singleton or a current profile, true after the load.
bool HapticSettings::reloadCurrentProfileFile()
{
    IBase* singleton = classManager->getSingleton(std::string("LocalProfileManager"));
    if (!singleton)
        return false;
    ILocalProfileManager* profiles = static_cast<ILocalProfileManager*>(singleton);
    if (!profiles->getCurrentProfile())
        return false;
    std::string path(std::string(profiles->getCurrentProfile()->name) += "/");
    path += getFileName();
    profiles->loadProfileFile(path);
    return true;
}

// +2C..+7E8: the accessor pairs, the by-level dispatchers (through the table, as shipped) and the
// by-class dispatchers. Table order; see the header for the slot map.
float HapticSettings::getForceCapX() { return m_forceCapX; }
void HapticSettings::setForceCapX(float value) { m_forceCapX = value; }
float HapticSettings::getForceCapY() { return m_forceCapY; }
void HapticSettings::setForceCapY(float value) { m_forceCapY = value; }
float HapticSettings::getForceCapZ() { return m_forceCapZ; }
void HapticSettings::setForceCapZ(float value) { m_forceCapZ = value; }
Vec3 HapticSettings::getControlBoxLength() { return m_controlBoxLength; }
void HapticSettings::setControlBoxLength(float x, float y, float z) { m_controlBoxLength.x = x; m_controlBoxLength.y = y; m_controlBoxLength.z = z; }
Vec3 HapticSettings::getControlBoxStiffness() { return m_controlBoxStiffness; }
void HapticSettings::setControlBoxStiffness(float x, float y, float z) { m_controlBoxStiffness.x = x; m_controlBoxStiffness.y = y; m_controlBoxStiffness.z = z; }
Vec3 HapticSettings::getControlBoxLengthSOLDIER() { return m_controlBoxLengthSOLDIER; }
void HapticSettings::setControlBoxLengthSOLDIER(float x, float y, float z) { m_controlBoxLengthSOLDIER.x = x; m_controlBoxLengthSOLDIER.y = y; m_controlBoxLengthSOLDIER.z = z; }
Vec3 HapticSettings::getControlBoxStiffnessSOLDIER() { return m_controlBoxStiffnessSOLDIER; }
void HapticSettings::setControlBoxStiffnessSOLDIER(float x, float y, float z) { m_controlBoxStiffnessSOLDIER.x = x; m_controlBoxStiffnessSOLDIER.y = y; m_controlBoxStiffnessSOLDIER.z = z; }
Vec3 HapticSettings::getControlBoxLengthAIR() { return m_controlBoxLengthAIR; }
void HapticSettings::setControlBoxLengthAIR(float x, float y, float z) { m_controlBoxLengthAIR.x = x; m_controlBoxLengthAIR.y = y; m_controlBoxLengthAIR.z = z; }
Vec3 HapticSettings::getControlBoxStiffnessAIR() { return m_controlBoxStiffnessAIR; }
void HapticSettings::setControlBoxStiffnessAIR(float x, float y, float z) { m_controlBoxStiffnessAIR.x = x; m_controlBoxStiffnessAIR.y = y; m_controlBoxStiffnessAIR.z = z; }
Vec3 HapticSettings::getControlBoxLengthHELI() { return m_controlBoxLengthHELI; }
void HapticSettings::setControlBoxLengthHELI(float x, float y, float z) { m_controlBoxLengthHELI.x = x; m_controlBoxLengthHELI.y = y; m_controlBoxLengthHELI.z = z; }
Vec3 HapticSettings::getControlBoxStiffnessHELI() { return m_controlBoxStiffnessHELI; }
void HapticSettings::setControlBoxStiffnessHELI(float x, float y, float z) { m_controlBoxStiffnessHELI.x = x; m_controlBoxStiffnessHELI.y = y; m_controlBoxStiffnessHELI.z = z; }
Vec3 HapticSettings::getControlBoxLengthLAND() { return m_controlBoxLengthLAND; }
void HapticSettings::setControlBoxLengthLAND(float x, float y, float z) { m_controlBoxLengthLAND.x = x; m_controlBoxLengthLAND.y = y; m_controlBoxLengthLAND.z = z; }
Vec3 HapticSettings::getControlBoxStiffnessLAND() { return m_controlBoxStiffnessLAND; }
void HapticSettings::setControlBoxStiffnessLAND(float x, float y, float z) { m_controlBoxStiffnessLAND.x = x; m_controlBoxStiffnessLAND.y = y; m_controlBoxStiffnessLAND.z = z; }
Vec3 HapticSettings::getControlBoxLengthSEA() { return m_controlBoxLengthSEA; }
void HapticSettings::setControlBoxLengthSEA(float x, float y, float z) { m_controlBoxLengthSEA.x = x; m_controlBoxLengthSEA.y = y; m_controlBoxLengthSEA.z = z; }
Vec3 HapticSettings::getControlBoxStiffnessSEA() { return m_controlBoxStiffnessSEA; }
void HapticSettings::setControlBoxStiffnessSEA(float x, float y, float z) { m_controlBoxStiffnessSEA.x = x; m_controlBoxStiffnessSEA.y = y; m_controlBoxStiffnessSEA.z = z; }
Vec3 HapticSettings::getAimSensitivity() { return m_aimSensitivity; }
void HapticSettings::setAimSensitivity(float x, float y, float z) { m_aimSensitivity.x = x; m_aimSensitivity.y = y; m_aimSensitivity.z = z; }
Vec3 HapticSettings::getAimSensitivityLength() { return m_aimSensitivityLength; }
void HapticSettings::setAimSensitivityLength(float x, float y, float z) { m_aimSensitivityLength.x = x; m_aimSensitivityLength.y = y; m_aimSensitivityLength.z = z; }
Vec3 HapticSettings::getTurnSensitivity() { return m_turnSensitivity; }
void HapticSettings::setTurnSensitivity(float x, float y, float z) { m_turnSensitivity.x = x; m_turnSensitivity.y = y; m_turnSensitivity.z = z; }
Vec3 HapticSettings::getAimSensitivitySOLDIER() { return m_aimSensitivitySOLDIER; }
void HapticSettings::setAimSensitivitySOLDIER(float x, float y, float z) { m_aimSensitivitySOLDIER.x = x; m_aimSensitivitySOLDIER.y = y; m_aimSensitivitySOLDIER.z = z; }
Vec3 HapticSettings::getAimSensitivityLengthSOLDIER() { return m_aimSensitivityLengthSOLDIER; }
void HapticSettings::setAimSensitivityLengthSOLDIER(float x, float y, float z) { m_aimSensitivityLengthSOLDIER.x = x; m_aimSensitivityLengthSOLDIER.y = y; m_aimSensitivityLengthSOLDIER.z = z; }
Vec3 HapticSettings::getTurnSensitivitySOLDIER() { return m_turnSensitivitySOLDIER; }
void HapticSettings::setTurnSensitivitySOLDIER(float x, float y, float z) { m_turnSensitivitySOLDIER.x = x; m_turnSensitivitySOLDIER.y = y; m_turnSensitivitySOLDIER.z = z; }
Vec3 HapticSettings::getAimSensitivityAIR() { return m_aimSensitivityAIR; }
void HapticSettings::setAimSensitivityAIR(float x, float y, float z) { m_aimSensitivityAIR.x = x; m_aimSensitivityAIR.y = y; m_aimSensitivityAIR.z = z; }
Vec3 HapticSettings::getAimSensitivityLengthAIR() { return m_aimSensitivityLengthAIR; }
void HapticSettings::setAimSensitivityLengthAIR(float x, float y, float z) { m_aimSensitivityLengthAIR.x = x; m_aimSensitivityLengthAIR.y = y; m_aimSensitivityLengthAIR.z = z; }
Vec3 HapticSettings::getTurnSensitivityAIR() { return m_turnSensitivityAIR; }
void HapticSettings::setTurnSensitivityAIR(float x, float y, float z) { m_turnSensitivityAIR.x = x; m_turnSensitivityAIR.y = y; m_turnSensitivityAIR.z = z; }
Vec3 HapticSettings::getAimSensitivityHELI() { return m_aimSensitivityHELI; }
void HapticSettings::setAimSensitivityHELI(float x, float y, float z) { m_aimSensitivityHELI.x = x; m_aimSensitivityHELI.y = y; m_aimSensitivityHELI.z = z; }
Vec3 HapticSettings::getAimSensitivityLengthHELI() { return m_aimSensitivityLengthHELI; }
void HapticSettings::setAimSensitivityLengthHELI(float x, float y, float z) { m_aimSensitivityLengthHELI.x = x; m_aimSensitivityLengthHELI.y = y; m_aimSensitivityLengthHELI.z = z; }
Vec3 HapticSettings::getTurnSensitivityHELI() { return m_turnSensitivityHELI; }
void HapticSettings::setTurnSensitivityHELI(float x, float y, float z) { m_turnSensitivityHELI.x = x; m_turnSensitivityHELI.y = y; m_turnSensitivityHELI.z = z; }
Vec3 HapticSettings::getAimSensitivityLAND() { return m_aimSensitivityLAND; }
void HapticSettings::setAimSensitivityLAND(float x, float y, float z) { m_aimSensitivityLAND.x = x; m_aimSensitivityLAND.y = y; m_aimSensitivityLAND.z = z; }
Vec3 HapticSettings::getAimSensitivityLengthLAND() { return m_aimSensitivityLengthLAND; }
void HapticSettings::setAimSensitivityLengthLAND(float x, float y, float z) { m_aimSensitivityLengthLAND.x = x; m_aimSensitivityLengthLAND.y = y; m_aimSensitivityLengthLAND.z = z; }
Vec3 HapticSettings::getTurnSensitivityLAND() { return m_turnSensitivityLAND; }
void HapticSettings::setTurnSensitivityLAND(float x, float y, float z) { m_turnSensitivityLAND.x = x; m_turnSensitivityLAND.y = y; m_turnSensitivityLAND.z = z; }
Vec3 HapticSettings::getAimSensitivitySEA() { return m_aimSensitivitySEA; }
void HapticSettings::setAimSensitivitySEA(float x, float y, float z) { m_aimSensitivitySEA.x = x; m_aimSensitivitySEA.y = y; m_aimSensitivitySEA.z = z; }
Vec3 HapticSettings::getAimSensitivityLengthSEA() { return m_aimSensitivityLengthSEA; }
void HapticSettings::setAimSensitivityLengthSEA(float x, float y, float z) { m_aimSensitivityLengthSEA.x = x; m_aimSensitivityLengthSEA.y = y; m_aimSensitivityLengthSEA.z = z; }
Vec3 HapticSettings::getTurnSensitivitySEA() { return m_turnSensitivitySEA; }
void HapticSettings::setTurnSensitivitySEA(float x, float y, float z) { m_turnSensitivitySEA.x = x; m_turnSensitivitySEA.y = y; m_turnSensitivitySEA.z = z; }
Vec3 HapticSettings::getPhysicsSOLDIERScale() { return m_physicsSOLDIERScale; }
void HapticSettings::setPhysicsSOLDIERScale(float x, float y, float z) { m_physicsSOLDIERScale.x = x; m_physicsSOLDIERScale.y = y; m_physicsSOLDIERScale.z = z; }
float HapticSettings::getPhysicsSOLDIERScaleXLow() { return m_physicsSOLDIERScaleXLow; }
void HapticSettings::setPhysicsSOLDIERScaleXLow(float value) { m_physicsSOLDIERScaleXLow = value; }
float HapticSettings::getPhysicsSOLDIERScaleYLow() { return m_physicsSOLDIERScaleYLow; }
void HapticSettings::setPhysicsSOLDIERScaleYLow(float value) { m_physicsSOLDIERScaleYLow = value; }
float HapticSettings::getPhysicsSOLDIERScaleZLow() { return m_physicsSOLDIERScaleZLow; }
void HapticSettings::setPhysicsSOLDIERScaleZLow(float value) { m_physicsSOLDIERScaleZLow = value; }
float HapticSettings::getPhysicsSOLDIERScaleXMed() { return m_physicsSOLDIERScaleXMed; }
void HapticSettings::setPhysicsSOLDIERScaleXMed(float value) { m_physicsSOLDIERScaleXMed = value; }
float HapticSettings::getPhysicsSOLDIERScaleYMed() { return m_physicsSOLDIERScaleYMed; }
void HapticSettings::setPhysicsSOLDIERScaleYMed(float value) { m_physicsSOLDIERScaleYMed = value; }
float HapticSettings::getPhysicsSOLDIERScaleZMed() { return m_physicsSOLDIERScaleZMed; }
void HapticSettings::setPhysicsSOLDIERScaleZMed(float value) { m_physicsSOLDIERScaleZMed = value; }
float HapticSettings::getPhysicsSOLDIERScaleXHigh() { return m_physicsSOLDIERScaleXHigh; }
void HapticSettings::setPhysicsSOLDIERScaleXHigh(float value) { m_physicsSOLDIERScaleXHigh = value; }
float HapticSettings::getPhysicsSOLDIERScaleYHigh() { return m_physicsSOLDIERScaleYHigh; }
void HapticSettings::setPhysicsSOLDIERScaleYHigh(float value) { m_physicsSOLDIERScaleYHigh = value; }
float HapticSettings::getPhysicsSOLDIERScaleZHigh() { return m_physicsSOLDIERScaleZHigh; }
void HapticSettings::setPhysicsSOLDIERScaleZHigh(float value) { m_physicsSOLDIERScaleZHigh = value; }
float HapticSettings::getPhysicsSOLDIERTransitionValue(int level)
{
    switch (level) { case 0: return getPhysicsSOLDIERTransitionValueMed(); case 1: return getPhysicsSOLDIERTransitionValueHigh(); default: return 0.0f; }
}
void HapticSettings::setPhysicsSOLDIERTransitionValue(int level, float value)
{
    switch (level) { case 0: setPhysicsSOLDIERTransitionValueMed(value); break; case 1: setPhysicsSOLDIERTransitionValueHigh(value); break; default: break; }
}
float HapticSettings::getPhysicsSOLDIERTransitionValueMed() { return m_physicsSOLDIERTransitionValueMed; }
void HapticSettings::setPhysicsSOLDIERTransitionValueMed(float value) { m_physicsSOLDIERTransitionValueMed = value; }
float HapticSettings::getPhysicsSOLDIERTransitionValueHigh() { return m_physicsSOLDIERTransitionValueHigh; }
void HapticSettings::setPhysicsSOLDIERTransitionValueHigh(float value) { m_physicsSOLDIERTransitionValueHigh = value; }
int HapticSettings::getPhysicsSOLDIERSmoothTimeX(int level)
{
    switch (level) { case 0: return getPhysicsSOLDIERSmoothTimeXLow(); case 1: return getPhysicsSOLDIERSmoothTimeXMed(); case 2: return getPhysicsSOLDIERSmoothTimeXHigh(); default: return 0; }
}
void HapticSettings::setPhysicsSOLDIERSmoothTimeX(int level, int value)
{
    switch (level) { case 0: setPhysicsSOLDIERSmoothTimeXLow(value); break; case 1: setPhysicsSOLDIERSmoothTimeXMed(value); break; case 2: setPhysicsSOLDIERSmoothTimeXHigh(value); break; default: break; }
}
int HapticSettings::getPhysicsSOLDIERSmoothTimeY(int level)
{
    switch (level) { case 0: return getPhysicsSOLDIERSmoothTimeYLow(); case 1: return getPhysicsSOLDIERSmoothTimeYMed(); case 2: return getPhysicsSOLDIERSmoothTimeYHigh(); default: return 0; }
}
void HapticSettings::setPhysicsSOLDIERSmoothTimeY(int level, int value)
{
    switch (level) { case 0: setPhysicsSOLDIERSmoothTimeYLow(value); break; case 1: setPhysicsSOLDIERSmoothTimeYMed(value); break; case 2: setPhysicsSOLDIERSmoothTimeYHigh(value); break; default: break; }
}
int HapticSettings::getPhysicsSOLDIERSmoothTimeZ(int level)
{
    switch (level) { case 0: return getPhysicsSOLDIERSmoothTimeZLow(); case 1: return getPhysicsSOLDIERSmoothTimeZMed(); case 2: return getPhysicsSOLDIERSmoothTimeZHigh(); default: return 0; }
}
void HapticSettings::setPhysicsSOLDIERSmoothTimeZ(int level, int value)
{
    switch (level) { case 0: setPhysicsSOLDIERSmoothTimeZLow(value); break; case 1: setPhysicsSOLDIERSmoothTimeZMed(value); break; case 2: setPhysicsSOLDIERSmoothTimeZHigh(value); break; default: break; }
}
int HapticSettings::getPhysicsSOLDIERSmoothTimeXLow() { return m_physicsSOLDIERSmoothTimeXLow; }
void HapticSettings::setPhysicsSOLDIERSmoothTimeXLow(int value) { m_physicsSOLDIERSmoothTimeXLow = value; }
int HapticSettings::getPhysicsSOLDIERSmoothTimeYLow() { return m_physicsSOLDIERSmoothTimeYLow; }
void HapticSettings::setPhysicsSOLDIERSmoothTimeYLow(int value) { m_physicsSOLDIERSmoothTimeYLow = value; }
int HapticSettings::getPhysicsSOLDIERSmoothTimeZLow() { return m_physicsSOLDIERSmoothTimeZLow; }
void HapticSettings::setPhysicsSOLDIERSmoothTimeZLow(int value) { m_physicsSOLDIERSmoothTimeZLow = value; }
int HapticSettings::getPhysicsSOLDIERSmoothTimeXMed() { return m_physicsSOLDIERSmoothTimeXMed; }
void HapticSettings::setPhysicsSOLDIERSmoothTimeXMed(int value) { m_physicsSOLDIERSmoothTimeXMed = value; }
int HapticSettings::getPhysicsSOLDIERSmoothTimeYMed() { return m_physicsSOLDIERSmoothTimeYMed; }
void HapticSettings::setPhysicsSOLDIERSmoothTimeYMed(int value) { m_physicsSOLDIERSmoothTimeYMed = value; }
int HapticSettings::getPhysicsSOLDIERSmoothTimeZMed() { return m_physicsSOLDIERSmoothTimeZMed; }
void HapticSettings::setPhysicsSOLDIERSmoothTimeZMed(int value) { m_physicsSOLDIERSmoothTimeZMed = value; }
int HapticSettings::getPhysicsSOLDIERSmoothTimeXHigh() { return m_physicsSOLDIERSmoothTimeXHigh; }
void HapticSettings::setPhysicsSOLDIERSmoothTimeXHigh(int value) { m_physicsSOLDIERSmoothTimeXHigh = value; }
int HapticSettings::getPhysicsSOLDIERSmoothTimeYHigh() { return m_physicsSOLDIERSmoothTimeYHigh; }
void HapticSettings::setPhysicsSOLDIERSmoothTimeYHigh(int value) { m_physicsSOLDIERSmoothTimeYHigh = value; }
int HapticSettings::getPhysicsSOLDIERSmoothTimeZHigh() { return m_physicsSOLDIERSmoothTimeZHigh; }
void HapticSettings::setPhysicsSOLDIERSmoothTimeZHigh(int value) { m_physicsSOLDIERSmoothTimeZHigh = value; }
Vec3 HapticSettings::getPhysicsLANDScale() { return m_physicsLANDScale; }
void HapticSettings::setPhysicsLANDScale(float x, float y, float z) { m_physicsLANDScale.x = x; m_physicsLANDScale.y = y; m_physicsLANDScale.z = z; }
float HapticSettings::getPhysicsLANDScaleXLow() { return m_physicsLANDScaleXLow; }
void HapticSettings::setPhysicsLANDScaleXLow(float value) { m_physicsLANDScaleXLow = value; }
float HapticSettings::getPhysicsLANDScaleYLow() { return m_physicsLANDScaleYLow; }
void HapticSettings::setPhysicsLANDScaleYLow(float value) { m_physicsLANDScaleYLow = value; }
float HapticSettings::getPhysicsLANDScaleZLow() { return m_physicsLANDScaleZLow; }
void HapticSettings::setPhysicsLANDScaleZLow(float value) { m_physicsLANDScaleZLow = value; }
float HapticSettings::getPhysicsLANDScaleXMed() { return m_physicsLANDScaleXMed; }
void HapticSettings::setPhysicsLANDScaleXMed(float value) { m_physicsLANDScaleXMed = value; }
float HapticSettings::getPhysicsLANDScaleYMed() { return m_physicsLANDScaleYMed; }
void HapticSettings::setPhysicsLANDScaleYMed(float value) { m_physicsLANDScaleYMed = value; }
float HapticSettings::getPhysicsLANDScaleZMed() { return m_physicsLANDScaleZMed; }
void HapticSettings::setPhysicsLANDScaleZMed(float value) { m_physicsLANDScaleZMed = value; }
float HapticSettings::getPhysicsLANDScaleXHigh() { return m_physicsLANDScaleXHigh; }
void HapticSettings::setPhysicsLANDScaleXHigh(float value) { m_physicsLANDScaleXHigh = value; }
float HapticSettings::getPhysicsLANDScaleYHigh() { return m_physicsLANDScaleYHigh; }
void HapticSettings::setPhysicsLANDScaleYHigh(float value) { m_physicsLANDScaleYHigh = value; }
float HapticSettings::getPhysicsLANDScaleZHigh() { return m_physicsLANDScaleZHigh; }
void HapticSettings::setPhysicsLANDScaleZHigh(float value) { m_physicsLANDScaleZHigh = value; }
float HapticSettings::getPhysicsLANDTransitionValue(int level)
{
    switch (level) { case 0: return getPhysicsLANDTransitionValueMed(); case 1: return getPhysicsLANDTransitionValueHigh(); default: return 0.0f; }
}
void HapticSettings::setPhysicsLANDTransitionValue(int level, float value)
{
    switch (level) { case 0: setPhysicsLANDTransitionValueMed(value); break; case 1: setPhysicsLANDTransitionValueHigh(value); break; default: break; }
}
float HapticSettings::getPhysicsLANDTransitionValueMed() { return m_physicsLANDTransitionValueMed; }
void HapticSettings::setPhysicsLANDTransitionValueMed(float value) { m_physicsLANDTransitionValueMed = value; }
float HapticSettings::getPhysicsLANDTransitionValueHigh() { return m_physicsLANDTransitionValueHigh; }
void HapticSettings::setPhysicsLANDTransitionValueHigh(float value) { m_physicsLANDTransitionValueHigh = value; }
int HapticSettings::getPhysicsLANDSmoothTimeX(int level)
{
    switch (level) { case 0: return getPhysicsLANDSmoothTimeXLow(); case 1: return getPhysicsLANDSmoothTimeXMed(); case 2: return getPhysicsLANDSmoothTimeXHigh(); default: return 0; }
}
void HapticSettings::setPhysicsLANDSmoothTimeX(int level, int value)
{
    switch (level) { case 0: setPhysicsLANDSmoothTimeXLow(value); break; case 1: setPhysicsLANDSmoothTimeXMed(value); break; case 2: setPhysicsLANDSmoothTimeXHigh(value); break; default: break; }
}
int HapticSettings::getPhysicsLANDSmoothTimeY(int level)
{
    switch (level) { case 0: return getPhysicsLANDSmoothTimeYLow(); case 1: return getPhysicsLANDSmoothTimeYMed(); case 2: return getPhysicsLANDSmoothTimeYHigh(); default: return 0; }
}
void HapticSettings::setPhysicsLANDSmoothTimeY(int level, int value)
{
    switch (level) { case 0: setPhysicsLANDSmoothTimeYLow(value); break; case 1: setPhysicsLANDSmoothTimeYMed(value); break; case 2: setPhysicsLANDSmoothTimeYHigh(value); break; default: break; }
}
int HapticSettings::getPhysicsLANDSmoothTimeZ(int level)
{
    switch (level) { case 0: return getPhysicsLANDSmoothTimeZLow(); case 1: return getPhysicsLANDSmoothTimeZMed(); case 2: return getPhysicsLANDSmoothTimeZHigh(); default: return 0; }
}
void HapticSettings::setPhysicsLANDSmoothTimeZ(int level, int value)
{
    switch (level) { case 0: setPhysicsLANDSmoothTimeZLow(value); break; case 1: setPhysicsLANDSmoothTimeZMed(value); break; case 2: setPhysicsLANDSmoothTimeZHigh(value); break; default: break; }
}
int HapticSettings::getPhysicsLANDSmoothTimeXLow() { return m_physicsLANDSmoothTimeXLow; }
void HapticSettings::setPhysicsLANDSmoothTimeXLow(int value) { m_physicsLANDSmoothTimeXLow = value; }
int HapticSettings::getPhysicsLANDSmoothTimeYLow() { return m_physicsLANDSmoothTimeYLow; }
void HapticSettings::setPhysicsLANDSmoothTimeYLow(int value) { m_physicsLANDSmoothTimeYLow = value; }
int HapticSettings::getPhysicsLANDSmoothTimeZLow() { return m_physicsLANDSmoothTimeZLow; }
void HapticSettings::setPhysicsLANDSmoothTimeZLow(int value) { m_physicsLANDSmoothTimeZLow = value; }
int HapticSettings::getPhysicsLANDSmoothTimeXMed() { return m_physicsLANDSmoothTimeXMed; }
void HapticSettings::setPhysicsLANDSmoothTimeXMed(int value) { m_physicsLANDSmoothTimeXMed = value; }
int HapticSettings::getPhysicsLANDSmoothTimeYMed() { return m_physicsLANDSmoothTimeYMed; }
void HapticSettings::setPhysicsLANDSmoothTimeYMed(int value) { m_physicsLANDSmoothTimeYMed = value; }
int HapticSettings::getPhysicsLANDSmoothTimeZMed() { return m_physicsLANDSmoothTimeZMed; }
void HapticSettings::setPhysicsLANDSmoothTimeZMed(int value) { m_physicsLANDSmoothTimeZMed = value; }
int HapticSettings::getPhysicsLANDSmoothTimeXHigh() { return m_physicsLANDSmoothTimeXHigh; }
void HapticSettings::setPhysicsLANDSmoothTimeXHigh(int value) { m_physicsLANDSmoothTimeXHigh = value; }
int HapticSettings::getPhysicsLANDSmoothTimeYHigh() { return m_physicsLANDSmoothTimeYHigh; }
void HapticSettings::setPhysicsLANDSmoothTimeYHigh(int value) { m_physicsLANDSmoothTimeYHigh = value; }
int HapticSettings::getPhysicsLANDSmoothTimeZHigh() { return m_physicsLANDSmoothTimeZHigh; }
void HapticSettings::setPhysicsLANDSmoothTimeZHigh(int value) { m_physicsLANDSmoothTimeZHigh = value; }
Vec3 HapticSettings::getPhysicsAIRScale() { return m_physicsAIRScale; }
void HapticSettings::setPhysicsAIRScale(float x, float y, float z) { m_physicsAIRScale.x = x; m_physicsAIRScale.y = y; m_physicsAIRScale.z = z; }
float HapticSettings::getPhysicsAIRScaleXLow() { return m_physicsAIRScaleXLow; }
void HapticSettings::setPhysicsAIRScaleXLow(float value) { m_physicsAIRScaleXLow = value; }
float HapticSettings::getPhysicsAIRScaleYLow() { return m_physicsAIRScaleYLow; }
void HapticSettings::setPhysicsAIRScaleYLow(float value) { m_physicsAIRScaleYLow = value; }
float HapticSettings::getPhysicsAIRScaleZLow() { return m_physicsAIRScaleZLow; }
void HapticSettings::setPhysicsAIRScaleZLow(float value) { m_physicsAIRScaleZLow = value; }
float HapticSettings::getPhysicsAIRScaleXMed() { return m_physicsAIRScaleXMed; }
void HapticSettings::setPhysicsAIRScaleXMed(float value) { m_physicsAIRScaleXMed = value; }
float HapticSettings::getPhysicsAIRScaleYMed() { return m_physicsAIRScaleYMed; }
void HapticSettings::setPhysicsAIRScaleYMed(float value) { m_physicsAIRScaleYMed = value; }
float HapticSettings::getPhysicsAIRScaleZMed() { return m_physicsAIRScaleZMed; }
void HapticSettings::setPhysicsAIRScaleZMed(float value) { m_physicsAIRScaleZMed = value; }
float HapticSettings::getPhysicsAIRScaleXHigh() { return m_physicsAIRScaleXHigh; }
void HapticSettings::setPhysicsAIRScaleXHigh(float value) { m_physicsAIRScaleXHigh = value; }
float HapticSettings::getPhysicsAIRScaleYHigh() { return m_physicsAIRScaleYHigh; }
void HapticSettings::setPhysicsAIRScaleYHigh(float value) { m_physicsAIRScaleYHigh = value; }
float HapticSettings::getPhysicsAIRScaleZHigh() { return m_physicsAIRScaleZHigh; }
void HapticSettings::setPhysicsAIRScaleZHigh(float value) { m_physicsAIRScaleZHigh = value; }
float HapticSettings::getPhysicsAIRTransitionValue(int level)
{
    switch (level) { case 0: return getPhysicsAIRTransitionValueMed(); case 1: return getPhysicsAIRTransitionValueHigh(); default: return 0.0f; }
}
void HapticSettings::setPhysicsAIRTransitionValue(int level, float value)
{
    switch (level) { case 0: setPhysicsAIRTransitionValueMed(value); break; case 1: setPhysicsAIRTransitionValueHigh(value); break; default: break; }
}
float HapticSettings::getPhysicsAIRTransitionValueMed() { return m_physicsAIRTransitionValueMed; }
void HapticSettings::setPhysicsAIRTransitionValueMed(float value) { m_physicsAIRTransitionValueMed = value; }
float HapticSettings::getPhysicsAIRTransitionValueHigh() { return m_physicsAIRTransitionValueHigh; }
void HapticSettings::setPhysicsAIRTransitionValueHigh(float value) { m_physicsAIRTransitionValueHigh = value; }
int HapticSettings::getPhysicsAIRSmoothTimeX(int level)
{
    switch (level) { case 0: return getPhysicsAIRSmoothTimeXLow(); case 1: return getPhysicsAIRSmoothTimeXMed(); case 2: return getPhysicsAIRSmoothTimeXHigh(); default: return 0; }
}
void HapticSettings::setPhysicsAIRSmoothTimeX(int level, int value)
{
    switch (level) { case 0: setPhysicsAIRSmoothTimeXLow(value); break; case 1: setPhysicsAIRSmoothTimeXMed(value); break; case 2: setPhysicsAIRSmoothTimeXHigh(value); break; default: break; }
}
int HapticSettings::getPhysicsAIRSmoothTimeY(int level)
{
    switch (level) { case 0: return getPhysicsAIRSmoothTimeYLow(); case 1: return getPhysicsAIRSmoothTimeYMed(); case 2: return getPhysicsAIRSmoothTimeYHigh(); default: return 0; }
}
void HapticSettings::setPhysicsAIRSmoothTimeY(int level, int value)
{
    switch (level) { case 0: setPhysicsAIRSmoothTimeYLow(value); break; case 1: setPhysicsAIRSmoothTimeYMed(value); break; case 2: setPhysicsAIRSmoothTimeYHigh(value); break; default: break; }
}
int HapticSettings::getPhysicsAIRSmoothTimeZ(int level)
{
    switch (level) { case 0: return getPhysicsAIRSmoothTimeZLow(); case 1: return getPhysicsAIRSmoothTimeZMed(); case 2: return getPhysicsAIRSmoothTimeZHigh(); default: return 0; }
}
void HapticSettings::setPhysicsAIRSmoothTimeZ(int level, int value)
{
    switch (level) { case 0: setPhysicsAIRSmoothTimeZLow(value); break; case 1: setPhysicsAIRSmoothTimeZMed(value); break; case 2: setPhysicsAIRSmoothTimeZHigh(value); break; default: break; }
}
int HapticSettings::getPhysicsAIRSmoothTimeXLow() { return m_physicsAIRSmoothTimeXLow; }
void HapticSettings::setPhysicsAIRSmoothTimeXLow(int value) { m_physicsAIRSmoothTimeXLow = value; }
int HapticSettings::getPhysicsAIRSmoothTimeYLow() { return m_physicsAIRSmoothTimeYLow; }
void HapticSettings::setPhysicsAIRSmoothTimeYLow(int value) { m_physicsAIRSmoothTimeYLow = value; }
int HapticSettings::getPhysicsAIRSmoothTimeZLow() { return m_physicsAIRSmoothTimeZLow; }
void HapticSettings::setPhysicsAIRSmoothTimeZLow(int value) { m_physicsAIRSmoothTimeZLow = value; }
int HapticSettings::getPhysicsAIRSmoothTimeXMed() { return m_physicsAIRSmoothTimeXMed; }
void HapticSettings::setPhysicsAIRSmoothTimeXMed(int value) { m_physicsAIRSmoothTimeXMed = value; }
int HapticSettings::getPhysicsAIRSmoothTimeYMed() { return m_physicsAIRSmoothTimeYMed; }
void HapticSettings::setPhysicsAIRSmoothTimeYMed(int value) { m_physicsAIRSmoothTimeYMed = value; }
int HapticSettings::getPhysicsAIRSmoothTimeZMed() { return m_physicsAIRSmoothTimeZMed; }
void HapticSettings::setPhysicsAIRSmoothTimeZMed(int value) { m_physicsAIRSmoothTimeZMed = value; }
int HapticSettings::getPhysicsAIRSmoothTimeXHigh() { return m_physicsAIRSmoothTimeXHigh; }
void HapticSettings::setPhysicsAIRSmoothTimeXHigh(int value) { m_physicsAIRSmoothTimeXHigh = value; }
int HapticSettings::getPhysicsAIRSmoothTimeYHigh() { return m_physicsAIRSmoothTimeYHigh; }
void HapticSettings::setPhysicsAIRSmoothTimeYHigh(int value) { m_physicsAIRSmoothTimeYHigh = value; }
int HapticSettings::getPhysicsAIRSmoothTimeZHigh() { return m_physicsAIRSmoothTimeZHigh; }
void HapticSettings::setPhysicsAIRSmoothTimeZHigh(int value) { m_physicsAIRSmoothTimeZHigh = value; }
Vec3 HapticSettings::getPhysicsHELIScale() { return m_physicsHELIScale; }
void HapticSettings::setPhysicsHELIScale(float x, float y, float z) { m_physicsHELIScale.x = x; m_physicsHELIScale.y = y; m_physicsHELIScale.z = z; }
float HapticSettings::getPhysicsHELIScaleXLow() { return m_physicsHELIScaleXLow; }
void HapticSettings::setPhysicsHELIScaleXLow(float value) { m_physicsHELIScaleXLow = value; }
float HapticSettings::getPhysicsHELIScaleYLow() { return m_physicsHELIScaleYLow; }
void HapticSettings::setPhysicsHELIScaleYLow(float value) { m_physicsHELIScaleYLow = value; }
float HapticSettings::getPhysicsHELIScaleZLow() { return m_physicsHELIScaleZLow; }
void HapticSettings::setPhysicsHELIScaleZLow(float value) { m_physicsHELIScaleZLow = value; }
float HapticSettings::getPhysicsHELIScaleXMed() { return m_physicsHELIScaleXMed; }
void HapticSettings::setPhysicsHELIScaleXMed(float value) { m_physicsHELIScaleXMed = value; }
float HapticSettings::getPhysicsHELIScaleYMed() { return m_physicsHELIScaleYMed; }
void HapticSettings::setPhysicsHELIScaleYMed(float value) { m_physicsHELIScaleYMed = value; }
float HapticSettings::getPhysicsHELIScaleZMed() { return m_physicsHELIScaleZMed; }
void HapticSettings::setPhysicsHELIScaleZMed(float value) { m_physicsHELIScaleZMed = value; }
float HapticSettings::getPhysicsHELIScaleXHigh() { return m_physicsHELIScaleXHigh; }
void HapticSettings::setPhysicsHELIScaleXHigh(float value) { m_physicsHELIScaleXHigh = value; }
float HapticSettings::getPhysicsHELIScaleYHigh() { return m_physicsHELIScaleYHigh; }
void HapticSettings::setPhysicsHELIScaleYHigh(float value) { m_physicsHELIScaleYHigh = value; }
float HapticSettings::getPhysicsHELIScaleZHigh() { return m_physicsHELIScaleZHigh; }
void HapticSettings::setPhysicsHELIScaleZHigh(float value) { m_physicsHELIScaleZHigh = value; }
float HapticSettings::getPhysicsHELITransitionValue(int level)
{
    switch (level) { case 0: return getPhysicsHELITransitionValueMed(); case 1: return getPhysicsHELITransitionValueHigh(); default: return 0.0f; }
}
void HapticSettings::setPhysicsHELITransitionValue(int level, float value)
{
    switch (level) { case 0: setPhysicsHELITransitionValueMed(value); break; case 1: setPhysicsHELITransitionValueHigh(value); break; default: break; }
}
float HapticSettings::getPhysicsHELITransitionValueMed() { return m_physicsHELITransitionValueMed; }
void HapticSettings::setPhysicsHELITransitionValueMed(float value) { m_physicsHELITransitionValueMed = value; }
float HapticSettings::getPhysicsHELITransitionValueHigh() { return m_physicsHELITransitionValueHigh; }
void HapticSettings::setPhysicsHELITransitionValueHigh(float value) { m_physicsHELITransitionValueHigh = value; }
int HapticSettings::getPhysicsHELISmoothTimeX(int level)
{
    switch (level) { case 0: return getPhysicsHELISmoothTimeXLow(); case 1: return getPhysicsHELISmoothTimeXMed(); case 2: return getPhysicsHELISmoothTimeXHigh(); default: return 0; }
}
void HapticSettings::setPhysicsHELISmoothTimeX(int level, int value)
{
    switch (level) { case 0: setPhysicsHELISmoothTimeXLow(value); break; case 1: setPhysicsHELISmoothTimeXMed(value); break; case 2: setPhysicsHELISmoothTimeXHigh(value); break; default: break; }
}
int HapticSettings::getPhysicsHELISmoothTimeY(int level)
{
    switch (level) { case 0: return getPhysicsHELISmoothTimeYLow(); case 1: return getPhysicsHELISmoothTimeYMed(); case 2: return getPhysicsHELISmoothTimeYHigh(); default: return 0; }
}
void HapticSettings::setPhysicsHELISmoothTimeY(int level, int value)
{
    switch (level) { case 0: setPhysicsHELISmoothTimeYLow(value); break; case 1: setPhysicsHELISmoothTimeYMed(value); break; case 2: setPhysicsHELISmoothTimeYHigh(value); break; default: break; }
}
int HapticSettings::getPhysicsHELISmoothTimeZ(int level)
{
    switch (level) { case 0: return getPhysicsHELISmoothTimeZLow(); case 1: return getPhysicsHELISmoothTimeZMed(); case 2: return getPhysicsHELISmoothTimeZHigh(); default: return 0; }
}
void HapticSettings::setPhysicsHELISmoothTimeZ(int level, int value)
{
    switch (level) { case 0: setPhysicsHELISmoothTimeZLow(value); break; case 1: setPhysicsHELISmoothTimeZMed(value); break; case 2: setPhysicsHELISmoothTimeZHigh(value); break; default: break; }
}
int HapticSettings::getPhysicsHELISmoothTimeXLow() { return m_physicsHELISmoothTimeXLow; }
void HapticSettings::setPhysicsHELISmoothTimeXLow(int value) { m_physicsHELISmoothTimeXLow = value; }
int HapticSettings::getPhysicsHELISmoothTimeYLow() { return m_physicsHELISmoothTimeYLow; }
void HapticSettings::setPhysicsHELISmoothTimeYLow(int value) { m_physicsHELISmoothTimeYLow = value; }
int HapticSettings::getPhysicsHELISmoothTimeZLow() { return m_physicsHELISmoothTimeZLow; }
void HapticSettings::setPhysicsHELISmoothTimeZLow(int value) { m_physicsHELISmoothTimeZLow = value; }
int HapticSettings::getPhysicsHELISmoothTimeXMed() { return m_physicsHELISmoothTimeXMed; }
void HapticSettings::setPhysicsHELISmoothTimeXMed(int value) { m_physicsHELISmoothTimeXMed = value; }
int HapticSettings::getPhysicsHELISmoothTimeYMed() { return m_physicsHELISmoothTimeYMed; }
void HapticSettings::setPhysicsHELISmoothTimeYMed(int value) { m_physicsHELISmoothTimeYMed = value; }
int HapticSettings::getPhysicsHELISmoothTimeZMed() { return m_physicsHELISmoothTimeZMed; }
void HapticSettings::setPhysicsHELISmoothTimeZMed(int value) { m_physicsHELISmoothTimeZMed = value; }
int HapticSettings::getPhysicsHELISmoothTimeXHigh() { return m_physicsHELISmoothTimeXHigh; }
void HapticSettings::setPhysicsHELISmoothTimeXHigh(int value) { m_physicsHELISmoothTimeXHigh = value; }
int HapticSettings::getPhysicsHELISmoothTimeYHigh() { return m_physicsHELISmoothTimeYHigh; }
void HapticSettings::setPhysicsHELISmoothTimeYHigh(int value) { m_physicsHELISmoothTimeYHigh = value; }
int HapticSettings::getPhysicsHELISmoothTimeZHigh() { return m_physicsHELISmoothTimeZHigh; }
void HapticSettings::setPhysicsHELISmoothTimeZHigh(int value) { m_physicsHELISmoothTimeZHigh = value; }
Vec3 HapticSettings::getPhysicsSEAScale() { return m_physicsSEAScale; }
void HapticSettings::setPhysicsSEAScale(float x, float y, float z) { m_physicsSEAScale.x = x; m_physicsSEAScale.y = y; m_physicsSEAScale.z = z; }
float HapticSettings::getPhysicsSEAScaleXLow() { return m_physicsSEAScaleXLow; }
void HapticSettings::setPhysicsSEAScaleXLow(float value) { m_physicsSEAScaleXLow = value; }
float HapticSettings::getPhysicsSEAScaleYLow() { return m_physicsSEAScaleYLow; }
void HapticSettings::setPhysicsSEAScaleYLow(float value) { m_physicsSEAScaleYLow = value; }
float HapticSettings::getPhysicsSEAScaleZLow() { return m_physicsSEAScaleZLow; }
void HapticSettings::setPhysicsSEAScaleZLow(float value) { m_physicsSEAScaleZLow = value; }
float HapticSettings::getPhysicsSEAScaleXMed() { return m_physicsSEAScaleXMed; }
void HapticSettings::setPhysicsSEAScaleXMed(float value) { m_physicsSEAScaleXMed = value; }
float HapticSettings::getPhysicsSEAScaleYMed() { return m_physicsSEAScaleYMed; }
void HapticSettings::setPhysicsSEAScaleYMed(float value) { m_physicsSEAScaleYMed = value; }
float HapticSettings::getPhysicsSEAScaleZMed() { return m_physicsSEAScaleZMed; }
void HapticSettings::setPhysicsSEAScaleZMed(float value) { m_physicsSEAScaleZMed = value; }
float HapticSettings::getPhysicsSEAScaleXHigh() { return m_physicsSEAScaleXHigh; }
void HapticSettings::setPhysicsSEAScaleXHigh(float value) { m_physicsSEAScaleXHigh = value; }
float HapticSettings::getPhysicsSEAScaleYHigh() { return m_physicsSEAScaleYHigh; }
void HapticSettings::setPhysicsSEAScaleYHigh(float value) { m_physicsSEAScaleYHigh = value; }
float HapticSettings::getPhysicsSEAScaleZHigh() { return m_physicsSEAScaleZHigh; }
void HapticSettings::setPhysicsSEAScaleZHigh(float value) { m_physicsSEAScaleZHigh = value; }
float HapticSettings::getPhysicsSEATransitionValue(int level)
{
    switch (level) { case 0: return getPhysicsSEATransitionValueMed(); case 1: return getPhysicsSEATransitionValueHigh(); default: return 0.0f; }
}
void HapticSettings::setPhysicsSEATransitionValue(int level, float value)
{
    switch (level) { case 0: setPhysicsSEATransitionValueMed(value); break; case 1: setPhysicsSEATransitionValueHigh(value); break; default: break; }
}
float HapticSettings::getPhysicsSEATransitionValueMed() { return m_physicsSEATransitionValueMed; }
void HapticSettings::setPhysicsSEATransitionValueMed(float value) { m_physicsSEATransitionValueMed = value; }
float HapticSettings::getPhysicsSEATransitionValueHigh() { return m_physicsSEATransitionValueHigh; }
void HapticSettings::setPhysicsSEATransitionValueHigh(float value) { m_physicsSEATransitionValueHigh = value; }
int HapticSettings::getPhysicsSEASmoothTimeX(int level)
{
    switch (level) { case 0: return getPhysicsSEASmoothTimeXLow(); case 1: return getPhysicsSEASmoothTimeXMed(); case 2: return getPhysicsSEASmoothTimeXHigh(); default: return 0; }
}
void HapticSettings::setPhysicsSEASmoothTimeX(int level, int value)
{
    switch (level) { case 0: setPhysicsSEASmoothTimeXLow(value); break; case 1: setPhysicsSEASmoothTimeXMed(value); break; case 2: setPhysicsSEASmoothTimeXHigh(value); break; default: break; }
}
int HapticSettings::getPhysicsSEASmoothTimeY(int level)
{
    switch (level) { case 0: return getPhysicsSEASmoothTimeYLow(); case 1: return getPhysicsSEASmoothTimeYMed(); case 2: return getPhysicsSEASmoothTimeYHigh(); default: return 0; }
}
void HapticSettings::setPhysicsSEASmoothTimeY(int level, int value)
{
    switch (level) { case 0: setPhysicsSEASmoothTimeYLow(value); break; case 1: setPhysicsSEASmoothTimeYMed(value); break; case 2: setPhysicsSEASmoothTimeYHigh(value); break; default: break; }
}
int HapticSettings::getPhysicsSEASmoothTimeZ(int level)
{
    switch (level) { case 0: return getPhysicsSEASmoothTimeZLow(); case 1: return getPhysicsSEASmoothTimeZMed(); case 2: return getPhysicsSEASmoothTimeZHigh(); default: return 0; }
}
void HapticSettings::setPhysicsSEASmoothTimeZ(int level, int value)
{
    switch (level) { case 0: setPhysicsSEASmoothTimeZLow(value); break; case 1: setPhysicsSEASmoothTimeZMed(value); break; case 2: setPhysicsSEASmoothTimeZHigh(value); break; default: break; }
}
int HapticSettings::getPhysicsSEASmoothTimeXLow() { return m_physicsSEASmoothTimeXLow; }
void HapticSettings::setPhysicsSEASmoothTimeXLow(int value) { m_physicsSEASmoothTimeXLow = value; }
int HapticSettings::getPhysicsSEASmoothTimeYLow() { return m_physicsSEASmoothTimeYLow; }
void HapticSettings::setPhysicsSEASmoothTimeYLow(int value) { m_physicsSEASmoothTimeYLow = value; }
int HapticSettings::getPhysicsSEASmoothTimeZLow() { return m_physicsSEASmoothTimeZLow; }
void HapticSettings::setPhysicsSEASmoothTimeZLow(int value) { m_physicsSEASmoothTimeZLow = value; }
int HapticSettings::getPhysicsSEASmoothTimeXMed() { return m_physicsSEASmoothTimeXMed; }
void HapticSettings::setPhysicsSEASmoothTimeXMed(int value) { m_physicsSEASmoothTimeXMed = value; }
int HapticSettings::getPhysicsSEASmoothTimeYMed() { return m_physicsSEASmoothTimeYMed; }
void HapticSettings::setPhysicsSEASmoothTimeYMed(int value) { m_physicsSEASmoothTimeYMed = value; }
int HapticSettings::getPhysicsSEASmoothTimeZMed() { return m_physicsSEASmoothTimeZMed; }
void HapticSettings::setPhysicsSEASmoothTimeZMed(int value) { m_physicsSEASmoothTimeZMed = value; }
int HapticSettings::getPhysicsSEASmoothTimeXHigh() { return m_physicsSEASmoothTimeXHigh; }
void HapticSettings::setPhysicsSEASmoothTimeXHigh(int value) { m_physicsSEASmoothTimeXHigh = value; }
int HapticSettings::getPhysicsSEASmoothTimeYHigh() { return m_physicsSEASmoothTimeYHigh; }
void HapticSettings::setPhysicsSEASmoothTimeYHigh(int value) { m_physicsSEASmoothTimeYHigh = value; }
int HapticSettings::getPhysicsSEASmoothTimeZHigh() { return m_physicsSEASmoothTimeZHigh; }
void HapticSettings::setPhysicsSEASmoothTimeZHigh(int value) { m_physicsSEASmoothTimeZHigh = value; }
float HapticSettings::getDamageScale() { return m_damageScale; }
void HapticSettings::setDamageScale(float value) { m_damageScale = value; }
float HapticSettings::getRecoilPunchScale(int weaponClass)
{
    switch (weaponClass) {
    case 0: return getRecoilPunchScaleRIFLE();
    case 1: return getRecoilPunchScaleRIFLELAUNCHER();
    case 2: return getRecoilPunchScaleCARBINE();
    case 3: return getRecoilPunchScaleLMG();
    case 4: return getRecoilPunchScaleSNIPER();
    case 6: return getRecoilPunchScaleLAUNCHER();
    case 7: return getRecoilPunchScaleSMG();
    case 8: return getRecoilPunchScaleSHOTGUN();
    case 9: return getRecoilPunchScaleKNIFE();
    case 11: return getRecoilPunchScaleDROP();
    case 12: return getRecoilPunchScaleTHROWN();
    case 100: return getRecoilPunchScaleV_LMG();
    case 101: return getRecoilPunchScaleV_HMG();
    case 102: return getRecoilPunchScaleV_HELIGUN();
    case 103: return getRecoilPunchScaleV_AIRGUN();
    case 104: return getRecoilPunchScaleV_MISSILE();
    case 105: return getRecoilPunchScaleV_ARMORCANNON();
    case 106: return getRecoilPunchScaleV_COAXIALGUN();
    default: return getRecoilPunchScalePISTOL();
    }
}
void HapticSettings::setRecoilPunchScale(int weaponClass, float value)
{
    switch (weaponClass) {
    case 0: setRecoilPunchScaleRIFLE(value); break;
    case 1: setRecoilPunchScaleRIFLELAUNCHER(value); break;
    case 2: setRecoilPunchScaleCARBINE(value); break;
    case 3: setRecoilPunchScaleLMG(value); break;
    case 4: setRecoilPunchScaleSNIPER(value); break;
    case 5: case 10: setRecoilPunchScalePISTOL(value); break;
    case 6: setRecoilPunchScaleLAUNCHER(value); break;
    case 7: setRecoilPunchScaleSMG(value); break;
    case 8: setRecoilPunchScaleSHOTGUN(value); break;
    case 9: setRecoilPunchScaleKNIFE(value); break;
    case 11: setRecoilPunchScaleDROP(value); break;
    case 12: setRecoilPunchScaleTHROWN(value); break;
    case 100: setRecoilPunchScaleV_LMG(value); break;
    case 101: setRecoilPunchScaleV_HMG(value); break;
    case 102: setRecoilPunchScaleV_HELIGUN(value); break;
    case 103: setRecoilPunchScaleV_AIRGUN(value); break;
    case 104: setRecoilPunchScaleV_MISSILE(value); break;
    case 105: setRecoilPunchScaleV_ARMORCANNON(value); break;
    case 106: setRecoilPunchScaleV_COAXIALGUN(value); break;
    default: break;
    }
}
float HapticSettings::getRecoilPunchTime(int weaponClass)
{
    switch (weaponClass) {
    case 0: return getRecoilPunchTimeRIFLE();
    case 1: return getRecoilPunchTimeRIFLELAUNCHER();
    case 2: return getRecoilPunchTimeCARBINE();
    case 3: return getRecoilPunchTimeLMG();
    case 4: return getRecoilPunchTimeSNIPER();
    case 6: return getRecoilPunchTimeLAUNCHER();
    case 7: return getRecoilPunchTimeSMG();
    case 8: return getRecoilPunchTimeSHOTGUN();
    case 9: return getRecoilPunchTimeKNIFE();
    case 11: return getRecoilPunchTimeDROP();
    case 12: return getRecoilPunchTimeTHROWN();
    case 100: return getRecoilPunchTimeV_LMG();
    case 101: return getRecoilPunchTimeV_HMG();
    case 102: return getRecoilPunchTimeV_HELIGUN();
    case 103: return getRecoilPunchTimeV_AIRGUN();
    case 104: return getRecoilPunchTimeV_MISSILE();
    case 105: return getRecoilPunchTimeV_ARMORCANNON();
    case 106: return getRecoilPunchTimeV_COAXIALGUN();
    default: return getRecoilPunchTimePISTOL();
    }
}
void HapticSettings::setRecoilPunchTime(int weaponClass, float value)
{
    switch (weaponClass) {
    case 0: setRecoilPunchTimeRIFLE(value); break;
    case 1: setRecoilPunchTimeRIFLELAUNCHER(value); break;
    case 2: setRecoilPunchTimeCARBINE(value); break;
    case 3: setRecoilPunchTimeLMG(value); break;
    case 4: setRecoilPunchTimeSNIPER(value); break;
    case 5: case 10: setRecoilPunchTimePISTOL(value); break;
    case 6: setRecoilPunchTimeLAUNCHER(value); break;
    case 7: setRecoilPunchTimeSMG(value); break;
    case 8: setRecoilPunchTimeSHOTGUN(value); break;
    case 9: setRecoilPunchTimeKNIFE(value); break;
    case 11: setRecoilPunchTimeDROP(value); break;
    case 12: setRecoilPunchTimeTHROWN(value); break;
    case 100: setRecoilPunchTimeV_LMG(value); break;
    case 101: setRecoilPunchTimeV_HMG(value); break;
    case 102: setRecoilPunchTimeV_HELIGUN(value); break;
    case 103: setRecoilPunchTimeV_AIRGUN(value); break;
    case 104: setRecoilPunchTimeV_MISSILE(value); break;
    case 105: setRecoilPunchTimeV_ARMORCANNON(value); break;
    case 106: setRecoilPunchTimeV_COAXIALGUN(value); break;
    default: break;
    }
}
float HapticSettings::getRecoilYawScale(int weaponClass)
{
    switch (weaponClass) {
    case 0: return getRecoilYawScaleRIFLE();
    case 1: return getRecoilYawScaleRIFLELAUNCHER();
    case 2: return getRecoilYawScaleCARBINE();
    case 3: return getRecoilYawScaleLMG();
    case 4: return getRecoilYawScaleSNIPER();
    case 6: return getRecoilYawScaleLAUNCHER();
    case 7: return getRecoilYawScaleSMG();
    case 8: return getRecoilYawScaleSHOTGUN();
    case 9: return getRecoilYawScaleKNIFE();
    case 11: return getRecoilYawScaleDROP();
    case 12: return getRecoilYawScaleTHROWN();
    case 100: return getRecoilYawScaleV_LMG();
    case 101: return getRecoilYawScaleV_HMG();
    case 102: return getRecoilYawScaleV_HELIGUN();
    case 103: return getRecoilYawScaleV_AIRGUN();
    case 104: return getRecoilYawScaleV_MISSILE();
    case 105: return getRecoilYawScaleV_ARMORCANNON();
    case 106: return getRecoilYawScaleV_COAXIALGUN();
    default: return getRecoilYawScalePISTOL();
    }
}
void HapticSettings::setRecoilYawScale(int weaponClass, float value)
{
    switch (weaponClass) {
    case 0: setRecoilYawScaleRIFLE(value); break;
    case 1: setRecoilYawScaleRIFLELAUNCHER(value); break;
    case 2: setRecoilYawScaleCARBINE(value); break;
    case 3: setRecoilYawScaleLMG(value); break;
    case 4: setRecoilYawScaleSNIPER(value); break;
    case 5: case 10: setRecoilYawScalePISTOL(value); break;
    case 6: setRecoilYawScaleLAUNCHER(value); break;
    case 7: setRecoilYawScaleSMG(value); break;
    case 8: setRecoilYawScaleSHOTGUN(value); break;
    case 9: setRecoilYawScaleKNIFE(value); break;
    case 11: setRecoilYawScaleDROP(value); break;
    case 12: setRecoilYawScaleTHROWN(value); break;
    case 100: setRecoilYawScaleV_LMG(value); break;
    case 101: setRecoilYawScaleV_HMG(value); break;
    case 102: setRecoilYawScaleV_HELIGUN(value); break;
    case 103: setRecoilYawScaleV_AIRGUN(value); break;
    case 104: setRecoilYawScaleV_MISSILE(value); break;
    case 105: setRecoilYawScaleV_ARMORCANNON(value); break;
    case 106: setRecoilYawScaleV_COAXIALGUN(value); break;
    default: break;
    }
}
float HapticSettings::getRecoilPitchScale(int weaponClass)
{
    switch (weaponClass) {
    case 0: return getRecoilPitchScaleRIFLE();
    case 1: return getRecoilPitchScaleRIFLELAUNCHER();
    case 2: return getRecoilPitchScaleCARBINE();
    case 3: return getRecoilPitchScaleLMG();
    case 4: return getRecoilPitchScaleSNIPER();
    case 6: return getRecoilPitchScaleLAUNCHER();
    case 7: return getRecoilPitchScaleSMG();
    case 8: return getRecoilPitchScaleSHOTGUN();
    case 9: return getRecoilPitchScaleKNIFE();
    case 11: return getRecoilPitchScaleDROP();
    case 12: return getRecoilPitchScaleTHROWN();
    case 100: return getRecoilPitchScaleV_LMG();
    case 101: return getRecoilPitchScaleV_HMG();
    case 102: return getRecoilPitchScaleV_HELIGUN();
    case 103: return getRecoilPitchScaleV_AIRGUN();
    case 104: return getRecoilPitchScaleV_MISSILE();
    case 105: return getRecoilPitchScaleV_ARMORCANNON();
    case 106: return getRecoilPitchScaleV_COAXIALGUN();
    default: return getRecoilPitchScalePISTOL();
    }
}
void HapticSettings::setRecoilPitchScale(int weaponClass, float value)
{
    switch (weaponClass) {
    case 0: setRecoilPitchScaleRIFLE(value); break;
    case 1: setRecoilPitchScaleRIFLELAUNCHER(value); break;
    case 2: setRecoilPitchScaleCARBINE(value); break;
    case 3: setRecoilPitchScaleLMG(value); break;
    case 4: setRecoilPitchScaleSNIPER(value); break;
    case 5: case 10: setRecoilPitchScalePISTOL(value); break;
    case 6: setRecoilPitchScaleLAUNCHER(value); break;
    case 7: setRecoilPitchScaleSMG(value); break;
    case 8: setRecoilPitchScaleSHOTGUN(value); break;
    case 9: setRecoilPitchScaleKNIFE(value); break;
    case 11: setRecoilPitchScaleDROP(value); break;
    case 12: setRecoilPitchScaleTHROWN(value); break;
    case 100: setRecoilPitchScaleV_LMG(value); break;
    case 101: setRecoilPitchScaleV_HMG(value); break;
    case 102: setRecoilPitchScaleV_HELIGUN(value); break;
    case 103: setRecoilPitchScaleV_AIRGUN(value); break;
    case 104: setRecoilPitchScaleV_MISSILE(value); break;
    case 105: setRecoilPitchScaleV_ARMORCANNON(value); break;
    case 106: setRecoilPitchScaleV_COAXIALGUN(value); break;
    default: break;
    }
}
float HapticSettings::getRecoilPunchScaleRIFLE() { return m_recoilPunchScaleRIFLE; }
void HapticSettings::setRecoilPunchScaleRIFLE(float value) { m_recoilPunchScaleRIFLE = value; }
float HapticSettings::getRecoilPunchTimeRIFLE() { return m_recoilPunchTimeRIFLE; }
void HapticSettings::setRecoilPunchTimeRIFLE(float value) { m_recoilPunchTimeRIFLE = value; }
float HapticSettings::getRecoilYawScaleRIFLE() { return m_recoilYawScaleRIFLE; }
void HapticSettings::setRecoilYawScaleRIFLE(float value) { m_recoilYawScaleRIFLE = value; }
float HapticSettings::getRecoilPitchScaleRIFLE() { return m_recoilPitchScaleRIFLE; }
void HapticSettings::setRecoilPitchScaleRIFLE(float value) { m_recoilPitchScaleRIFLE = value; }
float HapticSettings::getRecoilPunchScaleRIFLELAUNCHER() { return m_recoilPunchScaleRIFLELAUNCHER; }
void HapticSettings::setRecoilPunchScaleRIFLELAUNCHER(float value) { m_recoilPunchScaleRIFLELAUNCHER = value; }
float HapticSettings::getRecoilPunchTimeRIFLELAUNCHER() { return m_recoilPunchTimeRIFLELAUNCHER; }
void HapticSettings::setRecoilPunchTimeRIFLELAUNCHER(float value) { m_recoilPunchTimeRIFLELAUNCHER = value; }
float HapticSettings::getRecoilYawScaleRIFLELAUNCHER() { return m_recoilYawScaleRIFLELAUNCHER; }
void HapticSettings::setRecoilYawScaleRIFLELAUNCHER(float value) { m_recoilYawScaleRIFLELAUNCHER = value; }
float HapticSettings::getRecoilPitchScaleRIFLELAUNCHER() { return m_recoilPitchScaleRIFLELAUNCHER; }
void HapticSettings::setRecoilPitchScaleRIFLELAUNCHER(float value) { m_recoilPitchScaleRIFLELAUNCHER = value; }
float HapticSettings::getRecoilPunchScaleCARBINE() { return m_recoilPunchScaleCARBINE; }
void HapticSettings::setRecoilPunchScaleCARBINE(float value) { m_recoilPunchScaleCARBINE = value; }
float HapticSettings::getRecoilPunchTimeCARBINE() { return m_recoilPunchTimeCARBINE; }
void HapticSettings::setRecoilPunchTimeCARBINE(float value) { m_recoilPunchTimeCARBINE = value; }
float HapticSettings::getRecoilYawScaleCARBINE() { return m_recoilYawScaleCARBINE; }
void HapticSettings::setRecoilYawScaleCARBINE(float value) { m_recoilYawScaleCARBINE = value; }
float HapticSettings::getRecoilPitchScaleCARBINE() { return m_recoilPitchScaleCARBINE; }
void HapticSettings::setRecoilPitchScaleCARBINE(float value) { m_recoilPitchScaleCARBINE = value; }
float HapticSettings::getRecoilPunchScaleLMG() { return m_recoilPunchScaleLMG; }
void HapticSettings::setRecoilPunchScaleLMG(float value) { m_recoilPunchScaleLMG = value; }
float HapticSettings::getRecoilPunchTimeLMG() { return m_recoilPunchTimeLMG; }
void HapticSettings::setRecoilPunchTimeLMG(float value) { m_recoilPunchTimeLMG = value; }
float HapticSettings::getRecoilYawScaleLMG() { return m_recoilYawScaleLMG; }
void HapticSettings::setRecoilYawScaleLMG(float value) { m_recoilYawScaleLMG = value; }
float HapticSettings::getRecoilPitchScaleLMG() { return m_recoilPitchScaleLMG; }
void HapticSettings::setRecoilPitchScaleLMG(float value) { m_recoilPitchScaleLMG = value; }
float HapticSettings::getRecoilPunchScaleSNIPER() { return m_recoilPunchScaleSNIPER; }
void HapticSettings::setRecoilPunchScaleSNIPER(float value) { m_recoilPunchScaleSNIPER = value; }
float HapticSettings::getRecoilPunchTimeSNIPER() { return m_recoilPunchTimeSNIPER; }
void HapticSettings::setRecoilPunchTimeSNIPER(float value) { m_recoilPunchTimeSNIPER = value; }
float HapticSettings::getRecoilYawScaleSNIPER() { return m_recoilYawScaleSNIPER; }
void HapticSettings::setRecoilYawScaleSNIPER(float value) { m_recoilYawScaleSNIPER = value; }
float HapticSettings::getRecoilPitchScaleSNIPER() { return m_recoilPitchScaleSNIPER; }
void HapticSettings::setRecoilPitchScaleSNIPER(float value) { m_recoilPitchScaleSNIPER = value; }
float HapticSettings::getRecoilPunchScalePISTOL() { return m_recoilPunchScalePISTOL; }
void HapticSettings::setRecoilPunchScalePISTOL(float value) { m_recoilPunchScalePISTOL = value; }
float HapticSettings::getRecoilPunchTimePISTOL() { return m_recoilPunchTimePISTOL; }
void HapticSettings::setRecoilPunchTimePISTOL(float value) { m_recoilPunchTimePISTOL = value; }
float HapticSettings::getRecoilYawScalePISTOL() { return m_recoilYawScalePISTOL; }
void HapticSettings::setRecoilYawScalePISTOL(float value) { m_recoilYawScalePISTOL = value; }
float HapticSettings::getRecoilPitchScalePISTOL() { return m_recoilPitchScalePISTOL; }
void HapticSettings::setRecoilPitchScalePISTOL(float value) { m_recoilPitchScalePISTOL = value; }
float HapticSettings::getRecoilPunchScaleLAUNCHER() { return m_recoilPunchScaleLAUNCHER; }
void HapticSettings::setRecoilPunchScaleLAUNCHER(float value) { m_recoilPunchScaleLAUNCHER = value; }
float HapticSettings::getRecoilPunchTimeLAUNCHER() { return m_recoilPunchTimeLAUNCHER; }
void HapticSettings::setRecoilPunchTimeLAUNCHER(float value) { m_recoilPunchTimeLAUNCHER = value; }
float HapticSettings::getRecoilYawScaleLAUNCHER() { return m_recoilYawScaleLAUNCHER; }
void HapticSettings::setRecoilYawScaleLAUNCHER(float value) { m_recoilYawScaleLAUNCHER = value; }
float HapticSettings::getRecoilPitchScaleLAUNCHER() { return m_recoilPitchScaleLAUNCHER; }
void HapticSettings::setRecoilPitchScaleLAUNCHER(float value) { m_recoilPitchScaleLAUNCHER = value; }
float HapticSettings::getRecoilPunchScaleSMG() { return m_recoilPunchScaleSMG; }
void HapticSettings::setRecoilPunchScaleSMG(float value) { m_recoilPunchScaleSMG = value; }
float HapticSettings::getRecoilPunchTimeSMG() { return m_recoilPunchTimeSMG; }
void HapticSettings::setRecoilPunchTimeSMG(float value) { m_recoilPunchTimeSMG = value; }
float HapticSettings::getRecoilYawScaleSMG() { return m_recoilYawScaleSMG; }
void HapticSettings::setRecoilYawScaleSMG(float value) { m_recoilYawScaleSMG = value; }
float HapticSettings::getRecoilPitchScaleSMG() { return m_recoilPitchScaleSMG; }
void HapticSettings::setRecoilPitchScaleSMG(float value) { m_recoilPitchScaleSMG = value; }
float HapticSettings::getRecoilPunchScaleSHOTGUN() { return m_recoilPunchScaleSHOTGUN; }
void HapticSettings::setRecoilPunchScaleSHOTGUN(float value) { m_recoilPunchScaleSHOTGUN = value; }
float HapticSettings::getRecoilPunchTimeSHOTGUN() { return m_recoilPunchTimeSHOTGUN; }
void HapticSettings::setRecoilPunchTimeSHOTGUN(float value) { m_recoilPunchTimeSHOTGUN = value; }
float HapticSettings::getRecoilYawScaleSHOTGUN() { return m_recoilYawScaleSHOTGUN; }
void HapticSettings::setRecoilYawScaleSHOTGUN(float value) { m_recoilYawScaleSHOTGUN = value; }
float HapticSettings::getRecoilPitchScaleSHOTGUN() { return m_recoilPitchScaleSHOTGUN; }
void HapticSettings::setRecoilPitchScaleSHOTGUN(float value) { m_recoilPitchScaleSHOTGUN = value; }
float HapticSettings::getRecoilPunchScaleKNIFE() { return m_recoilPunchScaleKNIFE; }
void HapticSettings::setRecoilPunchScaleKNIFE(float value) { m_recoilPunchScaleKNIFE = value; }
float HapticSettings::getRecoilPunchTimeKNIFE() { return m_recoilPunchTimeKNIFE; }
void HapticSettings::setRecoilPunchTimeKNIFE(float value) { m_recoilPunchTimeKNIFE = value; }
float HapticSettings::getRecoilYawScaleKNIFE() { return m_recoilYawScaleKNIFE; }
void HapticSettings::setRecoilYawScaleKNIFE(float value) { m_recoilYawScaleKNIFE = value; }
float HapticSettings::getRecoilPitchScaleKNIFE() { return m_recoilPitchScaleKNIFE; }
void HapticSettings::setRecoilPitchScaleKNIFE(float value) { m_recoilPitchScaleKNIFE = value; }
float HapticSettings::getRecoilPunchScaleDEFIB() { return m_recoilPunchScaleDEFIB; }
void HapticSettings::setRecoilPunchScaleDEFIB(float value) { m_recoilPunchScaleDEFIB = value; }
float HapticSettings::getRecoilPunchTimeDEFIB() { return m_recoilPunchTimeDEFIB; }
void HapticSettings::setRecoilPunchTimeDEFIB(float value) { m_recoilPunchTimeDEFIB = value; }
float HapticSettings::getRecoilYawScaleDEFIB() { return m_recoilYawScaleDEFIB; }
void HapticSettings::setRecoilYawScaleDEFIB(float value) { m_recoilYawScaleDEFIB = value; }
float HapticSettings::getRecoilPitchScaleDEFIB() { return m_recoilPitchScaleDEFIB; }
void HapticSettings::setRecoilPitchScaleDEFIB(float value) { m_recoilPitchScaleDEFIB = value; }
float HapticSettings::getRecoilPunchScaleDROP() { return m_recoilPunchScaleDROP; }
void HapticSettings::setRecoilPunchScaleDROP(float value) { m_recoilPunchScaleDROP = value; }
float HapticSettings::getRecoilPunchTimeDROP() { return m_recoilPunchTimeDROP; }
void HapticSettings::setRecoilPunchTimeDROP(float value) { m_recoilPunchTimeDROP = value; }
float HapticSettings::getRecoilYawScaleDROP() { return m_recoilYawScaleDROP; }
void HapticSettings::setRecoilYawScaleDROP(float value) { m_recoilYawScaleDROP = value; }
float HapticSettings::getRecoilPitchScaleDROP() { return m_recoilPitchScaleDROP; }
void HapticSettings::setRecoilPitchScaleDROP(float value) { m_recoilPitchScaleDROP = value; }
float HapticSettings::getRecoilPunchScaleTHROWN() { return m_recoilPunchScaleTHROWN; }
void HapticSettings::setRecoilPunchScaleTHROWN(float value) { m_recoilPunchScaleTHROWN = value; }
float HapticSettings::getRecoilPunchTimeTHROWN() { return m_recoilPunchTimeTHROWN; }
void HapticSettings::setRecoilPunchTimeTHROWN(float value) { m_recoilPunchTimeTHROWN = value; }
float HapticSettings::getRecoilYawScaleTHROWN() { return m_recoilYawScaleTHROWN; }
void HapticSettings::setRecoilYawScaleTHROWN(float value) { m_recoilYawScaleTHROWN = value; }
float HapticSettings::getRecoilPitchScaleTHROWN() { return m_recoilPitchScaleTHROWN; }
void HapticSettings::setRecoilPitchScaleTHROWN(float value) { m_recoilPitchScaleTHROWN = value; }
float HapticSettings::getRecoilPunchScaleV_LMG() { return m_recoilPunchScaleV_LMG; }
void HapticSettings::setRecoilPunchScaleV_LMG(float value) { m_recoilPunchScaleV_LMG = value; }
float HapticSettings::getRecoilPunchTimeV_LMG() { return m_recoilPunchTimeV_LMG; }
void HapticSettings::setRecoilPunchTimeV_LMG(float value) { m_recoilPunchTimeV_LMG = value; }
float HapticSettings::getRecoilYawScaleV_LMG() { return m_recoilYawScaleV_LMG; }
void HapticSettings::setRecoilYawScaleV_LMG(float value) { m_recoilYawScaleV_LMG = value; }
float HapticSettings::getRecoilPitchScaleV_LMG() { return m_recoilPitchScaleV_LMG; }
void HapticSettings::setRecoilPitchScaleV_LMG(float value) { m_recoilPitchScaleV_LMG = value; }
float HapticSettings::getRecoilPunchScaleV_HMG() { return m_recoilPunchScaleV_HMG; }
void HapticSettings::setRecoilPunchScaleV_HMG(float value) { m_recoilPunchScaleV_HMG = value; }
float HapticSettings::getRecoilPunchTimeV_HMG() { return m_recoilPunchTimeV_HMG; }
void HapticSettings::setRecoilPunchTimeV_HMG(float value) { m_recoilPunchTimeV_HMG = value; }
float HapticSettings::getRecoilYawScaleV_HMG() { return m_recoilYawScaleV_HMG; }
void HapticSettings::setRecoilYawScaleV_HMG(float value) { m_recoilYawScaleV_HMG = value; }
float HapticSettings::getRecoilPitchScaleV_HMG() { return m_recoilPitchScaleV_HMG; }
void HapticSettings::setRecoilPitchScaleV_HMG(float value) { m_recoilPitchScaleV_HMG = value; }
float HapticSettings::getRecoilPunchScaleV_HELIGUN() { return m_recoilPunchScaleV_HELIGUN; }
void HapticSettings::setRecoilPunchScaleV_HELIGUN(float value) { m_recoilPunchScaleV_HELIGUN = value; }
float HapticSettings::getRecoilPunchTimeV_HELIGUN() { return m_recoilPunchTimeV_HELIGUN; }
void HapticSettings::setRecoilPunchTimeV_HELIGUN(float value) { m_recoilPunchTimeV_HELIGUN = value; }
float HapticSettings::getRecoilYawScaleV_HELIGUN() { return m_recoilYawScaleV_HELIGUN; }
void HapticSettings::setRecoilYawScaleV_HELIGUN(float value) { m_recoilYawScaleV_HELIGUN = value; }
float HapticSettings::getRecoilPitchScaleV_HELIGUN() { return m_recoilPitchScaleV_HELIGUN; }
void HapticSettings::setRecoilPitchScaleV_HELIGUN(float value) { m_recoilPitchScaleV_HELIGUN = value; }
float HapticSettings::getRecoilPunchScaleV_AIRGUN() { return m_recoilPunchScaleV_AIRGUN; }
void HapticSettings::setRecoilPunchScaleV_AIRGUN(float value) { m_recoilPunchScaleV_AIRGUN = value; }
float HapticSettings::getRecoilPunchTimeV_AIRGUN() { return m_recoilPunchTimeV_AIRGUN; }
void HapticSettings::setRecoilPunchTimeV_AIRGUN(float value) { m_recoilPunchTimeV_AIRGUN = value; }
float HapticSettings::getRecoilYawScaleV_AIRGUN() { return m_recoilYawScaleV_AIRGUN; }
void HapticSettings::setRecoilYawScaleV_AIRGUN(float value) { m_recoilYawScaleV_AIRGUN = value; }
float HapticSettings::getRecoilPitchScaleV_AIRGUN() { return m_recoilPitchScaleV_AIRGUN; }
void HapticSettings::setRecoilPitchScaleV_AIRGUN(float value) { m_recoilPitchScaleV_AIRGUN = value; }
float HapticSettings::getRecoilPunchScaleV_MISSILE() { return m_recoilPunchScaleV_MISSILE; }
void HapticSettings::setRecoilPunchScaleV_MISSILE(float value) { m_recoilPunchScaleV_MISSILE = value; }
float HapticSettings::getRecoilPunchTimeV_MISSILE() { return m_recoilPunchTimeV_MISSILE; }
void HapticSettings::setRecoilPunchTimeV_MISSILE(float value) { m_recoilPunchTimeV_MISSILE = value; }
float HapticSettings::getRecoilYawScaleV_MISSILE() { return m_recoilYawScaleV_MISSILE; }
void HapticSettings::setRecoilYawScaleV_MISSILE(float value) { m_recoilYawScaleV_MISSILE = value; }
float HapticSettings::getRecoilPitchScaleV_MISSILE() { return m_recoilPitchScaleV_MISSILE; }
void HapticSettings::setRecoilPitchScaleV_MISSILE(float value) { m_recoilPitchScaleV_MISSILE = value; }
float HapticSettings::getRecoilPunchScaleV_ARMORCANNON() { return m_recoilPunchScaleV_ARMORCANNON; }
void HapticSettings::setRecoilPunchScaleV_ARMORCANNON(float value) { m_recoilPunchScaleV_ARMORCANNON = value; }
float HapticSettings::getRecoilPunchTimeV_ARMORCANNON() { return m_recoilPunchTimeV_ARMORCANNON; }
void HapticSettings::setRecoilPunchTimeV_ARMORCANNON(float value) { m_recoilPunchTimeV_ARMORCANNON = value; }
float HapticSettings::getRecoilYawScaleV_ARMORCANNON() { return m_recoilYawScaleV_ARMORCANNON; }
void HapticSettings::setRecoilYawScaleV_ARMORCANNON(float value) { m_recoilYawScaleV_ARMORCANNON = value; }
float HapticSettings::getRecoilPitchScaleV_ARMORCANNON() { return m_recoilPitchScaleV_ARMORCANNON; }
void HapticSettings::setRecoilPitchScaleV_ARMORCANNON(float value) { m_recoilPitchScaleV_ARMORCANNON = value; }
float HapticSettings::getRecoilPunchScaleV_COAXIALGUN() { return m_recoilPunchScaleV_COAXIALGUN; }
void HapticSettings::setRecoilPunchScaleV_COAXIALGUN(float value) { m_recoilPunchScaleV_COAXIALGUN = value; }
float HapticSettings::getRecoilPunchTimeV_COAXIALGUN() { return m_recoilPunchTimeV_COAXIALGUN; }
void HapticSettings::setRecoilPunchTimeV_COAXIALGUN(float value) { m_recoilPunchTimeV_COAXIALGUN = value; }
float HapticSettings::getRecoilYawScaleV_COAXIALGUN() { return m_recoilYawScaleV_COAXIALGUN; }
void HapticSettings::setRecoilYawScaleV_COAXIALGUN(float value) { m_recoilYawScaleV_COAXIALGUN = value; }
float HapticSettings::getRecoilPitchScaleV_COAXIALGUN() { return m_recoilPitchScaleV_COAXIALGUN; }
void HapticSettings::setRecoilPitchScaleV_COAXIALGUN(float value) { m_recoilPitchScaleV_COAXIALGUN = value; }
float HapticSettings::getUnresolved48C() { return m_unresolved48C; }
void HapticSettings::setUnresolved48C(float value) { m_unresolved48C = value; }
float HapticSettings::getUnresolved490() { return m_unresolved490; }
void HapticSettings::setUnresolved490(float value) { m_unresolved490 = value; }
float HapticSettings::getGeneralShakeScale() { return m_generalShakeScale; }
void HapticSettings::setGeneralShakeScale(float value) { m_generalShakeScale = value; }
float HapticSettings::getExplosionShakeScale() { return m_explosionShakeScale; }
void HapticSettings::setExplosionShakeScale(float value) { m_explosionShakeScale = value; }
float HapticSettings::getSpeedShakeScale() { return m_speedShakeScale; }
void HapticSettings::setSpeedShakeScale(float value) { m_speedShakeScale = value; }

// (96 B): a linear search over the rows, comparing the name as given.
int HapticSettings::getWeaponClass(const std::string& name)
{
    for (std::vector<WeaponClassEntry>::size_type index = 0; index < m_weaponClassMap.size(); ++index) {
        if (name.compare(m_weaponClassMap[index].name) == 0)
            return m_weaponClassMap[index].weaponClass;
    }
    return -1;
}

// (330 B): a lowercased copy of the name (4B9AA0 is std::transform over its characters with
// the C tolower [INFERRED]) either updates the matching row's class id or becomes a new row.
void HapticSettings::addWeaponClassMap(const std::string& name, int weaponClass)
{
    std::string lowered(name);
    std::transform(lowered.begin(), lowered.end(), lowered.begin(), ::tolower);
    for (std::vector<WeaponClassEntry>::size_type index = 0; index < m_weaponClassMap.size(); ++index) {
        if (lowered.compare(m_weaponClassMap[index].name) == 0) {
            m_weaponClassMap[index].weaponClass = weaponClass;
            return;
        }
    }
    WeaponClassEntry entry;
    entry.name = lowered;
    entry.weaponClass = weaponClass;
    m_weaponClassMap.push_back(entry);
}

} }
#endif
