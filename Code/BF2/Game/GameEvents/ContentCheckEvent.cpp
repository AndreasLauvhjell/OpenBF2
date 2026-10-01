#include "ContentCheckEvent.h"

#include <cstring>                               // memcpy / memset

#include "../IGame.h"                            // game->queryInterface(IID_IGameServer)
#include "../IGameServer.h"                      // onContentCheckEvent
#include "../../Core/Ids.h"                      // IID_IGameServer
#include "../../IO/BitStream.h"                  // readBits / writeBits

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEContentCheck (0x8888BA4) = 46.
const int GEContentCheck = 46;

// Copies three 16-byte digests from ChecksumContext for the client response.
} // namespace

//  (C2) / 0x806D3E0 (C1), 128 B. Three digests copied in, four dwords at a time.
// The ELF spells each copy as four explicit 32-bit moves rather than a memcpy call - that is GCC
// expanding a fixed-size copy, not a different operation, so memcpy is what is written here
ContentCheckEvent::ContentCheckEvent(const io::ChecksumContext* context)
    : GameEvent()
{
    memcpy(m_miscHash,    context->getMiscHash(),    HASH_SIZE);   // +0x0C  <- context +0x04
    memcpy(m_archiveHash, context->getArchiveHash(), HASH_SIZE);   // +0x1C  <- context +0x14
    memcpy(m_levelHash,   context->getLevelHash(),   HASH_SIZE);   // +0x2C  <- context +0x24
}

// [CHOSEN, not recovered] Maker form zeroes digests before deserialization;
// the original maker leaves them uninitialized.
ContentCheckEvent::ContentCheckEvent()
    : GameEvent()
{
    memset(m_miscHash,    0, HASH_SIZE);
    memset(m_archiveHash, 0, HASH_SIZE);
    memset(m_levelHash,   0, HASH_SIZE);
}

int ContentCheckEvent::getType() const
{
    return GEContentCheck;
}

// Passes borrowed digest pointers and the transport connection ID to the server.
// The callback must not retain pointers after this event is released.
bool ContentCheckEvent::executeServer(int connectionId)
{
    IGameServer* server = static_cast<IGameServer*>(game->queryInterface(IID_IGameServer));
    if (!server)
        return false;

    server->onContentCheckEvent(connectionId, m_miscHash, m_archiveHash, m_levelHash);   // +0xC0
    return true;
}

// deSerialize - 0x806D460, 126 B. Three 128-bit blocks straight into the fields - no temporaries,
// no length prefix, nothing to bound. A fixed-size read cannot overrun, which is why this one needs
// none of ChangeSquadNameEvent's clamping.
bool ContentCheckEvent::deSerialize(io::BitStream* stream)
{
    stream->readBits(m_miscHash,    HASH_SIZE * 8);
    stream->readBits(m_archiveHash, HASH_SIZE * 8);
    stream->readBits(m_levelHash,   HASH_SIZE * 8);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool ContentCheckEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    stream->writeBits(m_miscHash,    HASH_SIZE * 8);
    stream->writeBits(m_archiveHash, HASH_SIZE * 8);
    stream->writeBits(m_levelHash,   HASH_SIZE * 8);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
