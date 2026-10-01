// Client squad manager shared by both images. Mutating requests dispatch locally
// through SquadManager when the local flag is set; otherwise they send matching
// events to the server. Unoverridden queries use the base implementation.
#ifndef GAME_GAMELOGIC_SQUADMANAGER_CLIENTSQUADMANAGER_H     // [INFERRED] guard style
#define GAME_GAMELOGIC_SQUADMANAGER_CLIENTSQUADMANAGER_H

#include <string>

#include "SquadManager.h"
#include "../../../Core/IBase.h"

namespace dice {
namespace hfe {

class ClientSquadManager : public SquadManager
{
public:
    ClientSquadManager();
    virtual ~ClientSquadManager();

    // the IBase quartet, this class's own
    virtual void  addRef();                           // +0x00
    virtual int   getRef();                           // +0x04
    virtual int   release();                          // +0x08
    virtual void* queryInterface(unsigned int) const; // +0x0C

    static IBase* create(unsigned int iid, IBase* outer);   // the ClassManager factory

    // the overridden ISquadManager slots
    virtual bool setSquadName(int team, int squadId, int playerId, const std::string& name, bool local);  // +0x1C
    virtual void setIsPrivate(int team, int squadId, bool isPrivate, bool local);                        // +0x20
    virtual bool setIsAccepted(int team, int squadId, bool accepted, OrderType orderType,
                               SquadOrderType squadOrderType, bool local);                               // +0x24
    virtual void inviteToSquad(int team, int squadId, int playerId, bool invited, bool suppress);        // +0x2C
    virtual bool addToSquad(int team, int squadId, int playerId);                                        // +0x30
    virtual void removeFromSquad(int team, int squadId, int playerId, bool clearInvite);                 // +0x34
    virtual void setSquadLeader(int team, int squadId, int playerId, bool local);                        // +0x38
    virtual int  getFirstUnusedSquad(int team) const;                                                    // +0x40   - Debug(), -1
    virtual void issueSquadOrder(int team, int squadId, SquadOrder& order, bool local);                  // +0x44
    virtual void issueCommanderOrder(int team, int squadId, SquadOrder& order, bool local);              // +0x48
    virtual void issueRequest(int team, int squadId, SquadOrder& order, bool local);                     // +0x4C
    virtual void applyForCommander(int team, int playerId);                                              // +0x58
    virtual int  popNextCommanderApplicant(int team);                                                    // +0x5C   - Debug(), -1
    virtual void acceptCommander(int team, int playerId);                                                // +0x60
    virtual void declineCommander(int team, int playerId);                                               // +0x64
    virtual void resignCommander(int team);                                                              // +0x68
    virtual void setCommander(int team, int playerId);                                                   // +0x6C
    virtual int  getCommander(int team);                                                                 // +0x70
    virtual void setCommanderToSoldierMode(int playerId);                                                // +0x78
    virtual void setCommanderToOverheadCamMode(int playerId);                                            // +0x7C
    virtual void setCommanderOverheadCamZoomMode(int playerId, bool zoom);                               // +0x80

private:
    /* +0x28 */ int m_refCount;               // = 1
    /* +0x2C */ int m_commanderPlayerId[2];   // = -1, -1  indexed [team - 1]: the binary addresses
                                              //   `this + 4 * (10 + team)`, i.e. team 1 -> +0x2C,
                                              //   team 2 -> +0x30 (team 0 would alias the refcount)
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMELOGIC_SQUADMANAGER_CLIENTSQUADMANAGER_H
