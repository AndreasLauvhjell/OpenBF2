// Mask object template construction and class interfaces.
#include "MaskObjectTemplate.h"
#include "Game/MaskObject.h"
#include "../../Core/Ids.h"
#include <cstdlib>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace dice {
namespace hfe {
namespace world {

// /0x818D800; PE 0x590EE0. Refcount and interface quartet are inherited.
MaskObjectTemplate::MaskObjectTemplate()
    : AnimatedBundleTemplate(), m_maskType(MOTNoMask), m_maskOffset{0.0f, -1.55f, 0.08f}
{
}

// ELF D1 0x818DA10 / D0 0x818DA50; PE shares AnimatedBundleTemplate's deleting destructor.
MaskObjectTemplate::~MaskObjectTemplate()
{
}


unsigned int MaskObjectTemplate::getClassID() const
{
    return CID_MaskObjectTemplate;
}

// test both own and AnimatedBundle CID, then Bundle directly.
void* MaskObjectTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_MaskObjectTemplate || classId == CID_AnimatedBundleTemplate)
        return this;
    return BundleTemplate::queryImplementation(classId);
}

// allocates 0x15C; PE 0x590D40 allocates 0x174 and inlines the derived ctor.
IObject* MaskObjectTemplate::createObject() const
{
    return new MaskObject(this);
}

bool MaskObjectTemplate::makeScript(io::IStream*)
{
    // [PLATFORM] This is shipped behavior, not an unrecovered frontier. ELF 0x818D8A0 calls
    // abort unconditionally. PE IConsoleSaveable table 0x7D3984+0x10 points to 0x590D90:
    // CC / xor al,al / retn 4. Preserve the breakpoint and false result if a debugger resumes.
#if defined(_MSC_VER)
    __debugbreak();
    return false;
#else
    std::abort();
#endif
}


void MaskObjectTemplate::setMaskType(MaskObjectType type)
{
    m_maskType = type;
}

MaskObjectType MaskObjectTemplate::getMaskType() const
{
    return m_maskType;
}

} // namespace world
} // namespace hfe
} // namespace dice
