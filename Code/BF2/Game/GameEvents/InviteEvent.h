// Invites a player to a squad or withdraws an invitation.
// An invitation sets the player's invite bit and immediately joins AI players; withdrawal clears it.
// The wire carries team, player and squad bytes followed by the invited bit.

#ifndef GAME_GAMEEVENTS_INVITEEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_INVITEEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class InviteEvent : public GameEvent                         // 0x10
{
public:
    // [INFERRED] argument order - the ELF has no constructor for this class; every InviteEvent in
    // the binary is made by the registry maker and filled by deSerialize.
    InviteEvent(int team, int playerId, int squadId, bool invited);
    InviteEvent() : m_team(0), m_playerId(0), m_squadId(0), m_invited(false) {}
    virtual ~InviteEvent() {}

    virtual int  getType() const;                            // GEInvite (25)
    virtual void executeClient();                            // empty
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ unsigned char m_team;
    /* +0x0D */ unsigned char m_playerId;
    /* +0x0E */ unsigned char m_squadId;
    /* +0x0F */ bool          m_invited;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_INVITEEVENT_H
