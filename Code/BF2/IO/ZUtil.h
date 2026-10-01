// zlib readers and writers used by ZStream.
#ifndef IO_ZUTIL_H
#define IO_ZUTIL_H

struct z_stream_s; // zlib; the header stays free of zlib.h

namespace dice { namespace hfe { namespace io {
class IStream;

// in both ports: three flag bytes, the z_stream, the source stream, the
// staging buffer's size and pointer.
class ZReader
{
public:
    ZReader();
    ~ZReader();
    bool open(IStream* stream);                  // 843A670 / PE 6D54B0: close, inflateInit, retain
    void close();                                // release the source, inflateEnd, zero the z_stream
    bool fill_zinput();                          // refill next_in from the source; true once it is dry
    unsigned int read(void* buffer, unsigned int size);
    bool eof() const;                            // 843A850 / PE 6D5150: at end, or no source

private:
    bool m_eof;                                  // +00  Z_STREAM_END reached
    bool m_byte1;                                // +01  [UNRESOLVED] zeroed by both constructors, never read
    bool m_inputExhausted;                       // +02  the last source read returned nothing
    z_stream_s* m_zstream;                       // +04  new(0x38), zeroed by close
    IStream* m_stream;                           // +08  retained while open
    unsigned int m_bufferSize;                   // +0C  4096
    unsigned char* m_buffer;                     // +10  new[](0x1000)
};

// : the z_stream, the sink stream, the staging buffer's size and pointer.
class ZWriter
{
public:
    ZWriter();
    ~ZWriter();
    bool open(IStream* stream);                  // 843A970 / PE 6D55A0: close, deflateInit level 3, retain
    void close();                                // flushData, release the sink, zero the z_stream
    unsigned int write(const void* buffer, unsigned int size);
    void flushData();                            // 843ACA0 / PE 6D5310: Z_FINISH until Z_STREAM_END, then deflateEnd

private:
    z_stream_s* m_zstream;                       // +00  new(0x38)
    IStream* m_stream;                           // +04  retained while open
    unsigned int m_bufferSize;                   // +08  4096
    unsigned char* m_buffer;                     // +0C  new[](0x1000)
};

} } }
#endif
