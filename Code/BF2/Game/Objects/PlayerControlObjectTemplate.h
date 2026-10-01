// Configuration for objects a player can occupy, including seat, exit, camera and sprint settings.
// Exit restrictions and alternate position are independent controls. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_PLAYERCONTROLOBJECTTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_PLAYERCONTROLOBJECTTEMPLATE_H

#include "Base/BundleTemplate.h"
#include "IPlayerControlObjectTemplate.h"
#include "../../Math/Vec3.h"
#include <set>
#include <string>
#include <vector>

namespace dice {
namespace anim { class AnimationSystemTemplate; }
namespace hfe {
class Player;
class ServerGameLogic;    // toggleEntryPoint's raw +0x2A8 read - see the friend note below
namespace world {

// Seat transform and asymmetric left/right rotation limits.
struct SeatTemplateInformation
{
    // Initializes the seat matrix with m[0] zero and the other diagonal elements one.
    SeatTemplateInformation();
    /*  0 */ std::string seatInformation;               // retyped (ctor constructs a string here)
    /*  4 */ Mat4 seatTransformation;
    /* 68 */ anim::AnimationSystemTemplate* seatAnimationSystem;
    /* 72 */ float seatLeftRotationLimit;               // asymmetric traverse
    /* 76 */ float seatRightRotationLimit;
};

class PlayerControlObject;                       // the object half reads pcoId + the alternate
                                                 // exit directly (Game/Objects/Game/)
struct GameTemplateConsoleAccess;
class PlayerControlObjectTemplate : public BundleTemplate, public IPlayerControlObjectTemplate
{
public:
    friend class PlayerControlObject;            // the object ctor's two template reads (m_pcoId,
                                                 // m_altSoldierExitPosition) - the sibling pattern
    friend class ::dice::hfe::Player;             // hasGasCloudImmunity's shipped open-vehicle read
    // toggleEntryPoint's exit-momentum scale: it reads m_positionalSpeedModifier (+0x2A8) RAW off
    // the exited vehicle's template, with no accessor call in either port (0x81042B0:
    // `fld dword ptr [eax+2A8h]`). It reaches this field for CID_LadderTemplate too, which is a
    // 364-byte SimpleObjectTemplate leaf with no such field - a shipped out-of-bounds read,
    // reproduced deliberately; see the body in Game/GameLogic/ServerGameLogic.cpp.
    friend class ::dice::hfe::ServerGameLogic;
    friend struct GameTemplateConsoleAccess;     // recovered direct-field exit console executors

    // Bodies the LadderContainer chain
    // made them load-bearing:
    PlayerControlObjectTemplate();
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual ~PlayerControlObjectTemplate();
    virtual void  templateInitialize();                              // adds the
                                                                     // "Ambient"/"AmbientDamaged"
                                                                     // sound templates
    virtual void* queryImplementation(unsigned int classId);         // CID_PlayerControlObject-
    virtual unsigned int getClassID() const;                         // +0x10
    virtual IObject* createObject() const;                           // +0x54
    virtual bool makeScript(io::IStream*);                           // +0xD4
                                                                     // Vehicle-seat virtual interface. [PLATFORM] Win32 keeps most accessors in the secondary interface table.
                                                                     // Seat rotation is stored in a matrix; its setter preserves translation.
                                                                     // The Euler getter returns a first-call static cache shared by all templates.

    virtual void  setPcoId(int);                                     // +0x140
    virtual int   getPcoId();                                        // +0x144
    virtual void  setPcoFlags(int);                                  // +0x148 bit work
    virtual unsigned int getPcoFlags() const;                        // +0x14C
    virtual void  setVehicleCategory(VehicleCategory);               // +0x150
    virtual int   getVehicleCategory() const;                        // +0x154
    virtual void  setControlsCategory(VehicleCategory);              // +0x158
    virtual int   getControlsCategory() const;                       // +0x15C
    virtual void  setVehicleType(VehicleType);                       // +0x160
    virtual int   getVehicleType();                                  // +0x164
    virtual void  setToolTipType(ToolTipType);                       // +0x168
    virtual int   getToolTipType();                                  // +0x16C

    virtual void  setSoldierExitLocation(const Vec3&, const Vec3&);  // +0x170
    virtual void  setSoldierExitPosition(const Vec3&);               // +0x174
    virtual Vec3  getSoldierExitPosition();                          // +0x178 , returned by value
    virtual void  setSoldierExitRotation(const Vec3&);               // +0x17C
    virtual Vec3  getSoldierExitRotation();                          // +0x180 , returned by value

    virtual void  setArtPos(bool);                                   // +0x184
    virtual bool  getArtPos();                                       // +0x188 PE 0x5793C0 (AL)

    // The HUD direction-bar block — six pairs, all trivial.
    virtual void  setDirBarXScale(float);                            // +0x18C
    virtual float getDirBarXScale();                                 // +0x190
    virtual void  setDirBarYScaleAbove(float);                       // +0x194
    virtual float getDirBarYScaleAbove();                            // +0x198
    virtual void  setDirBarYScaleBelow(float);                       // +0x19C
    virtual float getDirBarYScaleBelow();                            // +0x1A0
    virtual void  setDirBarRotate(float);                            // +0x1A4
    virtual float getDirBarRotate();                                 // +0x1A8
    virtual void  setDirBarYScaleMin(float);                         // +0x1AC
    virtual float getDirBarYScaleMin();                              // +0x1B0
    virtual void  setDirBarYScaleMax(float);                         // +0x1B4
    virtual float getDirBarYScaleMax();                              // +0x1B8

    // and named `add` — it appends rather than assigns.
    virtual void  addPcoPosId(int);                                  // +0x1BC

    virtual void  setNameTagOffset(Vec3);                            // +0x1C0
    virtual const Vec3& getNameTagOffset();                          // +0x1C4
    virtual void  setRidingWithOffset(Vec3);                         // +0x1C8
    virtual const Vec3& getRidingWithOffset();                       // +0x1CC
    virtual void  setRidingWithDistMod(float);                       // +0x1D0
    virtual float getRidingWithDistMod();                            // +0x1D4

    virtual void  setVehicleCameraShake(CameraShakeTriggers);        // +0x1D8
    virtual int   getVehicleCameraShake();                           // +0x1DC
    virtual void  setVehicleFov(float);                              // +0x1E0
    virtual float getVehicleFov();                                   // +0x1E4

    virtual void  setSeatLeftRotationLimit(float);                   // +0x1E8
    virtual float getSeatLeftRotationLimit() const;                  // +0x1EC
    virtual void  setSeatRightRotationLimit(float);                  // +0x1F0
    virtual float getSeatRightRotationLimit() const;                 // +0x1F4

    // Resets and fills the parent-seat string, transform, position, and rotation.
    virtual void  seatInformation(const std::string&, const Vec3&,
                                  const Vec3&);                      // +0x1F8

    virtual void  setSeatAnimationSystem(const std::string&);        // +0x1FC
    virtual const std::string& getSeatAnimationSystem();             // +0x200
    virtual void  setSeatParent(const std::string&);                 // +0x204
    virtual const std::string& getSeatParent() const;                // +0x208
    virtual void  setSeatPosition(const Vec3&);                      // +0x20C
    virtual const Vec3& getSeatPosition() const;                     // +0x210
    // / 135 B — an order of magnitude above the position pair. See the header block.
    virtual void  setSeatRotation(const Vec3&);                      // +0x214
    virtual const Vec3& getSeatRotation() const;                     // +0x218
    virtual const Mat4& getSeatTransformation() const;               // +0x21C

    virtual const std::string& getSoundFilter() const; // Returns the stored filter string.
    virtual void  setSoundFilter(const std::string&);                // +0x224
    virtual float getListenerObstruction() const;                    // +0x228
    virtual void  setListenerObstruction(float);                     // +0x22C

    virtual void  setMaintainCameraOnExit(bool);                     // +0x230
    virtual bool  getMaintainCameraOnExit();                         // +0x234
    virtual void  setMaintainCameraOnEnter(bool);                    // +0x238
    virtual bool  getMaintainCameraOnEnter();                        // +0x23C
    virtual void  setNeverDrawAs1p(bool);                            // +0x240
    virtual bool  getNeverDrawAs1p() const; // +0x244

protected:
    // layout, offsets 380..775. BundleTemplate occupies 0..379.
    // +380 is the real IPlayerControlObjectTemplate secondary base vptr (PE +0x22C); C++ supplies it.
    /* 384 */ int   m_refCount3;

    /* 388 */ SeatTemplateInformation m_seatParent;     // 388..467

    // entry / exit
    /* 468 */ Math::Vec3 m_soldierExitPosition;         // 468,472,476
    /* 480 */ Math::Vec3 m_soldierExitRotation;         // 480,484,488

    // Offsets exposed by the runtime vehicle as const references.
    /* 492 */ Math::Vec3 m_nameTagOffset;               // 492,496,500
    /* 504 */ Math::Vec3 m_ridingWithOffset;            // 504,508,512
    // default 1.0f, from the ctor (0x819E8A0) which writes 0x3F800000 here. Four fields in this
    // class default to 1.0f - see the block comment at m_regWhenMinInput below.
    /* 516 */ float m_ridingWithDistMod;                // = 1.0f
    /* 520 */ float m_damageForBeingOutSideWorld; // Defaults to 1000000.0f damage.

    /* 524 */ bool  m_dontClearTeamOnExit;
    /* 525 */ bool  m_destroyOnExit;
    /* 526 */ bool  m_dontAllowExit;
    /* 527 */ bool  m_hasRestrictedExit;

    /* 528 */ Math::Vec3 m_altSoldierExitPosition;      // 528,532,536
    // Regulation multiplier defaults to 1.0f.
    /* 540 */ float m_regWhenMinInput;                  // was `_dword21C`;  = 1.0f

    // Input regulation for yaw, pitch, roll and vertical position. Each range stores two components.
    /* 544 */ int   m_regulateYawInput;
    /* 548 */ int   m_regulatePitchInput;
    /* 552 */ int   m_regulateRollInput;
    /* 556 */ int   m_regulateVerticalPosInput;
    /* 560 */ Math::Vec2 m_regulateYaw;                 // 560,564
    /* 568 */ Math::Vec2 m_regulatePitch;               // 568,572
    /* 576 */ Math::Vec2 m_regulateRoll;                // 576,580
    /* 584 */ Math::Vec2 m_regulateVerticalPos;         // 584,588
    /* 592 */ float m_maxVertRegAngle;
    /* 596 */ float m_noVertRegAngle;

    // identity and categorisation
    /* 600 */ int   m_pcoId;
    /* 604 */ unsigned int m_pcoFlags;
    /* 608 */ int   _dword260;                          // [UNRESOLVED] 608..623
    /* 612 */ int   _dword264;
    /* 616 */ std::string _string268; // [UNRESOLVED] Field name.
    /* 620 */ int   _dword26C;
    /* 624 */ int   m_vehicleCategory;
    /* 628 */ int   m_controlsCategory;
    /* 632 */ int   m_vehicleType;
    /* 636 */ int   m_toolTipType;

    /* 640 */ bool  m_artPos;
    /* 641 */ bool  _byte281;                           // [UNRESOLVED]
    /* 642 */ bool  _byte282;
    /* 643 */ bool  _byte283;

    // addPcoPosId's ELF body is an inline std::vector<int>::push_back (0x8181850), and both
    // serializers re-read begin/end on every indexed iteration.
    /* 644 */ std::vector<int> m_pcoPosIds;

    // direction bar (the vehicle HUD compass)
    /* 656 */ float m_dirBarXScale;
    /* 660 */ float m_dirBarYScaleAbove;
    /* 664 */ float m_dirBarYScaleBelow;
    /* 668 */ float m_dirBarRotate;
    /* 672 */ float m_dirBarYScaleMax;
    /* 676 */ float m_dirBarYScaleMin;

    /* 680 */ float m_positionalSpeedModifier;          // = 1.0f;  .con property: `exitSpeedMod`
    /* 684 */ bool  m_disableSpawnPointsOnEnter;
    /* 685 */ bool  _byte2AD;                           // [UNRESOLVED]
    /* 686 */ bool  _byte2AE;
    /* 687 */ bool  _byte2AF;

    /* 688 */ int   m_vehicleCameraShake;
    /* 692 */ float m_vehicleFoV;

    /* 696 */ bool  m_toggleWeapon;
    /* 697 */ bool  _byte2B9;                           // [UNRESOLVED]
    /* 698 */ bool  _byte2BA;
    /* 699 */ bool  _byte2BB;

    // PE makeScript selects ostream's unsigned-int overload for both fields.
    /* 700 */ unsigned int m_cockpitLod;
    /* 704 */ unsigned int m_cockpitSubGeom;
    /* 708 */ std::string m_soundFilter; // Sound filter.
    // Serialized target types are unique.
    /* 712 */ std::set<int> m_targetObjectTypesToWatch;

    /* 724 */ float m_shakeFactor;                      // = 1.0f

    // sprint
    /* 728 */ float m_sprintRecoverTime;
    /* 732 */ float m_sprintDissipationTime;
    /* 736 */ float m_sprintLimit;
    /* 740 */ float m_sprintFactor;

    /* 744 */ bool  m_allowDucking;
    /* 745 */ bool  m_disableInputWhileDucking;
    /* 746 */ bool  m_autoUseAbility;
    /* 747 */ char  _pad2EB[1];

    /* 748 */ int   m_specialToggleWeaponInput;
    /* 752 */ float m_listenerObstruction;
    /* 756 */ float m_groundContactVolume;
    /* 760 */ float m_damagedAmbientSoundLimit;
    /* 764 */ int   m_crewKitIndex;

    /* 768 */ bool  m_maintainCameraOnExit;
    /* 769 */ bool  m_maintainCameraOnEnter;
    /* 770 */ bool  m_neverDrawAs1p;
    /* 771 */ bool  m_isOpenVehicle;                    // was `_byte303` [UNRESOLVED]

    /* 772 */ float m_boundingRadiusModifierWhenOccupied;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_PLAYERCONTROLOBJECTTEMPLATE_H
