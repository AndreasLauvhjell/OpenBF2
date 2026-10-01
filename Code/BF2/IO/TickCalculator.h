// Converts an absolute clock reading in seconds into a bounded count of fixed simulation ticks.
// The constructor derives tickLength from frequency; compute advances time in whole ticks.
// Header and field names are [INFERRED]. Public field access is [CHOSEN, not recovered]; callers
// read tickLength and maxTicksPerFrame directly. The object occupies 32 bytes on x86.

#ifndef IO_TICKCALCULATOR_H         // [INFERRED] guard style
#define IO_TICKCALCULATOR_H

namespace dice {
namespace hfe {

class TickCalculator
{
public:
    // the float argument is a frequency (ticks per second), not a step length — the ctor stores
    // 1.0f / ticksPerSecond. Signature frozen from _ZN4dice3hfe14TickCalculatorC1Eif; parameter
    // names [INFERRED] from the (10, 30.0f) call sites.
    TickCalculator(int maxTicksPerFrame, float ticksPerSecond);

    // Returns how many fixed ticks fit into [time, currentTime], clamped to maxTicksPerFrame — and
    // on the clamped path the entire backlog is discarded (all three clocks snap to currentTime).
    // int return attested by USE at both call sites (stored to int, signed compares throughout).
    int compute(double currentTime);

    // Layout is load-bearing; see the block comment above. All names [INFERRED].
    /* +0x00 */ float  tickLength;          // the fixed timestep, 1/30 s in every live instance
    /* +0x04 */ int    maxTicksPerFrame;    // catch-up cap, 10 in every live instance
    /* +0x08 */ double time;                // last consumed time; whole-tick granularity
    /* +0x10 */ double prevTickTime;        // write-only in this binary (= old nextTickTime)
    /* +0x18 */ double nextTickTime;        // = time + tickLength after any tick
};

} // namespace hfe
} // namespace dice

#endif // IO_TICKCALCULATOR_H
