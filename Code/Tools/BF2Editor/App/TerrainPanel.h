// Code/Tools/BF2Editor/App/TerrainPanel.h

// The Terrain panel: the view's brush tool and the settings the retail
// TerrainEditor plugin's brush panel had - the five height brushes, size 1..50 (times the primary tile's vertex spacing), strength
// 1..100, the Raise and Lower switches and the Platform height - plus the heightmap cluster, whether
// its heights changed, and what this version cannot do with them yet. The brush settings are the edit
// session's (EditorTerrain.brush sets the same ones). Below them, the surrounding terrain (census §10):
// the 3x3 sectors - a surrounding tile is saved with the level while it is active - a surrounding
// heightmap imported from an 8-bit raw, and the primary tile's edges smoothed. Editor-only code (D3).
#ifndef BF2EDITOR_APP_TERRAINPANEL_H
#define BF2EDITOR_APP_TERRAINPANEL_H

#include "Framework/EditorConfig.h"

#include <string>
#include <utility>
#include <vector>

namespace bf2editor
{

class EditSession;

class TerrainPanel
{
public:
    TerrainPanel();

    void draw(EditSession& session, bool* open);

private:
    // changedSinceSave compares every tile with its saved copy; it is asked again only after the
    // history moved (every stroke, undo and redo moves it).
    unsigned m_changedRevision;
    bool     m_changedKnown;
    bool     m_changed;
    std::string m_describe;
    char     m_importPath[512];     // the surrounding heightmap to import
    int      m_smoothWidth;         // smoothEdges' arguments (the retail button's 32 0.5 2)
    float    m_smoothAmplitude;
    float    m_smoothFalloff;
    // The level's overgrowth materials ("name id"), asked once while the Overgrowth brush is shown.
    std::vector<std::pair<std::string, int> > m_overgrowthMaterials;
    bool     m_overgrowthKnown = false;
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_TERRAINPANEL_H
