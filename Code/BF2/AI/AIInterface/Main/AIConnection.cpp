// [INFERRED] Source placement.

#include "AIConnection.h"
#include "../AIInstance.h"                         // cross-port IAI clear dispatch
#include "../../../Core/PluginDll.h"

#include <cstdlib>                                  // abort - the init failure path, see below

#include "../../../Core/ClassManager.h"             // classManager->getSimpleSingleton (vtable +0x5C)
#include "../../../Core/Ids.h"                      // IID_IBase, ai::IID_IAIConnection

namespace dice {
namespace hfe {

// ds: = _ZN4dice3hfe12classManagerE. Defined in Core/Globals.cpp; no header declares it
// so every consumer spells the extern itself (Core/MemoryPool.cpp, Physics/HeightmapCluster.cpp
// Game/Main/BF2Engine.cpp all do exactly this).
extern ClassManager* classManager;

namespace ai {

// Loaded library handle and borrowed AI interface owned by the connection.
struct AIConnectionData
{
    AIConnectionData() : m_lib(0), m_ai(0) {}

    void* m_lib;      // +0x00
    void* m_ai;       // +0x04
};

// Creates an empty connection with an initial reference count of one.
AIConnection::AIConnection()
    : m_refCount(1)
    , m_data(new AIConnectionData)
{
    m_data->m_lib = 0;
}

// Closes the AI connection and deletes its connection data.
AIConnection::~AIConnection()
{
    clear();                                    // devirtualised inside the destructor, as in the ELF
    delete m_data;
}

// Adds one reference; IBase::addRef returns no value.
void AIConnection::addRef()
{
    ++m_refCount;
}

int AIConnection::getRef()
{
    return m_refCount;
}

// Drops one reference, deletes this at zero, and returns the saved new count.
int AIConnection::release()
{
    const int refs = --m_refCount;
    if (refs == 0)
        delete this;                            // dispatches vptr+0x14, the deleting destructor
    return refs;
}

// AIConnection::queryInterface - , , const.
// Two ids and no adjustment: AIConnection -> IAIConnection -> IBase is single inheritance all the
// way (both _ZTI records are __si_class_type_info), so all three subobjects share one address.
void* AIConnection::queryInterface(unsigned int iid) const
{
    if (iid == ::dice::hfe::IID_IBase)
        return const_cast<AIConnection*>(this);
    if (iid == IID_IAIConnection)
        return const_cast<AIConnection*>(this);
    return 0;
}

// Loads the named AI library and obtains its registered AI singleton.
// Aborts if loading fails; an empty aidll setting is handled by the module before this call.
bool AIConnection::init(std::string& libraryName)
{
    m_data->m_lib = LoadLibraryBf(libraryName.c_str());

    if (m_data->m_lib)
    {
        // The library's static initialisers have run by now and have registered its object under
        // the simple-singleton name "AI". Reading it straight back out of the registry is how the
        // handle is returned. ClassManager vtable +0x5C.
        m_data->m_ai = classManager->getSimpleSingleton(std::string("AI"));
    }
    else if (libraryName.compare("") != 0)
    {
        abort();                                // [SHIPPED DEFECT] see above
    }

    return m_data->m_lib != 0 && m_data->m_ai != 0;
}

// Returns 1 unconditionally.
// Shut the AI object down first, then unload the library - in that order, and each step is
// null-guarded and then zeroed whether or not it ran. Called by the destructor and by
// AIInterfaceModule::closeAIConnection .
// The external object is the root IAI interface, not IBase. Typed dispatch is load-bearing here:
// clear is ELF +0x0C but PE +0x08 because MSVC collapses the destructor pair to one slot.
bool AIConnection::clear()
{
    if (m_data->m_ai)
        static_cast<IAIInstance*>(m_data->m_ai)->clear();
    m_data->m_ai = 0;

    if (m_data->m_lib)
        // ELF ignores the diagnostic name. Win32 uses it only if the optional deinitDll throws.
        FreeLibraryBf(m_data->m_lib, "AI");
    m_data->m_lib = 0;

    return true;
}

// Two loads, no null check - callers do that themselves.
void* AIConnection::getAI()
{
    return m_data->m_ai;
}

// Interface destructor.
// Fourteen bytes is the vptr reset and nothing else - an empty body. Out of line because the
// interface has a real destructor symbol, which is what reserves slots +0x10/+0x14 ahead of
// init/clear/getAI.
IAIConnection::~IAIConnection()
{
}

// `operator new(0xCu)` in ai::ClassFactory<AIConnection>::create is the authority on
// this 12-byte x86 size.
#if defined(_WIN64) || defined(__LP64__)
// [CHOSEN, not recovered] Native port: vptr, 32-bit refcount, aligned native data pointer.
// Both payload members are process addresses; no DLL handle or AI pointer is serialized.
static_assert(sizeof(AIConnection) == 24, "native x64 AIConnection layout");
static_assert(sizeof(AIConnectionData) == 16, "native x64 AI connection holds two pointers");
#else
typedef char AIConnection_is_twelve_bytes[(sizeof(AIConnection) == 12) ? 1 : -1];
#endif

} // namespace ai
} // namespace hfe
} // namespace dice
