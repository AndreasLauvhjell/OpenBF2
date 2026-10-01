// Windows filesystem, file streams and asynchronous I/O support.

// Preserve internal linkage for the header-defined ID table. FileChangeManager.cpp owns the
// external io:: file-change IDs; seeing its extern declarations first would emit a second copy.
#include "../../Core/Ids.h"
#include "Win32FileSystem.h"
#include "../../Core/Debug.h"
#include "../../Core/LoadStat.h"
#include "../../Core/StringUtil.h"
#include "../../Core/SmartItf.h"
#include "BufferedStream.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>      // SHGetFolderPathW(CSIDL_PERSONAL), crash-directory setup
#include <shellapi.h>   // SHFileOperationW directory deletion

namespace dice {
namespace hfe {
namespace io {

namespace {
// UTF-8/UTF-16 conversion helpers. Conversion failures are not checked. [INFERRED] Helper names.
std::wstring widenUtf8(const std::string& text)
{
    const int count = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, 0, 0);
    wchar_t* buffer = new wchar_t[count + 1];
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, buffer, count);
    buffer[count] = 0;
    const std::wstring result(buffer);
    delete[] buffer;
    return result;
}

std::string narrowUtf8(std::wstring text)
{
    const int count = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, 0, 0, 0, 0);
    char* buffer = new char[count];
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, buffer, count, 0, 0);
    const std::string result(buffer);
    delete[] buffer;
    return result;
}

// A drive separator anywhere enables the extended-path prefix.
std::string normalizeFileName(std::string path)
{
    fixPathDelimiters(path, '\\');
    return path;
}

std::wstring widenFileName(const std::string& path, bool extended)
{
    if (extended && (path.find(":/") != std::string::npos ||
                     path.find(":\\") != std::string::npos))
        return widenUtf8("\\\\?\\" + path);
    return widenUtf8(path);
}

// PE 0x676990 / 0x676900 retain the original missing empty-path guard.
std::string normalizeDirectory(std::string path)
{
    fixPathDelimiters(path, '\\');
    if (path.c_str()[path.size() - 1] != '\\') path += '\\';
    return path;
}

std::wstring normalizeWideDirectory(std::wstring path)
{
    for (std::size_t i = 0; i < path.size(); ++i)
        if (path[i] == L'/') path[i] = L'\\';
    if (path.c_str()[path.size() - 1] != L'\\') path += L'\\';
    return path;
}

// PE 0x40CC80 and 0x675880: make every backslash-delimited prefix, ignoring API failures.
std::wstring extractWideDirectory(const std::wstring& path)
{
    const int back = static_cast<int>(path.find_last_of(L'\\'));
    const int forward = static_cast<int>(path.find_last_of(L'/'));
    const int last = back > forward ? back : forward;
    return last == -1 ? std::wstring() : path.substr(0, last) + L"\\";
}

void createPathDirectories(const std::wstring& path)
{
    for (int i = 0, length = static_cast<int>(path.size()); i < length; ++i)
        if (path[i] == L'\\') CreateDirectoryW(path.substr(0, i).c_str(), 0);
}

// Repeated txt comparison is shipped.
bool isTextFile(const std::wstring& path)
{
    const std::size_t dot = path.find_last_of(L'.');
    const std::wstring extension = dot == std::wstring::npos ||
        (dot > 1 && path[dot - 1] == L'.') ? std::wstring() : path.substr(dot + 1);
    const wchar_t* types[] = {L"con", L"inc", L"log", L"txt", L"twk", L"tweak", L"font",
                             L"info", L"txt", L"fx", L"desc", L"ini", L"cfg", L"ai"};
    for (unsigned int i = 0; i < sizeof(types) / sizeof(types[0]); ++i)
        if (_wcsicmp(extension.c_str(), types[i]) == 0) return true;
    return false;
}

unsigned int writeTextFile(HANDLE handle, const void* source, unsigned int size)
{
    // nonzero OS write returns the original input size, even for a partial write.
    if (size == 0) return 0;
    char* converted = new char[2 * size];
    unsigned int length = 0;
    char previous = 0;
    const char* bytes = static_cast<const char*>(source);
    for (unsigned int i = 0; i < size; ++i)
    {
        if (previous != '\r' && bytes[i] == '\n') converted[length++] = '\r';
        converted[length++] = bytes[i];
        previous = bytes[i];
    }
    DWORD written = 0;
    WriteFile(handle, converted, length, &written, 0);
    delete[] converted;
    return written ? size : 0;
}

// PE 0x676200 is shared by getHomeSubstitution(true) and the dump-directory helper(false).
// A single cached wide path is important: startup's first lookup also establishes @home@.
const std::wstring& applicationDataDirectory(bool diagnose)
{
    static std::wstring documents;
    if (documents.empty())
    {
        wchar_t buffer[MAX_PATH];
        buffer[0] = 0;
        if (FAILED(SHGetFolderPathW(0, CSIDL_PERSONAL, 0, 0, buffer)))
        {
            if (diagnose)
                Debug(DebugTypeLog, "IO/File/Win32FileSystem.cpp", 87, "")
                    << "Couldn't resolve application data folder, terminating.";
            ExitProcess(1);
        }
        buffer[MAX_PATH - 1] = 0;
        documents = buffer;
        documents += L"\\Battlefield 2";
    }
    return documents;
}

// UNC paths retain the final slash; other paths lose one final separator.
std::wstring stripFinalSeparator(const std::wstring& path)
{
    const bool unc = path.size() > 2 && (path[0] == L'/' || path[0] == L'\\') &&
                     (path[1] == L'/' || path[1] == L'\\');
    if (path.empty() || unc) return path;
    const wchar_t last = path[path.size() - 1];
    return (last == L'/' || last == L'\\') ? path.substr(0, path.size() - 1) : path;
}
} // namespace

// Owns asynchronous-operation state and its event handle. [INFERRED] Class name.
// The x86 layout places overlapped at +0x10 and the owned handle at +0x24.
class Win32AsyncResult : public IAsyncResult
{
public:
    Win32AsyncResult() : m_refCount(1), asynchronous(false), bytes(0), overlapped(), m_handle(0) {}
    void addRef() override { ++m_refCount; }
    int getRef() override { return m_refCount; }
    int release() override
    {
        const int remaining = --m_refCount;
        if (remaining == 0) delete this;
        return remaining;
    }
    void* queryInterface(unsigned int iid) const override
    {
        return iid == IID_IBase || iid == IID_IAsyncResult ?
            const_cast<Win32AsyncResult*>(this) : 0;
    }
    bool isCompleted() override { return !asynchronous || overlapped.Internal != 259; }
    bool wait(int milliseconds) override
    {
        // Retail's synchronous result reports completed, but wait returns false.
        return asynchronous && WaitForSingleObject(overlapped.hEvent, milliseconds) != WAIT_TIMEOUT;
    }
    static Win32AsyncResult* pending(unsigned int offset)
    {
        Win32AsyncResult* result = new Win32AsyncResult;
        result->asynchronous = true;
        result->overlapped.Offset = offset;
        result->overlapped.hEvent = CreateEventA(0, TRUE, FALSE, 0);
        return result;
    }
    static Win32AsyncResult* completed(unsigned int count)
    {
        Win32AsyncResult* result = new Win32AsyncResult;
        result->bytes = count;
        result->asynchronous = false;
        return result;
    }
private:
    ~Win32AsyncResult()
    {
        CloseHandle(overlapped.hEvent);
        CloseHandle(m_handle); // Both closes are unconditional, including null.
    }
    int m_refCount;
public:
    bool asynchronous;
    unsigned int bytes;
    OVERLAPPED overlapped;
private:
    HANDLE m_handle;
};

// PE primary/secondary tables 0x7F68E8 / 0x7F68A8. Both real bases, all 19 distinct
// virtual bodies, ctor 0x676D00, copy ctor 0x677690 and dtor 0x6770B0 recovered.
class Win32FileStream : public INonlinearStream, public IAsyncStream
{
public:
    Win32FileStream(Win32FileSystem* owner, unsigned char text)
        : m_refCount(1), m_mode(1), m_handle(0), m_asyncHandle(0), m_pending(false),
          m_text(text), m_position(0), m_flags(0), m_cachedSize(0), m_owner(owner)
    {
        m_owner->addRef();
    }
    Win32FileStream(const Win32FileStream& source, unsigned char text)
        : m_refCount(1), m_mode(1), m_handle(0), m_asyncHandle(0), m_pending(false),
          m_text(text), m_position(0), m_cachedSize(0), m_owner(source.m_owner)
    {
        // The copy opens at position zero, ignores failure, then retains its owner.
        openFile(source.m_path, IFileSystem::FILEMODE_READ,
                 static_cast<IFileSystem::FileAccess>(source.m_mode), source.m_flags);
        m_owner->addRef();
    }
    void addRef() override { ++m_refCount; }
    int getRef() override { return m_refCount; }
    int release() override
    {
        const int remaining = --m_refCount;
        if (remaining == 0) delete this;
        return remaining;
    }
    void* queryInterface(unsigned int iid) const override
    {
        Win32FileStream* self = const_cast<Win32FileStream*>(this);
        if (iid == IID_IBase || iid == IID_IStream || iid == IID_INonlinearStream)
            return static_cast<INonlinearStream*>(self);
        if (iid == IID_IAsyncStream) return static_cast<IAsyncStream*>(self);
        return 0;
    }
    unsigned int read(void* buffer, unsigned int size) override
    {

        if (size == 0 || m_mode == 2) return 0;
        DWORD got = 0;
        if (!ReadFile(m_handle, buffer, size, &got, 0)) return 0;
        m_position += got;
        return got;
    }
    unsigned int write(const void* buffer, unsigned int size) override
    {
        // PE 0x675DA0, including text conversion's original-size position increment.
        if (size == 0 || m_mode == 0) return 0;
        DWORD put = 0;
        if (m_text) put = writeTextFile(m_handle, buffer, size);
        else if (!WriteFile(m_handle, buffer, size, &put, 0))
        {
            m_position += 0;
            return 0;
        }
        m_position += put;
        return put;
    }
    int skip(unsigned int offset) override
    { return seek(SEEKORIGIN_CURRENT, static_cast<int>(offset)); }
    int getAvailableBytes() override
    {
        const int size = getSize();
        return size - getPosition();
    }
    IStream* clone() override
    {
        // PE 0x677710 returns QI without an extra addRef or release.
        Win32FileStream* copy = new Win32FileStream(*this, m_text);
        return static_cast<IStream*>(copy->queryInterface(IID_IStream));
    }
    bool canRead() override { return m_mode == 0 || m_mode == 1; }
    bool canWrite() override { return m_mode == 2 || m_mode == 1; }
    void flushWrite() override {} // PE 0x539F20 is an actual empty retail function.
    int seek(SeekOrigin origin, int offset) override
    {
        // returns 0/1, not the new position. Unknown nonzero origins mean current.
        DWORD method;
        if (origin == SEEKORIGIN_BEGIN)
        {
            method = FILE_BEGIN;
            if (m_mode == 0 && m_position == offset) return 1;
        }
        else method = origin == SEEKORIGIN_END ? FILE_END : FILE_CURRENT;
        const DWORD moved = SetFilePointer(m_handle, offset, 0, method);
        if (moved == INVALID_SET_FILE_POINTER) return 0;
        m_position = moved;
        return 1;
    }
    int getPosition() override
    {
        // PE 0x675E70 caches the read-only cursor and otherwise queries the OS.
        if (m_mode == 0) return m_position;
        const DWORD position = SetFilePointer(m_handle, 0, 0, FILE_CURRENT);
        return position == INVALID_SET_FILE_POINTER ? 0 : position;
    }
    int getSize() override
    {
        // PE 0x675EA0 caches a nonzero read-only size.
        DWORD size = m_cachedSize;
        if (size == 0)
        {
            size = GetFileSize(m_handle, 0);
            if (size == INVALID_FILE_SIZE) return 0;
            if (m_mode == 0) m_cachedSize = size;
        }
        return size;
    }
    IAsyncResult* beginRead(void* buffer, unsigned int offset, unsigned int size) override
    {
        // second argument is offset, third count. Do not swap to match old
        // parameter labels in IAsyncStream; both positions have the same unsigned-int type.
        if (size == 0 || m_mode == 2) return 0;
        if ((m_flags & 1) && size > 500)
        {
            Win32AsyncResult* result = Win32AsyncResult::pending(offset);
            m_pending = true;
            if (!ReadFile(m_asyncHandle, buffer, size, 0, &result->overlapped) &&
                GetLastError() != ERROR_IO_PENDING)
            {
                result->release();
                return 0;
            }
            return result;
        }
        const int position = getPosition();
        if (!seek(SEEKORIGIN_BEGIN, offset)) return 0;
        const unsigned int count = read(buffer, size);
        if (count == 0 || !seek(SEEKORIGIN_BEGIN, position)) return 0;
        return Win32AsyncResult::completed(count);
    }
    unsigned int endRead(IAsyncResult* token) override
    {
        // The caller owns the token; no release here.
        Win32AsyncResult* result = static_cast<Win32AsyncResult*>(token);
        if (!result->asynchronous) return result->bytes;
        if (!result->isCompleted()) result->wait(100000);
        DWORD count = 0;
        m_pending = false;
        return GetOverlappedResult(m_asyncHandle, &result->overlapped, &count, TRUE) ? count : 0;
    }
    IAsyncResult* beginWrite(const void*, unsigned int, unsigned int) override
    {
        // PE 0x675FB0 is this diagnostic in retail, not an implementation frontier.
        Debug(DebugTypeError, "IO/File/Win32FileSystem.cpp", 542, "")
            << "Do not use async write, doesn't work for win32 text format conversion";
        return 0;
    }
    unsigned int endWrite(IAsyncResult* token) override
    {
        // PE 0x676030 differs from endRead only by leaving m_pending unchanged.
        Win32AsyncResult* result = static_cast<Win32AsyncResult*>(token);
        if (!result->asynchronous) return result->bytes;
        if (!result->isCompleted()) result->wait(100000);
        DWORD count = 0;
        return GetOverlappedResult(m_asyncHandle, &result->overlapped, &count, TRUE) ? count : 0;
    }
    bool openFile(const std::wstring& path, IFileSystem::FileMode mode,
                  IFileSystem::FileAccess access, unsigned int flags)
    {
        // Do not reset position or cached size here: the constructor owns those.
        if (path.empty()) return false;
        if (m_handle) CloseHandle(m_handle);
        m_handle = 0;
        if (m_asyncHandle) CloseHandle(m_asyncHandle);
        m_asyncHandle = 0;
        m_path = path;
        m_flags = flags;
        m_mode = access;
        ++loadStat->m_unknown28; // PE LoadStat +0x58.
        loadStat->setCurrentFile(makeNarrow(path));
        DWORD disposition;
        switch (mode)
        {
        case 0: disposition = CREATE_ALWAYS; break;
        case 1: disposition = CREATE_NEW; break;
        case 2: disposition = OPEN_EXISTING; break;
        case 3: disposition = OPEN_ALWAYS; break;
        default: return false;
        }
        DWORD desired;
        switch (access)
        {
        case 0: desired = GENERIC_READ; break;
        case 1: desired = GENERIC_READ | GENERIC_WRITE; break;
        case 2: desired = GENERIC_WRITE; break;
        default: return false;
        }
        const DWORD attributes = access == 0 ? 0x08000080 : FILE_ATTRIBUTE_NORMAL;
        if (mode == 0 || mode == 1 || mode == 3)
            createPathDirectories(normalizeWideDirectory(extractWideDirectory(m_path)));
        m_handle = CreateFileW(m_path.c_str(), desired, 3, 0, disposition, attributes, 0);
        if (m_handle == INVALID_HANDLE_VALUE) return false;
        if ((flags & 1) == 0 || access == 2) return true;
        m_asyncHandle = CreateFileW(m_path.c_str(), desired, 3, 0, OPEN_EXISTING,
                                    attributes | FILE_FLAG_OVERLAPPED, 0);
        return m_asyncHandle != INVALID_HANDLE_VALUE;
    }
private:
    friend class Win32FileSystem;
    ~Win32FileStream()
    {
        if (m_handle) CloseHandle(m_handle);
        m_handle = 0;
        if (m_asyncHandle) CloseHandle(m_asyncHandle);
        m_asyncHandle = 0;
        m_owner->release();
    }
    int m_refCount;                    // PE +0x08
    int m_mode;                        // PE +0x0C, FileAccess rather than FileMode
    HANDLE m_handle, m_asyncHandle;    // PE +0x10 / +0x14
    bool m_pending;                    // PE +0x18
    std::wstring m_path;               // PE +0x1C
    unsigned char m_text;              // PE +0x38
    int m_position;                    // PE +0x3C
    unsigned int m_flags, m_cachedSize;// PE +0x40 / +0x44
    Win32FileSystem* m_owner;          // PE +0x48
};

// Reference counting destroys the object when the decremented count reaches zero.
// Destruction is nonvirtual; the primary interface has no destructor slot.
void Win32FileSystem::addRef()
{
    ++m_refCount;
}

int Win32FileSystem::getRef()
{
    return m_refCount;
}

int Win32FileSystem::release()
{
    const int remaining = --m_refCount;

    if (remaining == 0)
        delete this;

    return remaining;
}

// Returns the requested filesystem or directory-notifier interface, with the required base-pointer adjustment.
void* Win32FileSystem::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IFileSystem)
        return static_cast<IFileSystem*>(const_cast<Win32FileSystem*>(this));

    if (iid == IID_IDirectoryChangeNotifier)
        return static_cast<IDirectoryChangeNotifier*>(const_cast<Win32FileSystem*>(this));

    return 0;
}

// PE ctor 0x677DB0 constructs map, deque and notification list in member order.
// The three words at +0x10 are untouched; source STL owns the compiler-specific representation.
Win32FileSystem::Win32FileSystem()
    : m_refCount(1)
    , m_unknown0C(0)
{
}

// reverse member destruction, including listener ownership.
Win32FileSystem::~Win32FileSystem()
{
    // list destruction releases listeners but does not close their
    // notification handles. removeListener owns the explicit close; preserve this defect.
}

// Default filesystem initialization intentionally does nothing.
void IFileSystem::init()
{
}

// Returns false by default; FileManager overrides this to track archive-backed opens.
bool IFileSystem::isLastFileOpenedFromArchive() const
{
    return false;
}

// Dedicated PE 0x676440. Directories use the shell's recursive delete, without the extended
// filename prefix; ordinary files use DeleteFileW. ELF UnixFileSystem deliberately uses unlink.
bool Win32FileSystem::deleteFile(const std::string& name)
{
    FileInfo info;
    if (!getFileInfo(name, info)) return false;
    const std::string normalized = normalizeFileName(name);
    if (info.isDirectory)
    {
        std::wstring wide = widenUtf8(normalized);
        wide += L'\0'; // pFrom is double-NUL terminated.
        SHFILEOPSTRUCTW operation = {};
        operation.wFunc = FO_DELETE;
        operation.pFrom = wide.c_str();
        operation.fFlags = 0x414;
        return SHFileOperationW(&operation) == 0;
    }
    return DeleteFileW(widenFileName(normalized, true).c_str()) != 0;
}

// PE 0x6765E0 / 0x6766E0 normalize the destination before the source.
bool Win32FileSystem::moveFile(const std::string& from, const std::string& to)
{
    const std::wstring destination = widenFileName(normalizeFileName(to), true);
    const std::wstring source = widenFileName(normalizeFileName(from), true);
    return MoveFileW(source.c_str(), destination.c_str()) != 0;
}

bool Win32FileSystem::copyFile(const std::string& from, const std::string& to, bool failIfExists)
{
    const std::wstring destination = widenFileName(normalizeFileName(to), true);
    const std::wstring source = widenFileName(normalizeFileName(from), true);
    return CopyFileW(source.c_str(), destination.c_str(), failIfExists) != 0;
}

// Reads a file into buffer and reports bytesRead. Success means opening succeeded, even if the read fails.
// Updates loading counters without invoking a progress callback.
bool Win32FileSystem::readFile(const std::string& name, void* buffer, unsigned int size,
                             unsigned int& bytesRead)
{
    Win32FileStream stream(this, 0);
    bool opened = false;
    {
        const std::string normalized = normalizeFileName(name);
        const std::wstring wide = widenFileName(normalized, true);
        if (!wide.empty())
        {
            if (stream.m_handle) CloseHandle(stream.m_handle);
            stream.m_handle = 0;
            if (stream.m_asyncHandle) CloseHandle(stream.m_asyncHandle);
            stream.m_asyncHandle = 0;
            stream.m_path = wide;
            stream.m_mode = 0;
            stream.m_flags = 0;
            ++loadStat->m_unknown28;
            loadStat->setCurrentFile(makeNarrow(wide)); // PE 0x40C840, locale conversion here.
            stream.m_handle = CreateFileW(stream.m_path.c_str(), GENERIC_READ, 3, 0,
                                          OPEN_EXISTING, 0x08000080, 0);
            opened = stream.m_handle != INVALID_HANDLE_VALUE;
        }
    }
    if (!opened)
    {
        bytesRead = 0;
        return false;
    }
    unsigned int available = stream.m_cachedSize;
    if (available == 0)
    {
        available = GetFileSize(stream.m_handle, 0);
        if (available == INVALID_FILE_SIZE)
        {
            bytesRead = 0;
            return true;
        }
        if (stream.m_mode == 0) stream.m_cachedSize = available;
    }
    if (available > size) available = size;
    DWORD got = 0;
    if (available == 0 || stream.m_mode == 2 ||
        !ReadFile(stream.m_handle, buffer, available, &got, 0))
        bytesRead = 0;
    else
    {
        stream.m_position += got;
        bytesRead = got;
    }
    return true;
}

// The bool chooses CREATE_NEW(1) / CREATE_ALWAYS(2); it is not append.
// Empty writes return false, and selected text extensions insert missing CR before each LF.
bool Win32FileSystem::writeFile(const std::string& name, const void* buffer,
                              unsigned int size, bool overwrite)
{
    const std::wstring wide = widenFileName(normalizeFileName(name), true);
    createPathDirectories(normalizeWideDirectory(extractWideDirectory(wide)));
    HANDLE handle = CreateFileW(wide.c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0,
                                1 + overwrite, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle == INVALID_HANDLE_VALUE) return false;
    DWORD written = 0;
    if (isTextFile(wide)) written = writeTextFile(handle, buffer, size);
    else WriteFile(handle, buffer, size, &written, 0);
    CloseHandle(handle);
    return written != 0;
}

// preserve insertion order and reload the record handle after the callback.
void Win32FileSystem::update()
{
    for (std::list<Notification>::iterator it = m_notifications.begin();
         it != m_notifications.end(); ++it)
    {
        if (WaitForSingleObject(it->handle, 0) == WAIT_OBJECT_0)
        {
            it->listener->onDirectoryChange(it->path, it->recursive, it->flags);
            FindNextChangeNotification(it->handle);
        }
    }
}

bool Win32FileSystem::addListener(IDirectoryChangeListener* listener, const std::string& path,
                                bool recursive, unsigned int flags)
{
    // retain before assigning path and opening; failed open pops the new record.
    Notification temporary;
    m_notifications.push_back(temporary);
    Notification& entry = m_notifications.back();
    entry.listener.assignQueriedInterface(listener, listener);
    entry.path = path;
    entry.recursive = recursive;
    entry.flags = flags;
    unsigned int filter = (flags & 1) != 0;
    if (flags & 2) filter |= 2;
    if (flags & 4) filter |= 4;
    if (flags & 8) filter |= 8;
    if (flags & 16) filter |= 16;
    std::string normalized = normalizeDirectory(entry.path);
    if (normalized.empty()) normalized = "\\";
    entry.handle = FindFirstChangeNotificationA(normalized.c_str(), entry.recursive, filter);
    if (entry.handle == INVALID_HANDLE_VALUE)
    {
        m_notifications.pop_back();
        return false;
    }
    return true;
}

void Win32FileSystem::removeListener(IDirectoryChangeListener* listener)
{
    // PE 0x677A90 removes only the first match.
    for (std::list<Notification>::iterator it = m_notifications.begin();
         it != m_notifications.end(); ++it)
        if (it->listener.get() == listener)
        {
            FindCloseChangeNotification(it->handle);
            m_notifications.erase(it);
            return;
        }
}

// UTF-8 path, access-dependent text conversion, real native stream, and the
// ClassManager-created BufferedStream when flags & 3 is zero. Failure destroys the raw stream.
IStream* Win32FileSystem::open(const std::string& name, FileMode mode, FileAccess access,
                             unsigned int flags)
{
    // PE 0x677750 widens before normalizing. Buffered wrapper helper 0x677EC0 is SmartItf.
    const std::wstring wide = widenFileName(name, true);
    const bool text = (access == 2 || access == 1) ? isTextFile(wide) : false;
    Win32FileStream* stream = new Win32FileStream(this, text);
    std::wstring normalized = wide;
    for (std::size_t i = 0; i < normalized.size(); ++i)
        if (normalized[i] == L'/') normalized[i] = L'\\';
    if (!stream->openFile(normalized, mode, access, flags))
    {
        delete stream;
        return 0;
    }
    if ((flags & 3) == 0)
    {
        SmartItf<IBufferedStream, &IID_IBufferedStream> holder;
        holder.create(CID_BufferedStream);
        IBufferedStream* buffered = holder;
        buffered->setStream(static_cast<IStream*>(stream->queryInterface(IID_IStream)));
        buffered->addRef();
        unsigned int size = stream->getSize();
        if (size == 0 || size > 0x80000) size = 0x80000;
        buffered->setBufferSize(size);
        stream->release();
        IStream* result = static_cast<INonlinearStream*>(
            buffered->queryInterface(IID_INonlinearStream));
        buffered->release();
        return result;
    }
    return static_cast<INonlinearStream*>(stream);
}

// Returns file information in shared static storage, or null for a missing or dot-prefixed entry.
// The next call overwrites the record; this helper is not thread-safe.
namespace {

const FileInfo* findOneEntry(const std::string& name)
{
    if (name.empty())
        return 0;                                    // the size test, before any syscall

    const std::wstring wide = widenFileName(normalizeFileName(name), true);

    WIN32_FIND_DATAW findData;
    HANDLE handle = FindFirstFileW(wide.c_str(), &findData);
    if (handle == INVALID_HANDLE_VALUE)
        return 0;
    FindClose(handle);

    static FileInfo s_found;                         // one shared record, see above

    if (findData.cFileName[0] == L'.')               // the DOT rule
        return 0;

    FileInfo built;                                  // sub_6767E0, with the L"" prefix (0x784844)
    std::wstring foundName(findData.cFileName);
    for (std::size_t i = 0; i < foundName.size(); ++i)
        if (foundName[i] == L'\\') foundName[i] = L'/';
    built.name = narrowUtf8(foundName);
    built.isDirectory        = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    built.size               = findData.nFileSizeLow;
    built.creationTimeLow    = findData.ftCreationTime.dwLowDateTime;
    built.creationTimeHigh   = findData.ftCreationTime.dwHighDateTime;
    built.lastAccessTimeLow  = findData.ftLastAccessTime.dwLowDateTime;
    built.lastAccessTimeHigh = findData.ftLastAccessTime.dwHighDateTime;
    built.lastWriteTimeLow   = findData.ftLastWriteTime.dwLowDateTime;
    built.lastWriteTimeHigh  = findData.ftLastWriteTime.dwHighDateTime;

    s_found = built;                                 // sub_677E20 - the member-wise copy
    return &s_found;
}

} // namespace

// Returns whether the named entry exists. Empty names and dot-prefixed entries return false on Windows.
bool Win32FileSystem::fileExists(const std::string& name)
{
    // Uses the shared lookup rules for empty and dot-prefixed names.
    return findOneEntry(name) != 0;
}

// Copies file information into out and returns true on success.
// [PLATFORM] Windows failure clears all fields except size; Unix failure leaves the record unchanged.
bool Win32FileSystem::getFileInfo(const std::string& name, FileInfo& out)
{
    const FileInfo* found = findOneEntry(name);
    if (found)
    {
        out = *found;                                // sub_677E20: every field the builder writes
        return true;
    }

    out.name = "";                                   // operator=("")
    out.isDirectory = false;                         // +28
    out.creationTimeLow    = 0;                      // +40
    out.creationTimeHigh   = 0;                      // +44
    out.lastWriteTimeLow   = 0;                      // +56
    out.lastWriteTimeHigh  = 0;                      // +60
    out.lastAccessTimeLow  = 0;                      // +48
    out.lastAccessTimeHigh = 0;                      // +52
    return false;
}

// Returns false if either lookup fails. Otherwise returns true because both lookups alias the same static record.
bool Win32FileSystem::compareFileTime(const std::string& a, const std::string& b)
{
    const FileInfo* first = findOneEntry(a);
    if (!first)
        return false;
    const FileInfo* second = findOneEntry(b);
    if (!second)
        return false;

    if (first->lastWriteTimeHigh != second->lastWriteTimeHigh)
        return first->lastWriteTimeHigh > second->lastWriteTimeHigh;
    return first->lastWriteTimeLow >= second->lastWriteTimeLow;
}

// Replaces path with its full ansi path and returns whether the API returned nonzero.
// The required size is not checked, so an oversized path can report success without a valid result.
bool Win32FileSystem::getFullFileName(std::string& path)
{
    CHAR buffer[512];

    if (!GetFullPathNameA(path.c_str(), 512, buffer, 0))
        return false;

    path = buffer;
    return true;
}

// PE 0x6767E0 builds each FileInfo from a by-value WIN32_FIND_DATAW plus relative prefix.
static FileInfo buildFindFileInfo(WIN32_FIND_DATAW data, const std::wstring& prefix)
{
    FileInfo info;
    std::wstring name = prefix + data.cFileName;
    for (std::size_t i = 0; i < name.size(); ++i)
        if (name[i] == L'\\') name[i] = L'/';
    info.name = narrowUtf8(name);
    info.isDirectory = (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    info.size = data.nFileSizeLow;
    info.creationTimeLow = data.ftCreationTime.dwLowDateTime;
    info.creationTimeHigh = data.ftCreationTime.dwHighDateTime;
    info.lastAccessTimeLow = data.ftLastAccessTime.dwLowDateTime;
    info.lastAccessTimeHigh = data.ftLastAccessTime.dwHighDateTime;
    info.lastWriteTimeLow = data.ftLastWriteTime.dwLowDateTime;
    info.lastWriteTimeHigh = data.ftLastWriteTime.dwHighDateTime;
    return info;
}

// Appends matching files, excluding all dot-prefixed names.
// Absolute-path detection uses the extended-drive prefix; UNC paths have no special handling.
static int findFilesRecursive(const std::wstring& path, bool recursive,
                              std::list<FileInfo>& out, const std::wstring& prefix)
{
    std::wstring pattern = normalizeWideDirectory(path);
    if (pattern.size() < 6 || pattern[5] != L':')
    {
        wchar_t current[MAX_PATH];
        GetCurrentDirectoryW(MAX_PATH, current); // Retail ignores failure and does not pre-zero.
        pattern = std::wstring(current) + L"\\" + pattern;
    }
    pattern += L"*.*";
    WIN32_FIND_DATAW data;
    HANDLE handle = FindFirstFileW(pattern.c_str(), &data);
    if (handle == INVALID_HANDLE_VALUE) return 0;
    do
    {
        if (data.cFileName[0] != L'.')
        {
            const FileInfo info = buildFindFileInfo(data, prefix);
            out.push_back(info);
            if (info.isDirectory && recursive)
            {
                const std::wstring child = std::wstring(data.cFileName) + L"\\";
                findFilesRecursive(path + child, true, out, prefix + child);
            }
        }
    }
    while (FindNextFileW(handle, &data));
    FindClose(handle);
    return static_cast<int>(out.size());
}

int Win32FileSystem::findFiles(const std::string& path, bool recursive, std::list<FileInfo>& out)
{
    // PE 0x677CA0, sort 0x678990 / comparison 0x6788C0. Includes the entries already in out.
    std::wstring wide = widenFileName(normalizeDirectory(path), true);
    for (std::size_t i = 0; i < wide.size(); ++i)
        if (wide[i] == L'/') wide[i] = L'\\';
    findFilesRecursive(wide, recursive, out, L"");
    out.sort([](const FileInfo& a, const FileInfo& b) {
        return _stricmp(a.name.c_str(), b.name.c_str()) < 0;
    });
    return static_cast<int>(out.size());
}

// cached nonempty cwd, trailing separator removal, UTF-8 and '/' normalization.
// The shipped 519-for-260 API size defect is retained. PE 0x897484 is string.size(), not a flag.
const std::string& Win32FileSystem::getRootSubstitution() const
{
    static std::string root;
    // PE 0x897484 is the cached string's size, not a separate once flag.
    if (root.empty())
    {
        WCHAR buffer[MAX_PATH];
        buffer[0] = 0;

        // Retail passes 519 for a 260-wchar buffer. Preserve the attested size defect.
        if (!GetCurrentDirectoryW(0x207, buffer))
        {
            Debug(DebugTypeError, "IO/File/Win32FileSystem.cpp", 1136, "")
                << "failed to get current working directory";
        }
        buffer[MAX_PATH - 1] = 0;

        std::wstring wide = stripFinalSeparator(buffer);
        for (std::wstring::size_type i = 0; i < wide.size(); ++i)
            if (wide[i] == L'\\') wide[i] = L'/';          // fixPathDelimiters(x, '/')

        root = narrowUtf8(wide);
    }

    return root;
}

// PE 0x676A20 -> 0x676200(true): Documents/Battlefield 2, not the root directory.
const std::string& Win32FileSystem::getHomeSubstitution() const
{
    std::wstring wide = stripFinalSeparator(applicationDataDirectory(true));
    for (std::size_t i = 0; i < wide.size(); ++i)
        if (wide[i] == L'\\') wide[i] = L'/';
    static std::string home;
    home = narrowUtf8(wide);
    return home;
}

IBase* Win32FileSystem::create(unsigned int iid, IBase* outer)
{
    if (outer != 0)
        return 0;

    Win32FileSystem* self = new Win32FileSystem();

    void* itf = self->queryInterface(iid);
    if (itf == 0)
        self->release();

    return static_cast<IBase*>(itf);
}

} // namespace io

// Returns the cached application-data dump directory. [INFERRED] Helper name.
// [PLATFORM] Windows startup uses this path before engine initialization.
const wchar_t* getCrashDumpDirectory()
{
    static const std::wstring directory = io::applicationDataDirectory(false) + L"\\dmp\\";

    // Client 0x6C5180 / dedicated 0x675880: attempt every backslash-delimited prefix,
    // including the drive prefix, on every call. Ignore CreateDirectoryW's return value.
    for (int i = 0, length = static_cast<int>(directory.length()); i < length; ++i)
        if (directory[i] == L'\\')
            CreateDirectoryW(directory.substr(0, i).c_str(), 0);
    return directory.c_str();
}

} // namespace hfe
} // namespace dice
