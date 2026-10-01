// Code/Tools/BF2Editor/App/ResourceBrowser.h

// The resource browser (WP4 part 2, E3): the mod's content as the engine'S file system sees it -
// the mounted archives (Objects, Common, Menu, Fonts, Shaders) layered over the mod folders - listed
// lazily, one folder at a time, through io::FileManager::findFiles (+0x34). An object template's .con
// file under Objects/ can be dragged onto the 3D view to place one (WP6 part 2, ViewportTools). Editor-
// only code (D3).
#ifndef BF2EDITOR_APP_RESOURCEBROWSER_H
#define BF2EDITOR_APP_RESOURCEBROWSER_H

#include "Framework/EditorConfig.h"

#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace bf2editor
{

class EditorSettings;

class ResourceBrowser
{
public:
    ResourceBrowser();

    // Bookmarked files and the templates placed last, in the editor's settings
    // (resources.bookmarks, resources.recent: paths joined by ';').
    void loadSettings(const EditorSettings& settings);
    void saveSettings(EditorSettings& settings) const;
    // A template file was placed from the panel: it goes to the top of the recent list.
    void noteUsed(const std::string& path);

    // Draws the "Resources" window (between ImGuiHost::beginFrame and endFrame).
    void draw(bool* open);

    // Adds a root folder (e.g. the open level's directory) below the archive roots.
    void addRoot(const std::string& path);

    const std::string& selected() const { return m_selected; }

    // The drag-and-drop payload of a template file: its engine path, NUL-terminated.
    static const char* templatePayloadType() { return "BF2E_TEMPLATE_CON"; }
    // The template a file defines, named after the file as the Objects tree names them:
    // "Objects/.../barrel_yellow/barrel_yellow.con" -> "barrel_yellow"; empty for anything that is not
    // a .con file under Objects/.
    static std::string templateOf(const std::string& path);

private:
    struct Node
    {
        std::string       path;         // engine path, no trailing slash
        std::string       name;
        bool              isDirectory;
        unsigned int      size;
        bool              listed;
        std::vector<Node> children;
    };

    struct NoCase
    {
        bool operator()(const std::string& a, const std::string& b) const { return _stricmp(a.c_str(), b.c_str()) < 0; }
    };
    struct BuildNode
    {
        std::map<std::string, BuildNode, NoCase>    dirs;
        std::map<std::string, unsigned int, NoCase> files;
    };

    void list(Node& node);
    static void flatten(const BuildNode& from, Node& to);
    void drawNode(Node& node);
    // One file row: selectable, a drag source for template files, and a context menu to bookmark it.
    void drawFile(const std::string& path, const std::string& label, unsigned int size);
    // The filter across every root (each is listed on first use): the matching files' paths.
    void search(Node& node, const std::string& needle, std::vector<const Node*>& found) const;
    static Node makeRoot(const std::string& path);

    std::vector<Node>        m_roots;
    std::string              m_selected;
    char                     m_filter[96];
    std::vector<std::string> m_bookmarks;
    std::vector<std::string> m_recent;
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_RESOURCEBROWSER_H
