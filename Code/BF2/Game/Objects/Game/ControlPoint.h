// Capturable world landmark with team ownership, flags and capture progress.
// Original ControlPoint.cpp also contained the template methods.

#ifndef GAME_OBJECTS_GAME_CONTROLPOINT_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_CONTROLPOINT_H

#include <vector>

#include "../Base/Bundle.h"
#include "../ControlPointTemplate.h"         // SupplyGroup, and the template this object reads
#include "../IControlPoint.h"                // the real second base

namespace dice {
namespace hfe {

class ControlPointNetworkable;      // Game/Objects/Networkables/ - the replication component (friend)

namespace world {

// The control-point and console interfaces are separate subobjects.
// Their offsets differ from the template hierarchy; use typed interface casts.
class ControlPoint : public Bundle, public IControlPoint     // IControlPoint = the +0x13C sub-object
{
public:
    explicit ControlPoint(const ControlPointTemplate* objectTemplate);   // read in full
    virtual ~ControlPoint();                                             // (D1) / 62 B (D0)

    // the quartet, refcount at +0x140
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int) const;// the four-case table above

    // THE capture gameplay - all written. Sizes are the ELF's.
    virtual void  init();
    virtual void  destroy(bool);
    virtual void  reset();                           // setTeam(start team), the
                                                     // counter / rate / neutral flag, setFlag, the
                                                     // pole height from the bounding box, the hoist
    virtual void  handleUpdate(float, unsigned int); // THE capture counter -
                                                     // server, status 1 only; counter += rate * dt;
                                                     // crossing 1.0 / 0.0 clamps, zeroes the rate
                                                     // and raises script event 25 (this, 1 / 0)
    virtual void  initNetworkableComp();

    // Capture state and ownership accessors.
    virtual void  setTeam(unsigned int);             // the HUD events to the
                                                     // players (64 = yours, 65 = theirs / lost) while
                                                     // playing, the team store, the nametag touch,
                                                     // and on the server: rate 0, every SpawnGroup
                                                     // and ObjectSpawner with this control point's
                                                     // id gets the team
    virtual int   getTeam();                         // +0x148 (int - the networkable
                                                     // sign-tests it before the wire)
    virtual void  setTakeOverChangePerSecond(float); // refuses a positive rate on
                                                     // a fully captured point and a negative one on
                                                     // an empty counter; stamps the world time
    virtual float getTakeOverChangePerSecond() const;// +0x14C
    virtual float getTimeUntilTakeOver();            // -1 with no rate; else
                                                     // (1 - counter - client extrapolation) / |rate|
    virtual float getTakeOverCounter();              // +0x154 (a float - `fld dword`)
    virtual void  setCapturingState(int);            // +0x16C
    virtual int   getCapturingState() const;         // +0x16C
    virtual void  setPlayer(int);                    // +0x178
    virtual int   getPlayer();                       // +0x178

    // The flag objects this control point owns and moves.
    virtual void  addFlags();
    virtual void  removeFlags();                     // removeChild + destroy(true)
    virtual void  refreshFlags();                    // removeFlags + addFlags
    virtual void  setFlag(int, bool);
    virtual int   getFlag() const;                   // +0x168 - the shown flag's team;
                                                     // the networkable ships it as a signed
                                                     // 3-bit int and feeds it back to setFlag
    virtual void  updateFlagPosition();

    // Queries the rest of the game asks a control point.
    virtual unsigned int getAreaValueTeam1();        // the template's areaValueTeam1 while
                                                     // the point is neutral (m_isNeutral), else 0
    virtual unsigned int getAreaValueTeam2();        // likewise areaValueTeam2
    virtual int   getControlerId();                  // the template's
                                                     // controlPointId (+0x190), raw read.
                                                     // dice's spelling, one 'l'
    virtual bool  getSupplyGroupsNeeded(int team, std::vector<SupplyGroup>& out);
                                                     // out = the template's list for team 1 / 2,
                                                     // true; any other team false, out untouched.
                                                     // the SupplyGroup element type is shared
                                                     // with ControlPointTemplate - the IDB names
                                                     // it in both containers
    virtual bool  getAllowCaptureByTeam(int);        // +0x16F+team, teams 1..2 only
    virtual void  setAllowCaptureByTeam(int, bool);  // same; other teams ignored
    virtual bool  getShowOnMinimap();                // the template's +0x1BC
    std::string   getName();                         // NON-virtual (not in either table),
                                                     // returns the template's name BY value (sret)

    // The replication component reads the capture fields RAW (init/updateStateMask/setNetUpdate on
    // +0x14C/+0x154/+0x158/+0x168/+0x16C/+0x170/+0x171) - the SpawnGroup/SpawnManager arrangement.
    friend class ::dice::hfe::ControlPointNetworkable;

protected:
    /* 0x140 */ int   m_refCount;                 // ctor 1
    /* 0x144 */ const ControlPointTemplate* m_template;   // ctor = the constructor argument

    // SET from THE template, then conditionally undone - see the ctor's server/client fork.
    /* 0x148 */ int   m_team;                     // ctor = m_template->startTeam (+0x188)

    /* 0x14C */ float m_takeOverChangePerSecond;  // ctor 0, reset() re-zeroes it; the setter
                                                  // (100 B) has two refusals - see the .cpp
    /* 0x150 */ float m_takeOverChangeTime;       // [CHOSEN name] ctor 0 - the world time the rate
                                                  // Client capture extrapolation uses this update time.
    /* 0x154 */ float m_takeOverCounter;          // ctor 0. THE capture progress - and the flag's
                                                  // hoist: updateFlagPosition clamps it to 0..1
                                                  // and lerps the flag's height between the
                                                  // template's hoistMin and hoistMax. reset():
                                                  // 1.0f if the point starts owned, 0.0f if
                                                  // neutral. (Was m_flagHoistFraction here.)
    /* 0x158 */ bool  m_isNeutral;                // [CHOSEN name] ctor false; reset() = (team == 0)
                                                  // Used by area-value queries and replicated as state bit 1.
    /* 0x159 */ char  _pad159[3];
    // Spawned flag per team, with slot 0 neutral; unresolved templates leave null entries.
    /* 0x15C */ IObject* m_flagObjects[3];
    /* 0x168 */ int   m_currentFlagTeam;          // [CHOSEN] name; ctor 0; setFlag's early-out
                                                  // compares it, then stores the new team;
                                                  // getFlag() returns it
    /* 0x16C */ int   m_capturingState;           // get/setCapturingState's field. ctor skips it;
                                                  // setFlag also stores the flag team into
                                                  // it after the trigger fires (was named
                                                  // m_shownFlagTeam from that one writer)
    /* 0x170 */ bool  m_allowCaptureByTeam[2];    // [0] = team 1 (+0x170), [1] = team 2 (+0x171);
                                                  // ctor true, true. The accessors index
                                                  // `byte[0x16F + team]` and ignore any other
                                                  // team - as shipped
    /* 0x172 */ char  _pad172[2];
    /* 0x174 */ float m_flagPoleHeight;           // [CHOSEN] name; ctor 3.0f - the height scale
                                                  // Scales flag hoisting; reset replaces it with the bounds' max.y.
    /* 0x178 */ int   m_player;                   // get/setPlayer's field; ctor -1
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_CONTROLPOINT_H
