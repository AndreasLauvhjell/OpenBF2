// Carries kill details and forwards the payload to game-logic listeners.
// The wire places the hit-bone index before the compressed direction, unlike memory order.
// [UNRESOLVED] The payload's original type name is unknown; several field names remain inferred.

#ifndef GAME_GAMEEVENTS_KILLEDBYEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_KILLEDBYEVENT_H

#include "../Common/GameEvent.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {

// Kill notification payload shared with fillKilledByData. Weapon and vehicle template
// ids have distinct in-memory and wire ordering; preserve the declared field layout.
struct KilledByData                                          // 0x20
{
    /* +0x00 */ unsigned char  killerPlayerId;
    /* +0x01 */ unsigned char  victimPlayerId;
    /* +0x02 */ bool           isKillerPlayerAlive;
    /* +0x03 */ // one byte of alignment padding
    /* +0x04 */ unsigned short field04;                  // the material, narrowed to 16 bits
                                                         //   (fillKilledByData's `mov [edx+4], ax`)
    /* +0x06 */ // two bytes of alignment padding
    /* +0x08 */ unsigned int   vehicleTemplateId;        // the killer's vehicle (or its default one)
    /* +0x0C */ unsigned int   someObjTemplateId;        // the weapon's template - the IDB's name,
                                                         //   role from fillKilledByData
    /* +0x10 */ Vec3           lastHitDirection;
    /* +0x1C */ unsigned char  lastHitBoneIndex;
};

class KilledByEvent : public GameEvent                       // 0x2C
{
public:
    typedef dice::hfe::KilledByData KilledByData;            // the old nested spelling

    KilledByEvent();                                         // the maker's form - no ELF ctor
    // The server's form (serverGiveDamage 0x80E08xx: GameEvent(), the vtable, the data zeroed and
    // then copied field by field - an inlined constructor from the record). [INFERRED form]
    explicit KilledByEvent(const KilledByData& data) : GameEvent(), m_data(data) {}
    virtual ~KilledByEvent() {}

    virtual int  getType() const;                            // GEKilledBy (21)
    virtual void executeClient();
    virtual bool executeServer(int connectionId);            // a no-op, see the .cpp
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ KilledByData m_data;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_KILLEDBYEVENT_H
