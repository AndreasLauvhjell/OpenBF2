// Demo configuration parser and diagnostic output. getAspect returns height/width.
// The output operator is not round-trippable: several keys lack a following space.
// Unknown input tokens warn and continue; rem consumes the rest of its line.

#include "DemoConfig.h"

#include <fstream>
#include <istream>
#include <ostream>
#include <cctype>       // tolower

#include "../../Core/Debug.h"            // dice::hfe::Debug, DEBUGTYPE_6
#include "../../Core/StringUtil.h"       // getFixPath, getFixPathDelimiters
#include "../../IO/File/FileManager.h"   // dice::hfe::io::fileManager (global), fileExists

namespace dice {
namespace hfe {

namespace {

// stream printers for the enums (file-local)
std::ostream& operator<<(std::ostream& os, DemoConfig::DemoMode m)
{
    switch (m) {
    case DemoConfig::PLAYBACK:    os << "playback";    break;
    case DemoConfig::RECORD:      os << "record";      break;
    case DemoConfig::RENDER:      os << "render";      break;
    case DemoConfig::RENDERSOUND: os << "rendersound"; break;
    default:
        Debug(DEBUGTYPE_6, "Game/DemoRecorder/DemoConfig.cpp", 83)      // line 83 attested
            << "Invalid demoMode " << (int)m;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, DemoConfig::CameraMode m)
{
    switch (m) {
    case DemoConfig::USER:            os << "user";     break;
    case DemoConfig::CAMERA_RECORD:   os << "record";   break;
    case DemoConfig::CAMERA_PLAYBACK: os << "playback"; break;
    default:
        Debug(DEBUGTYPE_6, "Game/DemoRecorder/DemoConfig.cpp", 120)     // line 120 attested
            << "Invalid cameraMode " << (int)m;
    }
    return os;
}

// scalar extractors (file-local)
std::string demoGetString(std::istream& is)
{
    is >> std::ws;
    std::string s;
    std::getline(is, s, is.widen('\n'));
    std::string::size_type n = s.size();
    while (n > 0 && (s[n-1] == ' ' || s[n-1] == '\t')) --n;  // trim trailing sp/tab only
    s.erase(n);
    return s;
}

std::string demoGetPath(std::istream& is)
{
    return getFixPathDelimiters(demoGetString(is), '/');
}

int   demoGetInt(std::istream& is)   { int v = 0;      is >> std::ws >> v; return v; }
bool  demoGetBool(std::istream& is)  { bool v = false; is >> std::ws >> v; return v; }
float demoGetFloat(std::istream& is) { float v = 0.0f; is >> std::ws >> v; return v; }

DemoConfig::DemoMode demoGetDemoMode(std::istream& is)
{
    std::string s = demoGetString(is);
    for (std::string::size_type i = 0; i < s.size(); ++i) s[i] = (char)tolower((unsigned char)s[i]);
    if (s == "playback")    return DemoConfig::PLAYBACK;
    if (s == "record")      return DemoConfig::RECORD;
    if (s == "render")      return DemoConfig::RENDER;
    if (s == "rendersound") return DemoConfig::RENDERSOUND;
    Debug(DEBUGTYPE_6, "Game/DemoRecorder/DemoConfig.cpp", 104)        // line 104 attested
        << "Invalid demoMode: " << s;
    return DemoConfig::PLAYBACK;
}

DemoConfig::CameraMode demoGetCameraMode(std::istream& is)
{
    std::string s = demoGetString(is);
    for (std::string::size_type i = 0; i < s.size(); ++i) s[i] = (char)tolower((unsigned char)s[i]);
    if (s == "playback") return DemoConfig::CAMERA_PLAYBACK;
    if (s == "record")   return DemoConfig::CAMERA_RECORD;
    if (s == "user")     return DemoConfig::USER;
    Debug(DEBUGTYPE_6, "Game/DemoRecorder/DemoConfig.cpp", 140)        // line 140 attested
        << "Invalid cameraMode: " << s;
    return DemoConfig::CAMERA_PLAYBACK;   // default on invalid (value 2)
}

} // anonymous namespace

// DemoConfig
// 18 immediate stores, defaults proven at each cited address in the extraction record.
DemoConfig::DemoConfig()
    : demoMode(PLAYBACK)
    , cameraMode(USER)
    , demoFile("")
    , cameraFile("")
    , renderFile("")
    , renderWidth(400)
    , renderHeight(300)
    , renderFps(30)
    , renderFieldOfView(90.0f)
    , renderCompressed(true)
    , renderPanorama(false)
    , numPanoramaSlices(0)
    , syncCameraAndDemo(true)
    , timeDemo(false)
    , renderStats(0)
    , vtuneFrame(-1)
    , vtuneRendering(false)
    , vtuneRepeatCount(1)
{
}

// compiler-generated destruction of the three COW std::string members (reverse order).
DemoConfig::~DemoConfig()
{
}

// renderHeight/renderWidth: the inverse of a conventional aspect ratio. Preserved.
float DemoConfig::getAspect() const
{
    return (float)renderHeight / (float)renderWidth;
}

// getAspect() * radians(renderFieldOfView). 0.017453292f == pi/180.
float DemoConfig::getFOV() const
{
    float rad = 0.017453292f * renderFieldOfView;
    return getAspect() * rad;
}

// private helper; emits the failure Debug and always returns false.
bool DemoConfig::fail(const std::string& msg) const
{
    Debug(DEBUGTYPE_6, "Game/DemoRecorder/DemoConfig.cpp", 231)        // line 231 attested
        << "DemoConfig sanity check failure: " << msg;
    return false;
}

// all referenced files must exist for the selected mode.
bool DemoConfig::sanityCheck() const
{
    if (demoMode == RENDER || demoMode == RENDERSOUND)
    {
        if (!io::fileManager->fileExists(demoFile))
            return fail(demoFile + " does not exist");
        if (!io::fileManager->fileExists(cameraFile))
            return fail(cameraFile + " does not exist");
    }
    if (demoMode != PLAYBACK)
        return true;
    if (!io::fileManager->fileExists(demoFile))
        return fail(demoFile + " does not exist");
    if (cameraMode == CAMERA_PLAYBACK && !io::fileManager->fileExists(cameraFile))
        return fail(cameraFile + " does not exist");
    return true;
}

// Diagnostic dump, not a round-trip format. Streams enum values as integers and
// preserves keys without trailing spaces and the original field order.
std::ostream& operator<<(std::ostream& os, const DemoConfig& c)
{
    os << "demo.demomode"           << (int)c.demoMode     << std::endl;   // raw int (dead printer)
    os << "demo.cameramode"         << (int)c.cameraMode   << std::endl;   // raw int (dead printer)
    os << "demo.demofile"           << c.demoFile          << std::endl;
    os << "demo.camerafile"         << c.cameraFile        << std::endl;
    os << "demo.renderfile"         << c.renderFile        << std::endl;
    os << "demo.renderwidth "       << c.renderWidth       << std::endl;
    os << "demo.renderheight "      << c.renderHeight      << std::endl;
    os << "demo.renderfps "         << c.renderFps         << std::endl;
    os << "demo.renderfieldofview " << c.renderFieldOfView << std::endl;
    os << "demo.rendercompressed "  << c.renderCompressed  << std::endl;
    os << "demo.renderpanorama "    << c.renderPanorama    << std::endl;
    os << "demo.numpanoramaslices " << c.numPanoramaSlices << std::endl;
    os << "demo.timedemo "          << c.timeDemo          << std::endl;
    os << "demo.renderstats "       << c.renderStats       << std::endl;
    os << "demo.vtuneframe "        << c.vtuneFrame        << std::endl;
    os << "demo.vtunerendering "    << c.vtuneRendering    << std::endl;   // before vtunerepeatcount
    os << "demo.vtunerepeatcount "  << c.vtuneRepeatCount  << std::endl;
    os << "demo.synccameraanddemo"  << c.syncCameraAndDemo << std::endl;   // NO trailing space
    return os;
}

// config parser. Field names come from these exact keys. Dispatch order is verbatim
// (synccameraanddemo is checked before renderpanorama). A bad token warns and continues.
std::istream& operator>>(std::istream& is, DemoConfig& c)
{
    while (is.good())
    {
        std::string t;
        is >> std::ws >> t;
        for (std::string::size_type i = 0; i < t.size(); ++i) t[i] = (char)tolower((unsigned char)t[i]);
        if      (t == "demo.demomode")          c.demoMode          = demoGetDemoMode(is);
        else if (t == "demo.cameramode")        c.cameraMode        = demoGetCameraMode(is);
        else if (t == "demo.demofile")          c.demoFile          = demoGetPath(is);
        else if (t == "demo.camerafile")        c.cameraFile        = demoGetPath(is);
        else if (t == "demo.renderfile")        c.renderFile        = demoGetPath(is);
        else if (t == "demo.renderwidth")       c.renderWidth       = demoGetInt(is);
        else if (t == "demo.renderheight")      c.renderHeight      = demoGetInt(is);
        else if (t == "demo.renderfps")         c.renderFps         = demoGetInt(is);
        else if (t == "demo.renderfieldofview") c.renderFieldOfView = demoGetFloat(is);
        else if (t == "demo.rendercompressed")  c.renderCompressed  = demoGetBool(is);
        else if (t == "demo.synccameraanddemo") c.syncCameraAndDemo = demoGetBool(is);
        else if (t == "demo.renderpanorama")    c.renderPanorama    = demoGetBool(is);
        else if (t == "demo.numpanoramaslices") c.numPanoramaSlices = demoGetInt(is);
        else if (t == "demo.timedemo")          c.timeDemo          = demoGetBool(is);
        else if (t == "demo.renderstats")       c.renderStats       = demoGetInt(is);
        else if (t == "demo.vtuneframe")        c.vtuneFrame        = demoGetInt(is);
        else if (t == "demo.vtunerendering")    c.vtuneRendering    = demoGetBool(is);
        else if (t == "demo.vtunerepeatcount")  c.vtuneRepeatCount  = demoGetInt(is);
        else if (t.empty() || t == "rem")       demoGetString(is);   // comment: eat rest of line
        else
            Debug(DEBUGTYPE_6, "Game/DemoRecorder/DemoConfig.cpp", 308)   // line 308 attested
                << "Syntax error in demo config: " << t;
    }
    if (c.demoMode == DemoConfig::RENDER && !c.renderCompressed)
        c.renderFile = getFixPath(c.renderFile, '/');
    return is;
}

// normalise the path, open for reading, parse via operator>>.
bool loadDemoConfig(DemoConfig* config, const std::string& filename)
{
    std::string path = getFixPathDelimiters(filename, '\\');
    if (path.empty())
        return false;
    std::ifstream file(path.c_str());   // ios::in
    if (!file)
        return false;
    file >> *config;
    return true;
}

} // namespace hfe
} // namespace dice
