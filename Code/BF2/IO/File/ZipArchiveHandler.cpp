// ZIP archive factory and handler. [INFERRED] Source placement.

#include "ZipArchive.h"

#include "../../Core/Ids.h"

namespace dice {
namespace hfe {
namespace io {

// Reference-counted handler without a destructor slot. The last release frees its storage directly.
void ZipArchiveHandler::addRef()
{
    ++m_refCount;
}

int ZipArchiveHandler::getRef()
{
    return m_refCount;
}

int ZipArchiveHandler::release()
{
    const bool last = (m_refCount == 1);
    --m_refCount;

    if (last)
        ::operator delete(this);        // no destructor exists - see above

    return m_refCount;
}

void* ZipArchiveHandler::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IArchiveHandler)
        return const_cast<ZipArchiveHandler*>(this);

    return 0;
}

// Returns the shared extension string "ZIP PK3".
const std::string& ZipArchiveHandler::getExtensions() const
{
    static const std::string extensions("ZIP PK3");   // 0x8AC3D88, guard 0x8AC3D80
    return extensions;
}

// Returns false: mounted ZIP archives are read-only.
bool ZipArchiveHandler::canWrite() const
{
    return false;
}

// Opens a new ZIP archive with the supplied arguments. The archive retains this handler.
IArchive* ZipArchiveHandler::open(const std::string& file, IFileSystem* baseSystem, bool flag)
{
    ZipArchive* archive = new ZipArchive(*this);        // operator new(0x30) - sizeof == 48

    if (!archive->open(file, baseSystem, flag))
    {
        archive->release();
        return 0;
    }

    return archive;
}

// Creates a handler interface with one owned reference. Returns null for aggregation or an unsupported interface.
IBase* ZipArchiveHandler::create(unsigned int iid, IBase* outer)
{
    if (outer != 0)
        return 0;

    ZipArchiveHandler* self = new ZipArchiveHandler();      // operator new(8u); refCount = 1

    void* itf = self->queryInterface(iid);
    if (itf == 0)
        self->release();

    return static_cast<IBase*>(itf);
}

} // namespace io
} // namespace hfe
} // namespace dice
