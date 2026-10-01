// Refreshes cached AI interfaces from the singleton registry.
// [INFERRED] Source placement.

#include "CommonSingletonServer.h"
#include "../../../AIInterface/SingletonServer.h"       // ISingletonServerUpdater - the typed kick

#include <string>

#include "../../../../Core/ClassManager.h"

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// definitions. `= 0` is the ELF's own initial state (.bss), not a convention
void* AISingletonServer::m_iaiInformationEnvironment               = 0;
void* AISingletonServer::m_iaiStrategicEnvironment                 = 0;
void* AISingletonServer::m_iDPDynamicCreator                       = 0;

// Refreshes the engine's singleton updater first when present, then the AI cached interfaces.
void AISingletonServer::updateStaticMethods()
{
    ISingletonServerUpdater* updater = static_cast<ISingletonServerUpdater*>(
        classManager->getSimpleSingleton("BFSingletonServerUpdater"));
    if (updater)
        updater->update();

    updateStaticMethodsInternal();
}

// the common set is filled first. Anything the three below might touch during their own lookup is
// already in place. The order is load-bearing and reproduced.
void AISingletonServer::updateStaticMethodsInternal()
{
    CommonSingletonServer::updateStaticMethodsInternal();

    m_iaiInformationEnvironment = classManager->getSimpleSingleton("IAIInformationEnvironment");
    m_iaiStrategicEnvironment   = classManager->getSimpleSingleton("IAIStrategicEnvironment");
    m_iDPDynamicCreator         = classManager->getSimpleSingleton("IDPDynamicCreator");
}

// Same shape, common set first.
void AISingletonServer::clearAllStaticMethods()
{
    CommonSingletonServer::clearAllStaticMethods();

    m_iaiInformationEnvironment = 0;
    m_iaiStrategicEnvironment   = 0;
    m_iDPDynamicCreator         = 0;
}

// Registers this updater only when the simple-singleton name is vacant.
AISingletonServerUpdater::AISingletonServerUpdater()
{
    if (!classManager->getSimpleSingleton("AISingletonServerUpdater"))
        classManager->registerSimpleSingleton("AISingletonServerUpdater", this);
}

// Unregisters the updater only if the registry still points to this instance.
AISingletonServerUpdater::~AISingletonServerUpdater()
{
    if (classManager->getSimpleSingleton("AISingletonServerUpdater") == this)
        classManager->unregisterSimpleSingleton("AISingletonServerUpdater");
}

// Refreshes DLL statics without calling the public cross-image entry point.
// Calling updateStaticMethods here would recurse through the engine updater.
void AISingletonServerUpdater::update()
{
    AISingletonServer::updateStaticMethodsInternal();
}

} // namespace ai
} // namespace hfe
} // namespace dice
