// Player registry, team selection and active/inactive player lists.
// [INFERRED] Path from ELF link order beside ObjectManager, not from __FILE__.
// List and ID-map updates must retain the shipped ordering.

#ifndef SCENE_OBJECT_PLAYERMANAGER_H     // [INFERRED] guard style
#define SCENE_OBJECT_PLAYERMANAGER_H

#include <list>
#include <map>
#include <string>
#include <utility>

#include "../../Core/Hash.h"    // Hash<K,V,HashFunc>, StringHashNoCaseHashFunc
#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace world {

// Player pointers need only a forward declaration here; implementation uses the complete interface.
class IPlayer;

// The vtable, in slot order, 48 slots / 192 bytes.
class PlayerManager /* : public IPlayerManager */
{
public:
    // IBase quartet, +0x00..+0x0C
    virtual void  addRef();                                                 // +0x00
    virtual int   getRef();                                                 // +0x04
    virtual int   release();                                                // +0x08
    virtual void* queryInterface(unsigned int iid) const;                   // +0x0C

    // lifecycle
    virtual void  addPlayer(IPlayer* player);                               // +0x10
    virtual void  removePlayer(IPlayer* player);                            // +0x14  0x83004F0, 643 B  written
    virtual void  removePlayerId(int id);                                   // +0x18  0x8300780, 129 B  written

    // lookup
    virtual IPlayer* getPlayerFromName(const std::string& name);            // +0x1C
    virtual IPlayer* getPlayerFromId(int id);                               // +0x20
    virtual IPlayer* getNextPlayerFromId(int id);                           // +0x24
    virtual IPlayer* getPlayerFromProfileId(int profileId);                 // +0x28  0x8302870, 101 B  written
    virtual IPlayer* getPlayerFromIdString(const std::string& id);          // +0x2C

    // BF2Engine::mainLoop calls this slot. 11 bytes - a plain load of +0x48, no null check, no
    // side effect. It is the only per-frame use of this class in the client loop.
    virtual IPlayer* getLocalHumanPlayer();                                 // +0x30
    virtual void  setLocalHumanPlayer(IPlayer* player);                     // +0x34  0x8301870, 42 B written
    virtual void  setLocalHumanPlayer(int id);                              // +0x38

    // The player lists
    // BF2Engine::mainLoop iterates the result of this slot. 11 bytes - `lea eax,[this+0Ch]`, a
    // reference to the live registry, not a copy. Adding or removing a player during that loop
    // invalidates it; nothing in this class defers such mutations the way ObjectManager's
    // m_inFrameUpdate latch does.
    virtual const std::list<IPlayer*>& getPlayers();                        // +0x3C
    virtual const std::list<IPlayer*>& getPlayersSortedByScore();           // +0x40  0x8301460, 182 B written
    virtual const std::list<IPlayer*>& getPlayersSortedByRank();            // +0x44  0x8301520, 182 B written
    virtual const std::list<IPlayer*>& getPlayers(int team);                // +0x48
    virtual const std::list<IPlayer*>& getPlayersSortedByScore(int team);   // +0x4C  0x83016C0, 198 B written
    virtual const std::list<IPlayer*>& getPlayersSortedByRank(int team);    // +0x50  0x8301790, 198 B written
    virtual const std::list<IPlayer*>& getPlayersInSquad(int team,
                                                         int squad);        // +0x54  0x8300810, 261 B written
    virtual const std::list<IPlayer*>& getPlayersInSquadSortedByScore(
                                            int team, int squad);           // +0x58  0x8300BE0, 204 B written
    virtual const std::list<IPlayer*>& getPlayersInSquadSortedByRank(
                                            int team, int squad);           // +0x5C  0x8300CB0, 204 B written
    virtual const std::list<IPlayer*>& getTeamSquadLeaders(int team);       // +0x60  0x8300920, 236 B written

    // The tick
    virtual void  updatePlayers(float deltaTime);                           // +0x64

    // Player classes
    // All three fully and implemented in PlayerManager.cpp. registerPlayerClass
    // is the one Core/ModuleRegistry.cpp calls (ModuleGameLogic::init); it was reached by the boot
    // harness, which is how the invented "registerTypeName" name was finally disproved.
    virtual void  registerPlayerClass(unsigned int classId,
                                      const std::string& className);        // +0x68
    virtual void  getPlayerClasses(
              std::list<std::pair<unsigned int, std::string> >& out) const; // +0x6C

    // Returns the stored class-name string by reference, or a shared fallback on a miss.
    // Keep the reference return for ABI compatibility and map-entry lifetime semantics.
    virtual const std::string& getClassNameFromPlayerClassID(
                                      unsigned int classId) const;          // +0x70

    // Creation and persistence
    virtual IPlayer* createPlayer(unsigned int classId,
                                  const std::string& name, int id);         // +0x74
    virtual IPlayer* createPlayer(const std::string& className,
                                  const std::string& name, int id);         // +0x78
    virtual bool  saveAllPlayers(const std::string& fileName);              // +0x7C

    // The active player
    virtual IPlayer* getActivePlayer() const;                               // +0x80
    virtual void  setActivePlayer(IPlayer* player);                         // +0x84

    // Counts
    virtual int   getNumberOfPlayers() const;                               // +0x88
    virtual int   getNumberOfHumanPlayers() const;                          // +0x8C  0x83027B0, 82 B  written
    virtual int   getNumberOfAliveHumanPlayers() const;                     // +0x90
    virtual int   getNrOfPlayerInTeam(int team);                            // +0x94
    virtual int   getNrOfPlayerInSquads(int team);                          // +0x98  0x8301FD0, 104 B written
    virtual int   getNrOfPlayersInSquad(int team, int squad);               // +0x9C  0x8300A10, 140 B written

    // dice's spelling: getNrOfPlayerInTeam and getNrOfPlayerInSquads are singular "Player" while
    // getNrOfPlayersInSquad is plural. All three are frozen exactly as mangled.
    virtual const std::list<IPlayer*>& getInvitedPlayers(int team,
                                                         int squad);        // +0xA0  0x8300AA0, 239 B written
    virtual int   getNrOfActiveSquads(int team);                            // +0xA4  0x8300B90, 77 B written

    // Team assignment
    // Relative desired team weights [INFERRED names]; includes the shipped coop branch.
    // The confirmed addBot caller passes 1,1; a human-autobalance caller is not established.
    virtual int   getTeamToAddPlayer(float team1Weight, float team2Weight); // +0xA8

    // Deactivation
    // The +0x3C map is the parking lot these two move players between - see the layout block.
    virtual bool  deactivatePlayer(int id);                                 // +0xAC
    virtual bool  reactivatePlayer(int id);                                 // +0xB0

    // the std::string is by value. `Ss`, not `RKSs`. Frozen.
    virtual bool  renamePlayer(IPlayer* player, std::string name);          // +0xB4


    virtual ~PlayerManager();                                               // +0xB8 / +0xBC

public:
    // C1 0x82FFB70 (391 B) / C2 0x82FF930 (565 B). recovered - see the .cpp, and read its warning
    // about the CPersistentExplosionEffectManager side effect before touching it.
    PlayerManager();

    // ClassManager factory registered by ModuleBF2. The singleton row belongs to world::Module.
    static IBase* create(unsigned int iid, IBase* outer);

private:
    // Non-virtual, by-value signatures are attested by the ELF mangled names. Both helpers refill
    // the same +0x14 output scratch list; +0x10 is the shared filter scratch list.
    const std::list<IPlayer*>& sortByRank(std::list<IPlayer*> players);
    const std::list<IPlayer*>& sortByScore(std::list<IPlayer*> players);

    /* +0x04 */ int      m_refCount;
    /* +0x08 */ IPlayer* m_activePlayer;

    /* +0x0C */ std::list<IPlayer*> m_players;

    // [INFERRED names] +0x10 / +0x14. The first is refilled by every team/squad/invite filter; the
    // second is refilled by both score and rank sorting. Both are returned by reference, so a
    // caller's filtered reference dies on the next filter and its sorted reference on the next
    // sort. Same pattern as WindManager::m_affectorsInRange.
    /* +0x10 */ std::list<IPlayer*> m_playersByScore;
    /* +0x14 */ std::list<IPlayer*> m_playersByRank;

    /* +0x18 */ Hash<std::string, IPlayer*, StringHashNoCaseHashFunc> m_playerNames;

    /* +0x24 */ std::map<unsigned int, std::string> m_playerClasses;

    // [INFERRED names] +0x30 / +0x3C. Both are std::map<int, IPlayer*> - the type is attested from
    // the single _Rb_tree<i, pair<Ki, world::IPlayer*>, ...>::_M_erase instantiation the destructor
    // calls for both of them.
    /* +0x30 */ std::map<int, IPlayer*> m_playersById;
    /* +0x3C */ std::map<int, IPlayer*> m_inactivePlayers;

    /* +0x48 */ IPlayer* m_localHumanPlayer;

    // [UNRESOLVED] +0x4C. Set to 0 by the constructor (0x82FFCCA) and read by nothing that has been
    // recovered. Modelled because the layout requires it; deliberately unnamed.
    /* +0x4C */ int m_unknown4C;
};

// Shared player-manager singleton. Keep every declaration typed identically for MSVC linkage.
extern PlayerManager* playerManager;

} // namespace world
} // namespace hfe
} // namespace dice

#endif // SCENE_OBJECT_PLAYERMANAGER_H
