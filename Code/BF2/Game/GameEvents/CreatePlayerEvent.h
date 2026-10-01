// Announces a player, team, spawn group and AI/remote flags to clients.
// Player, team and spawn-group IDs narrow to bytes; the network ID is 16 bits.
// Names are truncated to 31 characters plus a terminator.
// [UNRESOLVED] m_word34 is initialized to zero; its meaning is unknown.

#ifndef GAME_GAMEEVENTS_CREATEPLAYEREVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_CREATEPLAYEREVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

namespace world { class IPlayer; }

class CreatePlayerEvent : public GameEvent                  // 0x38
{
public:
    // Default event-maker form; deserialization supplies the payload.
    CreatePlayerEvent();

    CreatePlayerEvent(world::IPlayer* player, bool isRemote);
    virtual ~CreatePlayerEvent() {}

    virtual int  getType() const;                           // GECreatePlayer (5)  was documented as 4 - .rodata 0x8888EA0 says 5

    virtual void logInfo(bool, int) const;
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);
    virtual void executeClient();

protected:
    /* +0x0C */ unsigned char  m_teamId;
    /* +0x0D */ unsigned char  m_spawnGroupId;
    /* +0x0E */ bool           m_isRemote;
    /* +0x0F */ bool           m_isAIPlayer;
    /* +0x10 */ char           m_playerName[32];
    /* +0x30 */ unsigned char  m_playerId;
    /* +0x31 */ unsigned char  m_pad31;              // not written by the ctor
    /* +0x32 */ unsigned short m_playerNetId;
    /* +0x34 */ unsigned short m_word34;             // stored 0; meaning [UNRESOLVED]
    /* +0x36 */ unsigned short m_pad36;              // tail padding to 0x38
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_CREATEPLAYEREVENT_H
