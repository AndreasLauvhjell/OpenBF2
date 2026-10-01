// Demo configuration embedded in DemoRecorder. Member names are [INFERRED] from
// script keys; enumerator names are [CHOSEN]. Numeric enum values are fixed, while
// STL-containing object size varies with the toolchain.

#ifndef GAME_DEMORECORDER_DEMOCONFIG_H     // [INFERRED] guard style
#define GAME_DEMORECORDER_DEMOCONFIG_H

#include <string>
#include <iosfwd>

namespace dice {
namespace hfe {

class DemoConfig
{
public:
    // frozen nested enum type names (from the anon-ns operator<< symbols
    //   ...N4dice3hfe10DemoConfig8DemoModeE / ...10CameraModeE). Values proven; identifiers [CHOSEN].
    enum DemoMode
    {
        PLAYBACK    = 0,     // "playback"
        RECORD      = 1,     // "record"
        RENDER      = 2,     // "render"
        RENDERSOUND = 3      // "rendersound"
    };
    enum CameraMode          // identifiers prefixed to avoid clashing with DemoMode in class scope
    {
        USER            = 0, // "user"
        CAMERA_RECORD   = 1, // "record"
        CAMERA_PLAYBACK = 2  // "playback"
    };

    DemoConfig();
    ~DemoConfig();                           //  (compiler-generated: 3x ~std::string)
    // copy ctor / operator= are compiler-defaulted (no explicit symbols in the binary).

    bool  sanityCheck() const;               //   [CHOSEN bool]
    float getFOV() const;                    //   [CHOSEN float]
    float getAspect() const;                 //   [CHOSEN float] - returns height/width (inverse!)

private:
    bool  fail(const std::string& msg) const;   //   [CHOSEN bool] - always false

public:
    // the 0x40 data record. [INFERRED names]; offsets and ctor defaults proven.
    DemoMode    demoMode;            // +0x00 = playback
    CameraMode  cameraMode;          // +0x04 = user
    std::string demoFile;            // +0x08 = ""
    std::string cameraFile;          // +0x0C = ""
    std::string renderFile;          // +0x10 = ""
    int         renderWidth;         // +0x14 = 400
    int         renderHeight;        // +0x18 = 300
    int         renderFps;           // +0x1C = 30
    float       renderFieldOfView;   // +0x20 = 90.0f
    bool        renderCompressed;    // +0x24 = true
    bool        renderPanorama;      // +0x25 = false
    int         numPanoramaSlices;   // +0x28 = 0
    bool        syncCameraAndDemo;   // +0x2C = true
    bool        timeDemo;            // +0x2D = false
    int         renderStats;         // +0x30 = 0
    int         vtuneFrame;          // +0x34 = -1
    bool        vtuneRendering;      // +0x38 = false
    int         vtuneRepeatCount;    // +0x3C = 1
};

std::ostream& operator<<(std::ostream& os, const DemoConfig& cfg);    //  (debug dump)
std::istream& operator>>(std::istream& is, DemoConfig& cfg);          //  (config parser)
bool loadDemoConfig(DemoConfig* config, const std::string& filename);

} // namespace hfe
} // namespace dice

#endif // GAME_DEMORECORDER_DEMOCONFIG_H
