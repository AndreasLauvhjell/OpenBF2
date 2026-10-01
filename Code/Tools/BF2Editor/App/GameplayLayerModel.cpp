// Code/Tools/BF2Editor/App/GameplayLayerModel.cpp - see GameplayLayerModel.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/GameplayLayerModel.h"

#include "Framework/EditorLog.h"
#include "Framework/SourceControl.h"

#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <set>
#include <sstream>

namespace bf2editor
{

namespace
{

bool readWhole(const std::string& path, std::string& text)
{
    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    if (!in)
        return false;
    std::ostringstream buffer;
    buffer << in.rdbuf();
    text = buffer.str();
    return true;
}

bool isFile(const std::string& path)
{
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

// A child of `folder` as it is spelled on disk ("" when there is none).
std::string childOnDisk(const std::string& folder, const std::string& name)
{
    WIN32_FIND_DATAA found;
    const HANDLE search = FindFirstFileA((folder + name).c_str(), &found);
    if (search == INVALID_HANDLE_VALUE)
        return std::string();
    FindClose(search);
    return folder + found.cFileName;
}

std::vector<std::string> subfolders(const std::string& folder)
{
    std::vector<std::string> names;
    WIN32_FIND_DATAA found;
    const HANDLE search = FindFirstFileA((folder + "/*").c_str(), &found);
    if (search == INVALID_HANDLE_VALUE)
        return names;
    do
    {
        const std::string name = found.cFileName;
        if ((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && name != "." && name != "..")
            names.push_back(name);
    } while (FindNextFileA(search, &found));
    FindClose(search);
    return names;
}

// Whole, to a temporary file beside it, then over the old one (or new) in one step; read back.
bool writeFile(const std::string& path, const std::string& text, std::string& error)
{
    const bool existed = isFile(path);
    if (existed)
    {
        const ScmResult edit = sourceControl().openForEdit(path);
        if (!edit.ok)
        {
            error = "source control refused " + path + ": " + edit.message;
            return false;
        }
    }
    const std::string temporary = path + ".editor-tmp";
    {
        std::ofstream out(temporary.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        if (!out)
        {
            error = "cannot write " + temporary;
            return false;
        }
    }
    if (!MoveFileExA(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        DeleteFileA(temporary.c_str());
        error = "cannot replace " + path;
        return false;
    }
    std::string back;
    if (!readWhole(path, back) || back != text)
    {
        error = path + " does not hold what was written";
        return false;
    }
    if (!existed)
    {
        const ScmResult added = sourceControl().add(path);
        if (!added.ok)
            logLine(LogLevel::Warning, "layers: source control did not add %s: %s", path.c_str(), added.message.c_str());
    }
    return true;
}

} // namespace

void GameplayLayerModel::close()
{
    *this = GameplayLayerModel();
}

const GameplayLayer* GameplayLayerModel::layer(int id) const
{
    for (size_t i = 0; i < m_layers.size(); ++i)
        if (m_layers[i].id == id)
            return &m_layers[i];
    return 0;
}

int GameplayLayerModel::freeLayerId() const
{
    for (int id = 2; id < 20; ++id)
        if (!layer(id))
            return id;
    return -1;
}

bool GameplayLayerModel::open(const std::string& levelDirectory, const std::string& mod, const std::string& gameMode,
                              int layerSize, std::string& report)
{
    close();
    m_levelDirectory = levelDirectory;
    // The mod's filters: the retail editor's LayerInit.con beside the game, else stock bf2's.
    std::string init;
    m_filters = GameplayFilters::defaults();
    if (readWhole("bf2editor/mods/" + mod + "/LayerInit.con", init) && !m_filters.parse(init))
        m_filters = GameplayFilters::defaults();
    // The game-mode files on disk ("gamemodes" in stock levels).
    std::vector<GameplayBucket> buckets;
    const std::string modes = childOnDisk(levelDirectory, "GameModes");
    if (!modes.empty())
    {
        const std::vector<std::string> modeNames = subfolders(modes);
        for (size_t m = 0; m < modeNames.size(); ++m)
        {
            const std::vector<std::string> sizes = subfolders(modes + "/" + modeNames[m]);
            for (size_t s = 0; s < sizes.size(); ++s)
            {
                GameplayBucket bucket;
                bucket.mode = modeNames[m];
                bucket.players = sizes[s];
                const std::string file = modes + "/" + modeNames[m] + "/" + sizes[s] + "/GamePlayObjects.con";
                if (!isFile(file) || !readWhole(file, bucket.text))
                    continue;
                buckets.push_back(bucket);
                m_existing.push_back(std::make_pair(bucket.mode, bucket.players));
                m_filters.addMode(bucket.mode, std::string());
                m_filters.addPlayers(bucket.players);
            }
        }
    }
    char sizeText[16];
    std::snprintf(sizeText, sizeof(sizeText), "%d", layerSize);
    std::string editorFolder = childOnDisk(levelDirectory, "Editor");
    if (editorFolder.empty())
        editorFolder = levelDirectory + "Editor";
    m_editorPath = childOnDisk(editorFolder + "/", "GamePlayObjects.con");
    if (m_editorPath.empty())
        m_editorPath = editorFolder + "/GamePlayObjects.con";
    std::string error;
    if (isFile(m_editorPath) && readWhole(m_editorPath, m_editorText))
    {
        // The retail editor's own file, or this editor's.
        GameplaySet set;
        if (!set.parse(m_editorText, error))
        {
            report = m_editorPath + " is not in the retail layout (" + error + "): the gameplay layers are off";
            return false;
        }
        m_editorFileExists = true;
        std::string layers;
        const std::string layerPath = childOnDisk(editorFolder + "/", "Layer.con");
        if (!layerPath.empty() && readWhole(layerPath, layers) && !parseLayerFile(layers, m_layers, error))
        {
            report = layerPath + ": " + error + ": the gameplay layers are off";
            m_layers.clear();
            return false;
        }
        // The Default is never written; it has every filter.
        GameplayLayer defaults;
        defaults.name = "Default";
        defaults.modes = m_filters.modes;
        defaults.players = m_filters.players;
        m_layers.insert(m_layers.begin(), defaults);
        m_savedLayerText = layers;
        m_active = true;
        char text[200];
        std::snprintf(text, sizeof(text), "gameplay layers: the editor file %s with %u layers (%u objects)", m_editorPath.c_str(),
                      static_cast<unsigned>(m_layers.size()), static_cast<unsigned>(set.objects().size()));
        report = text;
        return true;
    }
    // No editor file: the game-mode files imported, when the loaded one comes back byte for byte.
    GameplayImport imported;
    importBuckets(buckets, m_filters, imported);
    m_notes = imported.notes;
    m_unmanaged = imported.unmanaged;
    bool loadedKept = false;
    for (size_t i = 0; i < imported.managed.size(); ++i)
        loadedKept = loadedKept || (_stricmp(imported.managed[i].first.c_str(), gameMode.c_str()) == 0
                                    && imported.managed[i].second == sizeText);
    if (!loadedKept)
    {
        report = "gameplay layers off: the game-mode file " + gameMode + " " + sizeText
               + " is not among those the import reproduces" + (imported.notes.empty() ? std::string() : " (" + imported.notes[0] + ")")
               + "; it is edited as it stands";
        return false;
    }
    m_editorText = imported.set.render(std::set<int>(), true);
    m_layers = imported.layers;
    // The import is what the files on disk hold: no change until an edit (writeGenerated makes Layer.con with the
    // editor file).
    m_savedLayerText = renderLayerFile(m_layers);
    m_active = true;
    char text[240];
    std::snprintf(text, sizeof(text), "gameplay layers: %u game-mode files imported into %u layers (%u objects); %u left as they are",
                  static_cast<unsigned>(imported.managed.size()), static_cast<unsigned>(m_layers.size()),
                  static_cast<unsigned>(imported.set.objects().size()), static_cast<unsigned>(imported.unmanaged.size()));
    report = text;
    return true;
}

std::vector<std::string> GameplayLayerModel::missingObjectLines(const std::function<bool(const std::string&, bool)>& present) const
{
    std::vector<std::string> lines;
    GameplaySet set;
    std::string error;
    if (!m_active || !set.parse(m_editorText, error))
        return lines;
    const std::vector<GameplayObject>& objects = set.objects();
    std::vector<bool> missing(objects.size());
    for (size_t i = 0; i < objects.size(); ++i)
        missing[i] = !present(objects[i].name, objects[i].section == GameplaySection::CombatArea);
    // Templates before instances, section by section, in the file's order.
    const GameplaySection order[3] = { GameplaySection::Spawner, GameplaySection::SpawnPoint, GameplaySection::ControlPoint };
    for (int k = 0; k < 3; ++k)
    {
        const int s = static_cast<int>(order[k]);
        for (size_t t = 0; t < set.templateOrder[s].size(); ++t)
            if (missing[static_cast<size_t>(set.templateOrder[s][t])])
            {
                const std::vector<std::string>& block = objects[static_cast<size_t>(set.templateOrder[s][t])].templateLines;
                lines.insert(lines.end(), block.begin(), block.end());
            }
        for (size_t t = 0; t < set.instanceOrder[s].size(); ++t)
            if (missing[static_cast<size_t>(set.instanceOrder[s][t])])
            {
                const std::vector<std::string>& block = objects[static_cast<size_t>(set.instanceOrder[s][t])].instanceLines;
                lines.insert(lines.end(), block.begin(), block.end());
            }
    }
    for (size_t t = 0; t < set.templateOrder[3].size(); ++t)
        if (missing[static_cast<size_t>(set.templateOrder[3][t])])
        {
            const std::vector<std::string>& block = objects[static_cast<size_t>(set.templateOrder[3][t])].instanceLines;
            lines.insert(lines.end(), block.begin(), block.end());
        }
    return lines;
}

bool GameplayLayerModel::writeGenerated(const std::string& editorText, std::string& report, int& written)
{
    report.clear();
    if (!m_active)
        return true;
    GameplaySet set;
    std::string error;
    if (!set.parse(editorText, error))
    {
        report = "the gameplay editor file is not in the retail layout (" + error + "): no game-mode file was written";
        return false;
    }
    std::string done;
    // An imported stock level writes no file of its own until its gameplay objects or its layers change: an untouched
    // save changes nothing, and the next open imports the same. Then the editor file and Layer.con are written
    // together, so every layer id in the one is named in the other.
    const bool create = !m_editorFileExists && (editorText != m_editorText || layerFileText() != m_savedLayerText);
    if (create || (m_editorFileExists && editorText != m_editorText))
    {
        CreateDirectoryA(m_editorPath.substr(0, m_editorPath.rfind('/')).c_str(), 0);
        if (!isFile(m_editorPath) || editorText != m_editorText)
        {
            std::string current;
            if (!isFile(m_editorPath) || !readWhole(m_editorPath, current) || current != editorText)
            {
                if (!writeFile(m_editorPath, editorText, report))
                    return false;
                ++written;
                done += (done.empty() ? "" : ", ") + m_editorPath.substr(m_levelDirectory.size());
            }
        }
        m_editorFileExists = true;
        m_editorText = editorText;
    }
    // Every game-mode file the layers make: the universe of filters and the files there are, never one left.
    std::vector<std::pair<std::string, std::string> > buckets;
    for (size_t m = 0; m < m_filters.modes.size(); ++m)
        for (size_t p = 0; p < m_filters.players.size(); ++p)
            buckets.push_back(std::make_pair(m_filters.modes[m], m_filters.players[p]));
    std::string modesFolder = childOnDisk(m_levelDirectory, "GameModes");
    if (modesFolder.empty())
        modesFolder = m_levelDirectory + "GameModes";
    for (size_t b = 0; b < buckets.size(); ++b)
    {
        bool left = false;
        for (size_t u = 0; u < m_unmanaged.size() && !left; ++u)
            left = _stricmp(m_unmanaged[u].first.c_str(), buckets[b].first.c_str()) == 0 && m_unmanaged[u].second == buckets[b].second;
        if (left)
            continue;
        std::string modeFolder = childOnDisk(modesFolder + "/", buckets[b].first);
        if (modeFolder.empty())
            modeFolder = modesFolder + "/" + buckets[b].first;
        std::string sizeFolder = childOnDisk(modeFolder + "/", buckets[b].second);
        if (sizeFolder.empty())
            sizeFolder = modeFolder + "/" + buckets[b].second;
        std::string path = childOnDisk(sizeFolder + "/", "GamePlayObjects.con");
        if (path.empty())
            path = sizeFolder + "/GamePlayObjects.con";
        const bool exists = isFile(path);
        if (!bucketWritten(set, m_layers, buckets[b].first, buckets[b].second, exists))
            continue;
        const std::string text = renderBucket(set, m_layers, buckets[b].first, buckets[b].second);
        std::string current;
        if (exists && readWhole(path, current) && current == text)
            continue;
        CreateDirectoryA(modesFolder.c_str(), 0);
        CreateDirectoryA(modeFolder.c_str(), 0);
        CreateDirectoryA(sizeFolder.c_str(), 0);
        if (!writeFile(path, text, report))
            return false;
        ++written;
        done += (done.empty() ? "" : ", ") + path.substr(m_levelDirectory.size());
    }
    // Layer.con, when the layers changed or the editor file was made.
    const std::string layers = layerFileText();
    if (layers != m_savedLayerText || create)
    {
        const std::string folder = m_editorPath.substr(0, m_editorPath.rfind('/'));
        std::string path = childOnDisk(folder + "/", "Layer.con");
        if (path.empty())
            path = folder + "/Layer.con";
        if (!writeFile(path, layers, report))
            return false;
        m_savedLayerText = layers;
        ++written;
        done += (done.empty() ? "" : ", ") + path.substr(m_levelDirectory.size());
    }
    if (!done.empty())
        report = "gameplay layers wrote " + done;
    return true;
}

} // namespace bf2editor
