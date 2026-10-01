// Path:     [INFERRED] TextureAtlasBuilder/TextureAtlasD3D9.h - no __FILE__ survives in the DLL.
// Contents: from TextureAtlasBuilder.dll 1.50: the two Direct3D 9 leaves the
//           creates return (TAB 0x10008220..0x10008C46).

#ifndef TEXTUREATLASBUILDER_TEXTUREATLASD3D9_H
#define TEXTUREATLASBUILDER_TEXTUREATLASD3D9_H

#include "TextureAtlas.h"
#include "TextureAtlasConfiguration.h"

namespace textureatlas {

// The configuration leaf (vftable 0x1001B888): the base's 16 slots and getTextureInfo (+0x40). It adds
// no member - 0x58 bytes like the base.
class TextureAtlasConfigurationD3D9 : public TextureAtlasConfiguration
{
public:
    virtual bool getTextureInfo(const std::string& inputDirectory, const std::string& name,
                                TextureInfo& info);                         // +0x40
};


// The atlas leaf (vftable 0x1001B938) - 0x58 bytes:
//     +0x4C m_maxWidth   2048 with a device, else 0
//     +0x50 m_maxHeight
//     +0x54 m_device     AddRef'd by the constructor, Released by the destructor

class TextureAtlasD3D9 : public TextureAtlas
{
public:
    explicit TextureAtlasD3D9(IDirect3DDevice9* device);
    virtual ~TextureAtlasD3D9();                                            // (deleting 0x10008C10)

    virtual bool buildTextures();                                           // +0x18
    virtual int  maxWidth();                                                // +0x1C
    virtual int  maxHeight();                                               // +0x20

private:
    int               m_maxWidth;
    int               m_maxHeight;
    IDirect3DDevice9* m_device;
};

} // namespace textureatlas

#endif // TEXTUREATLASBUILDER_TEXTUREATLASD3D9_H
