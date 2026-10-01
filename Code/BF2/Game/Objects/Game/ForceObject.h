// Input-driven force and torque on the root physics node. [INFERRED] Source placement.
// The delay expiry uses 30 Hz ticks; -1 means unarmed and is restored when required occupancy is lost.

#ifndef GAME_OBJECTS_GAME_FORCEOBJECT_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_FORCEOBJECT_H

#include "../Base/SimpleObject.h"

namespace dice {
namespace hfe {
namespace io { class PlayerInput; }
namespace world {

class ForceObjectTemplate;

class ForceObject : public SimpleObject
{
public:
    explicit ForceObject(const ForceObjectTemplate* objectTemplate);
    virtual ~ForceObject();

    virtual void disableCachedObject();
    virtual void enableCachedObject();
    virtual void handleUpdate(float, unsigned int);
    virtual void handlePlayerInput(IPlayer*, const io::PlayerInput&, float, unsigned int);
    virtual void handleMessage(TemplateMessage, IPlayer*, bool);
    virtual void initNetworkableComp();

    void addRotationalAcceleration(const Vec3& acceleration);

protected:
    /* 0x138 */ const ForceObjectTemplate* m_template;
    /* 0x13C */ float m_input;
    /* 0x140 */ int   m_dword140;                    // [UNRESOLVED] - no recovered body reads it
    /* 0x144 */ int   m_dword144;                    // the start-delay expiry tick; -1 = not started
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_FORCEOBJECT_H
