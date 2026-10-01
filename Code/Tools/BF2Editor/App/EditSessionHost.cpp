// Session camera bookmarks and undoable light presets.
// Keeps two bookmarks; a third replaces the oldest and toggle selects the farther one.
// Presets edit recognized light/fog/sky settings and list unsupported statements. Reset restores
// the last saved Sky.con values. Camera-derived sun/flare direction is [INFERRED].
#include "Framework/EditorConfig.h"
#include "App/EditSession.h"

#include "App/FreeCamera.h"
#include "App/LevelDocument.h"
#include "App/LevelSettings.h"
#include "Framework/EditorLog.h"
#include "Framework/LevelSettingsFile.h"
#include "Framework/ObjectLibrary.h"

#include <windows.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <vector>

namespace bf2editor
{

namespace
{

const char kLightPresetFolder[] = "bf2editor/Content/LightPreset";

std::string lower(std::string text)
{
    for (size_t i = 0; i < text.size(); ++i)
        if (text[i] >= 'A' && text[i] <= 'Z')
            text[i] = static_cast<char>(text[i] - 'A' + 'a');
    return text;
}

std::string trimmed(const std::string& text)
{
    size_t begin = 0;
    size_t end = text.size();
    while (begin < end && (text[begin] == ' ' || text[begin] == '\t'))
        ++begin;
    while (end > begin && (text[end - 1] == ' ' || text[end - 1] == '\t' || text[end - 1] == '\r'))
        --end;
    return text.substr(begin, end - begin);
}

// The settings a light preset can carry: Sky.con's light, fog and sky groups.
bool presetSetting(const SettingSpec& spec)
{
    return _stricmp(spec.file, "Sky.con") == 0
           && (std::strcmp(spec.group, "Light") == 0 || std::strcmp(spec.group, "Fog and sky") == 0);
}

struct PresetStatement
{
    std::string command;
    std::string value;
};

// A preset's statements as the console runs them with v_arg1 = BF2Editor: the `if v_arg1 == BF2Editor`
// branches taken, their `else` branches and any other condition not; rem lines and blanks skipped.
std::vector<PresetStatement> presetStatements(const std::string& text)
{
    std::vector<PresetStatement> out;
    std::vector<bool> taken;         // per open if: whether its current branch runs
    std::vector<bool> outer;         // per open if: whether the enclosing code runs
    size_t start = 0;
    while (start <= text.size())
    {
        size_t end = text.find('\n', start);
        if (end == std::string::npos)
            end = text.size();
        const std::string line = trimmed(text.substr(start, end - start));
        start = end + 1;
        if (line.empty())
            continue;
        const size_t blank = line.find_first_of(" \t");
        const std::string word = lower(line.substr(0, blank));
        const std::string rest = blank == std::string::npos ? std::string() : trimmed(line.substr(blank));
        const bool running = taken.empty() || (taken.back() && outer.back());
        if (word == "if")
        {
            std::string condition;
            for (size_t i = 0; i < rest.size(); ++i)
                if (rest[i] != ' ' && rest[i] != '\t')
                    condition += static_cast<char>(rest[i] >= 'A' && rest[i] <= 'Z' ? rest[i] - 'A' + 'a' : rest[i]);
            outer.push_back(running);
            taken.push_back(condition == "v_arg1==bf2editor");
            continue;
        }
        if (word == "else" && !taken.empty())
        {
            taken.back() = !taken.back();
            continue;
        }
        if (word == "endif" && !taken.empty())
        {
            taken.pop_back();
            outer.pop_back();
            continue;
        }
        if (!running || word == "rem")
            continue;
        PresetStatement statement;
        statement.command = line.substr(0, blank);
        statement.value = rest;
        out.push_back(statement);
    }
    return out;
}

// The mod's level folders: mods/<mod>/Levels/<level>/ of the open level, its parent.
std::string levelsFolder(const std::string& levelDir)
{
    std::string levels = levelDir;
    while (!levels.empty() && (levels[levels.size() - 1] == '/' || levels[levels.size() - 1] == '\\'))
        levels.erase(levels.size() - 1);
    const size_t slash = levels.find_last_of("/\\");
    return slash == std::string::npos ? std::string() : levels.substr(0, slash);
}

bool validLevelName(const std::string& name)
{
    return !name.empty() && name.size() <= 128 && name.find_first_of("/\\:*?\"<>|") == std::string::npos
           && name != "." && name != "..";
}

bool validPresetName(const std::string& name)
{
    if (name.empty() || name.size() > 64)
        return false;
    for (size_t i = 0; i < name.size(); ++i)
    {
        const char c = name[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return false;
    }
    return true;
}

} // namespace

// Camera position bookmarks

bool EditSession::cameraBookmark(const std::string& action, int index, std::string& result)
{
    char text[160];
    if (action == "add")
    {
        CameraBookmark bookmark;
        bookmark.x = m_camera.x();
        bookmark.y = m_camera.y();
        bookmark.z = m_camera.z();
        bookmark.yaw = m_camera.yawDegrees();
        bookmark.pitch = m_camera.pitchDegrees();
        const bool dropped = m_cameraBookmarks.size() == 2;
        if (dropped)
        {
            m_cameraBookmarks[0] = m_cameraBookmarks[1];
            m_cameraBookmarks[1] = bookmark;
        }
        else
            m_cameraBookmarks.push_back(bookmark);
        std::snprintf(text, sizeof(text), "position bookmark %u at %.1f/%.1f/%.1f%s",
                      static_cast<unsigned>(m_cameraBookmarks.size()), bookmark.x, bookmark.y, bookmark.z,
                      dropped ? " (the oldest of the two dropped)" : "");
        result = text;
        return true;
    }
    if (action == "toggle")
    {
        if (m_cameraBookmarks.size() != 2)
        {
            result = "toggling needs two position bookmarks (EditorCamera.addBookmark)";
            return false;
        }
        float distance[2];
        for (int i = 0; i < 2; ++i)
        {
            const float dx = m_cameraBookmarks[static_cast<size_t>(i)].x - m_camera.x();
            const float dy = m_cameraBookmarks[static_cast<size_t>(i)].y - m_camera.y();
            const float dz = m_cameraBookmarks[static_cast<size_t>(i)].z - m_camera.z();
            distance[i] = std::sqrt(dx * dx + dy * dy + dz * dz);
        }
        // The farther of the two, the second on a tie (retail's >=).
        return cameraBookmark("go", distance[1] >= distance[0] ? 2 : 1, result);
    }
    if (action == "go")
    {
        if (index < 1 || static_cast<size_t>(index) > m_cameraBookmarks.size())
        {
            std::snprintf(text, sizeof(text), "no position bookmark %d (there %s %u)", index,
                          m_cameraBookmarks.size() == 1 ? "is" : "are", static_cast<unsigned>(m_cameraBookmarks.size()));
            result = text;
            return false;
        }
        const CameraBookmark& bookmark = m_cameraBookmarks[static_cast<size_t>(index - 1)];
        m_camera.place(bookmark.x, bookmark.y, bookmark.z, bookmark.yaw, bookmark.pitch);
        std::snprintf(text, sizeof(text), "camera at position bookmark %d (%.1f/%.1f/%.1f)", index, bookmark.x, bookmark.y,
                      bookmark.z);
        result = text;
        return true;
    }
    if (action == "list")
    {
        result.clear();
        for (size_t i = 0; i < m_cameraBookmarks.size(); ++i)
        {
            std::snprintf(text, sizeof(text), "%s%u. [%.1f %.1f %.1f]", i ? "; " : "", static_cast<unsigned>(i + 1),
                          m_cameraBookmarks[i].x, m_cameraBookmarks[i].y, m_cameraBookmarks[i].z);
            result += text;
        }
        if (result.empty())
            result = "no position bookmarks";
        return true;
    }
    result = "no bookmark action '" + action + "'";
    return false;
}

// Light presets

std::vector<std::string> EditSession::lightPresetNames()
{
    std::vector<std::string> names;
    WIN32_FIND_DATAA found;
    HANDLE h = FindFirstFileA((std::string(kLightPresetFolder) + "/*.con").c_str(), &found);
    if (h == INVALID_HANDLE_VALUE)
        return names;
    do
    {
        if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        std::string name = found.cFileName;
        if (name.size() > 4 && _stricmp(name.c_str() + name.size() - 4, ".con") == 0)
            names.push_back(name.substr(0, name.size() - 4));
    } while (FindNextFileA(h, &found));
    FindClose(h);
    std::sort(names.begin(), names.end());
    return names;
}

std::vector<std::string> EditSession::lightLevelNames()
{
    std::vector<std::string> names;
    std::string levelDir, levelPath, error;
    if (!lightingLevel(levelDir, levelPath, error))
        return names;
    const std::string levels = levelsFolder(levelDir);
    if (levels.empty())
        return names;
    WIN32_FIND_DATAA found;
    HANDLE h = FindFirstFileA((levels + "/*").c_str(), &found);
    if (h == INVALID_HANDLE_VALUE)
        return names;
    do
    {
        const std::string name = found.cFileName;
        if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || name == "." || name == "..")
            continue;
        if (GetFileAttributesA((levels + "/" + name + "/Sky.con").c_str()) != INVALID_FILE_ATTRIBUTES)
            names.push_back(name);
    } while (FindNextFileA(h, &found));
    FindClose(h);
    std::sort(names.begin(), names.end());
    return names;
}

bool EditSession::lightPreset(const std::string& action, const std::string& argument, std::string& result)
{
    const std::vector<SettingSpec>& specs = levelSettingSpecs();
    if (action == "list")
    {
        const std::vector<std::string> names = lightPresetNames();
        result.clear();
        for (size_t i = 0; i < names.size(); ++i)
            result += (i ? ", " : "") + names[i];
        if (result.empty())
            result = std::string("no light presets in ") + kLightPresetFolder;
        return true;
    }
    if (action == "levels")
    {
        const std::vector<std::string> names = lightLevelNames();
        result.clear();
        for (size_t i = 0; i < names.size(); ++i)
            result += (i ? ", " : "") + names[i];
        if (result.empty())
            result = "no level of the mod has a Sky.con";
        return true;
    }
    if (!m_document || !m_document->settingsFile("Sky.con"))
    {
        result = "the level has no Sky.con the editor can write: refused";
        return false;
    }
    // The edits one action makes, as one history entry.
    std::vector<std::string> doLines, undoLines;
    std::vector<std::string> notes;
    const std::string label = action == "apply" ? "Light preset " + argument
                            : action == "level" ? "Light of the level " + argument
                            : action == "reset" ? std::string("Light back to the level's saved settings")
                            : action == "sun" ? std::string("Sun direction from the camera")
                            : std::string("Flare direction from the camera");
    // One setting to a value (the editor's form); unchanged ones skipped.
    struct Change
    {
        static void add(EditSession& session, const SettingSpec& spec, const std::string& value,
                        std::vector<std::string>& doLines, std::vector<std::string>& undoLines)
        {
            std::string old, ignored;
            const bool had = session.levelSetting(spec.key, old, ignored);
            if (had && settingToFile(spec, old) == settingToFile(spec, value))
                return;
            doLines.push_back("EditorLevelSettings.apply " + std::string(spec.key) + " \"" + value + "\"\n");
            undoLines.push_back(had ? "EditorLevelSettings.apply " + std::string(spec.key) + " \"" + old + "\"\n"
                                    : "EditorLevelSettings.unset " + std::string(spec.key) + "\n");
        }
    };
    if (action == "apply" || action == "level")
    {
        std::string text, error, path;
        if (action == "apply")
        {
            if (!validPresetName(argument))
            {
                result = "'" + argument + "' is not a light preset name (EditorLight.presets lists them)";
                return false;
            }
            path = std::string(kLightPresetFolder) + "/" + argument + ".con";
            if (!ObjectLibrary::readTextFile(path, text, error))
            {
                result = "no light preset " + argument + " (" + path + ")";
                return false;
            }
        }
        else
        {
            std::string levelDir, levelPath;
            if (!validLevelName(argument) || !lightingLevel(levelDir, levelPath, error))
            {
                result = "'" + argument + "' is not a level of the mod (EditorLight.levels lists them)";
                return false;
            }
            path = levelsFolder(levelDir) + "/" + argument + "/Sky.con";
            if (!ObjectLibrary::readTextFile(path, text, error))
            {
                result = "the level " + argument + " has no Sky.con (" + path + ")";
                return false;
            }
        }
        const std::vector<PresetStatement> statements = presetStatements(text);
        std::map<std::string, std::string> values;        // key -> value, the last statement winning
        std::vector<std::string> order;
        std::set<std::string> skipped;
        for (size_t i = 0; i < statements.size(); ++i)
        {
            const std::string command = lower(statements[i].command);
            const SettingSpec* spec = 0;
            for (size_t s = 0; s < specs.size() && !spec; ++s)
                if (presetSetting(specs[s]) && (lower(specs[s].command) == command
                                                || (specs[s].twin && lower(specs[s].twin) == command)))
                    spec = &specs[s];
            if (!spec)
            {
                // Two stock Sky.con lines are deliberately left: the light manager's init is a command, and
                // the base hemi map names the level's own ground hemi file (another level's would be wrong).
                if (command == "lightmanager.init")
                    notes.push_back(statements[i].command + ": a command, not a setting");
                else if (command == "hemimapmanager.setbasehemimap")
                    notes.push_back(statements[i].command + ": names that level's own hemi map, not applied");
                else
                    skipped.insert(_strnicmp(command.c_str(), "lightmapgeneration.", 19) == 0
                                       ? std::string("LightMapGeneration.* (the lightmap settings)")
                                       : statements[i].command);
                continue;
            }
            const std::string value = settingFromFile(*spec, statements[i].value);
            std::string invalid;
            if (!validSetting(*spec, value, invalid))
            {
                notes.push_back(statements[i].command + " " + statements[i].value + " (" + invalid + ")");
                continue;
            }
            if (!values.count(spec->key))
                order.push_back(spec->key);
            values[spec->key] = value;
        }
        for (size_t i = 0; i < order.size(); ++i)
            Change::add(*this, *findLevelSetting(order[i]), values[order[i]], doLines, undoLines);
        for (std::set<std::string>::const_iterator it = skipped.begin(); it != skipped.end(); ++it)
            notes.push_back(*it + ": not a level setting, not applied");
    }
    else if (action == "save")
    {
        if (!validPresetName(argument))
        {
            result = "'" + argument + "' is not a preset name (letters, digits, - and _)";
            return false;
        }
        const std::string path = std::string(kLightPresetFolder) + "/" + argument + ".con";
        if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            result = path + " is there already: refused (choose another name, or delete it first)";
            return false;
        }
        if (GetFileAttributesA(kLightPresetFolder) == INVALID_FILE_ATTRIBUTES)
        {
            result = std::string(kLightPresetFolder) + " is not there (the retail editor's folder beside the game)";
            return false;
        }
        std::string text;
        unsigned lines = 0;
        for (size_t s = 0; s < specs.size(); ++s)
        {
            std::string value, error;
            if (!presetSetting(specs[s]) || !levelSetting(specs[s].key, value, error))
                continue;
            // The stock presets name the terrain colours by their editor form.
            text += std::string(specs[s].twin ? specs[s].twin : specs[s].command) + " " + settingToFile(specs[s], value) + "\r\n";
            ++lines;
        }
        FILE* f = std::fopen(path.c_str(), "wb");
        if (!f || std::fwrite(text.data(), 1, text.size(), f) != text.size())
        {
            if (f)
                std::fclose(f);
            result = "could not write " + path;
            return false;
        }
        std::fclose(f);
        char counts[96];
        std::snprintf(counts, sizeof(counts), " (%u light, fog and sky settings)", lines);
        result = "saved the light preset " + path + counts;
        logLine(LogLevel::Info, "light: %s", result.c_str());
        return true;
    }
    else if (action == "reset")
    {
        LevelSettingsFile saved;
        saved.parse(m_document->settingsFile("Sky.con")->text());
        for (size_t s = 0; s < specs.size(); ++s)
        {
            if (!presetSetting(specs[s]))
                continue;
            std::string fileValue, old, ignored;
            const bool inFile = saved.value(specs[s].command, specs[s].index, specs[s].branch, fileValue);
            const bool had = levelSetting(specs[s].key, old, ignored);
            if (inFile)
                Change::add(*this, specs[s], settingFromFile(specs[s], fileValue), doLines, undoLines);
            else if (had)
            {
                // A statement an edit added: taken out again.
                doLines.push_back("EditorLevelSettings.unset " + std::string(specs[s].key) + "\n");
                undoLines.push_back("EditorLevelSettings.apply " + std::string(specs[s].key) + " \"" + old + "\"\n");
            }
        }
    }
    else if (action == "sun" || action == "flare")
    {
        const int sign = std::atoi(argument.c_str());
        if (sign != 1 && sign != -1)
        {
            result = "give 1 (the camera's view direction) or -1 (its opposite)";
            return false;
        }
        // The camera's matrix rows are right, up, forward and position.
        float matrix[16];
        m_camera.cameraToWorld(matrix);
        const float forward[3] = { matrix[8], matrix[9], matrix[10] };
        const float length = std::sqrt(forward[0] * forward[0] + forward[1] * forward[1] + forward[2] * forward[2]);
        if (length <= 0.0f)
        {
            result = "the camera has no view direction";
            return false;
        }
        float direction[3];
        for (int k = 0; k < 3; ++k)
        {
            direction[k] = sign * forward[k] / length;
            if (direction[k] == 0.0f)
                direction[k] = 0.0f;                    // never "-0" in the file
        }
        char value[96];
        std::snprintf(value, sizeof(value), "%g/%g/%g", direction[0], direction[1], direction[2]);
        const SettingSpec* spec = findLevelSetting(action == "sun" ? "light.sunDirection" : "sky.flareDirection");
        if (!spec)
        {
            result = "the level settings have no such direction";
            return false;
        }
        Change::add(*this, *spec, value, doLines, undoLines);
        notes.push_back(std::string(action == "sun" ? "sun" : "flare") + " direction " + value);
    }
    else
    {
        result = "no light action '" + action + "'";
        return false;
    }
    if (doLines.empty())
    {
        result = "the level has that light already";
        for (size_t i = 0; i < notes.size(); ++i)
            result += "; " + notes[i];
        return true;
    }
    HistoryEntry entry;
    entry.label = label;
    for (size_t i = 0; i < doLines.size(); ++i)
        entry.doScript += doLines[i];
    for (size_t i = undoLines.size(); i-- > 0;)
        entry.undoScript += undoLines[i];
    if (!m_history.execute(entry, result))
        return false;
    logLine(LogLevel::Info, "edit: %s", entry.label.c_str());
    char counts[64];
    std::snprintf(counts, sizeof(counts), " (%u setting%s)", static_cast<unsigned>(doLines.size()),
                  doLines.size() == 1 ? "" : "s");
    result = label + counts;
    for (size_t i = 0; i < notes.size(); ++i)
        result += "; " + notes[i];
    return true;
}

} // namespace bf2editor
