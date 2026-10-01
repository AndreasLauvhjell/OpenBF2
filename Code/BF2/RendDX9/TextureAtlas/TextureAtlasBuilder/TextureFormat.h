// Path:     [INFERRED] TextureAtlasBuilder/TextureFormat.h - no __FILE__ survives in the DLL; named for
//           its contents. The three helpers are the image's first code (TAB 0x10001000..0x100018DF).
// Contents: from TextureAtlasBuilder.dll 1.50, every body read.

#ifndef TEXTUREATLASBUILDER_TEXTUREFORMAT_H
#define TEXTUREATLASBUILDER_TEXTUREFORMAT_H

#ifndef NOMINMAX
#define NOMINMAX            // the bodies use std::min / std::max; windows.h's macros would replace them
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d9.h>
#include <string>

namespace textureatlas {

// (48 B, ECX = format): DXT1..DXT5.
bool __fastcall isDXT(D3DFORMAT format);

// the format's name without the D3DFMT_ prefix, "UNKNOWN" for anything else
// (hidden return in ECX, format in EDX). The pages' .tac/.tai never carry it; addTexture stores it in
// TextureInfo::format, which build() compares in split mode.
std::string __fastcall formatToString(D3DFORMAT format);

// (1,066 B, ECX = the name): the inverse, by exact (case-sensitive) comparison in the
// shipped order; an unknown name gives 0 (D3DFMT_UNKNOWN). buildTextures upper-cases first.
D3DFORMAT __fastcall stringToFormat(const std::string& name);

} // namespace textureatlas

#endif // TEXTUREATLASBUILDER_TEXTUREFORMAT_H
