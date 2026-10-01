// Bundle supporting input-driven and automatic rotation, stabilization and rotation sound.
// Camera, engine and landing-gear objects specialize this behavior. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_ROTATIONALBUNDLE_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_ROTATIONALBUNDLE_H

#include "../Base/Bundle.h"
#include "../../../Sound/ISoundNode.h"

namespace dice {
namespace hfe {
class ISound;
class EngineNetworkable;                    // the wire's typed m_rotationInput.z read
class RotationalBundleNetworkable;          // the wire reads/writes this class's recovered fields
namespace io { class PlayerInput; }
namespace world {

class RotationalBundleTemplate;
class GenericFireArm;

class RotationalBundle : public Bundle, public ISoundNode     // ISoundNode = the +0x13C sub-object
{
public:
    explicit RotationalBundle(const RotationalBundleTemplate* objectTemplate);
    virtual ~RotationalBundle();

    virtual void init();
    virtual void disableCachedObject();
    virtual void enableCachedObject();
    virtual void handleUpdate(float, unsigned int);
    virtual void handlePlayerInput(IPlayer*, const io::PlayerInput&, float, unsigned int);
    virtual void handleMessage(TemplateMessage, IPlayer*, bool);
    virtual void initNetworkableComp();

    // the eight own slots, +0x180..+0x19C
    virtual void setCurrentRotation(const Vec3& rotation);
    virtual const Vec3& getCurrentRotation() const;
    virtual void setCurrentRotationalSpeed(const Vec3& speed);
    virtual const Vec3& getCurrentRotationalSpeed() const;
    virtual const Mat4& getTransform();                                          // (ISoundNode's too)
    virtual const Math::Vec3* getVelocity();                                     // 0 (ISoundNode's too)
    virtual void applyStabilizedRotation();
    virtual void preHandlePlayerInput(io::PlayerInput&, float) const;

    // non-virtual
    void setState();
    // returns bool, not void - handleUpdate uses it as `if (goToSleep(dt)) return;`, the
    // first refusal for a bundle at rest. The void declaration was wrong.
    bool goToSleep(float);
    void applyRotation(const Vec3& rotation);
    // Updates the selected axis (0, 1 or 2) over deltaTime and clips it to the template limits.
    // The default old model treats automatic-reset input as position; other input drives angular speed.
    void calculateAndClipAngle(const RotationalBundleTemplate*, int axis, float deltaTime);
    // The turret stabiliser: it turns the root vehicle's own rotation into
    // the synthetic input this bundle needs to hold still against it, per axis, clamped to [-1,1].
    void updateRegulate(Vec3&, float) const;
    void updateRotationalSound(bool fromSpeed, float level);
    static void initSoundTemplates(RotationalBundleTemplate* objectTemplate);    // (the template's templateInitialize tail-jumps here)

    friend class ResponsePhysics;       // addFrictionNew reads m_rotationInput.z (the engine's throttle)
    friend class ::dice::hfe::EngineNetworkable;   // the wire's bit-9 pair reads m_rotationInput.z
                                        // on the engine object
    friend class ::dice::hfe::RotationalBundleNetworkable; // retail ELF offsets differ from the
    // GenericFireArm sets the owner that blocks rotation input.
    friend class GenericFireArm;

protected:
    /* 0x140 */ Vec3    m_currentRotation;
    /* 0x14C */ Vec3    m_currentRotationalSpeed;
    /* 0x158 */ Vec3    m_rotationInput;   // the per-axis input this tick. Wing::handleUpdate writes
                                           // .y from its lift regulator; handlePlayerInput is the
                                           // other writer. [CHOSEN name]
    /* 0x164 */ Vec3    m_excessInput;     // the per-axis input bank, and only used when the template
                                           // Cached forward axis of the absolute transform; initialized to (0, 0, 1).
    /* 0x170 */ Vec3    m_forwardAxis;              // ctor (0, 0, 1)
    /* 0x17C */ Vec3    m_restoreRotation;
    /* 0x188 */ bool    m_byte188;
    /* 0x189 */ bool    m_byte189; // Settled state suppresses speed-based rotation sound; cleared when rotation leaves a limit.
    /* 0x18C */ float   m_soundValue;
    /* 0x190 */ int     m_dword190;
    /* 0x194 */ bool    m_byte194;
    /* 0x195 */ bool    m_autoRotating;
    /* 0x196 */ bool    m_rotatesContinuously;
    // by handlePlayerInput, which is the only recovered writer: it stores
    // 0.5f here on the tail that flags the bundle as needing work. [CHOSEN] name.
    /* 0x198 */ float   m_inputRegulateAmount;
    // THE player whose input this bundle swallows. When handlePlayerInput is handed exactly
    // this player it zeroes the caller's entire 64-float input array and returns - see the .cpp.
    /* 0x19C */ IPlayer* m_inputOwner;                // [CHOSEN] name
    /* 0x1A0 */ ISound* m_rotationSound;
    /* 0x1A4 */ Vec3    m_regulate;                   // +0x1A4 +0x1A8 / +0x1AC
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_ROTATIONALBUNDLE_H
