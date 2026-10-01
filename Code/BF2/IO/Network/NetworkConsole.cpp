// Network prediction and NetworkableInfo console bindings. File split [CHOSEN, not recovered].
// Register Network before NetworkableInfo. Info creation/deletion requires the manager; property
// executors require a selected info record and rely on the console's activity check before dispatch.
// Preserve command spelling, types and defaults used by Networkables.con.
#include "../../Core/ConsoleObjectMacro.h"
#include "NetworkManager.h"
#include "NetworkableInfo.h"
#include "../../Core/IBase.h"
#include <string>

namespace dice {
namespace hfe {
namespace io {

extern IBase*          networkableInfoManager;   // ds:0x8AC8CA0, Core/Globals.cpp
extern NetworkManager* networkManager;           // ds:0x8AC3D1C, Core/Globals.cpp

// Returns a mutable view of the active policy for console setters, or null when unavailable.
INetworkableInfo* getActiveNetworkableInfo()
{
    if (!networkableInfoManager)
        return 0;
    INetworkableInfoManager* manager = reinterpret_cast<INetworkableInfoManager*>(networkableInfoManager);
    return const_cast<INetworkableInfo*>(manager->getActiveNetworkableInfo());
}

namespace {

const char* const kNetworkConsoleTag = "io.network";

inline INetworkableInfoManager* infoManager()
{
    return reinterpret_cast<INetworkableInfoManager*>(networkableInfoManager);
}

void consoleSetGlobalPredictionMode(PredictionMode mode)   { networkManager->setGlobalPredictionMode(mode); }
void consoleCreateNewInfo(const std::string& name)         { infoManager()->createNewInfo(name); }
void consoleDeleteInfo(const std::string& name)            { infoManager()->deleteInfo(name); }
void consoleSetPredictionMode(PredictionMode mode)         { getActiveNetworkableInfo()->setPredictionMode(mode); }
void consoleSetBasePriority(float priority)                { getActiveNetworkableInfo()->setBasePriority(priority); }
void consoleSetIsUnique(bool unique)                       { getActiveNetworkableInfo()->setIsUnique(unique); }
void consoleSetForceNetworkableId(bool force)              { getActiveNetworkableInfo()->setForceNetworkableId(force); }

} // namespace

BF2_CONSOLE_OBJECT("Network", 2, 0)

// Console property name is globalPredictionMode; PredictionMode is its value type.
BF2_CONSOLE_PROPERTY_FN(globalPredictionMode, PredictionMode, networkManager != 0,
                        static_cast<PredictionMode>(networkManager->getGlobalPredictionMode()),
                        consoleSetGlobalPredictionMode, kNetworkConsoleTag)

BF2_CONSOLE_OBJECT("NetworkableInfo", 2, 0)
BF2_CONSOLE_METHOD_1_ACTIVE(createNewInfo, consoleCreateNewInfo, networkableInfoManager != 0,
                            std::string, kNetworkConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(deleteInfo, consoleDeleteInfo, networkableInfoManager != 0,
                            std::string, kNetworkConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setPredictionMode, consoleSetPredictionMode, getActiveNetworkableInfo() != 0,
                            PredictionMode, kNetworkConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setBasePriority, consoleSetBasePriority, getActiveNetworkableInfo() != 0,
                            float, kNetworkConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setIsUnique, consoleSetIsUnique, getActiveNetworkableInfo() != 0,
                            bool, kNetworkConsoleTag)
BF2_CONSOLE_METHOD_1_ACTIVE(setForceNetworkableId, consoleSetForceNetworkableId, getActiveNetworkableInfo() != 0,
                            bool, kNetworkConsoleTag)

// Handed to the console registry by io::NetworkModule::init (Core/ModuleRegistry.cpp) beside the
// TU's 0x8AC8704 list.
ConsoleObjectList& networkConsoleObjects()
{
    return consoleBindingList(kNetworkConsoleTag);
}

} // namespace io
} // namespace hfe
} // namespace dice
