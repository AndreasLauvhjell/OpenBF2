// Path: attested by Core/ClassManager.cpp diagnostics; contents recovered from the symbolized ELF.
// Class and singleton registration, construction, lookup and release.

#include "ClassManager.h"
#include "Ids.h"                          // [INFERRED] header name; the constants are attested
#include "Debug.h"                        // Debug() - used at lines 46 and 53
#include "StringUtil.h"                   // formatString() - used at line 108
#include "Ids.h"                          // IID_IBase / IID_IClassManager - queryInterface
#include <map>                            // ClassInfo / SingletonInfo storage
#include <algorithm>                      // std::find - unregisterServer
#include <sstream>                        // std::stringstream - getPrintableSingletonList

namespace dice {
namespace hfe {

// Starts with one reference and leaves singletons uninitialized.
ClassManager::ClassManager()
    : m_refCount(1)             // +0x04
    , m_singletonsInit(false)   // +0x38, 0x83B9F51 - latched true later by initSingletons()
{
    // m_classServerTree, m_defaultClasses, m_classInfos, m_normalSingletons and m_simpleSingletons
    // default-construct; that is the rest of the binary's body - four heap-allocated _Rb_tree
    // header nodes and one three-null vector, in declaration order.
}

// Destroys the registry containers. Registered singletons must be unregistered separately before destruction.
ClassManager::~ClassManager()
{
}

// registerServer - the central routine of the class system.

// A module hands over two lists and ClassManager absorbs both: every class it can construct, and
// every named singleton it wants created. Both lists are built by the module on demand rather than
// held statically, which is why they are vectors passed by reference and destroyed on the way out.
void ClassManager::registerServer(IClassServer* server)
{
    addServer(server);

    // 1. absorb the class list
    std::vector<ClassRegInfo> classes;
    server->getClassList(classes);               // IClassServer vtable +0x10

    for (std::vector<ClassRegInfo>::iterator c = classes.begin(); c != classes.end(); ++c)
    {
        ClassInfo info;
        info.create  = c->create;
        info.classId = c->classId;
        info.name    = c->name;
        info.server  = server;

        // Duplicate registrations are reported and appended. Lookups select the first matching registration.
        ClassInfo existing;
        if (getClassInfo(c->classId, existing))
        {
            Debug(DEBUGTYPE_6, "Core/ClassManager.cpp", 122, __FUNCTION__)
                << "Duplicate registration of class id " << c->classId
                << ". Already used by:" << existing.name
                << " tried to set new name:" << c->name;
        }
        if (getClassInfo(c->name, existing))
        {
            Debug(DEBUGTYPE_6, "Core/ClassManager.cpp", 126, __FUNCTION__)
                << "Duplicate registration of class name \"" << c->name << "\""
                << " exists already as:" << existing.name;
        }

        m_classInfos.push_back(info);
    }

    // 2. absorb the singleton list
    std::vector<SingletonRegInfo> singletons;
    server->getSingletonList(singletons);        // IClassServer vtable +0x14

    for (std::vector<SingletonRegInfo>::iterator s = singletons.begin(); s != singletons.end(); ++s)
        registerSingleton(*s);                   // virtual, own vtable +0x44
}

// initSingletons -

// Walks the named-singleton map and instantiates everything that asked to be created at init time.
// Note the bit: this tests flags & 1. registerSingleton tests flags & 2 to decide whether to create
// immediately at registration instead. They are different bits and different moments.
void ClassManager::initSingletons()
{
    for (std::map<std::string, SingletonInfo>::iterator it = m_normalSingletons.begin();
         it != m_normalSingletons.end(); ++it)
    {
        if (it->second.flags & 1)
            createSingleton(it->first, it->second);
    }
    m_singletonsInit = true;
}

// createSingleton -

// Replaces whatever the global currently holds. Note the double indirection: SingletonInfo::instance
// is the address of the global pointer variable, so this writes through it and every holder of that
// global sees the new object.
void ClassManager::createSingleton(const std::string& name, SingletonInfo& info)
{
    IBase* previous = *info.instance;
    if (previous)
        previous->release();                     // IBase vtable +0x08

    *info.instance = createInstance(info.classId, info.iid, 0);   // own vtable +0x34

    // Builds the class name or fallback text; the resulting string has no consumer in this implementation.
    std::string className;
    if (!getClassName(info.classId, className))
        className = formatString("Unknown ID %d", info.classId);
}


// Creates class cid through its registered factory, requesting iid and passing outer for aggregation.
// Returns null when the class is not registered; otherwise returns the factory result.
IBase* ClassManager::createInstance(unsigned int cid, unsigned int iid, IBase* outer)
{
    ClassInfo info;
    if (!getClassInfo(cid, info))
        return 0;
    return info.create(iid, outer);
}

// Creates the default implementation of iid, returning null when no default class is registered.
// The first argument is an interface ID, not a class ID.
IBase* ClassManager::createInstance(unsigned int iid, IBase* outer)
{
    unsigned int cid;
    if (!getDefaultClass(iid, cid))
        return 0;
    return createInstance(cid, iid, outer);
}

// Creates an instance from a by-value class name by looking up ClassInfo and invoking its factory.
// Does not delegate through getClassID; preserve the independent lookup and factory call.
IBase* ClassManager::createInstance(std::string name, unsigned int iid, IBase* outer)
{
    ClassInfo info;
    if (!getClassInfo(name, info))
        return 0;
    return info.create(iid, outer);
}

// SingletonInfo stores instance, iid, classId, flags; createInstance receives classId before iid.
// createSingleton is not idempotent: an existing singleton is released and replaced.

// Registers a normal singleton and constructs it immediately when flag 0x2 is set.
// Flag 0x1 defers construction to initSingletons; flag 0x4 denotes an existing instance.
// Unknown class IDs use an "Unknown ID" display name and still attempt construction.
// This updates m_normalSingletons only; simple singletons use their separate API.
void ClassManager::registerSingleton(const SingletonRegInfo& info)
{
    // Built as a local first and copied into the map second - that is the order the binary uses
    // (the four fields are loaded off the parameter at the top of the function into adjacent stack
    // slots, and it is the stack copy, not the map's node, that createSingleton is handed below).
    SingletonInfo si;
    si.instance = info.instance;
    si.iid      = info.iid;
    si.classId  = info.classId;
    si.flags    = info.flags;

    m_normalSingletons[info.name] = si;

    // BIT 1 only. Bit 0 is initSingletons()'s to act on later, bit 2 means there is nothing to
    // construct at all. Handing over `si` rather than the map's copy is harmless: createSingleton
    // writes through si.instance, which is the address of a global, and never writes to si itself.
    if (si.flags & 2)
        createSingleton(info.name, si);

    // Preserve class-name construction even though its result is unused; the original purpose is [UNRESOLVED].
    // An unknown class ID uses a formatted fallback and does not prevent singleton creation.
    std::string className;
    if (!getClassName(si.classId, className))
        className = formatString("Unknown ID %d", si.classId);
}

// Finds the first matching class record by scanning registration order.
// Names compare without case; IDs compare exactly. Earlier duplicate registrations shadow later ones.
// On a match, copies the factory, class ID and name to the output record.
bool ClassManager::getClassInfo(unsigned int classId, ClassInfo& out)
{
    for (std::vector<ClassInfo>::iterator it = m_classInfos.begin(); it != m_classInfos.end(); ++it)
        if (it->classId == classId) { out = *it; return true; }
    return false;
}

bool ClassManager::getClassInfo(const std::string& name, ClassInfo& out)
{
    for (std::vector<ClassInfo>::iterator it = m_classInfos.begin(); it != m_classInfos.end(); ++it)
        if (equalsIgnoreCase(it->name, name)) { out = *it; return true; }
    return false;
}

// Looks up iid's default class; writes cid only on success and returns whether a mapping exists.
bool ClassManager::getDefaultClass(unsigned int iid, unsigned int& cid)
{
    std::map<unsigned int, unsigned int>::iterator it = m_defaultClasses.find(iid);
    if (it == m_defaultClasses.end())
        return false;
    cid = it->second;
    return true;
}

// IBase reference-count and interface operations precede the destructor slots.

void ClassManager::addRef()
{
    ++m_refCount;
}

int ClassManager::getRef()
{
    return m_refCount;
}

// Releases one reference, deleting this object at zero. Returns the remaining count.
int ClassManager::release()
{
    --m_refCount;
    int count = m_refCount;             // ebx
    if (count == 0 && this)
        delete this;                    // dispatched through vtable +0x14 (D0) in the ELF
    return count;
}

// `cmovz` in the ELF; an if/else here. Both ids yield the same pointer, which is correct only
// because the inheritance is single and non-virtual.
void* ClassManager::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<ClassManager*>(this);
    if (iid == IID_IClassManager)
        return const_cast<ClassManager*>(this);
    return 0;
}

// Registers a server with an empty mapped string and takes a reference.
// [UNRESOLVED] The discarded findServer call and unused mapped string retain their observed behavior.
void ClassManager::addServer(IClassServer* server)
{
    findServer(server);

    std::string s;
    m_classServerTree.insert(std::make_pair(server, s));

    server->addRef();                   // IClassServer vtable +0x00
}

// findServer - 0x83BAE70, 59 bytes. A plain membership test.
bool ClassManager::findServer(IClassServer* server)
{
    return m_classServerTree.find(server) != m_classServerTree.end();
}

// Removes a registered server and releases its reference. The server must be present; no missing-entry guard exists.
void ClassManager::removeServer(IClassServer* server)
{
    m_classServerTree.erase(m_classServerTree.find(server));
    server->release();                  // IClassServer vtable +0x08
}

// Unregisters singleton rows before class rows, both in reverse registration order.
// Class removal uses case-sensitive equality although lookup is case-insensitive, and erases the
// find result without an end check. [UNRESOLVED] purpose of the discarded findServer result.
void ClassManager::unregisterServer(IClassServer* server)
{
    findServer(server);

    // 1. drop the singletons, newest first
    std::vector<SingletonRegInfo> singletons;
    server->getSingletonList(singletons);        // IClassServer vtable +0x14

    for (std::vector<SingletonRegInfo>::reverse_iterator s = singletons.rbegin();
         s != singletons.rend(); ++s)
        unregisterSingleton(s->name);            // own vtable +0x48

    // 2. drop the classes, newest first
    std::vector<ClassRegInfo> classes;
    server->getClassList(classes);               // IClassServer vtable +0x10

    for (std::vector<ClassRegInfo>::reverse_iterator c = classes.rbegin();
         c != classes.rend(); ++c)
    {
        std::vector<ClassInfo>::iterator it =
            std::find(m_classInfos.begin(), m_classInfos.end(), c->name);
        m_classInfos.erase(it);                  // no end() check - see above
    }

    removeServer(server);
}

// Sets the default class used to satisfy the given interface ID.
void ClassManager::setDefaultClass(unsigned int iid, unsigned int classId)
{
    m_defaultClasses[iid] = classId;
}

// getClassName - 0x83BB8D0, 161 bytes. A getClassInfo wrapper. The 161 bytes are one ClassInfo
// local whose std::string member has to be constructed and destroyed on both exit paths.
bool ClassManager::getClassName(unsigned int classId, std::string& name)
{
    ClassInfo info;
    if (!getClassInfo(classId, info))
        return false;
    name = info.name;
    return true;
}

// getClassID - 0x83BB980, 153 bytes. The same wrapper the other way round, and note that it goes
// through getClassInfo(const std::string&) - so the name lookup is case-insensitive here too.
bool ClassManager::getClassID(const std::string& name, unsigned int& classId)
{
    ClassInfo info;
    if (!getClassInfo(name, info))
        return false;
    classId = info.classId;
    return true;
}

// Changes a registered singleton's class, recreating the instance immediately after singleton initialization.
// Returns false when name is not registered.
bool ClassManager::setSingletonClass(const std::string& name, unsigned int classId)
{
    std::map<std::string, SingletonInfo>::iterator it = m_normalSingletons.find(name);
    if (it == m_normalSingletons.end())
        return false;

    it->second.classId = classId;

    if (m_singletonsInit)
        createSingleton(it->first, it->second);

    return true;
}

// Always returns false without modifying classId, matching the original implementation.
bool ClassManager::getSingletonClass(const std::string& name, unsigned int& classId)
{
    return false;
}

// Removes the named singleton. Flags 0x1/0x2 transfer ownership to ClassManager, so those
// entries release their object and clear its global. Flag 0x4 entries only unregister.
void ClassManager::unregisterSingleton(const std::string& name)
{
    std::map<std::string, SingletonInfo>::iterator it = m_normalSingletons.find(name);
    if (it == m_normalSingletons.end())
        return;

    if (it->second.flags & 3)
    {
        IBase* obj = *it->second.instance;
        if (obj)
        {
            obj->release();                      // IBase vtable +0x08
            *it->second.instance = 0;
        }
    }

    m_normalSingletons.erase(it);
}

// Returns the registered singleton instance, or null for an unknown name or an instance not yet created.
IBase* ClassManager::getSingleton(const std::string& name)
{
    std::map<std::string, SingletonInfo>::iterator it = m_normalSingletons.find(name);
    if (it == m_normalSingletons.end())
        return 0;
    return *it->second.instance;
}

// Queries the named singleton for iid. A registered entry must already have a non-null instance.
void* ClassManager::getSingletonAs(const std::string& name, unsigned int iid)
{
    std::map<std::string, SingletonInfo>::iterator it = m_normalSingletons.find(name);
    if (it == m_normalSingletons.end())
        return 0;

    IBase* obj = *it->second.instance;
    return obj->queryInterface(iid);             // IBase vtable +0x0C - no null check, as shipped
}

// Registers a borrowed pointer by name. Re-registering an existing name keeps the first pointer.
void ClassManager::registerSimpleSingleton(const std::string& name, void* p)
{
    m_simpleSingletons.insert(std::make_pair(name, p));
}

// Removes the name without releasing or deleting its borrowed pointer.
void ClassManager::unregisterSimpleSingleton(const std::string& name)
{
    m_simpleSingletons.erase(name);
}

// getSimpleSingleton - 0x83BC4B0, 62 bytes. const, and the only const lookup in the class.
void* ClassManager::getSimpleSingleton(const std::string& name) const
{
    std::map<std::string, void*>::const_iterator it = m_simpleSingletons.find(name);
    if (it == m_simpleSingletons.end())
        return 0;
    return it->second;
}

// diagnostics

// Returns singleton names with simple entries before normal entries; preserve heading text and order.
std::string ClassManager::getPrintableSingletonList() const
{
    std::stringstream ss;

    ss << "\nSimple Singletons:\n------------------\n";
    for (std::map<std::string, void*>::const_iterator it = m_simpleSingletons.begin();
         it != m_simpleSingletons.end(); ++it)
        ss << it->first << "\n";

    ss << "\nNormal Singletons:\n------------------\n";
    for (std::map<std::string, SingletonInfo>::const_iterator it = m_normalSingletons.begin();
         it != m_normalSingletons.end(); ++it)
        ss << it->first << "\n";

    return ss.str();
}

// Appends registered class names containing sub. Matching is case-sensitive; out is not cleared.
void ClassManager::getAllClassIncludingSubstring(const std::string& sub,
                                                 std::vector<std::string>& out) const
{
    for (std::vector<ClassInfo>::const_iterator it = m_classInfos.begin();
         it != m_classInfos.end(); ++it)
    {
        if (it->name.find(sub) != std::string::npos)
            out.push_back(it->name);
    }
}

} // namespace hfe
} // namespace dice
