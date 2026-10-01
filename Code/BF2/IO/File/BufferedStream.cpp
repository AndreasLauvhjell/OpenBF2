// Buffered stream operations.
#include "BufferedStream.h"
#include "../../Core/Ids.h"
#include "../../Core/Debug.h"
#include <cstring>

namespace dice { namespace hfe { namespace io {

BufferedStream::Buffer::Buffer() : capacity(0x10000), size(0), position(0), bytes(0) {}
BufferedStream::Buffer::~Buffer() { delete[] bytes; }
void BufferedStream::Buffer::allocate()
{
    if (!bytes) bytes = new char[capacity];
}
BufferedStream::BufferedStream() : m_refCount(1) {}
BufferedStream::~BufferedStream()
{
    BufferedStream::flushWrite();
    // Member destruction frees write/read buffers, then releases async/nonlinear/stream in order.
}
void BufferedStream::addRef() { ++m_refCount; }
int BufferedStream::getRef() { return m_refCount; }
int BufferedStream::release()
{
    const int count = --m_refCount;
    if (!count) delete this;
    return count;
}
void* BufferedStream::queryInterface(unsigned int iid) const
{
    BufferedStream* self = const_cast<BufferedStream*>(this);
    if (iid == IID_IBase || iid == IID_IStream || iid == IID_IBufferedStream)
        return static_cast<IBufferedStream*>(self);
    if (iid == IID_INonlinearStream) return static_cast<INonlinearStream*>(self);
    if (iid == IID_IAsyncStream) return static_cast<IAsyncStream*>(self);
    return 0;
}
IBase* BufferedStream::create(unsigned int iid, IBase* outer)
{
    if (outer) return 0;
    BufferedStream* object = new BufferedStream;
    IBase* result = static_cast<IBase*>(object->queryInterface(iid));
    if (!result) object->release();
    return result;
}
void BufferedStream::setStream(IStream* stream)
{
    flushWrite();
    m_stream.assignQueriedInterface(stream, stream);
    // Release each old secondary before querying its replacement. No read-cache reset is shipped.
    m_nonlinear.assignQueriedInterface<INonlinearStream>(0, 0);
    if (stream)
    {
        INonlinearStream* nonlinear = static_cast<INonlinearStream*>(stream->queryInterface(IID_INonlinearStream));
        m_nonlinear.assignQueriedInterface(nonlinear, nonlinear);
    }
    m_async.assignQueriedInterface<IAsyncStream>(0, 0);
    if (stream)
    {
        IAsyncStream* async = static_cast<IAsyncStream*>(stream->queryInterface(IID_IAsyncStream));
        m_async.assignQueriedInterface(async, async);
    }
}
IStream* BufferedStream::getStream() { return m_stream.get(); }
void BufferedStream::setBufferSize(unsigned int size)
{
    // Retail changes only the capacities. It neither reallocates existing blocks nor resets cursors.
    m_read.capacity = size;
    m_write.capacity = size;
}
unsigned int BufferedStream::getBufferSize() { return m_read.capacity; }
unsigned int BufferedStream::read(void* destination, unsigned int size)
{
    flushWrite();
    m_read.allocate();
    char* dest = static_cast<char*>(destination);
    unsigned int copied = 0;
    if (m_read.position < m_read.size)
    {
        copied = m_read.size - m_read.position;
        if (copied >= size)
        {
            std::memcpy(dest, m_read.bytes + m_read.position, size);
            m_read.position += size;
            return size;
        }
        std::memcpy(dest, m_read.bytes + m_read.position, copied);
        dest += copied;
        size -= copied;
    }
    if (size < m_read.capacity)
    {
        m_read.size = m_stream->read(m_read.bytes, m_read.capacity);
        m_read.position = size <= m_read.size ? size : m_read.size;
        std::memcpy(dest, m_read.bytes, m_read.position);
        return copied + m_read.position;
    }
    m_read.size = 0;
    m_read.position = 0;
    return copied + m_stream->read(dest, size);
}
unsigned int BufferedStream::write(const void* source, unsigned int size)
{
    m_write.allocate();
    if (size >= m_write.capacity)
    {
        flushWrite();
        return m_stream->write(source, size);
    }
    if (m_write.capacity < size + m_write.size) flushWrite();
    std::memcpy(m_write.bytes + m_write.size, source, size);
    m_write.size += size;
    return size;
}
void BufferedStream::flushWrite()
{
    if (m_write.size)
    {
        m_stream->write(m_write.bytes, m_write.size); // short writes are discarded, as shipped
        m_write.size = 0;
    }
}
int BufferedStream::skip(unsigned int size)
{
    flushWrite();
    const unsigned int available = m_read.size - m_read.position;
    if (available > size)
    {
        m_read.position += size;
        return 1;
    }
    m_read.position = 0;
    m_read.size = 0;
    return m_stream->skip(size - available) ? 1 : 0;
}
int BufferedStream::getAvailableBytes()
{
    flushWrite();
    return m_read.size + m_stream->getAvailableBytes() - m_read.position;
}
IStream* BufferedStream::clone()
{
    Debug(DebugTypeError, "IO/File/BufferedStream.cpp", 146, "")
        << "BufferedStream doesn't support cloning yet";
    return 0; // Shipped fatal diagnostic, not a missing implementation.
}
bool BufferedStream::canRead() { return m_stream->canRead(); }
bool BufferedStream::canWrite() { return m_stream->canWrite(); }
int BufferedStream::seek(SeekOrigin origin, int offset)
{
    flushWrite();
    if (!m_nonlinear.get()) return 0;
    if (origin == SEEKORIGIN_CURRENT)
    {
        if (offset == 0) return 1;
        unsigned int position = getPosition();
        const unsigned int size = getSize();
        if (!calcSeek(origin, offset, size, position)) return 0;
        if (offset >= 0) return skip(offset);
        const int bufferedPosition = static_cast<int>(m_read.position + offset);
        if (bufferedPosition >= 0)
        {
            m_read.position = bufferedPosition;
            return 1;
        }
        // Retail clears size only and forwards offset+cached cursor, even after read-ahead.
        m_read.size = 0;
        return m_nonlinear->seek(SEEKORIGIN_CURRENT, bufferedPosition);
    }
    if (origin == SEEKORIGIN_BEGIN)
    {
        const unsigned int position = getPosition();
        if (static_cast<unsigned int>(offset) == position) return 1;
        if (static_cast<unsigned int>(offset) > position) return skip(offset - position);
    }
    m_read.size = 0;
    return m_nonlinear->seek(origin, offset);
}
int BufferedStream::getPosition()
{
    flushWrite();
    return m_nonlinear.get() ? m_read.position + m_nonlinear->getPosition() - m_read.size : 0;
}
int BufferedStream::getSize()
{
    flushWrite();
    return m_nonlinear.get() ? m_nonlinear->getSize() : 0;
}
IAsyncResult* BufferedStream::beginRead(void* dest, unsigned int offset, unsigned int size)
{ return m_async.get() ? m_async->beginRead(dest, offset, size) : 0; }
unsigned int BufferedStream::endRead(IAsyncResult* result)
{ return m_async.get() ? m_async->endRead(result) : 0; }
IAsyncResult* BufferedStream::beginWrite(const void* src, unsigned int offset, unsigned int size)
{ return m_async.get() ? m_async->beginWrite(src, offset, size) : 0; }
unsigned int BufferedStream::endWrite(IAsyncResult* result)
{
    // Both ports deliberately dispatch endRead (+0x34), not endWrite (+0x3C).
    return m_async.get() ? m_async->endRead(result) : 0;
}

} } }
