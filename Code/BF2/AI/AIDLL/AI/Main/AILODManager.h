// Controls AI level of detail and update budgets.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_MAIN_AILODMANAGER_H      // [INFERRED] guard style
#define AI_AIDLL_AI_MAIN_AILODMANAGER_H

#include <map>
#include <set>

namespace dice {
namespace hfe {
namespace ai {

class Bot;

// Per-player LOD record. [CHOSEN, not recovered] Five words preserve its known 20-byte
// size; field meanings remain unknown.
class PlayerLODStats
{
public:
    int m_words[5];
};

class AILODManager
{
public:
    AILODManager(float basicLODDistance, float f36);
    ~AILODManager();                                   // , [FRONTIER]

    // The whole vtable. See the header note.
    virtual void reset();

    void setBasicLODDistance(float distance);
    void lodEnable(bool enable);
    bool lodEnable() const;

    // the seven that are not written
    // Each is a named stop; `updateAllPlayers` is reached from the constructor and the rest from
    // `reset` or from the console, so none of them runs on a path this tree exercises.
    void updateAllPlayers();                           // [FRONTIER]
    void updatePlayers();                              // [FRONTIER]
    void updatePlayer(int playerId);                   // [FRONTIER]
    void updatePlayerMap();                            // [FRONTIER]
    void updateBots();                                 // [FRONTIER]
    void updateBot(Bot* bot);                          // [FRONTIER]
    void removePlayer(int playerId);                   // [FRONTIER]

    static AILODManager* m_instance;

    // layout, on the ELF, read store by store out of the constructor
    /*  +4 */ std::map<int, PlayerLODStats> m_playerStats;   // `reset` clears it
    /* +16 */ std::set<int> m_set16;                         // [UNRESOLVED] - a 4-byte element
    /* +28 */ float m_basicLODDistance;    // = the first argument; `setBasicLODDistance` writes it
    /* +32 */ float m_maxLODDistance;      // = the first argument too - see the header note [INFERRED]
    /* +36 */ float m_f36;                 // = the second argument [UNRESOLVED]
    /* +40 */ bool  m_lodEnable;           // = true; the `lodEnable` pair, and `reset` raises it again
    /* +44 */ int   m_i44;                 // = -1 [UNRESOLVED]
    /* +48 */ int   m_i48;                 // = 0 [UNRESOLVED]
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_MAIN_AILODMANAGER_H
