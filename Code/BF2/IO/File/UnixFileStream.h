// Seekable POSIX file stream. [INFERRED] Header placement.
// [CHOSEN] Separate files for stream operations originally grouped with UnixFileSystem.

#ifndef IO_FILE_UNIXFILESTREAM_H     // [INFERRED] guard style
#define IO_FILE_UNIXFILESTREAM_H

#include <string>

#include "IFileSystem.h"    // io::FileMode / io::FileAccess, and IO/Stream/IStream.h through it

namespace dice {
namespace hfe {
namespace io {

class UnixFileSystem;

class UnixFileStream : public INonlinearStream
{
public:
    UnixFileStream(UnixFileSystem& fileSystem);
    UnixFileStream(const UnixFileStream& other);
    virtual ~UnixFileStream();                             // +0x3C / +0x40

    // IBase quartet, +0x00..+0x0C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    // IStream, +0x10..+0x28 (+0x2C flushWrite is inherited, not overridden)
    virtual unsigned int read(void* buffer, unsigned int size);
    virtual unsigned int write(const void* buffer, unsigned int size);
    virtual int          skip(unsigned int offset);
    virtual int          getAvailableBytes();
    virtual IStream*     clone();
    virtual bool         canRead();
    virtual bool         canWrite();

    // INonlinearStream, +0x30..+0x38
    virtual int seek(SeekOrigin origin, int offset);
    virtual int getPosition();
    virtual int getSize();

    // non-virtual
    // Mangled _ZN4dice3hfe2io14UnixFileStream4openERKSsNS1_8FileModeENS1_10FileAccessEj: four
    // parameters. The 4th is never read; its `= 0` default is [INFERRED] - see the header block.
    bool open(const std::string& name, FileMode mode, FileAccess access,
              unsigned int flags = 0);
    void close();

private:
    int             m_refCount;      // +0x04
    int             m_fd;            // +0x08
    FileAccess      m_access;        // +0x0C  [INFERRED] name/type; never assigned on this platform
    std::string     m_unknown10;     // +0x10  [UNRESOLVED] never read or written
    int             m_unknown14;     // +0x14  [UNRESOLVED] zeroed, copied, never read
    UnixFileSystem& m_fileSystem;    // +0x18
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_FILE_UNIXFILESTREAM_H
