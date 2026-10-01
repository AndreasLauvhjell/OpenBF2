// Default player-control interface behavior, including constant results and explicit
// unsupported-operation diagnostics. [INFERRED] Source placement beside the attested header.

#include "IPlayerControlObject.h"

#include "../../../../Core/Debug.h"
#include "../../../../IO/Input/PlayerInput.h"       // checkSpecialToggle's by-value parameter

namespace dice {
namespace hfe {
namespace world {

namespace {
const char* const kHeaderPath = "Scene/Object/ObjectTypes/Game/IPlayerControlObject.h";
}

IObject* IPlayerControlObject::getOccupingObject() const            { return 0; }
bool     IPlayerControlObject::getExitPosition(Vec3&)               { return true; }
bool     IPlayerControlObject::getAltExitPosition(Vec3&)            { return true; }

// The two camera hooks and the four "Virtual impl" slots abort: Debug type 6 is process-fatal.
void IPlayerControlObject::addExternalCamera(ICameraObject*)                    // 0x81B1080, line 173
{
    Debug(DEBUGTYPE_6, kHeaderPath, 173, "") << "Default impl";
}
void IPlayerControlObject::removeExternalCamera(ICameraObject*)                 // 0x81B1170, line 174
{
    Debug(DEBUGTYPE_6, kHeaderPath, 174, "") << "Default impl";
}
bool IPlayerControlObject::canSpawn(const Mat4&)                    { return true; }
bool IPlayerControlObject::checkSpecialToggle(io::PlayerInput)                  // 0x81B1260, line 192
{
    Debug(DEBUGTYPE_6, kHeaderPath, 192, "") << "Virtual impl";
    return false;
}
// Reports the unsupported virtual operation. PlayerInputMap is an enum passed by value.
bool IPlayerControlObject::checkSpecialToggleLastUpdate(io::PlayerInputMap)     // 0x81B1350, line 193
{
    Debug(DEBUGTYPE_6, kHeaderPath, 193, "") << "Virtual impl";
    return false;
}
int   IPlayerControlObject::getDefaultTeam()                        { return 0; }
float IPlayerControlObject::getTimeToLiveUnused()                   { return -1.0f; }    // 0x81B1450 (flt_888B590)
bool  IPlayerControlObject::isAllowedToExit()                       { return true; }
void IPlayerControlObject::setTimeSpawned(float)                                // 0x81B1470, line 208
{
    Debug(DEBUGTYPE_6, kHeaderPath, 208, "") << "Virtual impl";
}
float IPlayerControlObject::getTimeSpawned() const                              // 0x81B1560, line 209
{
    Debug(DEBUGTYPE_6, kHeaderPath, 209, "") << "Virtual impl";
    return 0.0f;
}
int   IPlayerControlObject::getVehicleCameraShake()                 { return -1; }
float IPlayerControlObject::getVehicleFov()                         { return 0.0f; }
int   IPlayerControlObject::getCockpitLod()                         { return 0; }
int   IPlayerControlObject::getCockpitSubGeom()                     { return 0; }
void  IPlayerControlObject::setIsSprinting(bool)                    {}
void  IPlayerControlObject::setDriverDecalObject(IObject*)          {}
IObject* IPlayerControlObject::getDriverDecalObject() const         { return 0; }
void  IPlayerControlObject::addDriverDecalTransformation(const Mat4&) {}
std::vector<Mat4>* IPlayerControlObject::getDriverDecalTransformations() const { return 0; }
int   IPlayerControlObject::getSpecialToggleWeaponInput() const     { return 0x40; }
void  IPlayerControlObject::setIsDucking(bool)                      {}
bool  IPlayerControlObject::getIsDucking()                          { return false; }
bool  IPlayerControlObject::getAutoUseAbility() const               { return false; }
bool  IPlayerControlObject::applyCameraAnimation(const Mat4&, Mat4&) { return false; }
bool  IPlayerControlObject::allowEnterOtherVehicle()                { return false; }
bool  IPlayerControlObject::isChildSoldierActive() const            { return false; }
bool  IPlayerControlObject::getNeverDrawAs1p() const                { return false; }
void  IPlayerControlObject::removeChildVehicle(IObject*)            {}
int   IPlayerControlObject::getAmmoStatus()                         { return 15; }
void  IPlayerControlObject::setAmmoStatus(int)                      {}
void  IPlayerControlObject::onFirstGhost()                          {}
void  IPlayerControlObject::setGiveUnmannedInput(bool)              {}
void  IPlayerControlObject::preHandlePlayerInput(io::PlayerInput&, float) const {}

} // namespace world
} // namespace hfe
} // namespace dice
