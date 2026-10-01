// Wing object lifecycle and aerodynamic response.

#include "Wing.h"

#include "../../../Math/Mat4.h"                         // setRotateXDeg / rotMult / mult
#include "../../../Physics/PhysicsWing.h"              // world::calculateLift

#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"
#include "../ObjectLayoutAccess.h"                          // +0x44 physicsNode
#include "../RotationalBundleLeaves.h"                        // WingTemplate

namespace dice {
namespace hfe {
namespace world {

// RotationalBundle(template); the vptrs; the template; +0x1B4 = 0.
Wing::Wing(const WingTemplate* objectTemplate)
    : RotationalBundle(objectTemplate)
    , m_template(objectTemplate)                                                     // +0x1B0
    , m_liftRegulator(0.0f)                                                                  // +0x1B4
{
}

Wing::~Wing() {}                                                                     // ~RotationalBundle

// RotationalBundle::init(); then, from the root object upward (the root has no
// parent, so in practice the root itself), the first IPlayerControlObject answer -> the wing takes
// RotationalBundle's +0x196 and clears its 0x800 tick flag (message 4 raises it again).
void Wing::init()
{
    RotationalBundle::init();
    void* pco = 0;
    for (IObject* walk = getRootParent(this); walk; walk = walk->getParent())
    {
        pco = walk->queryInterface(IID_IPlayerControlObject);
        if (pco)
            break;
    }
    if (pco)
    {
        m_rotatesContinuously = true;                                                // +0x196
        updateFlags(0u, 0x800u);
    }
}

// see the header.
void Wing::handleMessage(TemplateMessage message, IPlayer* player, bool flag)
{
    if (object_detail::LayoutAccess::physicsNode(this))                              // +0x44
    {
        if (static_cast<int>(message) == 4)
            updateFlags(0x800u, 0u);
        RotationalBundle::handleMessage(message, player, flag);
        return;
    }
    Debug(DEBUGTYPE_6, "Game/Objects/Vehicle/Wing.cpp", 131) << "no physicsNodeComp!";
}


// Updates wing rotation and physics orientation over deltaTime.
// Applies the flap-weighted aerodynamic angle before restoring the full visible angle; both transform writes matter.
void Wing::handleUpdate(float deltaTime, unsigned int)
{
    const WingTemplate* wingTemplate = m_template;

    PhysicsWing* node =
        static_cast<PhysicsWing*>(object_detail::LayoutAccess::physicsNode(this));   // +0x44
    if (!node)
    {
        Debug(DEBUGTYPE_6, "Game/Objects/Vehicle/Wing.cpp", 73) << "No physicsNodeComp!";
        return;
    }

    getRootParent(this);                             // result discarded, as shipped

    if (wingTemplate->m_liftRegulated)               // template +0x22C
        m_rotationInput.y = m_liftRegulator;                // +0x15C <- +0x1B4

    calculateAndClipAngle(wingTemplate, 1, deltaTime);   // [FRONTIER] - 2,074 B

    Mat4 rotation;
    Mat4 combined;

    // pass 1: the aerodynamic frame
    float deflection = m_currentRotation.y * wingTemplate->m_flapLift
                     / (wingTemplate->m_flapLift + wingTemplate->m_wingLift)
                     - wingTemplate->m_pitchOffset;
    if (deflection < -360.0f)     deflection = -360.0f;
    else if (deflection > 360.0f) deflection =  360.0f;

    rotation = Mat4::getIdentity();
    {
        Mat4 pitch = Mat4::getIdentity();
        setRotateXDeg(pitch, deflection);
        const Mat4 previous = rotation;
        rotation.rotMult(previous, pitch);
    }
    combined.mult(rotation, *getBundleTransformation());
    setRelativeTransformation(combined);             // slot +0xEC

    // Harvest the absolute frame pass 1 just produced - orientation only.
    const Mat4& absolute = getAbsoluteTransformation();   // slot +0x84
    for (int i = 0; i < 12; ++i)
        node->m_matrix.m[i] = absolute.m[i];
    node->m_matrix.m[12] = 0.0f;                     // the translation is dropped
    node->m_matrix.m[13] = 0.0f;
    node->m_matrix.m[14] = 0.0f;
    node->m_matrix.m[15] = absolute.m[15];

    // pass 2: the visible frame, from the RAW angle - this is the one that stands
    float angle = m_currentRotation.y;
    if (angle < -360.0f)     angle = -360.0f;
    else if (angle > 360.0f) angle =  360.0f;

    rotation = Mat4::getIdentity();
    {
        Mat4 pitch = Mat4::getIdentity();
        setRotateXDeg(pitch, angle);
        const Mat4 previous = rotation;
        rotation.rotMult(previous, pitch);
    }
    combined.mult(rotation, *getBundleTransformation());
    setRelativeTransformation(combined);
}

float Wing::calculateNeutralLift(const Vec3& airflow, float pitchDegrees, float scale)
{
    Mat4 frame = Mat4::getIdentity();

    float angle = pitchDegrees;
    if (!(angle >= -360.0f))    angle = -360.0f;
    else if (angle > 360.0f)    angle = 360.0f;

    Mat4 pitch = Mat4::getIdentity();
    setRotateXDeg(pitch, angle);

    const Mat4 before = frame;
    frame.rotMult(before, pitch);

    Mat4 bundled;
    bundled.mult(frame, *getBundleTransformation());
    frame.mult(bundled, getParent()->getAbsoluteTransformation());               // parent's slot 33

    Vec3 third;  third.x  = frame.m[8]; third.y  = frame.m[9]; third.z  = frame.m[10];
    Vec3 second; second.x = frame.m[4]; second.y = frame.m[5]; second.z = frame.m[6];
    return calculateLift(airflow, third, second, scale);                         // `third` is unread
}

} // namespace world
} // namespace hfe
} // namespace dice
