// Queued engine-console commands and script execution. [INFERRED] File placement.

#include "BF2Console.h"

#include "../../IO/Console/Console.h"    // io::OldConsole, io::mainConsole, io::Console::ignoredString_
#include "../../Core/StringUtil.h"       // compareIgnoreCase(char const*, char const*), 0x83D0510

#include <cstring>                       // strlen

#include "../../Core/ConsoleObjects.h"    // the real ConsoleObject + ConsoleObjects, recovered
namespace dice {
namespace hfe {

class IBase;
static inline ConsoleObjects* consoleObjects() { return mainConsoleObjects; }

// Shared console-character ring. Client WM_CHAR fills it; updateConsole drains
// it and console toggles flush it. Dedicated images have no producer.
char consoleCharBuffer[256];                                 // ELF ds:0x8AC4000 / client ds:0x9FFA90
int  consoleCharCount = 0;                                   // ELF ds:0x8AC4100 / client ds:0x9FFC90
int  consoleCharRead  = 0;                                   // ELF ds:0x8AC4104 / client ds:0x9FFC94
int  consoleCharWrite = 0;                                   // ELF ds:0x8AC4108 / client ds:0x9FFC98

//  (C1), 45 bytes. The variant BF2Engine::init actually calls. Three stores: the flag byte
// zeroed at 0x812DABC, then m_commands' 12-byte header allocated and self-linked (0x812DABF-0x812DAD3).
BF2Console::BF2Console()
    : m_consoleActive(false)                                 //  mov byte ptr [this], 0
{
    // m_commands default-constructs itself; the header allocation is the pre-3.4 list shape above.
}

//  (D1) / 0x812DAE0 (D2), 50 bytes each. Empty in source - the emitted bodies are exactly
// m_commands' implicit destruction in the pre-3.4 list shape (__clear at 0x812DB32, then
// deallocate(header, 12) at 0x812DB46). D1 is called from BF2Engine::shutdownEngine (0x813949E);
// D2 has no callers. Defining this resolves the link.log unresolved ~BF2Console external.
BF2Console::~BF2Console()
{
}

// . Pure forward; `this` never touched. void [CHOSEN].
void BF2Console::outputToConsole(const std::string& text)
{
    io::mainConsole->output(text);                           // vcall vptr+0x14 at 0x812DB77
}

// . Pure forward; `this` never touched. Reference return [CHOSEN]: the callee
// (0x84516E0) returns &m_workingFile and this function passes eax through untouched.
const std::string& BF2Console::getWorkingFile()
{
    return io::mainConsole->getWorkingFile();                // vcall vptr+0x90 at 0x812DB90
}

// . Runs one console line at full access, discarding output. No callers in the
// ELF (Windows-client entry point). void [CHOSEN] - see the header amendment note.
void BF2Console::runConsoleCommand(const std::string& command)
{
    if (command.empty())                                     // 0x812DBB6 length-word test -
        return;                                              //   early-out before the access save/restore
    int access = io::mainConsole->getAccess();               // vcall vptr+0x9C at 0x812DBCC
    io::mainConsole->setAccess(1);                           // vcall vptr+0x98 at 0x812DBE7
    std::string line(command);
    line.append("\n", strlen("\n"));                         // /0x812DC18 - the binary really
                                                             //   strlen()s a one-char literal
    io::mainConsole->executeLine(line, io::Console::ignoredString_,      // vcall vptr+0x0C at 0x812DC55;
                                 false, false, false, false, false);     //   out-arg ds:0x8AC3C80; all five
                                                             //   bools zeroed (0x812DC26-0x812DC3E)
    io::mainConsole->setAccess(access);                      // vcall vptr+0x98 at 0x812DC7A
}

// of which ~1250 are std::string ctor/dtor scaffolding around the eight
// empty argument strings (all built from the shared "" literal IDA labels `filename`, 0x88E24BB -
// see BF2Engine.cpp:660). No callers in the ELF. void [CHOSEN].
void BF2Console::runConsoleFile(const std::string& filePath)
{
    int access = io::mainConsole->getAccess();
    io::mainConsole->setAccess(1);
    //  std::string::find(".con", 0, strlen(".con")); cmp eax,-1 at 0x812DD11.
    // pos = 0: a contains-anywhere test, NOT ends-with (the IDB user comment says "ends with";
    // the instruction stream says otherwise). "maps/foo.console" would be run un-suffixed.
    if (filePath.find(".con") == std::string::npos)          // jz 0x812DFD3 -> append branch
    {
        std::string path(filePath);
        path.append(".con", strlen(".con"));
        io::mainConsole->run(path, "", "", "", "", "", "", "", "",       // vcall vptr+0x28; bool result ignored
                             io::Console::ignoredString_);   // ds:0x8AC3C80 (xref 0x812DFDB)
    }
    else
    {
        io::mainConsole->run(filePath, "", "", "", "", "", "", "", "",
                             io::Console::ignoredString_);   // xref 0x812DD34
    }
    io::mainConsole->setAccess(access);                      // single join point, both branches
}

// . Forwards to the registry singleton; `this` never touched. void [CHOSEN].
void BF2Console::registerConsoleObjects(std::list<ConsoleObject*>& objects)
{
    consoleObjects()->registerConsoleObjects(objects);       // direct call, this = *ds:0x8990140
}

// . Refills m_commands with the function names of every active console object
// whose object name equals `command` (case-insensitive, exact - not a prefix test), and returns the
// member list by reference (eax = this+4 at 0x812E361). No callers in the ELF.
std::list<std::string>& BF2Console::getConsoleCommands(const std::string& command)
{
    m_commands.clear();
    const std::list<ConsoleObject*>& objs = consoleObjects()->getConsoleObjects();
    for (std::list<ConsoleObject*>::const_iterator it = objs.begin(); it != objs.end(); ++it)
    {
        ConsoleObject* co = *it;                             // node data at +8, 0x812E320
        // command.c_str() sits inside the loop in the binary (the inlined pre-3.4 c_str() writes
        // the terminal byte each iteration, 0x812E328-0x812E334) - kept inside to match.
        if (compareIgnoreCase(co->getObjectName(), command.c_str()) == 0
            && co->isActive())
        {
            // insert at begin() == push_front (0x812E3A0-0x812E3A6): the match list ends up in
            // reverse registry order. Odd, observable through the public API - preserved.
            m_commands.insert(m_commands.begin(), std::string(co->getFunctionName()));
        }
    }
    return m_commands;
}

// . As above, plus: return type name must equal "Void" and the binding must
// take no arguments - i.e. the commands you can invoke bare. Short-circuit order preserved:
// name, returnType, maxArgs, isActive (each its own vcall+branch). No callers in the ELF.
std::list<std::string>& BF2Console::getVoidConsoleCommands(const std::string& command)
{
    m_commands.clear();
    const std::list<ConsoleObject*>& objs = consoleObjects()->getConsoleObjects();
    for (std::list<ConsoleObject*>::const_iterator it = objs.begin(); it != objs.end(); ++it)
    {
        ConsoleObject* co = *it;
        if (compareIgnoreCase(co->getObjectName(), command.c_str()) == 0
            && compareIgnoreCase(co->getReturnType(), "Void") == 0         //  / 0x812E4A4, literal 0x88920C8
            && co->getMaxNoArgs() == 0
            && co->isActive())
        {
            m_commands.insert(m_commands.begin(), std::string(co->getFunctionName()));
        }
    }
    return m_commands;
}

// Drains pending console characters into OldConsole each frame. The dedicated
// image has no character producer.
void BF2Console::updateConsole()
{
    if (!m_consoleActive)                                    //  cmp byte ptr [this], 0
        return;

    for (;;)
    {
        int readIdx = consoleCharRead;
        int count   = consoleCharCount;                      //  (the 0xA7-skip path re-enters
                                                             //   here; readIdx was already stored back,
                                                             //   so a full `continue` is identical)
        unsigned char ch;
        // The only emptiness test is count==0 AND read==write (0x812E55C/0x812E560). If count is
        // 0 but the indices differ it still consumes a byte - exactly as compiled.
        if (count == 0 && readIdx == consoleCharWrite)
        {
            ch = 0;
        }
        else
        {
            if (readIdx > 255)                               //  signed compare: 255 is a valid
            {                                                //   index; wrap happens only when past it,
                readIdx = 0;                                 //   so after consuming buf[255] the stored
                consoleCharRead = 0;                         //   index sits at 256 until the next spin
            }
            ch = static_cast<unsigned char>(consoleCharBuffer[readIdx]);
            consoleCharRead = readIdx + 1;
            int c = count - 1;                               // 0x812E58F dec + cmovns:
            if (c < 0)                                       //   clamp at zero, never negative
                c = 0;
            consoleCharCount = c;                            //  (PE: `sub [count], 1; jns`)
        }
        if (ch == 0)                                         // empty, OR a literal NUL in
            return;                                          //   the buffer (which is thus consumed)
        if (ch == 0xA7)                                      // 0xA7 '§', the console-toggle
            continue;                                        //   key character: swallowed, not forwarded
        io::mainConsole->updateAsciiKey((char)ch);           // 0x812E5B7 vcall vptr+0x18;
                                                             //   movsx at 0x812E5AB = plain (signed) char
    }
}

// . Flag store first (0x812E5E1), then forward to the inherited
// Console::setConsoleActive slot (vptr+0xA4 at 0x812E5F5). Called by BF2Engine::filterGameInput
// (0x81370BB, with !m_consoleActive - the toggle, which then flushes the key buffer) and
// Demo::beginMainLoop (0x8141D29). void [CHOSEN].
void BF2Console::setConsoleActive(bool active)
{
    m_consoleActive = active;                                //  byte store
    io::mainConsole->setConsoleActive(active);               //  vcall vptr+0xA4
}

} // namespace hfe
} // namespace dice
