// Supply-depot configuration. Member names follow console properties. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_SUPPLYOBJECTTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_SUPPLYOBJECTTEMPLATE_H

#include "Base/BundleTemplate.h"

#include <vector>

namespace dice {
namespace hfe {
namespace world {

struct GameTemplateConsoleAccess;

// Inherits BundleTemplate's interface layout and reference counting.
class SupplyObjectTemplate : public BundleTemplate
{
public:
    SupplyObjectTemplate();

    virtual unsigned int getClassID() const;
    virtual ~SupplyObjectTemplate();
    virtual void* queryImplementation(unsigned int);
    virtual IObject* createObject() const;
    virtual bool  makeScript(io::IStream*);       // complete

    // Returns whether the object's player-control template has an allowed vehicle category.
    // An empty category list accepts every player-controlled vehicle; objects without that interface are rejected.
    bool testVehicleType(IObject* object) const;

protected:
    // The object half reads team, the four rates and the three storage sizes directly - the
    // ControlPoint/ControlPointTemplate arrangement. (Game/Objects/Game/SupplyObject.h)
    friend class SupplyObject;
    friend struct GameTemplateConsoleAccess;       // recovered ObjectTemplate.* binding executors

    // offsets are read; types are inferred unless marked otherwise
    // Every default below comes from the full ctor read, not from the retired compact pass.
    /* 0x17C */ float radius;                    // [INFERRED] float; ctor 2.0f (0x40000000)

    /* 0x180 */ int   team;                      // ctor 0

    /* 0x184 */ bool  workOnVehicles;            // ctor false
    /* 0x185 */ bool  workOnSoldiers;            // ctor true - a supply crate helps infantry unless
                                                 // content says otherwise; vehicles must opt in
    /* 0x186 */ char  _pad186[2];

    // The four *Speed fields are 4 bytes apart and are rates -> [INFERRED] float. All ctor 0.
    /* 0x188 */ float damageSpeed;
    /* 0x18C */ float healSpeed;
    /* 0x190 */ float refillAmmoSpeed;
    /* 0x194 */ float restoreSpeed;

    // Storage quantities use -1.0f as the unlimited sentinel.
    /* 0x198 */ float healStorageSize;           // ctor -1.0f (unlimited)
    /* 0x19C */ float ammoStorageSize;           // ctor -1.0f (unlimited)
    /* 0x1A0 */ float sharedStorageSize;         // ctor 0 - [INFERRED] float on the sibling
                                                 // argument; 0 is bit-identical either way

    // [UNRESOLVED] width the ctor writes this with a byte move
    // (`mov byte ptr [ebx+1A4h], 0`), so it is a bool and +0x1A5..+0x1A7 are padding.
    /* 0x1A4 */ bool  visibleOn3dMap;            // ctor false
    /* 0x1A5 */ char  _pad1A5[3];

    // The console name is "3dMapIcon"; use the literal when generating bindings.
    /* 0x1A8 */ int   mapIcon3d;                 // .con property: `3dMapIcon`; ctor -1 (no icon)
    /* 0x1AC */ bool  destroyWhenEmptyStorage;   // ctor false (byte store)
    /* 0x1AD */ char  _pad1AD[3];

    // Owned list of permitted vehicle types.
    /* 0x1B0 */ std::vector<int> vehicleTypes;   // .con method `addVehicleType`; ctor leaves it
                                                 // empty, which testVehicleType reads as
                                                 // "accept every vehicle"

    /* 0x1BC */ bool  teamFromClosestCP;         // ctor false (byte store)
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_SUPPLYOBJECTTEMPLATE_H
