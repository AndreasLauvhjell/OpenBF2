// Path:     [INFERRED] TextureAtlasBuilder/TextureAtlas.h - the DLL's atlas base class. Not to be confused
//           with RendDX9's own TextureAtlas (the .tai reader, ../TextureAtlas.h): this one is in
//           namespace textureatlas and lives only in TextureAtlasBuilder.dll.
// Contents: from TextureAtlasBuilder.dll 1.50; layout read from the constructor
//           (0x10002770), the destructor (0x100073D0) and every member access in build/save.

#ifndef TEXTUREATLASBUILDER_TEXTUREATLAS_H
#define TEXTUREATLASBUILDER_TEXTUREATLAS_H

#include "ITextureAtlas.h"

#include <string>
#include <vector>

namespace textureatlas {


// Platform-neutral atlas packing and .tai writing; the derived renderer supplies pages and limits.

class TextureAtlas : public ITextureAtlas
{
public:
    TextureAtlas();
    virtual ~TextureAtlas();                                    // (deleting: 0x100073A0)

    virtual void build(ITextureAtlasConfiguration* configuration);  // +0x04
    virtual bool save(const char* taiPath);                     // +0x08
    virtual void setSortMode(int mode);                         // +0x0C
    virtual void setErrorCallback(MessageFn callback);          // +0x10
    virtual void setWarningCallback(MessageFn callback);        // +0x14

    virtual bool buildTextures();                               // +0x18 0x10007300 - `return true`
    virtual int  maxWidth();                                    // +0x1C
    virtual int  maxHeight();                                   // +0x20

protected:
    std::string              m_outputPath;
    std::vector<Group>       m_pages;
    std::vector<TextureInfo> m_pending;
    int                      m_sortMode;
    MessageFn                m_error;
    MessageFn                m_warning;
};

// (23 B, __fastcall): the smallest power of two not below `value` (1 for value <= 1).
// Every call is inlined in the 1.50 image (build's page rounding, 0x10002D9C..0x10002DC9), which still
// keeps this out-of-line copy; the 2006 editor build calls it (E9 0x1005C8E0).
int __fastcall nextPowerOfTwo(int value);

} // namespace textureatlas

#endif // TEXTUREATLASBUILDER_TEXTUREATLAS_H
