// Requests an area's effect at a position, optionally attached to a networked object.
// The constructor requires an AmbientEffectArea despite the IObject parameter type.
// Deserialization resolves the area ID before execution; m_team selects an effect child,
// not a team. Attachment is suppressed when the template disables attachToEventObject.

#ifndef GAME_GAMEEVENTS_AMBIENTEFFECTAREAEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_AMBIENTEFFECTAREAEVENT_H

#include "../Common/GameEvent.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace world { class IObject; class AmbientEffectArea; }

class AmbientEffectAreaEvent : public GameEvent              // 0x24
{
public:
    // attested: _ZN4dice3hfe22AmbientEffectAreaEventC2EPNS0_5world7IObjectEiNS0_4Vec3Et is
    // (world::IObject*, int, Vec3, unsigned short) - the Vec3 BY value.
    AmbientEffectAreaEvent(world::IObject* area, int team, Vec3 position,
                           unsigned short objectNetId);
    AmbientEffectAreaEvent()
        : m_areaId(0), m_team(0), m_objectNetId(0), m_area(0)
        { m_position.x = m_position.y = m_position.z = 0.0f; }
    virtual ~AmbientEffectAreaEvent() {}

    virtual int  getType() const;                            // GEAmbientEffectArea (34)
    virtual void executeClient();
    virtual bool executeServer(int connectionId);            // empty
    virtual bool deSerialize(io::BitStream* stream);         // can abort
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ Vec3                      m_position;
    /* +0x18 */ unsigned int              m_areaId;
    /* +0x1C */ unsigned char             m_team;
    /* +0x1E */ unsigned short            m_objectNetId;
    /* +0x20 */ world::AmbientEffectArea* m_area;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_AMBIENTEFFECTAREAEVENT_H
