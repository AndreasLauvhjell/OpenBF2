// Broadcasts the round state and next transition tick as two 32-bit words.
// The noVehicles argument is ignored. The local-loopback flag applies the event
// immediately when a local human player exists; initialize both fields before sending.

#ifndef GAME_GAMEEVENTS_BEGINROUNDEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_BEGINROUNDEVENT_H

#include "../Common/GameEvent.h"
#include "../IGameLogic.h"                    // BeginRoundState

namespace dice {
namespace hfe {

class BeginRoundEvent : public GameEvent                    // 0x14
{
public:
    //  (C2) / 0x806A050 (C1), 91 B. See the two warnings in the header block: noVehicles
    // is accepted and discarded, and executeLocally can run executeClient() from inside the ctor.
    BeginRoundEvent(BeginRoundState state, unsigned int nextStateChangeAtTick,
                    bool noVehicles, bool executeLocally);

    // Leaves payload fields uninitialized for deSerialize to populate before use.
    BeginRoundEvent() {}
    virtual ~BeginRoundEvent() {}

    virtual int  getType() const;                           // GEBeginRound (56)
    virtual void executeClient();
    virtual bool executeServer(int connectionId);           // aborts, see the .cpp
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ int          m_beginRoundState;
    /* +0x10 */ unsigned int m_nextStateChangeAtTick;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_BEGINROUNDEVENT_H
