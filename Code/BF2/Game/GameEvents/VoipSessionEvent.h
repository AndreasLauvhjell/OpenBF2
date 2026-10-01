// Delivers the allocated voice-session ID to a newly created player.
// The constructor narrows the supplied integer to the 16-bit field and wire value.

#ifndef GAME_GAMEEVENTS_VOIPSESSIONEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_VOIPSESSIONEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class VoipSessionEvent : public GameEvent                   // 0x10
{
public:
    // NO ELF address - inlined at createPlayer's call site. See the header block.
    explicit VoipSessionEvent(int sessionId);

    // The maker leaves sessionId uninitialized until deserialization.
    VoipSessionEvent() {}
    virtual ~VoipSessionEvent() {}

    virtual int  getType() const;                           // GEVoipSession (54)

    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);
    virtual void executeClient();
    virtual bool executeServer(int connectionId);

protected:
    /* +0x0C */ unsigned short m_sessionId;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_VOIPSESSIONEVENT_H
