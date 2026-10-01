// Code/Tools/BF2Editor/Framework/EditorSettings.h

// The editor's own settings: a small `key = value` file beside the
// executable, BF2Editor.settings.ini - never the game's settings and never the retail editor's
// bf2editor\Settings. The window layout stays in ImGui's own BF2Editor.imgui.ini.

// Format: one `key = value` per line; keys are letters, digits and . _ - ; `#` or `;` starts a comment
// line; blank lines are ignored. A line that is not a setting is reported and skipped, so a damaged
// file costs only that line. Written back sorted by key. Engine-free. Editor-only code (D3).
#ifndef BF2EDITOR_FRAMEWORK_EDITORSETTINGS_H
#define BF2EDITOR_FRAMEWORK_EDITORSETTINGS_H

#include "Framework/EditorConfig.h"

#include <map>
#include <string>
#include <vector>

namespace bf2editor
{

class EditorSettings
{
public:
    // A missing file is not an error: every setting keeps its default. `problems` gets one line per
    // skipped line.
    bool load(const std::string& path, std::vector<std::string>& problems);
    bool save(const std::string& path, std::string& error) const;

    // The text form, for load/save and the checks.
    void parse(const std::string& text, std::vector<std::string>& problems);
    std::string serialize() const;

    bool has(const std::string& key) const;
    std::string get(const std::string& key, const std::string& fallback) const;
    // A value that does not parse as the type asked for gives the fallback.
    float getFloat(const std::string& key, float fallback) const;
    int getInt(const std::string& key, int fallback) const;
    bool getBool(const std::string& key, bool fallback) const;

    // False (and nothing stored) for a key outside the format; a value keeps one line.
    bool set(const std::string& key, const std::string& value);
    bool setFloat(const std::string& key, float value);
    bool setInt(const std::string& key, int value);
    bool setBool(const std::string& key, bool value);

    const std::map<std::string, std::string>& values() const { return m_values; }

    static bool isValidKey(const std::string& key);

private:
    std::map<std::string, std::string> m_values;
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_EDITORSETTINGS_H
