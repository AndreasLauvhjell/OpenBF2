// POSIX disk filesystem. [INFERRED] Header placement.
// [PLATFORM] Paths are normalized and lowercased; copyFile creates a hard link.
// File-info results retain the supplied name. [UNRESOLVED] Purpose of unused stored fields.

#ifndef IO_FILE_UNIXFILESYSTEM_H     // [INFERRED] guard style
#define IO_FILE_UNIXFILESYSTEM_H

#include <list>
#include <string>

#include "IFileSystem.h"

namespace dice {
namespace hfe {
namespace io {

class UnixFileStream;

class UnixFileSystem : public IFileSystem
{
public:
    UnixFileSystem();
    virtual ~UnixFileSystem();                                 // +0x50 / +0x54, 175 B each

    // IBase quartet, +0x00..+0x0C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    // Physical filesystem operations; initialization and archive-origin queries use the base implementations.
    virtual IStream* open(const std::string& name, FileMode, FileAccess, unsigned int);   // +0x18
    virtual bool   fileExists(const std::string& name);                                 // +0x1C
    virtual bool   deleteFile(const std::string& name);                                 // +0x20
    virtual bool   moveFile(const std::string& from, const std::string& to);            // +0x24
    virtual bool   copyFile(const std::string& from, const std::string& to,
                            bool failIfExists);                                         // +0x28
    virtual bool   readFile(const std::string& name, void* buffer, unsigned int size,
                            unsigned int& bytesRead);                                   // +0x2C
    virtual bool   writeFile(const std::string& name, const void* buffer,
                             unsigned int size, bool append);                           // +0x30
    virtual int    findFiles(const std::string& path, bool recursive,
                             std::list<FileInfo>& out);                                 // +0x34
    virtual bool   getFileInfo(const std::string& name, FileInfo& out);                 // +0x38
    virtual bool   compareFileTime(const std::string& a, const std::string& b);        // +0x3C
    virtual bool   getFullFileName(std::string& path);                                  // +0x40
    virtual void   update();                                                            // +0x44
    virtual const std::string& getHomeSubstitution() const;                             // +0x48
    virtual const std::string& getRootSubstitution() const;                             // +0x4C

    // Creates the requested filesystem interface with one reference. Rejects aggregation and unsupported IDs.
    static IBase* create(unsigned int iid, IBase* outer);

    // Normalizes separators and folds path case in place.
    static void fixUnixPath(std::string& path);

    // mkdir -p, stopping at each '/' — so the final component is never created. That is correct for
    // its only recovered caller (writeFile, creating the directory that will contain a file).
    bool createDirectory(const std::string& path);

private:
    // Stream operations maintain the open count and invoke the filesystem's error hook.
    friend class UnixFileStream;

    int findFiles_recursionHelper(const std::string& path, bool recursive,
                                  std::list<FileInfo>& out, unsigned int baseLen);

    // Filesystem error hook; this implementation intentionally emits nothing.
    static void logError(const char* what, const std::string& detail);

    int         m_refCount;        // +0x04
    // [INFERRED] Number of open streams, maintained by stream open, copy and close operations.
    int         m_openStreamCount; // +0x08
    std::string m_unknown0C;       // +0x0C  [UNRESOLVED] ×4
    std::string m_unknown10;       // +0x10
    std::string m_unknown14;       // +0x14
    std::string m_unknown18;       // +0x18
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_FILE_UNIXFILESYSTEM_H
