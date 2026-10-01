// Process-wide record of the last ranked player's lowercased name/id hashes and
// GameSpy auth token. [INFERRED] Header placement. The initial name-hash sentinel
// is 18193; its meaning remains [UNRESOLVED].

#ifndef GAME_PERSISTENCE_ANTIHACKS_H     // [INFERRED] guard style
#define GAME_PERSISTENCE_ANTIHACKS_H

#include <string>

namespace dice {
namespace hfe {

class AntiHacks
{
public:
    static AntiHacks* instance();                        // lazy singleton

    void addPlayerName(const std::string& name);         // djb2(lowercased) -> +0x00
    void addPlayerID(const std::string& id);             // djb2(lowercased) -> +0x04
    void addGSAuth(const std::string& auth);             // assign            -> +0x08

    ~AntiHacks();                                        // destroys m_gsAuth

    /* +0x00 */ int         m_playerNameHash;            // ctor default 18193 [UNRESOLVED]
    /* +0x04 */ int         m_playerIdHash;              // ctor default 0
    /* +0x08 */ std::string m_gsAuth;                    // ""

private:
    AntiHacks();                                         // reached only through instance()

    static AntiHacks* theInstance;                       // ds:0x8AB08A8
};

} // namespace hfe
} // namespace dice

#endif // GAME_PERSISTENCE_ANTIHACKS_H
