// [TEMP] [CHOSEN, not recovered] Construction and blink-behavior fixture.
// No HUD virtual stubs, singleton registration, module initialization, or retail DLLs.
#include <cstdint>
#include <list>
#include <string>
#include <vector>
#include <cstdio>
#include <cstring>
#include <limits>
#include <new>
#include <cmath>
#include <windows.h>
#define private public
#define protected public
#ifndef HUD_BLINK_ONLY
#include "../BF2/Menu/BF2HudManager.h"
#endif
#include "../BF2/Menu/HudBlinker.h"
#undef protected
#undef private

// Isolate the retail gated/discrete null-source defect without terminating the fixture.
static unsigned long updateFault(dice::hfe::hud::HudBlinker* blinker, float dt)
{
    __try { blinker->update(dt); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return GetExceptionCode(); }
    return 0;
}

int main()
{
    using namespace dice::hfe;
    using namespace dice::hfe::hud;
    unsigned checks = 0, failures = 0;
    auto check = [&](bool ok, const char* description) {
        ++checks;
        if (!ok) { ++failures; std::printf("FAIL: %s\n", description); }
    };

    bool output = false;
    HudBlinker oscillator(nullptr, &output, 0.0f, 0.0f, 1.0f, 0.0f, false);
    oscillator.update(1.0f);
    check(!output, "period equality does not toggle");
    oscillator.update(0.125f);
    check(output, "strictly exceeding period toggles");
    oscillator.update(1.0f);
    check(output, "elapsed remainder was discarded");
    oscillator.update(0.125f);
    check(!output, "second complete period toggles back");

    float source = 2.0f;
    HudBlinker range(&source, &output, 4.0f, 1.0f, 1.0f, 0.5f, true);
    range.update(0.75f);
    check(!output, "adaptive midpoint period equality");
    range.update(0.125f);
    check(output, "adaptive period interpolates from source");
    source = 4.0f;
    range.update(1.0f);
    check(output, "maximum-threshold clear still waits strictly past period");
    range.update(0.125f);
    check(!output, "source at maximum clears active blink");
    range.update(2.0f);
    check(!output && range.m_elapsed == 2.0f, "inactive out-of-range blink keeps elapsed time");
    source = 2.0f;
    range.update(0.0f);
    check(output && range.m_elapsed == 0.0f, "returning into range uses accumulated time");
    source = 1.0f;
    range.update(2.0f);
    check(!output, "source equal to minimum clears instead of toggling");

    source = -1.0f;
    output = true;
    HudBlinker invalid(&source, &output, 4.0f, 0.0f, 1.0f, 0.0f, false);
    invalid.update(2.0f);
    check(!output, "negative source clears active blink");
    source = std::numeric_limits<float>::quiet_NaN();
    output = true;
    invalid.update(2.0f);
    check(!output, "unordered source follows PE invalid-range branch");

    output = true;
    source = 0.0f;
    HudBlinker zeroMaximum(&source, &output, 0.0f, 0.0f, 1.0f, 0.5f, true);
    zeroMaximum.update(2.0f);
    check(output, "adaptive divide occurs before zero-maximum guard (NaN period)");
    HudBlinker noOutput(&source, nullptr, 4.0f, 0.0f, 1.0f, 0.0f, false);
    noOutput.update(2.0f);
    check(noOutput.m_elapsed == 0.0f, "null output returns before accumulating elapsed time");

    source = 3.0f;
    output = false;
    HudHighBlinker high(&source, &output, 2.0f, 4.0f, 1.0f, 0.5f, true);
    high.update(1.25f);
    check(!output, "high-threshold adaptive period uses lower bound as denominator");
    high.update(0.125f);
    check(output, "high-threshold open interval toggles after period");
    source = 2.0f;
    high.update(1.0f);
    check(output, "high-threshold lower equality waits for period");
    high.update(0.125f);
    check(!output, "high-threshold lower equality clears");
    source = 4.0f;
    output = true;
    high.update(2.0f);
    check(!output, "high-threshold upper equality clears");
    source = std::numeric_limits<float>::quiet_NaN();
    output = true;
    HudHighBlinker highInvalid(&source, &output, 2.0f, 4.0f, 1.0f, 0.0f, false);
    highInvalid.update(2.0f);
    check(!output, "high-threshold unordered source clears");
    highInvalid.update(2.0f);
    check(highInvalid.m_elapsed == 2.0f, "high-threshold inactive invalid state accumulates");
    source = 3.0f;
    highInvalid.update(0.0f);
    check(output, "high-threshold return to valid range consumes accumulated time");
    HudHighBlinker highNoOutput(&source, nullptr, 2.0f, 4.0f, 1.0f, 0.0f, false);
    highNoOutput.update(2.0f);
    check(highNoOutput.m_elapsed == 0.0f, "high-threshold null output does not advance time");
    HudHighBlinker highOscillator(nullptr, &output, 0.0f, 0.0f, 1.0f, 0.0f, false);
    highOscillator.update(1.0f);
    check(output, "high-threshold source-less oscillator has strict period boundary");
    highOscillator.update(0.25f);
    check(!output, "high-threshold source-less oscillator toggles");

    bool gate = true;
    float alpha = 1.0f;
    HudFloatBlinker fade(&gate, &alpha, true, 1.0f, true);
    check(fade.m_source == nullptr && fade.m_output == nullptr && fade.m_adaptive
          && fade.m_direction == -1.0f, "float controller preserves inherited constructor state");
    fade.update(0.25f);
    check(alpha == 0.75f, "float controller initially fades down");
    fade.update(0.75f);
    check(alpha == 0.0f && fade.m_elapsed == 0.0f && fade.m_direction == 1.0f,
          "smooth period equality reverses direction");
    fade.update(0.25f);
    check(alpha == 0.25f, "float controller fades up on alternate cycle");
    fade.update(2.0f);
    check(alpha == 1.0f && fade.m_elapsed == 0.0f, "smooth overshoot discards remainder");
    gate = false;
    fade.update(0.25f);
    check(alpha == 1.0f && fade.m_elapsed == 0.25f, "inactive completed fade accumulates time");
    alpha = 0.5f;
    fade.update(0.25f);
    check(alpha == 0.5f, "inactive gate still completes an unfinished fade");
    alpha = std::numeric_limits<float>::quiet_NaN();
    fade.update(0.125f);
    check(std::isnan(alpha) && fade.m_elapsed == 0.625f, "inactive unordered output stays untouched");

    HudFloatBlinker floatNoOutput(nullptr, nullptr, true, 1.0f, true);
    floatNoOutput.update(2.0f);
    check(floatNoOutput.m_elapsed == 0.0f, "float null output does not advance time");
    alpha = 0.0f;
    HudFloatBlinker discrete(nullptr, &alpha, true, 1.0f, false);
    discrete.update(1.0f);
    check(alpha == 0.0f, "discrete float controller waits strictly past period");
    discrete.update(0.25f);
    check(alpha == 1.0f && discrete.m_elapsed == 0.0f, "discrete float controller toggles to one");
    discrete.update(1.25f);
    check(alpha == 0.0f, "discrete float controller toggles back to zero");
    discrete.m_elapsed = 1.0f;
    discrete.update(std::numeric_limits<float>::epsilon() / 4.0f);
    check(alpha == 0.0f && discrete.m_elapsed == 1.0f,
          "float elapsed sum is rounded before threshold comparison");
    HudFloatBlinker zeroPeriod(nullptr, &alpha, true, 0.0f, true);
    alpha = 0.5f;
    zeroPeriod.update(1.0f);
    check(alpha == 0.5f, "active smooth controller rejects nonpositive period");
    HudFloatBlinker inactiveZero(&gate, &alpha, true, 0.0f, true);
    inactiveZero.update(0.25f);
    check(std::isnan(alpha), "inactive gate bypasses period guard (retail zero division)");
    gate = true;
    HudFloatBlinker gatedDiscrete(&gate, &alpha, true, 1.0f, false);
    check(updateFault(&gatedDiscrete, 2.0f) == EXCEPTION_ACCESS_VIOLATION,
          "gated discrete controller preserves retail null-source read");

    {
        HudBlinkManager manager;
        manager.update(2.0f);
        check(manager.m_blinkers.empty(), "empty manager update is safe");
        bool first = false, second = false;
        manager.add(new HudBlinker(nullptr, &first, 0.0f, 0.0f, 1.0f, 0.0f, false));
        manager.add(new HudBlinker(nullptr, &second, 0.0f, 0.0f, 2.0f, 0.0f, false));
        manager.update(1.5f);
        check(first && !second, "manager dispatches each owned controller's period");
        manager.add(nullptr);
        check(manager.m_blinkers.size() == 3 && !manager.m_blinkers.back(), "add preserves null entry");
        // Retail update would fault on this null; destructor must still tolerate it.
    }

    {
        HudBlinkManager manager;
        source = 3.0f;
        output = false;
        alpha = 1.0f;
        manager.add(new HudHighBlinker(&source, &output, 2.0f, 4.0f, 0.5f, 0.0f, false));
        manager.add(new HudFloatBlinker(&gate, &alpha, true, 1.0f, true));
        manager.update(0.75f);
        check(output && alpha == 0.25f, "manager dispatches both derived retail update slots");
        // Manager retains the shipped nonvirtual base deletion; variants own no resources.
    }

#ifndef HUD_BLINK_ONLY
    // Seed storage to make the shipped read-before-initialize defect reproducible.
    // The full HUD destructor unregisters global handlers and owns unrecovered widgets;
    // do not call it from this pre-init fixture. Members are reclaimed explicitly below.
    void* storage = ::operator new(sizeof(BF2HudManager));
    std::memset(storage, 0xA5, sizeof(BF2HudManager));
    BF2HudManager* hud = new (storage) BF2HudManager;
    check(reinterpret_cast<char*>(static_cast<io::IInputFilter*>(hud)) == reinterpret_cast<char*>(hud),
          "input-filter base is primary");
    check(reinterpret_cast<char*>(static_cast<IEventListener*>(hud)) - reinterpret_cast<char*>(hud) == 4,
          "event-listener base is at +4");
    check(hud->m_field010.empty() && hud->m_field038.empty() && hud->m_field060.empty()
          && hud->m_field08c.empty() && hud->m_field0c0.empty() && hud->m_field304.empty(),
          "all six native strings are constructed");
    check(hud->m_field0e8.empty() && hud->m_field2e0.empty() && hud->m_field33c.empty()
          && hud->m_field32c.empty(), "three lists and vector start empty");
    std::uint32_t oldA = 0, oldB = 0;
    std::memcpy(&oldA, &hud->m_field2a0, sizeof(oldA));
    std::memcpy(&oldB, &hud->m_field2a8, sizeof(oldB));
    check(oldA == 0xA5A5A5A5 && oldB == 0xA5A5A5A5 && hud->m_field2a4 == 5.0f && hud->m_field2ac == 5.0f,
          "retail copies preexisting bits before setting paired defaults");
    check(hud->m_field008 == 0xA5A5A5A5 && hud->m_field29c == 0xA5A5A5A5,
          "constructor does not zero untouched storage");
    bool clear = true;
    for (std::uint32_t value : hud->m_field18c) clear = clear && value == 0;
    check(clear && hud->m_field164 == 0 && hud->m_field174 == 0 && hud->m_field254 == 0,
          "late constructor clearing completed");
    check(hud->m_field244 == -1 && hud->m_field2ec == -1 && hud->m_field264 == -4000.0f
          && hud->m_field328 == 64 && hud->m_field350 == 1.0f, "nonzero state defaults preserved");
    check(hud->m_field2dc && hud->m_field2dc->m_blinkers.size() == 1,
          "constructor owns one initial blink controller");
    HudBlinker* initial = hud->m_field2dc->m_blinkers.front();
    check(initial->m_source == &hud->m_field2b8 && initial->m_output == &hud->m_field2b0,
          "controller points into native HUD members (no retail offset arithmetic)");
    hud->m_field2dc->update(1.0f);
    check(!hud->m_field2b0, "initial source equals maximum and stays inactive");
    hud->m_field2b8 = 1.5f;
    hud->m_field2dc->update(0.0f);
    check(hud->m_field2b0, "constructed HUD controller responds to changing source");

    delete hud->m_field2dc;
    using WideString = std::wstring;
    using String = std::string;
    using WordList = std::list<int>;
    using StringList = std::list<std::string>;
    using WordVector = std::vector<std::uint32_t>;
    hud->m_field33c.~StringList();
    hud->m_field32c.~WordVector();
    hud->m_field304.~String();
    hud->m_field2e0.~StringList();
    hud->m_field0e8.~WordList();
    hud->m_field0c0.~String();
    hud->m_field08c.~String();
    hud->m_field060.~WideString();
    hud->m_field038.~WideString();
    hud->m_field010.~WideString();
    ::operator delete(storage);
    std::printf("HUD construction/blink: %u/%u checks passed (native x86 HUD size %u)\n",
                checks - failures, checks, unsigned(sizeof(BF2HudManager)));
#else
    std::printf("HUD blink controllers (strict link): %u/%u checks passed\n", checks - failures, checks);
#endif
    return failures ? 1 : 0;
}
