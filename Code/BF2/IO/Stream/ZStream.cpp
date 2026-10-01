// [INFERRED] path; the class name and every method are the ELF's symbols (848CF20..848D470), the
// PE bodies (6B5620, 6B5480/6B5590, 6B5510/6B5530/6B55C0/6B5550/6B5570/6B5580, 80D430) agree.
#include "ZStream.h"
#include "../ZUtil.h"
#include "../../Core/Ids.h"
#include <new>

namespace dice { namespace hfe { namespace io {

// mode, table, refcount 1, the retained source, then the helper for the mode. A helper
// whose open fails is destroyed and left null, so the stream simply cannot read (or write).
ZStream::ZStream(IStream* source, ZStreamMode mode)
    : m_refCount(1), m_mode(mode), m_source(source), m_reader(0), m_writer(0)
{
    m_source->addRef();
    if (m_mode == 1) {
        m_reader = new ZReader;
        if (m_reader && !m_reader->open(m_source)) {
            delete m_reader;
            m_reader = 0;
        }
    } else {
        m_writer = new ZWriter;
        if (m_writer && !m_writer->open(m_source)) {
            delete m_writer;
            m_writer = 0;
        }
    }
}

// the writer is closed and freed first, then the reader, then the source released.
ZStream::~ZStream()
{
    if (m_writer) {
        m_writer->close();
        delete m_writer;
    }
    if (m_reader) {
        m_reader->close();
        delete m_reader;
    }
    m_source->release();
}

void ZStream::addRef() { ++m_refCount; }
int ZStream::getRef() { return m_refCount; }

// decrement; the owner that reached zero is deleted through the destructor slot.
int ZStream::release()
{
    const int references = --m_refCount;
    if (!references)
        delete this;
    return references;
}

// 848D450 / PE 6B54E0: IBase (1) and IStream (1662503166).
void* ZStream::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IStream)
        return const_cast<ZStream*>(this);
    return 0;
}

unsigned int ZStream::read(void* buffer, unsigned int size)
{
    if (m_reader)
        return m_reader->read(buffer, size);
    return 0;
}

unsigned int ZStream::write(const void* buffer, unsigned int size)
{
    if (m_writer)
        return m_writer->write(buffer, size);
    return 0;
}

// Discards count bytes in chunks of up to 512. Returns one on completion, zero when a read produces no bytes.
int ZStream::skip(unsigned int count)
{
    static unsigned char skipBuf[512];
    unsigned int remaining = count;
    if (!remaining)
        return 1;
    for (;;) {
        const unsigned int chunk = remaining < 512 ? remaining : 512;
        const unsigned int got = read(skipBuf, chunk);
        if (!got)
            return 0;
        remaining -= got;
        if (!remaining)
            return 1;
    }
}

// a reader that has not reached its end reports one byte available; nothing otherwise.
int ZStream::getAvailableBytes()
{
    if (!m_reader)
        return 0;
    return m_reader->eof() ? 0 : 1;
}

IStream* ZStream::clone() { return 0; }
bool ZStream::canRead() { return m_reader != 0; }
bool ZStream::canWrite() { return m_writer != 0; }

} } }
