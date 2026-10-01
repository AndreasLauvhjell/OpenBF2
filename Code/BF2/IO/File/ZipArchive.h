// Minizip-backed archive with case-insensitive entry lookup. Header path [INFERRED].
// Mounting indexes the central directory once; reads reposition pooled handles to stored entries.
// The owning handle list retains storage while the free list holds reusable pointers.
// Construction addRefs the archive handler; destruction releases it. Keep native container layout.

#ifndef IO_FILE_ZIPARCHIVE_H        // [INFERRED] guard style
#define IO_FILE_ZIPARCHIVE_H

#include <list>
#include <map>
#include <string>

#include "IFileSystem.h"            // IArchive, IArchiveHandler, IFileSystem enums, FileInfo
#include "unzip.h"                  // minizip 1.01e - External/zlib-1.2.3/contrib/minizip (the
                                    // include dirs are on the compile line; see build.cmd)

namespace dice {
namespace hfe {
namespace io {

class ZipReadStream;

class ZipArchive : public IArchive
{
public:
    // Minizip entry metadata and seek position, keyed by lowercase archive path.
    struct Entry
    {
        unz_file_info info;         // +0x00 .. +0x4F
        unz_file_pos  pos;          // +0x50 - the teleport target for unzGoToFilePos
    };

    // FileHandle - one pooled minizip handle: 8 bytes, {unzFile, 64 KB buffer}. init() unzOpens the
    // archive by name and allocates the buffer; the destructor unzCloses and frees. The pool:
    // openFileHandle takes from m_freeHandles (constructing+initing a new handle into m_handles
    // when the free list is empty), closeFileHandle unzCloseCurrentFile's and pushes back. Handles
    // are never destroyed before the archive itself is.
    struct FileHandle
    {
        FileHandle();                               // both members zero
        ~FileHandle();                              // unzClose + delete[] buffer

        bool init(const std::string& archiveName);  // unzOpen + new char[0x10000]

        unzFile fileHandle;         // +0x00
        char*   buffer;             // +0x04 - ZipReadStream's staging area, 0x10000 bytes
    };

    explicit ZipArchive(ZipArchiveHandler& handler);
    virtual ~ZipArchive();

    // IBase
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();                                // D0 via vptr+0x54
    virtual void* queryInterface(unsigned int iid) const;   // IBase/IFileSystem/IArchive

    // Opens an entry for reading. Entries up to 65535 bytes inflate into a MemStream; larger entries stream.
    // Returns null for unsupported access or inflation failure. Requires the MemStream factory.
    virtual IStream* open(const std::string& name, FileMode mode, FileAccess access,
                          unsigned int flags);

    // +0x1C, 0x848AB10: checkAndFixPathToFitZip + findEntry != 0.
    virtual bool fileExists(const std::string& name);

    virtual bool deleteFile(const std::string&);            // false; archives are
    virtual bool moveFile(const std::string&, const std::string&);          // false
    virtual bool copyFile(const std::string&, const std::string&, bool);    // false

    // Reads the entire entry into buffer, returning false unless it fits and is fully read.
    // The pooled handle is unchecked and is not returned after a seek-position failure.
    virtual bool readFile(const std::string& name, void* buffer, unsigned int size,
                          unsigned int& bytesRead);

    // Writes no data. Existing behavior returns true when the entry exists and its size fits size; append is ignored.
    virtual bool writeFile(const std::string& name, const void* buffer, unsigned int size,
                           bool append);

    // Appends matching entries and returns their count. Nonrecursive names are basenames; recursive names are relative to pattern.
    // Timestamp fields are left uninitialized.
    virtual int findFiles(const std::string& pattern, bool recursive, std::list<FileInfo>& out);

    // Fills the original name, uncompressed size and zero file times. Leaves out.isDirectory unchanged.
    virtual bool getFileInfo(const std::string& name, FileInfo& out);

    // Unsupported archive operations emit a fatal diagnostic before their nominal false return.
    virtual bool compareFileTime(const std::string& a, const std::string& b);
    virtual bool getFullFileName(std::string& path);

    virtual void update();                                  // empty

    virtual const std::string& getHomeSubstitution() const; // getEmptyString()
    virtual const std::string& getRootSubstitution() const; // getEmptyString()

    // IArchive
    virtual IArchiveHandler*   getHandler();                // +0x58
    virtual const std::string& getFilename() const;         // +0x5C
    virtual void               setPath(const std::string&); // +0x60
    virtual void               setContentCheck(bool);       // +0x64
    virtual bool               getContentCheck();           // +0x68

    // Opens the archive and reads its directory. Entry-table failure is fatal; baseSystem and flag are ignored.
    bool open(const std::string& file, IFileSystem* baseSystem, bool flag);

    bool readEntries(FileHandle* handle);

    FileHandle* openFileHandle();                           // the pool
    FileHandle* closeFileHandle(FileHandle* handle);

    // lowercase, force '/', then require the mount path to occur somewhere in
    // the string (std::string::find, not a prefix compare) - and then strip m_path.size() chars
    // OFF the front regardless of where the match was. The find/strip mismatch is the binary's.
    bool checkAndFixPathToFitZip(std::string& path);

    Entry* findEntry(const std::string& fixedPath);         // map find

private:
    int                             m_refCount;     // +0x04
    ZipArchiveHandler*              m_handler;      // +0x08
    std::string                     m_filename;     // +0x0C
    std::string                     m_path;         // +0x10
    std::string                     m_string14;     // +0x14  [UNRESOLVED] - dead, see layout block
    std::map<std::string, Entry>    m_entries;      // +0x18
    std::list<FileHandle*>          m_freeHandles;  // +0x20
    std::list<FileHandle>           m_handles;      // +0x24
    bool                            m_contentCheck; // +0x2C
};

// Read-only stream over a pooled ZIP handle. Keeps a reference to the archive.
// Forward skips decompress data; backward absolute seeks reopen the entry. Unsupported seeks and writes report errors.
// clone starts a fresh stream at position zero. Destruction is nonvirtual.
class ZipReadStream : public INonlinearStream
{
public:
    ZipReadStream(ZipArchive& archive, ZipArchive::Entry& entry);
    ZipReadStream(const ZipReadStream& other);
    ~ZipReadStream();

    // IBase
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();                                // D1 + operator delete
    virtual void* queryInterface(unsigned int iid) const;   // IBase/IStream/INonlinear

    // IStream / INonlinearStream
    virtual unsigned int read(void* dest, unsigned int n);
    virtual unsigned int write(const void* src, unsigned int n);    // fatal stub
    virtual int  skip(unsigned int n);                      // reads n bytes into a temp
    virtual int  getAvailableBytes();                       // uncompressed - position
    virtual IStream* clone();                               // fresh stream, position 0
    virtual bool canRead();                                 // true
    virtual bool canWrite();                                // false
    virtual int  seek(SeekOrigin origin, int offset);
    virtual int  getPosition();
    virtual int  getSize();                                 // entry's uncompressed size

    // Own slot, +0x3C: a function-local static "Name not stored per zipStream".
    virtual const std::string& getName();

private:
    int init();                                             // borrow handle + teleport

    int                     m_refCount;     // +0x04
    ZipArchive*             m_archive;      // +0x08
    ZipArchive::Entry*      m_entry;        // +0x0C
    ZipArchive::FileHandle* m_handle;       // +0x10
    unsigned int            m_position;     // +0x14 - logical position in the uncompressed entry
    unsigned int            m_buffered;     // +0x18 - bytes staged in the handle's buffer
    unsigned int            m_bufferPos;    // +0x1C - consumed prefix of the staged bytes
};

// Archive handler that creates a fresh ZipArchive for each open.
class ZipArchiveHandler : public IArchiveHandler
{
public:
    // [CHOSEN] form only - see above; nothing observable differs.
    ZipArchiveHandler() : m_refCount(1) {}

    virtual void  addRef();                                    // +0x00
    virtual int   getRef();                                    // +0x04
    virtual int   release();                                   // +0x08
    virtual void* queryInterface(unsigned int iid) const;      // +0x0C

    // Creates and opens an archive, returning an owned reference or null on failure.
    virtual IArchive*   open(const std::string& file, IFileSystem* baseSystem,
                             bool flag);
    virtual const std::string& getExtensions() const;          // +0x14  0x848BD50  88 B - "ZIP PK3"
    virtual bool        canWrite() const;                      // +0x18  0x848BDB0   7 B - false

    static IBase* create(unsigned int iid, IBase* outer);

private:
    /* +0x00 */ // vptr
    /* +0x04 */ int m_refCount;
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_FILE_ZIPARCHIVE_H
