// Sends a laser designator's two aim vectors to the server; client execution is fatal.
// The constructor requires a parent chain containing a player-control object.
// The network ID and weapon index precede six uncompressed floats on the wire.
// [UNRESOLVED] The two vectors' position/direction roles are not established.

#ifndef GAME_GAMEEVENTS_TARGETDIRECTIONEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_TARGETDIRECTIONEVENT_H

#include "../Common/GameEvent.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace world { class IObject; }

class TargetDirectionEvent : public GameEvent                // 0x28
{
public:
    // attested: _ZN4dice3hfe20TargetDirectionEventC2EPNS0_5world7IObjectERNS0_4Vec3ES6_ is
    // (world::IObject*, Vec3&, Vec3&).
    TargetDirectionEvent(world::IObject* object, Vec3& field0C, Vec3& field18);
    TargetDirectionEvent() : m_netId(0), m_weaponIndex(0)
        { m_field0C.x = m_field0C.y = m_field0C.z = 0.0f;
          m_field18.x = m_field18.y = m_field18.z = 0.0f; }
    virtual ~TargetDirectionEvent() {}

    virtual int  getType() const;                            // GETargetDirection (47)
    virtual void executeClient();                            // aborts
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ Vec3           m_field0C;
    /* +0x18 */ Vec3           m_field18;
    /* +0x24 */ unsigned short m_netId;
    /* +0x26 */ unsigned char  m_weaponIndex;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_TARGETDIRECTIONEVENT_H
