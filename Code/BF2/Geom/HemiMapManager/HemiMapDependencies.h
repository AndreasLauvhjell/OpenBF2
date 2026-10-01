// Path: [CHOSEN] narrow owner-TU seams used by the attested HemiMapManager.cpp.
// Recovered collaborators call their real class methods. Unrecovered collaborator contracts stop
// at named caller frontiers in their owner TUs; none is represented by a padded positional class.

#ifndef BF2_GEOM_HEMIMAPMANAGER_HEMIMAPDEPENDENCIES_H
#define BF2_GEOM_HEMIMAPMANAGER_HEMIMAPDEPENDENCIES_H

namespace dice {
namespace hfe {
namespace geom {

void bundledMeshSetHemiEnabled(unsigned char enabled);
void staticMeshRendererSetHemiEnabled(unsigned char enabled);
void roadCompiledRendererSetHemiEnabled(unsigned char enabled);
unsigned char roadCompiledRendererGetHemiDirectFlag();
void roadCompiledRendererSetHemiDirectFlag(unsigned char value);

class ITerrainEditable;                       // Geom/ITerrainEditable.h

// Optional TerrainEditable bracket in HemiMapManager::makeHemiMap (0x1012BA82..0x1012BAB9, restored at
// 0x1012BC94..0x1012BCA1 after the state pop). The shipped code queries the live Terrain object
// (dword_1024B930) for IID 820005 (dword_1021AE1C), saves its shader technique (ITerrainEditable
// +0x8C), sets technique 0 (+0x88) during both render passes, then restores the saved technique. A
// plain geom::Terrain answers only IBase and 820004, so its QI returns null and the bracket is
// skipped - the ordinary bake path. ITerrainEditable exists.
struct HemiTerrainOverride
{
    ITerrainEditable* editable;               // null unless the terrain answers 820005
    int               previousTechnique;      // 0 unless saved
};
HemiTerrainOverride beginHemiTerrainOverride();
void endHemiTerrainOverride(const HemiTerrainOverride& state);

} // namespace geom
} // namespace hfe
} // namespace dice

#endif
