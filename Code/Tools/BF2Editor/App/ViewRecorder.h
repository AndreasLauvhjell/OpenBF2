// Code/Tools/BF2Editor/App/ViewRecorder.h

// Record the view: the retail host's
// Tools.StartCaptureToAvi asked for a file and recorded the view's frames until StopCaptureToAvi, through its
// 2006 engine's editor renderer. Here the frames are the world as drawn, before the editor's panels go over
// it (taken where the paused renderer takes its frame, SceneOverlay), encoded as H.264 into an MP4 file by
// Windows Media Foundation [CHOSEN: an uncompressed AVI of this window fills the 1 GB an AVI 1.0 file may hold
// in about six seconds; MP4 is what players and editors read today]. Each frame keeps the time it was drawn
// at, so the video plays at the speed the view ran. Fails visibly when Media Foundation or its H.264 encoder
// is missing. Editor-only code (D3).
#ifndef BF2EDITOR_APP_VIEWRECORDER_H
#define BF2EDITOR_APP_VIEWRECORDER_H

#include "Framework/EditorConfig.h"

#include <windows.h>

#include <string>

struct IDirect3DDevice9;
struct IMFSinkWriter;

namespace bf2editor
{

class ViewRecorder
{
public:
    ViewRecorder();
    ~ViewRecorder();

    // A recording into `path` (an .mp4); the frame size is the first frame's. False when one is running or
    // the path is not one Media Foundation can write.
    bool start(const std::string& path, std::string& error);
    // The device's back buffer as it is now, as the next frame. Errors stop the recording (logged).
    void frame(IDirect3DDevice9* device);
    // Finishes the file: "recorded N frames (T s) to <path>".
    bool stop(std::string& result);

    // At the editor's exit: a running recording is finished, then Media Foundation is shut down.
    void shutdown();

    bool recording() const { return m_recording; }
    unsigned frames() const { return m_frames; }
    const std::string& path() const { return m_path; }

private:
    bool open(unsigned width, unsigned height, std::string& error);
    void close();

    IMFSinkWriter* m_writer;
    unsigned long  m_stream;
    bool           m_recording;
    bool           m_started;       // Media Foundation started by us
    bool           m_comInitialised;
    std::string    m_path;
    unsigned       m_frames;
    unsigned       m_width;
    unsigned       m_height;
    LARGE_INTEGER  m_begin;
    long long      m_lastTime;      // the last frame's time, 100 ns units
    std::string    m_error;
};

// The editor's one recorder (the shell's menu, the console's EditorView.record and the scene overlay share it).
ViewRecorder& viewRecorder();

} // namespace bf2editor

#endif // BF2EDITOR_APP_VIEWRECORDER_H
