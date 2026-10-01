#include "GrapplingHookContainerDetachEvent.h"

#include "../IGame.h"                            // queryInterface(IID_IGameServer)
#include "../Objects/IObject.h"                  // getTemplate, destroy
#include "../Objects/IObjectTemplate.h"          // getClassID
#include "../Objects/IPlayer.h"                  // getVehicle (+0x48) / getDefaultVehicle (+0xA4)
#include "../Objects/Game/GrapplingHookRope.h"
#include "../Objects/Game/GrapplingHookRopeContainer.h"
#include "../Objects/Game/Soldier.h"
#include "../../Core/Ids.h"
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEGrapplingHookContainerDetachEvent (0x8889650) = 62.
const int GEGrapplingHookContainerDetachEvent = 62;

// The ELF's guarded +0x144 is the ISoldier base of Soldier, not a raw data member. Expressing the
// two source-level casts lets MSVC apply its attested +0x15C adjustment on the PE build.
world::ISoldier* soldierInterfaceOf(world::IObject* defaultVehicle)
{
    if (!defaultVehicle)
        return 0;
    return static_cast<world::ISoldier*>(static_cast<world::Soldier*>(defaultVehicle));
}

// The shared precondition - identical in both arms: the player exists and is riding a container.
// getVehicle() (+0x48) is called three times in the ELF, once per test; all three virtual calls
// remain explicit so a stateful implementation retains retail's observable call sequence.
world::GrapplingHookRopeContainer* riddenRopeContainer(world::IPlayer* player)
{
    if (!player->getVehicle())                                                // +0x48, first call
        return 0;
    if (player->getVehicle()->getTemplate()->getClassID()                     // +0x48, second call
            != world::CID_GrapplingHookRopeContainerTemplate)
        return 0;
    return static_cast<world::GrapplingHookRopeContainer*>(
        player->getVehicle());                                                // +0x48, third call
}
} // namespace

//  (C2) / 0x8071D30 (C1). The Vec3 is zeroed and then assigned - a member-initialiser
// followed by three copies. Only the final state is observable, so only it is written.
GrapplingHookContainerDetachEvent::GrapplingHookContainerDetachEvent(int playerId,
                                                                    const Vec3& position)
    : GameEvent()
    , m_playerId(playerId)                                   // +0x0C
    , m_position(position)                                   // +0x10
{
}

// The maker's form - three zero stores and nothing for m_playerId. See the header block.
GrapplingHookContainerDetachEvent::GrapplingHookContainerDetachEvent()
    : GameEvent()
{
    m_position.x = 0.0f;
    m_position.y = 0.0f;
    m_position.z = 0.0f;
}

int GrapplingHookContainerDetachEvent::getType() const
{
    return GEGrapplingHookContainerDetachEvent;
}

// executeClient - 0x8071F40, 208 B. Position, unhook, destroy - IN that order. See the header.
void GrapplingHookContainerDetachEvent::executeClient()
{
    world::IPlayer* player = world::playerManager->getPlayerFromId(m_playerId);   // +0x20
    if (!player)
        return;

    world::GrapplingHookRopeContainer* container = riddenRopeContainer(player);
    if (!container)
        return;

    world::GrapplingHookRope* rope = container->m_rope;                           // +0x3C8
    if (!rope)
        return;

    container->setAbsolutePosition(m_position);                                  // +0x70
    rope->detachSoldier(soldierInterfaceOf(player->getDefaultVehicle()));
    container->destroy(true);                                                     // +0x34
}

// Unhooks, positions, then destroys the container. Does not rebroadcast or
// authenticate m_playerId against the transport sender.
bool GrapplingHookContainerDetachEvent::executeServer(int /*connectionId*/)
{
    if (!game->queryInterface(IID_IGameServer))
        return false;

    world::IPlayer* player = world::playerManager->getPlayerFromId(m_playerId);
    if (!player)
        return false;

    world::GrapplingHookRopeContainer* container = riddenRopeContainer(player);
    if (!container)
        return false;

    world::GrapplingHookRope* rope = container->m_rope;
    if (!rope)
        return true;                             // true here, false on the failures above

    rope->detachSoldier(soldierInterfaceOf(player->getDefaultVehicle()));
    container->setAbsolutePosition(m_position);                                  // second here
    container->destroy(true);
    return true;
}

// Reads all three position components into zeroed temporaries before assigning the position.
bool GrapplingHookContainerDetachEvent::deSerialize(io::BitStream* stream)
{
    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = playerId;

    float x = 0.0f, y = 0.0f, z = 0.0f;
    stream->readBits(&x, 32);
    stream->readBits(&y, 32);
    stream->readBits(&z, 32);
    m_position.x = x;
    m_position.y = y;
    m_position.z = z;

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x8071E60, 224 B. The three floats go straight out of the object, no temps.
bool GrapplingHookContainerDetachEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    stream->writeBits(&m_position.x, 32);
    stream->writeBits(&m_position.y, 32);
    stream->writeBits(&m_position.z, 32);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
