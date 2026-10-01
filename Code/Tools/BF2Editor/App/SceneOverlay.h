// Code/Tools/BF2Editor/App/SceneOverlay.h

// Draws the editor's ImGui frame inside Renderer::renderFrame's scene (WP4 part 2, a level open).
// renderFrame posts eventManager->postEvent(ECCore, 5 /*CEDrawDebugCounters*/, 0, 0.0f) with zero delay
// after the world, first-person and HUD draws and before the console draw, endScene and present
// (RendDX9/BF2Render.cpp, the frame-event posts). Delay-zero events are dispatched synchronously, so
// a listener on ECCore draws on the frame's own back buffer, inside its BeginScene/EndScene, and the
// frame keeps its real time. No RendDX9 change. Editor-only code (D3).
#ifndef BF2EDITOR_APP_SCENEOVERLAY_H
#define BF2EDITOR_APP_SCENEOVERLAY_H

#include "Framework/EditorConfig.h"

#include "Game/IEventListener.h"

#include <string>

struct IDirect3DDevice9;

namespace bf2editor
{

class FrozenFrame;
class ImGuiHost;

class SceneOverlay : public dice::hfe::IEventListener
{
public:
    SceneOverlay(ImGuiHost& imgui, IDirect3DDevice9* device);
    ~SceneOverlay();

    bool attach();      // registers on the engine's event manager (ECCore)
    void detach();

    // Before renderFrame: draw this frame's UI when the event arrives (false while the device's UI
    // objects are released), and optionally save the frame just after the UI is drawn.
    void arm(bool drawUi, const std::string& screenshotPath);
    // This frame's world, as drawn, kept in `frozen` before the UI goes over it (the paused renderer's).
    void setCapture(FrozenFrame* frozen) { m_capture = frozen; }
    // After renderFrame: whether the event arrived (false means renderFrame drew no scene).
    bool fired() const { return m_fired; }
    bool screenshotSaved() const { return m_screenshotSaved; }

    void handleEvent(dice::hfe::EventCategory category, unsigned int id, dice::hfe::EventNode* node,
                     void* userData) override;

private:
    SceneOverlay(const SceneOverlay&);
    SceneOverlay& operator=(const SceneOverlay&);

    ImGuiHost&        m_imgui;
    IDirect3DDevice9* m_device;
    bool              m_attached;
    bool              m_drawUi;
    bool              m_fired;
    bool              m_screenshotSaved;
    std::string       m_screenshotPath;
    FrozenFrame*      m_capture;
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_SCENEOVERLAY_H
