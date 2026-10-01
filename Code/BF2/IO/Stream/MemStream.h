// Owned memory stream with synchronous and asynchronous interface views. Header path [INFERRED].
// The IMemStream chain is primary; IAsyncStream is secondary and requires pointer adjustment.
// The split placing getPtr on IMemMapStream and size-management methods on IMemStream is
// [CHOSEN, not recovered]; their combined virtual order is fixed by the concrete implementation.
// Member names are [INFERRED].

#ifndef IO_STREAM_MEMSTREAM_H       // [INFERRED] guard style
#define IO_STREAM_MEMSTREAM_H

#include "IStream.h"
#include "../../Core/Ids.h"         // IID_IAsyncResult - DummySyncResult::queryInterface

namespace dice {
namespace hfe {
namespace io {

// The async-completion token. Surface from DummySyncResult<unsigned int>'s vtable (0x88DE988):
// quartet + isCompleted + wait. IID_IAsyncResult = 3936589563 (Core/Ids.h:692).
class IAsyncResult : public IBase
{
public:
    virtual bool isCompleted()                                          = 0;   // +0x10
    virtual bool wait(int)                                              = 0;   // +0x14

protected:
    ~IAsyncResult() {}
};

// Completed result token for synchronous operations exposed through the asynchronous interface.
// The caller owns the token reference; endRead/endWrite do not release it.
template <typename T>
class DummySyncResult : public IAsyncResult
{
public:
    explicit DummySyncResult(const T& value) : m_refCount(1), m_value(value) {}

    virtual void addRef()  { ++m_refCount; }
    virtual int  getRef()  { return m_refCount; }
    virtual int  release()
    {
        const int remaining = --m_refCount;
        if (remaining == 0)
            delete this;
        return remaining;
    }
    virtual void* queryInterface(unsigned int iid) const
    {
        if (iid == IID_IBase || iid == IID_IAsyncResult)
            return const_cast<DummySyncResult*>(this);
        return 0;
    }

    virtual bool isCompleted() { return true; }
    virtual bool wait(int)     { return true; }

    T getValue() const { return m_value; }      // +0x08 - what endRead/endWrite read

private:
    int m_refCount;
    T   m_value;
};

// Asynchronous stream operations take buffer, byte offset and byte count in that order.
// This interface can be a secondary base and requires pointer adjustment.
class IAsyncStream : public IStream
{
public:
    // PE 0x67E790/0x67E7F0 and ELF 0x8489EB0/0x8489F20: offset precedes byte count.
    virtual IAsyncResult* beginRead(void* dest, unsigned int offset,
                                    unsigned int n)                     = 0;   // +0x30
    virtual unsigned int  endRead(IAsyncResult*)                        = 0;   // +0x34
    virtual IAsyncResult* beginWrite(const void* src, unsigned int offset,
                                     unsigned int n)                    = 0;   // +0x38
    virtual unsigned int  endWrite(IAsyncResult*)                       = 0;   // +0x3C

protected:
    ~IAsyncStream() {}
};

// See the header block for why getPtr sits here and the quartet below on IMemStream.
class IMemMapStream : public INonlinearStream
{
public:
    virtual void* getPtr()                                              = 0;   // +0x3C

protected:
    ~IMemMapStream() {}
};

class IMemStream : public IMemMapStream
{
public:
    virtual void         setSize(unsigned int)                          = 0;   // +0x40
    virtual void         clear()                                        = 0;   // +0x44
    virtual void         setSizeLimit(unsigned int)                     = 0;   // +0x48
    virtual unsigned int getSizeLimit()                                 = 0;   // +0x4C

protected:
    ~IMemStream() {}
};

// Growable memory stream. Pre-size it before writing: growth replaces the buffer without preserving or freeing old data.
// clone returns null.
class MemStream : public IMemStream, public IAsyncStream
{
public:
    MemStream();
    virtual ~MemStream();

    // IBase (own bodies; the IAsyncStream face reaches them through thunks)
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();                                        // D0 via vptr+0x54
    virtual void* queryInterface(unsigned int iid) const;           // the six-iid list

    // IStream
    virtual unsigned int read(void* dest, unsigned int n);
    virtual unsigned int write(const void* src, unsigned int n);
    virtual int  skip(unsigned int n);
    virtual int  getAvailableBytes();                               // size - position
    virtual IStream* clone();                                       // returns null
    virtual bool canRead();                                         // true
    virtual bool canWrite();                                        // true
    // Seekable-stream operations; flushWrite retains the empty base implementation.
    virtual int  seek(SeekOrigin origin, int offset);               // calcSeek + store
    virtual int  getPosition();
    virtual int  getSize();

    // IMemMapStream / IMemStream
    virtual void* getPtr();
    virtual void         setSize(unsigned int n);
    virtual void         clear();                                   // size = pos = 0
    virtual void         setSizeLimit(unsigned int n);
    virtual unsigned int getSizeLimit();

    // IAsyncStream (synchronous implementation over memcpy + DummySyncResult)
    virtual IAsyncResult* beginRead(void* dest, unsigned int offset, unsigned int n);
    virtual unsigned int  endRead(IAsyncResult*);
    virtual IAsyncResult* beginWrite(const void* src, unsigned int offset, unsigned int n);
    virtual unsigned int  endWrite(IAsyncResult*);

    // Non-virtual (attested: absent from both vtables)
    void         setMaxSize(unsigned int n);                        // defect 1
    unsigned int getMaxSize();

    static IBase* create(unsigned int iid, IBase* outer);           // canonical factory

private:
    int           m_refCount;     // +0x08
    char*         m_buffer;       // +0x0C
    unsigned int  m_size;         // +0x10
    unsigned int  m_maxSize;      // +0x14
    unsigned int  m_position;     // +0x18
    unsigned int  m_sizeLimit;    // +0x1C
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_STREAM_MEMSTREAM_H
