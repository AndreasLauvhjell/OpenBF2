// Code/Tools/BF2Editor/App/LibraryPanel.h

// The "Bundles and brushes" panel: the library folder, the
// object bundles (saved from the selection, placed under the middle of the view or where the next click
// in the view lands, turned and grouped as asked), the selection's group, and the level brushes (defined
// from the selection, their settings and items, the view's paint tool, the erase radius). Every action
// runs its console line (EditorBundle.*, EditorBrush.*, EditorSelection.group*), so it is logged, and the
// level edits are undoable; brush edits write the brush's file at once. Editor-only code (D3).
#ifndef BF2EDITOR_APP_LIBRARYPANEL_H
#define BF2EDITOR_APP_LIBRARYPANEL_H

#include "Framework/EditorConfig.h"

#include <string>
#include <vector>

namespace bf2editor
{

class EditSession;

class LibraryPanel
{
public:
    LibraryPanel();

    void draw(EditSession& session, bool* open);

private:
    void drawBundles(EditSession& session);
    void drawBrushes(EditSession& session);
    // A console line, its answer logged and kept as the panel's last message.
    bool run(const std::string& line, const char* what);

    std::vector<std::string> m_bundles;         // the bundle folder, listed again on Refresh and after a save
    bool                     m_bundlesListed;
    std::string              m_selectedBundle;
    std::string              m_bundleInfo;
    float                    m_bundleYaw;
    char                     m_bundleName[80];
    char                     m_brushName[80];
    char                     m_itemTemplate[128];
    int                      m_seed;
    std::string              m_message;
    bool                     m_messageFailed;
    std::string              m_confirmDelete;   // "bundle:<name>" or "brush:<name>" waiting for a yes
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_LIBRARYPANEL_H
