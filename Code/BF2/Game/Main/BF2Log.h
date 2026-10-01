// Engine logging and assertion front end. BF2Engine creates the log before other
// services and closes it last. File writes are inert while their handles are closed.
// setExitInsteadOfErrorWindow deliberately ignores its argument and clears the flag.
// [UNRESOLVED] m_string0C remains unnamed; file-only accessors have no bodies.
// [CHOSEN] Do not supply accessor behavior without evidence.

#ifndef GAME_MAIN_BF2LOG_H          // [INFERRED] guard style
#define GAME_MAIN_BF2LOG_H

#include <string>

#include "../../Core/Debug.h"       // DebugType - attested enum, see Core/Debug.h

namespace dice {
namespace hfe {

namespace io { class File; }        // +0x18/+0x1C - IO/File/File.h

class BF2Log
{
public:
    BF2Log();
    virtual ~BF2Log();                                          // Releases log resources.

    // vtable +0x08, and the only virtual besides the destructor. BF2Engine::initEngine tests the
    // result and aborts startup on false, so the bool return is attested by use.
    virtual bool initDebugCallback();

    // A stub in this build - 18 bytes, `mov eax,1` and a 0x178 stack frame it never touches. The
    // reserved frame is the residue of a body compiled out for the dedicated server, the same
    // signature initLocalization shows. Its caller still tests the result.
    bool initLogFiles(const std::string& baseName);

    // The four flag pairs. Offsets read from the instructions, not inferred.
    bool getDebugCallbackDisabled() const;                      // +0x04
    void setDebugCallbackDisabled(bool);                        //         see .cpp
    bool getDoNotDisplayErrorWindow() const;                    // +0x05
    void setDoNotDisplayErrorWindow(bool);
    bool getExitInsteadOfErrorWindow() const;                   // +0x06
    void setExitInsteadOfErrorWindow(bool);                     //         ignores the
                                                                //        argument - see above
    // THE +0x07 pair HAS NO symbol IN THE ELF - no out-of-line body was ever emitted (inlined
    // everywhere, or never called). Declarations kept for the class surface; bodies would be
    // [CHOSEN], so they stay unwritten. Referencing them is a link error by design.
    bool getDebugOutputOnlyToFile() const;                      // +0x07  (no ELF symbol)
    void setDebugOutputOnlyToFile(bool);                        //        (no ELF symbol)

    // 3,700 bytes - the log rotation path, and it rotates both logs despite the name.
    // BF2Engine::startGame calls it with ("", "server") and ("", "client") to retag them once the
    // run's role is known. arg1 is a prefix in front of "Logs/", not a segment after it - the
    // note this header used to carry had the order backwards. See the body.
    void renameAndReOpenPureLog(const std::string&, const std::string&);

    // Guarded raw-text appends to the two Files; no-ops while the File* is null or closed.
    void addPureLogEntry(const std::string&);                   // > +0x1C
    void addLogEntry(DebugType, const std::string&);            // > +0x18
                                                                // Flushes logs, then terminates on fatal errors. Error type 6 cannot be ignored;
                                                                // the ignore-asserts setting applies only to the other assertion types.
    void displayErrorWindow(const std::string&, DebugType);

    // Flushes both log Files through IStream vtable +0x2C - the call that resolved that slot
    // as flush() (see IO/Stream/IStream.h).
    void flushWrite();

    void setLogInsteadOfAssert(bool, const std::string&);       //   ⛔ unwritten

    // Static DebugCallback adapter; the trailing user-data pointer selects the BF2Log instance.
    // The basename file argument is accepted but unused.
    static void debugCallback(DebugType type,
                              const std::string& module, const std::string& file,
                              int line,
                              const std::string& func, const std::string& text,
                              const std::string& fullFileName,
                              void* data);

private:
    bool        m_debugCallbackDisabled;    // +0x04
    bool        m_doNotDisplayErrorWindow;  // +0x05
    bool        m_exitInsteadOfErrorWindow; // +0x06
    bool        m_debugOutputOnlyToFile;    // +0x07
    std::string m_logFileName;              // +0x08
    std::string m_string0C;                 // +0x0C  [UNRESOLVED] name - still the only one
    // [INFERRED names] Log basenames exclude Logs/ and .log; rotation appends the role tag.
    std::string m_logName;                  // +0x10  goes with m_logFile     (+0x18)
    std::string m_pureLogName;              // +0x14  goes with m_pureLogFile (+0x1C)
    io::File*   m_logFile;                  // +0x18  [INFERRED] name - addLogEntry's target
    io::File*   m_pureLogFile;              // +0x1C  [INFERRED] name - addPureLogEntry's target
};

} // namespace hfe
} // namespace dice

#endif // GAME_MAIN_BF2LOG_H
