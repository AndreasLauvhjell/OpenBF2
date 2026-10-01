// Code/Tools/BF2Editor/App/FrozenFrame.cpp - see FrozenFrame.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/FrozenFrame.h"

#include "Framework/EditorLog.h"

#include <d3d9.h>

namespace bf2editor
{

bool FrozenFrame::capture(IDirect3DDevice9* device)
{
    if (!device)
        return false;
    IDirect3DSurface9* backBuffer = 0;
    if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)) || !backBuffer)
        return false;
    D3DSURFACE_DESC desc;
    backBuffer->GetDesc(&desc);
    if (m_texture && (m_width != desc.Width || m_height != desc.Height))
        release();
    if (!m_texture
        && FAILED(device->CreateTexture(desc.Width, desc.Height, 1, D3DUSAGE_RENDERTARGET, desc.Format, D3DPOOL_DEFAULT,
                                        &m_texture, 0)))
    {
        m_texture = 0;
        backBuffer->Release();
        logLine(LogLevel::Warning, "renderer: the frozen frame could not be kept (no %ux%u render target)", desc.Width,
                desc.Height);
        return false;
    }
    m_width = desc.Width;
    m_height = desc.Height;
    IDirect3DSurface9* target = 0;
    bool ok = SUCCEEDED(m_texture->GetSurfaceLevel(0, &target)) && target
           && SUCCEEDED(device->StretchRect(backBuffer, 0, target, 0, D3DTEXF_NONE));
    if (target)
        target->Release();
    backBuffer->Release();
    if (!ok)
        release();
    return ok;
}

void FrozenFrame::release()
{
    if (m_texture)
    {
        m_texture->Release();
        m_texture = 0;
    }
    m_width = m_height = 0;
}

} // namespace bf2editor
