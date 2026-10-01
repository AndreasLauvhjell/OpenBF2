// Console type and object registries using native containers. [INFERRED] Header placement.
#ifndef IO_CONSOLE_MANAGER_H
#define IO_CONSOLE_MANAGER_H

#include "../File/IFileSystem.h" // the existing NoCaseStringCompare
#include <map>
#include <string>
#include <vector>

namespace dice { namespace hfe {

// Attested comparator type. ELF 0x84581D0..0x845821D compares non-null strings
// with strcasecmp; when either is null it orders null before non-null.
struct NoCaseCharPtrCompare
{
    bool operator()(const char* a, const char* b) const
    {
        if (!a || !b) return !a && b;
#if defined(_MSC_VER)
        return _stricmp(a, b) < 0;
#else
        return strcasecmp(a, b) < 0;
#endif
    }
};

namespace console {
class Object;

class Manager
{
public:
    Manager();
    ~Manager();
    void registerNameForType(const char* type, const char* displayName);
    const char* getNameForType(const char* type) const;

private:
    // [UNRESOLVED] Object-info contents. Stored pointers are borrowed; destruction frees only the containers.
    struct ObjectsInfo;
    std::vector<ObjectsInfo*> m_objectsInfo;
    std::map<std::string, const char*, NoCaseStringCompare> m_namesForTypes;
    std::map<std::string, Object*, NoCaseStringCompare> m_objectProperties;
    std::map<const char*, std::vector<Object*>, NoCaseCharPtrCompare> m_objectsByType;
};

// Eager process-lifetime heap object in both original images, not a lazy/null slot.
extern Manager* manager;

} } }
#endif
