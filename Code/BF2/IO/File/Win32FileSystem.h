// Windows filesystem with a separate directory-change notification interface.

#ifndef IO_FILE_WIN32FILESYSTEM_H     // [INFERRED] guard style
#define IO_FILE_WIN32FILESYSTEM_H

#include <list>
#include <map>
#include <deque>
#include <vector>
#include <string>
#include "../../Core/SmartPtr.h"

#include "IFileSystem.h"
#include "FileChangeManager.h"   // IDirectoryChangeNotifier - the second base, see below

namespace dice {
namespace hfe {
namespace io {

class Win32FileStream;

// Windows filesystem with directory-change notifications through its second base interface.
class Win32FileSystem : public IFileSystem, public IDirectoryChangeNotifier
{
public:
    Win32FileSystem();
    ~Win32FileSystem(); // PE release 0x679060 directly calls 0x677D40; no destructor vtable slot.

    // IBase quartet
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    // Opens a UTF-8 path as a Win32 stream with access-dependent text conversion.
    // Uses a BufferedStream wrapper when the low two flag bits are clear.
    virtual IStream* open(const std::string& name, FileMode, FileAccess, unsigned int);
    virtual bool   fileExists(const std::string& name);
    virtual bool   deleteFile(const std::string& name);
    virtual bool   moveFile(const std::string& from, const std::string& to);
    virtual bool   copyFile(const std::string& from, const std::string& to,
                            bool failIfExists);
    virtual bool   readFile(const std::string& name, void* buffer, unsigned int size,
                            unsigned int& bytesRead);
    virtual bool   writeFile(const std::string& name, const void* buffer,
                             unsigned int size, bool overwrite);                    // CREATE_NEW/CREATE_ALWAYS
    // PE 0x677CA0 / walker 0x678520: append real FileInfo records, skip all dot-prefixed names,
    // recurse with a relative prefix, then sort the entire output list case-insensitively.
    virtual int    findFiles(const std::string& path, bool recursive,
                             std::list<FileInfo>& out);
    virtual bool   getFileInfo(const std::string& name, FileInfo& out);
    virtual bool   compareFileTime(const std::string& a, const std::string& b);
    virtual bool   getFullFileName(std::string& path);
    virtual void   update();
    virtual const std::string& getHomeSubstitution() const;
    virtual const std::string& getRootSubstitution() const;

    // Directory notifications retain listeners and store the recursive flag and Windows notification mask.
    virtual bool addListener(IDirectoryChangeListener* listener,
                             const std::string& path,
                             bool recursive, unsigned int flags);      // 2nd vt +0x10, PE 0x677B00
    virtual void removeListener(IDirectoryChangeListener* listener);   // 2nd vt +0x14, PE 0x677A90

    // non-virtual
    // CID_Win32FileSystem is 3230030075 (Core/Ids.h), adjacent to CID_UnixFileSystem. The factory
    // follows the same canonical shape read out of UnixFileSystem::create.
    static IBase* create(unsigned int iid, IBase* outer);

private:
    // PE ctor 0x677DB0 / dtor 0x677D40. Two genuine interface bases precede the count.
    // Offsets below refer to retail MSVC; current STL containers use compiler-owned layout.
    /* +0x00 */ // IFileSystem vptr (primary)                  -> PE 0x7F6948
    /* +0x04 */ // IDirectoryChangeNotifier vptr               -> PE 0x7F692C
    /* +0x08 */ int m_refCount;      // ctor sets 1
    /* +0x0C */ int m_unknown0C;     // ctor sets 0
    unsigned char m_unwritten10[12]; // PE +0x10..0x1B: ctor leaves these bytes untouched.
    // PE 0x678EB0 destroys a wide key and a vector of 96-byte records (wide name, FileInfo).
    // Names/record grouping are [INFERRED]; source STL layout is deliberately compiler-owned.
    typedef std::pair<std::wstring, FileInfo> CachedEntry;
    std::map<std::wstring, std::vector<CachedEntry> > m_directoryCache; // retail +0x1C
    // [UNRESOLVED] four-byte deque payload type. The folded destructor proves storage only,
    // not IObject ownership. No recovered operation populates this cache queue yet.
    std::deque<unsigned int> m_cacheQueue;                            // retail +0x28
    struct Notification
    {
        SmartPtr<IDirectoryChangeListener> listener;
        std::string path;
        bool recursive;
        unsigned int flags;
        void* handle;
        // PE 0x677B00 temporary initializes only the pointer/string; other fields are assigned
        // after insertion. Destruction is path, then listener release; it does not close handle.
        Notification() {}
    };
    std::list<Notification> m_notifications;                         // retail +0x3C
};


} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_FILE_WIN32FILESYSTEM_H
