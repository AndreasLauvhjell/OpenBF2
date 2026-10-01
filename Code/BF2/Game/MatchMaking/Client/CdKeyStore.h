// Client CD-key storage and dpapi conversion. Values prefixed with x9392 attempt
// decryption and retain the payload unchanged on failure. Unmarked values are
// encrypted and returned as marker plus hex, even to challenge-response callers.
// The expansion product mapping recognizes xpack only.

#ifndef GAME_MATCHMAKING_CLIENT_CDKEYSTORE_H          // [INFERRED] guard style
#define GAME_MATCHMAKING_CLIENT_CDKEYSTORE_H
#if !DEDICATED

#include <iosfwd>
#include <string>

struct _CRYPTOAPI_BLOB;                               // Win32 DATA_BLOB - kept out of this header

namespace dice {
namespace hfe {

// Owns a dpapi blob through one pointer. [INFERRED] class/member names.
// [CHOSEN, not recovered] Copying is disabled to prevent double-free; original
// callers only use stack-local instances and never copy them.
class ProtectedData
{
public:
    ProtectedData();                                  // 3 instructions - m_blob = 0
    ~ProtectedData();                                 // release()

    // . CryptProtectData over plainText including its NUL. False on an empty string
    // (and on a dpapi failure); any blob already held is released first.
    bool protect(const std::string& plainText);

    // . hexText is lowercase hex of a blob; on success it is replaced by the decrypted
    // plaintext. The body never touches `this` - it decodes into its own buffer - so the holder is
    // along for the ride. Kept a member because the shipped call passes one in ecx.
    bool unprotect(std::string& hexText);

    // . Writes the held blob as lowercase hex, two digits per byte, zero-filled.
    // std::setw is one-shot, so it is re-applied every iteration - as shipped.
    void writeHex(std::ostream& stream);

private:
    ProtectedData(const ProtectedData&);              // [CHOSEN, not recovered] - see above
    ProtectedData& operator=(const ProtectedData&);   // [CHOSEN, not recovered]

    void release();                                   // inlined at both of its sites in retail

    _CRYPTOAPI_BLOB* m_blob;
};

// [INFERRED names] Key-store helpers; only translateCdKey takes its string by reference.
std::string getRegistryCdKey(std::string productName);

// . Marked -> decrypt. Unmarked -> encrypt. See the block at the top of this file.
std::string translateCdKey(const std::string& storedKey);

// . The retail product name whose registry key holds the expansion's CD key.
std::string getExpansionProductName(std::string modName);

// . The GameSpy product id for the expansion (gamespy::g_XPackGameID = 1256), or -1.
// The client body spells 1256 as an immediate; handleChallenge's base-game id, by contrast, is a
// global read (ds:8E66BC = gamespy::g_gameID). Both spellings are kept where they belong.
int getExpansionGameId(std::string modName);

} // namespace hfe
} // namespace dice

#endif // !dedicated
#endif // GAME_MATCHMAKING_CLIENT_CDKEYSTORE_H
