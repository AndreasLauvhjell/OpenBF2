// Settings script loading and saving. [INFERRED] Header placement.
// Owns registered names, save groups and defaults; SettingsRepostitory owns values.
// [UNRESOLVED] Purpose of the six paired string sets.

#ifndef IO_SETTINGSMANAGER_H        // [INFERRED] guard style
#define IO_SETTINGSMANAGER_H

#include <map>
#include <set>
#include <string>

#include "../Math/Vec3.h"          // dice::hfe::Vec3 - the +0x84 map's value type

namespace dice {
namespace hfe {
namespace io {

// [CHOSEN] Four-byte enum representation for the ordered, trivially copied save-set key.
// The original type's enumerators and enum-versus-wrapper distinction remain [UNRESOLVED].
enum VariableSet { VariableSet_UNRECOVERED = 0 };

class SettingsManagerBase
{
public:
    SettingsManagerBase();

    // [CHOSEN] bool return on load() - BF2Engine::initEngine discards the result, so nothing
    // settles it. The 757-byte body plainly can fail.
    bool load(const std::string& file);

    // Saves the selected variable set. Returns false if the file cannot be opened or written.
    bool save(const std::string& file, VariableSet);

    bool registerUserVar(const std::string&, bool, VariableSet);
    // ten bytes. registerDevVar is a stub in this build while registerUserVar is 1,496 - dev
    // variables are compiled out of the shipping server. Worth knowing before assuming symmetry.
    bool registerDevVar(const std::string&, bool, VariableSet);

    // Returns a formatted listing of registered setting names with known types.
    std::string listKeys();

// [CHOSEN] Public access lets getType inspect the singleton's variable registry.
public:
    // The six paired sets: allocated, destroyed, and never touched by this TU. [UNRESOLVED]
    // The six maps: default values per variable name, read by save().
    /* +0x00 */ std::set<std::string>                        m_intUnknown;
    /* +0x0C */ std::map<std::string, int>                   m_intDefaults;
    /* +0x18 */ std::set<std::string>                        m_floatUnknown;
    /* +0x24 */ std::map<std::string, float>                 m_floatDefaults;
    /* +0x30 */ std::set<std::string>                        m_U32Unknown;
    /* +0x3C */ std::map<std::string, unsigned int>          m_U32Defaults;
    /* +0x48 */ std::set<std::string>                        m_boolUnknown;
    /* +0x54 */ std::map<std::string, bool>                  m_boolDefaults;
    /* +0x60 */ std::set<std::string>                        m_stringUnknown;
    /* +0x6C */ std::map<std::string, std::string>           m_stringDefaults;
    /* +0x78 */ std::set<std::string>                        m_Vec3Unknown;
    /* +0x84 */ std::map<std::string, Vec3>                  m_Vec3Defaults;

    // The registry proper
    /* +0x90 */ std::set<std::string>                        m_registeredVars;
    /* +0x9C */ std::map<VariableSet, std::set<std::string> > m_variableSets;
};

// Returns the registered variable's console type name, or an empty string when no stored type is found.
std::string getType(const std::string& name);

// Singleton owned and initialized by SettingsManager.cpp.
extern SettingsManagerBase* settingsManager;    // ds:0x8AC380C, name attested

void initSettingsManager();
void shutdownSettingsManager();

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_SETTINGSMANAGER_H
