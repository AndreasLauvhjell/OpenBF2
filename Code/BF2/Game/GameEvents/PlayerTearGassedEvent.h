// Client-reported tear-gas state replayed through Player::setBlurryVision on the
// server. Player id uses eight wire bits. [UNRESOLVED] The three float roles are
// not fully identified; preserve their argument order and neutral field names.

#ifndef GAME_GAMEEVENTS_PLAYERTEARGASSEDEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_PLAYERTEARGASSEDEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class PlayerTearGassedEvent : public GameEvent               // 0x1C
{
public:
    // attested: _ZN4dice3hfe21PlayerTearGassedEventC2Eifff is (int, float, float, float).
    PlayerTearGassedEvent(int playerId, float gasDamage, float f14, float f18);
    PlayerTearGassedEvent()
        : m_playerId(0), m_gasDamage(0.0f), m_field14(0.0f), m_field18(0.0f) {}
    virtual ~PlayerTearGassedEvent() {}

    virtual int  getType() const;                            // GEPlayerTearGassedEvent (65)
    virtual void executeClient();                            // empty
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ int   m_playerId;
    /* +0x10 */ float m_gasDamage;
    /* +0x14 */ float m_field14;
    /* +0x18 */ float m_field18;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_PLAYERTEARGASSEDEVENT_H
