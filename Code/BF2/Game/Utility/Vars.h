// Persistent console-variable registry. Lookup is case-insensitive; list ordering
// is case-sensitive. getX registers missing values and returns a live handle;
// queryX only reads and returns a fallback when absent.
// [CHOSEN] Default arguments and some return types remain interpretations;
// getInt/getVec3 expose field addresses, set/setValue report 0/1.

#ifndef GAME_UTILITY_VARS_H     // [INFERRED] guard style
#define GAME_UTILITY_VARS_H

#include <string>

#include "../../Math/Vec3.h"     // dice::hfe::Math::Vec3 (typedef'd to dice::hfe::Vec3)

namespace dice {
namespace hfe {

// The console-variable store. Every method is static; the backing list is a file-scope singleton.
class Vars
{
public:
    // registering getters: create-if-missing, return a live handle into storage
    // The second argument is the default used only when the variable does not yet exist.
    static int*         getInt   (const char* name, int defaultValue = 0);
    static float*       getFloat (const char* name, float defaultValue = 0.0f);
    static Math::Vec3*  getVec3  (const char* name, const Math::Vec3& defaultValue);
    // defaultValue may be 0: then a missing variable is NOT created and "" is returned.
    static const char*  getString(const char* name, const char* defaultValue = 0);

    // read-only queries: never create; return a default when the variable is missing
    static int                queryInt   (const char* name);   //   (0    if missing)
    static float              queryFloat (const char* name);   //   (0.0f if missing)
    static const Math::Vec3&  queryVec3  (const char* name);   //   (Vec3::getZero())
    static const char*        queryString(const char* name);   //   (""   if missing)

    // Writes 'value' (raw text) into the variable, coercing to its current type. Creates the
    // variable as an untyped string if it does not exist. Returns whether it already existed.
    static bool  setValue(const char* name, const char* value);

    // std::string convenience wrappers over getString/setValue.
    static std::string  get(std::string name);
    static bool         set(std::string name, std::string value);

    // Sorted, newline-separated "name value" dump of every variable whose name begins with
    // 'filter' (case-insensitive prefix; empty filter -> all).
    static std::string  list(const std::string& filter);

    // Stores a profiling flag (file-scope). [UNRESOLVED] where it is consumed - never read here.
    static void  enableProfiling(bool enable);
};

} // namespace hfe
} // namespace dice

#endif // GAME_UTILITY_VARS_H
