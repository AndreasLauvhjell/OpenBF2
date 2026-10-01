// Code/Tools/BF2Editor/App/MinimapPanel.cpp - see MinimapPanel.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/MinimapPanel.h"

#include "App/EditSession.h"
#include "App/FreeCamera.h"
#include "Framework/EditorConsole.h"
#include "Framework/EditorLog.h"

#include "Game/Objects/IObject.h"
#include "IO/File/File.h"
#include "IO/File/FileManager.h"

#include "imgui.h"

#include <d3d9.h>

#include <cmath>
#include <cstring>
#include <vector>

namespace bf2editor
{

namespace
{

// The map as last drawn, for minimapScreenOf.
struct LastMap
{
    bool  valid;
    float origin[2];
    float size[2];
    float bounds[4];
} g_lastMap = { false, { 0, 0 }, { 0, 0 }, { 0, 0, 0, 0 } };

} // namespace

bool minimapScreenOf(float x, float z, float& screenX, float& screenY)
{
    if (!g_lastMap.valid || g_lastMap.bounds[2] <= g_lastMap.bounds[0] || g_lastMap.bounds[3] <= g_lastMap.bounds[1])
        return false;
    screenX = g_lastMap.origin[0] + (x - g_lastMap.bounds[0]) / (g_lastMap.bounds[2] - g_lastMap.bounds[0]) * g_lastMap.size[0];
    screenY = g_lastMap.origin[1] + (g_lastMap.bounds[3] - z) / (g_lastMap.bounds[3] - g_lastMap.bounds[1]) * g_lastMap.size[1];
    return true;
}

namespace
{

// The DDS header fields the loader reads (DDS_HEADER, 124 bytes after the "DDS " magic).
const unsigned kDdsMagic = 0x20534444;              // "DDS "
const unsigned kFourCcDxt1 = 0x31545844;            // "DXT1"
const unsigned kFourCcDxt3 = 0x33545844;
const unsigned kFourCcDxt5 = 0x35545844;

unsigned readU32(const unsigned char* p)
{
    return static_cast<unsigned>(p[0]) | (static_cast<unsigned>(p[1]) << 8) | (static_cast<unsigned>(p[2]) << 16)
         | (static_cast<unsigned>(p[3]) << 24);
}

bool readEngineFile(const std::string& path, std::vector<unsigned char>& data)
{
    using namespace dice::hfe;
    if (!io::fileManager)
        return false;
    // io::File, the engine's own reader (the console loads .con scripts through it).
    io::File file;
    if (!file.open(path, io::IFileSystem::FILEMODE_READ, static_cast<io::IFileSystem::FileAccess>(0), 0))
        return false;
    const int size = file.getSize();
    if (size <= 0)
        return false;
    data.resize(static_cast<size_t>(size));
    return file.read(&data[0], static_cast<unsigned>(size)) == static_cast<unsigned>(size);
}

} // namespace

MinimapPanel::MinimapPanel()
    : m_texture(0)
    , m_device(0)
    , m_version(0)
    , m_width(0)
    , m_height(0)
    , m_status("no level is open")
{
    m_bounds[0] = m_bounds[1] = -512.0f;
    m_bounds[2] = m_bounds[3] = 512.0f;
}

MinimapPanel::~MinimapPanel()
{
    release();
}

void MinimapPanel::release()
{
    if (m_texture)
    {
        m_texture->Release();
        m_texture = 0;
    }
}

bool MinimapPanel::load(IDirect3DDevice9* device, const std::string& level, const float bounds[4])
{
    release();
    m_device = device;
    m_level = level;
    for (int i = 0; i < 4; ++i)
        m_bounds[i] = bounds[i];
    m_shown.clear();
    // The editor's own picture when the level has one (the retail navigator's), else the HUD map.
    const std::string picture = "Levels/" + level + "/minimap.dds";
    if (dice::hfe::io::fileManager && !dice::hfe::io::fileManager->fileExistsEx(picture, false).empty() && loadFile(picture))
        return true;
    return loadFile("Levels/" + level + "/Hud/Minimap/ingameMap.dds");
}

bool MinimapPanel::loadFile(const std::string& path)
{
    release();
    IDirect3DDevice9* const device = m_device;
    std::vector<unsigned char> data;
    if (!device || !readEngineFile(path, data) || data.size() < 128 || readU32(&data[0]) != kDdsMagic)
    {
        m_status = path + " could not be read as a DDS file";
        logLine(LogLevel::Warning, "minimap: %s", m_status.c_str());
        return false;
    }
    // Header: height at +12, width at +16; the pixel format's flags at +80 and FourCC at +84.
    m_height = static_cast<int>(readU32(&data[12]));
    m_width = static_cast<int>(readU32(&data[16]));
    const unsigned formatFlags = readU32(&data[80]);
    const unsigned fourCc = readU32(&data[84]);
    D3DFORMAT format = D3DFMT_UNKNOWN;
    unsigned blockBytes = 0;
    if ((formatFlags & 0x4) && fourCc == kFourCcDxt1)
    {
        format = D3DFMT_DXT1;
        blockBytes = 8;
    }
    else if ((formatFlags & 0x4) && fourCc == kFourCcDxt3)
    {
        format = D3DFMT_DXT3;
        blockBytes = 16;
    }
    else if ((formatFlags & 0x4) && fourCc == kFourCcDxt5)
    {
        format = D3DFMT_DXT5;
        blockBytes = 16;
    }
    // The top-down capture (renderer.makeTopWorldScreen) is uncompressed 32-bit A8R8G8B8, one row of pixels a
    // "block" row: the pixel format's RGB flag, 32 bits, the D3D masks.
    bool uncompressed = false;
    if (format == D3DFMT_UNKNOWN && (formatFlags & 0x40) && readU32(&data[88]) == 32 && readU32(&data[92]) == 0x00FF0000u
        && readU32(&data[96]) == 0x0000FF00u && readU32(&data[100]) == 0x000000FFu)
    {
        format = (formatFlags & 0x1) ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8;
        uncompressed = true;
    }
    const unsigned blocksWide = uncompressed ? 1u : static_cast<unsigned>((m_width + 3) / 4);
    const unsigned blocksHigh = uncompressed ? static_cast<unsigned>(m_height) : static_cast<unsigned>((m_height + 3) / 4);
    if (uncompressed)
        blockBytes = static_cast<unsigned>(m_width) * 4;
    if (format == D3DFMT_UNKNOWN || m_width <= 0 || m_height <= 0
        || data.size() < 128 + static_cast<size_t>(blocksWide) * blocksHigh * blockBytes)
    {
        m_status = path + ": only DXT1/3/5 and 32-bit RGB maps are shown (the stock maps are DXT1)";
        logLine(LogLevel::Warning, "minimap: %s", m_status.c_str());
        return false;
    }
    // The managed pool survives a device reset, so the texture needs no care when the device is lost.
    if (FAILED(device->CreateTexture(static_cast<UINT>(m_width), static_cast<UINT>(m_height), 1, 0, format,
                                     D3DPOOL_MANAGED, &m_texture, 0)))
    {
        m_texture = 0;
        m_status = "the device refused a texture for the minimap";
        logLine(LogLevel::Warning, "minimap: %s", m_status.c_str());
        return false;
    }
    D3DLOCKED_RECT locked;
    if (FAILED(m_texture->LockRect(0, &locked, 0, 0)))
    {
        release();
        m_status = "the minimap texture could not be written";
        return false;
    }
    const unsigned rowBytes = blocksWide * blockBytes;
    for (unsigned row = 0; row < blocksHigh; ++row)
        std::memcpy(static_cast<unsigned char*>(locked.pBits) + row * locked.Pitch, &data[128 + row * rowBytes], rowBytes);
    m_texture->UnlockRect(0);
    m_status.clear();
    m_shown = path;
    logLine(LogLevel::Info, "minimap: %s, %dx%d, covering %.0f..%.0f by %.0f..%.0f", path.c_str(), m_width, m_height,
            m_bounds[0], m_bounds[2], m_bounds[1], m_bounds[3]);
    return true;
}

void MinimapPanel::draw(bool* open, EditSession& session)
{
    // A capture made since the last load (EditorLighting.minimap) is shown at once.
    if (m_device && !m_level.empty() && session.minimapVersion() != m_version)
    {
        m_version = session.minimapVersion();
        const float bounds[4] = { m_bounds[0], m_bounds[1], m_bounds[2], m_bounds[3] };
        load(m_device, m_level, bounds);
    }
    if (!ImGui::Begin("Minimap", open))
    {
        ImGui::End();
        return;
    }
    if (ImGui::SmallButton("Generate"))
    {
        const ConsoleLineResult r = runConsoleLine("EditorLighting.minimap 512");
        logLine(r.failed ? LogLevel::Warning : LogLevel::Info, "minimap: %s", r.output.c_str());
    }
    ImGui::SetItemTooltip("Capture the editor's own picture of the level from above (Levels/<level>/minimap.dds, the "
                          "retail navigator's) and show it.");
    ImGui::SameLine();
    ImGui::TextDisabled("%s", m_shown.c_str());
    if (!m_texture)
    {
        ImGui::TextDisabled("%s", m_status.c_str());
        ImGui::End();
        return;
    }
    const float width = m_bounds[2] - m_bounds[0];
    const float depth = m_bounds[3] - m_bounds[1];
    const float side = ImGui::GetContentRegionAvail().x > 64.0f ? ImGui::GetContentRegionAvail().x : 64.0f;
    const ImVec2 size(side, side * static_cast<float>(m_height) / static_cast<float>(m_width));
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::Image(ImTextureRef(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(m_texture))), size);
    ImDrawList* list = ImGui::GetWindowDrawList();

    // World X/Z to the map: east to the right, north (+Z) up.
    struct Map
    {
        ImVec2 origin, size;
        float  bounds[4];
        ImVec2 toScreen(float x, float z) const
        {
            return ImVec2(origin.x + (x - bounds[0]) / (bounds[2] - bounds[0]) * size.x,
                          origin.y + (bounds[3] - z) / (bounds[3] - bounds[1]) * size.y);
        }
    } map = { origin, size, { m_bounds[0], m_bounds[1], m_bounds[2], m_bounds[3] } };
    g_lastMap.valid = true;
    g_lastMap.origin[0] = origin.x;
    g_lastMap.origin[1] = origin.y;
    g_lastMap.size[0] = size.x;
    g_lastMap.size[1] = size.y;
    for (int k = 0; k < 4; ++k)
        g_lastMap.bounds[k] = m_bounds[k];

    // The combat areas, by team (an exclusion magenta), the selected one thicker.
    if (session.combatAreasShown())
    {
        const std::vector<LevelObjectFile::CombatAreaData> areas = EditSession::combatAreas();
        std::vector<ImVec2> outline;
        for (size_t a = 0; a < areas.size(); ++a)
        {
            const LevelObjectFile::CombatAreaData& area = areas[a];
            if (area.points.size() < 2)
                continue;
            outline.clear();
            for (size_t i = 0; i < area.points.size(); ++i)
                outline.push_back(map.toScreen(area.points[i].first, area.points[i].second));
            const ImU32 colour = area.inverted ? IM_COL32(230, 90, 230, 255)
                               : area.team == 1 ? IM_COL32(255, 95, 75, 255)
                               : area.team == 2 ? IM_COL32(85, 150, 255, 255) : IM_COL32(255, 215, 70, 255);
            const bool active = _stricmp(area.name.c_str(), session.activeCombatArea().c_str()) == 0;
            list->AddPolyline(&outline[0], static_cast<int>(outline.size()), colour, active ? 2.5f : 1.5f, ImDrawFlags_Closed);
        }
    }

    // The open layer's control points (flags by team), and another layer shown beside it (white: its
    // combat areas dashed, its control points as rings).
    const std::vector<LayerSummary>& layers = session.layerSummaries();
    for (size_t l = 0; l < layers.size(); ++l)
    {
        if (!session.isOpenLayer(layers[l].mode, layers[l].size))
            continue;
        for (size_t c = 0; c < layers[l].controlPoints.size(); ++c)
        {
            const LayerControlPoint& point = layers[l].controlPoints[c];
            const ImVec2 p = map.toScreen(point.position[0], point.position[2]);
            const ImU32 colour = point.team == 1 ? IM_COL32(255, 95, 75, 255)
                               : point.team == 2 ? IM_COL32(85, 150, 255, 255) : IM_COL32(255, 215, 70, 255);
            list->AddLine(ImVec2(p.x, p.y), ImVec2(p.x, p.y - 10.0f), IM_COL32(20, 20, 20, 255), 2.0f);
            list->AddTriangleFilled(ImVec2(p.x, p.y - 10.0f), ImVec2(p.x + 7.0f, p.y - 7.0f), ImVec2(p.x, p.y - 4.0f), colour);
        }
    }
    if (const LayerSummary* compared = session.compareLayer())
    {
        for (size_t a = 0; a < compared->combatAreas.size(); ++a)
        {
            const std::vector<std::pair<float, float> >& points = compared->combatAreas[a].points;
            for (size_t i = 0; i + 1 < points.size() + 1 && points.size() >= 2; i += 2)
            {
                const std::pair<float, float>& from = points[i % points.size()];
                const std::pair<float, float>& to = points[(i + 1) % points.size()];
                list->AddLine(map.toScreen(from.first, from.second), map.toScreen(to.first, to.second),
                              IM_COL32(245, 245, 245, 230), 1.5f);
            }
        }
        for (size_t c = 0; c < compared->controlPoints.size(); ++c)
        {
            const LayerControlPoint& point = compared->controlPoints[c];
            list->AddCircle(map.toScreen(point.position[0], point.position[2]), 6.0f, IM_COL32(245, 245, 245, 255), 12, 2.0f);
        }
        ImGui::TextDisabled("white: %s %d (Gameplay layers)", compared->mode.c_str(), compared->size);
    }

    // The selection, then the camera: a triangle along its heading.
    const std::vector<unsigned>& ids = session.selection().ids();
    for (size_t i = 0; i < ids.size() && i < 500; ++i)
    {
        dice::hfe::Mat4 m;
        if (!EditSession::transformOf(ids[i], m))
            continue;
        const ImVec2 p = map.toScreen(m.m[12], m.m[14]);
        list->AddRectFilled(ImVec2(p.x - 2.5f, p.y - 2.5f), ImVec2(p.x + 2.5f, p.y + 2.5f), IM_COL32(255, 170, 40, 255));
    }
    const FreeCamera& camera = session.camera();
    const ImVec2 at = map.toScreen(camera.x(), camera.z());
    const float yaw = camera.yawDegrees() * 3.14159265f / 180.0f;
    const ImVec2 ahead(std::sin(yaw), -std::cos(yaw));          // +Z is up on the map
    const ImVec2 across(-ahead.y, ahead.x);
    list->AddTriangleFilled(ImVec2(at.x + ahead.x * 9.0f, at.y + ahead.y * 9.0f),
                            ImVec2(at.x - ahead.x * 5.0f + across.x * 5.0f, at.y - ahead.y * 5.0f + across.y * 5.0f),
                            ImVec2(at.x - ahead.x * 5.0f - across.x * 5.0f, at.y - ahead.y * 5.0f - across.y * 5.0f),
                            IM_COL32(90, 200, 255, 255));

    // With the terrain brush on, the map paints: a press starts a stroke, the brush works under the cursor once a frame
    // while the button is held (Shift: its other action), and the release ends it - one undoable edit, as
    // a stroke in the view is. Otherwise a click or a drag moves the camera.
    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 mouseNow = ImGui::GetIO().MousePos;
    const float mouseX = m_bounds[0] + (mouseNow.x - origin.x) / size.x * width;
    const float mouseZ = m_bounds[3] - (mouseNow.y - origin.y) / size.y * depth;
    if (session.terrainToolActive() || m_painting)
    {
        if (!m_painting && hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            std::string error;
            if (session.beginTerrainStroke(error))
            {
                m_painting = true;
                m_paintSecondary = ImGui::GetIO().KeyShift;
                logLine(LogLevel::Info, "terrain: a stroke through the minimap from %.1f / %.1f", mouseX, mouseZ);
            }
            else
                logLine(LogLevel::Warning, "terrain: %s", error.c_str());
        }
        if (m_painting && (!ImGui::IsMouseDown(ImGuiMouseButton_Left) || !session.terrainStrokeActive()))
        {
            m_painting = false;
            session.endTerrainStroke();
            logLine(LogLevel::Info, "terrain: the stroke through the minimap ends at %.1f / %.1f", mouseX, mouseZ);
        }
        else if (m_painting)
        {
            std::string error;
            if (!session.terrainStrokeTick(mouseX, mouseZ, m_paintSecondary, ImGui::GetIO().DeltaTime, error))
                logLine(LogLevel::Warning, "terrain: %s", error.c_str());
        }
        // The brush's circle on the map.
        int centreX = 0, centreZ = 0;
        float radius = 0.0f;
        std::string error;
        if ((hovered || m_painting) && session.terrain().brushAt(session.terrainBrush(), mouseX, mouseZ, centreX, centreZ, radius, error))
        {
            const ImVec2 centre = map.toScreen(static_cast<float>(centreX), static_cast<float>(centreZ));
            list->AddCircle(centre, radius / width * size.x, IM_COL32(255, 170, 40, 255), 32, 2.0f);
        }
        if (hovered)
            ImGui::SetTooltip("%.0f / %.0f\npaint with the terrain brush (Shift: its other action)", mouseX, mouseZ);
        ImGui::TextDisabled("%.0f x %.0f m; the terrain brush paints through the map", width, depth);
        ImGui::End();
        return;
    }
    if (hovered && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const float x = m_bounds[0] + (mouse.x - origin.x) / size.x * width;
        const float z = m_bounds[3] - (mouse.y - origin.y) / size.y * depth;
        float here = 0.0f, there = 0.0f;
        float y = camera.y();
        if (session.groundHeight(camera.x(), camera.z(), here) && session.groundHeight(x, z, there))
            y = there + (camera.y() - here);
        dice::hfe::Vec3 target;
        target.x = x;
        target.y = y;
        target.z = z;
        std::string ignored;
        session.setCameraPosition(target, ignored);
    }
    if (hovered)
    {
        const ImVec2 mouse = ImGui::GetIO().MousePos;
        ImGui::SetTooltip("%.0f / %.0f\nclick or drag to move the camera",
                          m_bounds[0] + (mouse.x - origin.x) / size.x * width,
                          m_bounds[3] - (mouse.y - origin.y) / size.y * depth);
    }
    ImGui::TextDisabled("%.0f x %.0f m; the camera %.0f / %.0f", width, depth, camera.x(), camera.z());
    ImGui::End();
}

} // namespace bf2editor
