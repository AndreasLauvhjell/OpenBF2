// Order handling. [INFERRED] Source split from Menu/BF2HudManager.cpp.
#include "BF2HudManager.h"
#if !DEDICATED
#include "Bf2MenuMapNode.h"
#include "HudCommander.h"
#include "../Core/EventManager.h"
#include "../Core/StringUtil.h"
#include "../Game/Main/BF2ClientSeams.h"
#include "../Game/GameLogic/SquadManager/ISquadManager.h"
#include "../Game/GameLogic/SquadManager/ISquadInfo.h"
#include "../Game/GameLogic/SquadManager/SquadOrder.h"
#include "../Game/GameServer/ServerSettings.h"   // isRequestAvailable: serverSettings->getNoVehicles
#include "../Game/Objects/IPlayer.h"
#include "../Scene/Object/PlayerManager.h"
#include <cmath>
#include <cfloat>
namespace dice { namespace hfe {
extern IBase* squadManager;
extern hud::BF2HudManager* hudManager;
namespace hud {
namespace {
// The shipped Vec3 comparison53C200 uses three ordered absolute-difference
// tests against FLT_EPSILON. Preserve its NaN rejection and short circuiting.
bool isInvalidOrderPosition(const Vec3& position)
{
    return std::fabs(static_cast<double>(position.x) - -4000.0) <= FLT_EPSILON
        && std::fabs(static_cast<double>(position.y) - -4000.0) <= FLT_EPSILON
        && std::fabs(static_cast<double>(position.z) - -4000.0) <= FLT_EPSILON;
}
}

// Announces the selected squad order.
void BF2HudManager::announceSquadOrder(SquadOrder* order, int squad)
{
    world::IPlayer* const local = world::playerManager->getLocalHumanPlayer();   // +0x30
    if (!local)
        return;
    if (!order)
        return;

    TargetChannel channel = TCSquad;                                   // esi = 3
    int receiver = 0;                                                  // the argument slot, reused
    std::string message("move");
    const int type = order->orderId;
    if (type == 5)
        message = order->targetNetId > 0 ? "attack_object" : "attack_position";
    else if (type == 6)
        message = "defend";
    else if (type == 10)
        message = "attack_cp";
    else if (type == 7)
        message = "repair";
    else if (type == 8)
        message = "destroy";
    else if (type == 9)
        message = "req_mines";
    else if (type == 0)
    {
        message = "req_artillery";
        channel = TCPlayer;                                            // esi = 0xE
    }
    else if (type == 1)
    {
        message = "req_uav";
        channel = TCPlayer;
    }
    else if (type == 3)
    {
        message = "req_supply";
        channel = TCPlayer;
    }
    else if (type == 2)
    {
        message = "req_vehicledrop";
        channel = TCPlayer;
    }

    if (local->getIsCommander())                                       // IPlayer +0x110
    {
        channel = TCPlayer;
        receiver = static_cast<ISquadManager*>(squadManager)->getSquadLeader(local->getTeam(), squad);  // +0x3C
    }
    else if (channel == TCPlayer)
    {
        if (static_cast<ISquadManager*>(squadManager)->getCommander(local->getTeam()) >= 0)   // +0x70, `jl`
            receiver = static_cast<ISquadManager*>(squadManager)->getCommander(local->getTeam());
    }
    game->radioMessage(channel, message, local, receiver, true, false);                        // +0x50
}

void BF2HudManager::sendSquadReply(int squad, std::string message, bool announce)
{
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    if (player && !message.empty()) {
        const int team = player->getTeam();
        const int leader = static_cast<ISquadManager*>(squadManager)->getSquadLeader(team, squad);
        game->radioMessage(static_cast<TargetChannel>(14), message, player, leader, announce, false);
    }
}

// Returns whether the commander asset for type is available. Invalid types return false.
// The HUD map and commander module must exist; artillery is unavailable when vehicles are disabled.
bool BF2HudManager::isRequestAvailable(int type)
{
    switch (static_cast<unsigned int>(type))
    {
    case 0:
        if (serverSettings->getNoVehicles())                           // ds:9A4D98, 44B950
            return false;
        return !m_field13c->m_artilleryReloading;                      // +0x44
    case 1:
        return !hudMap->m_uavReloading;                                // +0xAE0
    case 2:
        return !m_field13c->m_vehicleDropReloading;                    // +0x64
    case 3:
        return !m_field13c->m_supplyReloading;                         // +0x5C
    default:
        return false;
    }
}

// getRequestName - 7AFBE0, `retn 8` (the hidden return slot and the type), [INFERRED] name. The
// request's localized display name, through the virtual getLocalizedString (+0x38) with a
// std::string key; any other type answers L"" (8A3024, an empty wide literal). Jump table 7AFC58,
// the same unsigned range test.
std::wstring BF2HudManager::getRequestName(int type)
{
    switch (static_cast<unsigned int>(type))
    {
    case 0:
        return getLocalizedString("HUD_TEXT_COMMANDER_ARTILLERY");
    case 1:
        return getLocalizedString("HUD_TEXT_COMMANDER_UAV");
    case 2:
        return getLocalizedString("HUD_TEXT_COMMANDER_VEHICLEDROP");
    case 3:
        return getLocalizedString("HUD_TEXT_COMMANDER_SUPPLIES");
    default:
        return std::wstring(L"");
    }
}

void BF2HudManager::accept(bool accepted)
{
    if (hudMap && world::playerManager->getLocalHumanPlayer()) {
        const int requestType = hudMap->m_selectedRequestType;
        const int squad = hudMap->m_selectedRequestSquad;
        respondToRequest(accepted, squad, 2, requestType);
    }
}

void BF2HudManager::respondToRequest(bool accepted, int /*squad*/, int orderType, int requestType)
{
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    if (!player) return;
    std::string response("");
    std::string denial("");
    bool executed = false;
    if (orderType == 2) {
        hudManager->disableVOHelpMessage(37);
        switch (requestType) {
        case 0:
            if (accepted) {
                executed = launchArtillery(true);
                response = executed ? "artillery_accept" : "artillery_unavailable";
            } else response = "artillery_deny";
            denial = "artillery_deny";
            break;
        case 1:
            if (accepted) {
                executed = uav(true);
                response = executed ? "uav_accept" : "uav_unavailable";
            } else response = "uav_deny";
            denial = "uav_deny";
            break;
        case 2:
            if (accepted) {
                executed = vehicleDrop(true);
                response = executed ? "vehicledrop_accept" : "vehicledrop_unavailable";
            } else response = "vehicledrop_deny";
            denial = "vehicledrop_deny";
            break;
        case 3:
            if (accepted) {
                executed = supplies(true);
                response = executed ? "supply_accept" : "supply_unavailable";
            } else response = "supply_deny";
            denial = "supply_deny";
            break;
        }
    }
    // The passed squad is unused. Retail reloads the map's selected squad and
    // request fields while iterating every squad, including squad zero.
    for (int squad = 0; squad < 10; ++squad) {
        if (squad != hudMap->m_selectedRequestSquad) {
            if (executed && accepted) {
                ISquadInfo* info = static_cast<ISquadManager*>(squadManager)->getSquadInfo(player->getTeam(), squad);
                if (info && info->getRequest(static_cast<SquadOrderType>(hudMap->m_selectedRequestType))) {
                    const int type = hudMap->m_selectedRequestType;
                    const OrderType orderType = hudMap->m_selectedRequestOrderType;
                    const int team = player->getTeam();
                    static_cast<ISquadManager*>(squadManager)->setIsAccepted(team, squad, false,
                        orderType, static_cast<SquadOrderType>(type), false);
                    sendSquadReply(squad, denial, false);
                }
            }
        } else if (!executed && accepted) {
            if (game) game->localGameInfoMessage(static_cast<TargetChannel>(14), denial);
        } else {
            const int type = hudMap->m_selectedRequestType;
            const OrderType orderType = hudMap->m_selectedRequestOrderType;
            const int team = player->getTeam();
            static_cast<ISquadManager*>(squadManager)->setIsAccepted(team, squad, false,
                orderType, static_cast<SquadOrderType>(type), false);
            sendSquadReply(squad, response, true);
        }
    }
}

// Queues a reloading-asset request for four seconds before replying that the asset is in use. [INFERRED] Name.
void BF2HudManager::deferSquadRequest(int squad, int type)
{
    HudPendingRequest request;
    request.squad = squad;
    request.type = type;
    request.remaining = 4.0f;
    m_field32c.push_back(request);
}

// Announces a squad's request to the commander.
void BF2HudManager::announceSquadRequest(int squad, int type)
{
    world::IPlayer* const local = world::playerManager->getLocalHumanPlayer();   // +0x30
    if (!local)
        return;
    ISquadInfo* const info =
        static_cast<ISquadManager*>(squadManager)->getSquadInfo(local->getTeam(), squad);   // +0x28
    if (!info)
        return;
    if (!info->getIsAccepted(static_cast<SquadOrderType>(type), static_cast<OrderType>(2)))  // +0x08
        return;

    if (isRequestAvailable(type))
    {
        const char* const squadName = info->getSquadName().c_str();    // +0x00
        const std::wstring requestName = getRequestName(type);
        addTeamChat(replaceWideTokenFromString(                        // +0x120, +0x40
                        replaceWideToken(                              // +0x3C
                            getLocalizedString("HUD_HUD_REQUEST_RECIEVED_FROM_SQUAD"),   // +0x38
                            "#REQUESTNAME#", requestName.c_str()),
                        "#SQUADNAME#", squadName),
                    local, local->getTeam());
    }
    else
    {
        deferSquadRequest(squad, type);
    }
}

void BF2HudManager::sendOrder(int type, bool useMap)
{
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    if (!player) {
        addServerChat(hudManager->getLocalizedString("HUD_HUD_SQUAD_ALIVE_TO_ORDER"), 0);
        return;
    }
    player->getIsCommander(); // Discarded retail call, before optional map work.
    if (useMap) hudMap->prepareOrder(type);
    if (type > 9) {
        m_order.points[0] = Vec3{-4000.0f, -4000.0f, -4000.0f};
        m_order.targetNetId = 0;
        m_order.orderId = 4;
        m_order.flag2C = false;
    } else if (!useMap) m_order.orderId = type;
    if (isInvalidOrderPosition(m_order.points[0])) return;
    m_order.targetNetId = 0;
    if (player->getIsCommander() && m_field13c) {
        bool sent = false;
        // The event payload reuses the comparison vector's stack storage. Its
        // middle word starts as -4000's bits and survives synchronous callbacks.
        std::uint32_t notification[3] = {0xC57A0000u, 0xC57A0000u, 0xC57A0000u};
        for (int squad = 1; squad < 10; ++squad) {
            if (m_field13c->isSquadSelected(squad)
                && world::playerManager->getNrOfPlayersInSquad(player->getTeam(), squad) > 0) {
                m_order.flag2C = false;
                sent = true;
                static_cast<ISquadManager*>(squadManager)->issueCommanderOrder(player->getTeam(), squad, m_order, false);
                notification[0] = squad;
                notification[2] = 0;
                eventManager->postEvent(ECHud, 25, notification, 0.0f);
            }
        }
        if (!sent) addServerChat(hudManager->getLocalizedString("HUD_HUD_SQUAD_NO_SQUAD_SELECTED"), 0);
    } else {
        m_order.flag2C = true;
        const int squad = player->getSquadId();
        const int team = player->getTeam();
        static_cast<ISquadManager*>(squadManager)->issueSquadOrder(team, squad, m_order, false);
    }
}

void BF2HudManager::sendRequest(int type, bool useMap)
{
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    if (!player || !player->getIsAlive()) {
        addServerChat(hudManager->getLocalizedString("HUD_HUD_SQUAD_ALIVE_TO_REQUEST"), 0);
        return;
    }
    IBase* const aiInstance = unresolvedGlobalA33C08;
    const std::string mode(game->getCurrentMapGameMode());
    const bool needsCommander = compareIgnoreCase(mode, std::string("gpm_coop")) != 0;
    if (needsCommander && static_cast<ISquadManager*>(squadManager)->getCommander(player->getTeam()) < 0 && !aiInstance) {
        const std::string message("no_commander_available");
        game->localGameInfoMessage(static_cast<TargetChannel>(14), message);
        return;
    }
    if (useMap) hudMap->prepareOrder(type);
    if (player->getIsAlive()) {
        m_order.targetNetId = 0;
        if (type > 9) {
            m_order.points[0] = Vec3{-4000.0f, -4000.0f, -4000.0f};
            m_order.orderId = 0;
        } else if (!useMap) m_order.orderId = type;
        if (!isInvalidOrderPosition(m_order.points[0])) {
            m_order.flag2C = true;
            const int squad = player->getSquadId();
            const int team = player->getTeam();
            static_cast<ISquadManager*>(squadManager)->issueRequest(team, squad, m_order, false);
        }
    }
}
} } }
#endif
