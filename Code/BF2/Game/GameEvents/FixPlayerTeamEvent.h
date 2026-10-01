// Correct a tracked player's team after verification. Clients apply the answer
// only for the addressed local human and a team mismatch; server execution
// replaces the subject team with its authoritative value. All ids narrow to bytes.

#ifndef GAME_GAMEEVENTS_FIXPLAYERTEAMEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_FIXPLAYERTEAMEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {
namespace world { class IPlayer; }

class FixPlayerTeamEvent : public GameEvent                  // 0x10
{
public:
    // attested: _ZN4dice3hfe18FixPlayerTeamEventC2EPNS0_5world7IPlayerES4_ is (IPlayer*, IPlayer*).
    // The second argument is the subject - see the header block.
    FixPlayerTeamEvent(world::IPlayer* localPlayer, world::IPlayer* player);
    FixPlayerTeamEvent()
        : m_playerTeamId(0), m_localPlayerTeamId(0), m_playerId(0), m_localPlayerId(0) {}
    virtual ~FixPlayerTeamEvent() {}

    virtual int  getType() const;                            // GEFixPlayerTeam (69)
    virtual void logInfo(bool, int);                         // empty
    virtual void executeClient();
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

#endif // GAME_GAMEEVENTS_FIXPLAYERTEAMEVENT_H
