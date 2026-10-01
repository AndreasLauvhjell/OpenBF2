// Reserved-player-name list and configuration file loading.
// The singleton is constructed before engine parameter parsing; loading executes the
// configuration through the console, so populating the list requires its command bindings.

#include "ServerSettings.h"
#include "../../IO/Console/Console.h"    // io::mainConsole, io::Console::ignoredString_
#include "../../IO/File/FileManager.h"   // io::fileManager - load()'s existence gate

namespace dice {
namespace hfe {

ReservedSlots::ReservedSlots()
{
}

// Member destruction releases reserved nicknames before the backing filename.
ReservedSlots::~ReservedSlots()
{
}

// instance - ELF 0x80B9B60, 72 bytes. The guarded-function-local-static pattern: guard byte
// object 0x8AAB778, __cxa_atexit(__tcf_0_91) - a Meyers singleton, transcribed as one.
// Identical in shape to MapList::instance and ServerSettings::instance.
ReservedSlots* ReservedSlots::instance()
{
    static ReservedSlots s;
    return &s;
}

// getConfigFile - ELF 0x80B9D90, eight bytes: `mov eax,[ebp+this]; ret`. It returns `this`, which
// is the address of m_configFile because that member sits at +0x00 - and the caller in
// BF2Engine::parseParameters immediately does `mov eax,[eax]; mov eax,[eax-0Ch]` on the result to
// read the string's length (0x81334AC), which only type-checks for a std::string reference.
const std::string& ReservedSlots::getConfigFile() const
{
    return m_configFile;
}

// setConfigFile - ELF 0x80B9DA0, nine bytes, and the whole body is a tail jump:
// `push ebp; mov ebp,esp; pop ebp; jmp std::string::assign(const std::string&)` with `this`
// unchanged - so it assigns straight into the +0x00 member. No validation, no dirty flag.
void ReservedSlots::setConfigFile(const std::string& configFile)
{
    m_configFile.assign(configFile);
}

// Loads the reserved-player console script. Returns false for an empty or missing
// path; otherwise clears the current list before executing the script.
bool ReservedSlots::load()
{
    if (m_configFile.empty()
        || !io::fileManager->fileExists(m_configFile))    // vt +0x1C on ds:0x8AC8C64
        return false;

    const bool useRelative = io::mainConsole->getUseRelativePaths();
    io::mainConsole->setUseRelativePaths(false);

    io::mainConsole->run(m_configFile, "", "", "", "", "", "", "", "",
                         io::Console::ignoredString_);    // vt +0x28

    io::mainConsole->setUseRelativePaths(useRelative);
    return true;                                          // the run() result is NOT consulted
}

// Strips the clan tag through the first space, then checks the entire remaining nickname case-sensitively.
bool ReservedSlots::isNickReserved(const std::string& nick) const
{
    for (std::vector<std::string>::const_iterator it = m_nicks.begin(); it != m_nicks.end(); ++it)
    {
        std::string name = nick;
        const std::string::size_type space = name.find(" ");
        if (space != std::string::npos)
            name = name.substr(space + 1);               // drop the clan tag and its trailing space
        if (name == *it)
            return true;
    }
    return false;
}

// Publish the singleton pointer before console bindings test its availability.
ReservedSlots* reservedSlots = ReservedSlots::instance();   // ds:0x8AAB768

} // namespace hfe
} // namespace dice
