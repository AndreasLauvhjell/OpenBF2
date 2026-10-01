// Path: [INFERRED]. GameServer allocates this empty sampler object and exposes it through IGame.
#ifndef GAME_MAIN_STATSMONITOR_H
#define GAME_MAIN_STATSMONITOR_H

namespace dice {
namespace hfe {

// Retail allocates exactly one byte and calls no constructor. The type is intentionally empty.
class StatsMonitor
{
};

} // namespace hfe
} // namespace dice

#endif
