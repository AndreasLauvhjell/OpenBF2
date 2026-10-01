// Replicates voice enablement. Clients update the named player; servers relay status
// without applying it to a Player object. [INFERRED] Constructor argument order.

#ifndef GAME_GAMEEVENTS_VOIPONOFFEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_VOIPONOFFEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class VoipOnOffEvent : public GameEvent                      // 0x10
{
public:
    // [INFERRED] argument order - no ELF constructor exists.
    VoipOnOffEvent(int playerId, bool voipEnabled);
    VoipOnOffEvent() : m_playerId(0), m_voipEnabled(false) {}   // the maker's form
    virtual ~VoipOnOffEvent() {}

    virtual int  getType() const;                            // GEVoipOnOff (35)
    virtual void logInfo(bool, int);                         // empty
    virtual void executeClient();
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ unsigned char m_playerId;
    /* +0x0D */ bool          m_voipEnabled;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_VOIPONOFFEVENT_H
