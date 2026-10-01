// Non-polymorphic application facade owning one BF2Engine pointer.
// [INFERRED] source filename.

#ifndef BF2_H                       // [INFERRED] guard style not recoverable from a binary
#define BF2_H

#include <string>

namespace dice {
namespace hfe {

class BF2Engine;

class BF2
{
public:
    BF2();
    ~BF2();

    // Parses the command line into the settings repository, creates and initialises the engine,
    // and starts the game immediately if any launch option demands it.
    bool init(const std::string& commandLine);

    // Shuts the engine down and releases it.
    void shutdown();

    // One iteration of the outer loop. Returns false to leave main()'s while-loop.
    bool run();

    void startDemo(const std::string& demoName);

    // [PLATFORM] The Win32 message pump - from both PEs (client 0x4013F0,
    //            383 B, adds the focus-loss mute; dedicated 0x4012F0, 225 B); an empty stub in
    //            the Linux build (0x8052aa0, 7 bytes).
    void handleInGameWinMessages();

private:
    BF2Engine* m_engine;            // offset 0 - the only member
};

#if defined(_WIN32) && !DEDICATED
// Keyboard rings filled by window messages and consumed by menu/console text input.
// Toggling either UI flushes its ring. [INFERRED] Global names.
extern char consoleCharBuffer[256];
extern char menuKeyBuffer[256];
extern int  consoleCharCount, consoleCharRead, consoleCharWrite;
extern int  menuKeyCount, menuKeyRead, menuKeyWrite;
#endif

} // namespace hfe
} // namespace dice

#endif // BF2_H
