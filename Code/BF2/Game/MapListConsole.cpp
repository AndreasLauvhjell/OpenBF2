// The maplist console object's append binding. Entry insertion still obeys
// MapList's validation and modifiable rules. The remaining console operations
// are not registered here. [CHOSEN] Binding file placement; access level is end-user.

#include "../Core/ConsoleObjectMacro.h"
#include "MapList.h"
#include <string>

namespace dice {
namespace hfe {

namespace {

// Stands in for the original's file-scope list, the way kSvConsoleTag does for sv.
const char* const kMapListConsoleTag = "hfe.maplist";

// Appends only while the map list is modifiable; isObjectActive enforces this
// precondition. The receiver is not null-checked and the override flag stays false.
bool mapListAppend(const std::string& name, const std::string& gameMode, int mapSize)
{
    return MapList::consoleWriteAccess()->appendEntry(name, gameMode, mapSize, false);
}

} // namespace

// SetConsoleClassDefaults("maplist", 1, 0) - see the header block for where both values are read.
BF2_CONSOLE_OBJECT("maplist", 1, 0)

// mapList.append takes a map, mode, and optional size, returning success.
// Omitting the size records zero; it does not select a default map size.
BF2_CONSOLE_METHOD_2_3_RET_ACTIVE(append, mapListAppend, bool,
                                  MapList::consoleWriteAccess() != 0,
                                  std::string, std::string, int, 0,
                                  kMapListConsoleTag)

// Handed to the console registry by ModuleGame::init, the way the original registers its own list.
ConsoleObjectList& mapListConsoleObjects()
{
    return consoleBindingList(kMapListConsoleTag);
}

} // namespace hfe
} // namespace dice
