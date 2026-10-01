// Multi-stream request tracking. [INFERRED] Source placement.
// Construction and factory behavior are [CHOSEN, not recovered].

#include "AsyncMultiStreamer.h"
#include "../../Core/Ids.h"                // [INFERRED] header name; the constants are attested

namespace dice {
namespace hfe {
namespace io {

// [UNRESOLVED] AsyncMultiStream construction and update are not implemented; the storage remains a placeholder.
AsyncMultiStream::AsyncMultiStream(IAsyncStream* /*stream*/)
{
    // [UNRESOLVED] - not read. m_raw is a size placeholder, so there is nothing honest to set.
}

// [UNRESOLVED] body - _ZN4dice3hfe2io16AsyncMultiStream6updateEv, called from
// AsyncMultiStreamer::update at 0x847E9C0.
void AsyncMultiStream::update()
{
    // [UNRESOLVED] - not read.
}

// AsyncMultiStreamer

// [CHOSEN, not recovered] Initializes the streamer with one owned reference.
AsyncMultiStreamer::AsyncMultiStreamer()
    : m_refCount(1)             // [CHOSEN, not recovered] - the one-reference rule
{
}

// Destroys the list nodes without deleting the referenced streams. Remaining stream objects leak.
AsyncMultiStreamer::~AsyncMultiStreamer()
{
    // The vptr restore at 0x847E7A2 and the list teardown are both the compiler's, not statements.
    // Deliberately not deleting the AsyncMultiStream objects - see above.
}

// IBase quartet

// Core/IBase.h declares addRef() void.
void AsyncMultiStreamer::addRef()
{
    ++m_refCount;
}


int AsyncMultiStreamer::getRef()
{
    return m_refCount;
}

// Releases a reference and dispatches the virtual destructor at zero.
int AsyncMultiStreamer::release()
{
    const int count = m_refCount - 1;
    const bool dead = (m_refCount == 1);
    m_refCount = count;

    if (dead)
        delete this;            // virtual, through slot +0x20

    return count;
}

// The uniform two-id shape.
void* AsyncMultiStreamer::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<AsyncMultiStreamer*>(this);
    if (iid == IID_IAsyncMultiStreamer)
        return const_cast<AsyncMultiStreamer*>(this);
    return 0;
}

// IAsyncMultiStreamer

// Creates a multi-stream wrapper and registers it for updates.
// [UNRESOLVED] The concrete stream interface is incomplete; the returned view currently uses a reinterpret cast.
IAsyncMultiStream* AsyncMultiStreamer::beginMultiStream(IAsyncStream* stream)
{
    AsyncMultiStream* created = new AsyncMultiStream(stream);
    m_streams.push_back(created);                               // 0x847E8CE, insert before end()
    return reinterpret_cast<IAsyncMultiStream*>(created);       // see the caveat above
}

// Unregisters stream without deleting it. Always returns false, including when removal succeeds.
bool AsyncMultiStreamer::endMultiStream(IAsyncMultiStream* stream)
{
    for (std::list<AsyncMultiStream*>::iterator it = m_streams.begin(); it != m_streams.end(); ++it)
    {
        if (reinterpret_cast<IAsyncMultiStream*>(*it) == stream)
        {
            m_streams.erase(it);        // the node only; the object is not deleted
            break;
        }
    }

    return false;                       // unconditional. See above.
}

// Updates every registered multi-stream in list order.
void AsyncMultiStreamer::update()
{
    for (std::list<AsyncMultiStream*>::iterator it = m_streams.begin(); it != m_streams.end(); ++it)
        (*it)->update();                // 0x847E9C0, direct call
}

// [CHOSEN, not recovered] Optional class-factory entry matching the registry contract.
// The original server image has no registration for this factory.
IBase* AsyncMultiStreamer::create(unsigned int iid, IBase* outer)   // [CHOSEN, not recovered]
{
    if (outer != 0)
        return 0;

    AsyncMultiStreamer* self = new AsyncMultiStreamer();
    void* itf = self->queryInterface(iid);
    if (itf == 0)
        self->release();
    return static_cast<IBase*>(itf);
}

} // namespace io
} // namespace hfe
} // namespace dice
