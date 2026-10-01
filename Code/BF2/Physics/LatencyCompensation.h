// Shared latency-compensation controls and player-delay queries; header path [INFERRED].
// m_isEnabled defaults true and gates per-frame history publication.

#ifndef PHYSICS_LATENCYCOMPENSATION_H     // [INFERRED] guard style
#define PHYSICS_LATENCYCOMPENSATION_H

namespace dice {
namespace hfe {
namespace world {

class IPlayer;

class LatencyCompensation
{
public:
    static bool  m_isEnabled;                        // ds:0x898BA84 - static-init stores 1
    // Latency limits and local-player timing queries.
    static float getMaxLatency();
    static float getLocalLatency(IPlayer* player);
    static float getLatency(IPlayer* player);
    static float convertLatency(float latency);
};

} // namespace world

// [INFERRED] Shared latency inputs written by renderer/network code. Renderer values
// are defined in the shared latency translation unit so dedicated builds also have storage.
extern float rendererInverseFrameTime;   // ds:0xA07E50 / ELF 0x8ABA444 - 1.0f / renderer +0xA0
extern int   rendererFrameCount;         // ds:0xA07E54 / ELF 0x8ABA448 - renderer +0xA8

namespace io {
// ds:0xA07E4C / ELF 0x8ABA440, published by io::getPlayerInput from the fire trigger's sample
// latency. Lives in IO/Input/InputManager.cpp, which both images build.
extern float inputSampleLatency;
} // namespace io

} // namespace hfe
} // namespace dice

#endif // PHYSICS_LATENCYCOMPENSATION_H
