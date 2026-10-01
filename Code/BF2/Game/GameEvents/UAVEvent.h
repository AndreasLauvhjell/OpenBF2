// Requests a UAV operation at an X/Z position. The selector chooses a scan event
// identified by connection ID or a UAV event identified by team.
// [INFERRED] The selector name and four-scalar constructor remain inferred.

#ifndef GAME_GAMEEVENTS_UAVEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_UAVEVENT_H

#include "../Common/GameEvent.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {

class UAVEvent : public GameEvent                            // 0x18
{
public:
    // [INFERRED] argument order - no ELF constructor exists.
    UAVEvent(float positionX, float positionZ, int team, bool isScan);
#if !DEDICATED
    // Client PE7B2430: team byte, returned Vec2 by value, scan selector.
    UAVEvent(int team, Vec2 position, bool isScan);
#endif
    UAVEvent() : m_positionX(0.0f), m_positionZ(0.0f), m_team(0), m_isScan(false) {}
    virtual ~UAVEvent() {}

    virtual int  getType() const;                            // geuav (45)
    virtual void executeClient();                            // empty
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ float         m_positionX;
    /* +0x10 */ float         m_positionZ;
    /* +0x14 */ unsigned char m_team;
    /* +0x15 */ bool          m_isScan;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_UAVEVENT_H
