// Engine wrapper around OldConsole and the console-object registry. Owns console
// visibility and autocomplete matches. Dedicated-server stdin is handled by the
// status monitor, not this keyboard ring buffer. [INFERRED] Header placement.

#ifndef GAME_MAIN_BF2CONSOLE_H      // [INFERRED] guard style
#define GAME_MAIN_BF2CONSOLE_H

#include <list>
#include <string>

namespace dice {
namespace hfe {

class ConsoleObject;

// Shared character ring filled by client WM_CHAR, consumed by console/text fields,
// and flushed on console toggle. [INFERRED] names; [UNRESOLVED] original owning TU.
extern char consoleCharBuffer[256];           // pending characters, written through `char`
extern int  consoleCharCount;                 // clamped at 0 by the drain
extern int  consoleCharRead;                  // wraps to 0 only when > 255
extern int  consoleCharWrite;                 // advanced by the client's WM_CHAR writer only

class BF2Console
{
public:
    BF2Console();
    ~BF2Console();                                         //  (D1) / 0x812DAE0 (D2) - NOT virtual:
                                                           //   50 bytes each, no vtable symbol. Empty in
                                                           //   source; the bodies are m_commands' implicit
                                                           //   destruction. D1 called by shutdownEngine.

    void outputToConsole(const std::string&);              // > mainConsole->output, vptr+0x14
    const std::string& getWorkingFile();                   //   [CHOSEN] return - passes through
                                                           // [CHOSEN] void return types; original calls do not establish a returned value.
    void runConsoleCommand(const std::string&);            //   cmd+"\n" -> executeLine under access 1
    void runConsoleFile(const std::string&);               //   [+".con" if absent] -> run under access 1

    void registerConsoleObjects(std::list<ConsoleObject*>&);  // > mainConsoleObjects, `this` unused

    // returns both return eax = this+4 (0x812E361 / 0x812E481) - a reference
    // to the member match list, refilled on every call. [CHOSEN] non-const reference (vs pointer).
    // Both push_front their matches, so the result is in reverse registry order - preserved.
    std::list<std::string>& getConsoleCommands(const std::string&);
    std::list<std::string>& getVoidConsoleCommands(const std::string&);

    // Called every frame from BF2Engine::mainLoop (0x81375A3). Drains the consoleChar* ring (above;
    // the client's WM_CHAR writer 0x401000 fills it) into mainConsole->updateAsciiKey while
    // m_consoleActive (client body 0x446B20); swallows 0xA7 (the console-toggle key
    // character); stops on NUL or empty. void [CHOSEN], caller ignores eax.
    void updateConsole();
    // [INFERRED] accessor: the client's window message handler (Game/Main/BF2.cpp,
    // ) and BF2Engine::filterGameInput read the +0x00 flag through the menu's console pointer -
    // a public member or an inlined getter in dice's header.
    bool getConsoleActive() const { return m_consoleActive; }
    void setConsoleActive(bool);                           //   flag store, then forward to
                                                           //   the inherited Console slot, vptr+0xA4

private:
    // Offset annotations use the ELF layout; native std::list size may differ.
    /* +0x00 */ bool                   m_consoleActive;    // [INFERRED name] ctor zeroes it; toggled via
                                                           //   setConsoleActive; read by updateConsole and
                                                           //   (externally) BF2Engine::filterGameInput
    /* +0x04 */ std::list<std::string> m_commands;         // [INFERRED name] autocomplete matches; returned
                                                           //   by reference by the two getters above
};

} // namespace hfe
} // namespace dice

#endif // GAME_MAIN_BF2CONSOLE_H
