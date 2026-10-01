#include "ChallengeResponseEvent.h"

#include <cstring>                               // memset / memcpy

#include "../IGame.h"                            // game->queryInterface(IID_IGameServer)
#include "../IGameServer.h"                      // challengeResponse
#include "../../Core/Ids.h"                      // IID_IGameServer
#include "../../IO/BitStream.h"                  // readBits / writeBits

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEChallengeResponse (0x88885EC) = 2.
const int GEChallengeResponse = 2;
} // namespace

//  (C2) / 0x806AA90 (C1), 90 B. Zero 73 bytes, copy 73 in, store one dword.
// m_netVersion and m_gameId are NOT touched - see the header.
ChallengeResponseEvent::ChallengeResponseEvent(char* response, unsigned int field58)
    : GameEvent()
{
    memset(m_response, 0, RESPONSE_SIZE);
    memcpy(m_response, response, RESPONSE_SIZE);
    m_field58 = field58;                                     // +0x58
    // m_netVersion (+0x5C) and m_gameId (+0x60) deliberately left uninitialised, as in the ELF.
}

// The maker's form. The ELF has no such constructor; deSerialize fills every field, so zeroing
// here only matters if a caller reads the object before that.
ChallengeResponseEvent::ChallengeResponseEvent()
    : GameEvent()
    , m_field58(0)
    , m_netVersion(0)
    , m_gameId(0)
{
    memset(m_response, 0, RESPONSE_SIZE);
}

int ChallengeResponseEvent::getType() const
{
    return GEChallengeResponse;
}

// executeServer - 0x806A9E0, 68 B. Hand the whole event and the connection it arrived on to the
// GameServer; all the deciding happens there.
// THE event IS passed BY pointer AND outlives nothing. challengeResponse must consume it during
// the call - the dispatcher releases the event as soon as this returns.
bool ChallengeResponseEvent::executeServer(int connectionId)
{
    IGameServer* server = static_cast<IGameServer*>(game->queryInterface(IID_IGameServer));
    if (!server)
        return false;

    server->challengeResponse(connectionId, this);           // +0x2C
    return true;
}

// Clears and reads the fixed response blob, then the three trailing fields.
// Game ID decodes as one sign bit plus 31 magnitude bits.
bool ChallengeResponseEvent::deSerialize(io::BitStream* stream)
{
    memset(m_response, 0, RESPONSE_SIZE);

    stream->readBits(m_response, RESPONSE_SIZE * 8);

    int field58 = 0;
    stream->readBits(&field58, 32);
    m_field58 = static_cast<unsigned int>(field58);

    int netVersion = 0;
    stream->readBits(&netVersion, 32);
    m_netVersion = static_cast<unsigned int>(netVersion);

    int negative = 0;
    stream->readBits(&negative, 1);

    int magnitude = 0;
    stream->readBits(&magnitude, 31);
    m_gameId = (negative == 1) ? -magnitude : magnitude;

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

// serialize - 0x806AC10, 310 B. The mirror, including the sign-and-magnitude split.
bool ChallengeResponseEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    stream->writeBits(m_response, RESPONSE_SIZE * 8);

    int field58 = static_cast<int>(m_field58);
    stream->writeBits(&field58, 32);

    int netVersion = static_cast<int>(m_netVersion);
    stream->writeBits(&netVersion, 32);

    if (m_gameId < 0)
    {
        int negative = 1;
        stream->writeBits(&negative, 1);
        int magnitude = -m_gameId;                           // INT_MIN would overflow - dice's
        stream->writeBits(&magnitude, 31);
    }
    else
    {
        int negative = 0;
        stream->writeBits(&negative, 1);
        int magnitude = m_gameId;
        stream->writeBits(&magnitude, 31);
    }

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
