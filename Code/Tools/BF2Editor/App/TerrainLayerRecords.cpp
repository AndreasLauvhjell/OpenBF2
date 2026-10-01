// Code/Tools/BF2Editor/App/TerrainLayerRecords.cpp - see TerrainLayerRecords.h. Editor-only code (D3).

// The renderer's record class is declared for the native plugin ABI the editor's RendDX9.dll is built with
// (renddx9.cmd native, /MD): this translation unit includes Geom/Terrain.h with BF2_PLUGIN_ABI_NATIVE 1. The colour
// texture's slot takes a vc71::string by value (the editor builds RendDX9Abi/Vc71String.cpp natively: one CRT heap).
#define BF2_PLUGIN_ABI_NATIVE 1
#include "Framework/EditorConfig.h"
#include "App/TerrainLayerRecords.h"

#include "Framework/GroundPropertiesFile.h"

#include "Geom/ITerrainEditable.h"
#include "Geom/Terrain.h"

namespace bf2editor
{

std::string applyGroundLayer(dice::hfe::geom::ITerrainEditable* editable, int layer, const GroundLayer& settings)
{
    using namespace dice::hfe;
    if (!editable || layer < 0 || layer > 5)
        return "the layer";
    if (!settings.detailTexture.empty())
        editable->setDetailTextureName(layer, settings.detailTexture);
    geom::TerrainDetailTextureBase* const record = editable->getDetailTexture(static_cast<unsigned int>(layer));
    if (!record)
        return "the layer's record";
    record->setPlaneMap(static_cast<unsigned char>(settings.planeMap));
    record->setTopTiling(settings.topTiling);
    Vec2 side;
    side.x = settings.sideTiling[0];
    side.y = settings.sideTiling[1];
    record->setSideTiling(side);
    record->setYOffset(settings.yOffset);
    Vec2 colorTiling;
    colorTiling.x = settings.colorTiling[0];
    colorTiling.y = settings.colorTiling[1];
    record->setColorTextureTiling(colorTiling);
    record->setLowDetailType(settings.lowDetailType);
    record->setEnvMap(settings.envMap);
    // The plugin's SetColor (census §8: record +0x14, packed r | g << 8 | b << 16) - the slot Terrain.h still names
    // setEditableInt68.
    record->setEditableInt68(settings.color[0] | (settings.color[1] << 8) | (settings.color[2] << 16));
    // The colour texture paintColorFromTexture samples (TerrainDetailTextureEditable::setColorTextureName loads it). An
    // emptied name is not sent: the renderer would look for a texture named "".
    if (!settings.colorTexture.empty())
        record->setColorTextureName(vc71::string(settings.colorTexture.c_str()));
    return settings.material.empty() ? std::string() : std::string("the material");
}

} // namespace bf2editor
