// HUD radio menus with native container storage. [INFERRED] Names and header placement.
// Preserve the interface without adding a destructor slot.
#ifndef MENU_HUDRADIO_H
#define MENU_HUDRADIO_H
#include "../Core/BuildConfig.h"
#if !DEDICATED
#include <map>
#include <string>
namespace dice { namespace hfe { namespace hud {
class HudRadio {
public:
    HudRadio();
    ~HudRadio(); // Nonvirtual: the HUD explicitly destroys each concrete owner.
    // Names and reference/const spelling [INFERRED]; stack widths and by-value
    // strings are established by complete bodies and their MSVC imports.
    virtual void configure(std::string prefix, int count);
    virtual void setPrefix(std::string prefix);
    virtual bool selectOption(const std::string& option);
    virtual void reset();                                        // +0C 7A0BD0
    virtual void setEnabled(bool enabled);
    virtual bool isEnabled();
    virtual void addEntry(std::string key, std::string localizedId,
                          std::string command);
    virtual bool isActive();                                     // +1C 69CA50
    virtual void setActive(bool active);
    virtual void updateString(const char** destination, const char* source);
    virtual std::wstring getText(std::string key);
    virtual std::string getCommand(std::string key);              // +2C 7A1560
private:
    bool m_active;                  // PE +04; ctor false
    bool m_enabled;                 // +05; ctor true, +06/+07 padding untouched
    std::map<std::string, std::wstring> m_text; // +08; ordinary std::less<string>
    std::map<std::string, std::string> m_commands; // +14
    std::string m_path;             // +20; assigned "1" after count initialization
    std::string m_prefix;           // +3C; constructed from ""
    int m_count;                    // +58; ctor 1, configure accepts signed counts
    // Empty-string identity sentinel. [UNRESOLVED] Original array extent.
    static char s_emptyString;
};
} } }
#endif
#endif
