// Matchmaking backend interface implemented by GameSpyBackend.
// [INFERRED] header placement; callers use typed virtual dispatch.

#ifndef GAME_MATCHMAKING_SERVER_MMBACKEND_H     // [INFERRED] guard style
#define GAME_MATCHMAKING_SERVER_MMBACKEND_H

namespace dice {
namespace hfe {

class GameSpyCDKeyAuthenticator;

class MMBackend
{
public:
    virtual ~MMBackend() {}
    virtual bool isInitialized() = 0;                          // did the backend come up
    virtual GameSpyCDKeyAuthenticator* getAuthenticator() = 0; // the CD-key authenticator it owns
    virtual void update() = 0;                                 // per-tick (qr2_think when up)
    virtual void shutdown() = 0;                               // tear down the backend's services
};

} // namespace hfe
} // namespace dice

#endif // GAME_MATCHMAKING_SERVER_MMBACKEND_H
