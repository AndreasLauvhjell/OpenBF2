// [INFERRED] path. Full client PE410742/4C2510/4C25D0/4C2590/479830/4C3030/4C2A60/4C1D90/4C1FF0/
// /4C1B90/689430/4C1BA0/4C1BB0/4C1BC0/4B3AB0/4B1C80/4C1C00/4C1DB0 read; the HTTP-side
// bodies 4C2DF0/4C1B70/4C1B80/4C1BD0 read and left as diagnostic stops at their first use of the
// withheld HTTPRequest base.
#include "DemoLibrary.h"
#if !DEDICATED
#include "DemoHeader.h"
#include "../LocalProfileManager.h"
#include "../../Menu/ISwiffHost.h"       // postMessage - reportError's {1, text, 0} record
#include "../../Core/ClassManager.h"
#include "../../Core/Ids.h"
#include "../../Core/StringUtil.h"
#include "../../IO/File/FileManager.h"
#include "../../IO/Stream/ZStream.h"
#include <cstdio>
#include <cstdlib>
#include <list>
#include <new>

namespace dice { namespace hfe {
extern ClassManager* classManager;

DemoLibraryEntry::DemoLibraryEntry() : version(0) {} // constructs the five strings; the int is written by refresh

// (124 B): the withheld base's constructor (59C390) runs first in retail; then the table,
// refcount 1, the empty entry vector, the four strings, progress 0, the two flags clear and the
// pending bookmark -1.
DemoLibrary::DemoLibrary()
    : m_refCount(1), m_downloadProgress(0.0f), m_flagA4(false), m_pendingBookmark(-1), m_downloadFailed(false)
{
}

// (131 B): the four strings in reverse order, the entry rows and their storage, then the
// withheld base's destructor (59C3C0).
DemoLibrary::~DemoLibrary() {}

// (66 B): the settings-factory shape - aggregation refused, 176 retail bytes, construction
// only on a non-null allocation, unconditional queryInterface (the shipped null defect is kept).
IBase* DemoLibrary::create(unsigned int iid, IBase* outer)
{
    if (outer) return nullptr;
    void* allocation = ::operator new(sizeof(DemoLibrary));
    DemoLibrary* owner = allocation ? new (allocation) DemoLibrary : nullptr;
    IBase* result = static_cast<IBase*>(owner->queryInterface(iid));
    if (!result) owner->release();
    return result;
}
void DemoLibrary::addRef() { ++m_refCount; }
int DemoLibrary::getRef() { return m_refCount; }
// (26 B): decrement, destroy at zero through the destructor slot of the withheld base's table.
int DemoLibrary::release()
{
    const int references = --m_refCount;
    if (!references) delete this;
    return references;
}
// (36 B): IBase (1), then IDemoLibrary (121065).
void* DemoLibrary::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IDemoLibrary)
        return const_cast<DemoLibrary*>(this);
    return nullptr;
}

// (8 B): the scan's own result is discarded.
bool DemoLibrary::refresh()
{
    refreshEntries();
    return true;
}

// Clears existing entries, then scans the default profile's demos directory when
// a current profile exists. Adds readable bf2demo headers and releases the inflate
// wrapper before its underlying stream.
bool DemoLibrary::refreshEntries()
{
    std::vector<DemoLibraryEntry>().swap(m_entries);
    IBase* manager = classManager->getSingleton(std::string("LocalProfileManager"));
    if (!manager || !static_cast<ILocalProfileManager*>(manager)->getCurrentProfile())   // +44
        return false;
    std::string directory("@HOME@/Profiles/Default/demos/");
    std::list<io::FileInfo> files;
    io::fileManager->findFiles(directory, false, files);                                  // +34
    for (std::list<io::FileInfo>::iterator it = files.begin(); it != files.end(); ++it) {
        io::FileInfo info = *it;
        if (info.isDirectory)
            continue;
        if (!(extractFileExt(info.name) == "bf2demo"))
            continue;
        std::string path(std::string("@HOME@/Profiles/Default/demos/") += info.name);
        io::IStream* stream = io::fileManager->open(path, io::FILEMODE_READ, static_cast<io::FileAccess>(0), 0);
        if (!stream)
            continue;
        void* allocation = ::operator new(sizeof(io::ZStream));
        if (allocation) {
            io::ZStream* reader = new (allocation) io::ZStream(stream, io::ZStream::ZSTREAMMODE_READ);
            if (reader) {
                DemoHeader header;
                if (header.readFrom(reader)) {
                    DemoLibraryEntry entry;
                    entry.version = header.version;
                    entry.serverName = header.serverName;
                    entry.dateTime = header.dateTime;
                    entry.mapName = header.mapName;
                    entry.path = path;
                    entry.name = extractFileNameWithoutExt(entry.path);
                    m_entries.push_back(entry);
                }
                reader->release();
            }
        }
        stream->release();
    }
    return true;
}

// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: demo-library-http-request]
// Stops until the HTTP request base and transfer lifecycle are recovered.
bool DemoLibrary::download(const std::string&, const std::string&, const std::string&)
{
    std::fprintf(stderr, "[FRONTIER] dice::hfe::DemoLibrary::download: the HTTPRequest base (Game/Persistence/HTTPRequest.cpp, PE59C390) that PE4C2DF0 tests first is unrecovered\n");
    std::fflush(stderr);
    std::abort();
}

unsigned int DemoLibrary::getNumEntries()
{
    return static_cast<unsigned int>(m_entries.size());
}

//: the count is read through the table.
DemoLibraryEntry* DemoLibrary::getEntry(unsigned int index)
{
    if (index < getNumEntries())
        return &m_entries[index];
    return nullptr;
}

// (113 B): past the count nothing happens; otherwise the row's file is deleted (the result
// is not read) and the row erased.
bool DemoLibrary::removeEntry(unsigned int index)
{
    if (index >= getNumEntries())
        return false;
    io::fileManager->deleteFile(m_entries[index].path);                                  // +20
    m_entries.erase(m_entries.begin() + index);
    return true;
}

// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: demo-library-http-request]
// / 4C1B80 / 4C1BD0 forward to the withheld base (59C420 think, 59C400 busy, 59C440 cancel
// followed by the target file's deletion).
int DemoLibrary::update()
{
    std::fprintf(stderr, "[FRONTIER] dice::hfe::DemoLibrary::update: HTTPRequest 59C420 (PE4C1B70) is unrecovered\n");
    std::fflush(stderr);
    std::abort();
}
bool DemoLibrary::isDownloading()
{
    std::fprintf(stderr, "[FRONTIER] dice::hfe::DemoLibrary::isDownloading: HTTPRequest 59C400 (PE4C1B80) is unrecovered\n");
    std::fflush(stderr);
    std::abort();
}
bool DemoLibrary::cancelDownload()
{
    std::fprintf(stderr, "[FRONTIER] dice::hfe::DemoLibrary::cancelDownload: HTTPRequest 59C440 (PE4C1BD0) is unrecovered\n");
    std::fflush(stderr);
    std::abort();
}

const std::string& DemoLibrary::getDownloadName() { return m_downloadName; }
const std::string& DemoLibrary::getDownloadUrl() { return m_downloadUrl; }
const std::string& DemoLibrary::getDownloadDescription() { return m_downloadDescription; }
float DemoLibrary::getDownloadProgress() { return m_downloadProgress; }
bool DemoLibrary::getFlagA4() { return m_flagA4; }
void DemoLibrary::setFlagA4(bool value) { m_flagA4 = value; }
void DemoLibrary::setPendingBookmark(int index) { m_pendingBookmark = index; }
bool DemoLibrary::hasDownloadFailed() { return m_downloadFailed; }

// (117 B): the SwiffHost singleton, when present, receives the message record {1, text, 0}
// through +1E4; absent, nothing happens. Reached by the download path and the failure callback.
void DemoLibrary::reportError(const std::string& message)
{
    IBase* host = classManager->getSingleton(std::string("SwiffHost"));
    if (host)
        static_cast<ui::ISwiffHost*>(host)->postMessage(SwiffHostMessage(1, message, 0));   // +0x1E4
}

} }
#endif
