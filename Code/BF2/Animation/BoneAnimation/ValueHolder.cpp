// Shared animation value holders and their script persistence.

#include "ValueHolder.h"

#include "../../IO/Console/Console.h"     // io::mainConsole, io::Console::ignoredString_
#include "../../IO/File/File.h"           // saveValueHolders writes the .inc back out
#include "../../IO/Stream/IStream.h"
#include <sstream>

namespace dice {
namespace anim {

ValueHolderManager* valueHolderManager = 0;
// All soldier animation systems share this file, loaded once per process.

namespace {
const char* const kValueHoldersFile = "Objects/Soldiers/Common/Animations/ValueHolders.inc";
}
// Initializes values and message masks to zero; scripts supply any required nonzero range.

ValueHolder::ValueHolder()
    : min(0.0f)
    , max(0.0f)
    , normal(0.0f)
    , stopOnMessage(0)
    , passOnMessage(0)
{
}


ValueHolderManager::ValueHolderManager()
    : m_activeValueHolder(0)
    , m_valueHoldersLoaded(false)
{
}

// Deletes every holder.
ValueHolderManager::~ValueHolderManager()
{
    for (std::map<std::string, ValueHolder*, hfe::NoCaseStringCompare>::iterator it =
             m_valueHolders.begin(); it != m_valueHolders.end(); ++it)
    {
        if (it->second)
            delete it->second;
    }
    m_valueHolders.clear();
}

// static - the mangling takes no `this`.
const std::string& ValueHolderManager::getFilename()
{
    static const std::string s_filename(kValueHoldersFile);
    return s_filename;
}


// Loads shared value-holder definitions through the console.

void ValueHolderManager::loadValueHolders()
{
    if (m_valueHoldersLoaded)
        return;

    m_valueHoldersLoaded = true;              // before the read - see above

    const bool wasRelative = hfe::io::mainConsole->getUseRelativePaths();
    hfe::io::mainConsole->setUseRelativePaths(false);

    const std::string empty;
    hfe::io::mainConsole->run(getFilename(), empty, empty, empty, empty, empty, empty, empty, empty,
                              hfe::io::Console::ignoredString_);   // result discarded

    hfe::io::mainConsole->setUseRelativePaths(wasRelative);
}


// Writes all holders to the configured file without checking open/write status.
// Saving before loading can replace the file with an empty one.

void ValueHolderManager::saveValueHolders()
{
    hfe::io::File file;
    file.open(getFilename(), static_cast<hfe::io::IFileSystem::FileMode>(0),
              hfe::io::IFileSystem::FILEACCESS_2, 0);
    hfe::io::IStream* stream = file.getStream();

    for (std::map<std::string, ValueHolder*, hfe::NoCaseStringCompare>::iterator it =
             m_valueHolders.begin(); it != m_valueHolders.end(); ++it)
    {
        it->second->makeScript(stream);
    }

    file.close();
}


// Gets or creates a holder. Only creating a new holder changes the active selection.

ValueHolder* ValueHolderManager::createValueHolder(const std::string& name)
{
    ValueHolder* holder = get(name);
    if (holder)
        return holder;                        // not made active - see above

    holder = new ValueHolder();               // operator new(0x14) + ValueHolder()
    m_valueHolders.insert(std::make_pair(name, holder));
    m_activeValueHolder = holder;
    return holder;
}

// A keyed find; a miss is a null, not an insert.
ValueHolder* ValueHolderManager::get(const std::string& name)
{
    std::map<std::string, ValueHolder*, hfe::NoCaseStringCompare>::iterator it =
        m_valueHolders.find(name);
    return it == m_valueHolders.end() ? 0 : it->second;
}

// A name that does not exist clears the active holder rather than being ignored -
// the same behaviour AnimationSystemTemplateManager::setActiveSystem has.
void ValueHolderManager::setActiveByName(const std::string& name)
{
    m_activeValueHolder = get(name);
}

// The round trip: look the active pointer back up and return its name.
const std::string& ValueHolderManager::getActiveByName()
{
    return findName(m_activeValueHolder);
}


// Returns holder's registered name or a stable not-found message.

const std::string& ValueHolderManager::findName(ValueHolder* holder)
{
    static const std::string s_notFound("ValueHolder not found");

    if (!holder)
        return s_notFound;

    for (std::map<std::string, ValueHolder*, hfe::NoCaseStringCompare>::iterator it =
             m_valueHolders.begin(); it != m_valueHolders.end(); ++it)
    {
        if (it->second == holder)
            return it->first;
    }
    return s_notFound;
}

// Writes min, normal, max, then nonzero unsigned message masks in pass/stop order.
void ValueHolder::makeScript(hfe::io::IStream* stream)
{
    std::ostringstream output;
    output << "AnimationSystem.createValueHolder " << valueHolderManager->findName(this) << std::endl;
    output << "AnimationValueHolder.values " << min << " " << normal << " " << max << std::endl;
    if (passOnMessage) output << "AnimationValueHolder.passOnMessage " << passOnMessage << std::endl;
    if (stopOnMessage) output << "AnimationValueHolder.stopOnMessage " << stopOnMessage << std::endl;
    const std::string text = output.str();
    stream->write(text.c_str(), static_cast<unsigned int>(text.size()));
}

std::string ValueHolderManager::listValueHolders() const
{
    std::stringstream output;
    for (auto it = m_valueHolders.begin(); it != m_valueHolders.end(); ++it)
        output << it->first << "\n";
    return output.str();
}

} // namespace anim
} // namespace dice
