// AI control and camera queries. Floating-point sums follow the Windows evaluation order:
// form them in double and round only where the stored float result requires it.
// [INFERRED] Source placement.

#include "AIObjectControlInfo.h"

#include <algorithm>
#include <cfloat>
#include <cmath>

#include "AITemplateControlInfo.h"

#include "../SingletonServer.h"                              // BFSingletonServer::m_iInformationFactory
#include "../../AIDLL/AI/AIKnowledge/InformationFactory.h"   // IInformationFactory

#include "../../../Core/Ids.h"
#include "../../../Game/Objects/ICameraObject.h"
#include "../../../Game/Objects/IObject.h"
#include "../../../Math/Mat4.h"                              // setRotateYDeg
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"

namespace dice {
namespace hfe {
namespace ai {

namespace {

const float kPi               = 3.1415927f;     // PE flt_814880
const float kRadiansToDegrees = 57.295776f;     // PE flt_837AF0 (57.2957763671875)

// The sine of an angle as the shipped bodies clamp it. First inline, ieee-ordered on the unrounded
// dot product - a NaN passes - and rounded to the float the PE stores; then , the shared
// float clamp whose fcmov predicates select the lower bound for a NaN (ServerSettingsConsole.cpp
// records the proof; std::max(lo, std::min(v, hi)) is its exact spelling).
float clampSine(double dot)
{
    float sine;
    if (dot < -1.0)
        sine = -1.0f;
    else if (dot > 1.0)
        sine = 1.0f;
    else
        sine = static_cast<float>(dot);
    return std::max(-1.0f, std::min(sine, 1.0f));
}

// (inline in the ELF, and inline in getCameraRelativeRotation on the PE too): 1, -1, or 0
// and 0 for a NaN, since neither ordered test holds.
double signOf(double value)
{
    if (value > 0.0)
        return 1.0;
    if (value < 0.0)
        return -1.0;
    return 0.0;
}

// the engine's conditional normaliser (the ELF inlines it). A vector already of unit
// length (|len^2 - 1| < FLT_EPSILON) is left bitwise alone; a near-zero one is zeroed and answers
// 20001; anything else - a NaN included - is scaled by 1/sqrt. The squared length is rounded to
// float, the scale is not. The other TUs that need it carry file-local copies; so does this one.
int normalise(Vec3& value)
{
    const float lengthSquared = static_cast<float>(
        (static_cast<double>(value.x) * value.x + static_cast<double>(value.y) * value.y) +
        static_cast<double>(value.z) * value.z);
    if (std::fabs(static_cast<double>(lengthSquared) - 1.0) < FLT_EPSILON)
        return 0;
    if (std::fabs(lengthSquared) < FLT_EPSILON)
    {
        value.x = 0.0f;
        value.y = 0.0f;
        value.z = 0.0f;
        return -20001;
    }
    const double scale = 1.0 / std::sqrt(static_cast<double>(lengthSquared));
    value.x = static_cast<float>(scale * value.x);
    value.y = static_cast<float>(scale * value.y);
    value.z = static_cast<float>(scale * value.z);
    return 0;
}

// PE 0x580700, an out-of-line Mat4 helper the ELF inlines ([UNRESOLVED] name): the matrix row at `v`
// becomes v * m over the 3x3 part, every term read from the original row.
void rotateRow(const Mat4& m, float* v)
{
    const double x = v[0];
    const double y = v[1];
    const double z = v[2];
    const double rz = (m.m[10] * z + m.m[2] * x) + m.m[6] * y;
    const double ry = (m.m[9] * z + m.m[1] * x) + m.m[5] * y;
    const double rx = (m.m[8] * z + m.m[4] * y) + x * m.m[0];
    v[0] = static_cast<float>(rx);
    v[1] = static_cast<float>(ry);
    v[2] = static_cast<float>(rz);
}

// The dof rotation offset's matrix: identity, then setRotateYDeg of the offset in degrees (the
// product rounded to the float argument).
void dofRotation(Mat4& rotation, float dofOffset)
{
    rotation = Mat4::getIdentity();
    setRotateYDeg(rotation, static_cast<float>(static_cast<double>(kRadiansToDegrees) * dofOffset));
}

} // anonymous namespace

AIObjectControlInfo::AIObjectControlInfo(IAIObject* object, AITemplateControlInfo* aiTemplate)
    : IAIObjectControlInfo(object), m_template(aiTemplate)
{
}

AIObjectControlInfo::~AIObjectControlInfo()
{
}

// (the PE folds it with AITemplateControlInfo::getId, ).
int AIObjectControlInfo::getId() const
{
    return AIOFControlInfo;
}

void AIObjectControlInfo::createInformationPlugIn(unsigned int handle, bool real)
{
    static_cast<IInformationFactory*>(BFSingletonServer::m_iInformationFactory)
        ->requestInformationPlugin(handle, getId(), real);                         // +0x20
}

// A control-object query whose result is discarded, then the template's
// initFromObject (+0x14) - which keeps its own once-flag and asks for the control object again.
void AIObjectControlInfo::init()
{
    m_object->getObject()->queryInterface(world::IID_IPlayerControlObject);
    m_template->initFromObject(m_object);
}

// empty.
void AIObjectControlInfo::update()
{
}

// - through the template's virtual mask getters (+0x18..+0x20).
unsigned long long AIObjectControlInfo::getDriveControls() const
{
    return m_template->getDriveControls();
}

unsigned long long AIObjectControlInfo::getAimControls() const
{
    return m_template->getAimControls();
}

unsigned long long AIObjectControlInfo::getLookControls() const
{
    return m_template->getLookControls();
}

// template reads. drivePitch/driveRoll/aimThrottle/aimRoll and the two
// angle limits are never initialised by the template's constructor (see AITemplateControlInfo.h).
io::PlayerInputMap AIObjectControlInfo::getDriveThrottleControl() const  { return m_template->m_driveThrottleControl; }
io::PlayerInputMap AIObjectControlInfo::getDriveTurnControl() const      { return m_template->m_driveTurnControl; }
io::PlayerInputMap AIObjectControlInfo::getAimVerticalControl() const    { return m_template->m_aimVerticalControl; }
io::PlayerInputMap AIObjectControlInfo::getAimHorizontalControl() const  { return m_template->m_aimHorizontalControl; }
io::PlayerInputMap AIObjectControlInfo::getLookVerticalControl() const   { return m_template->m_lookVerticalControl; }
io::PlayerInputMap AIObjectControlInfo::getLookHorizontalControl() const { return m_template->m_lookHorizontalControl; }
io::PlayerInputMap AIObjectControlInfo::getDriveRollControl() const      { return m_template->m_driveRollControl; }
io::PlayerInputMap AIObjectControlInfo::getDrivePitchControl() const     { return m_template->m_drivePitchControl; }
io::PlayerInputMap AIObjectControlInfo::getAimThrottleControl() const    { return m_template->m_aimThrottleControl; }
io::PlayerInputMap AIObjectControlInfo::getAimRollControl() const        { return m_template->m_aimRollControl; }

float AIObjectControlInfo::getThrottleSensitivity() const       { return m_template->m_throttleSensitivity; }
float AIObjectControlInfo::getPitchSensitivity() const          { return m_template->m_pitchSensitivity; }
float AIObjectControlInfo::getRollSensitivity() const           { return m_template->m_rollSensitivity; }
float AIObjectControlInfo::getYawSensitivity() const            { return m_template->m_yawSensitivity; }
float AIObjectControlInfo::getProportionalConstant() const      { return m_template->m_proportionalConstant; }
float AIObjectControlInfo::getDerivativeConstant() const        { return m_template->m_derivativeConstant; }
float AIObjectControlInfo::getAngleSpeedConstant() const        { return m_template->m_angleSpeedConstant; }
float AIObjectControlInfo::getThrottleLookAhead() const         { return m_template->m_throttleLookAhead; }
float AIObjectControlInfo::getPitchLookAhead() const            { return m_template->m_pitchLookAhead; }
float AIObjectControlInfo::getRollLookAhead() const             { return m_template->m_rollLookAhead; }
float AIObjectControlInfo::getYawLookAhead() const              { return m_template->m_yawLookAhead; }
float AIObjectControlInfo::getThrottleScale() const             { return m_template->m_throttleScale; }
float AIObjectControlInfo::getPitchScale() const                { return m_template->m_pitchScale; }
float AIObjectControlInfo::getRollScale() const                 { return m_template->m_rollScale; }
float AIObjectControlInfo::getYawScale() const                  { return m_template->m_yawScale; }
float AIObjectControlInfo::getLookVerticalSensitivity() const   { return m_template->m_lookVerticalSensitivity; }
float AIObjectControlInfo::getLookHorizontalSensitivity() const { return m_template->m_lookHorizontalSensitivity; }
float AIObjectControlInfo::getLookVerticalLookAhead() const     { return m_template->m_lookVerticalLookAhead; }
float AIObjectControlInfo::getLookHorizontalLookAhead() const   { return m_template->m_lookHorizontalLookAhead; }
float AIObjectControlInfo::getLookVerticalScale() const         { return m_template->m_lookVerticalScale; }
float AIObjectControlInfo::getLookHorizontalScale() const       { return m_template->m_lookHorizontalScale; }
float AIObjectControlInfo::getMaxSpeedBumpinessFallOff() const  { return m_template->m_maxSpeedBumpinessFallOff; }
float AIObjectControlInfo::getMaxClimbAngle() const             { return m_template->m_maxClimbAngle; }
float AIObjectControlInfo::getMaxRollAngle() const              { return m_template->m_maxRollAngle; }

float AIObjectControlInfo::getCameraRelativeDofRotationOffset() const
{
    return m_template->m_cameraRelativeDofRotationOffset;
}

const Vec3& AIObjectControlInfo::getCameraRelativeMinRotation() const
{
    return m_template->m_cameraRelativeMinRotation;
}

const Vec3& AIObjectControlInfo::getCameraRelativeMaxRotation() const
{
    return m_template->m_cameraRelativeMaxRotation;
}

// Returns the active camera transform, or the object transform when no camera is active.
// The object must provide IPlayerControlObject.
const Mat4& AIObjectControlInfo::getCameraTransformation() const
{
    world::IPlayerControlObject* const pco = static_cast<world::IPlayerControlObject*>(
        m_object->getObject()->queryInterface(world::IID_IPlayerControlObject));
    world::ICameraObject* const camera = pco->getActiveCamera();
    if (!camera)
        return m_object->getTransformation();
    return static_cast<world::IObject*>(camera->queryInterface(world::IID_IObject))
        ->getAbsoluteTransformation();
}

// the template's camera matrix (+0xC0) composed with the object's
// transformation through Mat4::mult(camera, object).
void AIObjectControlInfo::getCameraBaseTransformation(Mat4& out) const
{
    out.mult(m_template->m_cameraBaseTransformation, m_object->getTransformation());
}

// getCameraBaseRight / Up / Dof / Pos - / ;
// One row of the object's transformation multiplied by the
// template's camera matrix C: out = row * C over the 3x3 part, plus C's translation for the
// position. Each sum in the PE's order (see the file header).
void AIObjectControlInfo::getCameraBaseRight(Vec3& out) const
{
    const float* const t = m_object->getTransformation().m;
    const float* const c = m_template->m_cameraBaseTransformation.m;
    out.x = static_cast<float>((c[8] * static_cast<double>(t[2]) + c[4] * static_cast<double>(t[1])) +
                               c[0] * static_cast<double>(t[0]));
    out.y = static_cast<float>((c[1] * static_cast<double>(t[0]) + c[9] * static_cast<double>(t[2])) +
                               c[5] * static_cast<double>(t[1]));
    out.z = static_cast<float>((c[2] * static_cast<double>(t[0]) + c[10] * static_cast<double>(t[2])) +
                               c[6] * static_cast<double>(t[1]));
}

void AIObjectControlInfo::getCameraBaseUp(Vec3& out) const
{
    const float* const t = m_object->getTransformation().m;
    const float* const c = m_template->m_cameraBaseTransformation.m;
    out.x = static_cast<float>((c[8] * static_cast<double>(t[6]) + c[4] * static_cast<double>(t[5])) +
                               t[4] * static_cast<double>(c[0]));
    out.y = static_cast<float>((c[9] * static_cast<double>(t[6]) + c[1] * static_cast<double>(t[4])) +
                               c[5] * static_cast<double>(t[5]));
    out.z = static_cast<float>((c[10] * static_cast<double>(t[6]) + c[2] * static_cast<double>(t[4])) +
                               c[6] * static_cast<double>(t[5]));
}

void AIObjectControlInfo::getCameraBaseDof(Vec3& out) const
{
    const float* const t = m_object->getTransformation().m;
    const float* const c = m_template->m_cameraBaseTransformation.m;
    out.x = static_cast<float>((c[8] * static_cast<double>(t[10]) + c[4] * static_cast<double>(t[9])) +
                               t[8] * static_cast<double>(c[0]));
    out.y = static_cast<float>((c[9] * static_cast<double>(t[10]) + c[5] * static_cast<double>(t[9])) +
                               c[1] * static_cast<double>(t[8]));
    out.z = static_cast<float>((c[10] * static_cast<double>(t[10]) + c[6] * static_cast<double>(t[9])) +
                               c[2] * static_cast<double>(t[8]));
}

void AIObjectControlInfo::getCameraBasePos(Vec3& out) const
{
    const float* const t = m_object->getTransformation().m;
    const float* const c = m_template->m_cameraBaseTransformation.m;
    out.x = static_cast<float>(((c[4] * static_cast<double>(t[13]) + c[8] * static_cast<double>(t[14])) +
                                t[12] * static_cast<double>(c[0])) + c[12]);
    out.y = static_cast<float>(((c[5] * static_cast<double>(t[13]) + c[1] * static_cast<double>(t[12])) +
                                c[9] * static_cast<double>(t[14])) + c[13]);
    out.z = static_cast<float>(((c[6] * static_cast<double>(t[13]) + c[2] * static_cast<double>(t[12])) +
                                c[10] * static_cast<double>(t[14])) + c[14]);
}

// Computes camera rotation relative to the base: folded horizontal asin, negated vertical
// asin, and zero roll. The horizontal fold uses the unrounded sign.
// Preserves the intermediate matrix multiplication even though its result is unused.
void AIObjectControlInfo::getCameraRelativeRotation(Vec3& out) const
{
    Mat4 base = Mat4::getIdentity();
    Mat4 unused = Mat4::getIdentity();
    unused.mult(m_object->getTransformation(), m_template->m_unknown100);
    getCameraBaseTransformation(base);
    const float* const b = base.m;
    const float* const c = getCameraTransformation().m;

    const double yaw = std::asin(static_cast<double>(clampSine(
        (b[2] * static_cast<double>(c[10]) + b[1] * static_cast<double>(c[9])) +
        b[0] * static_cast<double>(c[8]))));
    out.x = static_cast<float>(yaw);
    if ((b[9] * static_cast<double>(c[9]) + b[10] * static_cast<double>(c[10])) +
            b[8] * static_cast<double>(c[8]) < 0.0)
        out.x = static_cast<float>(signOf(yaw) * static_cast<double>(kPi) - yaw);

    out.y = static_cast<float>(-std::asin(static_cast<double>(clampSine(
        (b[6] * static_cast<double>(c[10]) + b[5] * static_cast<double>(c[9])) +
        b[4] * static_cast<double>(c[8])))));
    out.z = 0.0f;
}

// Tests directions against camera rotation limits. A (0, 0) limit pair disables that axis check.

// Pitch first, on the unturned base; then the dof turn
// then yaw.
bool AIObjectControlInfo::validateCameraDirection(const Vec3& direction) const
{
    const Vec3& minRotation = getCameraRelativeMinRotation();
    const Vec3& maxRotation = getCameraRelativeMaxRotation();
    Mat4 base = Mat4::getIdentity();
    getCameraBaseTransformation(base);
    float* const b = base.m;
    const double x = direction.x;
    const double y = direction.y;
    const double z = direction.z;

    if (minRotation.y != 0.0f || maxRotation.y != 0.0f)
    {
        const double pitch = -std::asin(static_cast<double>(clampSine((b[6] * z + b[5] * y) + b[4] * x)));
        if (pitch < minRotation.y || pitch > maxRotation.y)
            return false;
    }

    const float dofOffset = getCameraRelativeDofRotationOffset();
    if (dofOffset > 0.0f)
    {
        Mat4 rotation;
        dofRotation(rotation, dofOffset);
        rotateRow(rotation, &b[8]);
    }

    if (minRotation.x != 0.0f || maxRotation.x != 0.0f)
    {
        const double arcsine = std::asin(static_cast<double>(clampSine((b[0] * x + b[2] * z) + b[1] * y)));
        const float stored = static_cast<float>(arcsine);
        double yaw = arcsine;
        if ((b[10] * z + b[8] * x) + b[9] * y < 0.0)
            yaw = signOf(stored) * static_cast<double>(kPi) - stored;
        if (yaw < minRotation.x || yaw > maxRotation.x)
            return false;
    }
    return true;
}

// Yaw only. Here the PE inlines the dof turn, and sums
// it in another order than - reproduced term for term.
bool AIObjectControlInfo::validateCameraDirectionYaw(const Vec3& direction) const
{
    const Vec3& minRotation = getCameraRelativeMinRotation();
    const Vec3& maxRotation = getCameraRelativeMaxRotation();
    Mat4 base = Mat4::getIdentity();
    getCameraBaseTransformation(base);
    float* const b = base.m;

    const float dofOffset = getCameraRelativeDofRotationOffset();
    if (dofOffset > 0.0f)
    {
        Mat4 rotation;
        dofRotation(rotation, dofOffset);
        const float* const r = rotation.m;
        const double dx = b[8];
        const double dy = b[9];
        const double dz = b[10];
        const double rz = (dy * r[6] + dx * r[2]) + r[10] * dz;
        const double ry = (dx * r[1] + dy * r[5]) + r[9] * dz;
        const double rx = (dy * r[4] + dx * r[0]) + r[8] * dz;
        b[8] = static_cast<float>(rx);
        b[9] = static_cast<float>(ry);
        b[10] = static_cast<float>(rz);
    }

    if (!(minRotation.x != 0.0f || maxRotation.x != 0.0f))
        return true;

    const double x = direction.x;
    const double y = direction.y;
    const double z = direction.z;
    const double arcsine = std::asin(static_cast<double>(clampSine((b[0] * x + b[2] * z) + b[1] * y)));
    const float stored = static_cast<float>(arcsine);
    double yaw = arcsine;
    if ((b[8] * x + b[9] * y) + b[10] * z < 0.0)
        yaw = signOf(stored) * static_cast<double>(kPi) - stored;
    if (yaw < minRotation.x)
        return false;
    return !(yaw > maxRotation.x);
}

// Pitch only; no dof turn.
bool AIObjectControlInfo::validateCameraDirectionPitch(const Vec3& direction) const
{
    const Vec3& minRotation = getCameraRelativeMinRotation();
    const Vec3& maxRotation = getCameraRelativeMaxRotation();
    Mat4 base = Mat4::getIdentity();
    getCameraBaseTransformation(base);

    if (!(minRotation.y != 0.0f || maxRotation.y != 0.0f))
        return true;

    const float* const b = base.m;
    const double pitch = -std::asin(static_cast<double>(clampSine(
        (b[6] * static_cast<double>(direction.z) + b[5] * static_cast<double>(direction.y)) +
        b[4] * static_cast<double>(direction.x))));
    if (pitch < minRotation.y)
        return false;
    return !(pitch > maxRotation.y);
}

// The direction from the
// object'S own position (not the camera base's) to `position`, normalised (; a
// zero-length direction answers false), against the object's own transformation - a copy whose dof
// and then right rows the dof offset turns ( twice). Then pitch, then yaw, as above.
// Unlike its siblings this one never consults the template's camera matrix.
bool AIObjectControlInfo::validateCameraDirectionByPos(const Vec3& position) const
{
    Mat4 transformation = m_object->getTransformation();
    float* const t = transformation.m;
    Vec3 direction;
    direction.x = static_cast<float>(static_cast<double>(position.x) - t[12]);
    direction.y = static_cast<float>(static_cast<double>(position.y) - t[13]);
    direction.z = static_cast<float>(static_cast<double>(position.z) - t[14]);
    if (normalise(direction) != 0)
        return false;

    const Vec3& minRotation = getCameraRelativeMinRotation();
    const Vec3& maxRotation = getCameraRelativeMaxRotation();
    const float dofOffset = getCameraRelativeDofRotationOffset();
    if (dofOffset > 0.0f)
    {
        Mat4 rotation;
        dofRotation(rotation, dofOffset);
        rotateRow(rotation, &t[8]);
        rotateRow(rotation, &t[0]);
    }

    const double x = direction.x;
    const double y = direction.y;
    const double z = direction.z;
    if (minRotation.y != 0.0f || maxRotation.y != 0.0f)
    {
        const double pitch = -std::asin(static_cast<double>(clampSine((t[6] * z + t[5] * y) + t[4] * x)));
        if (pitch < minRotation.y || pitch > maxRotation.y)
            return false;
    }

    if (minRotation.x != 0.0f || maxRotation.x != 0.0f)
    {
        const double arcsine = std::asin(static_cast<double>(clampSine((t[2] * z + t[1] * y) + t[0] * x)));
        const float stored = static_cast<float>(arcsine);
        double yaw = arcsine;
        if ((t[10] * z + t[9] * y) + t[8] * x < 0.0)
            yaw = signOf(stored) * static_cast<double>(kPi) - stored;
        if (yaw < minRotation.x || yaw > maxRotation.x)
            return false;
    }
    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
