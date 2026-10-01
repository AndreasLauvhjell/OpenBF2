// Code/Tools/BF2Editor/App/FrameCapture.h

// Saves the device's back buffer as a 32-bit BMP (+screenshot, automated runs). Call it after the
// frame is drawn and before Present: with the discard swap effect the back buffer is undefined after
// Present. Uses only systemmem resources, released before returning, so it never holds anything a
// device Reset would trip over. Editor-only code (D3); RendDX9's own screenshot path is still a
// frontier (RendDX9Draw.cpp, sub_10005200).
#ifndef BF2EDITOR_APP_FRAMECAPTURE_H
#define BF2EDITOR_APP_FRAMECAPTURE_H

#include "Framework/EditorConfig.h"

#include <string>

struct IDirect3DDevice9;

namespace bf2editor
{

bool saveBackBufferBmp(IDirect3DDevice9* device, const std::string& path);

} // namespace bf2editor

#endif // BF2EDITOR_APP_FRAMECAPTURE_H
