// Routes a voice-mute change to m_targetPlayerId and updates m_mutedPlayerId's flag.
// The wire carries the mute bit before the two player IDs.
// [INFERRED] The constructor's argument order is inferred from the forwarding path.

#ifndef GAME_GAMEEVENTS_VOIPPLAYERMUTEEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_VOIPPLAYERMUTEEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class VoipPlayerMuteEvent : public GameEvent                 // 0x10
{
public:
    // [INFERRED] argument order; the stores are executeServer's inline copy, in its order.
    VoipPlayerMuteEvent(bool muted, int mutedPlayerId, int targetPlayerId);
    VoipPlayerMuteEvent() : m_muted(false), m_mutedPlayerId(0), m_targetPlayerId(0) {}
    virtual ~VoipPlayerMuteEvent() {}

    virtual int  getType() const;                            // GEVoipPlayerMuteEvent (38)
    virtual void logInfo(bool, int);                         // empty
    virtual void executeClient();
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ bool          m_muted;
    /* +0x0D */ unsigned char m_mutedPlayerId;
    /* +0x0E */ unsigned char m_targetPlayerId;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_VOIPPLAYERMUTEEVENT_H
