// Draws pass 17 as a source-to-destination blit, bracketed by StateManager.
// Other private pass and parameter paths are not implemented.

#ifndef BF2_GEOMMISC_FSQUADDRAWER_H
#define BF2_GEOMMISC_FSQUADDRAWER_H

namespace dice {
namespace hfe {

namespace dx9rend {
struct EngineTexture;
}

namespace geom {

class FSQuadDrawer;

// dword_10249978 in RendDX9.dll.  ModuleGeomMisc::init publishes it and close destroys it.  Retail
// close deliberately leaves the pointer value behind after freeing the object; the implementation
// preserves that writeback quirk.
extern FSQuadDrawer* g_FSQuadDrawer;

// The allocation/construction and destruction/free halves of ModuleGeomMisc::init/close.
void initializeFSQuadDrawer();
void destroyFSQuadDrawer();

// PE HemiMapManager::makeHemiMap passes (17, source, destination, true, 0) to the private general
// FSQuadDrawer submission method.  This is that exact, deliberately narrow operation.
void fsQuadDrawerDrawHemiMap(dx9rend::EngineTexture* source,
                             dx9rend::EngineTexture* destination);

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_GEOMMISC_FSQUADDRAWER_H
