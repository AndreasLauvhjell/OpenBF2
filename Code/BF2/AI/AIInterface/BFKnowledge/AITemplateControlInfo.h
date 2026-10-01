// Script-defined control inputs, PID settings and camera limits for AI objects.
// Use named members: 64-bit mask alignment shifts camera fields between ELF and MSVC.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATECONTROLINFO_H
#define AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATECONTROLINFO_H

#include "AITemplatePlugIn.h"

#include "../../../IO/Input/Input.h"            // io::PlayerInputMap

namespace dice {
namespace hfe {
namespace ai {

class AITemplateControlInfo : public AITemplatePlugIn
{
public:
    AITemplateControlInfo();
    virtual ~AITemplateControlInfo();

    virtual IAIObjectPlugIn* createPlugIn(IAIObject* object, AITemplate* aiTemplate);
    virtual int getId();                                // returns 1

    // +0x14. [INFERRED void] - a stop; its caller is chunk 07's AIObjectControlInfo.
    virtual void initFromObject(IAIObject* object);
    virtual unsigned long long getDriveControls();     // +0x18 - the u64 at +0x6C
    virtual unsigned long long getAimControls();       // +0x1C - the u64 at +0x84
    virtual unsigned long long getLookControls();      // +0x20 - the u64 at +0x94

    // each: degrees in, each component clamped to [-pi, pi] radians.
    void setCameraRelativeMinRotationDeg(const Vec3& degrees);
    void setCameraRelativeMaxRotationDeg(const Vec3& degrees);

    /* +0x04 */ float m_throttleSensitivity;            // 0
    /* +0x08 */ float m_pitchSensitivity;               // 0
    /* +0x0C */ float m_rollSensitivity;                // 0
    /* +0x10 */ float m_yawSensitivity;                 // 0
    /* +0x14 */ float m_proportionalConstant;           // 7
    /* +0x18 */ float m_derivativeConstant;             // 1
    /* +0x1C */ float m_angleSpeedConstant;             // 1.5
    /* +0x20 */ float m_throttleLookAhead;              // 0
    /* +0x24 */ float m_pitchLookAhead;                 // 0
    /* +0x28 */ float m_rollLookAhead;                  // 0
    /* +0x2C */ float m_yawLookAhead;                   // 0
    /* +0x30 */ float m_throttleScale;                  // 0
    /* +0x34 */ float m_pitchScale;                     // 0
    /* +0x38 */ float m_rollScale;                      // 0
    /* +0x3C */ float m_yawScale;                       // 0
    /* +0x40 */ float m_lookVerticalSensitivity;        // 0
    /* +0x44 */ float m_lookHorizontalSensitivity;      // 0
    /* +0x48 */ float m_lookVerticalLookAhead;          // 0
    /* +0x4C */ float m_lookHorizontalLookAhead;        // 0
    /* +0x50 */ float m_lookVerticalScale;              // 0
    /* +0x54 */ float m_lookHorizontalScale;            // 0
    /* +0x58 */ float m_maxSpeedBumpinessFallOff;       // 0.002
    /* +0x5C */ io::PlayerInputMap m_driveThrottleControl;   // PINone
    /* +0x60 */ io::PlayerInputMap m_driveTurnControl;       // PINone
    /* +0x64 */ io::PlayerInputMap m_drivePitchControl;      // never initialised
    /* +0x68 */ io::PlayerInputMap m_driveRollControl;       // never initialised
    /* +0x6C */ unsigned long long m_driveControls;          // 0
    /* +0x74 */ io::PlayerInputMap m_aimThrottleControl;     // never initialised
    /* +0x78 */ io::PlayerInputMap m_aimVerticalControl;     // PINone
    /* +0x7C */ io::PlayerInputMap m_aimHorizontalControl;   // PINone
    /* +0x80 */ io::PlayerInputMap m_aimRollControl;         // never initialised
    /* +0x84 */ unsigned long long m_aimControls;            // 0
    /* +0x8C */ io::PlayerInputMap m_lookVerticalControl;    // PINone
    /* +0x90 */ io::PlayerInputMap m_lookHorizontalControl;  // PINone
    /* +0x94 */ unsigned long long m_lookControls;           // 0
    /* +0x9C */ float m_maxClimbAngle;                  // never initialised
    /* +0xA0 */ float m_maxRollAngle;                   // never initialised
    /* +0xA4 */ float m_cameraRelativeDofRotationOffset;     // radians; 0
    /* +0xA8 */ Vec3  m_cameraRelativeMinRotation;           // radians; (-pi, -pi, -pi)
    /* +0xB4 */ Vec3  m_cameraRelativeMaxRotation;           // radians; (pi, pi, pi)
    // initFromObject fills both matrices from the object's first camera (row vectors):
    // +0xC0 = camera * inverse(object) - the camera relative to the object; AIObjectControlInfo's
    // getCameraBase* read it [INFERRED name]
    // +0x100 = inverse(object) * camera - [UNRESOLVED] reader
    /* +0xC0 */ Mat4  m_cameraBaseTransformation;       // identity - stored twice by the ctor
    /* +0x100 */ Mat4 m_unknown100;                     // identity
    /* +0x140 */ bool m_initialized;                    // initFromObject's once-flag - set only on
                                                        // the camera arm (see there); false
    /* +0x141 */ bool m_hasCameraRelativeRotationLimits;// [INFERRED name] both setters set it; false
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_AITEMPLATECONTROLINFO_H
