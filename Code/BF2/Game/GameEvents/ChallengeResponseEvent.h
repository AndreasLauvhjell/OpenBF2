// Client response carrying a 73-byte challenge answer, network version and game ID.
// The sender must fill network version and game ID after construction.
// Game IDs use sign-and-magnitude encoding; INT_MIN cannot be represented.
// [UNRESOLVED] m_field58 preserves the constructor's second argument with unknown meaning.

#ifndef GAME_GAMEEVENTS_CHALLENGERESPONSEEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_CHALLENGERESPONSEEVENT_H

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class ChallengeResponseEvent : public GameEvent              // 0x64
{
public:
    enum { RESPONSE_SIZE = 73 };

    // attested: _ZN4dice3hfe22ChallengeResponseEventC2EPcj is (char*, unsigned int).
    // Reads exactly RESPONSE_SIZE bytes from `response`; it is not a C string.
    ChallengeResponseEvent(char* response, unsigned int field58);
    ChallengeResponseEvent();                                // the maker's form
    virtual ~ChallengeResponseEvent() {}

    virtual int  getType() const;                            // GEChallengeResponse (2)
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    // executeClient is deliberately NOT declared - the slot is inherited.

    // GameServer::challengeResponse reads the decoded payload directly, as the ELF does
    // (no accessors exist in the binary).
    friend class GameServer;

    // And GameClient::handleChallenge (PE 4D1CF0) writes all four fields directly on the way out -
    // m_netVersion, m_field58, m_gameId, and m_response through gcd_compute_response. Same reason:
    // the client binary has no accessors either.
    friend class GameClient;

protected:
    /* +0x0C */ unsigned char m_response[RESPONSE_SIZE];
    /* +0x55 */ // three bytes of alignment padding
    /* +0x58 */ unsigned int  m_field58;
    /* +0x5C */ unsigned int  m_netVersion;
    /* +0x60 */ int           m_gameId;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_CHALLENGERESPONSEEVENT_H
