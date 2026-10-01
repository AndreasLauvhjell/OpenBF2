// One team's squad collection and commander election. Squad ids start at zero
// via a pre-incremented -1 seed. Squad pointers are stored in active and unused
// collections. Embedded STL containers make offsets toolchain-dependent; access
// members by name rather than assuming retail sizes.

#ifndef GAME_GAMELOGIC_SQUADMANAGER_TEAM_H           // [INFERRED] guard style
#define GAME_GAMELOGIC_SQUADMANAGER_TEAM_H

#include <list>
#include <map>
#include <string>

#include "CommanderManager.h"

namespace dice {
namespace hfe {

class Squad;                                         // pointer-only here - see above

class Team
{
public:
    // [INFERRED] Constructor spelling for the original inline scalar initialization.
    Team() : m_nextSquadId(-1), m_teamId(0) {}
    explicit Team(int teamId) : m_nextSquadId(-1), m_teamId(teamId) {}

    int getTeamId() const { return m_teamId; }       // `mov eax,[eax+30h]`

    int    createSquad(const std::string& name);
    Squad* getSquad(int squadId);
    int    getFirstUnusedSquad() const;
    void   reset();

    // NOT AN accessor ON this class - 0x811AD00 is thirteen bytes of `add [esp+4], 14h` followed
    // by a tail jump into CommanderManager::getCommander. That +0x14 is the third independent
    // confirmation of where the CommanderManager sits; see the layout note above.
    int    getCommander() const { return m_commanderManager.getCommander(); }

    void removeSquad(int squadId);
    // delete every squad, clear both containers. The
    // ~SquadManager path (GameServer::shutdown's squadManager->release()).
    void removeAllSquads();

    // The two disconnect-path forwarders into the CommanderManager. read AND
    // SquadManager::removeCommanderApplicant / resignCommander are one-line forwarders into these.
    void removeCommanderApplicant(int playerId);
    void resignCommander();
    void applyForCommander(int playerId);
    int  popNextCommanderApplicant();
    bool setCommander(int playerId);
    world::IObject* getCommanderCam();
    void acceptCommander(int playerId);
    void declineCommander(int playerId);

public:
    /* +0x00 */ int                             m_nextSquadId;    // = -1; createSquad pre-increments
    /* +0x04 */ std::map<unsigned char, Squad*> m_squads;
    /* +0x10 */ std::list<Squad*>               m_unusedSquads;
    /* +0x14 */ CommanderManager                m_commanderManager;
    /* +0x30 */ int                             m_teamId;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMELOGIC_SQUADMANAGER_TEAM_H
