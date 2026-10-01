// Requests a new commander-camera position. The server checks the camera type
// and applies the position with forceSetPosition. All three floats cross the wire uncompressed.

#ifndef GAME_GAMEEVENTS_COMMANDERCAMEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_COMMANDERCAMEVENT_H

#include "../Common/GameEvent.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {

class CommanderCamEvent : public GameEvent                   // 0x18
{
public:
    explicit CommanderCamEvent(Vec3 position);               // PE626230, by-value twelve bytes
    CommanderCamEvent() { m_position.x = 0.0f; m_position.y = 0.0f; m_position.z = 0.0f; }
    virtual ~CommanderCamEvent() {}

    virtual int  getType() const;                            // GECommanderCam (36)
    virtual void executeClient();                            // empty
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

protected:
    /* +0x0C */ Vec3 m_position;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_COMMANDERCAMEVENT_H
