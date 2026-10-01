// Remote-controlled object that turns its host toward a mission target.
// initMission initializes direction and angle state before updateMission. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_TURNABLEREMOTECONTROLLEDOBJECT_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_TURNABLEREMOTECONTROLLEDOBJECT_H

#include "RemoteControlledObject.h"

namespace dice {
namespace hfe {
namespace world {

class TurnableRemoteControlledObjectTemplate;

class TurnableRemoteControlledObject : public RemoteControlledObject
{
public:
    explicit TurnableRemoteControlledObject(const TurnableRemoteControlledObjectTemplate* objectTemplate);
    virtual ~TurnableRemoteControlledObject();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;
    virtual void  initMission(Vec3& target);
    virtual int   updateMission(float dt);
    virtual int   getStatus() const;

    void setHostObjectRelativeTransformation(Mat4& transform);

protected:
    /* 0x150 */ int   m_refCount;
    /* 0x154 */ const TurnableRemoteControlledObjectTemplate* m_template;
    /* 0x158 */ Vec3  m_targetDirection;
    /* 0x164 */ Vec3  m_initialHostForward;
    /* 0x170 */ int   m_turnDirection;                                      // initMission; ctor untouched
    /* 0x174 */ int   m_projectionAxis;                                     // initMission; ctor untouched
    /* 0x178 */ int   m_rotationAxis;                                       // initMission; ctor untouched
    /* 0x17C */ int   m_fixedAxis;                                          // initMission; ctor untouched
    /* 0x180 */ float m_targetAngle;                                        // initMission; ctor untouched
    /* 0x184 */ float m_previousAngle;                                      // ctor -1.0
    /* 0x188 */ Vec3  m_lastRotation;                                       // ctor zero
    /* 0x194 */ int   m_status;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_TURNABLEREMOTECONTROLLEDOBJECT_H
