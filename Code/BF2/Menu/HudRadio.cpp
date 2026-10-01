// HUD radio controls.
#include "HudRadio.h"
#if !DEDICATED
#include "BF2HudManager.h"
#include "HudInformationAdapter.h"
#include "../Game/Main/BF2ClientSeams.h" // existing typed hudManager global
#include "../Core/StringUtil.h"
#include "../IO/Console/Console.h"
#include <cstring>
#include <new>
#include <utility>
namespace dice { namespace hfe { namespace hud {
char HudRadio::s_emptyString;

HudRadio::HudRadio() : m_prefix("")
{
    m_count = 1;
    m_path = "1";
    m_enabled = true;
    m_active = false;
}
HudRadio::~HudRadio() {} // reverse native string/map destruction only.

void HudRadio::configure(std::string prefix, int count)
{
    setPrefix(prefix); // A second by-value copy; callback precedes the count store.
    m_count = count;
}
void HudRadio::setPrefix(std::string prefix) { m_prefix = prefix; }
void HudRadio::setEnabled(bool enabled) { m_enabled = enabled; }
bool HudRadio::isEnabled() { return m_enabled; }
bool HudRadio::isActive() { return m_active; }
void HudRadio::setActive(bool active) { m_active = active; }

void HudRadio::addEntry(std::string key, std::string localizedId, std::string command)
{
    if (!key.empty() && !localizedId.empty()) {
        const auto found = m_text.find(key);
        const bool exists = found != m_text.end();
        // Retail performs the find before the localization virtual callback.
        const std::wstring localized = hudManager->getLocalizedString(localizedId);
        if (exists) found->second = localized;
        else m_text.insert(std::make_pair(key, localized));
    }
    if (!key.empty() && !command.empty()) {
        const auto found = m_commands.find(key);
        // the existing-entry arm uses arg_1C, not arg_38. Preserve it.
        if (found != m_commands.end()) found->second = localizedId;
        else m_commands.insert(std::make_pair(key, command));
    }
}
std::wstring HudRadio::getText(std::string key)
{
    if (!key.empty()) {
        const auto found = m_text.find(key);
        if (found != m_text.end()) return std::wstring(found->second);
    }
    return std::wstring(L"");
}
std::string HudRadio::getCommand(std::string key)
{
    if (!key.empty()) {
        const auto found = m_commands.find(key);
        if (found != m_commands.end()) return std::string(found->second);
    }
    return std::string("");
}
void HudRadio::updateString(const char** destination, const char* source)
{
    // 7A0B20 compares before checking the old pointer. A null destination value
    // remains unsafe. 737EE0 clears after scalar delete; 737DC0 copies after it.
    if (!source || std::strcmp(source, *destination) == 0) return;
    const char* old = *destination;
    if (old && old != &s_emptyString) {
        ::operator delete(const_cast<char*>(old));
        *destination = 0;
    }
    const std::size_t bytes = std::strlen(source) + 1;
    char* replacement = static_cast<char*>(::operator new(bytes));
    std::memcpy(replacement, source, bytes);
    *destination = replacement;
}
void HudRadio::reset()
{
    m_path = "1";
    m_active = false;
    for (int i = 1; i <= m_count; ++i) {
        HudInformationAdapter* adapter = hudManager->getInformationAdapter();
        const char* prefix = m_prefix.c_str(); // cached before getText's callback
        adapter->setWideString(formatString("%s%iString", prefix, i),
                              getText(formatString("Interface_1:%i", i)));
    }
}
bool HudRadio::selectOption(const std::string& option)
{
    bool noOptions = true;
    std::wstring text;
    for (int i = 1; i <= m_count; ++i) {
        const std::string key = std::string("Interface_") + m_path + ":" + option;
        text = getText(key + formatString(":%i", i));
        if (text != L"") {
            HudInformationAdapter* adapter = hudManager->getInformationAdapter();
            const char* prefix = m_prefix.c_str();
            adapter->setWideString(formatString("%s%iString", prefix, i), text);
            noOptions = false;
        }
    }
    if (!noOptions) {
        m_path = m_path + ":" + option;
    } else {
        // Retail calculates this localized prefix and then never uses it.
        std::string context = makeNarrow(getText(std::string("Interface_") + m_path));
        if (context != "") context = context + "::";
        const std::string command = getCommand(std::string("Interface_") + m_path + ":" + option) + "\n";
        io::mainConsole->executeLine(command, io::Console::ignoredString_, false, false, false, false, false);
        m_active = false;
    }
    return true;
}
} } }
#endif
