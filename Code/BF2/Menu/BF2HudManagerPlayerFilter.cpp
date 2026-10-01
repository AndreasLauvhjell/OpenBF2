// HUD player-input filtering. [INFERRED] Source split.
#include "BF2HudManager.h"
#if !DEDICATED
#include "HudMessages.h"
#include "HudVehicleInformation.h"
#include "HudInformationAdapter.h"
#include "HudRadio.h"
#include "Ref2System.h"
#include "Bf2MenuMapNode.h"
#include "../IO/Input/PlayerInput.h"
#include "../Core/EventManager.h"
#include "../Core/System.h"
#include "../Core/Ids.h"
#include "../Game/IGame.h"
#include "../Game/IGameLogic.h"
#include "../Game/Main/BF2ClientSeams.h"
#include "../Game/Objects/IPlayer.h"
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/IVehicleHudComp.h"
#include "../Scene/Object/PlayerManager.h"
#include "../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include <cstring>

namespace dice { namespace hfe { namespace hud {
namespace {
// The signed upper guard does not guard negative array indices. The
// PE shift helper observes only CL and returns zero for counts64..255.
bool playerInputPressed(const io::PlayerInput& input, int index)
{
    if (index >= 64) return false;
    const unsigned shift = static_cast<unsigned char>(index);
    return shift < 64 && (input.mappingFlags & (1ULL << shift))
        && input.inputs[index] > 0.5f;
}

// first matching interface while walking the actual parent chain.
world::IPlayerControlObject* playerControlAncestor(world::IObject* object)
{
    while (object) {
        auto* control = static_cast<world::IPlayerControlObject*>(
            object->queryInterface(world::IID_IPlayerControlObject));
        if (control) return control;
        object = object->getParent();
    }
    return nullptr;
}
}

bool BF2HudManager::filterPlayerInput(io::PlayerInput& input)
{
    if (m_field0bf || !m_field248 || !game || gameLogic->getGameStatus() != 1)
        return true;

    if (isChatActive() || isSquadCreateActive()) {
        // Only the float array is cleared; all mapping/tail bytes survive.
        // Axes4..7 are copied as raw words, preserving their NaN payloads.
        std::uint32_t preserved[4] = {};
        for (int i = 0; i != 4; ++i)
            if (input.mappingFlags & (1ULL << (i + 4)))
                std::memcpy(&preserved[i], &input.inputs[i + 4], sizeof(float));
        if (m_field2fc) {
            m_field300 = (input.mappingFlags & 8) ? input.inputs[3] : 0.0f;
            m_field2fc = false;
        }
        std::memset(input.inputs, 0, sizeof(input.inputs));
        if (!isPointerEnabled()) {
            if (input.mappingFlags & 8) input.inputs[3] = m_field300;
            if (!isSquadCreateActive()) {
                for (int i = 0; i != 4; ++i)
                    if (input.mappingFlags & (1ULL << (i + 4)))
                        std::memcpy(&input.inputs[i + 4], &preserved[i], sizeof(float));
            }
        }
        return true;
    }

    if (hudVehicleInformation) hudVehicleInformation->setMouseLook(playerInputPressed(input, 11));
    bool canSelect = true;
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    if (player) {
        world::IObject* vehicle = player->getVehicle();
        if (vehicle) {
            if (auto* control = playerControlAncestor(vehicle)) {
                // PE+74 (5F67D0), ELF+70 (819D6D0): native overload ordering
                // differs, so dispatch the actual typed active-weapon method.
                if (world::IObject* weapon = control->getActiveWeapon(0))
                    if (weapon->getFlags() & 1) canSelect = false;
            }
            if (vehicle->m_vehicleHudComponent)
                canSelect = static_cast<world::IVehicleHudComp*>(
                    vehicle->m_vehicleHudComponent)->getHudInfo()->useSelectionIcons;
            if (game && game->isPlayingDemo()) canSelect = false;
        }
    } else {
        canSelect = false;
    }
    if (hudVehicleInformation)
        hudVehicleInformation->m_unknown28c = (input.mappingFlags & 8) ? input.inputs[3] : 0.0f;

    if ((playerInputPressed(input, 60) || playerInputPressed(input, 61))
        && (getMode() == 18 || getMode() == 19 || getMode() == 2
            || getMode() == 15 || getMode() == 21 || getMode() == 22
            || getMode() == 23 || getMode() == 24 || getMode() == 25
            || getMode() == 17 || getMode() == 20)) {
        const int previous = m_field104->getInt(std::string("MapZoom"));
        int next = previous;
        bool adjust = false;
        if (playerInputPressed(input, 60)) {
            next = static_cast<int>(static_cast<unsigned>(previous) + 1u);
            adjust = true;
        } else if (playerInputPressed(input, 61)) {
            next = static_cast<int>(static_cast<unsigned>(previous) - 1u);
            adjust = true;
        }
        if (adjust) {
            if (next > 2) next = 2;
            else if (next == -1) next = 0;
            if (next >= 0 && next != previous
                && ref2System->getPointerOwner() == hudMap) {
                m_field104->setMapZoom(next);
                eventManager->postEvent(ECHud, 9, this, 0);
                hudManager->disableVOHelpMessage(57);
            }
        }
    }

    if (player) {
        world::IObject* defaultVehicle = player->getDefaultVehicle();
        if (player->getVehicle() == defaultVehicle) {
            if (playerInputPressed(input, 32)) disableVOHelpMessage(3);
            if (playerInputPressed(input, 3)) disableVOHelpMessage(1);
            m_field2f5 = playerInputPressed(input, 3);
            if (!m_field2f5) m_field2f8 = 0.0f;
            if (m_field2f8 > 5.0f) {
                enableVOHelpMessage(70);
                m_field2f8 = 0.0f;
            }
            if (playerInputPressed(input, 4) || playerInputPressed(input, 5)) disableVOHelpMessage(0);
            if (playerInputPressed(input, 9)) disableVOHelpMessage(5);
            if (playerInputPressed(input, 38)) disableVOHelpMessage(6);
            if (playerInputPressed(input, 37)) {
                disableVOHelpMessage(7);
                disableVOHelpMessage(71);
            }
            if (playerInputPressed(input, 0)) disableVOHelpMessage(8);
        }
    }
    if (isCommanderShown()) {
        // fucom parity accepts unordered values as nonzero here.
        if (((input.mappingFlags & 8) && input.inputs[3] != 0.0f)
            || ((input.mappingFlags & 1) && input.inputs[0] != 0.0f)
            || ((input.mappingFlags & 0x2000) && input.inputs[13] != 0.0f))
            m_field2b8 = m_field2b4;
    }
    const int latched = m_field328;
    if (playerInputPressed(input, latched))
        input.setInput(static_cast<io::PlayerInputMap>(latched), 0.0f);
    else
        m_field328 = 64;

    if (getMode() == 7 || getMode() == 4 || getMode() == 6
        || getMode() == 5 || getMode() == 30 || getMode() == 31) {
        for (int i = 0; i != 64; ++i)
            if (i != 3 && i != 0 && i != 38 && i != 13 && i != 37 && i != 9)
                input.setInput(static_cast<io::PlayerInputMap>(i), 0.0f);
        return true;
    }
    if (!isCommanderShown() && getMode() != 0 && getMode() != 9
        && getMode() != 10 && getMode() != 2) {
        for (int i = 0; i != 64; ++i)
            if (i != 3 && i != 0 && i != 38 && i != 13)
                input.setInput(static_cast<io::PlayerInputMap>(i), 0.0f);
        return false;
    }

    if (playerInputPressed(input, 8)) {
        if (getMode() == 10) {
            int index = static_cast<int>(static_cast<unsigned>(m_field148->confirmSelection()) + 13u);
            if (index < 14) index = 14;
            else if (index > 22) index = 22;
            input.setInput(static_cast<io::PlayerInputMap>(index), true);
            if (input.mappingFlags & 0x100) input.inputs[8] = 0.0f;
            setMode(0, false);
            const double now = WorldPref::getWorldTime();
            HudMessages* messages = m_field148;
            messages->m_field028 = now + messages->m_field020;
            return true;
        }
        if (getMode() == 0 || getMode() == 9 || getMode() == 2) {
            const double readyAt = m_field148->m_field028;
            // 753590 test AH,5 accepts unordered along with >=.
            if (!(WorldPref::getWorldTime() < readyAt)
                && m_field108->isEnabled() && m_field10c->isEnabled()
                && m_field110->isEnabled() && m_field114->isEnabled()
                && m_field118->isEnabled()) {
                disableVOHelpMessage(2);
                return true;
            }
        }
        if (input.mappingFlags & 0x100) input.inputs[8] = 0.0f;
        return true;
    }

    if (canSelect && (getMode() == 0 || getMode() == 10) && m_field148) {
        if (playerInputPressed(input, 60)) {
            if (getMode() != 10) {
                m_field148->m_duration = 2.0f;
                setMode(10, true);
            }
            m_field148->selectNext(0);
        } else if (playerInputPressed(input, 61)) {
            if (getMode() != 10) {
                m_field148->m_duration = 2.0f;
                setMode(10, true);
            }
            m_field148->selectPrevious(0);
        } else if (playerInputPressed(input, 14) || playerInputPressed(input, 15)
            || playerInputPressed(input, 16) || playerInputPressed(input, 17)
            || playerInputPressed(input, 18) || playerInputPressed(input, 19)) {
            int item = 0;
            if (playerInputPressed(input, 14)) item = 1;
            else if (playerInputPressed(input, 15)) item = 2;
            else if (playerInputPressed(input, 16)) item = 3;
            else if (playerInputPressed(input, 17)) item = 4;
            else if (playerInputPressed(input, 18)) item = 5;
            else if (playerInputPressed(input, 19)) item = 6;
            else if (playerInputPressed(input, 21)) item = 8;
            if (!m_field148->m_itemCount) m_field148->refreshItems();
            if (m_field148->getSelectedItem() != item && item <= m_field148->m_itemCount
                && getMode() != 10) {
                m_field148->m_duration = 0.4f;
                setMode(10, true);
            }
        }
        return true;
    }
    if (isCommanderShown()
        && (playerInputPressed(input, 14) || playerInputPressed(input, 15)
            || playerInputPressed(input, 16) || playerInputPressed(input, 17)
            || playerInputPressed(input, 18) || playerInputPressed(input, 19)
            || playerInputPressed(input, 20) || playerInputPressed(input, 21))) {
        int index = 0;
        if (!playerInputPressed(input, 14)) {
            if (playerInputPressed(input, 15)) index = 1;
            else if (playerInputPressed(input, 16)) index = 2;
            else if (playerInputPressed(input, 17)) index = 3;
            else if (playerInputPressed(input, 18)) index = 4;
            else if (playerInputPressed(input, 19)) index = 5;
            else if (playerInputPressed(input, 20)) index = 6;
        }
        input.setInput(static_cast<io::PlayerInputMap>(index + 14), false);
    }
    return true;
}
} } }
#endif
