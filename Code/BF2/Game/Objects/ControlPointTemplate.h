// Authored capture-point settings, including per-team area values and supply-group lists.
// Original method placement is Game/Objects/Game/ControlPoint.cpp; current declarations remain here.

#ifndef GAME_OBJECTS_CONTROLPOINTTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_CONTROLPOINTTEMPLATE_H

#include <string>
#include <vector>
#include <list>

#include "Base/BundleTemplate.h"
#include "IControlPointTemplate.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace world {

// The element of ControlPointTemplate's per-team supply-group vectors. Its name is attested - the
// IDB types the containers as std::vector<dice::hfe::world::SupplyGroup> - and its shape is read
// from the code that walks them: 8-byte stride, the id compared as a dword at +0, the flag stored
// as the second word by addSupplyGroupNeeded's push-back arm.
// [UNRESOLVED] member names; only the types and offsets are recovered.
struct SupplyGroup
{
    int  group;      // +0x00 the id addSupplyGroupNeeded/removeSupplyGroupNeeded match on
    bool needed;     // +0x04 the third argument of addSupplyGroupNeeded
};

// Preserve the declared virtual order. Vector getters return values through the platform return-value ABI.
class ControlPointTemplate : public BundleTemplate, public IControlPointTemplate
{
public:
    ControlPointTemplate();                                          // read in full

    // the standard leaf five (BundleTemplate slots)
    virtual unsigned int getClassID() const;                         // +0x10
    virtual ~ControlPointTemplate();                                 // +0x14 +0x18
    virtual void* queryImplementation(unsigned int classId);         // +0x50
    virtual IObject* createObject() const;                           // +0x54
    virtual bool  makeScript(io::IStream*);                          // +0xD4

    // the quartet. queryInterface is NOT forwardable to the base here: the ELF body has SIX
    // cases where BundleTemplate's has five, the extra one being IID_IControlPointTemplate -> the
    // sub-object at +0x17C. Both bodies happen to be 84 bytes, so size is no guide. Written out.
    virtual void  addRef();                                          // +0x00
    virtual int   getRef();                                          // +0x04
    virtual int   release();                                         // +0x08
    virtual void* queryInterface(unsigned int) const;                // +0x0C

    virtual void  preCache() const;                                  // +0x6C
    // mapMaterial: five bytes. A bare `ret` - an empty override that deliberately suppresses the
    // base's material mapping, because this class fixes its own two materials in the constructor.
    virtual void  mapMaterial(unsigned int, const std::string&, unsigned int);   // +0x100

    // Scalar getter return types are [INFERRED] from their fields and console bindings.
    // Keep unsigned parameters, by-value strings and vector arguments as declared.
    virtual void  setStartTeam(int);                                 // +0x140
    virtual int   getStartTeam() const;                              // +0x144
    virtual void  setRadius(float);                                  // +0x148
    virtual float getRadius() const;                                 // +0x14C
    virtual void  setControlPointName(std::string);                  // +0x150
    virtual const std::string& getControlPointName();                // +0x154
    virtual void  setControlPointId(int);                            // +0x158
    virtual int   getControlPointId();                               // +0x15C

    virtual void  setDisableIfEnemyInsideRadius(bool);               // +0x160
    virtual bool  getDisableIfEnemyInsideRadius();                   // +0x164
    virtual void  setDisableWhenLosingControl(bool);                 // +0x168
    virtual bool  getDisableWhenLosingControl();                     // +0x16C
    virtual void  setLoseControlWhenEnemyClose(bool);                // +0x170
    virtual bool  getLoseControlWhenEnemyClose();                    // +0x174
    virtual void  setLoseControlWhenNotClose(bool);                  // +0x178
    virtual bool  getLoseControlWhenNotClose();                      // +0x17C

    virtual void  setTimeToGetControl(float);                        // +0x180
    virtual float getTimeToGetControl();                             // +0x184
    virtual void  setTimeToLoseControl(float);                       // +0x188
    virtual float getTimeToLoseControl();                            // +0x18C
    virtual void  setUnableToChangeTeam(bool);                       // +0x190
    virtual bool  getUnableToChangeTeam() const;                     // +0x194
    virtual void  setOnlyTakeableByTeam(unsigned int);               // +0x198
    virtual unsigned int getOnlyTakeableByTeam();                    // +0x19C
    virtual void  setMinNrToTakeControl(unsigned int);               // +0x1A0
    virtual unsigned int getMinNrToTakeControl();                    // +0x1A4
    virtual void  setIsUnstrategicControlPoint(bool);                // +0x1A8
    virtual bool  getIsUnstrategicControlPoint();                    // +0x1AC

    // PER-team — see the header block. Renamed from the [INFERRED] getAreaValue1/2.
    virtual void  setAreaValueTeam1(unsigned int);                   // +0x1B0
    virtual unsigned int getAreaValueTeam1();                        // +0x1B4
    virtual void  setAreaValueTeam2(unsigned int);                   // +0x1B8
    virtual unsigned int getAreaValueTeam2();                        // +0x1BC

    virtual void  setEnemyTicketLossWhenCaptured(int);               // +0x1C0
    virtual int   getEnemyTicketLossWhenCaptured();                  // +0x1C4
    virtual void  setShowOnMinimap(bool);                            // +0x1C8
    virtual bool  getShowOnMinimap();                                // +0x1CC
    virtual void  setSignalTriggerId(int);                           // +0x1D0
    virtual int   getSignalTriggerId() const;                        // +0x1D4
    virtual void  setRadiusOffset(const Vec3&);                      // +0x1D8
    virtual Vec3  getRadiusOffset();                                 // +0x1DC sret, BY value
    // Renamed from the [INFERRED] getSupplyGroupId.
    virtual void  setGroupId(int);                                   // +0x1E0
    virtual int   getGroupId();                                      // +0x1E4

    // Supply-group accessors require team 1 or 2 and index by team - 1.
    virtual int   getSupplyValueNeeded(int team);                    // +0x1E8
    virtual int   getSupplyValue(int team);                          // +0x1EC
    virtual void  addSupplyGroupNeeded(int team, int group, bool needed);   // +0x1F0
    virtual std::list<int> getSupplyGroupNeeded(int team);           // +0x1F4 sret
    virtual void  removeSupplyGroupNeeded(int team, int group);      // +0x1F8

    virtual void  setHoistFlag(bool);                                // +0x1FC
    virtual bool  getHoistFlag();                                    // +0x200
    virtual void  setFlagTemplateTeam0(std::string);                 // +0x204
    virtual const std::string& getFlagTemplateTeam0();               // +0x208
    virtual void  setFlagTemplateTeam1(std::string);                 // +0x20C
    virtual const std::string& getFlagTemplateTeam1();               // +0x210
    virtual void  setFlagTemplateTeam2(std::string);                 // +0x214
    virtual const std::string& getFlagTemplateTeam2();               // +0x218
    // A Vec2 pair, not two floats - the mangled name says NS0_4Vec2E by value, and the getter is
    // sret with the tell-tale `retn 4`.
    virtual void  setHoistMinMax(Vec2);                              // +0x21C
    virtual Vec2  getHoistMinMax();                                  // +0x220

    virtual void  setIsHemisphere(bool);                             // +0x224
    virtual bool  getIsHemisphere();                                 // +0x228
    virtual void  setForSoldierOnly(bool);                           // +0x22C
    virtual bool  getForSoldierOnly();                               // +0x230
    virtual void  setHasGreyCapturingState(bool);                    // +0x234
    virtual bool  getHasGreyCapturingState();                        // +0x238
    // Same Vec2 shape as the hoist pair.
    virtual void  setMinimapNameOffset(Vec2);                        // +0x23C
    virtual Vec2  getMinimapNameOffset();                            // +0x240

protected:
    // [CHOSEN] access mechanism: ControlPoint reads this template's fields RAW in the ELF
    // (startTeam +0x188 and unableToChangeTeam +0x1A0 in its ctor, the flag-template array +0x1F0,
    // hoistMinMax +0x1FC and signalTriggerId +0x204 in addFlags/setFlag/updateFlagPosition,
    // controlPointId +0x190 in getControlerId). dice kept both classes in ONE file - the attested
    // Game/Objects/Game/ControlPoint.cpp - and how their source granted the access is not
    // observable; a friend reproduces the reads without widening the public surface.
    friend class ControlPoint;

    // layout - 544 bytes total (0x220), every slot written by the constructor, every offset
    // corroborated by the accessor whose body touches it. BundleTemplate occupies 0x000..0x17B.
    // The IControlPointTemplate vptr at +0x17C is compiler-generated by the real secondary base.
    /* 0x180 */ int   m_refCount;                    // ctor 1
    /* 0x184 */ float radius;                        // ctor 1.0f
    /* 0x188 */ int   startTeam;                     // ctor 0
    /* 0x18C */ std::string controlPointName;        // ctor empty
    /* 0x190 */ int   controlPointId;                // ctor -1
    /* 0x194 */ float timeToGetControl;              // ctor 10.0f
    /* 0x198 */ float timeToLoseControl;             // ctor 10.0f

    // Four adjacent bytes, all written with byte moves by the ctor.
    /* 0x19C */ bool  disableIfEnemyInsideRadius;    // ctor false
    /* 0x19D */ bool  disableWhenLosingControl;      // ctor false
    /* 0x19E */ bool  loseControlWhenEnemyClose;     // ctor true
    /* 0x19F */ bool  loseControlWhenNotClose;       // ctor false
    /* 0x1A0 */ bool  unableToChangeTeam;            // ctor false
    /* 0x1A1 */ char  _pad1A1[3];
    /* 0x1A4 */ unsigned int minNrToTakeControl;     // ctor 1
    /* 0x1A8 */ bool  isUnstrategicControlPoint;     // ctor false
    /* 0x1A9 */ bool  isHemisphere;                  // ctor false  - note it sits here, between two
                                                     // unrelated fields, though its accessors are
                                                     // at the far end of the vtable (+0x224/+0x228)
    /* 0x1AA */ char  _pad1AA[2];
    /* 0x1AC */ unsigned int onlyTakeableByTeam;     // ctor 0
    /* 0x1B0 */ unsigned int areaValueTeam1;         // ctor 0
    /* 0x1B4 */ unsigned int areaValueTeam2;         // ctor 0
    /* 0x1B8 */ int   enemyTicketLossWhenCaptured;   // ctor 0
    /* 0x1BC */ bool  showOnMinimap;                 // ctor true
    /* 0x1BD */ char  _pad1BD[3];
    /* 0x1C0 */ int   groupId;                       // ctor -1

    // the supply-group storage, all three arrays indexed [team - 1]
    // The ctor zeroes exactly these bytes: two 12-byte vector triples in a two-iteration loop, then
    // the four ints one at a time. That is the whole of 0x1C4..0x1EB.
    /* 0x1C4 */ std::vector<SupplyGroup> m_supplyGroupsNeeded[2];   // 0x1C4 and 0x1D0
public:
    /* 0x1DC */ int   m_supplyValueNeeded[2]; // Console properties access the per-team values directly.
    /* 0x1E4 */ int   m_supplyValue[2];              // ctor 0, 0. The Team1/Team2 console
                                                     // properties also access these RAW.
protected:

    /* 0x1EC */ bool  hoistFlag;                     // ctor true
    /* 0x1ED */ char  _pad1ED[3];
    // Three empty strings built by a three-iteration loop in the ctor. preCache walks them as an
    // array indexed 0..2, which is what settles them as one member rather than three.
    /* 0x1F0 */ std::string m_flagTemplateTeam[3];   // ctor "", "", ""
    /* 0x1FC */ Vec2  hoistMinMax;                   // ctor (0.2f, 0.9f)
    /* 0x204 */ int   signalTriggerId;               // ctor -1
    /* 0x208 */ Vec3  radiusOffset;                  // ctor (0, 0, 0)
    /* 0x214 */ bool  forSoldierOnly;                // ctor false
    /* 0x215 */ bool  hasGreyCapturingState;         // ctor false
    /* 0x216 */ char  _pad216[2];
    /* 0x218 */ Vec2  minimapNameOffset;             // ctor (0, 0)
};


} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_CONTROLPOINTTEMPLATE_H
