// Tracks multi-stream requests. Header path [INFERRED].
// Construction and the factory are [CHOSEN, not recovered]; the dedicated registry has no factory
// for its singleton row. Do not publish it as a recovered service.
// Neither destruction nor endMultiStream deletes the pointed-to streams.
// Preserve the three interface methods before the concrete virtual destructor.
// The list layout is chosen; AsyncMultiStream construction and update remain [UNRESOLVED].

#ifndef IO_STREAM_ASYNCMULTISTREAMER_H          // [INFERRED] guard style, per Core/IBase.h
#define IO_STREAM_ASYNCMULTISTREAMER_H

#include <list>

#include "../../Core/IBase.h"

namespace dice {
namespace hfe {
namespace io {

// IAsyncStream  - IID_IAsyncStream      = 3036308476 (Core/Ids.h)
// IAsyncMultiStream - IID_IAsyncMultiStream = 2442294525
// Both left opaque: their own vtables are not read, and inventing slots would be worse than
// omitting them (the precedent IO/IInputFilter.h set).
class IAsyncStream;
class IAsyncMultiStream;

// [UNRESOLVED] Size placeholder for the concrete multi-stream.
// Replace the raw storage and interface cast when its layout and inheritance are established.
class AsyncMultiStream                  // not its home - see above
{
public:
    explicit AsyncMultiStream(IAsyncStream* stream);    // attested signature; body [UNRESOLVED]
    void     update();                                  // attested, non-virtual; body [UNRESOLVED]

private:
    unsigned char m_raw[32];            // sizeof 32 attested; layout [UNRESOLVED]
};

// Multi-stream pump interface. [CHOSEN, not recovered] Interface/concrete boundary.
// The destructor follows the three stream operations.
class IAsyncMultiStreamer : public IBase
{
public:
    virtual IAsyncMultiStream* beginMultiStream(IAsyncStream* stream)      = 0;   // +0x10
    virtual bool               endMultiStream(IAsyncMultiStream* stream)   = 0;   // +0x14
    virtual void               update()                                    = 0;   // +0x18

    virtual ~IAsyncMultiStreamer() {}                                             // +0x1C / +0x20
};

// Tracks multi-streams and updates each one. Removing a stream or destroying the list does not delete its objects.
class AsyncMultiStreamer : public IAsyncMultiStreamer
{
public:
    // [CHOSEN, not recovered] Initializes an empty manager with one reference.
    AsyncMultiStreamer();                                       // [CHOSEN, not recovered]

    // IBase quartet, +0x00..+0x0C
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;       // 0x847F5D0  - _ZNK..., const

    // IAsyncMultiStreamer, +0x10..+0x18
    virtual IAsyncMultiStream* beginMultiStream(IAsyncStream* stream);
    virtual bool               endMultiStream(IAsyncMultiStream* stream);
    virtual void               update();

    // +0x1C / +0x20
    virtual ~AsyncMultiStreamer();

    // [CHOSEN, not recovered] Factory matching the registry signature; no original registration is established.
    static IBase* create(unsigned int iid, IBase* outer);       // [CHOSEN, not recovered]

private:
    /* 0x04 */ int                            m_refCount;
    /* 0x08 */ std::list<AsyncMultiStream*>   m_streams;
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_STREAM_ASYNCMULTISTREAMER_H
