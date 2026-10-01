// Header name [INFERRED]; Core/Registry.cpp is attested by the ELF anonymous namespace.
#ifndef CORE_REGISTRY_H
#define CORE_REGISTRY_H

#include <string>

namespace dice { namespace hfe {

// Type attested by Registry::getString's ELF mangling; enumerator names [INFERRED].
// PE 0x450E00 selects HKLM for zero and HKCU for every nonzero value.
enum RegKeyCategory { RegKeyLocalMachine = 0, RegKeyCurrentUser = 1 };

class Registry
{
public:
    Registry();
    ~Registry();
    std::string getString(RegKeyCategory category, const std::string& name,
                          const std::string& defaultValue, const std::string& modName) const;
};

extern Registry* registry; // ELF 0x898BFB4; client PE 0x96ED6C

} }
#endif
