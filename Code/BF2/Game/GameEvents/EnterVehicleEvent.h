// Replicates a server-side vehicle entry to clients using the player and vehicle network IDs.
// The final wire bit requests making the vehicle the player's default.

#ifndef GAME_GAMEEVENTS_ENTERVEHICLEEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_ENTERVEHICLEEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class EnterVehicleEvent : public GameEvent                   // 0x14
{
public:
    // [INFERRED] argument order - no ELF constructor exists.
    EnterVehicleEvent(int playerId, unsigned short netId, bool setAsDefaultVehicle);
    EnterVehicleEvent() : m_playerId(0), m_netId(0), m_setAsDefaultVehicle(false) {}
    virtual ~EnterVehicleEvent() {}

    virtual int  getType() const;                            // GEEnterVehicle (9)
    virtual void logInfo(bool, int);                         // empty
    virtual void executeClient();
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    // executeServer is deliberately NOT declared - the slot is inherited.

protected:
    /* +0x0C */ unsigned char  m_playerId;
    /* +0x0D */ // one byte of alignment padding - see the header block
    /* +0x0E */ unsigned short m_netId;
    /* +0x10 */ bool           m_setAsDefaultVehicle;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_ENTERVEHICLEEVENT_H
