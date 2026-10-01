// Targetable-object configuration for lock-on weapons and AI. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_TARGETOBJECTTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_TARGETOBJECTTEMPLATE_H

#include "Base/BundleTemplate.h"

namespace dice {
namespace hfe {
namespace world {

class ArtilleryHandler;
struct GameTemplateConsoleAccess;

// Time to live is stored in seconds and as a derived 30 Hz tick count.
class TargetObjectTemplate : public BundleTemplate
{
public:
    TargetObjectTemplate();

    virtual unsigned int getClassID() const;
    virtual ~TargetObjectTemplate();
    virtual void* queryImplementation(unsigned int);
    virtual IObject* createObject() const;
    virtual bool  makeScript(io::IStream*);       // complete

    // NON-virtual (0x81C0F70, 62 B - three call xrefs, no vtable slot): the two ctors and one
    // generated console binding, ConsoleClass1165::executeObjectMethod at 0x8647EC5. So this is
    // the `.con` setter for the time-to-live property as well as the ctor's own initialiser.
    void setTimeToLive(float seconds);

protected:
    // The object half reads targetType and the timeToLive pair directly - the SupplyObject
    // arrangement. (Game/Objects/Game/TargetObject.h)
    friend class TargetObject;
    friend class ArtilleryHandler;                 // constructor selects local/networked targetType
    friend struct GameTemplateConsoleAccess;       // recovered ObjectTemplate.* binding executors

    /* 0x17C */ int   targetType;                     // [INFERRED] int - an enum/type id; ctor 2

    // Lifetime in seconds and cached 30 Hz ticks: truncate(seconds * 30) + 1.
    /* 0x180 */ float timeToLive;                     // ctor 5.0f, via setTimeToLive
    /* 0x184 */ int   timeToLiveTicks;                // = int(timeToLive * 30.0f) + 1  => 151

    // Adjacent single bytes -> both bools, +0x18A/+0x18B padding. The ctor confirms it: both are
    // written with byte moves.
    /* 0x188 */ bool  disableWhenWreck;                // ctor false
    /* 0x189 */ bool  disableWhenEmptyVehicle;         // ctor false
    /* 0x18A */ char  _pad18A[2];

    /* 0x18C */ float disableWhenEmptyVehicleDelay;   // [INFERRED] float - a delay; ctor 0
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_TARGETOBJECTTEMPLATE_H
