// Owns team membership, squad orders and commander-selection timing.
// [UNRESOLVED] m_field1C has no established role.
// [PLATFORM] The embedded map makes layout compiler-dependent; do not apply ELF sizes to MSVC.

#ifndef GAME_GAMELOGIC_SQUADMANAGER_SQUADMANAGER_H    // [INFERRED] guard style
#define GAME_GAMELOGIC_SQUADMANAGER_SQUADMANAGER_H

#include <map>
#include <string>

#include "ISquadManager.h"
#include "Team.h"

namespace dice {
namespace hfe {

namespace world { class OrderMarker; }


class Squad;                                         // the return of getSquad, below

class SquadManager : public ISquadManager
{
public:
    // THE factory - ELF 0x85312B0, 89 B. Registered as CID_SquadManager (50315) in
    // Core/ModuleRegistry.cpp.
    static IBase* create(unsigned int iid, IBase* outer);

    SquadManager();
    virtual ~SquadManager();

    // IBase, +0x00..+0x0C. All four
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    // ISquadManager, +0x10..+0x94.
    virtual void reset();
    virtual void reAddDynamicSpawnGroups();
    virtual void sendSquadManagerState(int);
    virtual bool setSquadName(int, int, int, const std::string&, bool);
    virtual void setIsPrivate(int, int, bool, bool);
    virtual bool setIsAccepted(int, int, bool, OrderType, SquadOrderType, bool);
    virtual ISquadInfo* getSquadInfo(int, int);
    virtual void inviteToSquad(int, int, int, bool, bool);
    virtual bool addToSquad(int, int, int);
    virtual void removeFromSquad(int, int, int, bool);
    virtual void setSquadLeader(int, int, int, bool);
    virtual int  getSquadLeader(int, int);
    virtual int  getFirstUnusedSquad(int) const;
    virtual void issueSquadOrder(int, int, SquadOrder&, bool);
    virtual void issueCommanderOrder(int, int, SquadOrder&, bool);
    virtual void issueRequest(int, int, SquadOrder&, bool);            //   (no-bot arm)
    virtual void resetSquad(int, int);
    virtual void findNewSquadLeader(int, int, int);
    virtual void applyForCommander(int, int);
    virtual int  popNextCommanderApplicant(int);
    virtual void acceptCommander(int, int);
    virtual void declineCommander(int, int);
    virtual void resignCommander(int);
    virtual void setCommander(int, int);
    virtual int  getCommander(int);
    virtual void* getCommanderCam(int);
    virtual void setCommanderToSoldierMode(int);
    virtual void setCommanderToOverheadCamMode(int);
    virtual void setCommanderOverheadCamZoomMode(int, bool);
    virtual void removeCommanderApplicant(int, int);
    virtual void addOrderMarker(int, SquadOrder&);
    virtual void removeAllOrderMarkers();
    virtual void removeOrderMarker(int);
    virtual void updateOrderMarkers(world::IPlayer*);

    // Creates a squad within a team; distinct from the static class factory.
    int   create(int team, const std::string& name);
    Team* getTeam(int team);
    const Team* getTeam(int team) const;
    void  removeAllSquads();
    void  remove(int, int);
    bool  addToSquad(int unused, int squadId, int playerId, int teamId);
    Squad* getSquad(int teamId, int squadId);
                                                                       // Squad*, NOT Team* -
    float getFirstCommanderSelectDelay() const;
    void  setFirstCommanderSelectDelay(float);
    float getCommanderSelectDelay() const;
    void  setCommanderSelectDelay(float);

public:
    /* +0x04 */ int                m_refCount;                 // = 1
    /* +0x08 */ std::map<int, Team> m_teams;                   // see the Team note above
    /* +0x14 */ float              m_firstCommanderSelectDelay;// = 15.0f
    /* +0x18 */ float              m_commanderSelectDelay;     // = 10.0f
    /* +0x1C */ float              m_field1C;                  // = 20.0f  [UNRESOLVED]
    /* +0x20 */ world::OrderMarker* m_orderMarker1;             // = 0; marker kind 1
    /* +0x24 */ world::OrderMarker* m_orderMarker2;             // = 0; marker kind 0
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMELOGIC_SQUADMANAGER_SQUADMANAGER_H
