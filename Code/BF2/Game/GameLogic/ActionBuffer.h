// Owns a player's queued input history and current action.
// clear(index) resets the input indices; these fields are separate from the list's size.
// [INFERRED] Each ActionBufferEntry contains only a PlayerAction.
// STL-dependent offsets must be validated for the active compiler and architecture.

#ifndef GAME_GAMELOGIC_ACTIONBUFFER_H     // [INFERRED] guard style
#define GAME_GAMELOGIC_ACTIONBUFFER_H

#include <list>

#include "../Common/PlayerAction.h"    // Game/Common/, not beside this file - the path is
                                       // attested twice in the binary; see PlayerAction.h

namespace dice {
namespace hfe {

// [INFERRED] - see the header block. The name is attested (it is in three STL mangled names); the
// content is arithmetic: 28 bytes, and PlayerAction is 28 bytes.
class ActionBufferEntry                      // 0x1C
{
public:
    PlayerAction m_action;
};

class ActionBuffer                           // 0x2C
{
public:
    ActionBuffer();
    ~ActionBuffer();

    // Returns the newly added entry for callers to inspect.
    ActionBufferEntry* add(const PlayerAction& action);
                                                          //   (stamps index), PE 5BC4D0
    void removeOlderThanIndex(int index);
    void clear(int index);
    int  size() const;
    bool pushBack(PlayerAction& action);

    // latch the front action into m_current / m_currentInputIndex and
    // drop it. The prediction replay in GameClient::handleControlObjectState calls it once,
    // before walking what is left. [INFERRED] name.
    void popFront();

    /* +0x00 */ std::list<ActionBufferEntry> m_entries;   // four bytes in the ELF - see above
    /* +0x04 */ int          m_lastInputIndex;
    /* +0x08 */ int          m_currentInputIndex;
    /* +0x0C */ PlayerAction m_current;
    // Consecutive ticks reusing the last action; reset when a queued action is consumed.
    // After 29 replay ticks, simulation switches to zero input.
    /* +0x28 */ int          m_replayCount;  // 0 - see simulatePlayerUpdate
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMELOGIC_ACTIONBUFFER_H
