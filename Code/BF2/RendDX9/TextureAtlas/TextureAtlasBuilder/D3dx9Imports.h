// Legacy D3DX declarations absent from modern Windows SDKs.
// Alias pragmas bridge stdcall names only when the SDK's decorated import is unavailable.

#ifndef TEXTUREATLASBUILDER_D3DX9IMPORTS_H
#define TEXTUREATLASBUILDER_D3DX9IMPORTS_H

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <d3d9.h>

// d3dx9tex.h, April 2005 (D3DX_SDK_VERSION 25).
#define TEXTUREATLAS_D3DX_DEFAULT       ((UINT)-1)       // D3DX_DEFAULT
#define TEXTUREATLAS_D3DX_FILTER_NONE   (1 << 0)         // D3DX_FILTER_NONE
#define TEXTUREATLAS_D3DXIFF_DDS        4                // D3DXIFF_DDS

// D3DXIMAGE_INFO - 28 bytes; getTextureInfo reads Width (+0x00), Height (+0x04), MipLevels (+0x0C)
// and Format (+0x10) out of its stack copy (TAB 0x100082D0..0x100082FA).
struct TextureAtlasD3dxImageInfo
{
    UINT            Width;
    UINT            Height;
    UINT            Depth;
    UINT            MipLevels;
    D3DFORMAT       Format;
    D3DRESOURCETYPE ResourceType;
    UINT            ImageFileFormat;    // D3DXIMAGE_FILEFORMAT
};

extern "C" HRESULT __stdcall D3DXGetImageInfoFromFileA(const char* pSrcFile,
                                                       TextureAtlasD3dxImageInfo* pSrcInfo);
extern "C" HRESULT __stdcall D3DXCreateTexture(IDirect3DDevice9* pDevice, UINT Width, UINT Height,
                                               UINT MipLevels, DWORD Usage, D3DFORMAT Format,
                                               D3DPOOL Pool, IDirect3DTexture9** ppTexture);
extern "C" HRESULT __stdcall D3DXCreateTextureFromFileExA(IDirect3DDevice9* pDevice, const char* pSrcFile,
                                                          UINT Width, UINT Height, UINT MipLevels,
                                                          DWORD Usage, D3DFORMAT Format, D3DPOOL Pool,
                                                          DWORD Filter, DWORD MipFilter, D3DCOLOR ColorKey,
                                                          TextureAtlasD3dxImageInfo* pSrcInfo,
                                                          PALETTEENTRY* pPalette,
                                                          IDirect3DTexture9** ppTexture);
extern "C" HRESULT __stdcall D3DXLoadSurfaceFromSurface(IDirect3DSurface9* pDestSurface,
                                                        const PALETTEENTRY* pDestPalette,
                                                        const RECT* pDestRect,
                                                        IDirect3DSurface9* pSrcSurface,
                                                        const PALETTEENTRY* pSrcPalette,
                                                        const RECT* pSrcRect, DWORD Filter,
                                                        D3DCOLOR ColorKey);
extern "C" HRESULT __stdcall D3DXSaveTextureToFileA(const char* pDestFile, UINT DestFormat,
                                                    IDirect3DBaseTexture9* pSrcTexture,
                                                    const PALETTEENTRY* pSrcPalette);

#pragma comment(linker, "/alternatename:_D3DXGetImageInfoFromFileA@8=_D3DXGetImageInfoFromFileA")
#pragma comment(linker, "/alternatename:_D3DXCreateTexture@32=_D3DXCreateTexture")
#pragma comment(linker, "/alternatename:_D3DXCreateTextureFromFileExA@56=_D3DXCreateTextureFromFileExA")
#pragma comment(linker, "/alternatename:_D3DXLoadSurfaceFromSurface@32=_D3DXLoadSurfaceFromSurface")
#pragma comment(linker, "/alternatename:_D3DXSaveTextureToFileA@16=_D3DXSaveTextureToFileA")

// dxerr9.h, April 2005: the HRESULT's symbolic name ("D3DERR_INVALIDCALL", "E_OUTOFMEMORY", ...).
// Third-party code; textureatlasbuilder.cmd links the SDK's DxErr9.lib when it is installed, and
// otherwise the [CHOSEN] stand-in in DxErr9StandIn.cpp.
extern "C" const char* __stdcall DXGetErrorString9A(HRESULT hr);

#endif // TEXTUREATLASBUILDER_D3DX9IMPORTS_H
