// Vehicle-seat configuration, sound initialization and script persistence. [INFERRED] Source placement.

#include "PlayerControlObjectTemplate.h"

#include "TemplateLeaves.h"                       // LadderContainerTemplate's declaration
#include "Game/LadderContainer.h"                 // LadderContainerTemplate::createObject's product
#include "Game/ZiplineContainer.h"                // ZiplineContainerTemplate::createObject's product
#include "Game/GrapplingHookRopeContainer.h"      // Grapple container template product
#include "Game/PlayerControlObject.h"             // PlayerControlObjectTemplate::createObject's product
#include "Game/Parachute.h"                       // ParachuteTemplate::createObject's product
#include "../../Animation/BoneAnimation/AnimationSystem.h" // seat animation template manager
#include "../../Core/StringUtil.h"                // compareIgnoreCase
#include "../../IO/Input/Input.h"                 // PlayerInputMap's shipped text form
#include "../../IO/Stream/IStream.h"              // makeScript write target
#include "../../Sound/SoundObjectTemplate.h"      // templateInitialize configures the pair's impls
#include "../../Core/Ids.h"
#include <cfloat>
#include <cmath>
#include <sstream>

namespace dice {
namespace hfe {
namespace world {

// Defined in Game/ModuleWorldObjTemplGame.cpp, their attested retail translation unit.
std::ostream& operator<<(std::ostream&, VehicleCategory);
std::ostream& operator<<(std::ostream&, VehicleType);

// SeatTemplateInformation ctor - ELF 0x819E7C0, 112 bytes: an empty string at +0, +4..+67 zeroed,
// and three multiplier floats (+24/+44/+64) at 1.0f.
SeatTemplateInformation::SeatTemplateInformation()
{
    for (int i = 0; i < 16; ++i)
        seatTransformation.m[i] = 0.0f;
    seatTransformation.m[5] = 1.0f;
    seatTransformation.m[10] = 1.0f;
    seatTransformation.m[15] = 1.0f;
    seatAnimationSystem = 0;
    seatLeftRotationLimit = 0.0f;
    seatRightRotationLimit = 0.0f;
}

// Initializes vehicle defaults, including neutral multipliers and out-of-world damage.
PlayerControlObjectTemplate::PlayerControlObjectTemplate()
{
    m_refCount3 = 1;                              // +384

    m_soldierExitPosition.x = 0.0f; m_soldierExitPosition.y = 0.0f; m_soldierExitPosition.z = 0.0f;
    m_soldierExitRotation.x = 0.0f; m_soldierExitRotation.y = 0.0f; m_soldierExitRotation.z = 0.0f;

    m_nameTagOffset.x = 0.0f;    m_nameTagOffset.y = 0.0f;    m_nameTagOffset.z = 0.0f;      // 492..500
    m_ridingWithOffset.x = 0.0f; m_ridingWithOffset.y = 0.0f; m_ridingWithOffset.z = 0.0f;   // 504..512

    m_damageForBeingOutSideWorld = 1000000.0f;    // instant kill outside the world
    m_ridingWithDistMod = 1.0f;

    m_dontClearTeamOnExit = false;
    m_destroyOnExit = false;
    m_dontAllowExit = false;
    m_hasRestrictedExit = false;

    m_altSoldierExitPosition.x = 0.0f; m_altSoldierExitPosition.y = 0.0f;
    m_altSoldierExitPosition.z = 0.0f;

    m_regulateYawInput = 64;                      // the input-code defaults; 64 appears five times
    m_regulatePitchInput = 64;
    m_regWhenMinInput = 1.0f;
    m_regulateRollInput = 64;
    m_regulateVerticalPosInput = 64;
    m_regulateYaw.x = 0.0f;         m_regulateYaw.y = 0.0f;
    m_regulatePitch.x = 0.0f;       m_regulatePitch.y = 0.0f;
    m_regulateRoll.x = 0.0f;        m_regulateRoll.y = 0.0f;
    m_regulateVerticalPos.x = 0.0f; m_regulateVerticalPos.y = 0.0f;
    m_maxVertRegAngle = 10.0f;
    m_noVertRegAngle = 20.0f;

    m_pcoFlags = 0;
    _dword260 = -1;
    m_pcoId = -1;
    m_artPos = false;
    // _string268: default-constructed (the ELF's _S_empty store).
    m_toolTipType = 22;
    m_vehicleCategory = 0;
    m_controlsCategory = 4;
    m_vehicleType = 26;

    // m_pcoPosIds: default-constructed empty std::vector<int>.

    m_dirBarXScale = 100.0f;
    m_dirBarYScaleAbove = 100.0f;
    m_dirBarYScaleBelow = 100.0f;
    m_cockpitLod = 0;
    m_cockpitSubGeom = 0;
    m_dirBarRotate = 0.0f;
    m_dirBarYScaleMax = 0.0f;
    m_dirBarYScaleMin = 0.0f;

    m_vehicleCameraShake = -1;
    m_vehicleFoV = 0.0f;
    m_positionalSpeedModifier = 1.0f;
    m_disableSpawnPointsOnEnter = false;
    m_toggleWeapon = false;

    // m_soundFilter / m_targetObjectTypesToWatch: default-constructed (the ELF builds the empty
    // string and the empty red-black tree by hand here).
    m_sprintFactor = 2.5f;
    m_specialToggleWeaponInput = 64;
    m_damagedAmbientSoundLimit = 0.5f;
    m_shakeFactor = 1.0f;
    m_sprintRecoverTime = 0.0f;
    m_sprintDissipationTime = 0.0f;
    m_sprintLimit = 0.0f;
    m_allowDucking = false;
    m_disableInputWhileDucking = true;            // the one true bool default
    m_autoUseAbility = false;
    m_listenerObstruction = 0.0f;
    m_groundContactVolume = 0.0f;
    m_crewKitIndex = -1;
    m_maintainCameraOnExit = false;
    m_maintainCameraOnEnter = false;
    m_neverDrawAs1p = false;
    m_isOpenVehicle = false;
    m_boundingRadiusModifierWhenOccupied = 0.0f;

    // The ELF ctor's last store re-zeroes m_seatParent.seatAnimationSystem - already zero from the
    // member ctor; preserved as this comment rather than a redundant statement.
}

// The complete exit-transform accessor group. ELF 0x8181490..0x8181609 was read instruction for
// instruction: the combined setter performs the same six scalar stores as the two three-store
// setters, and both getters materialize a Vec3 by value. The Win32 setSoldierExitLocation console
// executor (PE 0x4B4D00) independently confirms the two-Vec3 ABI through the secondary
// IPlayerControlObjectTemplate face. These bodies are load-bearing for the authored vehicle and
// stationary-weapon exit positions registered in ModuleWorldObjTemplGame.cpp.
void PlayerControlObjectTemplate::setSoldierExitLocation(const Vec3& position,
                                                         const Vec3& rotation)
{
    m_soldierExitPosition = position;
    m_soldierExitRotation = rotation;
}

void PlayerControlObjectTemplate::setSoldierExitPosition(const Vec3& position)
{
    m_soldierExitPosition = position;
}

Vec3 PlayerControlObjectTemplate::getSoldierExitPosition()
{
    return m_soldierExitPosition;
}

void PlayerControlObjectTemplate::setSoldierExitRotation(const Vec3& rotation)
{
    m_soldierExitRotation = rotation;
}

Vec3 PlayerControlObjectTemplate::getSoldierExitRotation()
{
    return m_soldierExitRotation;
}

// The class owns one refcount for all five of its interface views. ELF uses +0x180 and PE uses
// +0x230; those are the first words after the platform-sized IPlayerControlObjectTemplate base.
void PlayerControlObjectTemplate::addRef()
{
    ++m_refCount3;
}

int PlayerControlObjectTemplate::getRef()
{
    return m_refCount3;
}

int PlayerControlObjectTemplate::release()
{
    const int refs = --m_refCount3;
    if (refs == 0)
        delete this;
    return refs;
}

// . The interface offsets deliberately come from typed casts: they are
// +0x30/+0x164/+0x17C in ELF but +0x60/+0x210/+0x22C in the Win32 image.
void* PlayerControlObjectTemplate::queryInterface(unsigned int iid) const
{
    PlayerControlObjectTemplate* self = const_cast<PlayerControlObjectTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplate)
        return static_cast<IObjectTemplate*>(self);
    if (iid == IID_ISimpleObjectTemplate)
        return static_cast<ISimpleObjectTemplate*>(self);
    if (iid == IID_IBundleTemplate)
        return static_cast<IBundleTemplate*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    if (iid == IID_IPlayerControlObjectTemplate)
        return static_cast<IPlayerControlObjectTemplate*>(self);
    return 0;
}

// Destructor - ELF 0x819F070, 363 B. The only handwritten action before ordinary member/base
// teardown is a guarded manager release of the seat animation template. The container and string
// destruction visible in the ELF is compiler-emitted for the real C++ members below. The same
// ownership rule is independently visible in the Win32 LadderTemplate sibling at PE 0x59E6E0.
PlayerControlObjectTemplate::~PlayerControlObjectTemplate()
{
    if (m_seatParent.seatAnimationSystem)
        anim::animationSystemTemplateManager->release(m_seatParent.seatAnimationSystem);
}

// set/getSeatAnimationSystem - ELF 0x819F7E0 / 0x819F890. A same-name assignment is a no-op.
// When replacing an existing template, retail releases it before loading the new name. Preserve
// the shipped empty-name quirk: release happens, but the field is not explicitly nulled. The
// Win32 LadderTemplate sibling (PE 0x59C8B0 / 0x59C4C0) confirms the same load/release/getPath
// sequence and the same empty-name quirk with MSVC's one-destructor-slot manager offsets.
void PlayerControlObjectTemplate::setSeatAnimationSystem(const std::string& name)
{
    if (m_seatParent.seatAnimationSystem)
    {
        if (compareIgnoreCase(getSeatAnimationSystem().c_str(), name.c_str()) == 0)
            return;
        anim::animationSystemTemplateManager->release(m_seatParent.seatAnimationSystem);
    }

    if (!name.empty())
        m_seatParent.seatAnimationSystem = anim::animationSystemTemplateManager->load(name);
}

const std::string& PlayerControlObjectTemplate::getSeatAnimationSystem()
{
    return anim::animationSystemTemplateManager->getPath(m_seatParent.seatAnimationSystem);
}

// Adds Ambient and AmbientDamaged sound templates and disables 3D playback and looping.
// Dedicated servers use the shared NullSoundTemplate.
void PlayerControlObjectTemplate::templateInitialize()
{
    SoundObjectTemplate* ambient =
        static_cast<SoundObjectTemplate*>(addSoundTemplate("Ambient"));      // vtable +0x110
    ambient->impl()->setIs3dSound(false);        // impl() +0x150; setIs3dSound +0x68
    ambient->impl()->setLoopCount(0);            // setLoopCount +0x18

    SoundObjectTemplate* damaged =
        static_cast<SoundObjectTemplate*>(addSoundTemplate("AmbientDamaged"));
    damaged->impl()->setIs3dSound(false);
    damaged->impl()->setLoopCount(0);
}

// queryImplementation - the standard one-CID hop: CID_PlayerControlObjectTemplate is this,
// everything else defers to BundleTemplate's chain.
void* PlayerControlObjectTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_PlayerControlObjectTemplate)
        return this;
    return BundleTemplate::queryImplementation(classId);
}

// Identifies the vehicle-template class independently of BundleTemplate.
unsigned int PlayerControlObjectTemplate::getClassID() const
{
    return CID_PlayerControlObjectTemplate;       // 50370
}

// Typed seat and vehicle accessors. getSeatTransformation returns the stored matrix by reference.
const std::string& PlayerControlObjectTemplate::getSoundFilter() const { return m_soundFilter; }
int PlayerControlObjectTemplate::getVehicleCategory() const             { return m_vehicleCategory; }
int PlayerControlObjectTemplate::getControlsCategory() const            { return m_controlsCategory; }
float PlayerControlObjectTemplate::getSeatLeftRotationLimit() const
{
    return m_seatParent.seatLeftRotationLimit;
}
float PlayerControlObjectTemplate::getSeatRightRotationLimit() const
{
    return m_seatParent.seatRightRotationLimit;
}
const Mat4& PlayerControlObjectTemplate::getSeatTransformation() const
{
    return m_seatParent.seatTransformation;
}
float PlayerControlObjectTemplate::getListenerObstruction() const       { return m_listenerObstruction; }
bool PlayerControlObjectTemplate::getMaintainCameraOnEnter()            { return m_maintainCameraOnEnter; }
bool PlayerControlObjectTemplate::getNeverDrawAs1p() const              { return m_neverDrawAs1p; }


void PlayerControlObjectTemplate::setPcoId(int value) { m_pcoId = value; }
int PlayerControlObjectTemplate::getPcoId() { return m_pcoId; }

// . Despite its name, this setter only adds flags.
void PlayerControlObjectTemplate::setPcoFlags(int value) { m_pcoFlags |= value; }

unsigned int PlayerControlObjectTemplate::getPcoFlags() const { return m_pcoFlags; }


void PlayerControlObjectTemplate::setVehicleCategory(VehicleCategory value) { m_vehicleCategory = value; }
void PlayerControlObjectTemplate::setControlsCategory(VehicleCategory value) { m_controlsCategory = value; }

void PlayerControlObjectTemplate::setVehicleType(VehicleType value) { m_vehicleType = value; }
int PlayerControlObjectTemplate::getVehicleType() { return m_vehicleType; }

void PlayerControlObjectTemplate::setToolTipType(ToolTipType value) { m_toolTipType = value; }
int PlayerControlObjectTemplate::getToolTipType() { return m_toolTipType; }
// ; ELF 0x8181650 / PE 0x5793C0 (AL-only return).
void PlayerControlObjectTemplate::setArtPos(bool value) { m_artPos = value; }
bool PlayerControlObjectTemplate::getArtPos() { return m_artPos; }


void PlayerControlObjectTemplate::setDirBarXScale(float value) { m_dirBarXScale = value; }
float PlayerControlObjectTemplate::getDirBarXScale() { return m_dirBarXScale; }

void PlayerControlObjectTemplate::setDirBarYScaleAbove(float value) { m_dirBarYScaleAbove = value; }
float PlayerControlObjectTemplate::getDirBarYScaleAbove() { return m_dirBarYScaleAbove; }

void PlayerControlObjectTemplate::setDirBarYScaleBelow(float value) { m_dirBarYScaleBelow = value; }
float PlayerControlObjectTemplate::getDirBarYScaleBelow() { return m_dirBarYScaleBelow; }

void PlayerControlObjectTemplate::setDirBarRotate(float value) { m_dirBarRotate = value; }
float PlayerControlObjectTemplate::getDirBarRotate() { return m_dirBarRotate; }

void PlayerControlObjectTemplate::setDirBarYScaleMin(float value) { m_dirBarYScaleMin = value; }
float PlayerControlObjectTemplate::getDirBarYScaleMin() { return m_dirBarYScaleMin; }

void PlayerControlObjectTemplate::setDirBarYScaleMax(float value) { m_dirBarYScaleMax = value; }
float PlayerControlObjectTemplate::getDirBarYScaleMax() { return m_dirBarYScaleMax; }

// (primary face), with PE vector::push_back at 0x46EB70.
void PlayerControlObjectTemplate::addPcoPosId(int value) { m_pcoPosIds.push_back(value); }

// The mangled names and PE's three stack arguments prove BY-value Vec3 setter parameters.

void PlayerControlObjectTemplate::setNameTagOffset(Vec3 value) { m_nameTagOffset = value; }
const Vec3& PlayerControlObjectTemplate::getNameTagOffset() { return m_nameTagOffset; }

void PlayerControlObjectTemplate::setRidingWithOffset(Vec3 value) { m_ridingWithOffset = value; }
const Vec3& PlayerControlObjectTemplate::getRidingWithOffset() { return m_ridingWithOffset; }

void PlayerControlObjectTemplate::setRidingWithDistMod(float value) { m_ridingWithDistMod = value; }
float PlayerControlObjectTemplate::getRidingWithDistMod() { return m_ridingWithDistMod; }

void PlayerControlObjectTemplate::setVehicleCameraShake(CameraShakeTriggers value) { m_vehicleCameraShake = value; }
int PlayerControlObjectTemplate::getVehicleCameraShake() { return m_vehicleCameraShake; }

void PlayerControlObjectTemplate::setVehicleFov(float value) { m_vehicleFoV = value; }
float PlayerControlObjectTemplate::getVehicleFov() { return m_vehicleFoV; }

// ; ELF 0x8181AC0 / PE 0x579450. No angle clamp or conversion.
void PlayerControlObjectTemplate::setSeatLeftRotationLimit(float value) { m_seatParent.seatLeftRotationLimit = value; }
void PlayerControlObjectTemplate::setSeatRightRotationLimit(float value) { m_seatParent.seatRightRotationLimit = value; }

// . Direct writes, not virtual setter calls. Identity resets all
// sixteen matrix entries before reading rotation; translation is read only after the rotation call.
void PlayerControlObjectTemplate::seatInformation(const std::string& parent, const Vec3& position,
                                                  const Vec3& rotation)
{
    m_seatParent.seatInformation = parent;
    m_seatParent.seatTransformation = Mat4::getIdentity();
    setRotationZXY(m_seatParent.seatTransformation, -rotation.z, -rotation.y, -rotation.x);
    m_seatParent.seatTransformation.m[12] = position.x;
    m_seatParent.seatTransformation.m[13] = position.y;
    m_seatParent.seatTransformation.m[14] = position.z;
}


void PlayerControlObjectTemplate::setSeatParent(const std::string& value) { m_seatParent.seatInformation = value; }
const std::string& PlayerControlObjectTemplate::getSeatParent() const { return m_seatParent.seatInformation; }

// . Only the three translation components change.
void PlayerControlObjectTemplate::setSeatPosition(const Vec3& position)
{
    m_seatParent.seatTransformation.m[12] = position.x;
    m_seatParent.seatTransformation.m[13] = position.y;
    m_seatParent.seatTransformation.m[14] = position.z;
}

// . Returns the matrix's translation storage, not a copied vector.
const Vec3& PlayerControlObjectTemplate::getSeatPosition() const
{
    return *reinterpret_cast<const Vec3*>(&m_seatParent.seatTransformation.m[12]);
}

// . Save translation, reset the whole matrix, rotate, restore.
void PlayerControlObjectTemplate::setSeatRotation(const Vec3& rotation)
{
    const Vec3 position = {m_seatParent.seatTransformation.m[12],
                           m_seatParent.seatTransformation.m[13],
                           m_seatParent.seatTransformation.m[14]};
    m_seatParent.seatTransformation = Mat4::getIdentity();
    setRotationZXY(m_seatParent.seatTransformation, -rotation.z, -rotation.y, -rotation.x);
    m_seatParent.seatTransformation.m[12] = position.x;
    m_seatParent.seatTransformation.m[13] = position.y;
    m_seatParent.seatTransformation.m[14] = position.z;
}

// . Shipped defect: this static is initialized by the first calling
// template only. Later calls neither decompose the matrix nor update the result, even on another
// template. [INFERRED] lambda spelling of the observed function-local static initializer.
const Vec3& PlayerControlObjectTemplate::getSeatRotation() const
{
    static const Vec3 rotation = [this]() {
        float z, x, y;
        getRotationZXY(m_seatParent.seatTransformation, z, x, y);
        return Vec3{-y, -x, -z};
    }();
    return rotation;
}


void PlayerControlObjectTemplate::setSoundFilter(const std::string& value) { m_soundFilter = value; }

// > clamp helper 0x402500. [PLATFORM] Both clamp to [0,1],
// but unordered compares retain NaN in ELF and select the lower bound in PE. Keep signed zero.
void PlayerControlObjectTemplate::setListenerObstruction(float value)
{
#if defined(_WIN32)
    const float capped = value >= 1.0f ? 1.0f : value;
    m_listenerObstruction = capped >= 0.0f ? capped : 0.0f;
#else
    m_listenerObstruction = value < 0.0f ? 0.0f : (value > 1.0f ? 1.0f : value);
#endif
}


void PlayerControlObjectTemplate::setMaintainCameraOnExit(bool value) { m_maintainCameraOnExit = value; }
bool PlayerControlObjectTemplate::getMaintainCameraOnExit() { return m_maintainCameraOnExit; }

void PlayerControlObjectTemplate::setMaintainCameraOnEnter(bool value) { m_maintainCameraOnEnter = value; }
void PlayerControlObjectTemplate::setNeverDrawAs1p(bool value) { m_neverDrawAs1p = value; }

IObject* PlayerControlObjectTemplate::createObject() const
{
    return new PlayerControlObject(this);          // operator new(0x3BC)
}

// makeScript - ELF 0x819F8C0 (5,589 B) / PE 0x596200 (3,525 B), read in full in both ports.
// BundleTemplate's result is a hard gate. Every field below, including the repeated vector size
// and set end reads, follows the retail order; every emitted line uses std::endl. The enum stream
// operators are the shipped vocabulary functions recovered in ModuleWorldObjTemplGame.cpp.
bool PlayerControlObjectTemplate::makeScript(io::IStream* stream)
{
    if (!BundleTemplate::makeScript(stream))
        return false;

    std::ostringstream script;

    if (m_pcoId != -1)
        script << "ObjectTemplate.setPcoId " << m_pcoId << std::endl;

    for (int i = 0; i < static_cast<int>(m_pcoPosIds.size()); ++i)
        script << "ObjectTemplate.addPcoPosId " << m_pcoPosIds[i] << std::endl;

    if (m_artPos)
        script << "ObjectTemplate.artPos 1" << std::endl;

    if (m_soldierExitPosition.x * m_soldierExitPosition.x
            + m_soldierExitPosition.y * m_soldierExitPosition.y
            + m_soldierExitPosition.z * m_soldierExitPosition.z > 0.0f
        || m_soldierExitRotation.x * m_soldierExitRotation.x
            + m_soldierExitRotation.y * m_soldierExitRotation.y
            + m_soldierExitRotation.z * m_soldierExitRotation.z > 0.0f)
    {
        script << "ObjectTemplate.setSoldierExitLocation "
               << m_soldierExitPosition.x << '/' << m_soldierExitPosition.y << '/'
               << m_soldierExitPosition.z << ' '
               << m_soldierExitRotation.x << '/' << m_soldierExitRotation.y << '/'
               << m_soldierExitRotation.z << std::endl;
    }

    if (m_altSoldierExitPosition.x * m_altSoldierExitPosition.x
            + m_altSoldierExitPosition.y * m_altSoldierExitPosition.y
            + m_altSoldierExitPosition.z * m_altSoldierExitPosition.z > 0.0f)
    {
        script << "ObjectTemplate.altSoldierExitPosition "
               << m_altSoldierExitPosition.x << '/' << m_altSoldierExitPosition.y << '/'
               << m_altSoldierExitPosition.z << std::endl;
    }

    if (m_disableSpawnPointsOnEnter)
        script << "ObjectTemplate.disableSpawnPointsOnEnter 1" << std::endl;
    if (m_dontClearTeamOnExit)
        script << "ObjectTemplate.dontClearTeamOnExit 1" << std::endl;
    if (m_destroyOnExit)
        script << "ObjectTemplate.destroyOnExit 1" << std::endl;
    if (m_dontAllowExit)
        script << "ObjectTemplate.dontAllowExit 1" << std::endl;

    if (m_regulateYawInput != 64)
    {
        script << "ObjectTemplate.regulateYawInput "
               << static_cast<io::PlayerInputMap>(m_regulateYawInput) << std::endl;
        script << "ObjectTemplate.regulateYaw " << m_regulateYaw.x << '/'
               << m_regulateYaw.y << std::endl;
    }
    if (m_regulatePitchInput != 64)
    {
        script << "ObjectTemplate.regulatePitchInput "
               << static_cast<io::PlayerInputMap>(m_regulatePitchInput) << std::endl;
        script << "ObjectTemplate.regulatePitch " << m_regulatePitch.x << '/'
               << m_regulatePitch.y << std::endl;
    }
    if (m_regulateRollInput != 64)
    {
        script << "ObjectTemplate.regulateRollInput "
               << static_cast<io::PlayerInputMap>(m_regulateRollInput) << std::endl;
        script << "ObjectTemplate.regulateRoll " << m_regulateRoll.x << '/'
               << m_regulateRoll.y << std::endl;
    }
    if (m_regulateVerticalPosInput != 64)
    {
        script << "ObjectTemplate.regulateVerticalPosInput "
               << static_cast<io::PlayerInputMap>(m_regulateVerticalPosInput) << std::endl;
        script << "ObjectTemplate.regulateVerticalPos " << m_regulateVerticalPos.x << '/'
               << m_regulateVerticalPos.y << std::endl;
    }

    if (m_maxVertRegAngle != 10.0f)
        script << "ObjectTemplate.maxVertRegAngle " << m_maxVertRegAngle << std::endl;
    if (m_noVertRegAngle != 20.0f)
        script << "ObjectTemplate.noVertRegAngle " << m_noVertRegAngle << std::endl;

    if (!m_seatParent.seatInformation.empty())
    {
        script << "ObjectTemplate.seatInformation " << m_seatParent.seatInformation << ' '
               << m_seatParent.seatTransformation.m[12] << '/'
               << m_seatParent.seatTransformation.m[13] << '/'
               << m_seatParent.seatTransformation.m[14] << ' ';

        float zDegrees;
        float xDegrees;
        float yDegrees;
        getRotationZXY(m_seatParent.seatTransformation, zDegrees, xDegrees, yDegrees);
        script << -yDegrees << '/' << -xDegrees << '/' << -zDegrees << std::endl;
    }

    if (m_seatParent.seatAnimationSystem)
    {
        script << "ObjectTemplate.seatAnimationSystem "
               << m_seatParent.seatAnimationSystem->getPath() << std::endl;
    }
    if (m_seatParent.seatLeftRotationLimit != 0.0f)
    {
        script << "ObjectTemplate.seatLeftRotationLimit "
               << m_seatParent.seatLeftRotationLimit << std::endl;
    }
    if (m_seatParent.seatRightRotationLimit != 0.0f)
    {
        script << "ObjectTemplate.seatRightRotationLimit "
               << m_seatParent.seatRightRotationLimit << std::endl;
    }

    if (m_toggleWeapon)
        script << "ObjectTemplate.toggleWeapon 1" << std::endl;
    if (!m_soundFilter.empty())
        script << "ObjectTemplate.soundFilter " << m_soundFilter << std::endl;

    for (std::set<int>::const_iterator it = m_targetObjectTypesToWatch.begin();
         it != m_targetObjectTypesToWatch.end(); ++it)
    {
        script << "ObjectTemplate.addTargetObjectTypeToWatch " << *it << std::endl;
    }

    if (m_cockpitLod != 0)
        script << "ObjectTemplate.cockpitLod " << m_cockpitLod << std::endl;
    if (m_cockpitSubGeom != 0)
        script << "ObjectTemplate.cockpitSubGeom " << m_cockpitSubGeom << std::endl;

    if (m_shakeFactor != 1.0f)
        script << "ObjectTemplate.shakeFactor " << m_shakeFactor << std::endl;
    if (m_sprintRecoverTime != 0.0f)
        script << "ObjectTemplate.sprintRecoverTime " << m_sprintRecoverTime << std::endl;
    if (m_sprintDissipationTime != 0.0f)
        script << "ObjectTemplate.sprintDissipationTime " << m_sprintDissipationTime << std::endl;
    if (m_sprintLimit != 0.0f)
        script << "ObjectTemplate.sprintLimit " << m_sprintLimit << std::endl;
    if (m_sprintFactor != 2.5f)
        script << "ObjectTemplate.sprintFactor " << m_sprintFactor << std::endl;

    if (m_specialToggleWeaponInput != 64)
    {
        script << "ObjectTemplate.specialToggleWeaponInput "
               << static_cast<io::PlayerInputMap>(m_specialToggleWeaponInput) << std::endl;
    }
    if (m_vehicleCategory != 0)
    {
        script << "ObjectTemplate.vehicleCategory "
               << static_cast<VehicleCategory>(m_vehicleCategory) << std::endl;
    }
    if (m_controlsCategory != 4)
    {
        script << "ObjectTemplate.controlsCategory "
               << static_cast<VehicleCategory>(m_controlsCategory) << std::endl;
    }
    if (m_vehicleType != 26)
    {
        script << "ObjectTemplate.setVehicleType "
               << static_cast<VehicleType>(m_vehicleType) << std::endl;
    }

    if (m_allowDucking)
        script << "ObjectTemplate.allowDucking 1" << std::endl;
    if (!m_disableInputWhileDucking)
        script << "ObjectTemplate.disableInputWhileDucking 0" << std::endl;
    if (m_listenerObstruction != 0.0f)
        script << "ObjectTemplate.listenerObstruction " << m_listenerObstruction << std::endl;
    if (m_groundContactVolume != 0.0f)
        script << "ObjectTemplate.groundContactVolume " << m_groundContactVolume << std::endl;
    if (m_damagedAmbientSoundLimit != 0.5f)
    {
        script << "ObjectTemplate.damagedAmbientSoundLimit "
               << m_damagedAmbientSoundLimit << std::endl;
    }
    if (m_crewKitIndex != -1)
        script << "ObjectTemplate.crewKitIndex " << m_crewKitIndex << std::endl;
    if (m_autoUseAbility)
        script << "ObjectTemplate.autoUseAbility " << m_autoUseAbility << std::endl;
    if (m_hasRestrictedExit)
        script << "ObjectTemplate.hasRestrictedExit 1" << std::endl;
    if (m_neverDrawAs1p)
        script << "ObjectTemplate.neverDrawAs1p 1" << std::endl;
    if (m_maintainCameraOnExit)
    {
        script << "ObjectTemplate.maintainCameraOnExit "
               << m_maintainCameraOnExit << std::endl;
    }
    if (m_maintainCameraOnEnter)
    {
        script << "ObjectTemplate.maintainCameraOnEnter "
               << m_maintainCameraOnEnter << std::endl;
    }
    if (std::fabs(m_boundingRadiusModifierWhenOccupied) > FLT_EPSILON)
    {
        script << "ObjectTemplate.boundingRadiusModifierWhenOccupied "
               << m_boundingRadiusModifierWhenOccupied << std::endl;
    }

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

// Ladder-container template adds its own reference count and sets flag 0x40.
LadderContainerTemplate::LadderContainerTemplate()
{
    m_refCount = 1; // +0x308
    updateFlags(0x40, 0);                         // the ctor's tail
}

IObject* LadderContainerTemplate::createObject() const
{
    return new LadderContainer(this);
}

LadderContainerTemplate::~LadderContainerTemplate()
{
}

unsigned int LadderContainerTemplate::getClassID() const
{
    return CID_LadderContainerTemplate;           // 40006
}

void* LadderContainerTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_LadderContainerTemplate)   // read in full
        return this;
    return PlayerControlObjectTemplate::queryImplementation(classId);
}

// ..0x818D340. The DECLARE_CLASS expansion emits an own quartet over the leaf's
// +0x308 refcount; queryInterface forwards through the PCO-template chain.
void LadderContainerTemplate::addRef() { ++m_refCount; }
int LadderContainerTemplate::getRef() { return m_refCount; }
int LadderContainerTemplate::release()
{
    const int count = --m_refCount;
    if (count == 0)
        delete this;
    return count;
}
void* LadderContainerTemplate::queryInterface(unsigned int iid) const
{
    return PlayerControlObjectTemplate::queryInterface(iid);
}

// Enables collision-group bit 3 without changing object flags.
ParachuteTemplate::ParachuteTemplate()
{
    m_collisionGroups |= 8;
}

ParachuteTemplate::~ParachuteTemplate() {}

unsigned int ParachuteTemplate::getClassID() const { return CID_ParachuteTemplate; }

void* ParachuteTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_ParachuteTemplate)
        return this;
    return PlayerControlObjectTemplate::queryImplementation(classId);
}

// Creates the parachute object used by deploy-weapon projectile templates.
IObject* ParachuteTemplate::createObject() const
{
    return new Parachute(this);
}

// (849 B) / PE 0x599720 (151 B). The leaf contributes no text, but retail still
// constructs an ostringstream and performs the zero-length write after the PCO serializer passes.
bool ParachuteTemplate::makeScript(io::IStream* stream)
{
    if (!PlayerControlObjectTemplate::makeScript(stream))
        return false;

    std::ostringstream script;
    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

// ZiplineContainerTemplate - the LadderContainer's twin: identical 0x30C shape, the same refcount
// at +0x308 seeded to 1, the same updateFlags(0x40). The one difference is that it re-declares the
// IBase quartet, and its addRef is what named that offset for both classes.
ZiplineContainerTemplate::ZiplineContainerTemplate()
{
    m_refCount = 1;                               // +0x308
    updateFlags(0x40, 0);
}

ZiplineContainerTemplate::~ZiplineContainerTemplate() {}

unsigned int ZiplineContainerTemplate::getClassID() const { return CID_ZiplineContainerTemplate; }

void* ZiplineContainerTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_ZiplineContainerTemplate)
        return this;
    return PlayerControlObjectTemplate::queryImplementation(classId);
}

void ZiplineContainerTemplate::addRef()  { ++m_refCount; }
int  ZiplineContainerTemplate::getRef()  { return m_refCount; }

int ZiplineContainerTemplate::release()
{
    const int r = --m_refCount;
    if (r == 0)
        delete this;
    return r;
}

void* ZiplineContainerTemplate::queryInterface(unsigned int iid) const
{
    return PlayerControlObjectTemplate::queryInterface(iid);
}

// createObject: ELF 0x81D7000 (47 B), operator new(0x450); PE 0x57F000, operator new(0x4A0).
IObject* ZiplineContainerTemplate::createObject() const
{
    return new ZiplineContainer(this);
}

// GrapplingHookRopeContainerTemplate - ELF ctor 0x817F440 / PE counterpart. The leaf's complete
// surface is the same one-cid PCO-template hop as Ladder/Zipline, with one extra 1.5f scalar.
GrapplingHookRopeContainerTemplate::GrapplingHookRopeContainerTemplate()
    : m_float30C(1.5f)
{
    m_refCount = 1;
    updateFlags(0x40, 0);
}

GrapplingHookRopeContainerTemplate::~GrapplingHookRopeContainerTemplate() {}

unsigned int GrapplingHookRopeContainerTemplate::getClassID() const
{
    return CID_GrapplingHookRopeContainerTemplate;
}

void* GrapplingHookRopeContainerTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_GrapplingHookRopeContainerTemplate)
        return this;
    return PlayerControlObjectTemplate::queryImplementation(classId);
}

void GrapplingHookRopeContainerTemplate::addRef() { ++m_refCount; }
int GrapplingHookRopeContainerTemplate::getRef() { return m_refCount; }
int GrapplingHookRopeContainerTemplate::release()
{
    const int left = --m_refCount;
    if (left == 0)
        delete this;
    return left;
}

void* GrapplingHookRopeContainerTemplate::queryInterface(unsigned int iid) const
{
    return PlayerControlObjectTemplate::queryInterface(iid);
}

IObject* GrapplingHookRopeContainerTemplate::createObject() const
{
    return new GrapplingHookRopeContainer(this);
}

} // namespace world
} // namespace hfe
} // namespace dice
