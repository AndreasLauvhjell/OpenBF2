// Persistent configuration values with case-insensitive lookup and case-sensitive listing.
// New values enter at the front as untyped strings; their first typed access fixes the type.

#include "Vars.h"

#include <cstdio>       // sprintf, sscanf
#include <cstdlib>      // atoi, atof, free
#include <cstring>      // strlen, strdup
#include <list>

#include "../../Core/Debug.h"        // dice::hfe::Debug, DEBUGTYPE_6
#include "../../Core/StringUtil.h"   // dice::hfe::compareIgnoreCase (2-arg and 3-arg)

namespace dice {
namespace hfe {

namespace {

// [INFERRED] name. The type tag stored at ConfigValue+0. Values proven by CastTo/GetValue/SetValue.
enum ConfigType
{
    CONFIG_INT    = 0,   // scalar via atoi / "%d"
    CONFIG_FLOAT  = 1,   // scalar via atof / "%f"
    CONFIG_VEC3   = 2,   // myVec3 via sscanf/sprintf "%f/%f/%f"
    CONFIG_STRING = 3    // raw text, not yet coerced (the born state)
};

// File-local value record. sizeof == 0x1C in the ELF (libstdc++ 4-byte std::string). Field offsets
// proven from disassembly. myVec3 and the scalar do NOT overlap; only myInt/myFloat share +0x14.
struct ConfigValue
{
    ConfigType   myType;      // +0x00
    std::string  myName;      // +0x04
    Math::Vec3   myVec3;      // +0x08 (x,y,z at +0x08/+0x0C/+0x10)
    union {                   // +0x14
        int      myInt;
        float    myFloat;
    };
    std::string  myValue;     // +0x18  (serialised / raw text)

    ConfigValue() : myType(CONFIG_STRING), myInt(0) {}   // [INFERRED] - inlined at each call site

    // serialise the live value into myValue and return it.
    std::string& GetValue()
    {
        char s[76];
        if ( myType == CONFIG_VEC3 )       { sprintf(s, "%f/%f/%f", myVec3.x, myVec3.y, myVec3.z); myValue.assign(s, strlen(s)); }
        else if ( myType == CONFIG_FLOAT ) { sprintf(s, "%f", myFloat); myValue.assign(s, strlen(s)); }
        else if ( myType == CONFIG_INT )   { sprintf(s, "%d", myInt);   myValue.assign(s, strlen(s)); }
        // CONFIG_STRING: leave myValue untouched
        return myValue;
    }

    // assign 's' (if non-null) into myValue, then parse into the current type's field.
    // Returns false only when a Vec3 fails to read three floats; true otherwise.
    bool SetValue(const char* s)
    {
        if ( s )
            myValue.assign(s, strlen(s));
        if ( myType == CONFIG_VEC3 )
        {
            if ( sscanf(myValue.c_str(), "%f/%f/%f", &myVec3.x, &myVec3.y, &myVec3.z) != 3 )
                return false;
        }
        else if ( myType == CONFIG_FLOAT )
        {
            myFloat = (float)atof(myValue.c_str());
        }
        else if ( myType == CONFIG_INT )
        {
            myInt = atoi(myValue.c_str());
        }
        return true;
    }

    // coerce to 'requested'. An untyped(String) value morphs and re-parses; a value
    // already of a different concrete type reports failure (caller warns but keeps the storage).
    bool CastTo(ConfigType requested)
    {
        if ( myType != CONFIG_STRING )
            return requested == myType;
        myType = requested;
        SetValue(0);            // re-parse the stored text (return intentionally ignored)
        return true;
    }
};

// strict-weak ordering by name (case-sensitive), used by list::sort in DumpConfigValues.
inline bool operator<(const ConfigValue& a, const ConfigValue& b)
{
    return a.myName < b.myName;
}

typedef std::list<ConfigValue> ConfigValueList;

// The leaked singleton store. Inlined into every Vars method (identical guarded 'new' prologue).
inline ConfigValueList& configValues()
{
    static ConfigValueList* values = new ConfigValueList;   // guard at 0x8AACC65
    return *values;
}

// Profiling flag (0x8AACC64 byte 0). Written by Vars::enableProfiling, never read in this TU.
bool g_profilingEnabled = false;   // [UNRESOLVED] consumer

// first case-insensitive name match, or end() on miss.
ConfigValueList::iterator FindConfigValue(const char* name)
{
    ConfigValueList& cfg = configValues();
    for ( ConfigValueList::iterator it = cfg.begin(); it != cfg.end(); ++it )
    {
        if ( compareIgnoreCase(name, it->myName.c_str()) == 0 )
            return it;
    }
    return cfg.end();
}

// sort by name, then "name value\n" for each whose name matches 'filter' as a
// case-insensitive prefix (length filter.size()). Empty filter matches all. Caller free()s.
char* DumpConfigValues(const std::string& filter)
{
    ConfigValueList& cfg = configValues();
    std::string out;
    cfg.sort();
    for ( ConfigValueList::iterator it = cfg.begin(); it != cfg.end(); ++it )
    {
        if ( compareIgnoreCase(filter.c_str(), it->myName.c_str(), (int)filter.length()) == 0 )
        {
            out += it->myName;
            out += " ";
            out += it->GetValue();
            out += "\n";
        }
    }
    return strdup(out.c_str());
}

} // anonymous namespace

// returns int* (live handle). Debug line 203 (attested 0x814B861).
int* Vars::getInt(const char* name, int defaultValue)
{
    ConfigValueList& cfg = configValues();
    ConfigValueList::iterator it = FindConfigValue(name);
    if ( it == cfg.end() )
    {
        ConfigValue cv;
        cv.myType = CONFIG_INT;
        cv.myName = name;
        cv.myInt  = defaultValue;
        cfg.insert(cfg.begin(), cv);          // push_front
        return &cfg.front().myInt;
    }
    if ( !it->CastTo(CONFIG_INT) )
    {
        Debug(DEBUGTYPE_6, "Game/Utility/Vars.cpp", 203)
            << "Failed to cast config value " << name << " to Int";
    }
    return &it->myInt;
}

// returns float* (proven by demo.cpp). Debug line 232 (attested 0x814BB1B).
float* Vars::getFloat(const char* name, float defaultValue)
{
    ConfigValueList& cfg = configValues();
    ConfigValueList::iterator it = FindConfigValue(name);
    if ( it == cfg.end() )
    {
        ConfigValue cv;
        cv.myType  = CONFIG_FLOAT;
        cv.myName  = name;
        cv.myFloat = defaultValue;
        cfg.insert(cfg.begin(), cv);
        return &cfg.front().myFloat;
    }
    if ( !it->CastTo(CONFIG_FLOAT) )
    {
        Debug(DEBUGTYPE_6, "Game/Utility/Vars.cpp", 232)
            << "Failed to cast config value " << name << " to Float";
    }
    return &it->myFloat;
}

// returns Vec3* [CHOSEN]. Debug line 261 (attested 0x814BDDB).
Math::Vec3* Vars::getVec3(const char* name, const Math::Vec3& defaultValue)
{
    ConfigValueList& cfg = configValues();
    ConfigValueList::iterator it = FindConfigValue(name);
    if ( it == cfg.end() )
    {
        ConfigValue cv;
        cv.myType   = CONFIG_VEC3;
        cv.myName   = name;
        cv.myVec3.x = defaultValue.x;
        cv.myVec3.y = defaultValue.y;
        cv.myVec3.z = defaultValue.z;
        cfg.insert(cfg.begin(), cv);
        return &cfg.front().myVec3;
    }
    if ( !it->CastTo(CONFIG_VEC3) )
    {
        Debug(DEBUGTYPE_6, "Game/Utility/Vars.cpp", 261)
            << "Failed to cast config value " << name << " to Vec3";
    }
    return &it->myVec3;
}

// returns const char*. defaultValue==0 -> no create, return "". Debug line 302
// (attested 0x814C1BA) is present but unreachable (fresh type-3 SetValue never fails).
const char* Vars::getString(const char* name, const char* defaultValue)
{
    ConfigValueList& cfg = configValues();
    ConfigValueList::iterator it = FindConfigValue(name);
    if ( it == cfg.end() )
    {
        if ( defaultValue )
        {
            ConfigValue cv;
            cv.myType = CONFIG_STRING;
            cv.myName = name;
            if ( !cv.SetValue(defaultValue) )
            {
                Debug(DEBUGTYPE_6, "Game/Utility/Vars.cpp", 302)
                    << "Invalid default value " << defaultValue << " for parameter " << name;
            }
            cfg.insert(cfg.begin(), cv);
            return cfg.front().GetValue().c_str();
        }
        return "";
    }
    return it->GetValue().c_str();
}

// returns int by value, 0 if missing. Debug line 331.
int Vars::queryInt(const char* name)
{
    ConfigValueList& cfg = configValues();
    ConfigValueList::iterator it = FindConfigValue(name);
    if ( it == cfg.end() )
        return 0;
    if ( !it->CastTo(CONFIG_INT) )
    {
        Debug(DEBUGTYPE_6, "Game/Utility/Vars.cpp", 331)
            << "Failed to cast config value " << name << " to Int";
    }
    return it->myInt;
}

// returns float by value, 0.0f if missing. Debug line 357.
float Vars::queryFloat(const char* name)
{
    ConfigValueList& cfg = configValues();
    ConfigValueList::iterator it = FindConfigValue(name);
    if ( it == cfg.end() )
        return 0.0f;
    if ( !it->CastTo(CONFIG_FLOAT) )
    {
        Debug(DEBUGTYPE_6, "Game/Utility/Vars.cpp", 357)
            << "Failed to cast config value " << name << " to Float";
    }
    return it->myFloat;
}

// returns const Vec3& (&field on hit, shared getZero() on miss). Debug line 380.
const Math::Vec3& Vars::queryVec3(const char* name)
{
    ConfigValueList& cfg = configValues();
    ConfigValueList::iterator it = FindConfigValue(name);
    if ( it == cfg.end() )
        return Math::Vec3::getZero();
    if ( !it->CastTo(CONFIG_VEC3) )
    {
        Debug(DEBUGTYPE_6, "Game/Utility/Vars.cpp", 380)
            << "Failed to cast config value " << name << " to Vec3";
    }
    return it->myVec3;
}

// returns const char*, "" if missing. No coercion, no warning.
const char* Vars::queryString(const char* name)
{
    ConfigValueList& cfg = configValues();
    ConfigValueList::iterator it = FindConfigValue(name);
    if ( it == cfg.end() )
        return "";
    return it->GetValue().c_str();
}

// returns bool: true if the variable already existed (coerced in place); false if it
// was created (delegated to getString, which makes it an untyped string with value 'value').
bool Vars::setValue(const char* name, const char* value)
{
    ConfigValueList& cfg = configValues();
    ConfigValueList::iterator it = FindConfigValue(name);
    if ( it == cfg.end() )
    {
        getString(name, value);
        return false;
    }
    it->SetValue(value);
    return true;
}

// returns std::string. get-or-create with "" default (side-effect: creates if missing).
std::string Vars::get(std::string name)
{
    return std::string( getString(name.c_str(), "") );
}

// returns bool (forwards setValue).
bool Vars::set(std::string name, std::string value)
{
    return setValue(name.c_str(), value.c_str());
}

// returns std::string dump. Copies the strdup'd buffer and frees it.
std::string Vars::list(const std::string& filter)
{
    char* dump = DumpConfigValues(filter);
    std::string result(dump);
    if ( dump )
        free(dump);
    return result;
}

// stores the flag. returns void [CHOSEN].
void Vars::enableProfiling(bool enable)
{
    g_profilingEnabled = enable;
}

} // namespace hfe
} // namespace dice
