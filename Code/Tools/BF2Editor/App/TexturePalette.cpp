// Code/Tools/BF2Editor/App/TexturePalette.cpp - see TexturePalette.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/TexturePalette.h"

#include "App/TestHarness.h"
#include "Framework/EditorLog.h"

#include "IO/File/File.h"
#include "IO/File/FileManager.h"

#include "imgui.h"

#include <d3d9.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <list>
#include <map>
#include <vector>

namespace bf2editor
{

namespace TexturePalette
{

namespace
{

const unsigned kDdsMagic = 0x20534444;              // "DDS "
const unsigned kFourCcDxt1 = 0x31545844;            // "DXT1"
const unsigned kFourCcDxt3 = 0x33545844;
const unsigned kFourCcDxt5 = 0x35545844;
const int kThumb = 64;                              // thumbnail size in the popup, pixels
const int kLoadsPerFrame = 6;                       // thumbnails read a frame, so a large folder opens at once

IDirect3DDevice9* g_device = 0;

struct Thumb
{
    IDirect3DTexture9* texture = 0;
    bool               tried = false;
};
std::map<std::string, Thumb> g_thumbs;              // by engine path

// The open popup's folder and its textures (engine paths relative to the folder, '/', no extension filter applied).
std::string g_folder;
std::vector<std::string> g_entries;
char g_filter[96] = "";

std::string lowered(std::string text)
{
    for (size_t i = 0; i < text.size(); ++i)
        text[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
    return text;
}

std::string bareProperty(const std::string& property)
{
    std::string bare = lowered(property);
    if (bare.size() > 3 && (bare.compare(0, 3, "set") == 0 || bare.compare(0, 3, "get") == 0))
        bare = bare.substr(3);
    return bare;
}

unsigned readU32(const unsigned char* p)
{
    return static_cast<unsigned>(p[0]) | (static_cast<unsigned>(p[1]) << 8) | (static_cast<unsigned>(p[2]) << 16)
         | (static_cast<unsigned>(p[3]) << 24);
}

// The folder the palette shows for a value: from the value's own `textures` folder down, else the folder holding it;
// with no value, the property's stock folder. Engine form: '/', lower case, a trailing '/'.
std::string folderFor(const std::string& property, const std::string& value)
{
    std::string v = lowered(value);
    std::replace(v.begin(), v.end(), '\\', '/');
    const size_t textures = v.find("/textures/");
    if (textures != std::string::npos)
        return v.substr(0, textures + 10);
    const size_t slash = v.rfind('/');
    if (slash != std::string::npos)
        return v.substr(0, slash + 1);
    return bareProperty(property) == "decaltexturename" ? "objects/effects/decals/textures/" : "objects/effects/textures/";
}

// The value as stock files write it for a texture at `folder` + `relative` (relative with '/', with its extension).
std::string valueFor(const std::string& folder, const std::string& relative)
{
    std::string path = folder + relative;
    const size_t dot = path.rfind('.');
    if (dot != std::string::npos && path.find('/', dot) == std::string::npos)
        path.erase(dot);
    std::replace(path.begin(), path.end(), '/', '\\');
    return path;
}

void listFolder(const std::string& folder)
{
    g_folder = folder;
    g_entries.clear();
    if (!dice::hfe::io::fileManager)
        return;
    std::list<dice::hfe::io::FileInfo> found;
    dice::hfe::io::fileManager->findFiles(folder, true, found);
    for (std::list<dice::hfe::io::FileInfo>::const_iterator it = found.begin(); it != found.end(); ++it)
    {
        if (it->isDirectory)
            continue;
        std::string name = it->name;
        std::replace(name.begin(), name.end(), '\\', '/');
        const std::string low = lowered(name);
        if (low.size() > 4 && low.compare(low.size() - 4, 4, ".dds") == 0)
            g_entries.push_back(name);
    }
    std::sort(g_entries.begin(), g_entries.end(), [](const std::string& a, const std::string& b) {
        return lowered(a) < lowered(b);
    });
    logLine(LogLevel::Info, "texture palette: %u textures under %s", static_cast<unsigned>(g_entries.size()), folder.c_str());
}

// A DDS file's top level as a texture (DXT1/3/5 or 32-bit RGB), read through the engine's file manager.
IDirect3DTexture9* loadThumb(const std::string& path)
{
    if (!g_device || !dice::hfe::io::fileManager)
        return 0;
    dice::hfe::io::File file;
    if (!file.open(path, dice::hfe::io::IFileSystem::FILEMODE_READ, static_cast<dice::hfe::io::IFileSystem::FileAccess>(0), 0))
        return 0;
    const int size = file.getSize();
    if (size < 128)
        return 0;
    std::vector<unsigned char> data(static_cast<size_t>(size));
    if (file.read(&data[0], static_cast<unsigned>(size)) != static_cast<unsigned>(size) || readU32(&data[0]) != kDdsMagic)
        return 0;
    const int height = static_cast<int>(readU32(&data[12]));
    const int width = static_cast<int>(readU32(&data[16]));
    const unsigned flags = readU32(&data[80]);
    const unsigned fourCc = readU32(&data[84]);
    D3DFORMAT format = D3DFMT_UNKNOWN;
    unsigned blockBytes = 0;
    bool uncompressed = false;
    if ((flags & 0x4) && fourCc == kFourCcDxt1)
    {
        format = D3DFMT_DXT1;
        blockBytes = 8;
    }
    else if ((flags & 0x4) && (fourCc == kFourCcDxt3 || fourCc == kFourCcDxt5))
    {
        format = fourCc == kFourCcDxt3 ? D3DFMT_DXT3 : D3DFMT_DXT5;
        blockBytes = 16;
    }
    else if ((flags & 0x40) && readU32(&data[88]) == 32 && readU32(&data[92]) == 0x00FF0000u && readU32(&data[96]) == 0x0000FF00u
             && readU32(&data[100]) == 0x000000FFu)
    {
        format = (flags & 0x1) ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8;
        uncompressed = true;
    }
    if (format == D3DFMT_UNKNOWN || width <= 0 || height <= 0 || width > 4096 || height > 4096)
        return 0;
    const unsigned blocksWide = uncompressed ? 1u : static_cast<unsigned>((width + 3) / 4);
    const unsigned blocksHigh = uncompressed ? static_cast<unsigned>(height) : static_cast<unsigned>((height + 3) / 4);
    if (uncompressed)
        blockBytes = static_cast<unsigned>(width) * 4;
    const unsigned rowBytes = blocksWide * blockBytes;
    if (data.size() < 128 + static_cast<size_t>(rowBytes) * blocksHigh)
        return 0;
    IDirect3DTexture9* texture = 0;
    if (FAILED(g_device->CreateTexture(static_cast<UINT>(width), static_cast<UINT>(height), 1, 0, format, D3DPOOL_MANAGED,
                                       &texture, 0)))
        return 0;
    D3DLOCKED_RECT locked;
    if (FAILED(texture->LockRect(0, &locked, 0, 0)))
    {
        texture->Release();
        return 0;
    }
    for (unsigned row = 0; row < blocksHigh; ++row)
        std::memcpy(static_cast<unsigned char*>(locked.pBits) + row * locked.Pitch, &data[128 + row * rowBytes], rowBytes);
    texture->UnlockRect(0);
    return texture;
}

} // namespace

bool serves(const std::string& property)
{
    const std::string bare = bareProperty(property);
    return bare == "texturename" || bare == "decaltexturename";
}

void setDevice(IDirect3DDevice9* device)
{
    if (device != g_device)
        releaseAll();
    g_device = device;
}

void releaseAll()
{
    for (std::map<std::string, Thumb>::iterator it = g_thumbs.begin(); it != g_thumbs.end(); ++it)
        if (it->second.texture)
            it->second.texture->Release();
    g_thumbs.clear();
}

bool button(const char* id, const std::string& property, const std::string& value, std::string& out)
{
    bool picked = false;
    ImGui::PushID(id);
    if (ImGui::SmallButton("..."))
    {
        listFolder(folderFor(property, value));
        g_filter[0] = 0;
        ImGui::OpenPopup("palette");
    }
    ImGui::SetItemTooltip("Pick a texture from %s", folderFor(property, value).c_str());
    // A popup sizes itself to what it holds, so the list has a size of its own.
    if (ImGui::BeginPopup("palette"))
    {
        ImGui::Text("%s (%u)", g_folder.c_str(), static_cast<unsigned>(g_entries.size()));
        ImGui::SetNextItemWidth(540.0f);
        ImGui::InputTextWithHint("##filter", "filter by name", g_filter, sizeof(g_filter));
        const std::string filter = lowered(g_filter);
        if (g_entries.empty())
            ImGui::TextDisabled("no DDS texture in this folder of the mod");
        ImGui::BeginChild("thumbs", ImVec2(540.0f, 380.0f), ImGuiChildFlags_None);
        const float cell = static_cast<float>(kThumb) + ImGui::GetStyle().ItemSpacing.x + 8.0f;
        const int columns = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / cell));
        int loads = 0, shown = 0;
        for (size_t i = 0; i < g_entries.size(); ++i)
        {
            if (!filter.empty() && lowered(g_entries[i]).find(filter) == std::string::npos)
                continue;
            if (shown % columns)
                ImGui::SameLine();
            ++shown;
            const std::string path = g_folder + g_entries[i];
            Thumb& thumb = g_thumbs[path];
            // Only a thumbnail in sight is read, a few a frame.
            const bool visible = ImGui::IsRectVisible(ImVec2(static_cast<float>(kThumb) + 8.0f, static_cast<float>(kThumb) + 8.0f));
            if (visible && !thumb.tried && loads < kLoadsPerFrame)
            {
                thumb.tried = true;
                thumb.texture = loadThumb(path);
                ++loads;
            }
            ImGui::PushID(static_cast<int>(i));
            bool clicked;
            if (thumb.texture)
                clicked = ImGui::ImageButton("##t", ImTextureRef(static_cast<ImTextureID>(reinterpret_cast<intptr_t>(thumb.texture))),
                                             ImVec2(static_cast<float>(kThumb), static_cast<float>(kThumb)));
            else
                clicked = ImGui::Button(thumb.tried ? "(no preview)" : "...", ImVec2(static_cast<float>(kThumb) + 8.0f,
                                                                                       static_cast<float>(kThumb) + 8.0f));
            noteTestItem(valueFor(g_folder, g_entries[i]).c_str());   // a thumbnail by its value, for a scripted click
            ImGui::SetItemTooltip("%s", valueFor(g_folder, g_entries[i]).c_str());
            if (clicked)
            {
                out = valueFor(g_folder, g_entries[i]);
                picked = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::EndPopup();
    }
    ImGui::PopID();
    return picked;
}

} // namespace TexturePalette

} // namespace bf2editor
