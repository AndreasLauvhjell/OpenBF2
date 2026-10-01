// Server matchmaking services, browser reporting, and CD-key authentication.
// GameServer constructs this during initialization and ticks it during updates.

#ifndef GAME_GAMESERVER_MATCHMAKINGSERVER_H     // [INFERRED] guard style
#define GAME_GAMESERVER_MATCHMAKINGSERVER_H

#include <list>

namespace dice {
namespace hfe {

class GameServer;
class MMBackend;

// The GameSpy game-id constants (ds:0x888DF48 / 0x888DF4C). Used by GameServer::challengeResponse
// (the CD-key hash / gameId comparison) and, when browsing is un-deferred, by GameSpyBackend.
namespace gamespy {
extern int g_gameID;        // 1059 (0x423) - Battlefield 2
extern int g_XPackGameID;   // 1256         - the Special Forces expansion
extern const char* const g_gameName;   // "battlefield2" (ds:0x8986550)
} // namespace gamespy

// The CD-key authenticator's status result, the 2nd argument of its status callback. Values are the
// switch arms of cdkey_auth_callback (GameServer.cpp): 0 authorized, 1/2/4 various failures, 3 "no
// validation server" (the LAN / offline outcome that still authorises). [INFERRED names]; the enum
// itself is named in the mangled setStatusCallback signature (dice::hfe::AuthResult).
enum AuthResult
{
    AR_AUTHORIZED   = 0,   // > authOk (after the PunkBuster gate)
    AR_KEY_INVALID  = 1,   // > authFailed(19)
    AR_KEY_DISABLED = 2,   // > authFailed(20)
    AR_NO_AUTH      = 3,   // > authOk (no validation server reachable - the LAN outcome)
    AR_KEY_IN_USE   = 4    // > authFailed(37)
};

// The status callback GameServer::init registers (cdkey_auth_callback). context is the GameServer*.
typedef void (*AuthStatusCallback)(int localId, AuthResult result, void* context);

// CD-key authenticator with callback, server context, and Internet/LAN mode.
// [INFERRED] The data-free Authenticator base is folded into this concrete class.
// Preserve virtual declaration order; the original x86 layout is 16 bytes.
class GameSpyCDKeyAuthenticator
{
public:
    explicit GameSpyCDKeyAuthenticator(bool internetServer);
    virtual ~GameSpyCDKeyAuthenticator();                                    //  (D2/D1/D0)

    virtual void setStatusCallback(AuthStatusCallback callback, void* context);  // +0x08
    virtual const char* getChallengeString();                               // +0x0C
    // connectPlayer: the internet branch calls gcd_authenticate_user (the GameSpy validation
    // server); the local branch (bInternetServer == false, i.e. sv.internet 0) calls the status
    // callback synchronously with AR_AUTHORIZED - the whole LAN auth path. Returns dead (0).
    virtual unsigned int connectPlayer(int gameId, int localId, unsigned int userIp,
                                       const char* challenge, const char* response);  // +0x10
    virtual void disconnectPlayer(int gameId, int localId);                 // +0x14
    virtual void update();                                                   // +0x18

    // Layout - offsets read from the ctor / setStatusCallback / connectPlayer bodies.
    /* +0x04 */ AuthStatusCallback m_statusCallback;   // = 0 in the ctor
    /* +0x08 */ void*              m_gameServer;        // = 0 in the ctor; a GameServer*
    /* +0x0C */ bool               m_internetServer;   // = ctor argument
};

// MatchMakingServer - ctor 0x80BF630, getAuthenticator 0x80BF9C0, update 0x80BF950. A thin front
// over a heap `Server` pimpl (getAuthenticator = m_server->authenticator).
class MatchMakingServer
{
public:
    explicit MatchMakingServer(GameServer* gameServer);
    ~MatchMakingServer();                                 // frees the pimpl and its owned backends
    bool initializeServices(bool internetServer);         // stands up GameSpyBackend
    bool shutdownServices();                              // tears the backends down
    GameSpyCDKeyAuthenticator* getAuthenticator() const;
    void update();                                        // cache refresh then backend ticks

    // The pimpl. Size 0x18; field offsets read from the ctor's stores.
    struct Server
    {
        /* +0x00 */ GameServer*                gameServer;
        /* +0x04 */ GameSpyCDKeyAuthenticator* authenticator;   // getAuthenticator returns this
        std::list<MMBackend*>      backends;        // the registered backends (ELF: a 4-byte list node)
        /* +0x0C */ bool                       internetServer;
        /* +0x10 */ int                        queryCacheUpdateCount;   // = 30
        /* +0x14 */ int                        updateCount;             // = 0
    };

private:
    /* +0x00 */ Server* m_server;
};

// addBackend (0x80BF590): registers a backend and, if it initialised, adopts its authenticator.
void addBackend(MMBackend* backend, MatchMakingServer::Server* mmSv);

} // namespace hfe
} // namespace dice

#endif // GAME_GAMESERVER_MATCHMAKINGSERVER_H
