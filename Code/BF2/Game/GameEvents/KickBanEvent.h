// Notifies clients of a kick or ban; server reception is a fatal error.
// Construction on a server also executes the client notification without a local-player check.
// [UNRESOLVED] m_field10 and m_field14 retain unknown meanings and their existing wire widths.

#ifndef GAME_GAMEEVENTS_KICKBANEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_KICKBANEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class KickBanEvent : public GameEvent                        // 0x18
{
public:
    // attested: _ZN4dice3hfe12KickBanEventC2Eibi is (int, bool, int).
    KickBanEvent(int playerId, bool field10, int field14);
    KickBanEvent() : m_playerId(0), m_field10(false), m_field14(0) {}   // the maker's form
    virtual ~KickBanEvent() {}

    virtual int  getType() const;                            // GEKickBan (55)
    virtual void executeClient();
    virtual bool executeServer(int connectionId);            // aborts
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ int  m_playerId;
    /* +0x10 */ bool m_field10;
    /* +0x14 */ int  m_field14;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_KICKBANEVENT_H
