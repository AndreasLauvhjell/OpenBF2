// Interfaces for the six aspects of an AI object. Each plug-in borrows its owner and template.
// [INFERRED] Return types are derived from implementations.
// IAIObjectArmament has a pure virtual destructor with a definition.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_IAIOBJECTPLUGIN_H
#define AI_AIINTERFACE_BFKNOWLEDGE_IAIOBJECTPLUGIN_H

#include <vector>

#include "IAIObject.h"
#include "../../../IO/Input/Input.h"            // io::PlayerInputMap

namespace dice {
namespace hfe {

class BoundingBox;           // declared by name only - no class in this tree yet
struct LocalBoundingBox;     // Math/LocalBoundingBox.h
class IPlanePolygon;       // declared by name only - see AITemplateCover.h

namespace ai {

class IWeapon;

// IAIObjectPlugIn - six slots, no data. / .
class IAIObjectPlugIn
{
public:
    virtual ~IAIObjectPlugIn() {}                                                // +0x00/+0x04
    // The plug-in's index - AIObject::aiObjectAddPlugIn switches on it (IAIObjectFlags values).
    virtual int getId() const = 0;                                              // +0x08 / PE +0x04
    // Asks the AI DLL's information factory for this plug-in's information half on `handle`.
    // [INFERRED] void: every override tail-calls the factory and no caller reads the result.
    virtual void createInformationPlugIn(unsigned int handle, bool real) = 0;   // +0x0C / PE +0x08
    virtual void init() = 0;                                                    // +0x10 / PE +0x0C
    virtual void update() = 0;                                                  // +0x14 / PE +0x10
};

// IAIObjectArmament - 17 slots. / ; pure destructor .
class IAIObjectArmament : public IAIObjectPlugIn
{
public:
    explicit IAIObjectArmament(IAIObject* object) : m_object(object) {}
    virtual ~IAIObjectArmament() = 0;

    virtual int isAntiAircraft() const = 0;                                     // +0x18
    // The index into getWeapons of the weapon in hand, or -1.
    virtual int getActiveWeapon() const = 0;                                    // +0x1C
    virtual void setActiveWeapon(unsigned int index, int side) = 0;             // +0x20
    virtual IWeapon* getWeapon(unsigned int index) const = 0;                   // +0x24
    virtual void setLastFiredWeapon() = 0;                                      // +0x28
    virtual IWeapon* getLastFiredWeapon() const = 0;                            // +0x2C
    virtual const std::vector<IWeapon*>& getWeapons() const = 0;                // +0x30
    virtual int getWeaponIdFromActivationId(int activation, int fire) const = 0;    // +0x34
    virtual bool rearm(IWeapon* weapon, float amount, int side) = 0;            // +0x38
    virtual bool getProjectilePosition(Vec3& out) const = 0;                    // +0x3C
    virtual bool getProjectileSpeed(Vec3& out) const = 0;                       // +0x40

protected:
    /* +0x04 */ IAIObject* m_object;
};

inline IAIObjectArmament::~IAIObjectArmament() {}

// IAIObjectControlInfo - 57 slots. / ; / .
// The declaration order is the table's, and it is not grouped: the pitch/roll drive and aim
// controls were appended at the end (+0xD4..+0xE0) after the camera block.
class IAIObjectControlInfo : public IAIObjectPlugIn
{
public:
    explicit IAIObjectControlInfo(IAIObject* object) : m_object(object) {}
    virtual ~IAIObjectControlInfo() {}

    virtual unsigned long long getDriveControls() const = 0;                    // +0x18
    virtual io::PlayerInputMap getDriveThrottleControl() const = 0;             // +0x1C
    virtual io::PlayerInputMap getDriveTurnControl() const = 0;                 // +0x20
    virtual unsigned long long getAimControls() const = 0;                      // +0x24
    virtual io::PlayerInputMap getAimVerticalControl() const = 0;               // +0x28
    virtual io::PlayerInputMap getAimHorizontalControl() const = 0;             // +0x2C
    virtual unsigned long long getLookControls() const = 0;                     // +0x30
    virtual io::PlayerInputMap getLookVerticalControl() const = 0;              // +0x34
    virtual io::PlayerInputMap getLookHorizontalControl() const = 0;            // +0x38
    virtual float getThrottleSensitivity() const = 0;                           // +0x3C
    virtual float getPitchSensitivity() const = 0;                              // +0x40
    virtual float getRollSensitivity() const = 0;                               // +0x44
    virtual float getYawSensitivity() const = 0;                                // +0x48
    virtual float getProportionalConstant() const = 0;                          // +0x4C
    virtual float getDerivativeConstant() const = 0;                            // +0x50
    virtual float getAngleSpeedConstant() const = 0;                            // +0x54
    virtual float getThrottleLookAhead() const = 0;                             // +0x58
    virtual float getPitchLookAhead() const = 0;                                // +0x5C
    virtual float getRollLookAhead() const = 0;                                 // +0x60
    virtual float getYawLookAhead() const = 0;                                  // +0x64
    virtual float getThrottleScale() const = 0;                                 // +0x68
    virtual float getPitchScale() const = 0;                                    // +0x6C
    virtual float getRollScale() const = 0;                                     // +0x70
    virtual float getYawScale() const = 0;                                      // +0x74
    virtual float getLookVerticalSensitivity() const = 0;                       // +0x78
    virtual float getLookHorizontalSensitivity() const = 0;                     // +0x7C
    virtual float getLookVerticalLookAhead() const = 0;                         // +0x80
    virtual float getLookHorizontalLookAhead() const = 0;                       // +0x84
    virtual float getLookVerticalScale() const = 0;                             // +0x88
    virtual float getLookHorizontalScale() const = 0;                           // +0x8C
    virtual float getMaxSpeedBumpinessFallOff() const = 0;                      // +0x90
    virtual float getCameraRelativeDofRotationOffset() const = 0;               // +0x94
    virtual const Vec3& getCameraRelativeMinRotation() const = 0;               // +0x98
    virtual const Vec3& getCameraRelativeMaxRotation() const = 0;               // +0x9C
    virtual void getCameraRelativeRotation(Vec3& out) const = 0;                // +0xA0
    virtual const Mat4& getCameraTransformation() const = 0;                    // +0xA4
    virtual void getCameraBaseTransformation(Mat4& out) const = 0;              // +0xA8
    virtual void getCameraBaseRight(Vec3& out) const = 0;                       // +0xAC
    virtual void getCameraBaseUp(Vec3& out) const = 0;                          // +0xB0
    virtual void getCameraBaseDof(Vec3& out) const = 0;                         // +0xB4
    virtual void getCameraBasePos(Vec3& out) const = 0;                         // +0xB8
    virtual bool validateCameraDirection(const Vec3& direction) const = 0;      // +0xBC
    virtual bool validateCameraDirectionYaw(const Vec3& direction) const = 0;   // +0xC0
    virtual bool validateCameraDirectionPitch(const Vec3& direction) const = 0; // +0xC4
    virtual bool validateCameraDirectionByPos(const Vec3& position) const = 0;  // +0xC8
    virtual float getMaxClimbAngle() const = 0;                                 // +0xCC
    virtual float getMaxRollAngle() const = 0;                                  // +0xD0
    virtual io::PlayerInputMap getDriveRollControl() const = 0;                 // +0xD4
    virtual io::PlayerInputMap getDrivePitchControl() const = 0;                // +0xD8
    virtual io::PlayerInputMap getAimThrottleControl() const = 0;               // +0xDC
    virtual io::PlayerInputMap getAimRollControl() const = 0;                   // +0xE0

protected:
    /* +0x04 */ IAIObject* m_object;
};

// IAIObjectCover - 22 slots. / ; / .
class IAIObjectCover : public IAIObjectPlugIn
{
public:
    explicit IAIObjectCover(IAIObject* object) : m_object(object) {}
    virtual ~IAIObjectCover() {}

    // The outline is the template's IPlanePolygon; both non-const overloads hand back a new polygon
    // placed by a transformation (IPlanePolygon::fromTransform - the caller owns it).
    virtual IPlanePolygon* getObjectOutline(Mat4& transformation) = 0;          // +0x18
    virtual IPlanePolygon* getObjectOutline() = 0;                              // +0x1C
    virtual IPlanePolygon* getLocalObjectOutline() = 0;                         // +0x20
    virtual float getCoverValue() const = 0;                                    // +0x24
    virtual float getHeight() const = 0;                                        // +0x28
    virtual float getTransparency() const = 0;                                  // +0x2C
    virtual unsigned int getPortalCount() const = 0;                            // +0x30
    virtual unsigned int getPortalVertexCount(unsigned int portal) const = 0;   // +0x34
    virtual const Vec3& getPortalVertex(unsigned int portal, unsigned int vertex) const = 0;  // +0x38
    virtual void getPortal(unsigned int portal, std::vector<Vec3>& out) const = 0;  // +0x3C
    virtual const Vec3& getPortalCenter(unsigned int portal) const = 0;         // +0x40
    virtual const Vec3& getPortalNormal(unsigned int portal) const = 0;         // +0x44
    virtual float getPortalXZExtentR(unsigned int portal) const = 0;            // +0x48
    virtual float getPortalXZExtentL(unsigned int portal) const = 0;            // +0x4C
    virtual int getPathfindingPlaneCount() const = 0;                           // +0x50
    virtual const Vec3& getPathfindingPlaneCorner(int plane, int corner) const = 0;  // +0x54

protected:
    /* +0x04 */ IAIObject* m_object;
};

// IAIObjectMobile - 26 slots. / ; / .
class IAIObjectMobile : public IAIObjectPlugIn
{
public:
    explicit IAIObjectMobile(IAIObject* object) : m_object(object) {}
    virtual ~IAIObjectMobile() {}

    virtual const Vec3& getSpeed() = 0;                                         // +0x18
    virtual const Vec3& getRotationalSpeed() = 0;                               // +0x1C
    virtual float getSpeedMagnitude() = 0;                                      // +0x20
    virtual int getVehicleNumber() const = 0;                                   // +0x24
    virtual float getMaxSpeed() const = 0;                                      // +0x28
    virtual float getTurnRadius() const = 0;                                    // +0x2C
    virtual float getDrag() const = 0;                                          // +0x30
    virtual float getCoverSearchRadius() const = 0;                             // +0x34
    virtual float getLodHeight() const = 0;                                     // +0x38
    virtual bool isTurnable() const = 0;                                        // +0x3C
    virtual float getSoundSphereRadius() const = 0;                             // +0x40
    virtual float getSoundSphereRadiusMin() const = 0;                          // +0x44
    virtual float getSoundSphereRadiusMax() const = 0;                          // +0x48
    virtual float getHearingProbabilityMax() const = 0;                         // +0x4C
    virtual float getHearingProbabilityMin() const = 0;                         // +0x50
    virtual float getAvoidCollisionLookAhead() const = 0;                       // +0x54
    virtual IAIObject* getSector() const = 0;                                   // +0x58
    virtual int getLastPassedPortal() const = 0;                                // +0x5C
    virtual void positionChanged() = 0;                                         // +0x60
    virtual float getSprintStamina() const = 0;                                 // +0x64

protected:
    /* +0x04 */ IAIObject* m_object;
};

// IAIObjectPhysical - 32 slots. / ; / .
class IAIObjectPhysical : public IAIObjectPlugIn
{
public:
    explicit IAIObjectPhysical(IAIObject* object) : m_object(object) {}
    virtual ~IAIObjectPhysical() {}

    virtual float getHealth() = 0;                                              // +0x18
    virtual float getMaxHealth() const = 0;                                     // +0x1C
    virtual float getMass() const = 0;                                          // +0x20
    virtual float getVolume() const = 0;                                        // +0x24
    virtual float getArmour(const Vec3& direction) const = 0;                   // +0x28
    virtual float getBestArmour() const = 0;                                    // +0x2C
    virtual const LocalBoundingBox* getLocalBoundingBox() const = 0;            // +0x30
    // both bounding-BOX slots return A fresh `new BoundingBox` (0x60 bytes) the caller owns.
    virtual BoundingBox* getCurrentBoundingBox() = 0;                           // +0x34
    virtual BoundingBox* getBoundingBox(const Mat4& transformation) = 0;        // +0x38
    virtual float getRadius() const = 0;                                        // +0x3C
    virtual Vec3 getSphereLocalOffset() const = 0;                              // +0x40
    virtual float getSmallestRadius() const = 0;                                // +0x44
    virtual float getSubSpheres2d(std::vector<Vec2>& out) const = 0;            // +0x48
    virtual int getStrType() const = 0;                                         // +0x4C
    virtual bool isTouchingTerrain(Vec3& out) const = 0;                        // +0x50
    virtual bool isInAir() const = 0;                                           // +0x54
    virtual bool isInWater() const = 0;                                         // +0x58
    virtual int getMaterial() const = 0;                                        // +0x5C
    virtual float getManDownTimeLeft() const = 0;                               // +0x60
    virtual float getMaxManDownTime() const = 0;                                // +0x64
    virtual bool getManDown() const = 0;                                        // +0x68
    virtual bool doesAffectPathfinding() const = 0;                             // +0x6C
    virtual int getMapPosId() const = 0;                                        // +0x70
    virtual bool enablePhysics(int side) = 0;                                   // +0x74
    virtual bool disablePhysics(int side) = 0;                                  // +0x78
    virtual bool hasPhysicsEnabled() = 0;                                       // +0x7C

protected:
    /* +0x04 */ IAIObject* m_object;
};

// IAIObjectUnit - 33 slots. / ; / .
class IAIObjectUnit : public IAIObjectPlugIn
{
public:
    explicit IAIObjectUnit(IAIObject* object) : m_object(object) {}
    virtual ~IAIObjectUnit() {}

    virtual int getAmmo() const = 0;                                            // +0x18
    virtual int getMaxAmmo() const = 0;                                         // +0x1C
    virtual const int* getBattleStrengths() const = 0;                          // +0x20
    virtual const int* getStrategicStrengths() const = 0;                       // +0x24
    virtual int getBattleStrength(int index) const = 0;                         // +0x28
    virtual int getStrategicStrength(int index) const = 0;                      // +0x2C
    virtual void setBattleStrength(int index, int value) = 0;                   // +0x30
    virtual void setStrategicStrength(int index, int value) = 0;                // +0x34
    virtual int getEquipmentType() const = 0;                                   // +0x38
    virtual bool useNoPathfindingToGetToObject() const = 0;                     // +0x3C
    virtual bool hasExposedSoldier() const = 0;                                 // +0x40
    virtual bool getAlwaysTargetSoldier() const = 0;                            // +0x44
    virtual bool isOccupied() const = 0;                                        // +0x48
    virtual float getCullDistance() const = 0;                                  // +0x4C
    virtual float getCullDistanceSqr() const = 0;                               // +0x50
    virtual float getEntryRadius() const = 0;                                   // +0x54
    virtual IAIObject* getOccupier() const = 0;                                 // +0x58
    virtual IAIObject* getOccupiedObject() const = 0;                           // +0x5C
    virtual double getLastPlayerPossesionTime() const = 0;                      // +0x60 (dice's spelling)
    virtual void setLastPlayerPossesionTime(double time) = 0;                   // +0x64
    virtual int getTeamLock() const = 0;                                        // +0x68
    virtual bool mustBorrowArmament(IAIObject* vehicle) const = 0;              // +0x6C
    virtual int getOccupyingPlayerId() const = 0;                               // +0x70
    virtual io::PlayerInputMap getSelectKey() const = 0;                        // +0x74
    virtual void setMissileLockStatus(int status) = 0;                          // +0x78
    virtual int getMissileLockStatus() const = 0;                               // +0x7C
    virtual const Vec3& getEntryPositionOfNonMobile() const = 0;                // +0x80

protected:
    /* +0x04 */ IAIObject* m_object;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_IAIOBJECTPLUGIN_H
