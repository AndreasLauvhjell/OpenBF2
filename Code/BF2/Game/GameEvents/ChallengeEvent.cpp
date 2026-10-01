#include "ChallengeEvent.h"

#include <cstring>                               // memset / strlen / strncpy

#include "../IGame.h"                            // game->queryInterface(IID_IGameClient)
#include "../IGameClient.h"
#include "../../Core/Debug.h"                    // the constructor's abort
#include "../../Core/Ids.h"                      // IID_IGameClient
#include "../../IO/BitStream.h"                  // readBits / writeBits / skipBits

namespace dice {
namespace hfe {

namespace {
// .rodata dword GEChallenge (0x88885E8) = 1.
const int GEChallenge = 1;

} // namespace

// Copies the ten-byte challenge and mod name; an oversized name triggers a fatal diagnostic.
ChallengeEvent::ChallengeEvent(const char* challengeString, const std::string& modName)
    : GameEvent()
{
    memset(m_challengeString, 0, CHALLENGE_SIZE);
    m_modNameLength = 0;

    for (int i = 0; i < CHALLENGE_SIZE; ++i)
        m_challengeString[i] = challengeString[i];

    if (modName.size() > static_cast<std::string::size_type>(MAX_MOD_NAME_LENGTH))
    {
        Debug(DEBUGTYPE_6, "Game/GameEvents/ChallengeEvent.cpp", 44, "")
            << "modName to long " << modName;   // type 6 aborts - the clamp below is dead code
    }

    // strlen(c_str()), not size() - see the header.
    std::string::size_type length = strlen(modName.c_str());
    if (length > static_cast<std::string::size_type>(MAX_MOD_NAME_LENGTH))
        length = MAX_MOD_NAME_LENGTH;

    m_modNameLength = static_cast<unsigned char>(length);
    strncpy(m_modName, modName.c_str(), m_modNameLength);
    m_modName[m_modNameLength] = '\0';
}

// The maker's form. The ELF has no such constructor; deSerialize overwrites everything it reads,
// but m_modName's tail does not get cleared by it, so zeroing here is a rebuild courtesy.
ChallengeEvent::ChallengeEvent()
    : GameEvent()
    , m_modNameLength(0)
{
    memset(m_challengeString, 0, CHALLENGE_SIZE);
    memset(m_modName, 0, MOD_NAME_BUFFER_SIZE);
}

int ChallengeEvent::getType() const
{
    return GEChallenge;
}

// Passes the challenge to the client for a response; ignored when no client is present.
void ChallengeEvent::executeClient()
{
    IGameClient* gameClient = static_cast<IGameClient*>(game->queryInterface(IID_IGameClient));
    if (gameClient)
        gameClient->handleChallenge(this);
}

// Reads the challenge and length-prefixed mod name. [CHOSEN, not recovered] Limits
// stored names to the local buffer and skips excess bytes, preserving packet alignment
// while avoiding the original unbounded copy.
bool ChallengeEvent::deSerialize(io::BitStream* stream)
{
    memset(m_challengeString, 0, CHALLENGE_SIZE);
    m_modNameLength = 0;

    stream->readBits(m_challengeString, CHALLENGE_SIZE * 8);

    int length = 0;
    stream->readBits(&length, 8);

    unsigned int onTheWire = static_cast<unsigned int>(length) & 0xFFu;
    unsigned int stored    = onTheWire;
    if (stored > static_cast<unsigned int>(MAX_MOD_NAME_LENGTH))
        stored = MAX_MOD_NAME_LENGTH;                        // the divergence - see above

    m_modNameLength = static_cast<unsigned char>(stored);
    stream->readBits(m_modName, stored * 8);
    if (onTheWire > stored)
        stream->skipBits((onTheWire - stored) * 8);          // keep the cursor where the ELF puts it
    m_modName[stored] = '\0';

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

bool ChallengeEvent::serialize(io::BitStream* stream)
{
    int type = getType();
    stream->writeBits(&type, gameEventTypeIdBitCount());

    stream->writeBits(m_challengeString, CHALLENGE_SIZE * 8);

    int length = m_modNameLength;
    stream->writeBits(&length, 8);

    stream->writeBits(m_modName, static_cast<unsigned int>(m_modNameLength) * 8);

    if (!stream->hasError())
        return true;
    stream->clearError();
    return false;
}

} // namespace hfe
} // namespace dice
