// Code/Tools/BF2Editor/App/TerrainLayerRecords.h

// A ground layer's settings onto the renderer's detail-texture record of that layer (the TerrainEditable's six
// records, ITerrainEditable +0xA0), as the retail TerrainEditor plugin set them from GroundProperties.con
// via ITerrainEditable +0x94 for the texture (which fetches it), and the rest
// through the record's own slots. Kept in its own translation unit because the record class comes from the
// renderer's Geom/Terrain.h, which is compiled here with the native plugin ABI the editor's RendDX9 uses.
// Editor-only code (D3).
#ifndef BF2EDITOR_APP_TERRAINLAYERRECORDS_H
#define BF2EDITOR_APP_TERRAINLAYERRECORDS_H

#include "Framework/EditorConfig.h"

#include <string>

namespace dice { namespace hfe { namespace geom { class ITerrainEditable; } } }

namespace bf2editor
{

struct GroundLayer;

// Applies `settings` to `layer` (0..5). Answers the settings it could not apply (empty when all were): the colour
// texture and the material are not applied yet - the first goes through a record slot that takes the renderer's
// VC7.1-layout string, the second needs the material name turned into the engine's index.
std::string applyGroundLayer(dice::hfe::geom::ITerrainEditable* editable, int layer, const GroundLayer& settings);

} // namespace bf2editor

#endif // BF2EDITOR_APP_TERRAINLAYERRECORDS_H
