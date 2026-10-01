// Fixed-step tick scheduling. [INFERRED] Source placement.

#include "TickCalculator.h"

namespace dice {
namespace hfe {

// Stores the reciprocal of ticksPerSecond as the tick interval and the frame catch-up limit.
TickCalculator::TickCalculator(int maxTicksPerFrame_, float ticksPerSecond)
    : tickLength(1.0f / ticksPerSecond)
    , maxTicksPerFrame(maxTicksPerFrame_)
    , time(0.0)                             // 0x8438930 fldz; 0x8438932 fst qword [this+8]
    , prevTickTime(0.0)
    , nextTickTime(0.0)
{
}

// Returns the fixed simulation ticks due at currentTime.
// Keeps sub-tick time, but discards excess backlog when the per-frame maximum is exceeded.
int TickCalculator::compute(double currentTime)
{
    int numTicks = 0;
    double delta = currentTime - time;

    if (delta >= tickLength) // An unordered time delta produces no ticks.
    {
        while (delta - tickLength >= 0.0            // pre 0x843898A jb; post 0x84389A7 fucomip/jb
               && numTicks <= maxTicksPerFrame)     // pre 0x8438991 jg;  post 0x84389AB cmp/jle
        {                                           //   <= (not <): the count overshoots to max+1,
            delta -= tickLength;                    //   which is what arms the clamp below
            ++numTicks;
        }

        if (numTicks > 0)
        {
            prevTickTime = nextTickTime;            // 0x84389B9 read first / 0x84389C0 store +0x10
            time += numTicks * tickLength;          // 0x84389BC cvtsi2ss; 0x84389CB fmul;
                                                    // 0x84389CD faddp; 0x84389D1 store +0x08
            nextTickTime = time + tickLength;       // 0x84389D4 faddp; 0x84389D6 store +0x18
        }

        if (numTicks > maxTicksPerFrame)            // the clamp
        {
            time         = currentTime;             // 0x84389E6   backlog discarded, clocks snap
            numTicks     = maxTicksPerFrame;        // 0x84389E9   return the cap, not the true count
            prevTickTime = currentTime;
            nextTickTime = currentTime;
        }
    }

    return numTicks;
}

} // namespace hfe
} // namespace dice
