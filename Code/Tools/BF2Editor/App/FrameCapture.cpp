// Code/Tools/BF2Editor/App/FrameCapture.cpp - see FrameCapture.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/FrameCapture.h"

#include "Framework/EditorLog.h"

#include <windows.h>
#include <d3d9.h>

#include <cstdio>
#include <cstring>
#include <vector>

namespace bf2editor
{

namespace
{

bool writeBmp(const std::string& path, int width, int height, const std::vector<unsigned char>& bgra)
{
    BITMAPFILEHEADER file;
    BITMAPINFOHEADER info;
    std::memset(&file, 0, sizeof(file));
    std::memset(&info, 0, sizeof(info));
    const DWORD pixels = static_cast<DWORD>(bgra.size());
    file.bfType = 0x4D42;                                       // "BM"
    file.bfOffBits = sizeof(file) + sizeof(info);
    file.bfSize = file.bfOffBits + pixels;
    info.biSize = sizeof(info);
    info.biWidth = width;
    info.biHeight = -height;                                    // top-down rows
    info.biPlanes = 1;
    info.biBitCount = 32;
    info.biCompression = BI_RGB;
    info.biSizeImage = pixels;

    FILE* out = std::fopen(path.c_str(), "wb");
    if (!out)
        return false;
    const bool ok = std::fwrite(&file, sizeof(file), 1, out) == 1
                 && std::fwrite(&info, sizeof(info), 1, out) == 1
                 && std::fwrite(&bgra[0], 1, bgra.size(), out) == bgra.size();
    return std::fclose(out) == 0 && ok;
}

} // namespace

bool saveBackBufferBmp(IDirect3DDevice9* device, const std::string& path)
{
    if (!device)
        return false;
    IDirect3DSurface9* back = 0;
    if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back)) || !back)
    {
        logLine(LogLevel::Error, "screenshot: no back buffer");
        return false;
    }
    D3DSURFACE_DESC desc;
    back->GetDesc(&desc);
    bool ok = false;
    if (desc.Format != D3DFMT_A8R8G8B8 && desc.Format != D3DFMT_X8R8G8B8)
    {
        logLine(LogLevel::Error, "screenshot: back buffer format %d is not 32-bit RGB", static_cast<int>(desc.Format));
    }
    else
    {
        IDirect3DSurface9* copy = 0;
        if (SUCCEEDED(device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM,
                                                          &copy, 0))
            && SUCCEEDED(device->GetRenderTargetData(back, copy)))
        {
            D3DLOCKED_RECT locked;
            if (SUCCEEDED(copy->LockRect(&locked, 0, D3DLOCK_READONLY)))
            {
                std::vector<unsigned char> pixels(static_cast<size_t>(desc.Width) * desc.Height * 4);
                for (UINT y = 0; y < desc.Height; ++y)
                {
                    unsigned char* row = &pixels[static_cast<size_t>(y) * desc.Width * 4];
                    std::memcpy(row, static_cast<const unsigned char*>(locked.pBits) + y * locked.Pitch,
                                desc.Width * 4);
                    for (UINT x = 0; x < desc.Width; ++x)
                        row[x * 4 + 3] = 255;                       // X8R8G8B8 leaves alpha undefined
                }
                copy->UnlockRect();
                ok = writeBmp(path, static_cast<int>(desc.Width), static_cast<int>(desc.Height), pixels);
            }
        }
        if (copy)
            copy->Release();
        if (!ok)
            logLine(LogLevel::Error, "screenshot: could not copy the back buffer to %s", path.c_str());
        else
            logLine(LogLevel::Info, "screenshot: %ux%u -> %s", desc.Width, desc.Height, path.c_str());
    }
    back->Release();
    return ok;
}

} // namespace bf2editor
