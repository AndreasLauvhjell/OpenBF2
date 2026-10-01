// Team-owned squads and commander control. [INFERRED] source filename.

#include "Team.h"

#include "Squad.h"
#include "../SpawnSystem/SpawnGroup.h"
#include "../SpawnSystem/SpawnManager.h"

#include <algorithm>

namespace dice {
namespace hfe {

extern IBase* spawnManager;                          // ds:0x8AD3A00 - the tree's IBase* form

// Creates an owned squad, assigns its next ID and completes its dynamic spawn-group registration.
// New squads enter the borrowed unused-squad list. IDs start at zero and narrow to byte map keys,
// so creating more than 256 squads can collide while still returning the new integer ID.
int Team::createSquad(const std::string& name)
{
    Squad* const squad = new Squad(this);               // operator new(0x14C)

    squad->setSquadName(name);                          // string::assign

    m_nextSquadId += 1;                                 // pre-increment from -1 -> first id is 0
    squad->m_squadId        = m_nextSquadId;
    squad->m_leaderPlayerId = -1;                       // already -1 from the constructor

    // the second half of the handshake - see the note above.
    if (squad->m_spawnGroupId >= 0)
    {
        SpawnManager* const spawn = static_cast<SpawnManager*>(spawnManager);
        ISpawnGroup* const group  = spawn->getSpawnGroup(squad->m_spawnGroupId);   // vt +0x28
        if (group)
            group->setSquad(squad->m_squadId);          // vt +0x20 - now with the real id
    }

    // the key is the LOW byte of the id - see the note above.
    m_squads.insert(std::make_pair((unsigned char)squad->m_squadId, squad));
    m_unusedSquads.insert(m_unusedSquads.end(), squad);

    return squad->m_squadId;
}

// Looks up a squad using the id's low byte, returning null when absent. Values
// outside 0–255 alias rather than failing range validation.
Squad* Team::getSquad(int squadId)
{
    std::map<unsigned char, Squad*>::iterator i =
        m_squads.find(static_cast<unsigned char>(squadId));    // the narrowing is here
    return (i == m_squads.end()) ? 0 : i->second;
}

// getFirstUnusedSquad - ELF 0x811AC60, 48 B. The list's first entry is returned without mutation;
// an empty list yields -1. The virtual getSquadId call is preserved through the concrete pointer.
int Team::getFirstUnusedSquad() const
{
    if (m_unusedSquads.empty())
        return -1;
    return m_unusedSquads.front()->getSquadId();
}

// removeSquad - ELF 0x811AB00, 337 B. Lookup narrows the id to the map's byte key, removes the
// same pointer's first free-list occurrence, invokes the deleting destructor when non-null, then
// erases the owning map node. A missing id is a silent no-op.
void Team::removeSquad(int squadId)
{
    std::map<unsigned char, Squad*>::iterator squadIt =
        m_squads.find(static_cast<unsigned char>(squadId));
    if (squadIt == m_squads.end())
        return;

    std::list<Squad*>::iterator unusedIt =
        std::find(m_unusedSquads.begin(), m_unusedSquads.end(), squadIt->second);
    if (unusedIt != m_unusedSquads.end())
        m_unusedSquads.erase(unusedIt);

    if (squadIt->second)
        delete squadIt->second;
    m_squads.erase(squadIt);
}

// Deletes owned squads before clearing the list of borrowed unused-squad pointers.
// Round reset separately stands down the commander before clearing election state,
// and preserves squad membership, leaders and privacy. [CHOSEN] reset returns void.
void Team::removeAllSquads()
{
    for (std::map<unsigned char, Squad*>::iterator i = m_squads.begin(); i != m_squads.end(); ++i)
    {
        if (i->second)                                      // the binary's null guard
            delete i->second;                               // vt +0x2C, the deleting destructor
    }
    m_squads.clear();                                       // +0x04
    m_unusedSquads.clear();                                 // +0x10
}

void Team::reset()
{
    m_commanderManager.setCommander(-1, this);              // the 2-arg overload
    m_commanderManager.reset();

    for (std::map<unsigned char, Squad*>::iterator i = m_squads.begin(); i != m_squads.end(); ++i)
    {
        if (i->second)                                      // the binary's null guard
            i->second->reset(false);                        // false: members survive
    }
}

// Removes an application; if absent, declines only a matching proposal on a team
// that has no commander. Preserve this test order.
void Team::removeCommanderApplicant(int playerId)
{
    if (!m_commanderManager.removeCommanderApplicant(playerId))
    {
        if (m_commanderManager.getCommander() == -1)                // `getCommander() + 1 == 0`
        {
            if (playerId == m_commanderManager.getProposedCommander())
                m_commanderManager.declineCommander(playerId, this);
        }
    }
}

// resignCommander - ELF 0x811AD90, 28 B. A forwarder that hands the manager its owning team.
void Team::resignCommander()
{
    m_commanderManager.resignCommander(this);
}

// The remaining Team/CommanderManager facade - ELF 0x811AC90..0x811AD8F. Every body passes this
// as the owning Team; acceptCommander's target is declared but remains the deeper election frontier.
void Team::applyForCommander(int playerId)
{
    m_commanderManager.applyForCommander(playerId, this);
}

int Team::popNextCommanderApplicant()
{
    return m_commanderManager.popNextCommanderApplicant();
}

bool Team::setCommander(int playerId)
{
    return m_commanderManager.setCommander(playerId, this);
}

world::IObject* Team::getCommanderCam()
{
    return m_commanderManager.getCommanderCam(this);
}

void Team::acceptCommander(int playerId)
{
    m_commanderManager.acceptCommander(playerId, this);
}

void Team::declineCommander(int playerId)
{
    m_commanderManager.declineCommander(playerId, this);
}

} // namespace hfe
} // namespace dice
