// Code/Tools/BF2Editor/App/TerrainEditing.cpp - see TerrainEditing.h.
#include "Framework/EditorConfig.h"
#include "App/TerrainEditing.h"

#include "Core/ClassManager.h"
#include "Core/Ids.h"
#include "Core/SettingsRepostitory.h"
#include "Physics/Heightmap.h"
#include "Physics/HeightmapCluster.h"
#include "Physics/HeightmapClusterEditable.h"
#include "Physics/HeightmapEditable.h"

#include <windows.h>

#include <cmath>
#include <cstdio>
#include <cstring>

namespace dice { namespace hfe {
extern ClassManager* classManager;
extern SettingsRepostitory* settings;
} }

namespace bf2editor
{

using dice::hfe::world::IHeightmap;
using dice::hfe::world::IHeightmapCluster;
using dice::hfe::world::IHeightmapClusterEditable;
using dice::hfe::world::IHeightmapEditable;

namespace
{

IHeightmapCluster* cluster()
{
    if (!dice::hfe::classManager)
        return 0;
    return static_cast<IHeightmapCluster*>(dice::hfe::classManager->getSingleton("HeightmapCluster"));
}

IHeightmapEditable* editableTile(IHeightmap* tile)
{
    return tile ? static_cast<IHeightmapEditable*>(tile->queryInterface(dice::hfe::world::IID_IHeightmapEditable))
                : 0;
}

int writeFlags(const TerrainBrushSettings& s)
{
    return (s.raise ? 1 : 0) | (s.lower ? 2 : 0);
}

// Every file under `directory` (recursively), as paths relative to `root`.
void listFiles(const std::string& root, const std::string& relative, std::vector<std::string>& out)
{
    WIN32_FIND_DATAA found;
    const std::string pattern = root + "/" + relative + (relative.empty() ? "" : "/") + "*";
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

} // namespace

const char* terrainBrushName(TerrainBrush brush)
{
    switch (brush)
    {
    case TerrainBrush::Modify:   return "modify";
    case TerrainBrush::Smooth:   return "smooth";
    case TerrainBrush::Level:    return "level";
    case TerrainBrush::Platform: return "platform";
    case TerrainBrush::Random:   return "random";
    case TerrainBrush::Paint:    return "paint";
    case TerrainBrush::Overgrowth: return "overgrowth";
    }
    return "?";
}

const char* terrainPaintModeName(TerrainPaintMode mode)
{
    switch (mode)
    {
    case TerrainPaintMode::Detail: return "detail";
    case TerrainPaintMode::Color:  return "color";
    case TerrainPaintMode::Both:   return "both";
    }
    return "?";
}

bool terrainBrushFromName(const std::string& name, TerrainBrush& brush)
{
    const TerrainBrush all[] = { TerrainBrush::Modify, TerrainBrush::Smooth, TerrainBrush::Level,
                                 TerrainBrush::Platform, TerrainBrush::Random, TerrainBrush::Paint,
                                 TerrainBrush::Overgrowth };
    for (size_t i = 0; i < sizeof(all) / sizeof(all[0]); ++i)
    {
        if (_stricmp(name.c_str(), terrainBrushName(all[i])) == 0)
        {
            brush = all[i];
            return true;
        }
    }
    return false;
}

size_t TerrainChange::samples() const
{
    size_t n = 0;
    for (size_t i = 0; i < tiles.size(); ++i)
        n += static_cast<size_t>(tiles[i].columns) * static_cast<size_t>(tiles[i].rows);
    return n;
}

bool TerrainEditing::available(std::string& why) const
{
    if (!cluster())
    {
        why = "the level has no heightmap cluster";
        return false;
    }
    if (!dice::hfe::world::getHeightmapClusterEditable())
    {
        why = "the heightmap cluster is not the editable one (the editor image makes it so at boot)";
        return false;
    }
    return true;
}

bool TerrainEditing::brushAt(const TerrainBrushSettings& settings, float x, float z, int& centreX, int& centreZ,
                             float& radius, std::string& error) const
{
    IHeightmapCluster* c = cluster();
    if (!c)
    {
        error = "no heightmap cluster";
        return false;
    }
    IHeightmap* tile = c->getHeightmapInWorldCoords(x, z);
    if (!tile)
    {
        error = "the position is off the terrain";
        return false;
    }
    // Plugin 0x1001C9F0: snapped to the vertex grid of the tile under the cursor.
    const int s = static_cast<int>(tile->getScale().x);
    if (s <= 0)
    {
        error = "the tile under the position has no scale";
        return false;
    }
    centreX = s * static_cast<int>(std::floor(x / static_cast<float>(s) + 0.5f));
    centreZ = s * static_cast<int>(std::floor(z / static_cast<float>(s) + 0.5f));
    radius = static_cast<float>(settings.size) * c->getPrimaryScale().x;
    if (radius < tile->getScale().x)
        radius = tile->getScale().x;
    return true;
}

bool TerrainEditing::heightAt(float x, float z, float& height) const
{
    IHeightmapClusterEditable* e = dice::hfe::world::getHeightmapClusterEditable();
    IHeightmapCluster* c = cluster();
    if (!c || !c->isInsideHeightmapCluster(x, z))
        return false;
    height = e ? e->getWorldScaledHeightInWorldCoords(x, z) : c->getWorldScaledHeightInWorldCoords(x, z);
    return true;
}

float TerrainEditing::spacing() const
{
    IHeightmapCluster* c = cluster();
    return c ? c->getPrimaryScale().x : 0.0f;
}

float TerrainEditing::verticalScale() const
{
    IHeightmapCluster* c = cluster();
    return c ? c->getPrimaryScale().y : 0.0f;
}

void TerrainEditing::brushSamples(int centreX, int centreZ, float radius, std::vector<BrushSample>& out) const
{
    out.clear();
    IHeightmapCluster* c = cluster();
    if (!c || radius <= 0.0f)
        return;
    const float step = c->getPrimaryScale().x;
    if (step <= 0.0f)
        return;
    const int extent = c->getHeightmapSize() * c->getClusterSize() / 2;
    const int r = static_cast<int>(radius);
    const int x0 = (-extent < centreX - r) ? centreX - r : -extent;
    const int x1 = (centreX + r > extent) ? extent : centreX + r;
    const int z0 = (-extent < centreZ - r) ? centreZ - r : -extent;
    const int z1 = (centreZ + r <= extent) ? centreZ + r : extent;
    const int countX = static_cast<int>(static_cast<float>(x1 - x0) / step);
    const int countZ = static_cast<int>(static_cast<float>(z1 - z0) / step);
    for (int iz = 0; iz < countZ; ++iz)
    {
        const float sz = static_cast<float>(z0) + step * static_cast<float>(iz);
        for (int ix = 0; ix < countX; ++ix)
        {
            const float sx = static_cast<float>(x0) + step * static_cast<float>(ix);
            const float dx = sx - static_cast<float>(centreX), dz = sz - static_cast<float>(centreZ);
            const float distance = std::sqrt(dx * dx + dz * dz);
            if (distance > radius)
                continue;
            float weight = std::cos(distance / radius * 1.5707964f);
            if (weight < 0.0f)
                weight = 0.0f;
            BrushSample sample = { sx, sz, weight };
            out.push_back(sample);
        }
    }
}

bool TerrainEditing::apply(TerrainBrushSettings& settings, float x, float z, bool secondary, float frameTime,
                           std::string& error)
{
    IHeightmapClusterEditable* e = dice::hfe::world::getHeightmapClusterEditable();
    if (!e)
    {
        available(error);
        return false;
    }
    int cx = 0, cz = 0;
    float radius = 0.0f;
    if (!brushAt(settings, x, z, cx, cz, radius, error))
        return false;
    const float strength = static_cast<float>(settings.strength);
    const int flags = writeFlags(settings);
    // Only Modify and Platform have a second action in the retail plugin (census §3); the other
    // brushes do nothing on the other button.
    if (secondary && settings.mode != TerrainBrush::Modify && settings.mode != TerrainBrush::Platform)
        return true;
    switch (settings.mode)
    {
    case TerrainBrush::Modify:
    {
        const float amount = frameTime * strength * 0.5f;
        e->modifyHeight(cx, cz, radius, secondary ? -amount : amount, flags);
        return true;
    }
    case TerrainBrush::Smooth:
        e->smooth(cx, cz, radius, strength * 0.01f, flags);
        return true;
    case TerrainBrush::Level:
    {
        // Plugin 0x1001EBB0: the centre height goes into the height box first; the engine is given the
        // -1 sentinel and takes the centre itself.
        float h = 0.0f;
        if (heightAt(x, z, h))
            settings.height = h;
        e->setHeight(cx, cz, radius, strength * 0.01f, -1.0f, flags);
        return true;
    }
    case TerrainBrush::Platform:
        if (secondary)
        {
            // Plugin 0x1001C940: sample the height under the cursor into the box; no edit.
            float h = 0.0f;
            if (!heightAt(x, z, h))
            {
                error = "no ground under the position";
                return false;
            }
            settings.height = h;
            return true;
        }
        e->setHeight(cx, cz, radius, strength * 0.01f, settings.height, flags);
        return true;
    case TerrainBrush::Random:
        e->random(cx, cz, radius, strength, flags);
        return true;
    }
    error = "unknown brush";
    return false;
}

bool TerrainEditing::readTiles(std::vector<TileData>& tiles, std::string& error) const
{
    tiles.clear();
    IHeightmapCluster* c = cluster();
    if (!c)
    {
        error = "no heightmap cluster";
        return false;
    }
    const int half = c->getClusterSize() / 2;
    for (int sz = -half; sz <= half; ++sz)
    {
        for (int sx = -half; sx <= half; ++sx)
        {
            IHeightmap* tile = c->getHeightmapInSector(sx, sz);
            IHeightmapEditable* e = editableTile(tile);
            if (!tile || !e || !e->getHeightData())
                continue;
            TileData t;
            t.sectorX = sx;
            t.sectorZ = sz;
            t.modified = tile->getModified();
            tile->getSize(t.sizeX, t.sizeZ);
            t.sampleBytes = tile->getBitResolution() / 8;
            if (t.sampleBytes != 1 && t.sampleBytes != 2)
                continue;
            const size_t bytes = static_cast<size_t>(t.sizeX) * static_cast<size_t>(t.sizeZ) * t.sampleBytes;
            const unsigned char* data = static_cast<const unsigned char*>(e->getHeightData());
            t.data.assign(data, data + bytes);
            tiles.push_back(t);
        }
    }
    if (tiles.empty())
    {
        error = "no editable heightmap tile";
        return false;
    }
    return true;
}

bool TerrainEditing::beginStroke(std::string& error)
{
    std::vector<TileData> tiles;
    if (!readTiles(tiles, error))
        return false;
    m_strokeStart.swap(tiles);
    return true;
}

void TerrainEditing::endStroke(TerrainChange& change)
{
    change.tiles.clear();
    std::vector<TileData> now;
    std::string error;
    if (m_strokeStart.empty() || !readTiles(now, error))
    {
        m_strokeStart.clear();
        return;
    }
    for (size_t i = 0; i < m_strokeStart.size(); ++i)
    {
        const TileData& before = m_strokeStart[i];
        const TileData* after = 0;
        for (size_t j = 0; j < now.size(); ++j)
            if (now[j].sectorX == before.sectorX && now[j].sectorZ == before.sectorZ)
                after = &now[j];
        if (!after || after->data.size() != before.data.size() || after->data == before.data)
            continue;
        // The rectangle of samples that changed.
        const int b = before.sampleBytes;
        int minX = before.sizeX, minZ = before.sizeZ, maxX = -1, maxZ = -1;
        for (int row = 0; row < before.sizeZ; ++row)
        {
            const size_t offset = static_cast<size_t>(row) * before.sizeX * b;
            if (std::memcmp(&before.data[offset], &after->data[offset], static_cast<size_t>(before.sizeX) * b) == 0)
                continue;
            for (int col = 0; col < before.sizeX; ++col)
            {
                if (std::memcmp(&before.data[offset + col * b], &after->data[offset + col * b], b) != 0)
                {
                    if (col < minX) minX = col;
                    if (col > maxX) maxX = col;
                    if (row < minZ) minZ = row;
                    if (row > maxZ) maxZ = row;
                }
            }
        }
        if (maxX < 0)
            continue;
        TerrainTileChange t;
        t.sectorX = before.sectorX;
        t.sectorZ = before.sectorZ;
        t.column = minX;
        t.row = minZ;
        t.columns = maxX - minX + 1;
        t.rows = maxZ - minZ + 1;
        t.sampleBytes = b;
        for (int row = minZ; row <= maxZ; ++row)
        {
            const size_t offset = (static_cast<size_t>(row) * before.sizeX + minX) * b;
            const size_t length = static_cast<size_t>(t.columns) * b;
            t.before.insert(t.before.end(), before.data.begin() + offset, before.data.begin() + offset + length);
            t.after.insert(t.after.end(), after->data.begin() + offset, after->data.begin() + offset + length);
        }
        change.tiles.push_back(t);
    }
    m_strokeStart.clear();
}

bool TerrainEditing::restore(const TerrainChange& change, bool after, std::string& error)
{
    IHeightmapCluster* c = cluster();
    if (!c)
    {
        error = "no heightmap cluster";
        return false;
    }
    for (size_t i = 0; i < change.tiles.size(); ++i)
    {
        const TerrainTileChange& t = change.tiles[i];
        IHeightmap* tile = c->getHeightmapInSector(t.sectorX, t.sectorZ);
        IHeightmapEditable* e = editableTile(tile);
        if (!tile || !e || !e->getHeightData())
        {
            char text[96];
            std::snprintf(text, sizeof(text), "no editable tile in sector %d,%d", t.sectorX, t.sectorZ);
            error = text;
            return false;
        }
        int sizeX = 0, sizeZ = 0;
        tile->getSize(sizeX, sizeZ);
        if (tile->getBitResolution() / 8 != t.sampleBytes || t.column + t.columns > sizeX || t.row + t.rows > sizeZ)
        {
            error = "the tile no longer has the size the change was recorded against";
            return false;
        }
        const size_t bytes = static_cast<size_t>(sizeX) * sizeZ * t.sampleBytes;
        std::vector<unsigned char> data(static_cast<const unsigned char*>(e->getHeightData()),
                                        static_cast<const unsigned char*>(e->getHeightData()) + bytes);
        const std::vector<unsigned char>& source = after ? t.after : t.before;
        const size_t length = static_cast<size_t>(t.columns) * t.sampleBytes;
        for (int row = 0; row < t.rows; ++row)
        {
            const size_t target = (static_cast<size_t>(t.row + row) * sizeX + t.column) * t.sampleBytes;
            std::memcpy(&data[target], &source[static_cast<size_t>(row) * length], length);
        }
        // Retail's undo call (plugin 0x1001D1C0): the whole tile, its quadtree rebuilt, the modified
        // event posted for the renderer's terrain.
        e->setHeightData(&data[0]);
    }
    return true;
}

void TerrainEditing::markSaved()
{
    std::string error;
    if (!readTiles(m_saved, error))
        m_saved.clear();
    if (!seaLevel(m_savedSeaLevel))
        m_savedSeaLevel = 0.0f;
}

bool TerrainEditing::seaLevel(float& level) const
{
    IHeightmapCluster* c = cluster();
    if (!c)
        return false;
    level = c->getSeaWaterLevel();
    return true;
}

bool TerrainEditing::setSeaLevel(float level, std::string& error)
{
    IHeightmapCluster* c = cluster();
    if (!c)
    {
        error = "no heightmap cluster";
        return false;
    }
    if (!(level > -10000.0f && level < 10000.0f))
    {
        error = "the sea level is a height in metres";
        return false;
    }
    c->setSeaWaterLevel(level);
    return true;
}

bool TerrainEditing::changedSinceSave() const
{
    std::vector<TileData> now;
    std::string error;
    if (!readTiles(now, error))
        return false;
    float level = 0.0f;
    if (seaLevel(level) && level != m_savedSeaLevel)
        return true;
    if (now.size() != m_saved.size())
        return true;
    for (size_t i = 0; i < now.size(); ++i)
        if (now[i].sectorX != m_saved[i].sectorX || now[i].sectorZ != m_saved[i].sectorZ
            || now[i].modified != m_saved[i].modified || now[i].data != m_saved[i].data)
            return true;
    return false;
}

std::vector<TerrainEditing::Sector> TerrainEditing::sectors() const
{
    std::vector<Sector> out;
    IHeightmapCluster* c = cluster();
    if (!c)
        return out;
    const int half = c->getClusterSize() / 2;
    for (int sz = -half; sz <= half; ++sz)
    {
        for (int sx = -half; sx <= half; ++sx)
        {
            Sector s;
            s.x = sx;
            s.z = sz;
            IHeightmap* tile = c->getHeightmapInSector(sx, sz);
            s.present = tile != 0;
            s.modified = tile && tile->getModified();
            int width = 0, depth = 0;
            if (tile)
                tile->getSize(width, depth);
            s.width = width;
            s.bits = tile ? tile->getBitResolution() : 0;
            out.push_back(s);
        }
    }
    return out;
}

bool TerrainEditing::setSectorModified(int x, int z, bool modified, std::string& error)
{
    IHeightmapCluster* c = cluster();
    IHeightmap* tile = c ? c->getHeightmapInSector(x, z) : 0;
    if (!tile || !editableTile(tile))
    {
        char text[96];
        std::snprintf(text, sizeof(text), "no editable tile in sector %d,%d", x, z);
        error = text;
        return false;
    }
    if (x == 0 && z == 0)
    {
        error = "the primary tile is always saved";
        return false;
    }
    tile->setModified(modified);
    return true;
}

bool TerrainEditing::importSurrounding(const std::string& path, std::string& error)
{
    IHeightmapClusterEditable* e = dice::hfe::world::getHeightmapClusterEditable();
    IHeightmapCluster* c = cluster();
    if (!e || !c)
    {
        available(error);
        return false;
    }
    // The engine's own size rule: the first surrounding tile in sector order gives s.
    const int half = c->getClusterSize() / 2;
    IHeightmap* first = 0;
    for (int z = -half; z <= half && !first; ++z)
        for (int x = -half; x <= half && !first; ++x)
            if (x || z)
                first = c->getHeightmapInSector(x, z);
    if (!first || c->getClusterSize() != 3)
    {
        error = "the import needs a 3x3 cluster with surrounding tiles";
        return false;
    }
    int width = 0, depth = 0;
    first->getSize(width, depth);
    const unsigned long long side = 3ULL * static_cast<unsigned long long>(width) - 2ULL;
    WIN32_FILE_ATTRIBUTE_DATA info;
    if (!GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &info)
        || (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        error = "no file " + path;
        return false;
    }
    const unsigned long long size = (static_cast<unsigned long long>(info.nFileSizeHigh) << 32) | info.nFileSizeLow;
    if (size != side * side)
    {
        char text[160];
        std::snprintf(text, sizeof(text), "the file has %llu bytes; an 8-bit raw of %llux%llu (%llu bytes) fits this cluster",
                      size, side, side, side * side);
        error = text;
        return false;
    }
    if (!e->importSurroundingRawHeightMap(path))
    {
        error = "HeightmapClusterEditable::importSurroundingRawHeightMap refused " + path;
        return false;
    }
    return true;
}

bool TerrainEditing::smoothEdges(int width, float amplitude, float falloff, std::string& error)
{
    IHeightmapClusterEditable* e = dice::hfe::world::getHeightmapClusterEditable();
    if (!e)
    {
        available(error);
        return false;
    }
    if (width < 1 || width > 512)
    {
        error = "the width is 1..512 metres";
        return false;
    }
    e->smoothEdges(width, amplitude, falloff);
    return true;
}

bool TerrainEditing::saveHeightmaps(const std::string& stagingRoot, const std::string& levelPath,
                                    std::vector<std::string>& written, std::string& error)
{
    IHeightmapClusterEditable* e = dice::hfe::world::getHeightmapClusterEditable();
    if (!e)
    {
        available(error);
        return false;
    }
    if (!dice::hfe::settings)
    {
        error = "no settings repository";
        return false;
    }
    // HeightmapClusterEditable::save writes under the setting Editor_LevelSavePath; the editor points it
    // at a staging root for the call and puts the old value back.
    const std::string key = "Editor_LevelSavePath";
    const std::string previous = dice::hfe::settings->stringGet(key, std::string());
    dice::hfe::settings->stringSet(key, stagingRoot);
    const bool ok = e->save(levelPath);
    dice::hfe::settings->stringSet(key, previous);
    written.clear();
    listFiles(stagingRoot, "", written);
    if (!ok)
    {
        error = "HeightmapClusterEditable::save did not write all of Heightdata.con";
        return false;
    }
    return true;
}

std::string TerrainEditing::describe() const
{
    IHeightmapCluster* c = cluster();
    if (!c)
        return "no heightmap cluster";
    IHeightmapClusterEditable* e = dice::hfe::world::getHeightmapClusterEditable();
    const int half = c->getClusterSize() / 2;
    int tiles = 0, editable = 0;
    for (int sz = -half; sz <= half; ++sz)
        for (int sx = -half; sx <= half; ++sx)
        {
            IHeightmap* t = c->getHeightmapInSector(sx, sz);
            if (t)
            {
                ++tiles;
                if (editableTile(t))
                    ++editable;
            }
        }
    char text[320];
    int px = 0, pz = 0;
    IHeightmap* primary = c->getPrimaryHeightmap();
    if (primary)
        primary->getSize(px, pz);
    const dice::hfe::Vec3& ps = c->getPrimaryScale();
    std::snprintf(text, sizeof(text),
                  "cluster %s, size %d (heightmap size %d), %d tiles (%d editable), primary %dx%d at %.3g/%.6g/%.3g, "
                  "sea level %.3g",
                  e ? "editable" : "NOT editable", c->getClusterSize(), c->getHeightmapSize(), tiles, editable, px,
                  pz, ps.x, ps.y, ps.z, c->getSeaWaterLevel());
    return text;
}

} // namespace bf2editor
