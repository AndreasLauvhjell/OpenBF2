// Typed host file interfaces. Native mode uses engine declarations; retail mode uses
// VC7.1 string carriers with the same virtual order. Native strings cannot cross that boundary.

#ifndef RENDDX9ABI_HOSTFILEABI_H
#define RENDDX9ABI_HOSTFILEABI_H

#include "HfeAbi.h"

#if BF2_PLUGIN_ABI_NATIVE

#include "../BF2/IO/File/IFileSystem.h"

namespace renddx9abi {
typedef ::dice::hfe::io::IStream          HostStream;
typedef ::dice::hfe::io::INonlinearStream HostNonlinearStream;
typedef ::dice::hfe::io::IFileManager     HostFileManager;
typedef ::dice::hfe::io::FileMode         HostFileMode;
typedef ::dice::hfe::io::FileAccess       HostFileAccess;
typedef ::dice::hfe::io::SeekOrigin       HostSeekOrigin;
}

#else

namespace renddx9abi {
typedef int HostFileMode;
typedef int HostFileAccess;
typedef int HostSeekOrigin;

class HostStream : public ::dice::hfe::IBase
{
public:
    virtual unsigned int read(void*, unsigned int) = 0;                 // +0x10
    virtual unsigned int write(const void*, unsigned int) = 0;          // +0x14
    virtual int          skip(unsigned int) = 0;                         // +0x18
    virtual int          getAvailableBytes() = 0;                        // +0x1C
    virtual HostStream*  clone() = 0;                                    // +0x20
    virtual bool         canRead() = 0;                                  // +0x24
    virtual bool         canWrite() = 0;                                 // +0x28
    virtual void         flushWrite() = 0;                               // +0x2C

protected:
    ~HostStream() {}
};

class HostNonlinearStream : public HostStream
{
public:
    virtual int seek(HostSeekOrigin origin, int offset) = 0;             // +0x30
    virtual int getPosition() = 0;                                       // +0x34
    virtual int getSize() = 0;                                           // +0x38

protected:
    ~HostNonlinearStream() {}
};

struct HostFileInfo;
struct HostFileInfoList;
class HostArchive;
class HostArchiveHandler;
struct HostArchiveVector;

class HostFileSystem : public ::dice::hfe::IBase
{
public:
    virtual void init() = 0;                                             // +0x10
    virtual bool isLastFileOpenedFromArchive() const = 0;                // +0x14
    virtual HostStream* open(const HostString&, HostFileMode, HostFileAccess,
                             unsigned int) = 0;                           // +0x18
    virtual bool fileExists(const HostString&) = 0;                      // +0x1C
    virtual bool deleteFile(const HostString&) = 0;                      // +0x20
    virtual bool moveFile(const HostString&, const HostString&) = 0;     // +0x24
    virtual bool copyFile(const HostString&, const HostString&, bool) = 0; // +0x28
    virtual bool readFile(const HostString&, void*,
                          unsigned int, unsigned int&) = 0;               // +0x2C
    virtual bool writeFile(const HostString&, const void*,
                           unsigned int, bool) = 0;                       // +0x30
    virtual int  findFiles(const HostString&, bool,
                           HostFileInfoList&) = 0;                        // +0x34
    virtual bool getFileInfo(const HostString&, HostFileInfo&) = 0;      // +0x38
    virtual bool compareFileTime(const HostString&, const HostString&) = 0; // +0x3C
    virtual bool getFullFileName(HostString&) = 0;                       // +0x40
    virtual void update() = 0;                                           // +0x44
    virtual const HostString& getHomeSubstitution() const = 0;           // +0x48
    virtual const HostString& getRootSubstitution() const = 0;           // +0x4C

protected:
    ~HostFileSystem() {}
};

// VC7.1 std::vector<std::string> by-value carrier. Only its size/layout are relevant here; the
// renderer never calls setModPaths or constructs one.
struct HostStringVector
{
    void*                       allocator;
    HostString* first;
    HostString* last;
    HostString* end;
};

class HostFileManager : public HostFileSystem
{
public:
    virtual void            setBaseSystem(HostFileSystem*) = 0;           // +0x50
    virtual HostFileSystem* getBaseSystem() = 0;                          // +0x54
    virtual HostStream*     openwin32(const HostString&,
                                   HostFileMode,
                                   HostFileAccess,
                                   unsigned int) = 0;                     // +0x58
    virtual void         addArchiveHandler(HostArchiveHandler*) = 0;      // +0x5C
    virtual void         removeArchiveHandler(HostArchiveHandler*) = 0;   // +0x60
    virtual bool         addPathOverload(const HostString&,
                                         const HostString&) = 0;          // +0x64
    virtual bool         removePathOverload(const HostString&,
                                            const HostString&) = 0;       // +0x68
    virtual bool         addMergeDirectory(const HostString&,
                                           const HostString&) = 0;        // +0x6C
    virtual bool         removeMergeDirectory(const HostString&) = 0;    // +0x70
    virtual HostArchive* openArchive(const HostString&, int, bool) = 0;   // +0x74
    virtual void         permitMountArchives(bool) = 0;                   // +0x78
    virtual HostArchive* mountArchive(const HostString&, const HostString&,
                                      bool) = 0;                           // +0x7C
    virtual bool         unmountArchive(const HostString&) = 0;          // +0x80
    virtual void         unmountAllArchives() = 0;                         // +0x84
    virtual void         getAllArchives(HostArchiveVector&) = 0;          // +0x88
    virtual bool         hasMountedArchives() const = 0;                   // +0x8C
    virtual HostString   stripModPath(const HostString&) = 0;             // +0x90
    virtual void*        getPathList() const = 0;                          // +0x94
    virtual void         addPath(const HostString&) = 0;                  // +0x98
    virtual void         removePath(const HostString&) = 0;               // +0x9C
    virtual void         removeAllPaths() = 0;                             // +0xA0
    virtual void         getWritePath(const HostString&, HostString&) = 0; // +0xA4
    virtual int          getFileResolveMode() const = 0;                   // +0xA8
    virtual void         setFileResolveMode(int) = 0;                      // +0xAC
    virtual HostString   fileExistsEx(const HostString&, bool) = 0;       // +0xB0
    virtual HostString   fileExistsEx2(const HostString&) = 0;            // +0xB4
    virtual ~HostFileManager() {}                                           // +0xB8
    virtual void         setModPaths(HostStringVector) = 0;                // +0xBC
};

} // namespace renddx9abi

#endif // BF2_PLUGIN_ABI_NATIVE

#endif // RENDDX9ABI_HOSTFILEABI_H
