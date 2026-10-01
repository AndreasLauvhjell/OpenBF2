// Path:     [INFERRED] - Game/Objects/, beside IControlPoint.h and ISpawnPointTemplate.h.
// Contents: complete secondary interface recovered from the ControlPointTemplate PE and ELF
// vtables (69 slots: the IBase quartet plus 65 authored-control-point operations).
// Both shipped tables use this order. In particular, the two supply-value getters precede the
// signal/radius/group pairs in the secondary table even though the concrete primary table declares
// them later. Keeping this as a real base lets the compiler build the +0x17C adjusted thunks and
// makes queryInterface(IID_IControlPointTemplate) callable rather than a raw placeholder vptr.

#ifndef GAME_OBJECTS_ICONTROLPOINTTEMPLATE_H
#define GAME_OBJECTS_ICONTROLPOINTTEMPLATE_H

#include <list>
#include <string>

#include "../../Core/IBase.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace world {

class IControlPointTemplate : public IBase
{
public:
    virtual void  setStartTeam(int) = 0;                                      // +0x10
    virtual int   getStartTeam() const = 0;                                   // +0x14
    virtual void  setRadius(float) = 0;                                       // +0x18
    virtual float getRadius() const = 0;                                      // +0x1C
    virtual void  setControlPointName(std::string) = 0;                       // +0x20
    virtual const std::string& getControlPointName() = 0;                     // +0x24
    virtual void  setControlPointId(int) = 0;                                 // +0x28
    virtual int   getControlPointId() = 0;                                    // +0x2C
    virtual void  setDisableIfEnemyInsideRadius(bool) = 0;                    // +0x30
    virtual bool  getDisableIfEnemyInsideRadius() = 0;                        // +0x34
    virtual void  setDisableWhenLosingControl(bool) = 0;                      // +0x38
    virtual bool  getDisableWhenLosingControl() = 0;                          // +0x3C
    virtual void  setLoseControlWhenEnemyClose(bool) = 0;                     // +0x40
    virtual bool  getLoseControlWhenEnemyClose() = 0;                         // +0x44
    virtual void  setLoseControlWhenNotClose(bool) = 0;                       // +0x48
    virtual bool  getLoseControlWhenNotClose() = 0;                           // +0x4C
    virtual void  setTimeToGetControl(float) = 0;                             // +0x50
    virtual float getTimeToGetControl() = 0;                                  // +0x54
    virtual void  setTimeToLoseControl(float) = 0;                            // +0x58
    virtual float getTimeToLoseControl() = 0;                                 // +0x5C
    virtual void  setUnableToChangeTeam(bool) = 0;                            // +0x60
    virtual bool  getUnableToChangeTeam() const = 0;                          // +0x64
    virtual void  setOnlyTakeableByTeam(unsigned int) = 0;                    // +0x68
    virtual unsigned int getOnlyTakeableByTeam() = 0;                         // +0x6C
    virtual void  setMinNrToTakeControl(unsigned int) = 0;                    // +0x70
    virtual unsigned int getMinNrToTakeControl() = 0;                         // +0x74
    virtual void  setIsUnstrategicControlPoint(bool) = 0;                     // +0x78
    virtual bool  getIsUnstrategicControlPoint() = 0;                         // +0x7C
    virtual void  setAreaValueTeam1(unsigned int) = 0;                        // +0x80
    virtual unsigned int getAreaValueTeam1() = 0;                             // +0x84
    virtual void  setAreaValueTeam2(unsigned int) = 0;                        // +0x88
    virtual unsigned int getAreaValueTeam2() = 0;                             // +0x8C
    virtual void  setEnemyTicketLossWhenCaptured(int) = 0;                    // +0x90
    virtual int   getEnemyTicketLossWhenCaptured() = 0;                       // +0x94
    virtual void  setShowOnMinimap(bool) = 0;                                 // +0x98
    virtual bool  getShowOnMinimap() = 0;                                     // +0x9C
    virtual int   getSupplyValueNeeded(int team) = 0;                         // +0xA0
    virtual int   getSupplyValue(int team) = 0;                               // +0xA4
    virtual void  setSignalTriggerId(int) = 0;                                // +0xA8
    virtual int   getSignalTriggerId() const = 0;                             // +0xAC
    virtual void  setRadiusOffset(const Vec3&) = 0;                           // +0xB0
    virtual Vec3  getRadiusOffset() = 0;                                      // +0xB4
    virtual void  setGroupId(int) = 0;                                        // +0xB8
    virtual int   getGroupId() = 0;                                           // +0xBC
    virtual void  addSupplyGroupNeeded(int, int, bool) = 0;                   // +0xC0
    virtual std::list<int> getSupplyGroupNeeded(int) = 0;                     // +0xC4
    virtual void  removeSupplyGroupNeeded(int, int) = 0;                      // +0xC8
    virtual void  setHoistFlag(bool) = 0;                                     // +0xCC
    virtual bool  getHoistFlag() = 0;                                         // +0xD0
    virtual void  setFlagTemplateTeam0(std::string) = 0;                      // +0xD4
    virtual const std::string& getFlagTemplateTeam0() = 0;                    // +0xD8
    virtual void  setFlagTemplateTeam1(std::string) = 0;                      // +0xDC
    virtual const std::string& getFlagTemplateTeam1() = 0;                    // +0xE0
    virtual void  setFlagTemplateTeam2(std::string) = 0;                      // +0xE4
    virtual const std::string& getFlagTemplateTeam2() = 0;                    // +0xE8
    virtual void  setHoistMinMax(Vec2) = 0;                                   // +0xEC
    virtual Vec2  getHoistMinMax() = 0;                                       // +0xF0
    virtual void  setIsHemisphere(bool) = 0;                                  // +0xF4
    virtual bool  getIsHemisphere() = 0;                                      // +0xF8
    virtual void  setForSoldierOnly(bool) = 0;                                // +0xFC
    virtual bool  getForSoldierOnly() = 0;                                    // +0x100
    virtual void  setHasGreyCapturingState(bool) = 0;                         // +0x104
    virtual bool  getHasGreyCapturingState() = 0;                             // +0x108
    virtual void  setMinimapNameOffset(Vec2) = 0;                             // +0x10C
    virtual Vec2  getMinimapNameOffset() = 0;                                 // +0x110

protected:
    ~IControlPointTemplate() {}
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_ICONTROLPOINTTEMPLATE_H
