// [INFERRED] split of the attested Menu/BF2HudManager.cpp.
// BF2.exe 753DE0, all 5845 bytes read.
#include "BF2HudManager.h"
#if !DEDICATED
#include "HudKitSelection.h"
#include "HudQuestionManager.h"
#include "../Core/Debug.h"
#include "../Core/Ids.h"
#include "../Core/StringUtil.h"
#include "../Game/HudInformationLayer.h"
#include "../Game/IGame.h"
#include "../Game/IGameLogic.h"
#include "../Game/Main/BF2ClientSeams.h"
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/IObjectTemplate.h"
#include "../Game/Objects/IPlayer.h"
#include "../Game/Objects/VehicleHudCompTemplate.h"
#include "../Game/Objects/WeaponHudCompTemplate.h"
#include "../IO/Network/NetworkManager.h"
#include "../Scene/Object/PlayerManager.h"
#include "../Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../Sound/BfSound.h"
#include <cstring>
#include <utility>

namespace dice { namespace hfe {
extern IGameLogic* gameLogic;
namespace hud {
namespace {
// Scalar payload reads retain the shipped unaligned accesses. In particular,
// event 14 falls through to 3 without acquiring a KilledByData object lifetime.
template<class T> T eventValue(const void* data, unsigned offset = 0)
{
    T value;
    std::memcpy(&value, static_cast<const unsigned char*>(data) + offset, sizeof(value));
    return value;
}
const char* const hudSource = "C:\\dice\\Projects\\BF2Branches\\Patch_1_50\\Code\\BF2\\Menu\\BF2HudManager.cpp";

// [CHOSEN, not recovered] Team colour tokens for kill messages. #PLAYERCOLOR#, #PLAYERCOLOR1# and
// #PLAYERCOLOR2# expand to the four RGBA digits of a section-sign colour escape (Menu/ColorEscape.h):
// blue for the local player's team, red for any other team, white without a local player. Retail
// strings contain no such token, so they are unchanged. Local experiment, no issue yet.
std::string teamColorDigits(world::IPlayer* local, int team)
{
    if (!local) return "FFFF";
    return team == local->getTeam() ? "5AFF" : "F33F";
}
}

void BF2HudManager::handleGameLogicEvent(unsigned int id, EventNode*, void* data)
{
    using namespace world;
    switch (id) {
    case 8: {
        IPlayer* local = playerManager->getLocalHumanPlayer();
        if (!data || !local) return;
        const int localId = local->getId();
        if (localId != eventValue<int>(data)) return;
        const int pending = m_field11c->m_pendingTeam;
        const int team = eventValue<int>(data, 4);
        if (gameLogic && gameLogic->getGameStatus() == 1 && local->getIsAlive())
            m_field11c->m_field005 = true;
        const int displayed = team == -1 ? local->getTeam() : team;
        m_field11c->setDisplayedTeam(displayed);
        if (pending == -1) return;
        if (team == -1) {
            m_field11c->m_teamChangePending = false;
            return;
        }
        auto text = getLocalizedString(pending != team ? "HUD_HUD_MENU_TEAMS_EQUALIZE" : "HUD_TEXT_MENU_CHANGED_TEAM");
        text = replaceWideInputKeys(std::move(text));
        static_cast<HudInformationLayer*>(g_hudInformationLayer)->setDeathMessage(std::move(text));
        if (pending == team) {
            m_field11c->m_pendingTeam = team;
            m_field11c->suicide();
        }
        if (gameLogic) {
            const char* name = gameLogic->getTeamName(eventValue<int>(data, 4)).c_str();
            g_bfSound->playSound(formatString("%s_TeamSelect", name));
        }
        if (pending != team) {
            if (getMode() == 1) setDone(true);
        } else if (getMode() != 1) {
            setMode(1, true);
            m_field11c->setDisplayedTeam(team);
        }
        if (m_field100) m_field100->clear();
        refreshChatCategory(8);
        return;
    }
    case 14: {
        IPlayer* local = playerManager->getLocalHumanPlayer();
        IObject* object = io::networkManager->getObject(eventValue<unsigned short>(data));
        if (object && local && local->getDefaultVehicle() == object && getMode() == 1)
            setMode(0, true);
        // Retail falls through into kill-feed processing. ArmorGLComp posts only
        // two bytes here; the following larger payload reads are a shipped defect.
    }
    case 3: {
        IPlayer* local = playerManager->getLocalHumanPlayer();
        if (!data) return;
        std::string weaponName;
        IPlayer* victim = playerManager->getPlayerFromId(eventValue<unsigned char>(data, 1));
        IPlayer* killer = eventValue<unsigned char>(data, 2)
            ? playerManager->getPlayerFromId(eventValue<unsigned char>(data)) : nullptr;
        bool localKill = false;
        if (local && victim && !local->getIsCommander()) {
            const int localId = local->getId();
            localKill = localId == eventValue<unsigned char>(data) && eventValue<unsigned char>(data, 2);
        }
        if (localKill) {
            const int victimTeam = victim->getTeam();
            if (victimTeam != local->getTeam()) {
                const char* name = victim->getName().c_str();
                const int team = local->getTeam();
                std::string copiedName(name);
                auto text = getLocalizedString("HUD_CHAT_YOUGOTAKILLSCORE");
                text = replaceWideTokenFromString(std::move(text), "#PLAYERNAME#", std::move(copiedName));
                text = replaceWideTokenFromString(std::move(text), "#PLAYERCOLOR#",
                                                  teamColorDigits(local, victimTeam));
                text = replaceWideInputKeys(std::move(text));
                addServerChat(std::move(text), team);
            }
        }
        if (killer) {
            bool missingWeapon = false;
            if (victim) {
                const int victimId = victim->getId();
                if (victimId == killer->getId()) {
                    const int idAgain = victim->getId();
                    IPlayer* resolved = playerManager->getPlayerFromId(idAgain);
                    if (resolved) {
                        const char* name = resolved->getName().c_str();
                        const int team = resolved->getTeam();
                        std::string copiedName(name);
                        auto text = getLocalizedString("HUD_HUD_KILLEDBY_MYSELF");
                        text = replaceWideInputKeys(std::move(text));
                        text = replaceWideTokenFromString(std::move(text), "#PLAYERNAME#", std::move(copiedName));
                        addTeamChat(std::move(text), nullptr, team);
                    }
                    return;
                }
            }
            bool localVictim = false;
            if (local) {
                const int localId = local->getId();
                localVictim = localId == eventValue<unsigned char>(data, 1);
            }
            if (localVictim) {
                auto text = getLocalizedString("HUD_TEXT_MENU_YOU_WERE_KILLED");
                text = replaceWideInputKeys(std::move(text));
                static_cast<HudInformationLayer*>(g_hudInformationLayer)->setDeathMessage(std::move(text));
                const char* name = killer->getName().c_str();
                const int team = local->getTeam();
                std::string copiedName(name);
                text = getLocalizedString("HUD_CHAT_YOUWEREKILLEDBY");
                text = replaceWideTokenFromString(std::move(text), "#PLAYERNAME#", std::move(copiedName));
                text = replaceWideTokenFromString(std::move(text), "#PLAYERCOLOR#",
                                                  teamColorDigits(local, killer->getTeam()));
                text = replaceWideInputKeys(std::move(text));
                addServerChat(std::move(text), team);
            }
            IObjectTemplate* vehicleTemplate = objectTemplateManager->getTemplate(eventValue<unsigned int>(data, 8));
            IObjectTemplate* weaponTemplate = nullptr;
            if (vehicleTemplate) {
                weaponTemplate = objectTemplateManager->getTemplate(eventValue<unsigned int>(data, 12));
                if (weaponTemplate) {
                    weaponTemplate->queryImplementation(CID_GenericFireArmTemplate);
                    if (vehicleTemplate->getClassID() == CID_SoldierTemplate ||
                        vehicleTemplate->getClassID() == CID_LadderContainerTemplate ||
                        vehicleTemplate->getClassID() == CID_ParachuteTemplate) {
                        auto* component = weaponTemplate->queryComponent(IID_IWeaponHudCompTemplate);
                        if (component) weaponName = static_cast<WeaponHudCompTemplate*>(component)->getHudName();
                        else missingWeapon = true;
                    }
                }
                if (weaponName.empty() && !missingWeapon) {
                    auto* component = vehicleTemplate->queryComponent(IID_IVehicleHudCompTemplate);
                    if (component) weaponName = static_cast<VehicleHudCompTemplate*>(component)->getHudName();
                    else if (weaponTemplate) {
                        component = weaponTemplate->queryComponent(IID_IWeaponHudCompTemplate);
                        if (component) weaponName = static_cast<WeaponHudCompTemplate*>(component)->getHudName();
                    }
                }
            }
            if (weaponName.empty() && !vehicleTemplate && killer->getVehicle() && killer->getVehicle()->getTemplate()) {
                // The object exposes a const template, but retail dispatches
                // the template's non-const component query on this path.
                auto* component = const_cast<IObjectTemplate*>(killer->getVehicle()->getTemplate())->queryComponent(IID_IVehicleHudCompTemplate);
                if (component) weaponName = static_cast<VehicleHudCompTemplate*>(component)->getHudName();
            }
            if (!victim) return;
            const int victimTeam = victim->getTeam();
            const char* key = victimTeam != killer->getTeam()
                ? (!weaponName.empty() && !missingWeapon ? "HUD_HUD_KILLEDBY_KILLS" : "HUD_HUD_KILLEDBY_KILLED")
                : "HUD_HUD_KILLEDBY_TEAMKILLS";
            const char* victimName = victim->getName().c_str();
            const char* killerName = killer->getName().c_str();
            const int team = killer->getTeam();
            std::string copiedVictimName(victimName);
            auto weaponText = getLocalizedString(weaponName);
            std::string copiedKillerName(killerName);
            auto text = getLocalizedString(key);
            text = replaceWideTokenFromString(std::move(text), "#PLAYERNAME1#", std::move(copiedKillerName));
            text = replaceWideToken(std::move(text), "#WEAPON#", std::move(weaponText));
            text = replaceWideTokenFromString(std::move(text), "#PLAYERNAME2#", std::move(copiedVictimName));
            text = replaceWideTokenFromString(std::move(text), "#PLAYERCOLOR1#", teamColorDigits(local, team));
            text = replaceWideTokenFromString(std::move(text), "#PLAYERCOLOR2#",
                                              teamColorDigits(local, victimTeam));
            addTeamChat(std::move(text), killer, team);
        } else {
            if (!victim) return;
            weaponName = "HUD_TEXT_COMMANDER_ARTILLERY";
            const unsigned char code = eventValue<unsigned char>(data);
            if (code == 0xFD || code == 0xFE) {
                int team;
                const char* name;
                if (code == 0xFD) {
                    team = victim->getTeam() == 1 ? 2 : 1;
                    name = victim->getName().c_str();
                } else {
                    name = victim->getName().c_str();
                    team = victim->getTeam();
                }
                std::string copiedName(name);
                auto weaponText = getLocalizedString(weaponName);
                std::string killerName(" ");
                auto text = getLocalizedString(code == 0xFD ? "HUD_HUD_KILLEDBY_KILLS" : "HUD_HUD_KILLEDBY_TEAMKILLS");
                text = replaceWideTokenFromString(std::move(text), "#PLAYERNAME1#", std::move(killerName));
                text = replaceWideToken(std::move(text), "#WEAPON#", std::move(weaponText));
                text = replaceWideTokenFromString(std::move(text), "#PLAYERNAME2#", std::move(copiedName));
                text = replaceWideTokenFromString(std::move(text), "#PLAYERCOLOR1#",
                                                  teamColorDigits(local, team));
                text = replaceWideTokenFromString(std::move(text), "#PLAYERCOLOR2#",
                                                  teamColorDigits(local, victim->getTeam()));
                addTeamChat(std::move(text), victim, team);
            }
        }
        m_field2ec = victim->getId();
        return;
    }
    case 9: case 10: case 11: case 12: {
        IPlayer* local = playerManager->getLocalHumanPlayer();
        if (!local || (id != 11 && local->getIsCommander())) return;
        const int subtype = eventValue<int>(data);
        const char* key = nullptr;
        if (id == 9) {
            switch (subtype) {
            case 0: return;
            case 1: key = "HUD_HUD_ASSIST_KILL_TARGETING"; break;
            case 2: key = "HUD_HUD_ASSIST_KILL_KILL_DAMAGE"; break;
            case 3: key = "HUD_HUD_ASSIST_KILL_DRIVER_PASSENGER"; break;
            default: Debug(DEBUGTYPE_6, hudSource, 0x151A, "") << "Unknown Assist-type " << subtype; return;
            }
        } else if (id == 11) {
            const int targetId = eventValue<int>(data, 4);
            const int third = eventValue<int>(data, 8);
            if (targetId == local->getId()) {
                const std::string message("AUTO_MOODGP_friendlyfire");
                game->radioMessage(static_cast<TargetChannel>(14), message, local, third, true, true);
                return;
            }
            switch (subtype) {
            case 0: key = "HUD_HUD_TEAM_DAMAGE_PLAYER"; break;
            case 1: key = "HUD_HUD_TEAM_DAMAGE_VEHICLE"; break;
            default: Debug(DEBUGTYPE_6, hudSource, 0x152E, "") << "Unknown Teamdamage-type " << subtype; return;
            }
        } else if (id == 12) {
            switch (subtype) {
            case 0: key = "HUD_HUD_CONQUEST_FULL_CAPTURE"; break;
            case 1: key = "HUD_HUD_CONQUEST_DEFENDED_FLAG"; break;
            case 2: key = "HUD_HUD_CONQUEST_CAPTURE_ASSIST"; break;
            case 3: key = "HUD_HUD_CONQUEST_NEUTRALIZE"; break;
            case 4: key = "HUD_HUD_CONQUEST_NEUTRALIZE_ASSIST"; break;
            default: Debug(DEBUGTYPE_6, hudSource, 0x1548, "") << "Unknown Conquest-type " << subtype; return;
            }
        } else {
            switch (subtype) {
            case 0: key = "HUD_HUD_REPLENISH_GOT_HEALING_POINT"; break;
            case 1: key = "HUD_HUD_REPLENISH_GOT_REPAIR_POINT"; break;
            case 2: key = "HUD_HUD_REPLENISH_GOT_AMMO_POINT"; break;
            case 3: key = "HUD_HUD_REPLENISH_DRIVER"; break;
            case 4: key = "HUD_HUD_REPLENISH_REVIVE_SCORE"; break;
            case 5: key = "HUD_HUD_DESTROY_STRATEGIC"; break;
            default: Debug(DEBUGTYPE_6, hudSource, 0x1564, "") << "Unknown Replenish-type " << subtype; return;
            }
        }
        const int team = local->getTeam();
        auto text = getLocalizedString(key);
        text = replaceWideInputKeys(std::move(text));
        addServerChat(std::move(text), team);
        return;
    }
    case 4: {
        IPlayer* local = playerManager->getLocalHumanPlayer();
        const int team = eventValue<int>(data);
        if (local && local->getTeam() == team) g_bfSound->playSound(std::string("tickets_low"));
        return;
    }
    case 1: {
        const int status = eventValue<int>(data);
        if (status == 4) {
            showConnectionMessage(getLocalizedString("GAME_MESSAGE_GAME_PAUSED"));
            if (isCommanderShown()) setMode(15, true);
            setMode(0, true);
        } else if (status == 2) {
            IPlayer* local = playerManager->getLocalHumanPlayer();
            if (local) {
                if (isCommanderShown()) setMode(15, true);
                setMode(11, true);
                if (gameLogic) gameLogic->selectSpawnGroup(local, 0);
                local->enableVOHelp(false);
                playRoundResultSound();
            }
        } else if (status == 1) {
            if (m_field324 != 4) {
                resetHudState();
                IPlayer* local = playerManager->getLocalHumanPlayer();
                if (!local || !local->getIsAlive()) setMode(1, true);
            }
            clearConnectionMessage();
        }
        m_field324 = status;
        return;
    }
    default: return;
    }
}
} } }
#endif
