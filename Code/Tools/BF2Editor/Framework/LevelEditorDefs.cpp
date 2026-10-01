// Code/Tools/BF2Editor/Framework/LevelEditorDefs.cpp - see LevelEditorDefs.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/LevelEditorDefs.h"

#include <cstring>
#include <fstream>
#include <sstream>

namespace bf2editor
{

namespace
{

// The line's words (blanks and tabs separate them; a trailing CR is dropped).
std::vector<std::string> wordsOf(const std::string& line)
{
    std::vector<std::string> words;
    std::string word;
    for (size_t i = 0; i <= line.size(); ++i)
    {
        const char c = i < line.size() ? line[i] : ' ';
        if (c == ' ' || c == '\t' || c == '\r')
        {
            if (!word.empty())
                words.push_back(word);
            word.clear();
        }
        else
            word += c;
    }
    return words;
}

std::vector<std::vector<std::string> > commandLines(const std::string& text, const char* command)
{
    std::vector<std::vector<std::string> > out;
    size_t start = 0;
    while (start < text.size())
    {
        size_t end = text.find('\n', start);
        if (end == std::string::npos)
            end = text.size();
        const std::vector<std::string> words = wordsOf(text.substr(start, end - start));
        start = end + 1;
        if (!words.empty() && _stricmp(words[0].c_str(), command) == 0)
            out.push_back(words);
    }
    return out;
}

bool readFile(const std::string& path, std::string& text)
{
    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    if (!in)
        return false;
    std::ostringstream bytes;
    bytes << in.rdbuf();
    text = bytes.str();
    return true;
}

} // namespace

void LevelEditorDefs::parseSpawnerDefs(const std::string& text, std::vector<std::string>& problems)
{
    m_presets.clear();
    const std::vector<std::vector<std::string> > lines = commandLines(text, "ObjectSpawnerDef.AddDef");
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (lines[i].size() < 4)
        {
            problems.push_back("ObjectSpawnerDef.AddDef needs a name and two templates: '"
                               + (lines[i].size() > 1 ? lines[i][1] : std::string()) + "' skipped");
            continue;
        }
        SpawnerPreset preset;
        preset.name = lines[i][1];
        preset.team1 = lines[i][2];
        preset.team2 = lines[i][3];
        m_presets.push_back(preset);
    }
}

void LevelEditorDefs::parseTeams(const std::string& text, std::vector<std::string>& problems)
{
    m_teams.clear();
    const std::vector<std::vector<std::string> > lines = commandLines(text, "LevelSettings.AddTeam");
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (lines[i].size() < 3)
        {
            problems.push_back("LevelSettings.AddTeam needs a team and its language: '"
                               + (lines[i].size() > 1 ? lines[i][1] : std::string()) + "' skipped");
            continue;
        }
        TeamDefinition team;
        team.name = lines[i][1];
        team.language = lines[i][2];
        m_teams.push_back(team);
    }
}

void LevelEditorDefs::load(const std::string& modName, std::vector<std::string>& problems)
{
    const std::string folder = "bf2editor/mods/" + modName + "/LevelEditor/";
    std::string text;
    m_presets.clear();
    m_teams.clear();
    if (readFile(folder + "ObjectSpawnerDef.con", text))
        parseSpawnerDefs(text, problems);
    if (readFile(folder + "LevelSettings.con", text))
        parseTeams(text, problems);
}

const SpawnerPreset* LevelEditorDefs::findPreset(const std::string& name) const
{
    for (size_t i = 0; i < m_presets.size(); ++i)
        if (_stricmp(m_presets[i].name.c_str(), name.c_str()) == 0)
            return &m_presets[i];
    return 0;
}

const TeamDefinition* LevelEditorDefs::findTeam(const std::string& name) const
{
    for (size_t i = 0; i < m_teams.size(); ++i)
        if (_stricmp(m_teams[i].name.c_str(), name.c_str()) == 0)
            return &m_teams[i];
    return 0;
}

} // namespace bf2editor
