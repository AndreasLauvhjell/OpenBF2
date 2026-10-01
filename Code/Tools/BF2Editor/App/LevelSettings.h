// Maps level settings to in-place statements in Init.con, Sky.con and Water.con.
// Keeps terrain game/editor settings equal. Live settings also run in the engine;
// settings without a binding change only the file.
#ifndef BF2EDITOR_APP_LEVELSETTINGS_H
#define BF2EDITOR_APP_LEVELSETTINGS_H

#include "Framework/EditorConfig.h"

#include "Framework/LevelSettingsFile.h"

#include <string>
#include <vector>

namespace bf2editor
{

// How a value is written and edited.
enum class SettingKind
{
    Quoted,         // "text" (the command takes the text without the quotes)
    QuotedPair,     // "a" "b" (the command takes "a b")
    Word,           // text as it is (a texture path)
    Integer,
    Number,
    Numbers,        // a/b[/c[/d]]: `components` numbers
    Color,          // r/g/b in 0..1
    Color255,       // r/g/b in 0..255 (the fog colours)
    Camera          // x/y/z yaw/pitch/roll (gameLogic.setBeforeSpawnCamera)
};

struct SettingSpec
{
    const char*               key;          // the editor's name for it: "team1.name"
    const char*               group;        // the panel's section
    const char*               label;
    const char*               file;         // "Init.con"
    const char*               command;      // as the stock files spell it
    const char*               index;        // its index words ("1", "64 2"), "" for none
    LevelSettingsFile::Branch branch;
    const char*               twin;         // the editor-branch command kept equal to it, or 0
    SettingKind               kind;
    int                       components;   // Numbers: how many
    bool                      live;         // run in the engine when it changes
};

const std::vector<SettingSpec>& levelSettingSpecs();
const SettingSpec* findLevelSetting(const std::string& key);

// The value as the command takes it, from the file's (quotes off for the quoted kinds); and back.
std::string settingFromFile(const SettingSpec& spec, const std::string& fileValue);
std::string settingToFile(const SettingSpec& spec, const std::string& value);
// Whether `value` is one the setting takes; `error` says why not.
bool validSetting(const SettingSpec& spec, const std::string& value, std::string& error);

} // namespace bf2editor

#endif // BF2EDITOR_APP_LEVELSETTINGS_H
