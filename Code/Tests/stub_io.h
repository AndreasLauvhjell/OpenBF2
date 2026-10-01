// Test scaffolding: provisional I/O declarations for isolated syntax checks.

#ifndef BF2_STUB_IO_H
#define BF2_STUB_IO_H

#include <string>
#include "shadow/IO/SettingsManager.h"

namespace dice {
namespace hfe {
namespace io {

// io::Console::ignoredString_ - BF2Engine.cpp: "every call site passes
// io::Console::ignoredString_ (_ZN4dice3hfe2io7Console14ignoredString_E,
// ELF 0x8AC3C80)". It is bound to a NON-const std::string& parameter, so it
// cannot be const here.
class Console
{
public:
    static std::string ignoredString_;
};

// io::mainConsole (ELF 0x898CBE4). Slot numbers are the recovered comments'.
class OldConsole
{
public:
    virtual ~OldConsole();

    virtual void setExitCallBack(void (*)(void*), void*);          // used by init()
    virtual void loadCommandHistory(const std::string&);           // vptr+0xAC
    virtual void saveCommandHistory(const std::string&);           // vptr+0xB0
    virtual void setHandleCommandHook(bool (*)(const char*, int)); // vptr+0xBC
    virtual void setUseRelativePaths(bool);                        // vptr+0x78

    // vptr+0x2C. BF2Engine.cpp: "runFullAccess takes the script name, 8
    // optional argument strings (all empty here) and a trailing NON-const
    // std::string&".
    virtual bool runFullAccess(const std::string& script,
                               const std::string&, const std::string&,
                               const std::string&, const std::string&,
                               const std::string&, const std::string&,
                               const std::string&, const std::string&,
                               std::string& out);
};

extern OldConsole* mainConsole;

class IFileSystem
{
public:
    virtual ~IFileSystem();
};

class IArchiveHandler
{
public:
    virtual ~IArchiveHandler();
};

// io::fileManager (ELF 0x8AC8C80).
class FileManager
{
public:
    virtual ~FileManager();
    virtual void addPath(const std::string&);
    virtual void permitMountArchives(bool);          // FileManager vptr+0x78
    virtual void setBaseSystem(IFileSystem*);
    virtual void addArchiveHandler(IArchiveHandler*);
};

extern FileManager* fileManager;

// io::fileChangeManager (ELF 0x8AC8C84); setEnable is vptr+0x1C.
class FileChangeManager
{
public:
    virtual ~FileChangeManager();
    virtual void setEnable(bool);
};

extern FileChangeManager* fileChangeManager;

// io::networkManager (ELF 0x8AC3D1C), operator new(0x84).
class NetworkManager
{
public:
    NetworkManager();
    virtual ~NetworkManager();
};

extern NetworkManager* networkManager;

// BF2EngineSetup::initNetwork does `return io::setSocketManager() != 0;`.
void* setSocketManager();

// Class/interface ids used as SmartItf template arguments and create() inputs.
// Typed `unsigned int` to match the IID_/CID_ declarations the recovered
// BF2Engine.cpp makes for its own ids.
extern unsigned int IID_IFileSystem;
extern unsigned int IID_IArchiveHandler;
extern unsigned int CID_UnixFileSystem;
extern unsigned int CID_ZipArchiveHandler;

} // namespace io
} // namespace hfe
} // namespace dice

#endif // BF2_STUB_IO_H
