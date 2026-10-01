// Admin console bindings registered by ModuleGameLogic at access level 1.
// [CHOSEN, not recovered] Separate translation unit beside the receivers.
// runNextLevel performs the action; nextLevel is its property, and no runLevel binding exists.

#include "../../Core/ConsoleObjectMacro.h"
#include "../../Core/Ids.h"
#include "../../Core/SettingsRepostitory.h"
#include "../IGame.h"
#include "../IGameLogic.h"
#include "../IGameServer.h"
#include "../GameLogic/ServerGameLogic.h"
#include "BanList.h"

#include <list>
#include <string>

namespace dice {
namespace hfe {

namespace {

const char* const kBanListConsoleTag = "hfe.gameLogic.adminBanList";

// ConsoleClass322..325 use this exact server-game-logic selection: the GSIsServer setting first,
// then the published gameLogic pointer. The executor trusts the console's active-object gate.
ServerGameLogic* adminServerGameLogic()
{
    if (!settings->boolGet(std::string("GSIsServer"), false))
        return 0;
    return static_cast<ServerGameLogic*>(gameLogic);
}

void adminRunNextLevel()
{
    adminServerGameLogic()->runNextLevel();
}

int adminCurrentLevel()
{
    return adminServerGameLogic()->getCurrentLevel();
}

int adminNextLevel()
{
    return adminServerGameLogic()->getNextLevel();
}

void adminSetNextLevel(int index)
{
    adminServerGameLogic()->setNextLevel(index);
}

void adminRestartMap()
{
    adminServerGameLogic()->restartMap();
}

std::string adminBanPlayer(const std::string& id)
{
    return BanList::getInstance()->consoleBanPlayer(id);
}

std::string adminBanPlayer(const std::string& id, const std::string& time)
{
    return BanList::getInstance()->consoleBanPlayer(id, time);
}

std::string adminBanPlayerKey(const std::string& id)
{
    return BanList::getInstance()->consoleBanPlayerKey(id);
}

std::string adminBanPlayerKey(const std::string& id, const std::string& time)
{
    return BanList::getInstance()->consoleBanPlayerKey(id, time);
}

void adminAddAddressToBanList(const std::string& address)
{
    BanList::getInstance()->consoleAddAddressToBanList(address);
}

void adminAddAddressToBanList(const std::string& address, const std::string& time)
{
    BanList::getInstance()->consoleAddAddressToBanList(address, time);
}

void adminAddKeyToBanList(const std::string& key)
{
    BanList::getInstance()->consoleAddKeyToBanList(key);
}

void adminAddKeyToBanList(const std::string& key, const std::string& time)
{
    BanList::getInstance()->consoleAddKeyToBanList(key, time);
}

void adminRemoveAddressFromBanList(const std::string& address)
{
    BanList::getInstance()->consoleRemoveAddressFromBanList(address);
}

void adminRemoveKeyFromBanList(const std::string& key)
{
    BanList::getInstance()->consoleRemoveKeyFromBanList(key);
}

void adminClearBanList()
{
    BanList::getInstance()->consoleClearBanList();
}

std::string adminListBannedAddresses()
{
    return BanList::getInstance()->consoleListBannedAddresses();
}

std::string adminListBannedKeys()
{
    return BanList::getInstance()->consoleListBannedKeys();
}

IGameServer* adminGameServer()
{
    return game ? static_cast<IGameServer*>(game->queryInterface(IID_IGameServer)) : 0;
}

IGame* adminGame()
{
    return game ? static_cast<IGame*>(game->queryInterface(IID_IGame)) : 0;
}

std::string adminKickPlayer(const std::string& id)
{
    return adminGameServer()->consoleKickPlayer(id);
}

std::string adminListPlayers()
{
    return adminGame()->listPlayers();
}

} // anonymous namespace

BF2_CONSOLE_OBJECT("admin", 1, 0)

BF2_CONSOLE_METHOD_0_STATIC_ACTIVE(runNextLevel, adminRunNextLevel,
                                    adminServerGameLogic() != 0, kBanListConsoleTag)
BF2_CONSOLE_METHOD_0_RET_ACTIVE(currentLevel, adminCurrentLevel, int,
                                adminServerGameLogic() != 0, kBanListConsoleTag)
BF2_CONSOLE_PROPERTY_FN(nextLevel, int, adminServerGameLogic() != 0,
                        adminNextLevel(), adminSetNextLevel, kBanListConsoleTag)
BF2_CONSOLE_METHOD_0_STATIC_ACTIVE(restartMap, adminRestartMap,
                                    adminServerGameLogic() != 0, kBanListConsoleTag)
BF2_CONSOLE_METHOD_1_2_RET_ACTIVE(banPlayer, adminBanPlayer, std::string,
                                  BanList::getInstance() != 0,
                                  std::string, std::string, kBanListConsoleTag)
BF2_CONSOLE_METHOD_1_2_RET_ACTIVE(banPlayerKey, adminBanPlayerKey, std::string,
                                  BanList::getInstance() != 0,
                                  std::string, std::string, kBanListConsoleTag)
BF2_CONSOLE_METHOD_1_2_STATIC_ACTIVE(addAddressToBanList, adminAddAddressToBanList,
                                     BanList::getInstance() != 0,
                                     std::string, std::string, kBanListConsoleTag)
BF2_CONSOLE_METHOD_1_2_STATIC_ACTIVE(addKeyToBanList, adminAddKeyToBanList,
                                     BanList::getInstance() != 0,
                                     std::string, std::string, kBanListConsoleTag)
BF2_CONSOLE_METHOD_1_STATIC_ACTIVE(removeAddressFromBanList,
                                   adminRemoveAddressFromBanList,
                                   BanList::getInstance() != 0,
                                   std::string, kBanListConsoleTag)
BF2_CONSOLE_METHOD_1_STATIC_ACTIVE(removeKeyFromBanList,
                                   adminRemoveKeyFromBanList,
                                   BanList::getInstance() != 0,
                                   std::string, kBanListConsoleTag)
BF2_CONSOLE_METHOD_0_STATIC_ACTIVE(clearBanList, adminClearBanList,
                                    BanList::getInstance() != 0, kBanListConsoleTag)
BF2_CONSOLE_METHOD_0_RET_ACTIVE(listBannedAddresses, adminListBannedAddresses, std::string,
                                BanList::getInstance() != 0, kBanListConsoleTag)
BF2_CONSOLE_METHOD_0_RET_ACTIVE(listBannedKeys, adminListBannedKeys, std::string,
                                BanList::getInstance() != 0, kBanListConsoleTag)
BF2_CONSOLE_METHOD_1_RET_ACTIVE(kickPlayer, adminKickPlayer, std::string,
                                adminGameServer() != 0, std::string, kBanListConsoleTag)
BF2_CONSOLE_METHOD_0_RET_ACTIVE(listPlayers, adminListPlayers, std::string,
                                adminGame() != 0, kBanListConsoleTag)

std::list<ConsoleObject*>& banListConsoleObjects()
{
    return consoleBindingList(kBanListConsoleTag);
}

} // namespace hfe
} // namespace dice
