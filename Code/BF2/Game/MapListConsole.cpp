// The maplist console object: append, list and currentMap. Entry insertion still obeys
// MapList's validation and modifiable rules; the read commands serve the rotation text and
// selection that level scripts and server tooling consume. [CHOSEN] Binding file placement;
// access level is end-user.

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

// maplist.list - the rotation as console text (ELF ConsoleClass761::executeObjectMethod at
// 0xa91d4b calls formatListForConsole(false)).
std::string mapListList()
{
    return MapList::consoleReadAccess()->formatListForConsole(false);
}

// maplist.currentMap - the selected rotation index, or -1 for none (ELF getCurrentMap 0x4173f0).
int mapListCurrentMap()
{
    return MapList::consoleReadAccess()->getCurrentMap();
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

BF2_CONSOLE_METHOD_0_RET_ACTIVE(list, mapListList, std::string,
                                MapList::consoleReadAccess() != 0, kMapListConsoleTag)
BF2_CONSOLE_METHOD_0_RET_ACTIVE(currentMap, mapListCurrentMap, int,
                                MapList::consoleReadAccess() != 0, kMapListConsoleTag)

// Handed to the console registry by ModuleGame::init, the way the original registers its own list.
ConsoleObjectList& mapListConsoleObjects()
{
    return consoleBindingList(kMapListConsoleTag);
}

} // namespace hfe
} // namespace dice
