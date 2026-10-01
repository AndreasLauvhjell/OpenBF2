// Code/Tools/BF2Editor/Framework/Shortcuts.cpp - see Shortcuts.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/Shortcuts.h"

#include "Framework/EditorSettings.h"

#include <cctype>

namespace bf2editor
{

namespace
{

std::string lowered(const std::string& text)
{
    std::string out(text);
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

std::string trimmed(const std::string& text)
{
    const size_t begin = text.find_first_not_of(" \t");
    if (begin == std::string::npos)
        return std::string();
    return text.substr(begin, text.find_last_not_of(" \t") - begin + 1);
}

// Every action the keyboard reaches, with the keys the editor had before they could be changed.
const ShortcutAction kActions[] = {
    { "file.save", "Save the level", "File", "Ctrl+S", false },
    { "edit.undo", "Undo", "Edit", "Ctrl+Z", false },
    { "edit.redo", "Redo", "Edit", "Ctrl+Y", false },
    { "edit.redoAlt", "Redo (second key)", "Edit", "Ctrl+Shift+Z", false },
    { "edit.cut", "Cut", "Edit", "Ctrl+X", false },
    { "edit.copy", "Copy", "Edit", "Ctrl+C", false },
    { "edit.paste", "Paste at the view centre", "Edit", "Ctrl+V", false },
    { "edit.duplicate", "Duplicate", "Edit", "Ctrl+D", false },
    { "edit.delete", "Delete", "Edit", "Delete", false },
    { "selection.clear", "Clear the selection", "Selection", "Escape", false },
    { "selection.snapToGround", "Snap to the ground", "Selection", "End", false },
    { "selection.frame", "Frame the selection", "Selection", "F", false },
    { "gizmo.select", "Select tool", "Tools", "1", false },
    { "gizmo.move", "Move tool", "Tools", "2", false },
    { "gizmo.rotate", "Rotate tool", "Tools", "3", false },
    { "gizmo.space", "World / local axes", "Tools", "X", false },
    { "terrain.tool", "Terrain brush", "Tools", "T", false },
    { "brush.tool", "Object paint brush", "Tools", "B", false },
    { "undergrowth.tool", "Undergrowth paint", "Tools", "U", false },
    { "camera.resetSpeed", "Reset the camera speed", "Camera", "Ctrl+R", false },
    { "camera.free", "Free camera", "Camera", "Ctrl+F5", false },
    { "camera.topDown", "Top-down camera", "Camera", "Ctrl+F6", false },
    { "camera.orbit", "Orbit camera", "Camera", "Ctrl+F7", false },
    { "camera.forward", "Move forward", "Camera", "W", true },
    { "camera.back", "Move back", "Camera", "S", true },
    { "camera.left", "Move left", "Camera", "A", true },
    { "camera.right", "Move right", "Camera", "D", true },
    { "camera.up", "Rise", "Camera", "E", true },
    { "camera.down", "Sink", "Camera", "Q", true },
    { "view.fullView", "Full view (hide the panels)", "View", "F11", false },
};

Shortcuts::KeyValidator g_keyValidator = 0;

} // namespace

Shortcuts& editorShortcuts()
{
    static Shortcuts table;
    return table;
}

void Shortcuts::setKeyValidator(KeyValidator validator)
{
    g_keyValidator = validator;
}

bool KeyChord::operator==(const KeyChord& other) const
{
    return lowered(key) == lowered(other.key) && ctrl == other.ctrl && shift == other.shift && alt == other.alt;
}

bool parseChord(const std::string& text, KeyChord& out)
{
    KeyChord chord;
    const std::string whole = trimmed(text);
    if (whole.empty() || lowered(whole) == "none")
    {
        out = chord;
        return true;
    }
    size_t start = 0;
    while (start <= whole.size())
    {
        size_t plus = whole.find('+', start);
        // A last part of "+" itself ("Ctrl++") is the plus key, which ImGui names "Equal"/"KeypadAdd";
        // it is not accepted.
        if (plus == std::string::npos)
            plus = whole.size();
        const std::string part = trimmed(whole.substr(start, plus - start));
        if (part.empty())
            return false;
        const std::string low = lowered(part);
        const bool last = plus == whole.size();
        if (!last)
        {
            if (low == "ctrl" || low == "control")
                chord.ctrl = true;
            else if (low == "shift")
                chord.shift = true;
            else if (low == "alt")
                chord.alt = true;
            else
                return false;
        }
        else
        {
            if (low == "ctrl" || low == "control" || low == "shift" || low == "alt")
                return false;                       // a modifier alone is no key
            chord.key = part;
            if (chord.key.size() == 1)
                chord.key[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(chord.key[0])));
        }
        start = plus + 1;
        if (last)
            break;
    }
    if (chord.key.empty())
        return false;
    out = chord;
    return true;
}

std::string formatChord(const KeyChord& chord)
{
    if (chord.key.empty())
        return std::string();
    std::string text;
    if (chord.ctrl)
        text += "Ctrl+";
    if (chord.shift)
        text += "Shift+";
    if (chord.alt)
        text += "Alt+";
    return text + chord.key;
}

const std::vector<ShortcutAction>& Shortcuts::actions()
{
    static const std::vector<ShortcutAction> all(kActions, kActions + sizeof(kActions) / sizeof(kActions[0]));
    return all;
}

const ShortcutAction* Shortcuts::action(const std::string& id)
{
    const std::vector<ShortcutAction>& all = actions();
    for (size_t i = 0; i < all.size(); ++i)
        if (id == all[i].id)
            return &all[i];
    return 0;
}

Shortcuts::Shortcuts()
{
    resetAll();
}

const KeyChord& Shortcuts::chord(const std::string& id) const
{
    static const KeyChord none;
    std::map<std::string, KeyChord>::const_iterator found = m_chords.find(id);
    return found == m_chords.end() ? none : found->second;
}

bool Shortcuts::set(const std::string& id, const std::string& chordText, std::string& error)
{
    const ShortcutAction* a = action(id);
    if (!a)
    {
        error = "no action '" + id + "'";
        return false;
    }
    KeyChord chord;
    if (!parseChord(chordText, chord) || (!chord.empty() && g_keyValidator && !g_keyValidator(chord.key)))
    {
        error = "'" + chordText + "' is not a key (Ctrl+Shift+Z, F5, Delete, 1 ...)";
        return false;
    }
    if (a->held && (chord.ctrl || chord.shift || chord.alt))
    {
        error = std::string(a->label) + " is held down: it takes a key without Ctrl, Shift or Alt";
        return false;
    }
    m_chords[id] = chord;
    return true;
}

void Shortcuts::reset(const std::string& id)
{
    const ShortcutAction* a = action(id);
    if (!a)
        return;
    KeyChord chord;
    parseChord(a->defaultChord, chord);
    m_chords[id] = chord;
}

void Shortcuts::resetAll()
{
    m_chords.clear();
    const std::vector<ShortcutAction>& all = actions();
    for (size_t i = 0; i < all.size(); ++i)
        reset(all[i].id);
}

bool Shortcuts::isDefault(const std::string& id) const
{
    const ShortcutAction* a = action(id);
    KeyChord chordDefault;
    return a && parseChord(a->defaultChord, chordDefault) && chord(id) == chordDefault;
}

std::vector<std::string> Shortcuts::conflicts(const std::string& id) const
{
    std::vector<std::string> out;
    const KeyChord& mine = chord(id);
    if (mine.empty())
        return out;
    const ShortcutAction* self = action(id);
    const bool selfHeld = self && self->held;
    const std::vector<ShortcutAction>& all = actions();
    for (size_t i = 0; i < all.size(); ++i)
    {
        if (id == all[i].id)
            continue;
        const KeyChord& other = chord(all[i].id);
        bool clash = other == mine;
        // A camera key and a shortcut on the same key clash unless the shortcut needs Ctrl: the camera
        // leaves its keys alone while Ctrl is down (FreeCamera::update), and moves with any other.
        if (!clash && selfHeld != all[i].held && lowered(other.key) == lowered(mine.key))
            clash = !(selfHeld ? other.ctrl : mine.ctrl);
        if (clash)
            out.push_back(all[i].id);
    }
    return out;
}

void Shortcuts::load(const EditorSettings& settings)
{
    resetAll();
    const std::vector<ShortcutAction>& all = actions();
    for (size_t i = 0; i < all.size(); ++i)
    {
        const std::string key = std::string("shortcuts.") + all[i].id;
        if (!settings.has(key))
            continue;
        std::string ignored;
        if (!set(all[i].id, settings.get(key, std::string()), ignored))
            reset(all[i].id);
    }
}

void Shortcuts::save(EditorSettings& settings) const
{
    const std::vector<ShortcutAction>& all = actions();
    for (size_t i = 0; i < all.size(); ++i)
    {
        const std::string key = std::string("shortcuts.") + all[i].id;
        if (isDefault(all[i].id))
        {
            // A default is not written; one written before is dropped by writing the default back.
            if (settings.has(key))
                settings.set(key, display(all[i].id));
            continue;
        }
        const std::string text = display(all[i].id);
        settings.set(key, text.empty() ? std::string("none") : text);
    }
}

} // namespace bf2editor
