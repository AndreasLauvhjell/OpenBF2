// Replicate trigger bounds and acknowledge their network object. Execution hooks
// are inert; eventReceivedByRemote performs the acknowledgement.
// The constructor ignores its trailing integers and leaves netId unset.
// [UNRESOLVED] The scalar roles and unused trailing word remain unidentified;
// retain their storage and the reversed trailing-byte wire order.

#ifndef GAME_GAMEEVENTS_UPDATETRIGGEREVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_UPDATETRIGGEREVENT_H

#include "../Common/GameEvent.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {

class UpdateTriggerEvent : public GameEvent                  // 0x38
{
public:
    // attested: (unsigned int, const Vec3&, const Vec3&, int, int).
    // The last two arguments are accepted AND discarded - see the header block.
    UpdateTriggerEvent(unsigned int field0C, const Vec3& min, const Vec3& max,
                       int discarded1, int discarded2);
    UpdateTriggerEvent();                                    // the maker's form
    virtual ~UpdateTriggerEvent() {}

    virtual int  getType() const;                            // GEUpdateTrigger (59)
    virtual void logInfo(bool, int);                         // empty
    virtual void eventReceivedByRemote(void* node);
    virtual void executeClient();                            // empty
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    // executeServer is deliberately NOT declared - the slot is inherited.

protected:
    /* +0x0C */ unsigned int   m_field0C;
    /* +0x10 */ unsigned short m_netId;
    /* +0x12 */ // two bytes of alignment padding
    /* +0x14 */ Vec3           m_min;
    /* +0x20 */ Vec3           m_max;
    /* +0x2C */ int            m_field2C;
    /* +0x30 */ int            m_field30;
    /* +0x34 */ int            m_field34;    // untouched by everything in the ELF - see above
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_UPDATETRIGGEREVENT_H
