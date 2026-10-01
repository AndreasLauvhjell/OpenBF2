// Geared vehicle engine selected by EngineTemplate type bit 0x40.
// [INFERRED] Source placement; uncertain field names remain annotated.

#ifndef GAME_OBJECTS_GAME_CARENGINE_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_CARENGINE_H

#include "EngineBase.h"

namespace dice {
namespace hfe {
namespace world {

class CarEngine : public EngineBase
{
public:
    explicit CarEngine(const EngineTemplate* objectTemplate);
    virtual ~CarEngine();

    virtual void  handleUpdate(float, unsigned int);
    virtual void  reset();
    virtual float getGearRatio(int gear) const;
    virtual float getCurrentTorque() const;
    virtual int   getGearChange() const;
    virtual void  setGearChange(int change);

    void revDownEngine(float targetInput, float dt);
    void applyBrakes(float throttle, float threshold);

protected:
    /* 0x210 */ int   m_gearChange;
    /* 0x214 */ float m_gearChangeRatio;
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_CARENGINE_H
