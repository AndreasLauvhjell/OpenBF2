// Requests a free-camera toggle for the sending connection. Carries only its event
// type; player identity comes from the connection. [INFERRED] Header placement.

#ifndef GAME_GAMEEVENTS_TOGGLEFREECAMERAEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_TOGGLEFREECAMERAEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class ToggleFreeCameraEvent : public GameEvent               // 0x0C - no members
{
public:
    ToggleFreeCameraEvent() {}
    virtual ~ToggleFreeCameraEvent() {}

    virtual int  getType() const;                            // GEToggleFreeCamera (40)
    virtual void executeClient();                            // empty
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);         // reads nothing
    virtual bool serialize(io::BitStream* stream);           // the type id only
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_TOGGLEFREECAMERAEVENT_H
