// Checksum calculation state. [INFERRED] Header placement.
#ifndef IO_CHECKSUMCONTEXT_H
#define IO_CHECKSUMCONTEXT_H

#include <string>

namespace dice { namespace hfe { namespace io {

// ELF 0x8433B80 / dedicated PE 0x6604C0. Reads exactly 16 bytes and returns lowercase hex.
std::string hash_to_string(const void* hash);

// Computes content checksums shared by server and client.
// MapInfo supplies the ordinal used for archive checksums; ContentCheckEvent carries the resulting digests.
class ChecksumContext
{
public:
    ChecksumContext();
    void setChallengeOrdinal(int ordinal);      // / client PE 6B57E0
    void runMiscChecksum();
    void runArchiveChecksum();                  // / client
    const unsigned char* getMiscHash() const;
    const unsigned char* getArchiveHash() const;   // / client PE 659990
    const unsigned char* getLevelHash() const;     // / client PE 6B5910
private:
    int m_challengeOrdinal;
    unsigned char m_miscHash[16];
    unsigned char m_archiveHash[16];
    unsigned char m_levelHash[16];
};
} } }

#endif
