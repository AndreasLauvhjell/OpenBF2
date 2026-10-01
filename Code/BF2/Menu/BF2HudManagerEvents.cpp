// HUD event registration and dispatch. [INFERRED] Source split.
#include "BF2HudManager.h"
#if !DEDICATED
#include "Bf2MenuMapNode.h"
#include "HudChat.h"
#include "HudCommander.h"
#include "HudInformationAdapter.h"
#include "HudKitSelection.h"
#include "HudMessages.h"
#include "HudQuestionManager.h"
#include "HudScoreboard.h"
#include "HudSquad.h"
#include "HudVoip.h"
#include "HudVoteMap.h"
#include "HudVehicleInformation.h"
#include "../AI/AIInterface/AIInstance.h"
#include "../Core/EventManager.h"
#include "../Core/StringUtil.h"
#include "../Game/IGame.h"
#include "../Game/IGameLogic.h"
#include "../Game/GameLogic/SquadManager/ISquadManager.h"
#include "../Game/Objects/ICameraObject.h"
#include "../Sound/NullSound.h"                 // ISoundEngine +0x70/+0x74, ISound +0x00/+0x04
#include "../Game/HudInformationLayer.h"
#include "../Game/Main/BF2ClientSeams.h"
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/IPlayer.h"
#include "../IO/Network/INetworkable.h"
#include "../Scene/Object/PlayerManager.h"
#include "../Sound/BfSound.h"
#include <cstring>
#include <utility>

namespace dice { namespace hfe {
extern IGameLogic* gameLogic;
extern IBase* squadManager;                      // ds:99EF84
extern hud::BF2HudManager* hudManager;
namespace hud {

void BF2HudManager::configureMap(const std::string& name)
{
    if (!hudMap || !gameLogic) return;
    auto texture = formatString("Levels/%s/Hud/Minimap/ingameMap.tga", name.c_str());
    hudMap->setMinimapTexture(std::move(texture));
    texture = formatString("Ingame/Flags/Icons/Minimap/%s/miniMap_flag.tga", gameLogic->getTeamName(1).c_str());
    hudMap->setTeamFlagTexture(std::move(texture), 1);
    texture = formatString("Ingame/Flags/Icons/Minimap/%s/miniMap_flag.tga", gameLogic->getTeamName(2).c_str());
    hudMap->setTeamFlagTexture(std::move(texture), 2);
    hudMap->setControlPointTexture("Ingame/Flags/Icons/Minimap/Neutral/miniMap_CP.tga", 0);
    texture = formatString("Ingame/Flags/Icons/Minimap/%s/miniMap_CP.tga", gameLogic->getTeamName(1).c_str());
    hudMap->setControlPointTexture(std::move(texture), 1);
    texture = formatString("Ingame/Flags/Icons/Minimap/%s/miniMap_CP.tga", gameLogic->getTeamName(2).c_str());
    hudMap->setControlPointTexture(std::move(texture), 2);
    texture = formatString("Ingame/Flags/Icons/Minimap/%s/miniMap_CPBase.tga", gameLogic->getTeamName(1).c_str());
    hudMap->setControlPointBaseTexture(std::move(texture), 1);
    texture = formatString("Ingame/Flags/Icons/Minimap/%s/miniMap_CPBase.tga", gameLogic->getTeamName(2).c_str());
    hudMap->setControlPointBaseTexture(std::move(texture), 2);
}

void BF2HudManager::resetHudState()
{
    setMenuLoading(false);
    if (m_field140) m_field140->resetRows();
    if (hudChat) hudChat->resetRows();
    if (m_field11c) m_field11c->reset();
    if (m_field100) m_field100->clear();
    clearDisconnectMessage();
    clearConnectionMessage();
    clearDeathMessage();
    if (m_field13c) {
        m_field13c->setArtilleryReloading(true);
        m_field13c->setSuppliesReloading(true);
        m_field13c->setVehicleDropReloading(true);
    }
    if (hudMap) {
        hudMap->setAirstrikeMarker(Vec3{0, 0, 0}, 0);
        hudMap->setUAVMarker(Vec3{0, 0, 0}, 0);
        hudMap->resetSatelliteReload();
        hudMap->resetUAVReload();
        hudMap->clearVehicleDrops();
    }
    if (m_field104) {
        m_field104->setBool("IngameHelpShow", false);
        m_field104->setBool("CommanderShow", false);
    }
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    if (player) {
        const int team = player->getTeam();
        m_field104->setTeam(team);
    }
    if (m_field148) m_field148->m_field028 = m_field148->m_field020;
    const auto& players = world::playerManager->getPlayers();
    for (auto it = players.begin(); it != players.end(); ++it)
        if (*it) (*it)->resetSpamFilter();
}

void BF2HudManager::handleEvent(EventCategory category, unsigned int id,
                              EventNode* node, void* data)
{
    switch (category) {
    case ECHud: handleHudEvent(id, node, data); return;
    case ECGame: handleGameEvent(id, node, data); return;
    case ECGameLogic: handleGameLogicEvent(id, node, data); return;
    case static_cast<EventCategory>(10): break;
    default: return;
    }
    if (id == 3) {
        if (hudMap) hudMap->releaseMinimapTexture();
        if (m_field140) m_field140->resetRows();
        if (hudChat) hudChat->resetRows();
        if (m_field11c) m_field11c->reset();
    } else if (id == 9) {
        m_field320 = true;
        resetHudState();
        hudVehicleInformation->m_killedState = false;
        if (isCommanderShown()) setMode(15, true);
        setMode(0, true);
        closeChat();
    } else if (id == 1) {
        if (isCommanderShown()) setMode(15, true);
        if (m_field100) {
            m_field100->replyAll(false);
            m_field100->clear();
        }
        setMode(0, true);
        closeChat();
        clearDisconnectMessage();
        // MSVC groups each overloaded name, reversing its overloads: this
        // string overload is PE+34 (the ELF table's corresponding slot is+30).
        g_bfSound->stopSound(m_field304);
    } else if (id == 2) {
        hudManager->setMenuLoading(false);
        eventManager->postEvent(ECHud, 2, this, 0.0f);
        if (gameLogic) {
            const bool hadAI = ai::g_aiInstance != nullptr;
            if (gameLogic->getGameStatus() == 1 ||
                (hadAI && gameLogic->getGameStatus() == 4))
                eventManager->postEvent(ECHud, 67, nullptr, 4.0f);
        }
    }
}

void BF2HudManager::handleGameEvent(unsigned int id, EventNode*, void* data)
{
    world::IPlayer* local = world::playerManager->getLocalHumanPlayer();
    if (id == 11) {
        auto* player = static_cast<world::IPlayer*>(data);
        if (local && player) {
            const int localId = local->getId();
            if (player->getId() == localId && m_field148)
                m_field148->m_visible = false; // complete 46CA90 byte store
        }
    } else if (id == 8) {
        if (!local) return;
        // GameLogic::killPlayer posts the same eight-byte buffer used by
        // PostRemote: int player id, Boolean byte, three unused padding bytes.
        // Read its wire fields without inventing a C++ payload object's lifetime.
        int playerId;
        std::memcpy(&playerId, data, sizeof(playerId));
        auto* player = world::playerManager->getPlayerFromId(playerId);
        if (!player) return;
        int team = player->getTeam();
        if (m_field248) {
            const int id = player->getId();
            if (id == local->getId()) {
                clearConnectionMessage();
                if (!m_field11c->m_field005) {
                    auto text = replaceWideInputKeys(getLocalizedString("HUD_TEXT_MENU_YOU_WERE_KILLED"));
                    static_cast<HudInformationLayer*>(g_hudInformationLayer)->setDeathMessage(std::move(text));
                } else {
                    m_field11c->m_field005 = false;
                    team = team == 2 ? 1 : 2;
                }
                if (getMode() != 11) onSpawn();
            }
        }
        if (static_cast<const unsigned char*>(data)[4]) {
            // MSVC evaluates the by-value replacement name before localization.
            // Each inner callee owns/destroys its argument before the next call.
            addTeamChat(replaceWideTokenFromString(
                replaceWideInputKeys(getLocalizedString("HUD_HUD_KILLEDBY_MYSELF")),
                "#PLAYERNAME#", std::string(player->getName().c_str())), nullptr, team);
        }
        m_field2ec = -1;
    } else if (id == 6) {
        auto* player = static_cast<world::IPlayer*>(data);
        if (!player || !local || !m_field248) return;
        const int localId = local->getId();
        if (player->getId() != localId) return;
        eventManager->postEvent(ECHud, 31, this, 0.0f);
        setDone(true);
        m_field11c->m_teamChangePending = false;
        if (m_field11c->m_pendingTeam == -1) {
            const int team = local->getTeam();
            m_field11c->m_pendingTeam = team;
        }
        world::IObject* vehicle = player->getVehicle();
        if (vehicle) {
            io::INetworkable* networkable = vehicle->getNetworkable();
            if (networkable) hudMap->removeTimedMarker(networkable->m_networkableId);
        }
    }
}

// Leaves the previous HUD mode before entering the requested mode.
void BF2HudManager::applyModeTransition()
{
    world::IPlayer* const local = world::playerManager->getLocalHumanPlayer();

    // phase 1: leaving the old mode -----------------------------------------------------
    switch (getPreviousMode())
    {
    case 0:
        // The VoIP flag is cleared only for a player who is in a squad or commanding.
        if (local && m_field124 && (local->getSquadId() || local->getIsCommander()))
            m_field124->m_enabled = false;
        break;
    case 10:
        if (m_field148) m_field148->m_visible = false;   // the five-byte 46CA90 store
        break;
    case 11:
        m_field104->setBool("VictoryShow", false);
        m_field104->setBool("VictoryRankShow", false);
        disablePointer();
        break;
    case 12:
        disablePointer();
        if (m_field120) m_field120->update(false);
        if (m_field128) m_field128->show(false);
        break;
    case 9:
        if (m_field120) m_field120->update(false);
        if (m_field128) m_field128->show(false);
        break;
    case 14:
        closeChat();
        break;
    case 15:
        if (m_field13c)
        {
            m_field13c->update(false);
            if (!game->getCurrentMapName().empty() && !isCommanderMode(getMode()) && local)
            {
                configureMap(game->getCurrentMapName());
                if (squadManager)
                    static_cast<ISquadManager*>(squadManager)
                        ->setCommanderToSoldierMode(local->getId());
                if (world::ICameraObject* const camera = local->getCamera())
                    camera->setViewMode(camera->getViewMode(), 0, true);
                if (m_field2f4)
                {
                    m_field2f4 = false;
                    g_bfSound->engine()->setTrigger(m_field2f0);
                    if (m_field2f0) { m_field2f0->release(); m_field2f0 = 0; }
                }
            }
        }
        disablePointer();
        break;
    case 17:
        if (m_field138 && getMode() != getPreviousMode() && getMode() != 20
            && getMode() != 9 && getMode() != 13 && getMode() != 14)
            m_field138->setVisible(false);
        disablePointer();
        break;
    case 18:
        if (m_field138 && getMode() != getPreviousMode() && getMode() != 19
            && getMode() != 9 && getMode() != 13 && getMode() != 14)
            m_field138->setVisible(false);
        if (getMode() != 13) disablePointer();
        break;
    case 1: case 3: case 4: case 5: case 6: case 7: case 8: case 16: case 19:
    case 20: case 21: case 22: case 23: case 24: case 25: case 27: case 28:
    case 29: case 30: case 31:
        disablePointer();
        break;
    default:                       // 2, 13, 26 do nothing at all
        break;
    }

    // phase 2: entering the new mode -----------------------------------------------------
    switch (getMode())
    {
    case 0:
        if (local && m_field124 && (local->getSquadId() || local->getIsCommander()))
            m_field124->m_enabled = true;
        break;
    case 1:
        if (m_field11c)
        {
            m_field11c->setSpawnGroup(m_field11c->m_selectedSpawnGroup[m_field11c->m_selectedTeam]);
            hudMap->m_spawnGroup =
                m_field11c->m_selectedSpawnGroup[m_field11c->m_selectedTeam];
        }
        enablePointer(m_field168, getMode());
        m_field228 = true;
        m_field320 = false;
        break;
    case 2:  disableVOHelpMessage(73); break;
    case 9:  disableVOHelpMessage(74); break;
    case 3:
        if (activateRadio(m_field110)) enablePointer(m_field164, getMode());
        else setMode(getPreviousMode(), false);
        break;
    case 7:
        if (activateRadio(m_field114)) enablePointer(m_field164, getMode());
        else setMode(getPreviousMode(), false);
        break;
    case 8:
        if (activateRadio(m_field118)) enablePointer(m_field164, getMode());
        else setMode(getPreviousMode(), false);
        break;
    case 4:
        if (activateRadio(m_field108))
        {
            enablePointer(m_field164, getMode());
            disableVOHelpMessage(64);
        }
        else setMode(getPreviousMode(), false);
        break;
    case 5:
        if (activateRadio(m_field10c))
        {
            enablePointer(m_field164, getMode());
            disableVOHelpMessage(64);
        }
        else setMode(getPreviousMode(), false);
        break;
    case 6:
        enablePointer(m_field164, getMode());
        disableVOHelpMessage(50);
        cacheMouseInput(false);
        break;
    case 10:
        // The only caller of HudMessages::showItems, and it reverts the mode when that answers
        // false - which is what pins showItems' return to the new m_visible.
        if (m_field148)
        {
            if (m_field148->showItems()) disableVOHelpMessage(72);
            else setMode(getPreviousMode(), false);
        }
        break;
    case 11:
        m_field120->updateFinalList();
        // m_field230 latches which of the two Victory flags is raised, and the arms are not
        // symmetric: the true arm clears the latch, the false arm leaves it standing.
        if (m_field230)
        {
            m_field104->setBool("VictoryRankShow", true);
            m_field104->setBool("VictoryShow", false);
            m_field230 = false;
        }
        else
        {
            m_field104->setBool("VictoryRankShow", false);
            m_field104->setBool("VictoryShow", true);
        }
        break;
    case 15:
        if (m_field13c && local && local->getIsAlive())
        {
            m_field13c->update(true);
            enablePointer(m_field168, getMode());
            if (!wasCommanderMode())
            {
                configureMap(game->getCurrentMapName());
                if (gameLogic && !m_field2f4)
                {
                    m_field2f4 = true;
                    m_field2f0 = g_bfSound->engine()->getTrigger();
                    if (m_field2f0) m_field2f0->addRef();
                    // engine() is called a second time here rather than reused.
                    ISoundEngine* const engine = g_bfSound->engine();
                    engine->setTrigger(static_cast<ISound*>(g_bfSound->getSound(
                        formatString("Commander_%s_ambience",
                                     gameLogic->getTeamName(local->getTeam()).c_str()))));
                }
            }
        }
        break;
    case 16:
        disableVOHelpMessage(40);
        enablePointer(m_field168, getMode());
        break;
    case 20:
        disableVOHelpMessage(42);
        enablePointer(m_field168, getMode());
        break;
    case 21:
        disableVOHelpMessage(27);
        enablePointer(m_field168, getMode());
        break;
    case 17:
    case 18:
        if (local && m_field138)
        {
            m_field138->setVisible(true);
            enablePointer(m_field168, getMode());
            disableVOHelpMessage(62);
        }
        break;
    case 22:
        enableVOHelpMessage(16);
        disableVOHelpMessage(31);
        enablePointer(m_field16c, getMode());
        break;
    case 23:
        enableVOHelpMessage(18);
        disableVOHelpMessage(30);
        enablePointer(m_field170, getMode());
        break;
    case 24:
        enableVOHelpMessage(17);
        disableVOHelpMessage(32);
        enablePointer(m_field174, getMode());
        break;
    case 25:
        enablePointer(m_field174, getMode());
        break;
    case 30:
    case 31:
        enablePointer(m_field164, getMode());
        break;
    case 12: case 19: case 26: case 27: case 28: case 29:
        enablePointer(m_field168, getMode());
        break;
    case 13:
    case 14:
        break;
    default:
        disablePointer();
        break;
    }
}

} } }
#endif
