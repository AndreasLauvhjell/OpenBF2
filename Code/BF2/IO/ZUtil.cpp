// Zlib stream adapters. Version macros describe the vendored library used by this build.
#include "ZUtil.h"
#include "Stream/IStream.h"
#include "../Core/Debug.h"
#include "zlib.h"
#include <cstring>
#include <new>

namespace dice { namespace hfe { namespace io {

// ZReader

// the three flag bytes, then the z_stream allocation (left uninitialized until open
// zeroes it), no source, a 4 KB staging buffer.
ZReader::ZReader()
    : m_eof(false), m_byte1(false), m_inputExhausted(false), m_zstream(0), m_stream(0),
      m_bufferSize(4096), m_buffer(0)
{
    m_zstream = static_cast<z_stream_s*>(::operator new(sizeof(z_stream)));
    m_buffer = new unsigned char[4096];
}

// close, free the z_stream, free the buffer when there is one.
ZReader::~ZReader()
{
    close();
    ::operator delete(m_zstream);
    if (m_buffer)
        delete[] m_buffer;
}

// close first; a null source or a failed inflateInit refuses. next_in starts at the
// staging buffer with nothing available; the source is retained.
bool ZReader::open(IStream* stream)
{
    close();
    if (!stream)
        return false;
    if (inflateInit(m_zstream) != Z_OK)
        return false;
    m_zstream->next_in = m_buffer;
    m_zstream->avail_in = 0;
    m_zstream->next_out = 0;
    m_zstream->avail_out = 0;
    m_stream = stream;
    stream->addRef();
    return true;
}

// the source is released and the inflate state ended only while open; the z_stream is
// zeroed either way. The flag bytes are not reset here.
void ZReader::close()
{
    if (m_stream) {
        m_stream->release();
        m_stream = 0;
        inflateEnd(m_zstream);
    }
    std::memset(m_zstream, 0, sizeof(z_stream));
}

// nothing to do while input remains; otherwise one source read into the staging
// buffer. True when that read returned nothing.
bool ZReader::fill_zinput()
{
    if (m_zstream->avail_in)
        return false;
    const unsigned int got = m_stream->read(m_buffer, m_bufferSize);
    m_zstream->next_in = m_buffer;
    m_zstream->avail_in = got;
    return got == 0;
}

// Inflates up to size bytes into buffer and returns the count produced.
// Stops at stream end, source exhaustion or a zlib error; subsequent end-of-stream reads return zero.
unsigned int ZReader::read(void* buffer, unsigned int size)
{
    if (m_eof)
        return 0;
    m_zstream->next_out = static_cast<Bytef*>(buffer);
    m_zstream->avail_out = size;
    if (m_zstream->avail_out) {
        for (;;) {
            if (!m_zstream->avail_in && !m_inputExhausted)
                m_inputExhausted = fill_zinput();
            const int result = inflate(m_zstream, Z_NO_FLUSH);
            if (result == Z_STREAM_END)
                m_eof = true;
            if (result != Z_OK || m_eof)
                break;
            if (!m_zstream->avail_out)
                break;
        }
    }
    return static_cast<unsigned int>(m_zstream->next_out - static_cast<Bytef*>(buffer));
}

// at end, or not open.
bool ZReader::eof() const
{
    return m_eof || !m_stream;
}

// ZWriter


ZWriter::ZWriter()
    : m_zstream(0), m_stream(0), m_bufferSize(4096), m_buffer(0)
{
    m_zstream = static_cast<z_stream_s*>(::operator new(sizeof(z_stream)));
    m_buffer = new unsigned char[4096];
}


ZWriter::~ZWriter()
{
    close();
    ::operator delete(m_zstream);
    if (m_buffer)
        delete[] m_buffer;
}

// close first; a null sink or a failed deflateInit at level 3 refuses; the sink is
// retained.
bool ZWriter::open(IStream* stream)
{
    close();
    if (!stream)
        return false;
    if (deflateInit(m_zstream, 3) != Z_OK)
        return false;
    m_stream = stream;
    stream->addRef();
    return true;
}

// while open, the pending data is finished into the sink before it is released; the
// z_stream is zeroed either way.
void ZWriter::close()
{
    if (m_stream) {
        flushData();
        m_stream->release();
        m_stream = 0;
    }
    std::memset(m_zstream, 0, sizeof(z_stream));
}

// Compresses input into the sink through a staging buffer and returns the consumed count.
// Z_BUF_ERROR and Z_STREAM_ERROR emit fatal diagnostics.
unsigned int ZWriter::write(const void* buffer, unsigned int size)
{
    m_zstream->next_in = static_cast<Bytef*>(const_cast<void*>(buffer));
    m_zstream->avail_in = size;
    m_zstream->next_out = m_buffer;
    m_zstream->avail_out = m_bufferSize;
    if (!m_zstream->avail_in && m_zstream->avail_out)
        return size;
    for (;;) {
        m_zstream->next_out = m_buffer;
        m_zstream->avail_out = m_bufferSize;
        const int result = deflate(m_zstream, Z_NO_FLUSH);
        if (result == Z_BUF_ERROR) {
            Debug(DEBUGTYPE_6, "IO/ZUtil.cpp", 168, "") << "zlib buffer error, compression failed";
            return 0;
        }
        if (result == Z_STREAM_ERROR) {
            Debug(DEBUGTYPE_6, "IO/ZUtil.cpp", 172, "") << "zlib buffer error, inconsistent stream state";
            return 0;
        }
        const unsigned int produced = m_bufferSize - m_zstream->avail_out;
        if (produced)
            m_stream->write(m_buffer, produced);
        if (!m_zstream->avail_in && m_zstream->avail_out)
            return size;
    }
}

// Finishes compression and writes remaining output until stream end. Zlib buffer and stream errors are fatal.
void ZWriter::flushData()
{
    for (;;) {
        m_zstream->avail_in = 0;
        m_zstream->next_in = m_buffer;
        m_zstream->avail_out = m_bufferSize;
        m_zstream->next_out = m_buffer;
        const int result = deflate(m_zstream, Z_FINISH);
        if (result == Z_BUF_ERROR) {
            Debug(DEBUGTYPE_6, "IO/ZUtil.cpp", 196, "") << "zlib buffer error, compression failed";
            return;
        }
        if (result == Z_STREAM_ERROR) {
            Debug(DEBUGTYPE_6, "IO/ZUtil.cpp", 200, "") << "zlib buffer error, inconsistent stream state";
            return;
        }
        m_stream->write(m_buffer, m_bufferSize - m_zstream->avail_out);
        if (result == Z_STREAM_END) {
            deflateEnd(m_zstream);
            return;
        }
    }
}

} } }
