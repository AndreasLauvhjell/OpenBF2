// GameSpy query-reporting and CD-key authentication backend.
// Initialization failures leave it inactive. The SDK availability check may report Available after network failure.

#ifndef GAME_MATCHMAKING_SERVER_GAMESPYBACKEND_H
#define GAME_MATCHMAKING_SERVER_GAMESPYBACKEND_H

#include <string>

#include "MMBackend.h"
#include "MatchMakingServer.h"        // MatchMakingServer::Server, GameSpyCDKeyAuthenticator

namespace dice {
namespace hfe {

// GameSpyBackend - ctor 0x80BE430. RTTI base MMBackend. Layout is self-contained (created via
// `new GameSpyBackend`, used only through MMBackend / its own methods), so the members are logical.
class GameSpyBackend : public MMBackend
{
public:
    explicit GameSpyBackend(MatchMakingServer::Server* mmServer);
    virtual ~GameSpyBackend();

    virtual bool isInitialized();
    virtual GameSpyCDKeyAuthenticator* getAuthenticator();         // returns &m_cdkAuth
    virtual void update();                                         // qr2_think when up
    virtual void shutdown();                                       // gcd_shutdown + qr2_shutdown

    void registerKeys();                                          // the bf2_* browser keys

private:
    int                        m_isInitialized;
    MatchMakingServer::Server*  m_matchmakingSv;
    void*                       m_qr2;              // qr2_t (opaque here to keep qr2.h/Winsock out of this header)
    int                        m_gameSpyPort;
    std::string                m_gameSpyIP;
    bool                       m_allowNATNegotiation;
    GameSpyCDKeyAuthenticator  m_cdkAuth;           // the authenticator, a member (getAuthenticator returns its address)
};

} // namespace hfe
} // namespace dice

#endif // GAME_MATCHMAKING_SERVER_GAMESPYBACKEND_H
