// Latency configuration and query helpers. [INFERRED] Source path.

#include "LatencyCompensation.h"
#include "../Game/Objects/IPlayer.h"
#include "../Game/Utility/Vars.h"

namespace dice {
namespace hfe {

// The two renderer words - dice::hfe, the namespace BF2Render::render already writes them in. See
// the header for why they are defined here and not in BF2Render.cpp.
// A dedicated image never writes either one - it has no renderer - so both keep the zero below,
// which is what makes getLocalLatency's render-ahead term vanish there even when the cvar is on.
float rendererInverseFrameTime = 0.0f;   // ds:0xA07E50, BF2.exe 0x4534B7
int   rendererFrameCount = 0;            // ds:0xA07E54, BF2.exe 0x4534CA

namespace world {

// ds:0x898BA84 - the shipped static-init stores 1: lag compensation is on by default.
bool LatencyCompensation::m_isEnabled = true;

namespace {
// _Z41__static_initialization_and_destruction_0ii_378 @ 0x835C6B0 registers the
// complete latency-compensation family in this order.  Keeping the otherwise-unread handles here
// preserves the shipped console surface while getLatency uses the five marked below.
int*   const g_useClientWindow = Vars::getInt("phy-latencyComp-useClientWindow", 1); // +0x450, used
int*   const g_useInput = Vars::getInt("phy-latencyComp-useInput", 1);               // +0x454
int*   const g_useRenderAhead = Vars::getInt("phy-latencyComp-useRenderAhead", 0);   // +0x458
float* const g_maxRenderAheadTime =
    Vars::getFloat("phy-latencyComp-maxRenderAheadTime", 0.033333335f);               // +0x45C
float* const g_pingFactor = Vars::getFloat("phy-latencyComp-pingFactor", 0.5f);       // +0x460, used
float* const g_defaultLatency = Vars::getFloat("phy-latencyComp-default", 0.05f);    // +0x464, used
float* const g_maxLatency = Vars::getFloat("phy-latencyComp-max", 0.3f);             // +0x468, used
float* const g_debugLatency = Vars::getFloat("phy-latencyComp-debug", -1.0f);        // +0x46C, used
}

// one load of the cvar handle registered at static init, then its float.
float LatencyCompensation::getMaxLatency()
{
    return *g_maxLatency;
}

// Returns local input delay plus optional render-ahead delay; null/remote players return zero.
// Only render-ahead is clamped here. [PLATFORM] Preserves ELF's addition to initial zero,
// which can differ from the PE direct load for negative zero.
float LatencyCompensation::getLocalLatency(IPlayer* player)
{
    if (!player || player->getIsRemote())               // ELF +0x5C / PE +0x58
        return 0.0f;

    float latency = 0.0f;
    if (*g_useInput)
        latency += io::inputSampleLatency;              // ds:0x8ABA440 / 0xA07E4C

    if (!*g_useRenderAhead)
        return latency;

    float renderAhead =
        static_cast<float>(rendererFrameCount) * rendererInverseFrameTime;
    if (renderAhead > *g_maxRenderAheadTime)
        renderAhead = *g_maxRenderAheadTime;
    return latency + renderAhead;
}

// Returns zero when disabled; otherwise adds default latency and clamps only the upper
// bound. Negative results remain negative. Unlike getLatency, debug latency cannot
// override the disabled state here.
float LatencyCompensation::convertLatency(float latency)
{
    if (!m_isEnabled)
        return 0.0f;

    latency += *g_defaultLatency;
    if (latency > *g_maxLatency)
        return *g_maxLatency;
    return latency;
}

// Returns player compensation with an upper bound only; negative results remain possible.
// Positive debug latency overrides disabled state. Otherwise remote players use ping/interpolation
// and local players use getLocalLatency; disabled or null players return zero.
float LatencyCompensation::getLatency(IPlayer* player)
{
    float latency = *g_debugLatency;
    if (latency <= 0.0f)
    {
        latency = 0.0f;
        if (!m_isEnabled || !player)
            return latency;

        if (player->getIsRemote())
        {
            latency = static_cast<float>(player->getPing()) * *g_pingFactor * 0.001f;
            if (*g_useClientWindow)
                latency += static_cast<float>(player->getInterpolationTime()) * 0.001f;
            else
                latency += *g_defaultLatency;
        }
        else
        {
            latency = getLocalLatency(player);                    // written above
        }
    }

    if (latency > *g_maxLatency)
        return *g_maxLatency;
    return latency;
}

} // namespace world
} // namespace hfe
} // namespace dice
