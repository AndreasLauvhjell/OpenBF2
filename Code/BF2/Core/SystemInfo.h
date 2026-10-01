// Platform information and formatted time, exposed through the systemInfo() singleton.
// Header path [INFERRED]. ISystemInfo has a pure virtual destructor with an out-of-line definition;
// keep it first in the virtual declaration order. Its constructor is also out of line.
// PosixSystemInfo owns a string, so its native layout depends on the compiler's STL ABI.

#ifndef CORE_SYSTEMINFO_H           // [INFERRED] guard style
#define CORE_SYSTEMINFO_H

#include <string>

namespace dice {
namespace hfe {

// The abstract interface. Eight vtable slots; declaration order below is the slot order.
class ISystemInfo
{
public:
    ISystemInfo();

    // +0x00 / +0x04. pure, and defined out of line - see the header note. D2 0x83C4690,

    virtual ~ISystemInfo() = 0;

    // +0x08. Returns a reference - the implementation caches, and every call site treats the
    // returned eax as a `std::string*` (e.g. getServerName 0x8149DDF, makeFilename 0x83CF92A).
    virtual const std::string& getComputerName() const = 0;

    // +0x0C. Returns by value - the call is an sret call everywhere (hidden return slot in
    // [esp], `this` in [esp+4]; the Posix body ends `retn 4`). Nine of the nineteen systemInfo()
    // call sites are this one.
    virtual std::string getDateTimeString() const = 0;

    // Returns the major page-fault count. [CHOSEN] Unsigned return type.
    virtual unsigned int getNumberOfPageFaults() const = 0;

    // +0x14. [CHOSEN] return type - both call sites (LoadStat::setNextLoadPart 0x83C48AF and
    // BF2Engine::handleEvent 0x8139CC1) discard the result, and the Posix body is `xor eax,eax`.
    virtual unsigned int getMemoryUsage() const = 0;

    // Writes memory totals through the two references. [CHOSEN] Parameter names.
    virtual void getMemoryInfo(unsigned int& totalPhysical,
                               unsigned int& availablePhysical) const = 0;

    // Reports whether a debugger is installed. [CHOSEN] Boolean return type.
    virtual bool getIsDebuggerInstalled() const = 0;
};

// POSIX system information with explicitly marked Windows adaptations.
class PosixSystemInfo : public ISystemInfo
{
public:
    PosixSystemInfo();
    virtual ~PosixSystemInfo();


    virtual const std::string& getComputerName() const;      // +0x08
    virtual std::string        getDateTimeString() const;    // +0x0C
    virtual unsigned int       getNumberOfPageFaults() const;// +0x10
    virtual unsigned int       getMemoryUsage() const;       // +0x14
    virtual void               getMemoryInfo(unsigned int& totalPhysical,
                                             unsigned int& availablePhysical) const;
                                                             // +0x18
    virtual bool               getIsDebuggerInstalled() const;// +0x1C

private:
    // [CHOSEN] Field name. Mutable cache allows the const hostname query to initialize it lazily.
    mutable std::string m_computerName;
};

// Returns the process-local system-information instance.
ISystemInfo* systemInfo();

} // namespace hfe
} // namespace dice

#endif // CORE_SYSTEMINFO_H
