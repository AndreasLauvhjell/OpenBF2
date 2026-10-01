// Radio integration. [INFERRED] Source split from Menu/BF2HudManager.cpp.
#include "BF2HudManager.h"
#if !DEDICATED
#include "HudRadio.h"
#include "Bf2MenuMapNode.h"
#include "../Core/Ids.h"
#include "../Game/IGameLogic.h"
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/IObjectTemplate.h"
#include "../Game/Objects/IPlayer.h"
#include "../Game/Objects/IVehicleHudComp.h"
#include "../IO/Network/INetworkable.h"
#include "../Scene/Object/PlayerManager.h"
#include "../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
namespace dice { namespace hfe { namespace hud {
namespace {
// PE440060; inferred local spelling. No ownership transfer or addRef.
world::IPlayerControlObject* findParentControlObject(world::IObject* object)
{
    while (object) {
        auto* control = static_cast<world::IPlayerControlObject*>(
            object->queryInterface(world::IID_IPlayerControlObject));
        if (control) return control;
        object = object->getParent();
    }
    return nullptr;
}

// PE758430; this is unused in every instruction, so this split uses a local
// helper. The order classification and its name are inferred; the integer
// values and repeated team reads are direct evidence.
int classifyRadioTarget(world::IObject* object)
{
    int order = 4;
    if (!object) return order;
    world::IPlayer* local = world::playerManager->getLocalHumanPlayer();
    world::IPlayerControlObject* control = findParentControlObject(object);
    if (!control) return order;
    const std::map<int, world::IPlayerControlObject*> pcos(*control->getPcos());
    for (auto it = pcos.begin(); it != pcos.end(); ++it) {
        // Unlike openRadio, the classifier does not guard a null map value.
        world::IPlayer* occupant = it->second->getOccupingPlayer();
        if (occupant) {
            const int team = occupant->getTeam();
            if (team == local->getTeam()) order = 6;
            else {
                const int repeatedTeam = occupant->getTeam();
                if (repeatedTeam != local->getTeam()) order = 5;
            }
        }
    }
    return order;
}
}

void BF2HudManager::openRadio(bool alternate)
{
    world::IPlayer* local = world::playerManager->getLocalHumanPlayer();
    world::IPlayer* occupant = nullptr;
    m_field1e4 = 0;
    if (local) {
        world::IObject* target;
        if (findSpottedTarget(target, m_field1e8, local->getViewDistance(), true) && target) {
            world::IPlayerControlObject* control = findParentControlObject(target);
            if (control) {
                auto* pcos = control->getPcos();
                for (auto it = pcos->begin(); it != pcos->end(); ++it) {
                    if (it->second) {
                        world::IPlayer* found = it->second->getOccupingPlayer();
                        if (found) occupant = found;
                    }
                }
                if (occupant) {
                    const int team = occupant->getTeam();
                    if (team != local->getTeam()) {
                        auto* component = static_cast<world::IVehicleHudComp*>(target->m_vehicleHudComponent);
                        if (component && component->getHudInfo()->usePlayerIcon)
                            target = world::getRootParent(occupant->getDefaultVehicle());
                        io::INetworkable* networkable = target->getNetworkable();
                        if (networkable) m_field1e4 = networkable->m_networkableId;
                    }
                }
            }
        }
    }
    if (alternate) {
        setMode(31, true);
        return;
    }
    if (!local) return;
    bool vehicleRadio = true;
    world::IObject* vehicle = local->getVehicle();
    if (vehicle && vehicle->m_vehicleHudComponent)
        vehicleRadio = static_cast<world::IVehicleHudComp*>(vehicle->m_vehicleHudComponent)->getHudInfo()->useVehicleCommRose;
    world::IObject* defaultVehicle = local->getDefaultVehicle();
    if (local->getVehicle() != defaultVehicle && m_field10c->isEnabled() && vehicleRadio)
        setMode(5, true);
    else if (m_field108->isEnabled())
        setMode(4, true);
    enableVOHelpMessage(50);
}

void BF2HudManager::openSquadRadio(bool alternate)
{
    world::IPlayer* local = world::playerManager->getLocalHumanPlayer();
    if (alternate) {
        setMode(30, true);
        return;
    }
    world::IObject* target;
    m_order.targetNetId = 0;
    if (findSpottedTarget(target, m_field1e8, 1000.0f, false)) {
        m_order.orderId = 4;
        updateOrderPositionColor(m_field1e8);
        world::IObject* root = world::getRootParent(target);
        if (root) {
            auto* invalidObjects = gameLogic->GetInvalidVehicleDropObjectList();
            for (auto it = invalidObjects->begin(); it != invalidObjects->end(); ++it) {
                if (root->getTemplate() && it->first == root->getTemplate()->getName() && it->second) {
                    m_field35c = true;
                    m_field350 = 0.6f;
                    m_field354 = 0.6f;
                    m_field358 = 0.6f;
                }
            }
        }
        if (target) {
            io::INetworkable* networkable = target->getNetworkable();
            if (networkable) {
                m_order.targetNetId = networkable->m_networkableId;
                m_order.orderId = classifyRadioTarget(target);
            }
        }
    }
    m_order.points[0] = m_field1e8;
    Vec2 coordinates{0.0f, 0.0f};
    meme::worldCoordinatesToMap(m_field1e8, coordinates);
    hudMap->m_commanderPosition = coordinates;
    if (!local) return;
    if (local->getIsSquadLeader()) {
        if (!m_field114->isActive()) {
            setMode(7, true);
            enableVOHelpMessage(65);
        }
    } else if (local->getIsCommander()) {
        if (!m_field118->isActive()) setMode(8, true);
    } else if (static_cast<unsigned int>(local->getSquadId()) > 0) {
        if (!m_field110->isActive()) setMode(3, true);
    }
}

} } }
#endif
