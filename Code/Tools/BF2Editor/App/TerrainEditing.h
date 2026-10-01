// Editor height brushes, stroke undo and heightmap saving through the engine's editable cluster.
// Brush centers snap to the tile grid; radius scales by primary-tile spacing with a cursor-tile minimum.
// Only Modify scales strength by frame time; other brushes apply per tick. Raise/lower flags constrain writes.
// Undo stores changed sample rectangles and restores them through setHeightData to rebuild spatial state.
// Dirty state compares samples with the last opened/saved snapshot.
#ifndef BF2EDITOR_APP_TERRAINEDITING_H
#define BF2EDITOR_APP_TERRAINEDITING_H

#include "Framework/EditorConfig.h"

#include <string>
#include <vector>

namespace bf2editor
{

enum class TerrainBrush
{
    Modify,
    Smooth,
    Level,
    Platform,
    Random,
    Paint,      // retail mode 7: the detail layers and the colormap, on the renderer's TerrainEditable (App/EditableTerrain)
    Overgrowth  // retail mode 9: the overgrowth's material or shadow map (the renderer's Overgrowth.paint* rows)
};

// What a Paint stroke paints (the retail panel's "Paint Mode": Detail 1, Color 2, Both 3).
enum class TerrainPaintMode
{
    Detail = 1,
    Color = 2,
    Both = 3
};
const char* terrainPaintModeName(TerrainPaintMode mode);

const char* terrainBrushName(TerrainBrush brush);
bool terrainBrushFromName(const std::string& name, TerrainBrush& brush);

struct TerrainBrushSettings
{
    TerrainBrush mode     = TerrainBrush::Modify;
    int          size     = 5;       // 1..50 (retail slider); times the primary tile's scale.x
    int          strength = 50;      // 1..100
    bool         raise    = true;    // write flag bit 0
    bool         lower    = true;    // write flag bit 1
    float        height   = 0.0f;    // Platform's target, metres
    // Paint.
    TerrainPaintMode paintMode        = TerrainPaintMode::Detail;
    bool             colorFromTexture = true;           // the colour source: the layer's colour texture, else `color`
    int              layer            = -1;             // the detail layer 0..5; -1 none (retail: "No layer selected!")
    unsigned         color            = 0xFF808080u;    // 0xAABBGGRR; [CHOSEN] a picker - retail took the layer's
                                                        // ground-property colour, with no alpha
    // Overgrowth.
    bool             overgrowthShadow   = false;        // the shadow map, else the material map
    int              overgrowthMaterial = 1;            // a material id of the level's Overgrowth.con (1..15)
    int              overgrowthShade    = 255;          // the shadow map's value, 0..255
};

// One tile's changed rectangle, as raw samples (the tile's own 8- or 16-bit data).
struct TerrainTileChange
{
    int sectorX = 0;
    int sectorZ = 0;
    int column  = 0;        // the rectangle within the tile's grid
    int row     = 0;
    int columns = 0;
    int rows    = 0;
    int sampleBytes = 1;
    std::vector<unsigned char> before;
    std::vector<unsigned char> after;
};

struct TerrainChange
{
    std::vector<TerrainTileChange> tiles;
    size_t samples() const;
};

class TerrainEditing
{
public:
    // Whether the level's cluster is the editable one (the editor image makes it so); `why` otherwise.
    bool available(std::string& why) const;

    // The brush at a world position: the snapped centre and the radius; false off the terrain.
    bool brushAt(const TerrainBrushSettings& settings, float x, float z, int& centreX, int& centreZ,
                 float& radius, std::string& error) const;

    // One tick of the brush. `secondary` is the other button: Modify lowers, Platform samples the
    // height under the cursor into `settings.height` (no edit), and the other brushes do nothing. Level
    // also writes the centre height into `settings.height`, as the retail height box showed it.
    bool apply(TerrainBrushSettings& settings, float x, float z, bool secondary, float frameTime,
               std::string& error);

    // The ground height (world metres) at (x, z), from the cluster.
    bool heightAt(float x, float z, float& height) const;
    // The primary tile's vertex spacing (scale.x): a brush of size n reaches n times this. 0 without a
    // cluster.
    float spacing() const;
    // The primary tile's vertical scale (scale.y): heights are its raw samples times this. 0 without a cluster.
    float verticalScale() const;

    // The vertices a brush at (centreX, centreZ) of `radius` touches, walked as the engine's brushes
    // walk them (the square clamped to the cluster, stepped by the primary tile's scale.x, the samples
    // within the radius; HeightmapClusterEditable::modifyHeight), with the cosine falloff weight. For
    // the view's cursor.
    struct BrushSample
    {
        float x;
        float z;
        float weight;
    };
    void brushSamples(int centreX, int centreZ, float radius, std::vector<BrushSample>& out) const;

    // Strokes.
    bool beginStroke(std::string& error);
    bool strokeActive() const { return !m_strokeStart.empty(); }
    // Ends the stroke; `change` holds what moved (no tiles when nothing did).
    void endStroke(TerrainChange& change);
    // Writes a change's before or after samples back.
    bool restore(const TerrainChange& change, bool after, std::string& error);

    // Dirty state: remember the tiles as they are now (open, save); compare with them. A tile's heights
    // and its "modified" flag both count.
    void markSaved();
    bool changedSinceSave() const;

    // The surrounding terrain (census §10)
    // The cluster's sectors: which hold a tile, and each tile's "modified" flag - the retail sector
    // grid's "active": Heightdata.con writes it per tile (`heightmap.setModified`), and the retail
    // editor's CreateSurroundingPatch / RemoveSurroundingPatch only set it.
    struct Sector
    {
        int  x;
        int  z;
        bool present;
        bool modified;
        int  width;         // samples per side
        int  bits;          // 8 or 16
    };
    std::vector<Sector> sectors() const;
    bool setSectorModified(int x, int z, bool modified, std::string& error);
    // HeightmapClusterEditable::importSurroundingRawHeightMap: one 8-bit raw of (3s-2)^2 bytes (s = a
    // surrounding tile's width) copied into the eight surrounding tiles. Checked here first: the engine
    // reports a missing or wrong-sized file through Debug(DebugTypeError), which stops the process in
    // this tree.
    bool importSurrounding(const std::string& path, std::string& error);
    // HeightmapClusterEditable::smoothEdges(width, amplitude, falloff): the heights across the primary
    // tile's edges (the retail button sends 32 0.5 2).
    bool smoothEdges(int width, float amplitude, float falloff, std::string& error);

    // The level's water height: the cluster's sea level (Heightdata.con's `heightmapcluster.
    // setSeaWaterLevel`, the renderer's WaterHeight). Part of the dirty comparison.
    bool seaLevel(float& level) const;
    bool setSeaLevel(float level, std::string& error);

    // Saves the heightmaps through HeightmapClusterEditable::save into `stagingRoot` (the setting
    // Editor_LevelSavePath) with the level path `levelPath` ("Levels/<name>"), and lists the files it
    // wrote there (relative to the staging root). The caller moves them into place.
    bool saveHeightmaps(const std::string& stagingRoot, const std::string& levelPath,
                        std::vector<std::string>& written, std::string& error);

    // A one-line description of the cluster for EditorTerrain.info.
    std::string describe() const;

private:
    struct TileData
    {
        int  sectorX;
        int  sectorZ;
        int  sizeX;
        int  sizeZ;
        int  sampleBytes;
        bool modified;
        std::vector<unsigned char> data;
    };
    bool readTiles(std::vector<TileData>& tiles, std::string& error) const;

    std::vector<TileData> m_strokeStart;
    std::vector<TileData> m_saved;
    float                 m_savedSeaLevel = 0.0f;
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_TERRAINEDITING_H
