// Server-to-client unlock levels: two teams by seven kits, four bits per level.
// Construction reads maps with operator[] and inserts missing keys 0..6 as zero.
// The constructor does not execute locally; server execution is inert.

#ifndef GAME_GAMEEVENTS_UNLOCKEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_UNLOCKEVENT_H

#include <map>

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class UnlockEvent : public GameEvent                         // 0x4C
{
public:
    // [UNRESOLVED] Enumerator names; known callers pass value 1.
    enum UnlockEventTypes : int;

    // `unlocks` IS AN array OF TWO maps, not one - and it is read through operator[], so it is
    // not const and the call has side effects on it.
    UnlockEvent(UnlockEventTypes type, int playerId,
                std::map<int, int>* unlocks);
    UnlockEvent();                                           // the maker's form
    virtual ~UnlockEvent() {}

    virtual int  getType() const;                            // GEUnlock (42)
    virtual void executeClient();
    virtual bool executeServer(int connectionId);            // empty
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ int m_unlockType;        // held as int - see the opaque enum note above
    /* +0x10 */ int m_playerId;
    /* +0x14 */ int m_unlocks[2][7];     // [slot][kit], four bits each on the wire
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_UNLOCKEVENT_H
