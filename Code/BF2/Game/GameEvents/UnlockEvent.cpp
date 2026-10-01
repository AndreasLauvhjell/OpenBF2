#include "UnlockEvent.h"

#include "../Objects/IPlayer.h"                  // setUnlocks (+0x25C)
#include "../../Core/EventManager.h"             // eventManager->postEvent
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEUnlock (0x888AC80) = 42.
const int GEUnlock = 42;

// The notification executeClient raises: category ECHud (2), event id 0x1F = 31, NO payload.
const unsigned int NOTIFY_UNLOCK = 0x1Fu;

// The array's shape. Both loops in the ELF are `<= 1` and `<= 6` over a flat dword run at +0x14.
const int UNLOCK_SLOTS = 2;
const int UNLOCK_KITS  = 7;

// The width loop the ELF emits inline for the 2-bit type field - see ConnectionTypeEvent.cpp for
// the full derivation. Only the type uses it here; everything else is a literal width.
unsigned int bitsToHold(unsigned int highest)
{
    unsigned int bits = 0;
    do
        ++bits;
    while (((1u << bits) - 1u) < highest);
    return bits;
}
} // namespace

// The maker's form. deSerialize fills every field; the zeroing is the rebuild's courtesy - see
// GameEventMakerMaker<T> in Game/Common/GameEvent.h.
UnlockEvent::UnlockEvent()
    : GameEvent()
    , m_unlockType(0)
    , m_playerId(0)
{
    for (int slot = 0; slot < UNLOCK_SLOTS; ++slot)
        for (int kit = 0; kit < UNLOCK_KITS; ++kit)
            m_unlocks[slot][kit] = 0;
}

// Copies fourteen unlock values from two caller maps using operator[], inserting
// zero-valued missing keys in those maps. Construction does not execute locally.
UnlockEvent::UnlockEvent(UnlockEventTypes type, int playerId, std::map<int, int>* unlocks)
    : GameEvent()
    , m_unlockType(static_cast<int>(type))                   // +0x0C
    , m_playerId(playerId)                                   // +0x10
{
    for (int slot = 0; slot < UNLOCK_SLOTS; ++slot)
        for (int kit = 0; kit < UNLOCK_KITS; ++kit)
            m_unlocks[slot][kit] = unlocks[slot][kit];        // operator[] - it inserts
}

int UnlockEvent::getType() const
{
    return GEUnlock;
}

// Builds the two unlock maps and applies them to an existing player without a
// null check. The payload-free HUD notification reads the updated player state.
void UnlockEvent::executeClient()
{
    world::IPlayer* player = world::playerManager->getPlayerFromId(m_playerId);   // +0x20

    std::map<int, int> unlocks[UNLOCK_SLOTS];
    for (int slot = 0; slot < UNLOCK_SLOTS; ++slot)
        for (int kit = 0; kit < UNLOCK_KITS; ++kit)
            unlocks[slot][kit] = m_unlocks[slot][kit];

    player->setUnlocks(unlocks);                                                  // +0x25C

    eventManager->postEvent(ECHud, NOTIFY_UNLOCK, 0, 0.0f);                       // +0x14
}

// `mov eax, 1; ret`. The server originates these; it never receives one.
bool UnlockEvent::executeServer(int /*connectionId*/)
{
    return true;
}

// deSerialize - 0x807FD70, 272 B. Type, player, then the fourteen nibbles in flat order.
bool UnlockEvent::deSerialize(io::BitStream* stream)
{
    int unlockType = 0;
    stream->readBits(&unlockType, bitsToHold(2));
    m_unlockType = unlockType;

    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = playerId;

    for (int slot = 0; slot < UNLOCK_SLOTS; ++slot)
    {
        for (int kit = 0; kit < UNLOCK_KITS; ++kit)
        {
            int level = 0;
            stream->readBits(&level, 4);
            m_unlocks[slot][kit] = level;
        }
    }

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// Writes levels as unchecked four-bit values; levels above 15 are silently truncated.
bool UnlockEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int unlockType = m_unlockType;
    stream->writeBits(&unlockType, bitsToHold(2));

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    for (int slot = 0; slot < UNLOCK_SLOTS; ++slot)
    {
        for (int kit = 0; kit < UNLOCK_KITS; ++kit)
        {
            int level = m_unlocks[slot][kit];
            stream->writeBits(&level, 4);
        }
    }

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
