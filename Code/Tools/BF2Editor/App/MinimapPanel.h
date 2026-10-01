// Displays the level minimap.dds, falling back to Hud/Minimap/ingameMap.dds through the engine filesystem.
// Marks camera and selection; clicking or dragging moves the camera while preserving ground clearance.
// The map covers the primary heightmap, with +Z up and +X right.
#ifndef BF2EDITOR_APP_MINIMAPPANEL_H
#define BF2EDITOR_APP_MINIMAPPANEL_H

#include "Framework/EditorConfig.h"

#include <string>

struct IDirect3DDevice9;
struct IDirect3DTexture9;

namespace bf2editor
{

// Where world (x, z) is drawn on the Minimap panel as it was last drawn (window pixels); false when the panel has
// not drawn its map (the test harness's EditorTest.mouseToMinimap).
bool minimapScreenOf(float x, float z, float& screenX, float& screenY);

class EditSession;

class MinimapPanel
{
public:
    MinimapPanel();
    ~MinimapPanel();

    // The map of `level`, covering minX..maxX by minZ..maxZ in the world. False, with the reason kept
    // for the panel and logged, when there is none to show.
    bool load(IDirect3DDevice9* device, const std::string& level, const float bounds[4]);
    void release();

    // The "Minimap" window.
    void draw(bool* open, EditSession& session);

private:
    MinimapPanel(const MinimapPanel&);
    MinimapPanel& operator=(const MinimapPanel&);

    bool loadFile(const std::string& path);

    IDirect3DTexture9* m_texture;
    int                m_width;
    int                m_height;
    float              m_bounds[4];     // min X, min Z, max X, max Z
    std::string        m_status;        // what the panel says when there is no map
    std::string        m_shown;         // the file shown
    IDirect3DDevice9*  m_device;        // for a reload after a capture
    std::string        m_level;
    unsigned           m_version;       // EditSession::minimapVersion at the last load
    // A terrain brush stroke painted through the map: on while the button is held.
    bool               m_painting = false;
    bool               m_paintSecondary = false;
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_MINIMAPPANEL_H
