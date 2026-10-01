#include "GrapplingHookCreateEvent.h"

#include "../IGame.h"                            // queryInterface(IID_IGameServer)
#include "../Objects/IObject.h"                  // getTemplate
#include "../Objects/IObjectTemplate.h"          // getClassID
#include "../Objects/IPlayer.h"                  // getId (+0x54)
#include "../Objects/Game/GrapplingHookRope.h"
#include "../../Core/Ids.h"                      // IID_IGameServer, CID_GrapplingHookRopeTemplate
#include "../../IO/BitStream.h"                  // readBits / writeBits
#include "../../IO/Network/NetworkManager.h"     // getObject
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId

namespace dice {
namespace hfe {
namespace io { extern NetworkManager* networkManager; }      // ds:0x8AC3D1C

namespace {
// .rodata dword GEGrapplingHookCreateEvent (0x8889790) = 63.
const int GEGrapplingHookCreateEvent = 63;

} // namespace

//  (C2) / 0x8072880 (C1). Six zero stores for the two vectors, then five assignments.
// Only the final state is observable, so the zeroing is left to the member-initialisers.
GrapplingHookCreateEvent::GrapplingHookCreateEvent(unsigned short ropeNetId, int playerId,
                                                   float field14, Vec3 fieldA, Vec3 fieldB)
    : GameEvent()
    , m_ropeNetId(ropeNetId)                                 // +0x0C
    , m_playerId(playerId)                                   // +0x10
    , m_field14(field14)                                     // +0x14
    , m_fieldA(fieldA)                                       // +0x18
    , m_fieldB(fieldB)                                       // +0x24
{
}

// The maker's form - six zero stores and nothing else. See the header block.
GrapplingHookCreateEvent::GrapplingHookCreateEvent()
    : GameEvent()
{
    m_fieldA.x = m_fieldA.y = m_fieldA.z = 0.0f;
    m_fieldB.x = m_fieldB.y = m_fieldB.z = 0.0f;
}

int GrapplingHookCreateEvent::getType() const
{
    return GEGrapplingHookCreateEvent;
}

// Initializes a matching rope after validating the player. Preserve three network
// lookups and the getPlayerFromId/getId round trip. Other template types are ignored.
void GrapplingHookCreateEvent::executeClient()
{
    if (!io::networkManager->getObject(m_ropeNetId))
        return;                                              // result discarded

    world::IObject* probe = io::networkManager->getObject(m_ropeNetId);        // the second lookup
    if (probe->getTemplate()->getClassID() != world::CID_GrapplingHookRopeTemplate)
        return;

    if (m_playerId == -1)                                    // dead on a received event
        return;

    world::GrapplingHookRope* rope = static_cast<world::GrapplingHookRope*>(
        io::networkManager->getObject(m_ropeNetId));                           // the third lookup

    world::IPlayer* player = world::playerManager->getPlayerFromId(m_playerId);   // +0x20
    if (!player)
        return;

    rope->m_ownerPlayerId = player->getId();
    rope->m_creationScalar = m_field14;
    rope->m_creationPosition = m_fieldA;
    rope->m_creationDirection = m_fieldB;
    rope->initialize();
}

// the "answer whether I am a server and do nothing" shape.
bool GrapplingHookCreateEvent::executeServer(int /*connectionId*/)
{
    return game->queryInterface(IID_IGameServer) != 0;
}

// THE NET ID IS stored AS A word AND THE player ID AS A dword, from reads of 16 and 8 bits -
// `mov [edx+0Ch], ax` against `mov [edx+10h], eax`. That asymmetry is what fixes the two field
// widths; the IDB's struct disagrees.
// each Vec3 IS read into three zeroed temps AND stored after ALL three reads.
bool GrapplingHookCreateEvent::deSerialize(io::BitStream* stream)
{
    int ropeNetId = 0;
    stream->readBits(&ropeNetId, 16);
    m_ropeNetId = static_cast<unsigned short>(ropeNetId);

    int playerId = 0;
    stream->readBits(&playerId, 8);
    m_playerId = playerId;

    stream->readBits(&m_field14, 32);            // in place

    float x = 0.0f, y = 0.0f, z = 0.0f;
    stream->readBits(&x, 32);
    stream->readBits(&y, 32);
    stream->readBits(&z, 32);
    m_fieldA.x = x;
    m_fieldA.y = y;
    m_fieldA.z = z;

    x = y = z = 0.0f;
    stream->readBits(&x, 32);
    stream->readBits(&y, 32);
    stream->readBits(&z, 32);
    m_fieldB.x = x;
    m_fieldB.y = y;
    m_fieldB.z = z;

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x8072A90, 352 B. The exact mirror.
bool GrapplingHookCreateEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int ropeNetId = m_ropeNetId;
    stream->writeBits(&ropeNetId, 16);

    int playerId = m_playerId;
    stream->writeBits(&playerId, 8);

    stream->writeBits(&m_field14, 32);

    stream->writeBits(&m_fieldA.x, 32);
    stream->writeBits(&m_fieldA.y, 32);
    stream->writeBits(&m_fieldA.z, 32);

    stream->writeBits(&m_fieldB.x, 32);
    stream->writeBits(&m_fieldB.y, 32);
    stream->writeBits(&m_fieldB.z, 32);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
