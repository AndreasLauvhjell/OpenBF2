// Dedicated-server status display backed by a platform implementation.
// Enabled by GSStatusMonitor unless +noStatusMonitor is supplied; updates run during
// steady frames, client waits and level loading.
#ifndef DICE_HFE_GAME_MAIN_STATUSMONITOR_H
#define DICE_HFE_GAME_MAIN_STATUSMONITOR_H

namespace dice {
namespace hfe {

class StatusMonitorImpl;

class StatusMonitor
{
public:
    StatusMonitor();
    ~StatusMonitor();

    // the largest single function in this class by an order of magnitude,
    // and effectively the whole class. See the parameter note on StatusMonitorImpl::shouldUpdate:
    // true means "we are inside a level load, do not read the keyboard".
    void update(bool duringLoad);

private:
    StatusMonitorImpl* m_impl;      // +0x00, and the only member. [INFERRED] name.
};

} // namespace hfe
} // namespace dice

#endif
