// AI module registration and lifecycle entry points.
// [INFERRED] Source placement.

#include "AIModule.h"

#include <list>
#include <vector>

#include "../../../../Core/ClassManager.h"
#include "../../../../Core/ConsoleObjects.h"
#include "../../../../Core/DllModule.h"
#include "../../../../Core/Ids.h"
#include "../AIUtils/checkPointer.h"
#include "BF2AI.h"

namespace dice {
namespace hfe {

// Core/Globals.cpp declares these in the engine; in a module image they are this file's own copies
// filled by initDll. No header declares them, so every consumer spells the extern itself.
extern ClassManager*   classManager;
extern ConsoleObjects* mainConsoleObjects;

// Module instances allocated during DLL initialization.
IModule* dllmodules[] =
{
    new ai::AIModule,
    0
};

namespace ai {

namespace {

// Console objects registered by module initialization and removed on close.

} // anonymous namespace

// Defined in AIConsole.cpp, beside the binding itself.
ConsoleObjects::ConsoleObjectList& aiConsoleObjects();
ConsoleObjects::ConsoleObjectList& aiSettingsConsoleObjects();   // AISettings.cpp
// AIStrategicEntity/AIStrategicObjectManager.cpp - `aiStrategicArea.createFromControlPoint`, the
// one command of nine on that object whose whole chain is written.
ConsoleObjects::ConsoleObjectList& aiStrategicAreaConsoleObjects();
// Pathfinding console commands for navigation meshes, properties, and the trace map.
ConsoleObjects::ConsoleObjectList& aiPathfindingConsoleObjects();

// Two stores and nothing else - the vptr, and a refcount of one.
AIModule::AIModule()
    : m_refCount(1)
{
}

// The IBase quartet. Textbook, and reproduced because "textbook" is itself a finding - the AI module
// does not do anything unusual with lifetime.

void AIModule::addRef()
{
    ++m_refCount;
}

int AIModule::getRef()
{
    return m_refCount;
}

// Decrements the reference count and frees this without a destructor at zero.
// Requires a heap-allocated module with no member cleanup.
int AIModule::release()
{
    const int n = --m_refCount;
    if (n == 0)
        operator delete(this);
    return n;
}

// Returns the interface pointer for IBase, IClassServer, or IModule, testing in that order.
void* AIModule::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IClassServer)
        return const_cast<AIModule*>(this);

    if (iid == IID_IModule)
        return const_cast<AIModule*>(this);

    return 0;
}

// IClassServer - both empty. See the note in AIModule.h; this is the fact that the AI registers no
// class ids and is therefore reachable only as a simple singleton.

// - a bare `ret`.
void AIModule::getClassList(std::vector<ClassRegInfo>& /*out*/)
{
}

// - a bare `ret`.
void AIModule::getSingletonList(std::vector<SingletonRegInfo>& /*out*/)
{
}

// Creates the AI singleton and publishes its console bindings.
bool AIModule::init()
{
    std::fflush(stderr);
    mainConsoleObjects->registerConsoleObjects(aiConsoleObjects());
    mainConsoleObjects->registerConsoleObjects(aiSettingsConsoleObjects());
    mainConsoleObjects->registerConsoleObjects(aiStrategicAreaConsoleObjects());
    mainConsoleObjects->registerConsoleObjects(aiPathfindingConsoleObjects());

    BF2AI* ai = new BF2AI;
    // Preserves the explicit allocation check even with throwing operator new.
    checkPointer(ai);

    // true even if the above failed - see the file header.
    return true;
}

// Unregisters console objects and looks up the AI through getSingleton. The original
// uses the wrong registry for this simple singleton, leaving the object allocated.
bool AIModule::close()
{
    mainConsoleObjects->unregisterConsoleObjects(aiConsoleObjects());
    mainConsoleObjects->unregisterConsoleObjects(aiSettingsConsoleObjects());
    mainConsoleObjects->unregisterConsoleObjects(aiStrategicAreaConsoleObjects());
    mainConsoleObjects->unregisterConsoleObjects(aiPathfindingConsoleObjects());

    // getSingleton (+0x4C) searches m_normalSingletons. "AI" was registered into
    // m_simpleSingletons. This returns null on every run.
    IBase* ai = classManager->getSingleton("AI");

    // The unregister does hit the right map, so the name is correctly released either way.
    classManager->unregisterSimpleSingleton("AI");

    if (ai)
    {
        // IAI vtable +0x04 - the deleting destructor , not release. IAI is not an IBase and
        // has no refcount to drop. [FRONTIER] to express in C++ until IAI's destructor is declared
        // in a way this file can reach; the call is recorded rather than written, because on every
        // real run `ai` is null and the arm is dead.
        // reinterpret_cast<IAI*>(ai)->~IAI; // conceptually; the slot is , so: delete
    }

    return true;
}

} // namespace ai
} // namespace hfe
} // namespace dice
