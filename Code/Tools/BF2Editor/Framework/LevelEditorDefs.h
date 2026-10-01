// Code/Tools/BF2Editor/Framework/LevelEditorDefs.h

// The retail level editor's per-mod lists: the retail
// editor reads bf2editor/mods/<mod>/LevelEditor/ObjectSpawnerDef.con, whose `ObjectSpawnerDef.AddDef <name>
// <team 1 template> <team 2 template>` lines name vehicle pairs a spawner can be given in one step, and
// LevelSettings.con, whose `LevelSettings.AddTeam <name> <language>` lines are the teams a level can have.
// They are read from the user's own retail install. Engine-free. Editor-only code (D3).
#ifndef BF2EDITOR_FRAMEWORK_LEVELEDITORDEFS_H
#define BF2EDITOR_FRAMEWORK_LEVELEDITORDEFS_H

#include "Framework/EditorConfig.h"

#include <string>
#include <vector>

namespace bf2editor
{

struct SpawnerPreset
{
    std::string name;           // "ME_US_APC"
    std::string team1;          // its team 1 template
    std::string team2;          // its team 2 template
};

struct TeamDefinition
{
    std::string name;           // "US"
    std::string language;       // "English"
};

class LevelEditorDefs
{
public:
    // The two files' text. Lines other than the two commands (rem, blank, anything else) are skipped;
    // a command with too few words is reported in `problems` and skipped. Command names are compared
    // without case, as the console does.
    void parseSpawnerDefs(const std::string& text, std::vector<std::string>& problems);
    void parseTeams(const std::string& text, std::vector<std::string>& problems);
    // Both files of a mod: bf2editor/mods/<mod>/LevelEditor/ObjectSpawnerDef.con and LevelSettings.con.
    // A missing file leaves its list empty (no problem reported).
    void load(const std::string& modName, std::vector<std::string>& problems);

    const std::vector<SpawnerPreset>& spawnerPresets() const { return m_presets; }
    const std::vector<TeamDefinition>& teams() const { return m_teams; }
    const SpawnerPreset* findPreset(const std::string& name) const;      // without case
    const TeamDefinition* findTeam(const std::string& name) const;       // without case

private:
    std::vector<SpawnerPreset>  m_presets;
    std::vector<TeamDefinition> m_teams;
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_LEVELEDITORDEFS_H
