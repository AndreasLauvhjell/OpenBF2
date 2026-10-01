// Owns a tagged console transcript file. Header and member names are [INFERRED].
// Each record stores a byte tag, a signed 32-bit length and payload: W for a directory, E for a
// command ending in newline. Lengths remain 32-bit on x64; the class contains only an owned File*.
// The constructor selects writing or reading; destruction closes and deletes the file.
// [UNRESOLVED] writer return types: callers discard them, so the void API is [CHOSEN, not recovered].
// executeCompiled remains declaration-only.

#ifndef IO_CONSOLE_CONSOLECOMPILER_H     // [INFERRED] guard style
#define IO_CONSOLE_CONSOLECOMPILER_H

#include <string>

namespace dice {
namespace hfe {
namespace io {

class File;

class ConsoleCompiler
{
public:
    ConsoleCompiler(const std::string& fileName, bool writing);
    ~ConsoleCompiler();

    void setWorkingDir(const std::string&);   // writes to the transcript file
    void addLine(const std::string&);         // 0x8451B20 - [UNRESOLVED] return type
    void executeCompiled();                   // replays via mainConsole

private:
    File* m_file;
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif
