// Platform drawing interface for the dedicated-server status display.
// Linux uses ncurses and Windows uses console APIs. Keep virtual declaration order
// with the destructor first; StatusMonitor dispatches through this interface.
#ifndef DICE_HFE_GAME_MAIN_STATUSMONITORIMPL_H
#define DICE_HFE_GAME_MAIN_STATUSMONITORIMPL_H

#include <string>

namespace dice {
namespace hfe {

class StatusMonitorImpl
{
public:
    // [CHOSEN] Inline base destructor placement; preserve its leading virtual slot.
    virtual ~StatusMonitorImpl() {}

    // Returns whether the display should redraw, normally at most twice per second.
    // When duringLoad is false, polls and forwards keyboard input; a keypress redraws
    // immediately. During loading, checks only the refresh timer.
    virtual bool shouldUpdate(bool duringLoad) = 0;

    virtual void clearScreen() = 0;

    // order: width first, height second. Proven from the posix body, not assumed - 0x8304482 loads
    // window._maxy (byte +4) and stores it through the second reference, 0x830449A loads window._maxx
    // (byte +6) and stores it through the first. Both get +1 because ncurses' _maxy/_maxx are
    // inclusive maxima, not counts. Both are set to -1 when there is no window.
    virtual void getDimensions(int& width, int& height) = 0;

    // order: x (column) first, y (row) second. Proven from `wmove(win, y, x)` at 0x83044EB, where
    // the second argument comes from arg_8 and the third from arg_4.
    virtual void outputString(int x, int y, const std::string& text) = 0;

    virtual void flushToScreen() = 0;

    // . `operator new(0x14)` then the posix constructor - so sizeof is 20 on the ELF, of
    // which the base contributes only the vptr.
    static StatusMonitorImpl* createInstance();
};

} // namespace hfe
} // namespace dice

#endif
