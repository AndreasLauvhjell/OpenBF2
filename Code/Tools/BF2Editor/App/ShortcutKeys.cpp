// Code/Tools/BF2Editor/App/ShortcutKeys.cpp - see ShortcutKeys.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/ShortcutKeys.h"

#include "App/ImGuiHost.h"

#include "imgui.h"

#include <cctype>
#include <map>

namespace bf2editor
{

namespace
{

bool isModifierKey(ImGuiKey key)
{
    return key == ImGuiKey_LeftCtrl || key == ImGuiKey_RightCtrl || key == ImGuiKey_LeftShift
        || key == ImGuiKey_RightShift || key == ImGuiKey_LeftAlt || key == ImGuiKey_RightAlt
        || key == ImGuiKey_LeftSuper || key == ImGuiKey_RightSuper || key == ImGuiKey_ReservedForModCtrl
        || key == ImGuiKey_ReservedForModShift || key == ImGuiKey_ReservedForModAlt
        || key == ImGuiKey_ReservedForModSuper;
}

// Mouse buttons and gamepad inputs are named keys too; a shortcut is a keyboard key.
bool isKeyboardKey(ImGuiKey key)
{
    return key >= ImGuiKey_NamedKey_BEGIN && key < ImGuiKey_GamepadStart && !isModifierKey(key);
}

bool g_suspended = false;

bool knownKey(const std::string& name);

} // namespace

void installShortcutKeys()
{
    Shortcuts::setKeyValidator(&knownKey);
}

void suspendShortcuts(bool suspended)
{
    g_suspended = suspended;
}

namespace
{

bool knownKey(const std::string& name)
{
    return imguiKeyOf(name) != ImGuiKey_None;
}

} // namespace

int imguiKeyOf(const std::string& name)
{
    static std::map<std::string, int> byName;
    if (byName.empty())
    {
        for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key)
        {
            if (!isKeyboardKey(static_cast<ImGuiKey>(key)))
                continue;
            std::string lower = ImGui::GetKeyName(static_cast<ImGuiKey>(key));
            for (size_t i = 0; i < lower.size(); ++i)
                lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower[i])));
            byName[lower] = key;
        }
    }
    std::string lower = name;
    for (size_t i = 0; i < lower.size(); ++i)
        lower[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(lower[i])));
    std::map<std::string, int>::const_iterator found = byName.find(lower);
    return found == byName.end() ? ImGuiKey_None : found->second;
}

bool shortcutPressed(const char* id, bool anywhere)
{
    const KeyChord& chord = shortcuts().chord(id);
    if (chord.empty() || g_suspended)
        return false;
    const ImGuiKey key = static_cast<ImGuiKey>(imguiKeyOf(chord.key));
    if (key == ImGuiKey_None || !ImGui::IsKeyPressed(key, false))
        return false;
    const ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl != chord.ctrl || io.KeyShift != chord.shift || io.KeyAlt != chord.alt)
        return false;
    if (chord.ctrl || chord.alt || anywhere)
        return !io.WantTextInput;
    return keyboardForView();
}

bool shortcutDown(const char* id)
{
    const KeyChord& chord = shortcuts().chord(id);
    if (chord.empty())
        return false;
    const ImGuiKey key = static_cast<ImGuiKey>(imguiKeyOf(chord.key));
    return key != ImGuiKey_None && ImGui::IsKeyDown(key);
}

bool chordPressed(KeyChord& out)
{
    const ImGuiIO& io = ImGui::GetIO();
    for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key)
    {
        if (!isKeyboardKey(static_cast<ImGuiKey>(key)) || !ImGui::IsKeyPressed(static_cast<ImGuiKey>(key), false))
            continue;
        out.key = ImGui::GetKeyName(static_cast<ImGuiKey>(key));
        out.ctrl = io.KeyCtrl;
        out.shift = io.KeyShift;
        out.alt = io.KeyAlt;
        return true;
    }
    return false;
}

} // namespace bf2editor
