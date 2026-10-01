// [TEMP] [CHOSEN, not recovered] No-SDK fallback for TextureAtlasBuilder error text.
// [RETIRE-WHEN: textureatlas-dxerr9] Remove when the real DxErr9 library is available
// for all supported builds. Only diagnostic text differs; callers do not branch on it.
#if defined(BF2_IMAGE_TEXTUREATLASBUILDER)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cstdio>

extern "C" const char* __stdcall DXGetErrorString9A(HRESULT hr)
{
    // One buffer, overwritten by the next call - the real function returns a pointer to a static
    // string as well, and buildTextures copies it into a std::string at once.
    static char text[96];
    std::sprintf(text, "[CHOSEN] DxErr9.lib absent - HRESULT 0x%08lX", static_cast<unsigned long>(hr));
    return text;
}

#endif // BF2_IMAGE_TEXTUREATLASBUILDER
