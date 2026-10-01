// Code/Tools/BF2Editor/App/ResourceBrowser.cpp - see ResourceBrowser.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/ResourceBrowser.h"

#include "Framework/EditorConsole.h"
#include "Framework/EditorLog.h"
#include "Framework/EditorSettings.h"

#include "IO/File/FileManager.h"

#include "imgui.h"

#include <algorithm>
#include <cstring>
#include <list>

namespace bf2editor
{

namespace
{

// The archive mount points ServerArchives.con / ClientArchives.con create (mods/bf2).
const char* const kArchiveRoots[] = { "Objects", "Common", "Menu", "Fonts", "Shaders" };

const size_t kRecentLimit = 8;

std::vector<std::string> splitPaths(const std::string& text)
{
    std::vector<std::string> paths;
    size_t start = 0;
    while (start <= text.size())
    {
        size_t end = text.find(';', start);
        if (end == std::string::npos)
            end = text.size();
        if (end > start)
            paths.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    return paths;
}

std::string joinPaths(const std::vector<std::string>& paths)
{
    std::string text;
    for (size_t i = 0; i < paths.size(); ++i)
        text += (i ? ";" : "") + paths[i];
    return text;
}

std::string fileName(const std::string& path)
{
    const size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

bool containsIgnoreCase(const std::string& text, const char* needle)
{
    if (!needle || !*needle)
        return true;
    std::string a(text), b(needle);
    std::transform(a.begin(), a.end(), a.begin(), ::tolower);
    std::transform(b.begin(), b.end(), b.begin(), ::tolower);
    return a.find(b) != std::string::npos;
}

} // namespace

ResourceBrowser::ResourceBrowser()
{
    m_filter[0] = 0;
    for (size_t i = 0; i < sizeof(kArchiveRoots) / sizeof(kArchiveRoots[0]); ++i)
        m_roots.push_back(makeRoot(kArchiveRoots[i]));
}

ResourceBrowser::Node ResourceBrowser::makeRoot(const std::string& path)
{
    Node node;
    node.path = path;
    node.name = path;
    node.isDirectory = true;
    node.size = 0;
    node.listed = false;
    return node;
}

void ResourceBrowser::loadSettings(const EditorSettings& settings)
{
    m_bookmarks = splitPaths(settings.get("resources.bookmarks", std::string()));
    m_recent = splitPaths(settings.get("resources.recent", std::string()));
    if (m_recent.size() > kRecentLimit)
        m_recent.resize(kRecentLimit);
}

void ResourceBrowser::saveSettings(EditorSettings& settings) const
{
    settings.set("resources.bookmarks", joinPaths(m_bookmarks));
    settings.set("resources.recent", joinPaths(m_recent));
}

void ResourceBrowser::noteUsed(const std::string& path)
{
    for (size_t i = 0; i < m_recent.size(); ++i)
    {
        if (_stricmp(m_recent[i].c_str(), path.c_str()) == 0)
        {
            m_recent.erase(m_recent.begin() + static_cast<std::ptrdiff_t>(i));
            break;
        }
    }
    m_recent.insert(m_recent.begin(), path);
    if (m_recent.size() > kRecentLimit)
        m_recent.resize(kRecentLimit);
}

void ResourceBrowser::addRoot(const std::string& path)
{
    for (size_t i = 0; i < m_roots.size(); ++i)
    {
        if (_stricmp(m_roots[i].path.c_str(), path.c_str()) == 0)
            return;
    }
    m_roots.push_back(makeRoot(path));
}

// A zip archive's findFiles (IO/File/ZipArchive.cpp) returns files only - it has no directory
// entries, and its non-recursive mode lists just the files directly in the folder - so a root is
// listed once, recursively (as GameLogic::loadAllConFiles("Objects/") sweeps it), and the folder tree
// is built from the relative names. Loose folders come back the same way.
void ResourceBrowser::list(Node& node)
{
    node.listed = true;
    node.children.clear();
    if (!dice::hfe::io::fileManager)
        return;
    std::list<dice::hfe::io::FileInfo> files;
    dice::hfe::io::fileManager->findFiles(node.path + "/", true, files);

    // Build through ordered maps (the tree can hold tens of thousands of files), then flatten.
    BuildNode root;
    for (std::list<dice::hfe::io::FileInfo>::const_iterator it = files.begin(); it != files.end(); ++it)
    {
        if (it->isDirectory || it->name.empty())
            continue;
        BuildNode* parent = &root;
        std::string rest = it->name;
        size_t slash;
        while ((slash = rest.find_first_of("/\\")) != std::string::npos)
        {
            if (slash > 0)
                parent = &parent->dirs[rest.substr(0, slash)];
            rest = rest.substr(slash + 1);
        }
        if (!rest.empty())
            parent->files[rest] = it->size;
    }
    flatten(root, node);
}

void ResourceBrowser::flatten(const BuildNode& from, Node& to)
{
    to.children.clear();
    to.children.reserve(from.dirs.size() + from.files.size());
    for (std::map<std::string, BuildNode, NoCase>::const_iterator d = from.dirs.begin(); d != from.dirs.end(); ++d)
    {
        Node child;
        child.name = d->first;
        child.path = to.path + "/" + d->first;
        child.isDirectory = true;
        child.size = 0;
        child.listed = true;                        // filled by the same sweep
        to.children.push_back(child);
        flatten(d->second, to.children.back());
    }
    for (std::map<std::string, unsigned int, NoCase>::const_iterator f = from.files.begin(); f != from.files.end(); ++f)
    {
        Node child;
        child.name = f->first;
        child.path = to.path + "/" + f->first;
        child.isDirectory = false;
        child.size = f->second;
        child.listed = true;
        to.children.push_back(child);
    }
}

void ResourceBrowser::drawNode(Node& node)
{
    if (node.isDirectory)
    {
        const bool open = ImGui::TreeNodeEx(node.path.c_str(), ImGuiTreeNodeFlags_SpanAvailWidth, "%s",
                                            node.name.c_str());
        if (!open)
            return;
        if (!node.listed)
            list(node);
        if (node.children.empty())
            ImGui::TextDisabled("(empty)");
        for (size_t i = 0; i < node.children.size(); ++i)
            drawNode(node.children[i]);
        ImGui::TreePop();
        return;
    }
    drawFile(node.path, node.name, node.size);
}

void ResourceBrowser::drawFile(const std::string& path, const std::string& label, unsigned int size)
{
    ImGui::PushID(path.c_str());
    const bool isSelected = m_selected == path;
    if (ImGui::Selectable(label.c_str(), isSelected))
        m_selected = path;
    const std::string templateName = templateOf(path);
    if (!templateName.empty() && ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload(templatePayloadType(), path.c_str(), path.size() + 1);
        ImGui::Text("%s", templateName.c_str());
        ImGui::TextDisabled("drop it on the level to place one");
        ImGui::EndDragDropSource();
    }
    else if (ImGui::IsItemHovered())
    {
        const char* hint = templateName.empty() ? "\nright-click to bookmark"
                                                : "\ndrag it onto the level to place one; right-click to bookmark";
        if (size)
            ImGui::SetTooltip("%s\n%u bytes%s", path.c_str(), size, hint);
        else
            ImGui::SetTooltip("%s%s", path.c_str(), hint);
    }
    if (ImGui::BeginPopupContextItem("file"))
    {
        size_t marked = m_bookmarks.size();
        for (size_t i = 0; i < m_bookmarks.size(); ++i)
            if (_stricmp(m_bookmarks[i].c_str(), path.c_str()) == 0)
                marked = i;
        if (marked == m_bookmarks.size())
        {
            if (ImGui::MenuItem("Bookmark"))
                m_bookmarks.push_back(path);
        }
        else if (ImGui::MenuItem("Remove the bookmark"))
        {
            m_bookmarks.erase(m_bookmarks.begin() + static_cast<std::ptrdiff_t>(marked));
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
}

void ResourceBrowser::search(Node& node, const std::string& needle, std::vector<const Node*>& found) const
{
    for (size_t i = 0; i < node.children.size() && found.size() < 400; ++i)
    {
        const Node& child = node.children[i];
        if (child.isDirectory)
            search(node.children[i], needle, found);
        else if (containsIgnoreCase(child.name, needle.c_str()))
            found.push_back(&child);
    }
}

std::string ResourceBrowser::templateOf(const std::string& path)
{
    if (path.size() < 13 || _strnicmp(path.c_str(), "Objects/", 8) != 0)
        return std::string();
    const size_t dot = path.rfind('.');
    const size_t slash = path.find_last_of("/\\");
    if (dot == std::string::npos || _stricmp(path.c_str() + dot, ".con") != 0 || slash == std::string::npos
        || dot <= slash + 1)
        return std::string();
    return path.substr(slash + 1, dot - slash - 1);
}

void ResourceBrowser::draw(bool* open)
{
    if (!ImGui::Begin("Resources", open))
    {
        ImGui::End();
        return;
    }
    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputTextWithHint("##filter", "find a file by name", m_filter, sizeof(m_filter));
    if (ImGui::Button("Refresh"))
    {
        for (size_t i = 0; i < m_roots.size(); ++i)
            m_roots[i].listed = false;
    }
    // The selected template previewed in the level view.
    ImGui::SameLine();
    const std::string selectedTemplate = templateOf(m_selected);
    if (ImGui::Button("Preview") && !selectedTemplate.empty())
    {
        const ConsoleLineResult shown = runConsoleLine("EditorPreview.show " + selectedTemplate);
        if (!shown.failed)
            runConsoleLine("EditorPreview.frame");
        logLine(shown.failed ? LogLevel::Warning : LogLevel::Info, "preview: %s", shown.output.c_str());
    }
    ImGui::SetItemTooltip("Show the selected template beside the level and look at it (EditorPreview).");
    ImGui::SameLine();
    ImGui::TextDisabled("%s", m_selected.empty() ? "(nothing selected)" : m_selected.c_str());
    ImGui::Separator();
    ImGui::BeginChild("tree");
    if (m_filter[0])
    {
        // A search: every root, listed on first use, flattened to the files whose names match.
        std::vector<const Node*> found;
        for (size_t i = 0; i < m_roots.size() && found.size() < 400; ++i)
        {
            if (!m_roots[i].listed)
                list(m_roots[i]);
            search(m_roots[i], m_filter, found);
        }
        ImGui::TextDisabled(found.size() >= 400 ? "the first 400 matches" : "%u matches",
                            static_cast<unsigned>(found.size()));
        for (size_t i = 0; i < found.size(); ++i)
            drawFile(found[i]->path, found[i]->path, found[i]->size);
    }
    else
    {
        if (!m_bookmarks.empty() && ImGui::CollapsingHeader("Bookmarks", ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (size_t i = 0; i < m_bookmarks.size(); ++i)
                drawFile(m_bookmarks[i], fileName(m_bookmarks[i]), 0);
        }
        if (!m_recent.empty() && ImGui::CollapsingHeader("Recently placed", ImGuiTreeNodeFlags_DefaultOpen))
        {
            for (size_t i = 0; i < m_recent.size(); ++i)
                drawFile(m_recent[i], fileName(m_recent[i]), 0);
        }
        for (size_t i = 0; i < m_roots.size(); ++i)
        {
            if (i == 0)
                ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);      // Objects opens on first use
            drawNode(m_roots[i]);
        }
    }
    ImGui::EndChild();
    ImGui::End();
}

} // namespace bf2editor
