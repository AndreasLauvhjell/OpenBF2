// Path: [INFERRED] IO/Console/Manager.cpp; see Manager.h for TU/path provenance.
#include "Manager.h"

namespace dice { namespace hfe { namespace console {

// Default-constructs the registry containers. This class has no virtual interface.
Manager::Manager() = default;

// ordinary reverse-order member destruction.
// Pointer values are non-owning here; there is no delete of Object/ObjectsInfo.
Manager::~Manager() = default;

// Maps a case-insensitive type key to a borrowed display-name pointer, replacing any prior value.
void Manager::registerNameForType(const char* type, const char* displayName)
{
    m_namesForTypes[std::string(type)] = displayName;
}

// ELF 0x8457230 / dedicated PE 0x679CE0: find without insertion, return null
// on a miss and the original pointer on a hit (not the map's key storage).
const char* Manager::getNameForType(const char* type) const
{
    const auto entry = m_namesForTypes.find(std::string(type));
    return entry == m_namesForTypes.end() ? nullptr : entry->second;
}

// Process-lifetime console manager, allocated during static initialization.
Manager* manager = new Manager;

} } }
