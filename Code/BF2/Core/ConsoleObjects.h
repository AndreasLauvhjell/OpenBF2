// Engine-wide registry used to resolve console commands and properties. Header path [INFERRED].
// Modules register their binding lists here. ConsoleObjects is nonpolymorphic; the stored
// ConsoleObject entries are polymorphic and have a separate interface.

#ifndef CORE_CONSOLEOBJECTS_H     // [INFERRED] guard style
#define CORE_CONSOLEOBJECTS_H

#include <list>
#include <map>
#include <string>

#include "ConsoleObject.h"
#include "Hash.h"                       // Hash<>, StringHashFunc, calcStringHashValueNoCase
#include "../IO/File/IFileSystem.h"     // dice::hfe::NoCaseStringCompare - the map's comparator

namespace dice {
namespace hfe {

class ConsoleObjects
{
public:
    // Bindings retain insertion order for autocomplete alongside the lookup multimap.
    // The rebuild uses the default list allocator; retail used DllStlAllocator for DLL heap ownership.
    // Private layout therefore differs, while this source interface exposes the same operations.
    typedef std::list<ConsoleObject*> ConsoleObjectList;

    // Constructs an empty conversion hash, duplicate-name map and insertion-order list.
    ConsoleObjects();


    // both return bool, not void. Each ends in `mov eax, 1`. The three local shims this header
    // replaces declared them `void`; that was a guess, and it was wrong. No caller in the engine
    // tests the result, so nothing observable changes - but the signature is public surface and is
    // now correct.
    bool registerConsoleObjects(ConsoleObjectList& objects);
    bool unregisterConsoleObjects(ConsoleObjectList& objects);

    // - `lea eax, [this+24]; ret`, i.e. it hands back the member list
    // itself. [CHOSEN] as a const reference rather than a pointer; the two compile identically and
    // Game/Main/BF2Console.cpp's shim had already chosen the reference.
    const ConsoleObjectList& getConsoleObjects() const;

    // getConsoleObject - This is the .con engine's
    // entry point; IO/Console/Console.cpp:730 calls it as
    //     mainConsoleObjects.getConsoleObject(objectName, methodName)
    // See the implementation for the get/set prefix fallback and the polarity contradiction it
    // exposes.
    ConsoleObject* getConsoleObject(const std::string& objectName,
                                    const std::string& methodName);


    // A rename table for the type names shown in console help: Core/ModuleRegistry.cpp:1015
    // registers "string" -> "std::string", and :1218 loops a whole table of them.
    bool        registerTypeConversion(const std::string& from, const std::string& to);
    std::string typeAsString(const std::string& type) const;

private:
    // lower_bound/upper_bound over the equal range,
    // returning the first active binding - see the implementation.
    ConsoleObject* findInMultiMap(const std::string& key);

    // The original x86 ELF layout is 0x20 bytes; current STL layout may differ.
    // The name map allows duplicates. Conversion hashing ignores case but equality remains case-sensitive.

    Hash<std::string, std::string, StringHashFunc>                            m_typeConversions;
    std::multimap<std::string, ConsoleObject*, NoCaseStringCompare>           m_consoleObjectMap;
    ConsoleObjectList                                                         m_consoleObjects;
};

// Points to the engine's statically constructed console registry.
// The singleton row registers its existing address with flag 0x4; it does not construct the object.
// Side DLLs obtain the host instance through their initialization handshake.
extern ConsoleObjects* mainConsoleObjects;

} // namespace hfe
} // namespace dice

#endif // CORE_CONSOLEOBJECTS_H
