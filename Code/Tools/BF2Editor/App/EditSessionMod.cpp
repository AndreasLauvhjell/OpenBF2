// Code/Tools/BF2Editor/App/EditSessionMod.cpp - the mod manager's description pages. Editor-only code (D3).

// The retail mod manager's settings page edited the mod's mod.desc (title, description, URL, logo, icon, menu
// music, version) and its level page a level's Info/<level>.desc (the name the menus show, the loading
// screen's briefing, the loading music). The files are the contract: the game's menus read them. Here a field
// is set as one undoable edit whose replays are EditorMod.applyDescription / removeDescription, and the level
// save writes the files (App/DescriptionDocuments). The level's modes list stays the gameplay layers' work
// (MapDescription). Retail's content page chose its own editor's content list (Content.ini), which is not a
// game file.
#include "Framework/EditorConfig.h"
#include "App/EditSession.h"

#include "App/LevelDocument.h"
#include "Framework/EditorLog.h"

namespace bf2editor
{

DescriptionDocuments& EditSession::descriptions()
{
    if (!m_descriptions.loaded() && m_document && m_document->isOpen())
        m_descriptions.load(m_document->levelDirectory());
    return m_descriptions;
}

bool EditSession::description(const std::string& action, const std::string& which, const std::string& field,
                              const std::string& value, std::string& result)
{
    if (!m_document || !m_document->isOpen())
    {
        result = "no level is open";
        return false;
    }
    DescriptionDocuments& documents = descriptions();
    if (which != "mod" && which != "level")
    {
        result = "the description is the mod's or the level's";
        return false;
    }
    const DescriptionDocuments::Which file = which == "mod" ? DescriptionDocuments::Mod : DescriptionDocuments::Level;
    if (!documents.exists(file))
    {
        result = documents.path(file) + " is not there";
        return false;
    }
    if (action == "list")
    {
        size_t count = 0;
        const char* const* names = DescriptionDocuments::fields(file, count);
        result.clear();
        for (size_t i = 0; i < count; ++i)
        {
            std::string text;
            const bool present = documents.value(file, names[i], text);
            result += std::string(i ? "; " : "") + names[i] + " " + (present ? (text.empty() ? "(empty)" : text) : "(none)");
        }
        return true;
    }
    if (!DescriptionDocuments::validField(file, field))
    {
        size_t count = 0;
        const char* const* names = DescriptionDocuments::fields(file, count);
        std::string list;
        for (size_t i = 0; i < count; ++i)
            list += std::string(i ? ", " : "") + names[i];
        result = "'" + field + "' is not a field of " + documents.path(file) + " (" + list + ")";
        return false;
    }
    if (action == "apply" || action == "remove")
        return documents.apply(file, field, value, action == "remove", result);
    if (action != "set")
    {
        result = "no description action '" + action + "'";
        return false;
    }
    if (value.find('"') != std::string::npos || value.find('\n') != std::string::npos)
    {
        result = "a description field is one line without double quotes";
        return false;
    }
    std::string old;
    const bool present = documents.value(file, field, old);
    if (present && old == value)
    {
        result = "the " + field + " is that already";
        return true;
    }
    HistoryEntry entry;
    entry.label = "Set the " + which + " description's " + field;
    entry.doScript = "EditorMod.applyDescription " + which + " " + field + " \"" + value + "\"\n";
    entry.undoScript = present ? "EditorMod.applyDescription " + which + " " + field + " \"" + old + "\"\n"
                               : "EditorMod.removeDescription " + which + " " + field + "\n";
    if (!m_history.execute(entry, result))
        return false;
    logLine(LogLevel::Info, "edit: %s", entry.label.c_str());
    result = entry.label + " (" + documents.path(file) + "; File > Save writes it)";
    return true;
}

} // namespace bf2editor
