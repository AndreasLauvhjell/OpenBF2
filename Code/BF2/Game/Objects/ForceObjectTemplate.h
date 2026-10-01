// Force-emitter settings. Member names follow console properties. [INFERRED] Source placement.
// constantForce is a floating-point scalar consumed by ForceObject::handleUpdate.

#ifndef GAME_OBJECTS_FORCEOBJECTTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_FORCEOBJECTTEMPLATE_H

#include "Base/SimpleObjectTemplate.h"

namespace dice {
namespace hfe {
namespace world {

class ForceObjectTemplate : public SimpleObjectTemplate
{
    // ForceObject consumes these template fields directly.
    friend class ForceObject;
    friend struct VehicleTemplateConsoleAccess;
public:
    ForceObjectTemplate();                            // read in full
    virtual unsigned int getClassID() const;
    virtual ~ForceObjectTemplate();
    virtual void* queryImplementation(unsigned int);
    virtual IObject* createObject() const;
    virtual bool  makeScript(io::IStream*);

protected:
    /* 0x164 */ int   input;                     // [INFERRED] an io::PlayerInputMap id - the console
                                                 // API declares `input` as PlayerInputMap on the
                                                 // other templates that expose it
    /* 0x168 */ float force;                     // [INFERRED] float - 4 bytes to the next field

    // +0x16C to +0x178 is twelve bytes, so this one is a Vec3 by spacing, which also agrees with the
    // console type strings. This is the field the conflict above does NOT affect.
    /* 0x16C */ float rotationalForce[3];        // 0x16C..0x177

    /* 0x178 */ float constantForce;

    // Counter-force mode tests velocity against the force direction.
    // The transform flag selects relative coordinates when set and absolute coordinates when clear.
    /* 0x17C */ bool  isCounterForce;
    /* 0x17D */ bool  useAbsoluteDirection;
    /* 0x17E */ char  _pad17E[2];

    /* 0x180 */ float startDelay;                // [INFERRED] float - a delay
    /* 0x184 */ bool  onlyWhenOccupied;          // [INFERRED] bool
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_FORCEOBJECTTEMPLATE_H
