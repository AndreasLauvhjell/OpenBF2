// Cached interfaces shared by AI subsystems. Refresh after registrations change.
// Registry names are case-sensitive and need not match the member-name prefixes.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_MAIN_COMMONSINGLETONSERVER_H     // [INFERRED] guard style
#define AI_AIDLL_AI_MAIN_COMMONSINGLETONSERVER_H

#include "../../../AIInterface/SingletonServer.h"    // ISingletonServerUpdater - the shared base

namespace dice {
namespace hfe {
namespace ai {

// The twenty-eight, in the exact order updateStaticMethodsInternal fills them and
// clearAllStaticMethods nulls them. The order is reproduced because both functions agree on it and
// because it is the only thing that makes the two bodies checkable against each other.
class CommonSingletonServer
{
public:
    // empty - five bytes, a bare `ret`. The base class publishes the entry point and does nothing
    // with it; only AISingletonServer's override is ever called. Worth knowing before anyone
    // "fixes" it.
    static void updateStaticMethods();              // - empty
    static void updateStaticMethodsInternal();
    static void clearAllStaticMethods();

    static void* m_iAimerHelper;                        // 1 "AimerHelper" no I prefix
    static void* m_iBFTimer;                            // 2 "IBFTimer"
    static void* m_iaiInformationCourier;               // 3 "IAIInformationCourier"
    static void* m_iaiInformationEnvironmentModifiers;  // 4 "IAIInformationEnvironmentModifiers"
                                                        // plural, and distinct from
                                                        // AISingletonServer::m_iaiInformationEnvironment
    static void* m_iaiPathfindingCourier;               // 5 "IAIPathfindingCourier"
    static void* m_iaiTerrain;                          // 6 "IAITerrain"
    static void* m_iaiGame;                             // 7 "IAIGame"
    static void* m_iaiGameEffector;                     // 8 "IAIGameEffector"
    static void* m_iaiObjectEnvironment;                // 9 "IAIObjectEnvironment"
    static void* m_iaiPhysics;                          // 10 "IAIPhysics"
    static void* m_iDPManager;                          // 11 "IDPManager"
    static void* m_iDPStaticCreator;                    // 12 "IDPStaticCreator"
    static void* m_iDPAIObjectCreator;                  // 13 "IDPAIObjectCreator"
    static void* m_iaiSettings;                         // 14 "IAISettings"
    static void* m_iaiSettingsModifier;                 // 15 "IAISettingsModifier"
    static void* m_iAICollisionHandler;                 // 16 "IAICollisionHandler" m_iAI, not m_iai
    static void* m_iBFCollisionHandler;                 // 17 "IBFCollisionHandler"
    static void* m_iKitTemplateManager;                 // 18 "IKitTemplateManager"
    static void* m_iaiPlayerFactory;                    // 19 "IAIPlayerFactory"
    static void* m_iInformationFactory;                 // 20 "IInformationFactory"
    static void* m_iMapRenderer;                        // 21 "IMapRenderer"
    static void* m_iDebugRenderer;                      // 22 "IDebugRenderer"
    static void* m_iBFModuleCourier;                    // 23 "IBFModuleCourier"
    static void* m_iBFCommander;                        // 24 "IBFCommander"
    static void* m_iBFSquad;                            // 25 "IBFSquad"
    static void* m_iBFRadioComm;                        // 26 "IBFRadioComm"
    static void* m_iBFGraphCreator;                     // 27 "IBFGraphCreator"
    static void* m_iAIMineManager;                      // 28 "IAIMineManager" m_iAI, not m_iai
};

// Refreshes cached AI interfaces alongside the common singleton roster.
class AISingletonServer
{
public:
    // the entry point. Looks up "BFSingletonServerUpdater" and calls its typed
    // update method (ELF +0x08 / PE +0x04), then chains to the internal. The updater it kicks is
    // the engine's - the mechanism
    // is symmetric across the DLL boundary, and this module registers its own as
    // "AISingletonServerUpdater" (which BF2AI::clear destroys by that name).
    static void updateStaticMethods();

    // Calls CommonSingletonServer::updateStaticMethodsInternal first, then fills
    // the three below - so the common set is available to anything the AI set might touch.
    static void updateStaticMethodsInternal();

    // Same shape: clears the common set first, then its own three.
    static void clearAllStaticMethods();

    static void* m_iaiInformationEnvironment;       // "IAIInformationEnvironment"
    static void* m_iaiStrategicEnvironment;         // "IAIStrategicEnvironment"
    static void* m_iDPDynamicCreator;               // "IDPDynamicCreator"
};

// AI DLL participant in the cross-module singleton refresh.
class AISingletonServerUpdater : public ISingletonServerUpdater
{
public:
    AISingletonServerUpdater();
    virtual ~AISingletonServerUpdater();            // , 260
    virtual void update();
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_MAIN_COMMONSINGLETONSERVER_H
