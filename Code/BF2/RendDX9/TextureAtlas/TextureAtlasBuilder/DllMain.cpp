// TextureAtlasBuilder DLL entry point. Allocations use the default memory bucket.
#if defined(BF2_IMAGE_TEXTUREATLASBUILDER)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

BOOL APIENTRY DllMain(HMODULE /*module*/, DWORD /*reason*/, LPVOID /*reserved*/)
{
    return TRUE;
}

#endif // BF2_IMAGE_TEXTUREATLASBUILDER
