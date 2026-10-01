// Path: [INFERRED] beside VehicleHudCompTemplate.h.
#include "VehicleHudIcons.h"

namespace dice { namespace hfe { namespace world {

// and BF2.exe 0x65B020 default-construct exactly four vectors and
// three strings. They do not write the three scalar handles. The enclosing
// VehicleHudCompTemplate constructor owns that initialization (ELF 0x822A2A0;
// ). Explicit member initializers preserve that distinction.
VehicleHudIcons::VehicleHudIcons()
    : vehicleIcons(), abilityIcons(), miniMapIcon(), typeIcon(), spottedIcon(),
      vehicleIconPtrs(), abilityIconPtrs()
{
}

} } }
