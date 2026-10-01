// Reference-counted zlib stream. Mode one inflates; other modes deflate. [INFERRED] Header placement.
#ifndef IO_STREAM_ZSTREAM_H
#define IO_STREAM_ZSTREAM_H
#include "IStream.h"

namespace dice { namespace hfe { namespace io {
class ZReader;
class ZWriter;

class ZStream : public IStream
{
public:
    // Only the value 1 is attested (both constructors test `mode == 1` for the reader); the
    // enumerator names are [INFERRED]. The demo library passes 1 (PE 4C2A60).
    enum ZStreamMode { ZSTREAMMODE_WRITE = 0, ZSTREAMMODE_READ = 1 };

    ZStream(IStream* source, ZStreamMode mode);
    virtual ~ZStream();                                   // 848D120 body; the +30 table slot (ELF +30/+34)

    virtual void addRef();
    virtual int getRef();
    virtual int release();                                // 848D410 / PE 6CF0E0 (shared): delete at zero
    virtual void* queryInterface(unsigned int iid) const; // IBase, IStream
    virtual unsigned int read(void* buffer, unsigned int size);         // the reader, else 0
    virtual unsigned int write(const void* buffer, unsigned int size);  // the writer, else 0
    virtual int skip(unsigned int count);                 // reads through a 512-byte static
    virtual int getAvailableBytes();                      // 848D380: 1 while the reader is not at end
    virtual IStream* clone();                             // null
    virtual bool canRead();                               // a reader exists
    virtual bool canWrite();                              // a writer exists

private:
    int m_refCount;                                       // +04
    ZStreamMode m_mode;                                   // +08
    IStream* m_source;                                    // +0C  retained for the object's life
    ZReader* m_reader;                                    // +10  mode 1
    ZWriter* m_writer;                                    // +14  every other mode
};

} } }
#endif
