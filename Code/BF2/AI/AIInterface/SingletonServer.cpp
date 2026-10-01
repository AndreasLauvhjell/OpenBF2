#include "SingletonServer.h"

#include "AIInstance.h"
#include "IAIEventCourier.h"
#include "../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../../Core/ClassManager.h"

#include <string>

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

void*            BFSingletonServer::m_iaiObjectUtils       = 0;
void*            BFSingletonServer::m_iDebugRenderer       = 0;
void*            BFSingletonServer::m_iDPAIObjectCreator   = 0;
void*            BFSingletonServer::m_iInformationFactory  = 0;
IAIEventCourier* BFSingletonServer::m_iaiEventCourier      = 0;
void*            BFSingletonServer::m_iaiPlayerCourier     = 0;

// The updater destructor is defined in CommonSingletonServer.cpp, which is linked into
// both the engine and aidll.

void AIInstanceServer::updateInstance()
{
    g_aiInstance = reinterpret_cast<IBase*>(
        classManager->getSimpleSingleton(std::string("AI")));
}

void BFSingletonServer::updateStaticMethods()
{
    ISingletonServerUpdater* updater = static_cast<ISingletonServerUpdater*>(
        classManager->getSimpleSingleton(std::string("AISingletonServerUpdater")));
    if (updater)
        updater->update();

    updateStaticMethodsInternal();
}

void BFSingletonServer::updateStaticMethodsInternal()
{
    CommonSingletonServer::updateStaticMethodsInternal();
    m_iaiObjectUtils      = classManager->getSimpleSingleton(std::string("IAIObjectUtils"));
    m_iDebugRenderer      = classManager->getSimpleSingleton(std::string("IDebugRenderer"));
    m_iDPAIObjectCreator  = classManager->getSimpleSingleton(std::string("IDPAIObjectCreator"));
    m_iInformationFactory = classManager->getSimpleSingleton(std::string("IInformationFactory"));
    m_iaiEventCourier     = static_cast<IAIEventCourier*>(
        classManager->getSimpleSingleton(std::string("IAIEventCourier")));
    m_iaiPlayerCourier    = classManager->getSimpleSingleton(std::string("IAIPlayerCourier"));
}

void BFSingletonServer::clearAllStaticMethods()
{
    CommonSingletonServer::clearAllStaticMethods();
    m_iaiObjectUtils      = 0;
    m_iDebugRenderer      = 0;
    m_iDPAIObjectCreator  = 0;
    m_iInformationFactory = 0;
    m_iaiEventCourier     = 0;
    m_iaiPlayerCourier    = 0;
}

void* RenderSingletonServer::m_debugLineDrawer   = 0;
void* RenderSingletonServer::m_debugCircleDrawer = 0;
void* RenderSingletonServer::m_debugTextWriter   = 0;
void* RenderSingletonServer::m_debugRenderer     = 0;
void* RenderSingletonServer::m_iDebugRenderer    = 0;

// Five normal-registry lookups, in the binary's own order - which is not
// the members' address order; see the header.
void RenderSingletonServer::updateStaticMethods()
{
    m_debugLineDrawer   = classManager->getSingleton(std::string("DebugLineDrawer"));
    m_debugCircleDrawer = classManager->getSingleton(std::string("DebugCircleDrawer"));
    m_debugRenderer     = classManager->getSingleton(std::string("DebugRenderer"));
    m_debugTextWriter   = classManager->getSingleton(std::string("DebugTextWriter"));
    m_iDebugRenderer    = classManager->getSingleton(std::string("IDebugRenderer"));
}

BFSingletonServerUpdater::BFSingletonServerUpdater()
{
    const std::string name("BFSingletonServerUpdater");
    if (!classManager->getSimpleSingleton(name))
        classManager->registerSimpleSingleton(name, this);
}

BFSingletonServerUpdater::~BFSingletonServerUpdater()
{
    const std::string name("BFSingletonServerUpdater");
    if (classManager->getSimpleSingleton(name) == this)
        classManager->unregisterSimpleSingleton(name);
}

void BFSingletonServerUpdater::update()
{
    BFSingletonServer::updateStaticMethodsInternal();
}

typedef char BFSingletonServerUpdater_is_one_pointer[
    (sizeof(BFSingletonServerUpdater) == sizeof(void*)) ? 1 : -1];

} // namespace ai
} // namespace hfe
} // namespace dice
