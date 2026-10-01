#include "GrapplingHookContainerCreateEvent.h"

#include <string>

#include "../IGame.h"                            // game->queryInterface(IID_IGameServer)
#include "../Objects/IObject.h"
#include "../Objects/IObjectTemplate.h"          // createObject (+0x54)
#include "../Objects/IPlayer.h"                  // getDefaultVehicle (+0xA4)
#include "../Objects/Game/GrapplingHookRope.h"
#include "../Objects/Game/GrapplingHookRopeContainer.h"
#include "../Objects/Game/Soldier.h"
#include "../../Core/Ids.h"                      // IID_IGameServer
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../IO/Network/NetworkManager.h"     // getObject
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId
#include "../../Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"   // getTemplate (+0x24)

namespace dice {
namespace hfe {
namespace io    { extern NetworkManager* networkManager; }               // ds:0x8AC3D1C

namespace {
// .rodata dword GEGrapplingHookContainerCreateEvent (0x88895A0) = 60.
const int GEGrapplingHookContainerCreateEvent = 60;

// The template both sides look up by name. A bare literal in the ELF at 0x88895A4, and the same
// string world::GrapplingHookRope::attachSoldier uses - the two must agree or the replay diverges.
const char* const CONTAINER_TEMPLATE_NAME = "GrapplingHookRopeContainer";

} // namespace

//  (C2) / 0x8071890 (C1). Four stores and nothing else - no loopback, no zeroing.
GrapplingHookContainerCreateEvent::GrapplingHookContainerCreateEvent(int playerId, int ropeNetId,
                                                                    int field14, float field18)
    : GameEvent()
    , m_playerId(playerId)                                   // +0x0C
    , m_ropeNetId(ropeNetId)                                 // +0x10
    , m_field14(field14)                                     // +0x14
    , m_field18(field18)                                     // +0x18
{
}

int GrapplingHookContainerCreateEvent::getType() const
{
    return GEGrapplingHookContainerCreateEvent;
}

// Creates a rope container and attaches the player's default soldier. Requires
// the GrapplingHookRopeContainer template. Preserve both rope lookups; the first
// checks existence and the second supplies the object.
void GrapplingHookContainerCreateEvent::executeClient()
{
    if (!io::networkManager->getObject(static_cast<unsigned short>(m_ropeNetId)))
        return;                                              // result discarded - see above

    if (m_playerId == -1)                                    // dead on a received event
        return;

    world::IPlayer* player = world::playerManager->getPlayerFromId(m_playerId);   // +0x20
    if (!player)
        return;

    world::GrapplingHookRope* rope = static_cast<world::GrapplingHookRope*>(
        io::networkManager->getObject(static_cast<unsigned short>(m_ropeNetId))); // the second call
    if (!rope->m_initialized)                                                     // byte +0x205
        return;

    world::IObjectTemplate* containerTemplate =
        world::objectTemplateManager->getTemplate(std::string(CONTAINER_TEMPLATE_NAME));   // +0x24

    world::GrapplingHookRopeContainer* container =
        static_cast<world::GrapplingHookRopeContainer*>(
            containerTemplate->createObject());                                   // +0x54
    container->init();                                                            // +0x20

    world::Soldier* soldier =
        static_cast<world::Soldier*>(player->getDefaultVehicle());                 // +0xA4

    rope->initContainer(container, soldier, m_field14, m_field18);                 // recovered attach replay
}

// executeServer - 0x8071BA0, 41 B. The "answer whether I am a server and do nothing" shape.
bool GrapplingHookContainerCreateEvent::executeServer(int /*connectionId*/)
{
    return game->queryInterface(IID_IGameServer) != 0;
}

// deSerialize - 0x80718D0, 192 B. Three narrow reads through zeroed temps, then the float in
// place - which is what makes the -1 guard unreachable. See the header block.
bool GrapplingHookContainerCreateEvent::deSerialize(io::BitStream* stream)
{
    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = playerId;

    int ropeNetId = 0;
    stream->readBits(&ropeNetId, 16);
    m_ropeNetId = ropeNetId;

    int field14 = 0;
    stream->readBits(&field14, 8);
    m_field14 = field14;

    stream->readBits(&m_field18, 32);            // in place - the field is already 32 bits

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x8071990, 240 B. The exact mirror.
bool GrapplingHookContainerCreateEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    int ropeNetId = m_ropeNetId;
    stream->writeBits(&ropeNetId, 16);

    int field14 = m_field14;
    stream->writeBits(&field14, 8);

    stream->writeBits(&m_field18, 32);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
