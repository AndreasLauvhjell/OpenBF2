// Exposes vehicle controls, aiming limits and camera transforms to the AI.
// Access template fields by name: 64-bit mask alignment differs between ELF and MSVC.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTCONTROLINFO_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTCONTROLINFO_H

#include "IAIObjectPlugIn.h"

namespace dice {
namespace hfe {
namespace ai {

class AITemplateControlInfo;

class AIObjectControlInfo : public IAIObjectControlInfo
{
public:
    AIObjectControlInfo(IAIObject* object, AITemplateControlInfo* aiTemplate);
    virtual ~AIObjectControlInfo();

    virtual int getId() const;                                  // +0x08
    virtual void createInformationPlugIn(unsigned int handle, bool real);   // +0x0C
    virtual void init();                                        // +0x10
    virtual void update();                                      // +0x14, - empty

    virtual unsigned long long getDriveControls() const;        // +0x18
    virtual io::PlayerInputMap getDriveThrottleControl() const; // +0x1C
    virtual io::PlayerInputMap getDriveTurnControl() const;     // +0x20
    virtual unsigned long long getAimControls() const;          // +0x24
    virtual io::PlayerInputMap getAimVerticalControl() const;   // +0x28
    virtual io::PlayerInputMap getAimHorizontalControl() const; // +0x2C
    virtual unsigned long long getLookControls() const;         // +0x30
    virtual io::PlayerInputMap getLookVerticalControl() const;  // +0x34
    virtual io::PlayerInputMap getLookHorizontalControl() const;    // +0x38
    virtual float getThrottleSensitivity() const;               // +0x3C
    virtual float getPitchSensitivity() const;                  // +0x40
    virtual float getRollSensitivity() const;                   // +0x44
    virtual float getYawSensitivity() const;                    // +0x48
    virtual float getProportionalConstant() const;              // +0x4C
    virtual float getDerivativeConstant() const;                // +0x50
    virtual float getAngleSpeedConstant() const;                // +0x54
    virtual float getThrottleLookAhead() const;                 // +0x58
    virtual float getPitchLookAhead() const;                    // +0x5C
    virtual float getRollLookAhead() const;                     // +0x60
    virtual float getYawLookAhead() const;                      // +0x64
    virtual float getThrottleScale() const;                     // +0x68
    virtual float getPitchScale() const;                        // +0x6C
    virtual float getRollScale() const;                         // +0x70
    virtual float getYawScale() const;                          // +0x74
    virtual float getLookVerticalSensitivity() const;           // +0x78
    virtual float getLookHorizontalSensitivity() const;         // +0x7C
    virtual float getLookVerticalLookAhead() const;             // +0x80
    virtual float getLookHorizontalLookAhead() const;           // +0x84
    virtual float getLookVerticalScale() const;                 // +0x88
    virtual float getLookHorizontalScale() const;               // +0x8C
    virtual float getMaxSpeedBumpinessFallOff() const;          // +0x90
    virtual float getCameraRelativeDofRotationOffset() const;   // +0x94
    virtual const Vec3& getCameraRelativeMinRotation() const;   // +0x98
    virtual const Vec3& getCameraRelativeMaxRotation() const;   // +0x9C
    virtual void getCameraRelativeRotation(Vec3& out) const;    // +0xA0,
    virtual const Mat4& getCameraTransformation() const;        // +0xA4
    virtual void getCameraBaseTransformation(Mat4& out) const;  // +0xA8
    virtual void getCameraBaseRight(Vec3& out) const;           // +0xAC
    virtual void getCameraBaseUp(Vec3& out) const;              // +0xB0
    virtual void getCameraBaseDof(Vec3& out) const;             // +0xB4
    virtual void getCameraBasePos(Vec3& out) const;             // +0xB8
    virtual bool validateCameraDirection(const Vec3& direction) const;      // +0xBC
    virtual bool validateCameraDirectionYaw(const Vec3& direction) const;   // +0xC0
    virtual bool validateCameraDirectionPitch(const Vec3& direction) const; // +0xC4
    virtual bool validateCameraDirectionByPos(const Vec3& position) const;  // +0xC8
    virtual float getMaxClimbAngle() const;                     // +0xCC
    virtual float getMaxRollAngle() const;                      // +0xD0
    virtual io::PlayerInputMap getDriveRollControl() const;     // +0xD4
    virtual io::PlayerInputMap getDrivePitchControl() const;    // +0xD8
    virtual io::PlayerInputMap getAimThrottleControl() const;   // +0xDC
    virtual io::PlayerInputMap getAimRollControl() const;       // +0xE0

private:
    /* +0x08 */ AITemplateControlInfo* m_template;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AIOBJECTCONTROLINFO_H
