// [INFERRED] Source placement.

#include "AITemplateControlInfo.h"

#include <map>

#include "AIObjectControlInfo.h"
#include "BFGame.h"                                          // IAIGame::getIsPrecaching (+0x68)
#include "IAIObject.h"

#include "../../AIDLL/AI/AIUtils/checkPointer.h"
#include "../../AIDLL/AI/Main/CommonSingletonServer.h"

#include "../../../Core/Ids.h"
#include "../../../Game/Objects/ICameraObject.h"
#include "../../../Game/Objects/IObject.h"
#include "../../../Game/Objects/IObjectTemplate.h"
#include "../../../Game/Objects/RotationalBundleTemplate.h"  // m_minRotation / m_maxRotation
#include "../../../Game/Utility/Vars.h"
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"

namespace dice {
namespace hfe {
namespace world {
// PlayerControlObjectHelpers.cpp (, ); no header declares it - PlayerControlObject.cpp
// carries the same local declaration.
void findCameras(IObject* root, int pcoId, std::map<int, ICameraObject*>& out);
} // namespace world
} // namespace hfe
} // namespace dice

namespace dice {
namespace hfe {
namespace ai {

namespace {
const float kDegToRad = 0.017453292f;   // the image's single-precision constant
const float kPi       = 3.1415927f;

// Clamps X/Y with <= comparisons and Z with >. NaN becomes pi for X/Y but remains NaN for Z.
float clampXY(float radians) { return radians < -kPi ? -kPi : (radians <= kPi ? radians : kPi); }
float clampZ(float radians)  { return radians < -kPi ? -kPi : (radians > kPi ? kPi : radians); }
} // anonymous namespace

// Leaves six fields uninitialized and initializes both matrices to identity, including
// the repeated writes to the first matrix.
AITemplateControlInfo::AITemplateControlInfo()
    : m_throttleSensitivity(0.0f), m_pitchSensitivity(0.0f), m_rollSensitivity(0.0f),
      m_yawSensitivity(0.0f), m_proportionalConstant(-7.0f), m_derivativeConstant(-1.0f),
      m_angleSpeedConstant(1.5f), m_throttleLookAhead(0.0f), m_pitchLookAhead(0.0f),
      m_rollLookAhead(0.0f), m_yawLookAhead(0.0f), m_throttleScale(0.0f), m_pitchScale(0.0f),
      m_rollScale(0.0f), m_yawScale(0.0f), m_lookVerticalSensitivity(0.0f),
      m_lookHorizontalSensitivity(0.0f), m_lookVerticalLookAhead(0.0f),
      m_lookHorizontalLookAhead(0.0f), m_lookVerticalScale(0.0f), m_lookHorizontalScale(0.0f),
      m_maxSpeedBumpinessFallOff(0.002f), m_driveThrottleControl(io::PINone),
      m_driveTurnControl(io::PINone), m_driveControls(0), m_aimVerticalControl(io::PINone),
      m_aimHorizontalControl(io::PINone), m_aimControls(0), m_lookVerticalControl(io::PINone),
      m_lookHorizontalControl(io::PINone), m_lookControls(0), m_cameraRelativeDofRotationOffset(0.0f),
      m_cameraBaseTransformation(Mat4::getIdentity()), m_unknown100(Mat4::getIdentity()),
      m_initialized(false),
      m_hasCameraRelativeRotationLimits(false)
{
    m_cameraRelativeMinRotation.x = -kPi;
    m_cameraRelativeMinRotation.y = -kPi;
    m_cameraRelativeMinRotation.z = -kPi;
    m_cameraRelativeMaxRotation.x = kPi;
    m_cameraRelativeMaxRotation.y = kPi;
    m_cameraRelativeMaxRotation.z = kPi;
}

// - the vptr store and, for , the delete. Nothing else.
AITemplateControlInfo::~AITemplateControlInfo()
{
}

// createPlugIn - , : `new AIObjectControlInfo(object, this)` (0xC B) through
// checkPointer.
IAIObjectPlugIn* AITemplateControlInfo::createPlugIn(IAIObject* object, AITemplate* /*aiTemplate*/)
{
    return checkPointer(new AIObjectControlInfo(object, this));
}

// `mov eax, 1`.
int AITemplateControlInfo::getId()
{
    return 1;
}

// Initializes control and camera limits from the first live object outside precaching.
// Script-supplied limits take precedence. The object must have a control object.
void AITemplateControlInfo::initFromObject(IAIObject* object)
{
    if (m_initialized ||
        static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getIsPrecaching())     // +0x68
        return;

    world::IObject* const engineObject = object->getObject();
    world::IPlayerControlObject* const pco = static_cast<world::IPlayerControlObject*>(
        engineObject->queryInterface(world::IID_IPlayerControlObject));
    std::map<int, world::ICameraObject*> cameras;
    world::findCameras(engineObject, pco->getPcoId(), cameras);                     // +0x30

    const float kPitchLimit = 1.4835298f;                                           // 85 degrees
    if (const_cast<world::IObjectTemplate*>(engineObject->getTemplate())
            ->queryImplementation(world::CID_SoldierTemplate))                      // +0x50
    {
        m_cameraRelativeMinRotation.x = -3.1415927f;
        m_cameraRelativeMinRotation.y = -kPitchLimit;
        m_cameraRelativeMaxRotation.x = 3.1415927f;
        m_cameraRelativeMinRotation.z = 0.0f;
        m_cameraRelativeMaxRotation.y = kPitchLimit;
        m_cameraRelativeMaxRotation.z = 0.0f;
        return;
    }

    if (cameras.empty())
    {
        const float left = *Vars::getFloat("soldier-lookLeftAngle", -40.0f);
        m_cameraRelativeMinRotation.y = -kPitchLimit;
        m_cameraRelativeMinRotation.z = 0.0f;
        m_cameraRelativeMinRotation.x =
            static_cast<float>(static_cast<double>(left) * static_cast<double>(0.017453292f));
        const float right = *Vars::getFloat("soldier-lookRightAngle", 40.0f);
        m_cameraRelativeMaxRotation.y = kPitchLimit;
        m_cameraRelativeMaxRotation.z = 0.0f;
        m_cameraRelativeMaxRotation.x =
            static_cast<float>(static_cast<double>(0.017453292f) * static_cast<double>(right));
        return;
    }

    world::IObject* const camera = static_cast<world::IObject*>(
        static_cast<IBase*>(cameras.begin()->second)->queryInterface(world::IID_IObject));
    if (!m_hasCameraRelativeRotationLimits)
    {
        const world::RotationalBundleTemplate* const bundle =
            static_cast<const world::RotationalBundleTemplate*>(
                const_cast<world::IObjectTemplate*>(camera->getTemplate())
                    ->queryImplementation(world::CID_RotationalBundleTemplate));
        m_cameraRelativeMinRotation = bundle->m_minRotation;
        m_cameraRelativeMaxRotation = bundle->m_maxRotation;
    }

    Mat4 inverse;
    inverse.setInverse(engineObject->getAbsoluteTransformation());                  // +0x84
    m_unknown100.mult(inverse, camera->getAbsoluteTransformation());
    inverse.setInverse(engineObject->getAbsoluteTransformation());
    m_cameraBaseTransformation.mult(camera->getAbsoluteTransformation(), inverse);
    m_initialized = true;
}


// the three masks, whole.
unsigned long long AITemplateControlInfo::getDriveControls() { return m_driveControls; }
unsigned long long AITemplateControlInfo::getAimControls()   { return m_aimControls; }
unsigned long long AITemplateControlInfo::getLookControls()  { return m_lookControls; }

// setCameraRelativeMin/MaxRotationDeg - , each, identical but for
// the target: the limits flag set first, then each component converted to radians and clamped to
// [-pi, pi]. Stock aircraft use "-100/-10/0" and "100/60/0" (88 uses of each).
void AITemplateControlInfo::setCameraRelativeMinRotationDeg(const Vec3& degrees)
{
    m_hasCameraRelativeRotationLimits = true;
    m_cameraRelativeMinRotation.x = clampXY(degrees.x * kDegToRad);
    m_cameraRelativeMinRotation.y = clampXY(degrees.y * kDegToRad);
    m_cameraRelativeMinRotation.z = clampZ(kDegToRad * degrees.z);
}

void AITemplateControlInfo::setCameraRelativeMaxRotationDeg(const Vec3& degrees)
{
    m_hasCameraRelativeRotationLimits = true;
    m_cameraRelativeMaxRotation.x = clampXY(degrees.x * kDegToRad);
    m_cameraRelativeMaxRotation.y = clampXY(degrees.y * kDegToRad);
    m_cameraRelativeMaxRotation.z = clampZ(kDegToRad * degrees.z);
}

} // namespace ai
} // namespace hfe
} // namespace dice
