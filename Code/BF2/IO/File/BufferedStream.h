// Buffered stream with three distinct IStream subobjects and no virtual inheritance. [INFERRED] Header placement.
#ifndef IO_FILE_BUFFEREDSTREAM_H
#define IO_FILE_BUFFEREDSTREAM_H
#include "../Stream/MemStream.h"
#include "../../Core/SmartPtr.h"

namespace dice { namespace hfe { namespace io {

class IBufferedStream : public IStream
{
public:
    virtual void setStream(IStream*) = 0;
    virtual IStream* getStream() = 0;
    virtual void setBufferSize(unsigned int) = 0;
    virtual unsigned int getBufferSize() = 0;
protected:
    ~IBufferedStream() {}
};

class BufferedStream : public IBufferedStream, public INonlinearStream, public IAsyncStream
{
public:
    BufferedStream();
    virtual ~BufferedStream();
    virtual void addRef();
    virtual int getRef();
    virtual int release();
    virtual void* queryInterface(unsigned int) const;
    virtual unsigned int read(void*, unsigned int);
    virtual unsigned int write(const void*, unsigned int);
    virtual int skip(unsigned int);
    virtual int getAvailableBytes();
    virtual IStream* clone();
    virtual bool canRead();
    virtual bool canWrite();
    virtual void flushWrite();
    virtual void setStream(IStream*);
    virtual IStream* getStream();
    virtual void setBufferSize(unsigned int);
    virtual unsigned int getBufferSize();
    virtual int seek(SeekOrigin, int);
    virtual int getPosition();
    virtual int getSize();
    virtual IAsyncResult* beginRead(void*, unsigned int, unsigned int);
    virtual unsigned int endRead(IAsyncResult*);
    virtual IAsyncResult* beginWrite(const void*, unsigned int, unsigned int);
    virtual unsigned int endWrite(IAsyncResult*);
    static IBase* create(unsigned int, IBase*);
private:
    struct Buffer
    {
        Buffer();
        ~Buffer();
        void allocate();
        unsigned int capacity, size, position;
        char* bytes;
    };
    int m_refCount;                     // +0x0C on both retail ports
    SmartPtr<IStream> m_stream;         // independently retained interfaces, +0x10/+0x14/+0x18
    SmartPtr<INonlinearStream> m_nonlinear;
    SmartPtr<IAsyncStream> m_async;
    Buffer m_read;                     // +0x1C
    Buffer m_write;                    // +0x2C
};

} } }
#endif
