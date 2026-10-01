// One team's commander election, stored by value in Team. Copies retain applicant state
// and a weak camera handle. Teardown destroys the camera through the game before releasing that handle.
// A commander ID of -1 means no commander.

#ifndef GAME_GAMELOGIC_SQUADMANAGER_COMMANDERMANAGER_H   // [INFERRED] guard style
#define GAME_GAMELOGIC_SQUADMANAGER_COMMANDERMANAGER_H

#include "../../../Core/WeakPtr.h"

#include <set>

namespace dice {
namespace hfe {

class Team;
namespace world { class IObject; }

class CommanderManager
{
public:
    CommanderManager();
    CommanderManager(const CommanderManager& other);
    virtual ~CommanderManager();

    int  getCommander() const;
    int  getProposedCommander() const;

    // The election queue, timing, both commander-change arms and camera lifetime are recovered.
    // Every method takes the owning Team because CommanderManager has no back-pointer of its own.
    void reset();

    // TWO setCommander overloads. The short form forwards its player id as argument three
    // while planting -1 in the long form's dead first argument. Both are
    bool setCommander(int playerId, Team* team);
    bool setCommander(int unused, Team* team, int playerId);
    void applyForCommander(int playerId, Team* team);
    int  popNextCommanderApplicant();
    // returns bool (was `void`): Team::removeCommanderApplicant branches on it - `test al` at
    // and the body materialises `find() != end()` before the erase.
    bool removeCommanderApplicant(int playerId);
    void acceptCommander(int playerId, Team* team);
    void declineCommander(int playerId, Team* team);
    void resignCommander(Team* team);
    void spawnCommanderCam(int teamId);
    world::IObject* getCommanderCam(Team* team);

public:
    /* +0x04 */ int           m_commander;            // = -1  "none", not 0
    /* +0x08 */ int           m_proposedCommander;    // = -1
    /* +0x0C */ std::set<int> m_applicants;           // retail: 12 bytes, +0x0C..+0x17
    /* +0x18 */ WeakPtr<world::IObject> m_commanderCam; // default-null; one-word weak handle
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMELOGIC_SQUADMANAGER_COMMANDERMANAGER_H
