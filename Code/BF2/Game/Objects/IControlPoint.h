// Capture-point interface for ownership, capture progress and team values.
// Preserve virtual declaration order. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_ICONTROLPOINT_H     // [INFERRED] guard style
#define GAME_OBJECTS_ICONTROLPOINT_H

#include <vector>

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace world {

struct SupplyGroup;                                  // ControlPointTemplate.h's element type

class IControlPoint : public IBase
{
public:
    virtual unsigned int getAreaValueTeam1() = 0;                            // +0x10
    virtual unsigned int getAreaValueTeam2() = 0;                            // +0x14
    virtual void  setTeam(unsigned int team) = 0;                            // +0x18
    virtual int   getTeam() = 0;                                             // +0x1C
    virtual void  setPlayer(int playerId) = 0;                               // +0x20
    virtual int   getPlayer() = 0;                                           // +0x24
    virtual void  reset() = 0;                                               // +0x28
    virtual bool  getAllowCaptureByTeam(int team) = 0;                       // +0x2C
    virtual void  setAllowCaptureByTeam(int team, bool allow) = 0;           // +0x30
    virtual float getTimeUntilTakeOver() = 0;                                // +0x34
    virtual bool  getShowOnMinimap() = 0;                                    // +0x38
    virtual void  setTakeOverChangePerSecond(float rate) = 0;                // +0x3C
    virtual float getTakeOverChangePerSecond() const = 0;                    // +0x40
    virtual bool  getSupplyGroupsNeeded(int team, std::vector<SupplyGroup>& out) = 0;   // +0x44 bool (read)
    virtual void  refreshFlags() = 0;                                        // +0x48
    virtual void  setFlag(int team, bool) = 0;                               // +0x4C
    virtual int   getFlag() const = 0;                                       // +0x50
    virtual void  setCapturingState(int state) = 0;                          // +0x54
    virtual int   getCapturingState() const = 0;                             // +0x58
    virtual float getTakeOverCounter() = 0;                                  // +0x5C

protected:
    ~IControlPoint() {}
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_ICONTROLPOINT_H
