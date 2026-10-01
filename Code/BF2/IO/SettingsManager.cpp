// Loads settings by executing console script at access level 1, then restores the previous level.
// Saving emits SettingsManager.<type>Set commands for the same interpreter. Preserve console
// registration and access semantics; this layer does not use a separate settings grammar.

#include "SettingsManager.h"
#include "File/File.h"                // io::File - save() builds one on the stack

#include "Console/Console.h"
#include "../Core/Debug.h"                  // dice::hfe::Debug, DEBUGTYPE_6 - registerUserVar
#include "../Core/SettingsRepostitory.h"    // dice::hfe::settings - save() and getType()
#include "../Core/StringUtil.h"             // dice::hfe::formatString - save() and listKeys()


namespace dice {
namespace hfe {
namespace io {

// Shared output sink for settings scripts; load does not clear it. [UNRESOLVED] Any external consumer.
static std::string g_consoleOutput;

// Constructed during static initialization; the explicit initialization hook is intentionally empty.
SettingsManagerBase* settingsManager = new SettingsManagerBase();

// No initialization work; static initialization has already created the singleton.
void initSettingsManager()
{
}

// Initializes the setting containers. This class has no virtual interface.
SettingsManagerBase::SettingsManagerBase()
{
    // The fourteen members default-construct. That is the binary's entire body.
}


// Executes the settings file at access level one and restores the previous access level.
// Returns the console script's result.
bool SettingsManagerBase::load(const std::string& file)
{
    const int savedAccess = mainConsole->getAccess();
    mainConsole->setAccess(1);                       // see the header block

    // Settings files run without command arguments.
    const std::string a1, a2, a3, a4, a5, a6, a7, a8;

    const bool ok = mainConsole->run(file, a1, a2, a3, a4, a5, a6, a7, a8, g_consoleOutput);

    mainConsole->setAccess(savedAccess);
    return ok;
}

// Writes registered variables in set as console commands to file.
// Returns false for an unknown set or when the file cannot be opened.
// Vec3 values and names without a repository value are omitted.
bool SettingsManagerBase::save(const std::string& file, VariableSet set)
{
    File f;

    // Reject unknown sets before opening the output file.
    std::map<VariableSet, std::set<std::string> >::iterator group = m_variableSets.find(set);
    if (group == m_variableSets.end())
        return false;

    // [UNRESOLVED] Original enumerator names for the mode and access constants.
    if (!f.open(file, FileMode(0), FileAccess(2), 0))
        return false;

    f.write(formatString("rem\nrem Settingsfile automatically saved by bf2\nrem\n"));

    for (std::set<std::string>::iterator i = m_registeredVars.begin();
         i != m_registeredVars.end();
         ++i)
    {
        const std::string& name = *i;

        if (group->second.find(name) == group->second.end())
            continue;                                                // not in the requested set

        if (settings->floatExists(name))
        {
            float def = -1.0f;                                       // 0xBF800000 at 0x8436C2C
            std::map<std::string, float>::iterator d = m_floatDefaults.find(name);
            if (d != m_floatDefaults.end())
                def = d->second;
            f.write(formatString("SettingsManager.floatSet %s %f\n",
                                 name.c_str(), settings->floatGet(name, def)));
        }
        else if (settings->intExists(name))
        {
            int def = -1;
            std::map<std::string, int>::iterator d = m_intDefaults.find(name);
            if (d != m_intDefaults.end())
                def = d->second;
            f.write(formatString("SettingsManager.intSet %s %d\n",
                                 name.c_str(), settings->intGet(name, def)));
        }
        else if (settings->U32Exists(name))
        {
            unsigned int def = 0;
            std::map<std::string, unsigned int>::iterator d = m_U32Defaults.find(name);
            if (d != m_U32Defaults.end())
                def = d->second;
            // %d, not %u - dice's own format string, printing an unsigned through a signed
            // conversion. Preserved.
            f.write(formatString("SettingsManager.U32Set %s %d\n",
                                 name.c_str(), settings->U32Get(name, def)));
        }
        else if (settings->stringExists(name))
        {
            std::string def("UnSet");
            std::map<std::string, std::string>::iterator d = m_stringDefaults.find(name);
            if (d != m_stringDefaults.end())
                def = d->second;
            f.write(formatString("SettingsManager.stringSet %s \"%s\"\n",
                                 name.c_str(), settings->stringGet(name, def).c_str()));
        }
        else if (settings->boolExists(name))
        {
            bool def = false;
            std::map<std::string, bool>::iterator d = m_boolDefaults.find(name);
            if (d != m_boolDefaults.end())
                def = d->second;
            f.write(formatString("SettingsManager.boolSet %s %d\n",
                                 name.c_str(), settings->boolGet(name, def)));
        }
    }

    f.close();
    return true;
}

// Registers a user variable and, when save is true, includes it in the requested save set.
// Duplicate registration emits a fatal diagnostic.
bool SettingsManagerBase::registerUserVar(const std::string& name, bool save, VariableSet set)
{
    if (m_registeredVars.find(name) != m_registeredVars.end())
    {
        Debug(DEBUGTYPE_6, "IO/SettingsManager.cpp", 115)
            << "Var " << name << " is already registred";            // [sic]
        return false;
    }

    m_registeredVars.insert(name);

    if (save)
    {
        if (m_variableSets.find(set) == m_variableSets.end())
            m_variableSets.insert(std::make_pair(set, std::set<std::string>()));

        m_variableSets.find(set)->second.insert(name);
    }

    return true;
}

// Returns true without registering developer variables, matching this shipped build.
bool SettingsManagerBase::registerDevVar(const std::string&, bool, VariableSet)
{
    return true;
}

// Returns the first registered type found in float, int, U32, string, Vec3, bool order.
// Uses the global settings manager and returns an empty string when no type is found.
std::string getType(const std::string& name)
{
    if (settingsManager->m_registeredVars.find(name) != settingsManager->m_registeredVars.end()
        && settings->floatExists(name))
        return "float";

    if (settingsManager->m_registeredVars.find(name) != settingsManager->m_registeredVars.end()
        && settings->intExists(name))
        return "int";

    if (settingsManager->m_registeredVars.find(name) != settingsManager->m_registeredVars.end()
        && settings->U32Exists(name))
        return "U32";

    if (settingsManager->m_registeredVars.find(name) != settingsManager->m_registeredVars.end()
        && settings->stringExists(name))
        return "string";

    if (settingsManager->m_registeredVars.find(name) != settingsManager->m_registeredVars.end()
        && settings->Vec3Exists(name))
        return "Vec3";

    if (settingsManager->m_registeredVars.find(name) != settingsManager->m_registeredVars.end()
        && settings->boolExists(name))
        return "bool";

    return "";
}

// Returns a formatted listing of registered variables with known types.
std::string SettingsManagerBase::listKeys()
{
    std::string out;
    std::string type;

    out += formatString("Registered SettingsManagerVariables\n");

    for (std::set<std::string>::iterator i = m_registeredVars.begin();
         i != m_registeredVars.end();
         ++i)
    {
        type = getType(*i);
        if (!type.empty())
            out += formatString("%s %s\n", i->c_str(), type.c_str());
    }

    return out;
}

// Destroys the settings singleton. The global pointer is left dangling; do not call twice.
void shutdownSettingsManager()
{
    if (!settingsManager)
        return;

    // The binary tears the fourteen members down explicitly, in reverse declaration order, then
    // frees the object. Expressed here as the destructor the compiler would have generated.
    delete settingsManager;

    // not nulled in the original - see above.
}

} // namespace io
} // namespace hfe
} // namespace dice
