// Code/Tools/BF2Editor/Framework/DdsImage.h

// A DDS texture's top level on the CPU: read as 32-bit bgra from the formats the game's terrain maps use (32-, 24- and
// 16-bit RGB by their bit masks, DXT1, DXT3, DXT5), scaled, and written back as an uncompressed A8R8G8B8 DDS with one
// level. The editable terrain reads its editor detail maps as A8R8G8B8 of the terrain's detail-map size (512, the
// renderer's default), while a level's game detail maps are compressed and smaller (Karkand: 256, R5G6B5 or DXT1), so
// the editor converts them when it stages them (App/EditableTerrain). Engine-free (checked in bf2_editor_checks).
// Editor-only code (D3).
#ifndef BF2EDITOR_FRAMEWORK_DDSIMAGE_H
#define BF2EDITOR_FRAMEWORK_DDSIMAGE_H

#include "Framework/EditorConfig.h"

#include <string>
#include <vector>

namespace bf2editor
{

struct DdsImage
{
    int                   width = 0;
    int                   height = 0;
    std::vector<unsigned> pixels;       // 0xAARRGGBB per texel (the A8R8G8B8 dword), rows top to bottom

    // The file's top level; false with `error` for a format it does not read.
    bool read(const std::string& bytes, std::string& error);
    // Bilinear to `w` x `h`.
    DdsImage scaled(int w, int h) const;
    // An A8R8G8B8 DDS with one level.
    std::string write() const;
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_DDSIMAGE_H
