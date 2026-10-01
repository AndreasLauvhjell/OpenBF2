// Path:     [INFERRED] - Game/Objects/, beside the concrete PlayerControlObjectTemplate.
// Contents: complete secondary interface recovered from the shipped ELF and Win32 vtables.
// The interface has 69 slots: the IBase quartet followed by 65 player-control-template methods.
// ELF places the subobject at +0x17C and Win32 at +0x22C; using a real C++ base preserves that
// platform-dependent adjustment. Both shipped tables put getVehicleCategory at +0x24.

#ifndef GAME_OBJECTS_IPLAYERCONTROLOBJECTTEMPLATE_H
#define GAME_OBJECTS_IPLAYERCONTROLOBJECTTEMPLATE_H

#include <iosfwd>

#include "../../Core/IBase.h"
#include "../../Math/Mat4.h"
#include "../../Math/Vec3.h"

#include <string>

namespace dice {
namespace hfe {
namespace world {

// The parameter types are named by the ELF symbols. Their enumerator sets are not needed by this
// interface and remain unrecovered; the shipped ABI uses 32-bit enum storage.
enum VehicleCategory : int;

// Shared console/makeScript vocabulary, implemented in the attested Game object-template module.
std::ostream& operator<<(std::ostream&, VehicleCategory);
std::istream& operator>>(std::istream&, VehicleCategory&);
enum VehicleType : int;
// defined beside the VehicleType writer in the Game object-template
// module. The HUD's Radio.sendSpottedMessage converter reads its argument through it (that TU
// declares it locally: it sees IGame.h's placeholder VehicleType, not this opaque one).
std::istream& operator>>(std::istream&, VehicleType&);
enum ToolTipType : int;
enum CameraShakeTriggers : int;

class IPlayerControlObjectTemplate : public IBase
{
public:
    virtual void setPcoId(int) = 0;                                           // +0x10
    virtual int getPcoId() = 0;                                               // +0x14
    virtual void setPcoFlags(int) = 0;                                        // +0x18
    virtual unsigned int getPcoFlags() const = 0;                             // +0x1C
    virtual void setVehicleCategory(VehicleCategory) = 0;                     // +0x20
    virtual int getVehicleCategory() const = 0;                               // +0x24
    virtual void setControlsCategory(VehicleCategory) = 0;                    // +0x28
    virtual int getControlsCategory() const = 0;                              // +0x2C
    virtual void setVehicleType(VehicleType) = 0;                             // +0x30
    virtual int getVehicleType() = 0;                                         // +0x34
    virtual void setToolTipType(ToolTipType) = 0;                             // +0x38
    virtual int getToolTipType() = 0;                                         // +0x3C

    virtual void setSoldierExitLocation(const Vec3&, const Vec3&) = 0;        // +0x40
    virtual void setSoldierExitPosition(const Vec3&) = 0;                     // +0x44
    virtual Vec3 getSoldierExitPosition() = 0;                                // +0x48
    virtual void setSoldierExitRotation(const Vec3&) = 0;                     // +0x4C
    virtual Vec3 getSoldierExitRotation() = 0;                                // +0x50
    virtual void setArtPos(bool) = 0;                                         // +0x54
    virtual bool getArtPos() = 0;                                             // +0x58 PE 0x5793C0 (AL)

    virtual void setDirBarXScale(float) = 0;                                  // +0x5C
    virtual float getDirBarXScale() = 0;                                      // +0x60
    virtual void setDirBarYScaleAbove(float) = 0;                             // +0x64
    virtual float getDirBarYScaleAbove() = 0;                                 // +0x68
    virtual void setDirBarYScaleBelow(float) = 0;                             // +0x6C
    virtual float getDirBarYScaleBelow() = 0;                                 // +0x70
    virtual void setDirBarRotate(float) = 0;                                  // +0x74
    virtual float getDirBarRotate() = 0;                                      // +0x78
    virtual void setDirBarYScaleMin(float) = 0;                               // +0x7C
    virtual float getDirBarYScaleMin() = 0;                                   // +0x80
    virtual void setDirBarYScaleMax(float) = 0;                               // +0x84
    virtual float getDirBarYScaleMax() = 0;                                   // +0x88

    virtual void setNameTagOffset(Vec3) = 0;                                  // +0x8C
    virtual const Vec3& getNameTagOffset() = 0;                               // +0x90
    virtual void setRidingWithOffset(Vec3) = 0;                               // +0x94
    virtual const Vec3& getRidingWithOffset() = 0;                            // +0x98
    virtual void setRidingWithDistMod(float) = 0;                             // +0x9C
    virtual float getRidingWithDistMod() = 0;                                 // +0xA0
    virtual void setVehicleCameraShake(CameraShakeTriggers) = 0;              // +0xA4
    virtual int getVehicleCameraShake() = 0;                                  // +0xA8
    virtual void setVehicleFov(float) = 0;                                    // +0xAC
    virtual float getVehicleFov() = 0;                                        // +0xB0
    virtual void setSeatLeftRotationLimit(float) = 0;                         // +0xB4
    virtual float getSeatLeftRotationLimit() const = 0;                       // +0xB8
    virtual void setSeatRightRotationLimit(float) = 0;                        // +0xBC
    virtual float getSeatRightRotationLimit() const = 0;                      // +0xC0

    virtual void seatInformation(const std::string&, const Vec3&,
                                 const Vec3&) = 0;                            // +0xC4
    virtual void setSeatAnimationSystem(const std::string&) = 0;              // +0xC8
    virtual const std::string& getSeatAnimationSystem() = 0;                  // +0xCC
    virtual void setSeatParent(const std::string&) = 0;                       // +0xD0
    virtual const std::string& getSeatParent() const = 0;                     // +0xD4
    virtual void setSeatPosition(const Vec3&) = 0;                            // +0xD8
    virtual const Vec3& getSeatPosition() const = 0;                          // +0xDC
    virtual void setSeatRotation(const Vec3&) = 0;                            // +0xE0
    virtual const Vec3& getSeatRotation() const = 0;                          // +0xE4
    virtual const Mat4& getSeatTransformation() const = 0;                    // +0xE8
    virtual const std::string& getSoundFilter() const = 0;                    // +0xEC
    virtual void setSoundFilter(const std::string&) = 0;                      // +0xF0
    virtual float getListenerObstruction() const = 0;                         // +0xF4
    virtual void setListenerObstruction(float) = 0;                           // +0xF8
    virtual void setMaintainCameraOnExit(bool) = 0;                           // +0xFC
    virtual bool getMaintainCameraOnExit() = 0;                               // +0x100
    virtual void setMaintainCameraOnEnter(bool) = 0;                          // +0x104
    virtual bool getMaintainCameraOnEnter() = 0;                              // +0x108
    virtual void setNeverDrawAs1p(bool) = 0;                                  // +0x10C
    virtual bool getNeverDrawAs1p() const = 0;                                // +0x110

protected:
    ~IPlayerControlObjectTemplate() {}
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_IPLAYERCONTROLOBJECTTEMPLATE_H
