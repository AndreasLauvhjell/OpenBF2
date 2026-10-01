#include "GrapplingHookContainerUpdateEvent.h"

#include "../IGame.h"                            // queryInterface, sendGameEventToAllPlaying
#include "../Objects/IObject.h"                  // getTemplate
#include "../Objects/IObjectTemplate.h"          // getClassID
#include "../Objects/IPlayer.h"                  // getVehicle (+0x48)
#include "../Objects/Game/GrapplingHookRope.h"
#include "../Objects/Game/GrapplingHookRopeContainer.h"
#include "../Objects/Game/RopeLink.h"
#include "../../Core/Ids.h"                      // IID_IGameServer, CID_GrapplingHookRopeContainerTemplate
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../Math/Vec3.h"
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId / getLocalHumanPlayer

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEGrapplingHookContainerUpdateEvent (0x88896F0) = 61.
const int GEGrapplingHookContainerUpdateEvent = 61;

// Requires a player currently riding a rope container. Preserve three separate
// vehicle queries before the unbounded link lookup and interpolation.
world::GrapplingHookRopeContainer* riddenRopeContainer(world::IPlayer* player)
{
    if (!player->getVehicle())                                               // +0x48, first call
        return 0;
    if (player->getVehicle()->getTemplate()->getClassID()                    // +0x48, second call
            != world::CID_GrapplingHookRopeContainerTemplate)
        return 0;
    return static_cast<world::GrapplingHookRopeContainer*>(
        player->getVehicle());                                               // +0x48, third call
}
} // namespace

//  (C2) / 0x8072240 (C1). Four stores; no zeroing, no loopback.
GrapplingHookContainerUpdateEvent::GrapplingHookContainerUpdateEvent(int playerId, int segmentIndex,
                                                                    float t, float field18)
    : GameEvent()
    , m_playerId(playerId)                                   // +0x0C
    , m_segmentIndex(segmentIndex)                           // +0x10
    , m_t(t)                                                 // +0x14
    , m_field18(field18)                                     // +0x18
{
}

int GrapplingHookContainerUpdateEvent::getType() const
{
    return GEGrapplingHookContainerUpdateEvent;
}

// Skips local-human movement so network updates do not override local prediction.
void GrapplingHookContainerUpdateEvent::executeClient()
{
    world::IPlayer* player = world::playerManager->getPlayerFromId(m_playerId);   // +0x20
    if (!player)
        return;
    if (player == world::playerManager->getLocalHumanPlayer())                    // +0x30
        return;

    world::GrapplingHookRopeContainer* container = riddenRopeContainer(player);
    if (!container)
        return;

    world::GrapplingHookRope* rope = container->m_rope;
    world::RopeLink* segment = rope->m_ropeLinks[m_segmentIndex];             // unbounded
    container->m_segment = segment;
    container->m_segmentIndex = m_segmentIndex;
    container->m_segmentProgress = m_t;
    container->m_eventField18 = m_field18;

    const Vec3* p0 = &segment->m_startParticle->m_position;
    const Vec3* p1 = &segment->m_endParticle->m_position;
    Vec3 scaled;
    scaled.x = (p1->x - p0->x) * m_t;
    scaled.y = (p1->y - p0->y) * m_t;
    scaled.z = (p1->z - p0->z) * m_t;

    // The ELF re-reads the start particle through the just-stored container segment.
    const Vec3* base = &container->m_segment->m_startParticle->m_position;
    Vec3 position;
    position.x = scaled.x + base->x;
    position.y = scaled.y + base->y;
    position.z = scaled.z + base->z;
    container->setAbsolutePosition(position);                                 // +0x70
}

// Applies the update then broadcasts a newly allocated copy. Does not authenticate
// the packet's player ID against the transport sender.
bool GrapplingHookContainerUpdateEvent::executeServer(int /*connectionId*/)
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
    world::RopeLink* segment = rope->m_ropeLinks[m_segmentIndex];             // unbounded
    container->m_segment = segment;
    container->m_segmentIndex = m_segmentIndex;
    container->m_segmentProgress = m_t;
    container->m_eventField18 = m_field18;

    const Vec3* p0 = &segment->m_startParticle->m_position;
    const Vec3* p1 = &segment->m_endParticle->m_position;
    Vec3 scaled;
    scaled.x = (p1->x - p0->x) * m_t;
    scaled.y = (p1->y - p0->y) * m_t;
    scaled.z = (p1->z - p0->z) * m_t;

    const Vec3* base = &container->m_segment->m_startParticle->m_position;
    Vec3 position;
    position.x = scaled.x + base->x;
    position.y = scaled.y + base->y;
    position.z = scaled.z + base->z;
    container->setAbsolutePosition(position);                                 // +0x70

    game->sendGameEventToAllPlaying(new GrapplingHookContainerUpdateEvent(
        m_playerId, m_segmentIndex, m_t, m_field18));                             // +0x84
    return true;
}

// deSerialize - 0x8072280, 176 B. Two narrow reads through zeroed temps, then the two floats in
// place.
bool GrapplingHookContainerUpdateEvent::deSerialize(io::BitStream* stream)
{
    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = playerId;

    int segmentIndex = 0;
    stream->readBits(&segmentIndex, 8);
    m_segmentIndex = segmentIndex;

    stream->readBits(&m_t, 32);
    stream->readBits(&m_field18, 32);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x8072330, 224 B. The exact mirror.
bool GrapplingHookContainerUpdateEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    int segmentIndex = m_segmentIndex;
    stream->writeBits(&segmentIndex, 8);

    stream->writeBits(&m_t, 32);
    stream->writeBits(&m_field18, 32);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
