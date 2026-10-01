// Replicates a dropped kit and its uncompressed position to clients.
// The event identifies the player, kit and dropping vehicle. Unlike pickup handling,
// client execution does not require the player to have a current vehicle.

#ifndef GAME_GAMEEVENTS_HANDLEDROPEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_HANDLEDROPEVENT_H

#include "../Common/GameEvent.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {

class HandleDropEvent : public GameEvent                     // 0x20
{
public:
    // attested: (unsigned char, unsigned short, unsigned short, const Vec3&).
    HandleDropEvent(unsigned char playerId, unsigned short kitNetId,
                    unsigned short vehicleNetId, const Vec3& position);
    HandleDropEvent() : m_playerId(0), m_kitNetId(0), m_vehicleNetId(0)
        { m_position.x = 0.0f; m_position.y = 0.0f; m_position.z = 0.0f; }
    virtual ~HandleDropEvent() {}

    virtual int  getType() const;                            // GEHandleDrop (13)
    virtual void logInfo(bool, int);                         // empty
    virtual void executeClient();
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    // executeServer is deliberately NOT declared - the slot is inherited.

protected:
    /* +0x0C */ unsigned char  m_playerId;
    /* +0x0D */ // one byte of alignment padding
    /* +0x0E */ unsigned short m_kitNetId;
    /* +0x10 */ unsigned short m_vehicleNetId;
    /* +0x12 */ // two bytes of alignment padding
    /* +0x14 */ Vec3           m_position;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_HANDLEDROPEVENT_H
