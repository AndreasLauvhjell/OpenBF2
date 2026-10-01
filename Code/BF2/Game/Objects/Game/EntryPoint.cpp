// Vehicle entry-point object.

#include "EntryPoint.h"

#include "../../../Core/Ids.h"
#include "../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"
#include "../TemplateLeaves.h"                       // EntryPointTemplate

namespace dice {
namespace hfe {
namespace world {

namespace {
    // stores handleUpdate's second argument in an unnamed .bss word (ds:0x8AAE118) that
    // nothing else in the ELF reads by name - a leftover dice probe. Kept, TU-static [CHOSEN].
    unsigned int g_entryPointLastUpdateFlags = 0;
}

// SimpleObject(template); the two vptrs; +0x13C = 1; +0x140 = 0.
EntryPoint::EntryPoint(const EntryPointTemplate* objectTemplate)
    : SimpleObject(objectTemplate)
    , m_refCount(1)
    , m_linkedPlayerControlObject(0)
{
}

// / 0x81E0B20 / 0x81E0B60: nothing of its own - ~SimpleObject.
EntryPoint::~EntryPoint()
{
}

// The quartet - 0x81E15D0 / 0x81E1610 / 0x81E1640 / 0x81E16A0 (the -312 thunks land on the same
// bodies).
void EntryPoint::addRef()  { ++m_refCount; }
int  EntryPoint::getRef()  { return m_refCount; }
int  EntryPoint::release()
{
    const int left = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = left;
    if (last)
        delete this;                                             // the D0 slot
    return left;
}

void* EntryPoint::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObject)
        return const_cast<EntryPoint*>(this);
    if (iid == IID_IEntryPointObject)
        return static_cast<IEntryPointObject*>(const_cast<EntryPoint*>(this));          // this + 0x138
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(const_cast<EntryPoint*>(this));       // this + 0x80
    return 0;
}

// SimpleObject::init(); with a parent, walk the parent chain and keep the first
// object that answers IID_IPlayerControlObject - the seat's PCO. Note the walk starts at the
// parent and asks each ancestor's queryInterface (vslot +0x0C), so a PCO nested inside another PCO
// links the nearest one.
void EntryPoint::init()
{
    SimpleObject::init();
    if (m_parent)
    {
        IObject* p = m_parent;
        void* pco = 0;
        for (;;)
        {
            pco = p->queryInterface(IID_IPlayerControlObject);
            if (pco)
                break;
            p = p->getParent();                                                        // +0x28
            if (!p)
                break;
        }
        m_linkedPlayerControlObject = static_cast<IPlayerControlObject*>(pco);        // +0x140
    }
}

// the probe store (see the anonymous namespace).
void EntryPoint::handleUpdate(float, unsigned int flags)
{
    g_entryPointLastUpdateFlags = flags;
}

// enterable unless the linked PCO says it is occupied (IPlayerControlObject +0x18).
// The player and the seat index are not read.
bool EntryPoint::enter(IPlayer*, int&)
{
    if (!m_linkedPlayerControlObject)
        return true;
    return !m_linkedPlayerControlObject->isOccupied();
}


bool EntryPoint::exit(IPlayer*)                                       { return true; }
IPlayer* EntryPoint::getPlayer() const                                { return 0; }
IPlayerControlObject* EntryPoint::getLinkedPlayerControlObject()      { return m_linkedPlayerControlObject; }

} // namespace world
} // namespace hfe
} // namespace dice
