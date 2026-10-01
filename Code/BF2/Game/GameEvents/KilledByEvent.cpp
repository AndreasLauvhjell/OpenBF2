#include "KilledByEvent.h"

#include <cstring>                               // memset

#include "../IGame.h"                            // game->queryInterface(IID_IGameServer)
#include "../GameLogic/Player.h"                 // getPlayer - the identity function
#include "../Objects/IObject.h"                  // getTemplate
#include "../Objects/IObjectTemplate.h"          // getClassID
#include "../Objects/IPlayer.h"                  // getVehicle
#include "../Objects/ObjectLayoutAccess.h"        // LayoutAccess::armorComponent (IObject +0x3C)
#include "../Objects/GameLogicComp/IArmorGLComp.h" // setLastHitBoneIndex / setLastHitDirection
#include "../../Core/EventManager.h"             // eventManager->postEvent
#include "../../Core/IBase.h"
#include "../../Core/Ids.h"                      // IID_IGameServer, CID_SoldierTemplate
#include "../../IO/BitStream.h"                  // readBits / writeBits / the compressed vector
#include "../../Scene/Object/PlayerManager.h"    // getPlayerFromId

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEKilledBy (0x8889C84) = 21.
const int GEKilledBy = 21;

// The notification executeClient raises: category ECGameLogic (0), event id 3.
const unsigned int NOTIFY_KILLED_BY = 3u;

// The scale both wire halves pass to the compressed-vector primitives. A bare 1000.0f in the ELF.
const float LAST_HIT_DIRECTION_SCALE = 1000.0f;

} // namespace

// The maker's form. The ELF has no constructor for this class at all; deSerialize fills every
// field, so the zeroing here is the rebuild's courtesy rather than recovered behaviour - see
// GameEventMakerMaker<T> in Game/Common/GameEvent.h for what the original leaves behind instead.
KilledByEvent::KilledByEvent()
    : GameEvent()
{
    memset(&m_data, 0, sizeof(m_data));
}

int KilledByEvent::getType() const
{
    return GEKilledBy;
}

// Posts the kill notification before resolving the victim, then marks soldier
// corpses. Listeners must consume the borrowed payload synchronously. A matching
// soldier vehicle must have an armor component; this path does not search parents.
void KilledByEvent::executeClient()
{
    eventManager->postEvent(ECGameLogic, NOTIFY_KILLED_BY, &m_data, 0.0f);        // +0x14

    world::IPlayer* found =
        world::playerManager->getPlayerFromId(m_data.victimPlayerId);             // +0x20
    world::IPlayer* victim = getPlayer(found);                                    // identity, 8 B
    if (!victim)
        return;

    world::IObject* vehicle = victim->getVehicle();                               // +0x48
    if (!vehicle)
        return;

    if (vehicle->getTemplate()->getClassID() != world::CID_SoldierTemplate)
        return;

    // NOT a [FRONTIER] shim pair: both slots are already declared, at these exact offsets, in
    // Game/Objects/GameLogicComp/IArmorGLComp.h (slots 45 and 47 of ArmorGLComp's own table).
    world::IArmorGLComp* armor = static_cast<world::IArmorGLComp*>(
        world::object_detail::LayoutAccess::armorComponent(vehicle));             // +0x3C

    armor->setLastHitBoneIndex(m_data.lastHitBoneIndex, true);                    // +0xB4
    armor->setLastHitDirection(m_data.lastHitDirection, true);                    // +0xBC
}

// executeServer - 0x8075FB0, 41 B. Nothing but the query - the same "answer whether I am a server
// and do nothing" shape MedalEvent has. `setnz al` is the whole conversion.
bool KilledByEvent::executeServer(int /*connectionId*/)
{
    return game->queryInterface(IID_IGameServer) != 0;
}

// Reads the bone index before direction. The vector reader accepts compressed and raw forms.
bool KilledByEvent::deSerialize(io::BitStream* stream)
{
    int killerPlayerId = 0;
    stream->readBits(&killerPlayerId, 8);
    m_data.killerPlayerId = static_cast<unsigned char>(killerPlayerId);

    int victimPlayerId = 0;
    stream->readBits(&victimPlayerId, 8);
    m_data.victimPlayerId = static_cast<unsigned char>(victimPlayerId);

    int isKillerPlayerAlive = 0;
    stream->readBits(&isKillerPlayerAlive, 1);
    m_data.isKillerPlayerAlive = (isKillerPlayerAlive == 1);

    int field04 = 0;
    stream->readBits(&field04, 16);
    m_data.field04 = static_cast<unsigned short>(field04);

    int someObjTemplateId = 0;
    stream->readBits(&someObjTemplateId, 32);
    m_data.someObjTemplateId = static_cast<unsigned int>(someObjTemplateId);

    int vehicleTemplateId = 0;
    stream->readBits(&vehicleTemplateId, 32);
    m_data.vehicleTemplateId = static_cast<unsigned int>(vehicleTemplateId);

    int lastHitBoneIndex = 0;
    stream->readBits(&lastHitBoneIndex, 8);
    m_data.lastHitBoneIndex = static_cast<unsigned char>(lastHitBoneIndex);

    m_data.lastHitDirection =
        stream->readCompressedVector(stream->getCompressionVector(),
                                     LAST_HIT_DIRECTION_SCALE);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// Without a compression reference, writes selector zero and three raw floats.
// Calling writeCompressedVector in that case would dereference the missing reference.
bool KilledByEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    int killerPlayerId = m_data.killerPlayerId;
    stream->writeBits(&killerPlayerId, 8);

    int victimPlayerId = m_data.victimPlayerId;
    stream->writeBits(&victimPlayerId, 8);

    int isKillerPlayerAlive = m_data.isKillerPlayerAlive ? 1 : 0;
    stream->writeBits(&isKillerPlayerAlive, 1);

    int field04 = m_data.field04;
    stream->writeBits(&field04, 16);

    int someObjTemplateId = static_cast<int>(m_data.someObjTemplateId);
    stream->writeBits(&someObjTemplateId, 32);

    int vehicleTemplateId = static_cast<int>(m_data.vehicleTemplateId);
    stream->writeBits(&vehicleTemplateId, 32);

    int lastHitBoneIndex = m_data.lastHitBoneIndex;
    stream->writeBits(&lastHitBoneIndex, 8);

    if (stream->hasCompressionVector())
    {
        stream->writeCompressedVector(m_data.lastHitDirection,
                                      stream->getCompressionVector(),
                                      LAST_HIT_DIRECTION_SCALE);
    }
    else
    {
        int selector = 0;                        // the two-bit "uncompressed" selector, by hand
        stream->writeBits(&selector, 2);
        stream->writeBits(&m_data.lastHitDirection.x, 32);
        stream->writeBits(&m_data.lastHitDirection.y, 32);
        stream->writeBits(&m_data.lastHitDirection.z, 32);
    }

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
