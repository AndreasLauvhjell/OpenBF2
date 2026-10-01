// Browses textures through engine findFiles and returns mod-rooted, extensionless paths with backslashes.
// Uses the current value's folder or the property's default particle/decal folder.
// Loads DXT1/3/5 and 32-bit RGB DDS thumbnails incrementally into the managed pool until editor shutdown.
#ifndef BF2EDITOR_APP_TEXTUREPALETTE_H
#define BF2EDITOR_APP_TEXTUREPALETTE_H

#include "Framework/EditorConfig.h"

#include <string>

struct IDirect3DDevice9;

namespace bf2editor
{

namespace TexturePalette
{

// Whether the palette serves this property (its name without a set/get prefix, any case).
bool serves(const std::string& property);
// The device thumbnails are made on (EditorApp, once the renderer is up; 0 before the renderer goes).
void setDevice(IDirect3DDevice9* device);
// The button beside a texture property's text and its popup; true when a texture was picked (`out` is the value).
bool button(const char* id, const std::string& property, const std::string& value, std::string& out);
// Every thumbnail released (before the device goes).
void releaseAll();

} // namespace TexturePalette

} // namespace bf2editor

#endif // BF2EDITOR_APP_TEXTUREPALETTE_H
