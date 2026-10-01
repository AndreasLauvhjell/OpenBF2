// Code/Tools/BF2Editor/App/SceneOverlay.cpp - see SceneOverlay.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/SceneOverlay.h"

#include "App/FrameCapture.h"
#include "App/FrozenFrame.h"
#include "App/ImGuiHost.h"
#include "App/ViewRecorder.h"
#include "Framework/EditorLog.h"

#include "Core/EventManager.h"

#include <d3d9.h>

namespace bf2editor
{

namespace
{
const unsigned int kDrawDebugCounters = 5;      // ECCore id renderFrame posts inside its scene
}

SceneOverlay::SceneOverlay(ImGuiHost& imgui, IDirect3DDevice9* device)
    : m_imgui(imgui)
    , m_device(device)
    , m_attached(false)
    , m_drawUi(false)
    , m_fired(false)
    , m_screenshotSaved(false)
    , m_capture(0)
{
}

SceneOverlay::~SceneOverlay()
{
    detach();
}

bool SceneOverlay::attach()
{
    if (m_attached)
        return true;
    if (!dice::hfe::eventManager
        || !dice::hfe::eventManager->registerEventHandler(dice::hfe::ECCore, this, 0))
    {
        logLine(LogLevel::Error, "overlay: could not register on ECCore");
        return false;
    }
    m_attached = true;
    return true;
}

void SceneOverlay::detach()
{
    if (!m_attached)
        return;
    if (dice::hfe::eventManager)
        dice::hfe::eventManager->unregisterEventHandler(dice::hfe::ECCore, this);
    m_attached = false;
}

void SceneOverlay::arm(bool drawUi, const std::string& screenshotPath)
{
    m_drawUi = drawUi;
    m_fired = false;
    m_screenshotPath = screenshotPath;
}

void SceneOverlay::handleEvent(dice::hfe::EventCategory category, unsigned int id, dice::hfe::EventNode*, void*)
{
    if (category != dice::hfe::ECCore || id != kDrawDebugCounters || m_fired)
        return;
    m_fired = true;
    // The paused renderer's frame: the world as drawn, before the UI goes over it.
    if (m_capture)
    {
        m_capture->capture(m_device);
        m_capture = 0;
    }
    // A recording of the view (Tools > Record the view): the world as drawn, before the UI.
    if (viewRecorder().recording())
        viewRecorder().frame(m_device);
    if (m_device)
        m_device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);   // View > Wireframe ends with the world
    if (m_drawUi)
        m_imgui.renderDrawData();
    if (!m_screenshotPath.empty())
    {
        m_screenshotSaved = saveBackBufferBmp(m_device, m_screenshotPath);
        m_screenshotPath.clear();
    }
}

} // namespace bf2editor
