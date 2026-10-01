// Lighting and minimap bakes invoke renderer commands and write files immediately, outside undo (RW-273).
// Minimap capture disables shadow updates to avoid window-sized depth clipping.
// Environment saves stage files before copying them into the level; the 1.50 EnvMapTemplate factory
// returns null, so it currently produces no environment objects. Remove only the bake's temporary lightingTex.dds.
#include "Framework/EditorConfig.h"
#include "App/EditSession.h"

#include "App/EditorLevel.h"
#include "App/LevelDocument.h"
#include "Framework/EditorLog.h"

#include "Core/SettingsRepostitory.h"
#include "Core/SmartItf.h"
#include "Geom/XpLightManager.h"
#include "IO/Console/Console.h"

#include <windows.h>

#include <cstdio>
#include <vector>

namespace bf2editor
{

namespace
{

// One engine console line with full access; its answer, trimmed (EditSession.cpp's engineLine).
std::string engineLine(const std::string& line)
{
    std::string answer;
    dice::hfe::io::mainConsole->executeLine(line + "\n", answer, false, false, false, false, true);
    const size_t begin = answer.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return std::string();
    return answer.substr(begin, answer.find_last_not_of(" \t\r\n") - begin + 1);
}

bool fileThere(const std::string& path)
{
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// Every file under root/relative, as paths relative to root.
void listFiles(const std::string& root, const std::string& relative, std::vector<std::string>& out)
{
    WIN32_FIND_DATAA found;
    const std::string pattern = root + "/" + (relative.empty() ? std::string() : relative + "/") + "*";
    HANDLE h = FindFirstFileA(pattern.c_str(), &found);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do
    {
        const std::string name = found.cFileName;
        if (name == "." || name == "..")
            continue;
        const std::string path = relative.empty() ? name : relative + "/" + name;
        if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            listFiles(root, path, out);
        else
            out.push_back(path);
    } while (FindNextFileA(h, &found));
    FindClose(h);
}

void makeFolders(const std::string& path)
{
    for (size_t i = 0; i < path.size(); ++i)
        if (path[i] == '/' || path[i] == '\\')
            CreateDirectoryA(path.substr(0, i).c_str(), 0);
}

} // namespace

bool EditSession::lightingLevel(std::string& levelDir, std::string& levelPath, std::string& error) const
{
    if (!m_document || !m_document->isOpen() || !dice::hfe::io::mainConsole)
    {
        error = "no level is open";
        return false;
    }
    // "mods/bf2/Levels/<name>/" and the level path the engine's files name ("Levels/<name>").
    levelDir = m_document->levelDirectory();
    const size_t levels = levelDir.find("Levels/");
    if (levels == std::string::npos)
    {
        error = "cannot find Levels/ in " + levelDir;
        return false;
    }
    levelPath = levelDir.substr(levels);
    if (!levelPath.empty() && levelPath[levelPath.size() - 1] == '/')
        levelPath.erase(levelPath.size() - 1);
    if (!levelDir.empty() && levelDir[levelDir.size() - 1] != '/')
        levelDir += '/';
    return true;
}

bool EditSession::bakeGroundHemi(int resolution, const dice::hfe::Vec3& position, float size, std::string& result)
{
    std::string levelDir, levelPath;
    if (!lightingLevel(levelDir, levelPath, result))
        return false;
    if (resolution < 16 || resolution > 4096 || size <= 0.0f)
    {
        result = "a hemi map is 16 to 4096 pixels square and covers a positive size";
        return false;
    }
    const std::string file = levelDir + "groundhemi.dds";
    const bool hadLightingTexture = fileThere("lightingTex.dds");
    char line[512];
    std::snprintf(line, sizeof(line), "hemiMapManager.makeHemiMap %d %s %g/%g/%g %g", resolution, file.c_str(), position.x,
                  position.y, position.z, size);
    const std::string answer = engineLine(line);
    if (!hadLightingTexture && fileThere("lightingTex.dds"))
        DeleteFileA("lightingTex.dds");
    if (answer != "1")
    {
        result = "the hemi bake failed (hemiMapManager.makeHemiMap answered '" + answer + "')";
        return false;
    }
    const std::string shown = engineLine("lightManager.groundHemi " + levelPath + "/groundhemi");
    std::snprintf(line, sizeof(line), " (%dx%d, from %g/%g/%g over %g m)", resolution, resolution, position.x, position.y,
                  position.z, size);
    result = "baked " + file + line + (shown.empty() ? "; the view shows it" : "; lightManager.groundHemi answered '" + shown + "'")
           + " (written now: not an undoable edit)";
    logLine(LogLevel::Info, "lighting: %s", result.c_str());
    return true;
}

bool EditSession::captureMinimap(int resolution, std::string& result)
{
    std::string levelDir, levelPath;
    if (!lightingLevel(levelDir, levelPath, result))
        return false;
    float bounds[4];
    if (!m_level.worldBounds(bounds))
    {
        result = "the level has no terrain extent to capture";
        return false;
    }
    if (resolution < 16 || resolution > 4096)
    {
        result = "a minimap is 16 to 4096 pixels square";
        return false;
    }
    const float size = bounds[2] - bounds[0];
    const std::string file = levelDir + "minimap";
    char line[512];
    std::snprintf(line, sizeof(line), "renderer.makeTopWorldScreen %d %s 0/500/0 %g", resolution, file.c_str(), size);
    // The shadow-map pass hands the renderer's own (window-sized) depth-stencil back in the middle of the
    // frame (RW-273), so a capture taller than the window keeps only the window's rows. With the light
    // manager's shadow updates off for the capture - as retail turned the sky dome off around its own -
    // that pass does not run and the whole square is drawn; they are put back after.
    dice::hfe::geom::LightManager* const lights =
        dice::hfe::classManager ? dice::hfe::geom::asLightManager(dice::hfe::classManager->getSingleton(std::string("LightManager"))) : 0;
    const bool shadows = lights && lights->getShadowUpdatesEnabled();
    if (shadows)
        lights->setShadowUpdatesEnabled(false);
    const std::string answer = engineLine(line);
    if (shadows)
        lights->setShadowUpdatesEnabled(true);
    if (!answer.empty() || !fileThere(file + ".dds"))
    {
        result = "the capture wrote no " + file + ".dds (renderer.makeTopWorldScreen answered '" + answer + "')";
        return false;
    }
    ++m_minimapVersion;
    std::snprintf(line, sizeof(line), " (the top-down view over %g m; the sky dome is not turned off: the rebuilt "
                  "RendDX9 binds no renderer.drawSkyDome)", size);
    result = "captured " + file + ".dds" + line + " (written now: not an undoable edit)";
    logLine(LogLevel::Info, "lighting: %s", result.c_str());
    return true;
}

bool EditSession::generateEnvMaps(std::string& result)
{
    std::string levelDir, levelPath;
    if (!lightingLevel(levelDir, levelPath, result))
        return false;
    if (!dice::hfe::settings)
    {
        result = "no settings repository";
        return false;
    }
    const std::string generated = engineLine("envmapManager.generateEnvMaps");
    if (!generated.empty())
    {
        result = "envmapManager.generateEnvMaps answered '" + generated + "'";
        return false;
    }
    // EnvMapManager::save writes <Editor_LevelSavePath>/<level path>/Envmaps/...: a staging folder for the call.
    char temp[MAX_PATH];
    GetTempPathA(MAX_PATH, temp);
    char stamp[32];
    std::snprintf(stamp, sizeof(stamp), "bf2editor_envmaps_%lu", static_cast<unsigned long>(GetCurrentProcessId()));
    std::string staging = std::string(temp) + stamp;
    for (size_t i = 0; i < staging.size(); ++i)
        if (staging[i] == '\\')
            staging[i] = '/';
    makeFolders(staging + "/" + levelPath + "/Envmaps/");
    const std::string key = "Editor_LevelSavePath";
    const std::string previous = dice::hfe::settings->stringGet(key, std::string());
    dice::hfe::settings->stringSet(key, staging);
    const std::string saved = engineLine("envmapManager.save");
    dice::hfe::settings->stringSet(key, previous);
    std::vector<std::string> written;
    listFiles(staging, "", written);
    int copied = 0;
    std::string failed;
    for (size_t i = 0; i < written.size(); ++i)
    {
        const std::string from = staging + "/" + written[i];
        // "Levels/<name>/Envmaps/x" -> "<levelDir>Envmaps/x"
        const std::string inLevel = written[i].size() > levelPath.size() + 1 ? written[i].substr(levelPath.size() + 1) : written[i];
        const std::string to = levelDir + inLevel;
        makeFolders(to);
        if (CopyFileA(from.c_str(), to.c_str(), FALSE))
            ++copied;
        else
            failed += (failed.empty() ? "" : ", ") + to;
        DeleteFileA(from.c_str());
    }
    if (written.empty())
    {
        // The save is a void row: an empty answer is its only one. In 1.50 EnvMapTemplate::createObject returns
        // no object (its shipped body is `return 0`), so no environment-map object ever reaches the manager
        // and EnvMapManager::save has none to write - retail's editor generated them on its own 2006 engine.
        result = "envmapManager.save wrote nothing: in 1.50 an environment-map template makes no object "
                 "(EnvMapTemplate::createObject returns null, as shipped), so the manager holds none to save"
               + (saved.empty() ? std::string() : " (it answered '" + saved + "')");
        return false;
    }
    if (copied == 0)
    {
        result = "envmapManager.save wrote files that could not be copied into " + levelDir + "Envmaps: " + failed;
        return false;
    }
    char count[96];
    std::snprintf(count, sizeof(count), "%d file%s", copied, copied == 1 ? "" : "s");
    result = std::string("generated the environment maps and wrote ") + count + " into " + levelDir + "Envmaps"
           + (failed.empty() ? std::string() : "; not copied: " + failed) + " (written now: not an undoable edit)";
    logLine(LogLevel::Info, "lighting: %s", result.c_str());
    return failed.empty();
}

} // namespace bf2editor
