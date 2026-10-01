// Checks the sender's view of another player's team; FixPlayerTeamEvent carries the reply.
// Constructor argument one is the sender, argument two the subject. Both IDs and teams
// are narrowed to bytes; the sender ID identifies the reply recipient.

#ifndef GAME_GAMEEVENTS_VERIFYPLAYERTEAMEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_VERIFYPLAYERTEAMEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {
namespace world { class IPlayer; }

class VerifyPlayerTeamEvent : public GameEvent               // 0x10
{
public:
    // attested: _ZN4dice3hfe21VerifyPlayerTeamEventC2EPNS0_5world7IPlayerES4_ is (IPlayer*, IPlayer*).
    VerifyPlayerTeamEvent(world::IPlayer* localPlayer, world::IPlayer* player);
    VerifyPlayerTeamEvent()
        : m_playerTeamId(0), m_localPlayerTeamId(0), m_playerId(0), m_localPlayerId(0) {}
    virtual ~VerifyPlayerTeamEvent() {}

    virtual int  getType() const;                            // GEVerifyPlayerTeam (68)
    virtual void logInfo(bool, int);                         // empty
    virtual void executeClient();                            // does nothing, see the .cpp
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ unsigned char m_playerTeamId;
    /* +0x0D */ unsigned char m_localPlayerTeamId;
    /* +0x0E */ unsigned char m_playerId;
    /* +0x0F */ unsigned char m_localPlayerId;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_VERIFYPLAYERTEAMEVENT_H
