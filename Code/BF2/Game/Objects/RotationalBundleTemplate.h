// Configures pivot rotation under player or AI input, including limits, speed and automatic reset.
// [INFERRED] Source placement; native STL layout differs from the original ELF layout.

#ifndef GAME_OBJECTS_ROTATIONALBUNDLETEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_ROTATIONALBUNDLETEMPLATE_H

#include "Base/BundleTemplate.h"
#include "../../Math/Vec3.h"
#include "../../IO/Input/PlayerInput.h"      // the PlayerInputMap / GameInputMap enums, and io::Input<T,N>

namespace dice {
namespace hfe {
namespace io { class NetworkManager; }      // compareProperties reads the rotation fields raw
class RotationalBundleNetworkable;           // init reads m_automaticReset raw
namespace world {

// Input-axis bindings use io::PlayerInputMap from IO/Input/Input.h.

class RotationalBundleTemplate : public BundleTemplate
{
public:
    // when EngineTemplate (which derives this) became the deep drive's
    // frontier and this class turned out to be declared-but-never-defined.
    RotationalBundleTemplate();                         // read in full

    virtual unsigned int getClassID() const;            // reads the CID global
    virtual ~RotationalBundleTemplate();                // vptr resets + ~BundleTemplate
    virtual void templateInitialize();                  // a tail-jump into the object side:
                                                        // RotationalBundle::initSoundTemplates
    virtual void* queryImplementation(unsigned int);    // CID hop -> Bundle's chain
    virtual IObject* createObject() const;              // new(0x1B0) RotationalBundle
    virtual bool makeScript(io::IStream*);              // ELF/PE recovered

    // NO IBase quartet override - it inherits BundleTemplate's, so there is no refcount field
    // of its own. (Confirmed: no RotationalBundleTemplate::addRef symbol exists.)

    // The three NON-virtual input-map readers - 0x81EE060 / 0x81EE070 / 0x81EE080, 14 B each, plain
    // field returns. io::NetworkManager::compareProperties calls them (and reads the rotation /
    // speed / acceleration fields and m_automaticReset RAW - hence the friend below) to decide
    // whether two rotational bundles of one vehicle can share a net property.
    io::PlayerInputMap getInputToYaw() const   { return m_inputToYaw; }
    io::PlayerInputMap getInputToPitch() const { return m_inputToPitch; }
    io::PlayerInputMap getInputToRoll() const  { return m_inputToRoll; }

    bool getRememberExcessInput() const        { return m_rememberExcessInput; }

    // Console properties routed through typed accessors.
    void setRememberExcessInput(bool value)    { m_rememberExcessInput = value; }
    // getLesserYawAtSpeed - 0x81EE0F0, a plain read.
    float getLesserYawAtSpeed() const          { return m_lesserYawAtSpeed; }
    // Sets the value and authored flag only for positive inputs; other inputs leave both unchanged.
    void setLesserYawAtSpeed(float value)
    {
        if (value > 0.0f)
        {
            m_lesserYawAtSpeed = value;
            m_lesserYawAtSpeedSet = true;
        }
    }

    friend class ::dice::hfe::io::NetworkManager;
    friend class ::dice::hfe::RotationalBundleNetworkable;   // init reads m_automaticReset RAW, the
                                                             // same way NetworkManager does
    friend class FloatingBundle;                        // handleUpdate reads m_maxRotation.y
    friend class RotationalBundle;                      // the object's ctor / init / setState / handleMessage read the
                                                        // rotation limits and flags directly

    // Fields written directly by the rotational-bundle console bindings. Preserve declaration order.
public:
    // rotation limits and pivot
    /* 380 */ Math::Vec3 m_minRotation;                 // 380,384,388
    /* 392 */ Math::Vec3 m_maxRotation;                 // 392,396,400
    /* 404 */ Math::Vec3 m_pivotPosition;               // 404,408,412  console: PivotPosition

    // rates
    /* 416 */ Math::Vec3 m_maxSpeed;                    // 416,420,424
    /* 428 */ Math::Vec3 m_maxResetSpeed;               // 428,432,436
    /* 440 */ Math::Vec3 m_acceleration;                // 440,444,448
    /* 452 */ Math::Vec3 m_deAcceleration;              // 452,456,460
    /* 464 */ Math::Vec3 m_continousRotationSpeed;      // 464,468,472
                                                        // note the engine's own spelling -
                                                        // "continous", not "continuous". The
                                                        // console binding carries the same typo,
                                                        // so both must keep it.

    // input mapping
    /* 476 */ io::PlayerInputMap m_inputToYaw;
    /* 480 */ io::PlayerInputMap m_inputToPitch;
    /* 484 */ io::PlayerInputMap m_inputToRoll;
    // behaviour flags (m_automaticReset stays in the island - the console writes it too)
    /* 488 */ bool m_automaticReset;

    // These console properties access their fields directly.
    /* 489 */ bool m_snapToZeroOnNoInput;
    /* 490 */ bool m_useDeAcceleration;
    /* 491 */ bool m_restoreRotationOnExit;

    // [INFERRED] name for the yaw-stabilization flag.
    /* 492 */ bool m_automaticYawStabilization;         // [INFERRED]
    /* 493 */ bool m_automaticPitchStabilization;

    /* 494 */ bool m_rememberExcessInput;

    // A set/value pair: the flag records whether the property was authored at all, separately from
    // its value. Same shape as the *ScoreLimit pairs in ScoreManager.
    /* 495 */ bool  m_lesserYawAtSpeedSet;
    /* 496 */ float m_lesserYawAtSpeed;

    /* 500 */ bool m_startAutoRotateOnEnter;
    /* 501 */ bool m_debugAutoRotate;                   // was `_byte1F5`; .con `debugAutoRotate`
    /* 502 */ char _pad1F6[2];

    // stabilisation regulators
    // Each is a Vec2 in the console API, and the IDB resolves each as two consecutive floats.
    /* 504 */ Math::Vec2 m_regulateYaw;                 // 504,508
    /* 512 */ Math::Vec2 m_regulatePitch;               // 512,516
    /* 520 */ Math::Vec2 m_regulateRoll;                // 520,524

    /* 528 */ bool m_capMaxSpeed;                       // was `_byte210`; .con `capMaxSpeed`.
                                                        // [UNRESOLVED] the three bytes after it -
                                                        // no console property writes them
protected:
    /* 529 */ bool _byte211;
    /* 530 */ bool _byte212;
    /* 531 */ bool _byte213;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_ROTATIONALBUNDLETEMPLATE_H
