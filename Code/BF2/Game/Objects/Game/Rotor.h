// Visual rotor driven by a named engine, with blade/disc selection and a timed spin-down.
// Resolves its engine on the first update; subsequent rotation occurs during visual updates.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_ROTOR_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_ROTOR_H

#include "../Base/Bundle.h"

namespace dice {
namespace hfe {
namespace world {

class RotorTemplate;

// . Free function, external linkage in both shipped images, and it sits
// immediately in front of Rotor::handleVisualUpdate - its only caller - which is why it is declared
// here rather than in ObjectUtil.h. Wraps rotation[axis] into (-180, 180] by a single turn.
void clipRotation(Vec3& rotation, int axis);

class Rotor : public Bundle
{
public:
    explicit Rotor(const RotorTemplate* objectTemplate);
    virtual ~Rotor();

    virtual void disableCachedObject();
    virtual void enableCachedObject();
    virtual void handleVisualUpdate(float, float);
    virtual void handleUpdate(float, unsigned int);
    virtual void SetActiveLod(ObjectLodType lod);                           // (dice's capital S; the ELF answers 0/1 in eax on a void slot)

    IObject* getEngine();

protected:
    /* 0x13C */ bool  m_needsEngine;
    /* 0x140 */ int   m_geomState;        // the geom index currently selected: 0 = blades, 1 = blur, -1 = neither yet
    /* 0x144 */ float m_spinDownTimer;    // 3.0 while the engine runs, counted down by deltaTime once it stops
    /* 0x148 */ float m_spinDownSpeed;    // the rate captured on the last running frame, faded out by the timer
    /* 0x14C */ IObject* m_engine;
    /* 0x150 */ Vec3  m_rotation;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_ROTOR_H
