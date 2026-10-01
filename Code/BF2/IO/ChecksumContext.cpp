// Checksum calculation and hexadecimal formatting. [INFERRED] Source placement.
#include "ChecksumContext.h"
#include "File/FileManager.h"
#include "File/IFileSystem.h"                    // IArchive::getFilename - runArchiveChecksum
#include "Stream/IStream.h"                      // getSize / seek / read
#include "../Core/System.h"
#include "../Core/StringUtil.h"                  // getLowerCase / formatString
#include "../External/GameSpy/md5.h"
#include <algorithm>                             // std::sort - the ELF's introsort + insertion sort
#include <cstring>
#include <vector>

namespace dice { namespace hfe { namespace io {

ChecksumContext::ChecksumContext() : m_challengeOrdinal(0) {}

const unsigned char* ChecksumContext::getMiscHash() const { return m_miscHash; }

// ELF 0x8433C30 / PE 0x6605C0. Missing files are skipped; no delimiter is hashed between files.
// The two discarded clock reads and repeated available-byte polls are part of the shipped body.
void ChecksumContext::runMiscChecksum()
{
    g_system->getWallClockTime();
    const char* const files[] = {
        "ClientArchives.con", "ServerArchives.con", "Init.con", "GameLogicInit.con", 0
    };
    MD5_CTX md5 = {};
    MD5Init(&md5);
    unsigned char buffer[0x4000];
    for (const char* const* path = files; *path; ++path)
    {
        IStream* stream = fileManager->open(std::string(*path),
            static_cast<IFileSystem::FileMode>(2), static_cast<IFileSystem::FileAccess>(0), 0);
        if (stream)
        {
            while (static_cast<unsigned int>(stream->getAvailableBytes()) > 0)
            {
                const unsigned int count = stream->read(buffer, sizeof(buffer));
                if (count)
                    MD5Update(&md5, buffer, count);
            }
            stream->release();
        }
    }
    MD5Final(m_miscHash, &md5);
    g_system->getWallClockTime();
}

// / client PE 6B57E0 (`mov [ecx], eax`). The ordinal the server put in the
// MapInfo block; runArchiveChecksum starts its sampling from it.
void ChecksumContext::setChallengeOrdinal(int ordinal)
{
    m_challengeOrdinal = ordinal;
}

const unsigned char* ChecksumContext::getArchiveHash() const { return m_archiveHash; }
const unsigned char* ChecksumContext::getLevelHash() const   { return m_levelHash; }

// Computes separate archive and level checksums from sorted archive names.
// Samples every tenth 16 KB block starting at the challenge ordinal, then hashes each file's size.
void ChecksumContext::runArchiveChecksum()
{
    g_system->getWallClockTime();

    std::vector<IArchive*> archives;
    fileManager->getAllArchives(archives);                                     // +0x88
    std::vector<std::string> names;
    for (std::vector<IArchive*>::const_iterator it = archives.begin(); it != archives.end(); ++it)
        names.push_back(getLowerCase((*it)->getFilename()));                  // +0x5C
    std::sort(names.begin(), names.end());

    for (int pass = 1; pass <= 2; ++pass)
    {
        MD5_CTX md5 = {};
        MD5Init(&md5);
        for (std::vector<std::string>::const_iterator name = names.begin(); name != names.end(); ++name)
        {
            const bool isLevel = name->find("/levels/", 0, 8) != std::string::npos;
            if (isLevel ? (pass == 1) : (pass == 2))
                continue;

            // Archive streams must implement INonlinearStream; this path seeks and queries their size without a runtime type check.
            INonlinearStream* stream = static_cast<INonlinearStream*>(
                fileManager->open(*name, static_cast<IFileSystem::FileMode>(2),
                                  static_cast<IFileSystem::FileAccess>(0), 2));
            if (!stream)
                continue;

            const unsigned int fileSize = static_cast<unsigned int>(stream->getSize());   // +0x38
            const unsigned int blocks   = (fileSize >> 14) + 1;
            unsigned char buffer[0x4000];
            for (unsigned int n = static_cast<unsigned int>(m_challengeOrdinal); n < blocks; n += 10)
            {
                memset(buffer, 0, sizeof(buffer));
                stream->seek(static_cast<SeekOrigin>(0), static_cast<int>(n << 14));   // +0x30
                const unsigned int count = stream->read(buffer, sizeof(buffer));       // +0x10
                if (count)
                    MD5Update(&md5, buffer, count);
            }
            std::string sizeText = formatString("%d", fileSize);                       // the ELF re-terminates it
            MD5Update(&md5, reinterpret_cast<unsigned char*>(&sizeText[0]),
                      static_cast<unsigned int>(sizeText.size()));
            stream->release();                                                         // +0x08
        }
        MD5Final(pass == 1 ? m_archiveHash : m_levelHash, &md5);
    }

    g_system->getWallClockTime();
}

std::string hash_to_string(const void* hash)
{
    static const char hextab[] = "0123456789abcdef"; // ELF names this local static.
    const unsigned char* bytes = static_cast<const unsigned char*>(hash);
    char text[33];
    text[32] = 0;
    for (int i = 0; i < 16; ++i)
    {
        const unsigned char value = *bytes++;
        text[2 * i] = hextab[value >> 4];
        text[2 * i + 1] = hextab[value & 15];
    }
    return std::string(text);
}

} } }
