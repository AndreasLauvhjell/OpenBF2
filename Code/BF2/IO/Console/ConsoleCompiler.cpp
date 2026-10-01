// Path: [INFERRED] beside ConsoleCompiler.h; no __FILE__ witness for this class.
// Contents: recovered ELF constructor/destructor, paired PE/ELF binary transcript writers.
#include "ConsoleCompiler.h"
#include "../File/File.h"

namespace dice { namespace hfe { namespace io {

// Owns the compiled-console file. Serialized lengths are four-byte integers regardless of pointer width.
ConsoleCompiler::ConsoleCompiler(const std::string& fileName, bool writing)
    : m_file(new File(fileName, static_cast<FileMode>(writing ? 0 : 2),
                      static_cast<FileAccess>(writing ? 2 : 0), 0))
{
}

ConsoleCompiler::~ConsoleCompiler()
{
    m_file->close();
    delete m_file;
}

void ConsoleCompiler::setWorkingDir(const std::string& directory)
{
    const int size = static_cast<int>(directory.size());
    m_file->write("W", 1);
    m_file->write(size);
    m_file->write(directory.c_str(), static_cast<unsigned int>(size));
}

// Prefixes are case-sensitive, anchored at zero, and checked in this shipped order.
// 'E' is followed by int(length + 1), raw command bytes, then a newline.
void ConsoleCompiler::addLine(const std::string& line)
{
    if (line.find("include ", 0) == 0) return;
    if (line.find("run ", 0)     == 0) return;
    if (line.find("rem ", 0)     == 0) return;

    const unsigned int size = static_cast<unsigned int>(line.size());
    m_file->write("E", 1);
    m_file->write(static_cast<int>(size + 1));
    m_file->write(line.c_str(), size);
    m_file->write("\n", 1);
}

} } }

