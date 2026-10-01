// File ownership and filesystem forwarding. [INFERRED] Source placement.

#include "File.h"

#include "FileManager.h"     // io::fileManager - open() goes through the virtual filesystem
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace io {

// Owns one stream reference and releases it on destruction.
File::File()
    : m_file(0)
{
}

// Opens path with the requested mode, access and flags.
// [UNRESOLVED] Whether the original constructor forwards flags or always passes zero.
File::File(const std::string& path, IFileSystem::FileMode mode, IFileSystem::FileAccess access, unsigned int flags)
    : m_file(0)
{
    open(path, mode, access, flags);
}

File::~File()
{
    if (m_file)
        m_file->release();                      // IStream vptr +0x08
}

// Closes the previous stream and opens path through FileManager.
// A failed open leaves this File closed.
bool File::open(const std::string& path, IFileSystem::FileMode mode, IFileSystem::FileAccess access, unsigned int flags)
{
    close();

    IStream* opened = fileManager->open(path, mode, access, flags);   // FileManager vptr, virtual

    if (m_file)                                 // dead after close(), reproduced anyway
    {
        m_file->release();
        m_file = 0;
    }

    // [CHOSEN] cast, paired with the m_file retype in File.h: every stream a filesystem produces
    // is file-like (UnixFileStream / Win32FileStream / MemStream / ZipReadStream all sit on the
    // INonlinearStream spine and answer its IID), and File forwards the seek trio that lives there.
    m_file = static_cast<INonlinearStream*>(opened);
    return m_file != 0;
}

bool File::isOpen()
{
    return m_file != 0;
}

// Releases an open file and clears the handle.
// [CHOSEN] No return value; existing callers do not consume the original indeterminate closed-file result.
void File::close()
{
    if (m_file)
    {
        m_file->release();                      // IStream vptr +0x08
        m_file = 0;
    }
}

// The raw pair: straight forwards, no null check on m_file
//  neither guards m_file. read/write on a closed File dereferences null in the original too;
// isOpen() exists precisely because these do not check.
unsigned int File::read(void* buffer, unsigned int size)
{
    return m_file->read(buffer, size);          // IStream vptr +0x10
}

unsigned int File::write(const void* buffer, unsigned int size)
{
    return m_file->write(buffer, size);         // IStream vptr +0x14
}

int File::seek(SeekOrigin origin, int offset) { return m_file->seek(origin, offset); }  // +0x30
int File::getPosition()                       { return m_file->getPosition(); }         // +0x34
int File::getAvailableBytes()                 { return m_file->getAvailableBytes(); }   // +0x1C
int File::getSize()                           { return m_file->getSize(); }             // +0x38

IStream* File::getStream()                      { return m_file; }

// Reads or writes raw native bytes; success requires the complete value size. No endian conversion is performed.
bool File::read(unsigned int& v)   { return read(&v, sizeof(v)) == sizeof(v); }
bool File::read(int& v)            { return read(&v, sizeof(v)) == sizeof(v); }
bool File::read(unsigned short& v) { return read(&v, sizeof(v)) == sizeof(v); }
bool File::read(short& v)          { return read(&v, sizeof(v)) == sizeof(v); }
bool File::read(unsigned char& v)  { return read(&v, sizeof(v)) == sizeof(v); }
bool File::read(char& v)           { return read(&v, sizeof(v)) == sizeof(v); }
bool File::read(float& v)          { return read(&v, sizeof(v)) == sizeof(v); }
bool File::read(double& v)         { return read(&v, sizeof(v)) == sizeof(v); }
bool File::read(bool& v)           { return read(&v, sizeof(v)) == sizeof(v); }

// Reads wide characters through delimiter, consuming it and returning true even on EOF or short read.
// Embedded NUL truncates out. Input must fit the unchecked 4096-character buffer.
bool File::read(std::wstring& out, wchar_t delimiter)
{
    wchar_t buffer[4096];
    unsigned int count = 0;
    while (m_file->read(buffer + count, sizeof(wchar_t)) == sizeof(wchar_t)
           && buffer[count] != delimiter)
        ++count;
    buffer[count] = 0;
    out = buffer;
    return true;
}

bool File::write(unsigned int v)   { return write(&v, sizeof(v)) == sizeof(v); }
bool File::write(int v)            { return write(&v, sizeof(v)) == sizeof(v); }
bool File::write(unsigned short v) { return write(&v, sizeof(v)) == sizeof(v); }
bool File::write(short v)          { return write(&v, sizeof(v)) == sizeof(v); }
bool File::write(unsigned char v)  { return write(&v, sizeof(v)) == sizeof(v); }
bool File::write(char v)           { return write(&v, sizeof(v)) == sizeof(v); }
bool File::write(float v)          { return write(&v, sizeof(v)) == sizeof(v); }
bool File::write(double v)         { return write(&v, sizeof(v)) == sizeof(v); }
bool File::write(bool v)           { return write(&v, sizeof(v)) == sizeof(v); }

// Writes the string bytes without a length prefix or trailing NUL.
bool File::write(const std::string& v)
{
    return write(v.c_str(), static_cast<unsigned int>(v.size())) == v.size();
}

// Reads or writes vector components as raw structure bytes, without endian conversion.
bool File::read(Vec2& v)  { return read(&v, sizeof(v)) == sizeof(v); }
bool File::read(Vec3& v)  { return read(&v, sizeof(v)) == sizeof(v); }
bool File::read(Vec4& v)  { return read(&v, sizeof(v)) == sizeof(v); }

bool File::write(const Vec2& v) { return write(&v, sizeof(v)) == sizeof(v); }
bool File::write(const Vec3& v) { return write(&v, sizeof(v)) == sizeof(v); }
bool File::write(const Vec4& v) { return write(&v, sizeof(v)) == sizeof(v); }

} // namespace io
} // namespace hfe
} // namespace dice
