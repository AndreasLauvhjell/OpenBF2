// Replicates vehicle exit using the player ID; the client already knows the current vehicle.
// [UNRESOLVED] The trailing flag's meaning remains unknown.

#ifndef GAME_GAMEEVENTS_EXITVEHICLEEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_EXITVEHICLEEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class ExitVehicleEvent : public GameEvent                    // 0x10
{
public:
    // [INFERRED] argument order - no ELF constructor exists.
    ExitVehicleEvent(int playerId, bool field0D);
    ExitVehicleEvent() : m_playerId(0), m_field0D(false) {}
    virtual ~ExitVehicleEvent() {}

    virtual int  getType() const;                            // GEExitVehicle (10)
    virtual void logInfo(bool, int);                         // empty
    virtual void executeClient();
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    // executeServer is deliberately NOT declared - the slot is inherited.

protected:
    /* +0x0C */ unsigned char m_playerId;
    /* +0x0D */ bool          m_field0D;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_EXITVEHICLEEVENT_H
