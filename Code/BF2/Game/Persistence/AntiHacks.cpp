// Anti-cheat identity hashes and GameSpy authentication data.
#include "AntiHacks.h"

#include <cctype>                                // isupper / tolower - the djb2 lowercasing

namespace dice {
namespace hfe {

// ds:0x8AB08A8 - the process-wide instance, null until instance() first constructs it. The ELF's
// _GLOBAL__I__ZN4dice3hfe9AntiHacks11theInstanceE is the static-init that zeroes it before main.
AntiHacks* AntiHacks::theInstance = 0;

namespace {

// The djb2 hash the add* setters and GameServer::handleClientInfo share, inlined at each ELF site
// (there is no shared symbol): seed 5381, lowercase each byte, `h = c ^ (33*h)`, walk to the NUL.
// Unsigned arithmetic to make the 32-bit wrap defined; the result is stored into an int field.
int djb2LowerHash(const char* s)
{
    unsigned int h = 5381;
    for (int c = static_cast<unsigned char>(*s); c; c = static_cast<unsigned char>(*++s))
    {
        if (isupper(c))
            c = tolower(c);
        h = static_cast<unsigned int>(c) ^ (33u * h);
    }
    return static_cast<int>(h);
}

} // namespace

// lazy singleton. The ctor below is inlined here in the ELF (new(0xC), the field stores).
AntiHacks* AntiHacks::instance()
{
    if (!theInstance)
        theInstance = new AntiHacks();
    return theInstance;
}

AntiHacks::AntiHacks()
    : m_playerNameHash(18193)                    // 0x4711 - the ELF's default; [UNRESOLVED] meaning
    , m_playerIdHash(0)
    , m_gsAuth()
{
}

// the incoming name's djb2 hash, stored at +0x00.
void AntiHacks::addPlayerName(const std::string& name)
{
    m_playerNameHash = djb2LowerHash(name.c_str());
}

// byte-identical to addPlayerName but stores at +0x04.
void AntiHacks::addPlayerID(const std::string& id)
{
    m_playerIdHash = djb2LowerHash(id.c_str());
}

// a plain assign into the auth string.
void AntiHacks::addGSAuth(const std::string& auth)
{
    m_gsAuth = auth;
}

// only the std::string member needs destroying; the compiler-generated body matches.
AntiHacks::~AntiHacks()
{
}

} // namespace hfe
} // namespace dice
