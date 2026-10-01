#include "BeginRoundEvent.h"

#include "../../Core/Debug.h"                    // executeServer's abort
#include "../../Core/EventManager.h"             // eventManager - executeClient's local repost
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getLocalHumanPlayer - the ctor's loopback gate
#include "../IGameLogic.h"                       // gameLogic - executeClient's target

namespace dice {
namespace hfe {

// The GE wire id is 56 (.rodata GEBeginRound; BeginRoundEvent::getType 0x806A460).
// Its registration-order index is 39; do not use that index on the wire.
const int GEBeginRound = 56;

//  (C2) / 0x806A050 (C1), 91 B. GameEvent's ctor, the vptr, the two stores - and then the
// loopback arm. See BeginRoundEvent.h for the two warnings this body carries.
BeginRoundEvent::BeginRoundEvent(BeginRoundState state, unsigned int nextStateChangeAtTick,
                                 bool /*noVehicles*/, bool executeLocally)
    : GameEvent()
    , m_beginRoundState(static_cast<int>(state))                                 // +0x0C
    , m_nextStateChangeAtTick(nextStateChangeAtTick)                             // +0x10
{
    // noVehicles is discarded. A listen server can apply the event immediately to its local human player.
    if (executeLocally && world::playerManager->getLocalHumanPlayer())            // +0x30
        executeClient();
}

int BeginRoundEvent::getType() const
{
    return GEBeginRound;
}

// serialize - 0x806A140, 171 B. The type id in the registry's width, then the two words raw.
bool BeginRoundEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int state = m_beginRoundState;
    stream->writeBits(&state, 32);

    int tick = static_cast<int>(m_nextStateChangeAtTick);
    stream->writeBits(&tick, 32);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// deSerialize - 0x806A0B0, 135 B. The mirror, less the type id: the dispatcher above has already
// consumed that to decide which class to build.
bool BeginRoundEvent::deSerialize(io::BitStream* stream)
{
    int state = 0;
    stream->readBits(&state, 32);
    m_beginRoundState = state;

    int tick = 0;
    stream->readBits(&tick, 32);
    m_nextStateChangeAtTick = static_cast<unsigned int>(tick);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// Applies remote round state only when not server, but always reposts the local
// notification. Listen-server constructor loopback therefore only reposts.
void BeginRoundEvent::executeClient()
{
    if (!gameLogic->isServer())
    {
        gameLogic->setBeginRoundState(static_cast<BeginRoundState>(m_beginRoundState));   // +0x160
        gameLogic->setNextStateChangeAtTick(m_nextStateChangeAtTick);                     // +0x168
    }
    eventManager->postEvent(static_cast<EventCategory>(2), 72u, 0, 0.0f);                 // +0x14
}

// Fatal protocol misuse: this server-bound event reaches a type-6 diagnostic.
// The broadcast is one-way; the nominal Boolean return is unreachable.
bool BeginRoundEvent::executeServer(int)
{
    Debug(DEBUGTYPE_6, "Game/GameEvents/BeginRoundEvent.cpp", 99, "")
        << "Server shouldnt get beginroundevents from outside.";
    return true;
}

} // namespace hfe
} // namespace dice
