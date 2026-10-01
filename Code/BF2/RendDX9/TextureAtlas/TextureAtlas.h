// Path:     [INFERRED] header name in the attested directory Code/BF2/RendDX9/TextureAtlas/. The
//           implementation file is attested - TextureAtlas.cpp is quoted with lines 62 and 75 by
//           its own loadTAI diagnostics - but no __FILE__ names its header.
// Contents: the 20-byte TextureAtlas, recovered from RendDX9.dll. One atlas object per
//           .tai file; the TextureManager's atlas cache (TextureManager.cpp) owns a vector of them.

#ifndef BF2_RENDDX9_TEXTUREATLAS_TEXTUREATLAS_H
#define BF2_RENDDX9_TEXTUREATLAS_TEXTUREATLAS_H

#include "../../../RendDX9Abi/Vc71String.h"
#include "../../../RendDX9Abi/Vc71Map.h"

#include <list>
#include <vector>

namespace dice {
namespace hfe {
namespace dx9rend {

struct EngineTexture;

// The 20-byte per-entry record at node+40 of the atlas's own map (0x40-byte nodes):
//     +0x00 width  +0x04 height  +0x08 uOffset  +0x0C vOffset  +0x10 the atlas page texture
// The .tai line supplies them as (uOffset, vOffset, width, height) and loadTAI stores them
// re-ordered exactly like this; Meme::Texture::setTexture reads all five.
struct TextureAtlasEntry
{
    float          width;      // +0x00
    float          height;     // +0x04
    float          uOffset;    // +0x08
    float          vOffset;    // +0x0C
    EngineTexture* page;       // +0x10
};


// dice::hfe TextureAtlas - 20 bytes (0x14), ONE vptr:
//     +0x00  vptr        off_101DE47C
//     +0x04  refCount    ctor: 1
//     +0x08  entry map   12 bytes {comp, head, size} - 0x40-byte nodes (sub_100163E0 allocates
//                        0x40; _Color/_Isnil at +60/+61), NOT the common 0x30 shape

struct TextureAtlas
{
    void*            vptr;      // +0x00
    int              refCount;  // +0x04
    vc71::map_layout entries;   // +0x08
};

static_assert(sizeof(TextureAtlas) == 0x14, "AtlasCache::loadAtlases allocates 0x14 per atlas");

TextureAtlas*      textureAtlasCtor(void* storage);
void               textureAtlasDtor(TextureAtlas* atlas);
bool               textureAtlasLoadTAI(TextureAtlas* atlas,
                                       const vc71::string& path);          // TextureAtlas_loadTAI, 0x10015970
TextureAtlasEntry* textureAtlasFind(TextureAtlas* atlas,
                                    const vc71::string& name);
// Releases entry page references and empties the map while retaining its sentinel.
void               textureAtlasReleaseAll(TextureAtlas* atlas);
// Appends distinct pages in map order to the caller-owned vector.
void               textureAtlasCollectPages(TextureAtlas* atlas,
                                            std::vector<EngineTexture*>& pages);
// Appends normalized entry names in map order.
void               textureAtlasCollectNames(TextureAtlas* atlas,
                                            std::vector<vc71::string>& names);


// Generates <outputDirectory><atlasName>.tai and numbered DDS pages from source names.
// Returns false only for an empty source list; builder diagnostics do not affect the result.
bool               textureAtlasBuild(TextureAtlas* atlas, vc71::string outputDirectory,
                                     vc71::string atlasName, vc71::string inputDirectory,
                                     const std::list<vc71::string>& textures, bool dxt5);
// the entry the callers use (Overgrowth.generateAtlas / save /
// reload, DecalManager.generateTexture, the undergrowth rebuild): copies its three strings BY value
// and forwards to the builder.
bool               textureAtlasGenerate(TextureAtlas* atlas, vc71::string outputDirectory,
                                        vc71::string atlasName, vc71::string inputDirectory,
                                        const std::list<vc71::string>& textures, bool dxt5);
// generate("", name, "", textures, false). No caller in the 1.50 image.
bool               textureAtlasGenerate(TextureAtlas* atlas, const vc71::string& atlasName,
                                        const std::list<vc71::string>& textures);

} // namespace dx9rend
} // namespace hfe
} // namespace dice

#endif // BF2_RENDDX9_TEXTUREATLAS_TEXTUREATLAS_H
