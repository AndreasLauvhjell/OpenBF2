// File-change listener management and directory-watch processing. [INFERRED] Source placement.

#include "FileChangeManager.h"

#include "../../Core/Ids.h"          // IID_IBase
#include "../../Core/StringUtil.h"   // equalsIgnoreCase, fixPathDelimiters, extractFilePath,
                                     // extractFileName, getFixPath
#include "../../Core/System.h"       // g_system->getExactTime()
#include "FileManager.h"             // io::fileManager, fileExistsEx, getFileInfo, getBaseSystem

namespace dice {
namespace hfe {
namespace io {

// Initializes an inactive file-change manager with one reference.
FileChangeManager::FileChangeManager()
    : m_refCount(1)
    , m_initialised(false)
    , m_dirty(false)
    , m_delayTime(0.1f)         // 0x3DCCCCCD, written at 0x8480608
    , m_enable(false)
{
}

// Removes all listeners and watches before releasing container storage.
FileChangeManager::~FileChangeManager()
{
    removeAll();
}

// IBase

// (Materialises the new count in eax; Core/IBase.h
// declares addRef void and every other implementor agrees, so the value is discarded here.)
void FileChangeManager::addRef()
{
    ++m_refCount;
}


int FileChangeManager::getRef()
{
    return m_refCount;
}

// The `&& this` is GCC's null guard on `delete this`;
// the call it guards is vtable +0x34, the deleting destructor.
int FileChangeManager::release()
{
    int count = m_refCount - 1;
    bool last  = (m_refCount == 1);
    m_refCount = count;

    if (last && this)
        delete this;

    return count;
}

// Returns the requested base, file-manager or change-listener interface, or null.
void* FileChangeManager::queryInterface(unsigned int iid) const
{
    FileChangeManager* self = const_cast<FileChangeManager*>(this);

    if (iid == dice::hfe::IID_IBase || iid == IID_IFileChangeManager)
        return static_cast<IFileChangeManager*>(self);

    if (iid == IID_IDirectoryChangeListener)
        return static_cast<IDirectoryChangeListener*>(self);

    return 0;
}

// the two slots BF2Engine actually drives

// Enables queue processing without changing directory registrations. Notifications still accumulate while disabled.
void FileChangeManager::setEnable(bool enable)
{
    m_enable = enable;
}


bool FileChangeManager::getEnable()
{
    return m_enable;
}


void FileChangeManager::setDelayTime(float seconds)
{
    m_delayTime = seconds;
}


float FileChangeManager::getDelayTime()
{
    return m_delayTime;
}

// Processes at most one directory whose debounce delay has expired.
void FileChangeManager::update()
{
    if (!m_enable)
        return;

    float now = static_cast<float>(g_system->getExactTime());

    for (std::vector<DirChange>::iterator it = m_dirChanges.begin(); it != m_dirChanges.end(); ++it)
    {
        // The emitted test is `now <= it->time + m_delayTime  ->  keep looking`.
        if (now <= it->time + m_delayTime)
            continue;

        processChange(it->dir);
        m_dirChanges.erase(it);
        return;
    }
}

// Queues or refreshes a directory notification for later processing.
// New directories are silently dropped when the queue exceeds its cap; trailing arguments are unused.
void FileChangeManager::onDirectoryChange(const std::string& dir, bool /*unknownB*/, unsigned int /*unknownJ*/)
{
    if (m_dirChanges.size() > 10)               // `(end - begin) >> 3 <= 0xA` at 0x8481690
        return;

    float now = static_cast<float>(g_system->getExactTime());

    for (std::vector<DirChange>::iterator it = m_dirChanges.begin(); it != m_dirChanges.end(); ++it)
    {
        if (equalsIgnoreCase(it->dir, dir))
        {
            it->time = now;                     // refresh in place, do not re-queue
            return;
        }
    }

    DirChange change;
    change.dir  = dir;
    change.time = now;
    m_dirChanges.push_back(change);
}

// - the shipped body really is a bare `return true`.
bool FileChangeManager::allowFileChange(const std::string& /*fullName*/)
{
    return true;
}

// registration

// Called lazily by addListener; nothing else calls it,
// and the flag it sets is read nowhere else. It exists so that a FileChangeManager constructed but
// never used costs nothing - except that in this build it costs nothing either way, because the
// body is a single store.
void FileChangeManager::init()
{
    m_initialised = true;
}

// Registers a file listener and returns the resolved path, or an empty string on failure.
std::string FileChangeManager::addListener(IFileChangeListener* listener,
                                           const std::string&   fileName,
                                           unsigned int         userData)
{
    if (!m_initialised)
        init();

    if (fileName.empty())
        return "";

    // Archive files may resolve here even though their virtual directory cannot be watched by the OS.
    std::string full = fileManager->fileExistsEx(fileName, false);
    if (full == "")                     // emitted as compare(const char* "") == 0
        return "";

    fixPathDelimiters(full, '/');

    std::string dir = getFixPath(extractFilePath(full, '/'), '/');
    if (!dir.empty())
        dir = std::string(dir, 0, dir.size() - 1);      // drop the trailing separator

    if (!addPath(dir))
        return "";

    // std::map::operator[] followed by resize(size()+1) - the emitted shape is _M_fill_insert of
    // exactly one element, which is what resize compiles to when it grows by one.
    std::vector<File>& files = m_files[dir];
    files.resize(files.size() + 1);

    File& f    = files.back();
    f.name     = extractFileName(full);   // leaf only - the directory lives in the map key
    f.userData = userData;
    f.time     = getFileTime(full);
    f.listener = listener;

    listener->addRef();
    m_dirty = true;

    return full;
}

// Removes matching listener entries; an empty fileName removes the listener everywhere.
// Only the empty-name branch unregisters directory notifications; the named branch marks the iteration dirty.
bool FileChangeManager::removeListener(IFileChangeListener* listener, const std::string& fileName)
{
    if (fileName.empty())
    {
        for (Files::iterator d = m_files.begin(); d != m_files.end(); ++d)
        {
            // Restart the scan after each erase.
            bool rescan = true;
            while (rescan)
            {
                rescan = false;
                std::vector<File>& files = d->second;

                for (std::vector<File>::iterator f = files.begin(); f != files.end(); ++f)
                {
                    if (f->listener != listener)
                        continue;

                    IFileSystem* base = fileManager->getBaseSystem();
                    IDirectoryChangeNotifier* notifier = static_cast<IDirectoryChangeNotifier*>(
                        base->queryInterface(IID_IDirectoryChangeNotifier));
                    if (notifier)
                        notifier->removeListener(this);

                    f->listener->release();
                    files.erase(f);
                    rescan = true;
                    break;
                }
            }
        }
        return true;            // unconditional - this arm cannot report "not found"
    }

    // This lookup does not normalize the path as addListener does; callers must use the registered spelling.
    std::string dir = extractFilePath(fileName, '/');
    if (!dir.empty())
        dir = std::string(dir, 0, dir.size() - 1);

    Files::iterator d = m_files.find(dir);
    if (d == m_files.end())
        return false;

    bool removed = false;
    std::vector<File>& files = d->second;

    for (std::vector<File>::iterator f = files.begin(); f != files.end(); ++f)
    {
        if (f->listener != listener)
            continue;

        // The left half is dead in this arm; it is in the emitted code and is kept.
        if (!fileName.empty() && !equalsIgnoreCase(extractFileName(fileName), f->name))
            continue;

        f->listener->release();
        files.erase(f);
        m_dirty = true;
        removed = true;
        break;                  // one file per call
    }

    return removed;
}

// Removes listener from every registered path through the virtual filename overload.
bool FileChangeManager::removeListener(IFileChangeListener* listener)
{
    return removeListener(listener, std::string());
}

// Releases registered listeners and empties the file map.
// Directory watches and queued paths remain registered, including when called during destruction.
void FileChangeManager::removeAll()
{
    for (Files::iterator d = m_files.begin(); d != m_files.end(); ++d)
    {
        std::vector<File>& files = d->second;
        for (std::vector<File>::iterator f = files.begin(); f != files.end(); ++f)
            f->listener->release();
    }

    m_files.clear();
    m_dirty = true;
}

// Registers a directory watch. An already known path succeeds without another registration.
bool FileChangeManager::addPath(const std::string& path)
{
    for (std::vector<std::string>::iterator it = m_paths.begin(); it != m_paths.end(); ++it)
    {
        if (equalsIgnoreCase(*it, path))
            return true;
    }

    IFileSystem* base = fileManager->getBaseSystem();
    IDirectoryChangeNotifier* notifier = static_cast<IDirectoryChangeNotifier*>(
        base->queryInterface(IID_IDirectoryChangeNotifier));
    if (!notifier)
        return false;

    // [UNRESOLVED] the last two arguments; 0 and 16 are the literals pushed at 0x8481B5E/0x8481B62.
    if (!notifier->addListener(this, path, false, 16))
        return false;

    m_paths.push_back(path);
    return true;
}

// Returns the original FileInfo word pair as a 64-bit change stamp; this is distinct from FileInfo::time.
long long FileChangeManager::getFileTime(const std::string& name)
{
    FileInfo info;

    if (!fileManager->getFileInfo(name, info))
        return 0;

    return (long long)info.lastWriteTimeLow | ((long long)info.lastWriteTimeHigh << 32);
}

// Refreshes registered timestamps and notifies changed files.
// Timestamps are updated before callbacks; listener changes during a callback restart the iteration.
void FileChangeManager::processChange(const std::string& dir)
{
    std::string path = getFixPath(dir, '/');
    if (!path.empty() && path[path.size() - 1] == '/')
        path = std::string(path, 0, path.size() - 1);

    m_dirty = false;

    bool rescan = true;
    while (rescan)
    {
        rescan = false;

        Files::iterator d = m_files.find(path);
        if (d == m_files.end())
            return;

        std::vector<File>& files = d->second;

        for (std::vector<File>::iterator f = files.begin(); f != files.end(); ++f)
        {
            // The ELF builds `path + "/" + f->name` twice - once for the stat and once for the
            // notification. Built once here; identical result, no observable difference.
            std::string full = path + "/" + f->name;

            long long now = getFileTime(full);
            if (now == f->time)
                continue;

            f->time = now;

            if (allowFileChange(full))                          // vtable +0x3C
                f->listener->onFileChange(full, f->userData);   // listener vtable +0x10

            if (m_dirty)
            {
                m_dirty = false;
                rescan  = true;      // iterators are dead - start over from find()
                break;
            }
        }
    }
}

// create - Identical in shape to
// io::UnixFileSystem::create (0x84FD880) - the two sit 96 bytes apart in the same registry block.
IBase* FileChangeManager::create(unsigned int iid, IBase* outer)
{
    if (outer)                                  // aggregation is not supported anywhere in BF2
        return 0;

    FileChangeManager* p = new FileChangeManager;    // operator new(0x40)

    IBase* result = (IBase*)p->queryInterface(iid);
    if (!result)
        p->release();

    return result;
}

} // namespace io
} // namespace hfe
} // namespace dice
