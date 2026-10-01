// Code/Tools/BF2Editor/App/FrameProfile.h

// The Profiler panel (WP5 part 2): where each of the last few hundred editor frames spent its time,
// measured by the frame loop (EditorApp::runInteractive) with the performance counter:
//   world  - the world updates before drawing (events, the object grid, the frame-update objects);
//   ui     - building the ImGui frame (camera, panels, viewport tools and the edits they make);
//   render - the renderer's frame, present included (renderFrame, or the editor's own bracket);
//   idle   - the pacing sleep to about 60 frames a second.
// Editor-only code (D3).
#ifndef BF2EDITOR_APP_FRAMEPROFILE_H
#define BF2EDITOR_APP_FRAMEPROFILE_H

#include "Framework/EditorConfig.h"

namespace bf2editor
{

struct FramePhases
{
    float world;        // milliseconds, each
    float ui;
    float render;
    float idle;
    float total;
    FramePhases() : world(0.0f), ui(0.0f), render(0.0f), idle(0.0f), total(0.0f) {}
};

class FrameProfile
{
public:
    FrameProfile();

    void push(const FramePhases& phases);
    void draw(bool* open);

    enum { kFrames = 300 };

private:
    const FramePhases& at(int age) const;       // 0 = the newest frame

    FramePhases m_frames[kFrames];
    int         m_next;
    int         m_count;
    bool        m_paused;
};

} // namespace bf2editor

#endif // BF2EDITOR_APP_FRAMEPROFILE_H
