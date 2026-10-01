// Client loading-screen HUD state. [INFERRED] Names and source placement.
#include "HudLoadingScreen.h"
#if !DEDICATED
#include "../Game/HudInformationLayer.h"
#include "../Game/Main/BF2ClientSeams.h"
#include <cstdio>
#include <cstring>
#include <string>
namespace dice { namespace hfe { namespace hud {
namespace {
// only the fixed sprintf output is observed. Capacity is [INFERRED]
// from that output, not IDA's unrelated840-byte aggregate data-item extent.
char buildNumberText[sizeof("BF2 Build #3153")];
// PE A20CF0:7AA720 copies exactly512 bytes; no forced trailing NUL.
char debugText[512];
}

HudLoadingScreen::HudLoadingScreen(meme::System* system)
    : m_system(system), m_show(false), m_progress(0.0f),
      m_mapName(nullptr), m_debug(nullptr), m_buildNumber(buildNumberText)
{
    // +0C and the alignment bytes +05..07 are deliberately not initialized.
    std::sprintf(buildNumberText, "BF2 Build #%d", 3153);
}

HudLoadingScreen::~HudLoadingScreen() {} // Actual83D270 ret; no unregister/free.

void HudLoadingScreen::registerData()
{
    // Requires the information-layer singleton. Reload it for each registration call.
    static_cast<HudInformationLayer*>(g_hudInformationLayer)->registerBool(
        std::string("ShowLoadingScreen"), &m_show);
    static_cast<HudInformationLayer*>(g_hudInformationLayer)->registerFloat(
        std::string("LoadingBarValue"), &m_progress);
    static_cast<HudInformationLayer*>(g_hudInformationLayer)->registerString(
        std::string("LoadingPicturePathString"), &m_picture);
    static_cast<HudInformationLayer*>(g_hudInformationLayer)->registerString(
        std::string("LoadingScreenMapName"), &m_mapName);
    static_cast<HudInformationLayer*>(g_hudInformationLayer)->registerString(
        std::string("LoadingScreenDebug"), &m_debug);
    static_cast<HudInformationLayer*>(g_hudInformationLayer)->registerString(
        std::string("LoadingScreenBuildNr"), &m_buildNumber);
    m_picture = "Load/default_loadscreen.tga";
}

void HudLoadingScreen::setProgress(float percent)
{
    // 0.01f is PE892ADC bits3C23D70A. x87 keeps the product above float
    // precision until the final store; do not use a float-rounded product.
    m_progress = static_cast<float>(1.0 - static_cast<double>(percent) *
                                         static_cast<double>(0.01f));
}

void HudLoadingScreen::setDebug(const char* text)
{
    std::strncpy(debugText, text, sizeof(debugText));
    m_debug = debugText;
}
} } }
#endif
