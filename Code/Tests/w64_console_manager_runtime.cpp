// [TEMP] [CHOSEN, not recovered] Native console manager/type registration fixture.
// Links actual Manager and Math TUs plus source-extracted real module definitions.
// ModuleRegistry/Globals also compile whole; no collaborators are replaced.
#include "IO/Console/Manager.h"
#include "Core/Modules.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>

namespace {
int checks, failures;
void check(bool success, const char* label)
{
    ++checks;
    if (!success) { ++failures; std::printf("FAIL %s\n", label); }
}
}

int main()
{
    using namespace dice::hfe;
    using console::Manager;
    static_assert(!std::is_polymorphic<Manager>::value, "original manager has no vptr");
    check(console::manager != nullptr, "global manager is constructed before main");
    check(console::manager->getNameForType("std::string") == nullptr, "no invented constructor registrations");

    console::ModuleConsole module;
    check(module.init(), "actual ModuleConsole init");
    for (const char* spelling : {"std::string", "std::string&", "const std::string&", "CONST STD::STRING&"})
    {
        const char* name = console::manager->getNameForType(spelling);
        check(name && std::strcmp(name, "string") == 0, "three string aliases and case-insensitive lookup");
    }
    check(module.close() && console::manager->getNameForType("std::string") != nullptr,
          "module close preserves manager registrations");

    MathModule math;
    check(math.init(), "actual MathModule init");
    for (int dimension = 2; dimension <= 4; ++dimension)
    {
        const std::string base = "Vec" + std::to_string(dimension);
        const std::string display = "vec" + std::to_string(dimension);
        for (const std::string& spelling : {base, base + "&", "const " + base + "&"})
        {
            const char* name = console::manager->getNameForType(spelling.c_str());
            check(name && name == console::manager->getNameForType(base.c_str()) && name == display,
                  "nine actual math aliases preserve display pointer and spelling");
        }
    }
    check(math.close(), "actual MathModule close");

    char key[] = "OwnedKey";
    char firstName[] = "first", secondName[] = "second";
#if defined(_WIN64)
    check(reinterpret_cast<std::uintptr_t>(firstName) > UINT32_MAX,
          "native pointer identity check actually crosses 32-bit range");
#endif
    {
        Manager local;
        local.registerNameForType(key, firstName);
        key[0] = 'X';
        check(local.getNameForType("ownedkey") == firstName, "key copy and borrowed display pointer");
        firstName[0] = 'F';
        check(local.getNameForType("OWNEDKEY")[0] == 'F', "display value is not copied");
        local.registerNameForType("oWnEdKeY", secondName);
        check(local.getNameForType("OwnedKey") == secondName, "case-equivalent registration replaces");
        check(local.getNameForType("XwnedKey") == nullptr, "key does not follow caller buffer");
        check(console::manager->getNameForType("OwnedKey") == nullptr, "manager instances remain independent");
        local.registerNameForType("", firstName);
        check(local.getNameForType("") == firstName, "empty key is accepted");
        local.registerNameForType("null-value", nullptr);
        check(local.getNameForType("NULL-VALUE") == nullptr, "null display pointer is retained");
        local.registerNameForType("NULL-VALUE", secondName);
        check(local.getNameForType("null-value") == secondName, "null display can be replaced");
        const std::string longKey(4096, 'k');
        local.registerNameForType(longKey.c_str(), firstName);
        check(local.getNameForType(std::string(4096, 'K').c_str()) == firstName, "long key uses real string storage");
    }
    check(firstName[0] == 'F' && secondName[0] == 's', "destruction does not delete borrowed display pointers");
    NoCaseCharPtrCompare compare;
    check(compare(nullptr, "x") && !compare("x", nullptr) && !compare(nullptr, nullptr), "attested null ordering");
    check(!compare("Type", "TYPE") && compare("a", "B") && !compare("B", "a"), "attested character comparator");
    std::printf("Console manager runtime: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
