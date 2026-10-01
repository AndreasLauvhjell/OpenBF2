// Code/Tools/BF2Editor/App/ShortcutKeys.h

// The editor's one shortcut table (Framework/Shortcuts) and its Dear ImGui side: key names to ImGui
// keys, "was this action's chord pressed", "is this held action's key down", and the capture of a new
// chord for the Keyboard shortcuts window. A chord matches only with exactly its modifiers, so Ctrl+Z
// and Ctrl+Shift+Z stay apart. Editor-only code (D3).
#ifndef BF2EDITOR_APP_SHORTCUTKEYS_H
#define BF2EDITOR_APP_SHORTCUTKEYS_H

#include "Framework/EditorConfig.h"

#include "Framework/Shortcuts.h"

#include <string>

namespace bf2editor
{

inline Shortcuts& shortcuts() { return editorShortcuts(); }

// Installs the key-name check into the table (at start-up).
void installShortcutKeys();
// While the Keyboard shortcuts window captures a key, no shortcut fires.
void suspendShortcuts(bool suspended);

// The ImGuiKey of a key name as ImGui spells it ("Z", "F5", "Delete"); 0 (ImGuiKey_None) when there is
// none.
int imguiKeyOf(const std::string& name);

// The action's chord went down this frame (no key repeat), with exactly its modifiers. Chords with Ctrl
// or Alt are ignored while text is typed; plain keys only when the view has the keyboard
// (keyboardForView()) - or, `anywhere`, whenever no text is typed (F11's full view).
bool shortcutPressed(const char* id, bool anywhere = false);
// A held action's key is down (the camera's movement).
bool shortcutDown(const char* id);

// The chord just pressed, for the Keyboard shortcuts window's capture: a named key other than a
// modifier, with the modifiers held; false when none went down this frame.
bool chordPressed(KeyChord& out);

} // namespace bf2editor

#endif // BF2EDITOR_APP_SHORTCUTKEYS_H
