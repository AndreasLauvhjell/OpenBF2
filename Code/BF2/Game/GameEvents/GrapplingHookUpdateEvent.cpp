#include "GrapplingHookUpdateEvent.h"

#include "../IGame.h"                            // queryInterface, sendGameEventToAllPlaying
#include "../Objects/IObject.h"                  // getTemplate
#include "../Objects/IObjectTemplate.h"          // getClassID
#include "../Objects/Game/GrapplingHookRope.h"
#include "../../Core/Ids.h"                      // IID_IGameServer, CID_GrapplingHookRopeTemplate
#include "../../IO/BitStream.h"                  // readBits / writeBits / the compressed vector
#include "../../IO/Network/NetworkManager.h"     // getObject
#include "../../Math/Vec3.h"
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId / getLocalHumanPlayer

namespace dice {
namespace hfe {
namespace io { extern NetworkManager* networkManager; }      // ds:0x8AC3D1C

namespace {
// .rodata dword GEGrapplingHookUpdateEvent (0x8889828) = 64.
const int GEGrapplingHookUpdateEvent = 64;

// The scale both wire halves pass to the compressed-vector primitives. A bare 1000.0f in the ELF -
// the same constant KilledByEvent uses.
const float ROPE_VECTOR_SCALE = 1000.0f;

// The particle vector at rope +0x14C is source-level typed now. Its 44-byte element stride is
// attested independently by RopeParticle::RopeParticle (ELF 0x81A6220) and by the LEA sequence in
// both execute arms below. Indexing remains deliberately unbounded, exactly as shipped.
} // namespace

// The only constructor. createEvent's inlined copy (0x805E870) is nine zero stores covering every
// field - +0x0C, +0x0E, +0x10, +0x14, +0x18, +0x1C, +0x20, +0x24, +0x28.
GrapplingHookUpdateEvent::GrapplingHookUpdateEvent()
    : GameEvent()
    , m_ropeNetId(0)
    , m_segmentIndex(0)
    , m_flag28(false)
{
    m_start.x = m_start.y = m_start.z = 0.0f;
    m_end.x   = m_end.y   = m_end.z   = 0.0f;
}

int GrapplingHookUpdateEvent::getType() const
{
    return GEGrapplingHookUpdateEvent;
}

// Applies remote rope updates, skipping the local human owner's prediction.
// The owner is read from the rope; repeated object lookups remain observable.
void GrapplingHookUpdateEvent::executeClient()
{
    if (!io::networkManager->getObject(m_ropeNetId))
        return;                                              // result discarded

    world::IObject* probe = io::networkManager->getObject(m_ropeNetId);
    if (probe->getTemplate()->getClassID() != world::CID_GrapplingHookRopeTemplate)
        return;

    world::GrapplingHookRope* rope = static_cast<world::GrapplingHookRope*>(
        io::networkManager->getObject(m_ropeNetId));

    world::IPlayer* owner =
        world::playerManager->getPlayerFromId(rope->m_ownerPlayerId);                  // +0x1A0
    if (!owner)
        return;
    if (owner == world::playerManager->getLocalHumanPlayer())                             // +0x30
        return;

    world::RopeParticle& segment = rope->m_particles[m_segmentIndex];          // unbounded
    segment.m_previousPosition = m_end;                                        // +0x0C first
    segment.m_position = m_start;                                              // +0x00 second
    segment.m_eventFlag = m_flag28;                                            // +0x29 last
}

// Applies the update without checking sender ownership and broadcasts a fresh event.
bool GrapplingHookUpdateEvent::executeServer(int /*connectionId*/)
{
    if (!game->queryInterface(IID_IGameServer))
        return false;

    if (!io::networkManager->getObject(m_ropeNetId))
        return false;

    world::IObject* probe = io::networkManager->getObject(m_ropeNetId);
    if (probe->getTemplate()->getClassID() != world::CID_GrapplingHookRopeTemplate)
        return false;

    world::GrapplingHookRope* rope = static_cast<world::GrapplingHookRope*>(
        io::networkManager->getObject(m_ropeNetId));
    world::RopeParticle& segment = rope->m_particles[m_segmentIndex];          // unbounded
    segment.m_previousPosition = m_end;                                        // +0x0C first
    segment.m_position = m_start;                                              // +0x00 second
    segment.m_eventFlag = m_flag28;                                            // +0x29 last

    GrapplingHookUpdateEvent* relay = new GrapplingHookUpdateEvent;
    relay->m_ropeNetId    = m_ropeNetId;
    relay->m_segmentIndex = m_segmentIndex;
    relay->m_start        = m_start;
    relay->m_end          = m_end;
    relay->m_flag28       = m_flag28;
    game->sendGameEventToAllPlaying(relay);                                    // +0x84

    return true;
}

// The vector reader handles compressed and raw forms itself. The flag is the final wire field.
bool GrapplingHookUpdateEvent::deSerialize(io::BitStream* stream)
{
    int ropeNetId = 0;
    stream->readBits(&ropeNetId, 16);
    m_ropeNetId = static_cast<unsigned short>(ropeNetId);

    int segmentIndex = 0;
    stream->readBits(&segmentIndex, 8);
    m_segmentIndex = static_cast<unsigned char>(segmentIndex);

    m_start = stream->readCompressedVector(stream->getCompressionVector(), ROPE_VECTOR_SCALE);
    m_end   = stream->readCompressedVector(stream->getCompressionVector(), ROPE_VECTOR_SCALE);

    int flag = 0;
    stream->readBits(&flag, 1);
    m_flag28 = (flag == 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x8072F60, 528 B. THE TWO-armed vector write, twice - see the header block. The
// else-arm's hand-written two-bit zero selector is exactly what writeCompressedVector's selector-0
// path emits; it exists because writeCompressedVector would dereference a reference vector that is
// not there.
bool GrapplingHookUpdateEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int ropeNetId = m_ropeNetId;
    stream->writeBits(&ropeNetId, 16);

    int segmentIndex = m_segmentIndex;
    stream->writeBits(&segmentIndex, 8);

    if (stream->hasCompressionVector())
    {
        stream->writeCompressedVector(m_start, stream->getCompressionVector(), ROPE_VECTOR_SCALE);
    }
    else
    {
        int selector = 0;                        // the two-bit "uncompressed" selector, by hand
        stream->writeBits(&selector, 2);
        stream->writeBits(&m_start.x, 32);
        stream->writeBits(&m_start.y, 32);
        stream->writeBits(&m_start.z, 32);
    }

    if (stream->hasCompressionVector())
    {
        stream->writeCompressedVector(m_end, stream->getCompressionVector(), ROPE_VECTOR_SCALE);
    }
    else
    {
        int selector = 0;
        stream->writeBits(&selector, 2);
        stream->writeBits(&m_end.x, 32);
        stream->writeBits(&m_end.y, 32);
        stream->writeBits(&m_end.z, 32);
    }

    int flag = m_flag28 ? 1 : 0;
    stream->writeBits(&flag, 1);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
