// Shared stream interfaces and serialization helpers. [INFERRED] Source placement.
#ifndef IO_STREAM_ISTREAM_H     // [INFERRED] guard style
#define IO_STREAM_ISTREAM_H

#include <string>
#include <vector>

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace io {

// Reference-counted byte streams. read/write return transferred byte counts.
// INonlinearStream adds seeking; IAsyncStream may be a secondary base, so use adjusted queryInterface pointers.
// [INFERRED] Seek-origin enumerator names.
enum SeekOrigin
{
    SEEKORIGIN_BEGIN   = 0,
    SEEKORIGIN_CURRENT = 1,
    SEEKORIGIN_END     = 2
};

class IStream : public IBase
{
public:
    // +0x00 addRef, +0x04 getRef, +0x08 release, +0x0C queryInterface - inherited from IBase.
    // +0x08 is called by File.cpp's close()/~File() and was identified independently by demo.cpp.

    virtual unsigned int read(void* buffer, unsigned int size)         = 0;   // +0x10
    virtual unsigned int write(const void* buffer, unsigned int size)  = 0;   // +0x14

    // Advances from the current position and returns the seek result.
    virtual int skip(unsigned int offset)                              = 0;   // +0x18

    virtual int getAvailableBytes()                                    = 0;   // +0x1C

    // Returns a cloned stream, or null when unsupported. Cursor behavior depends on the implementation: ZIP clones start at zero.
    virtual IStream* clone()                                           = 0;   // +0x20

    // Reports stream access capabilities. Stream access encoding differs from filesystem open modes.
    virtual bool canRead()                                             = 0;   // +0x24
    virtual bool canWrite()                                            = 0;   // +0x28

    // Default write-flush hook; performs no operation.
    virtual void flushWrite() {}                                              // +0x2C

protected:
    ~IStream() {}
};

// The repositionable extension - see the header block for the three witnesses that put the trio
// here and not on IStream. Everything file-like implements this (UnixFileStream, Win32FileStream,
// MemStream via IMemMapStream, ZipReadStream); pure network/pipe streams would not.
class INonlinearStream : public IStream
{
public:
    virtual int seek(SeekOrigin origin, int offset)                    = 0;   // +0x30
    virtual int getPosition()                                          = 0;   // +0x34
    virtual int getSize()                                              = 0;   // +0x38

protected:
    ~INonlinearStream() {}
};

// Calculates a seek position and writes pos on success, leaving it unchanged on failure.
// End-relative negative offsets preserve the existing size - offset behavior and may land beyond the end.
bool calcSeek(SeekOrigin origin, int offset, unsigned int size, unsigned int& pos);

// Reads a length-prefixed string from stream into out. [INFERRED] Inline placement.
inline bool read(IStream* stream, std::string& out)
{
    int length = 0;
    stream->read(&length, 4); // The shipped helper deliberately ignores this byte count.

    if (length <= 0)
    {
        out.clear();
        return true;
    }

    // The recovered implementations use an owning, zero-filled byte vector. There is no size cap,
    // nothrow allocation, or exception recovery; normal std::vector exceptions propagate.
    std::vector<char> bytes(static_cast<std::size_t>(length), 0);
    const unsigned int count = stream->read(&bytes[0], static_cast<unsigned int>(length));
    if (count == static_cast<unsigned int>(length))
    {
        out.assign(&bytes[0], count); // Exact count: embedded NUL bytes are retained.
        return true;
    }

    out.clear();
    return false;
}

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_STREAM_ISTREAM_H
