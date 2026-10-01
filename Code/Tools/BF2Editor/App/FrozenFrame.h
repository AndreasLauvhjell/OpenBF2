// Code/Tools/BF2Editor/App/FrozenFrame.h

// The view's last world frame, kept while the renderer is paused. The scene overlay copies the back buffer here after the world is drawn and before the
// UI (SceneOverlay::setCapture), and a paused frame draws it behind the UI instead of running the
// renderer's frame. A D3DPOOL_DEFAULT render target: let go of when the device is lost, and taken again by
// the next capture. Editor-only code (D3).
#ifndef BF2EDITOR_APP_FROZENFRAME_H
#define BF2EDITOR_APP_FROZENFRAME_H

#include "Framework/EditorConfig.h"

struct IDirect3DDevice9;
struct IDirect3DTexture9;

namespace bf2editor
{

class FrozenFrame
{
public:
    FrozenFrame() : m_texture(0), m_width(0), m_height(0) {}
    ~FrozenFrame() { release(); }

    // The device's back buffer copied into the kept texture (made, or made again at a new size).
    bool capture(IDirect3DDevice9* device);
    void release();
    IDirect3DTexture9* texture() const { return m_texture; }

private:
    FrozenFrame(const FrozenFrame&);
    FrozenFrame& operator=(const FrozenFrame&);

    IDirect3DTexture9* m_texture;
    unsigned int       m_width;
    unsigned int       m_height;
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_FROZENFRAME_H
