// Code/Tools/BF2Editor/Framework/Shortcuts.h

// The editor's keyboard shortcuts: every action the
// keyboard reaches, its default key, and the user's own keys, kept in the settings file as
// `shortcuts.<action> = Ctrl+Shift+Z` (an empty value unbinds). A chord is a key as Dear ImGui names it
// ("Z", "F5", "Delete", "1") with any of Ctrl, Shift and Alt; a "held" action (the camera's movement)
// takes a key alone and is polled while it is down. Engine-free and ImGui-free (App/ShortcutKeys maps the
// names to ImGui keys). Editor-only code (D3).
#ifndef BF2EDITOR_FRAMEWORK_SHORTCUTS_H
#define BF2EDITOR_FRAMEWORK_SHORTCUTS_H

#include "Framework/EditorConfig.h"

#include <map>
#include <string>
#include <vector>

namespace bf2editor
{

class EditorSettings;

struct KeyChord
{
    std::string key;        // "" when the action has no key
    bool        ctrl;
    bool        shift;
    bool        alt;
    KeyChord() : ctrl(false), shift(false), alt(false) {}
    bool empty() const { return key.empty(); }
    bool operator==(const KeyChord& other) const;
    bool operator!=(const KeyChord& other) const { return !(*this == other); }
};

// "Ctrl+Shift+Z" (modifiers in any order and case, "Control" too; "" or "none" is no key); false for
// anything else.
bool parseChord(const std::string& text, KeyChord& out);
// "Ctrl+Shift+Z"; "" for no key.
std::string formatChord(const KeyChord& chord);

struct ShortcutAction
{
    const char* id;             // "edit.undo"
    const char* label;          // "Undo"
    const char* group;          // "Edit"
    const char* defaultChord;   // "Ctrl+Z"
    bool        held;           // polled while down, no modifiers (the camera's movement)
};

class Shortcuts
{
public:
    Shortcuts();

    static const std::vector<ShortcutAction>& actions();
    static const ShortcutAction* action(const std::string& id);

    // The action's key now (its default until changed); an empty chord for an unknown action.
    const KeyChord& chord(const std::string& id) const;
    std::string display(const std::string& id) const { return formatChord(chord(id)); }
    // False with `error` for an unknown action, a chord that does not parse, or a held action given
    // modifiers. An empty text unbinds the action.
    bool set(const std::string& id, const std::string& chordText, std::string& error);
    void reset(const std::string& id);
    void resetAll();
    bool isDefault(const std::string& id) const;
    // The other actions with the same (non-empty) key.
    std::vector<std::string> conflicts(const std::string& id) const;

    // `shortcuts.<id>` for each action that differs from its default.
    void load(const EditorSettings& settings);
    void save(EditorSettings& settings) const;

    // Whether a key name is one the input layer knows (App/ShortcutKeys installs the check); every name
    // passes while none is installed (the checks program).
    typedef bool (*KeyValidator)(const std::string& key);
    static void setKeyValidator(KeyValidator validator);

private:
    std::map<std::string, KeyChord> m_chords;
};

// The editor's one table (the view, the menus, the camera, the Keyboard shortcuts window and the
// EditorShortcut console object all read it).
Shortcuts& editorShortcuts();

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_SHORTCUTS_H
