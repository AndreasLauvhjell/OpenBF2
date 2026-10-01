// Server challenge containing ten raw bytes and a mod name of at most fifteen characters.
// The challenge bytes are not null-terminated. Overlong mod names cause a fatal error.
// Length validation uses string::size(), while copying stops at the first embedded null.

#ifndef GAME_GAMEEVENTS_CHALLENGEEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_CHALLENGEEVENT_H

#include <string>

#include "../Common/GameEvent.h"

namespace dice {
namespace hfe {

class ChallengeEvent : public GameEvent                      // 0x28
{
public:
    enum { CHALLENGE_SIZE = 10, MOD_NAME_BUFFER_SIZE = 16, MAX_MOD_NAME_LENGTH = 15 };

    // attested: _ZN4dice3hfe14ChallengeEventC2EPKcRKSs is (const char*, const std::string&).
    // The first argument is read for exactly CHALLENGE_SIZE bytes and is NOT treated as a
    // C string - it may contain NULs.
    ChallengeEvent(const char* challengeString, const std::string& modName);
    ChallengeEvent();                                        // the maker's form
    virtual ~ChallengeEvent() {}

    virtual int  getType() const;                            // GEChallenge (1)
    virtual void executeClient();
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);

    // GameClient reads the challenge and mod-name buffers directly.
    friend class GameClient;

protected:
    /* +0x0C */ char          m_challengeString[CHALLENGE_SIZE];
    /* +0x16 */ unsigned char m_modNameLength;
    /* +0x17 */ char          m_modName[MOD_NAME_BUFFER_SIZE];
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_CHALLENGEEVENT_H
