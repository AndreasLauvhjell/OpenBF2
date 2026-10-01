// Replicates a kit pickup to a client whose player has a controlled vehicle.
// [INFERRED] Constructor ordering and the kit/vehicle network-ID names remain inferred.

#ifndef GAME_GAMEEVENTS_HANDLEPICKUPEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_HANDLEPICKUPEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class HandlePickupEvent : public GameEvent                   // 0x14
{
public:
    // [INFERRED] argument order - no ELF constructor exists. The order follows HandleDropEvent's,
    // which IS attested.
    HandlePickupEvent(unsigned char playerId, unsigned short kitNetId,
                      unsigned short vehicleNetId);
    HandlePickupEvent() : m_playerId(0), m_kitNetId(0), m_vehicleNetId(0) {}
    virtual ~HandlePickupEvent() {}

    virtual int  getType() const;                            // GEHandlePickup (14)
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
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_HANDLEPICKUPEVENT_H
