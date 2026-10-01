// Camera integration. [INFERRED] Source split from Menu/BF2HudManager.cpp.
#include "BF2HudManager.h"
#if !DEDICATED
#include "Bf2MenuMapNode.h"
#include "../Core/Ids.h"
#include "../Game/IGame.h"
#include "../Game/IGameLogic.h"
#include "../Game/GameEvents/CommanderCamEvent.h"
#include "../Game/Objects/IPlayer.h"
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/OverheadCameraTemplate.h"
#include "../Game/GameLogic/SquadManager/ISquadManager.h"
#include "../Scene/Object/PlayerManager.h"
#include <new>
namespace dice { namespace hfe {
extern IBase* squadManager;
namespace hud {

float BF2HudManager::getCommanderCameraHeight()
{
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    if (player && player->getOverheadCameraEnabled()) {
        world::IObject* camera = static_cast<world::IObject*>(
            static_cast<ISquadManager*>(squadManager)->getCommanderCam(player->getTeam()));
        if (camera && camera->getTemplate()->getClassID() == world::CID_OverheadCameraTemplate) {
            // Retail guards only this second template read, after its class-ID callback.
            const auto* objectTemplate = static_cast<const world::OverheadCameraTemplate*>(camera->getTemplate());
            if (objectTemplate) m_field2bc = objectTemplate->m_height;
        }
    }
    return m_field2bc;
}

bool BF2HudManager::setCommanderCameraPosition(Vec2 position)
{
    world::IPlayer* player = world::playerManager->getLocalHumanPlayer();
    if (!player) return false;
    if (!gameLogic->isServer()) {
        void* storage = ::operator new(sizeof(CommanderCamEvent));
        CommanderCamEvent* event = storage ? new(storage) CommanderCamEvent(
            Vec3{position.x, m_field2bc, position.y}) : nullptr;
        const int id = player->getId();
        game->sendGameEvent(id, event);
    } else {
        CommanderCamEvent event(Vec3{position.x, m_field2bc, position.y});
        event.executeServer(player->getId());
    }
    // The server event's own false result is deliberately ignored by this caller.
    return true;
}

Vec2 BF2HudManager::getCommanderPosition(bool relative)
{
    Vec3 position(Vec3::getZero());
    Vec2 center{0.0f, 0.0f};
    if (hudMap) {
        const Vec2 coordinates(hudMap->m_commanderPosition);
        meme::mapCoordinatesToWorld(position, coordinates);
    }
    if (relative) {
        // Retail leaves the map pointer and zoom index unchecked in this arm.
        center = hudMap->m_centers[hudMap->m_zoomLevel];
    }
    return Vec2{position.x - center.x, position.z - center.y};
}

bool BF2HudManager::zoomCommander()
{
    if (!hudMap) return false;
    hudMap->cycleZoom();
    setCommanderCameraPosition(getCommanderPosition(true));
    return true; // The camera-position result is ignored in the retail caller.
}

} } }
#endif
