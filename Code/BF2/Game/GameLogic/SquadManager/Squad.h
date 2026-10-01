// Stores squad membership and optional squad, commander and request orders.
// The Team back-pointer is borrowed. Request types clamp to the range 0..3.
// [PLATFORM] Embedded STL containers make the layout compiler-dependent.

#ifndef GAME_GAMELOGIC_SQUADMANAGER_SQUAD_H          // [INFERRED] guard style
#define GAME_GAMELOGIC_SQUADMANAGER_SQUAD_H

#include <set>
#include <string>

#include "ISquadInfo.h"
#include "SquadOrder.h"
#include "../../../Math/Vec3.h"

namespace dice {
namespace hfe {

class Team;
namespace world { class IPlayer; }

// SquadOrder is shared through SquadOrder.h. Its order ID is an int in memory
// but a byte on the wire. Construction initializes only its point vectors;
// order metadata and acceptance remain untouched until assigned.

class Squad : public ISquadInfo
{
public:
    explicit Squad(Team* team);
    virtual ~Squad();

    // the vtable, read from 0x8891BA0. ISquadInfo is the sole offset-zero base and has NO
    // IBase quartet; Squad's destructor pair follows its ten inherited slots at +0x28/+0x2C.
    virtual const std::string& getSquadName();                   // +0x00
    virtual bool  getIsPrivate();                                // +0x04
    virtual bool  getIsAccepted(SquadOrderType, OrderType);      // +0x08
    virtual int   getSquadId();                                  // +0x0C
    virtual SquadOrder* getSquadOrders();                        // +0x10
    virtual SquadOrder* getCommanderOrders();                    // +0x14
    virtual SquadOrder* getRequest(SquadOrderType);              // +0x18
    virtual int   getSpawnGroupId();                             // +0x1C
    virtual void  addSquadLeaderSpawnPoint(world::IPlayer*);     // +0x20
    virtual void  removeSquadLeaderSpawnPoint(world::IPlayer*);  // +0x24

    // NON-virtual
    bool setSquadName(const std::string& name);
    bool setSquadLeader(int playerId);

    void reset(bool full);
    void setIsAccepted(bool, OrderType, SquadOrderType);

    bool addToSquad(int playerId);
    bool removeFromSquad(int playerId, bool, bool);
    bool setSquadLeader(int, int);
    void issueSquadOrder(SquadOrder&);
    void issueCommanderOrder(SquadOrder&);
    void issueRequest(SquadOrder&);

public:
    /* +0x004 */ std::string   m_squadName;
    /* +0x008 */ int           m_squadId;              // = 0
    /* +0x00C */ int           m_leaderPlayerId;       // = -1  "no leader", not 0
    /* +0x010 */ bool          m_isPrivate;            // = 0
    /* +0x011 */ bool          m_hasSquadOrders;       // = 0   getSquadOrders' gate
    /* +0x012 */ bool          m_hasCommanderOrders;   // = 0   getCommanderOrders' gate
    /* +0x013 */ bool          m_hasRequest[4];        // = 0   getRequest's gate, per type
    /* +0x018 */ SquadOrder    m_squadOrders;
    /* +0x048 */ SquadOrder    m_requests[4];
    /* +0x108 */ SquadOrder    m_commanderOrders;
    /* +0x138 */ int           m_spawnGroupId;         // = -1
    /* +0x13C */ Team*         m_team;                 // back-pointer, NOT owned
    /* +0x140 */ std::set<int> m_members;              // retail: 12 bytes
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMELOGIC_SQUADMANAGER_SQUAD_H
