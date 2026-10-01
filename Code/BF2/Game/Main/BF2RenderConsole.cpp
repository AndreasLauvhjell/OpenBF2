// Client console bindings for renderer.listScreenModes and renderer.screenMode.
// The renderer DLL registers additional commands under the same console object.
// [CHOSEN] [PLATFORM] These bindings are client-only here; the dedicated ELF retains
// inactive bindings, so its error text differs from this build's unknown-command response.

#include "../../Core/BuildConfig.h"

#if !DEDICATED

#include "../../Core/ConsoleObjectMacro.h"
#include "BF2ClientSeams.h"             // dice::hfe::BF2Render and g_bf2Render (ds:0x9A5580)

#include <string>

namespace dice {
namespace hfe {

namespace {

// Stands in for the original's file-scope list (ELF 0x8B1704C, shared with maplist and sv, whose
// tree halves keep their own tags too). ModuleGame::init must register it - see the accessor below.
const char* const kBF2RenderConsoleTag = "hfe.renderer";

// ConsoleClass607's target, read from the PE executor 0x428CFB: `if (argCount == 0)
// { hasReturnValue = 0; g_bf2Render->+0x24(); }`. The receiver is NOT re-tested there - the
// null case is isObjectActive's (0x428D87, `g_bf2Render != 0`), upstream, exactly as below.
void rendererListScreenModes()
{
    ::dice::hfe::g_bf2Render->listScreenModes();                    // +0x24
}

} // namespace

// SetConsoleClassDefaults("renderer", 2, 0) - PE 0x84381D, ELF 0x87B2573.
BF2_CONSOLE_OBJECT("renderer", 2, 0)

// Builds the renderer's mode list when a renderer exists. The original listing
// output is compiled out, so this zero-argument command prints nothing.
BF2_CONSOLE_METHOD_0_STATIC_ACTIVE(listScreenModes, rendererListScreenModes,
                                   ::dice::hfe::g_bf2Render != 0, kBF2RenderConsoleTag)

// Gets or selects the renderer's display-mode index. An out-of-range write is fatal;
// 1 is the initial unselected value, not a valid mode to set.
BF2_CONSOLE_PROPERTY(screenMode, int, ::dice::hfe::g_bf2Render, setDisplayMode, getDisplayModeIndex,
                     kBF2RenderConsoleTag)

// Handed to the console registry by ModuleGame::init / close (Core/ModuleRegistry.cpp) beside the
// other halves of the 0x8B1704C list, under `#if !DEDICATED`.
ConsoleObjectList& bf2RenderConsoleObjects()
{
    return consoleBindingList(kBF2RenderConsoleTag);
}

} // namespace hfe
} // namespace dice

#endif // !dedicated
