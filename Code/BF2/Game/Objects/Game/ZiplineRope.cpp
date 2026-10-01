// Path:     [INFERRED] - see ZiplineRope.h.
// Contents: complete ZiplineRope leaf recovered from bf2.i64 and bf2_w32ded.exe.

#include "ZiplineRope.h"

#include "../../../Core/Ids.h"
#include "../TemplateLeaves.h"

namespace dice {
namespace hfe {
namespace world {

// ELF C2 0x81CDBD0 / C1 0x81CDC30; PE 0x57B220.  The two ports agree on the
// SimpleObject construction, the 1/0 tail, and updateFlags(0x1804, 0).
ZiplineRope::ZiplineRope(const ZiplineRopeTemplate* objectTemplate)
    : SimpleObject(objectTemplate)
    , m_refCount(1)
    , m_boundingRadiusModifier(0.0f)
{
    updateFlags(0x1804u, 0u);
}

// /0x81CDCC0/0x81CDCF0; PE 0x57B280 family: no owned tail state.
ZiplineRope::~ZiplineRope()
{
}

// /0x81D6070/0x81D6090/0x81D60E0.  The query has exactly two
// answers; notably, it does not expose the inherited IConsoleSaveable subobject.
void ZiplineRope::addRef() { ++m_refCount; }
int ZiplineRope::getRef() { return m_refCount; }
int ZiplineRope::release()
{
    const int left = --m_refCount;
    if (left == 0)
        delete this;
    return left;
}

void* ZiplineRope::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObject)
        return const_cast<ZiplineRope*>(this);
    return 0;
}

// an exact tail call to the SimpleObject implementation.
void ZiplineRope::init()
{
    SimpleObject::init();
}

// an exact tail call; both arguments are forwarded by the ABI.
void ZiplineRope::handleVisualUpdate(float deltaTime, float interpolation)
{
    SimpleObject::handleVisualUpdate(deltaTime, interpolation);
}

// the shipped body is empty, not an unresolved reconstruction stub.
void ZiplineRope::handleUpdate(float, unsigned int)
{
}

// ; PE vtable counterpart: one load from the own trailing float.
float ZiplineRope::getBoundingRadiusModifier() const
{
    return m_boundingRadiusModifier;
}

} // namespace world
} // namespace hfe
} // namespace dice
