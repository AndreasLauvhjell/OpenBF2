// Test scaffolding for isolated MSVC syntax checks. Declarations are provisional and must not enter engine builds.

#ifndef BF2_STUB_CORE_H
#define BF2_STUB_CORE_H

#include <string>
#include <vector>

// this include must stay outside THE namespace blocks below.
// Putting it inside `namespace dice { namespace hfe {` nests the real header's own namespaces,
// yielding dice::hfe::dice::hfe::EventCategory and a wall of C2061s. That mistake has now been made
// three times in this tree (IObject.h, NetworkableBase.h, and here) and the compile gate has caught
// it every time — which is the argument for the gate, not for being more careful.
#include "../BF2/Game/IEventListener.h"
// Use real Mutex and Debug declarations.
#include "../BF2/Core/Mutex.h"
#include "../BF2/Core/Debug.h"

namespace dice {
namespace hfe {

class BF2;
class BF2Engine;
class IModule;
class IEventListener;
struct SingletonRegInfo;
// EventNode is forward-declared only; tests do not dereference it.
class EventNode;

// Provisional Thread declaration for syntax checks; supports named start, stop, and virtual destruction.
class Thread
{
public:
    enum Priority { PRIORITY_STUB_PLACEHOLDER = 0 };
    Thread();
    virtual ~Thread();
    bool start(const char*, Priority);
    void stop();
};

// Provisional event-manager surface for syntax checks.
class EventManager
{
public:
    virtual ~EventManager();
    virtual bool registerEventHandler(EventCategory, IEventListener*, int);
    virtual bool unregisterEventHandler(EventCategory, IEventListener*);
    virtual void postEvent(EventCategory, unsigned int, void*, float);
};

// Provisional class-manager surface; generic singleton lookup returns void*.
class ClassManager
{
public:
    virtual ~ClassManager();
    virtual void  registerServer(IModule*);
    virtual void  unregisterServer(IModule*);
    virtual void  setDefaultClass(unsigned int iid, unsigned int cid);
    virtual void  initSingletons();
    virtual void  registerSingleton(const SingletonRegInfo&);
    virtual void  unregisterSingleton(const std::string&);
    virtual void* getSingleton(const std::string&);
};

// SingletonServer::init() - BF2Engine.cpp: "genuinely static - 0x813907E pushes
// nothing".
class SingletonServer
{
public:
    static bool init();
};

// systemInfo() - BF2Engine.cpp cites _ZN4dice3hfe10systemInfoEv returning the
// function-local static PosixSystemInfo, and getIsDebuggerInstalled at vptr+0x1C.
class ISystemInfo
{
public:
    virtual ~ISystemInfo();
    virtual bool getIsDebuggerInstalled() const;
};
ISystemInfo* systemInfo();

// Used by BF2Engine::initEngine(); no signature is stated in the recovered tree.
bool compareIgnoreCase(const std::string&, const std::string&);

namespace memory {
void memory_update_modules();                 // called by BF2::init()
void set_max_alloc_size(unsigned int);        // named in BF2Engine.cpp's parseParameters notes
}

// Provisional interface handle supporting create and pointer conversion.
template <class T, unsigned int* Iid>
class SmartItf
{
public:
    SmartItf();
    ~SmartItf();
    bool create(unsigned int cid);
    operator T*() const;
    T* operator->() const;
};

} // namespace hfe
} // namespace dice

#endif // BF2_STUB_CORE_H
