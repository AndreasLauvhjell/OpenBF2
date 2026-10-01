// Code/Tools/BF2Editor/Framework/ConsoleCompletion.h

// The Console panel's helper: the retail
// ConsoleUI picked a console object, then one of its methods, from lists, completed what was typed and kept the
// typed lines between sessions. Here Tab completes the line's command word - the object name first ("Editor" ->
// the objects that start so, "EditorSel" -> "EditorSelection."), then the method ("EditorSelection.tr" ->
// "EditorSelection.translate ") - and the panel shows the matches as the line is typed. The names come from the
// engine's console registry (dice::hfe::mainConsoleObjects); this class only matches text. Engine-free.
// Editor-only code (D3).
#ifndef BF2EDITOR_FRAMEWORK_CONSOLECOMPLETION_H
#define BF2EDITOR_FRAMEWORK_CONSOLECOMPLETION_H

#include "Framework/EditorConfig.h"

#include <string>
#include <vector>

namespace bf2editor
{

class ConsoleCompletion
{
public:
    struct Result
    {
        std::string              text;          // the line after completion (as typed when nothing matched)
        std::vector<std::string> candidates;    // what matched: "Object." entries, or full "Object.method" names
        bool                     completed;     // the text changed
    };

    // Full names, "Object.method". Names that differ only in case are kept once (the first spelling).
    void setNames(const std::vector<std::string>& names);
    size_t size() const { return m_names.size(); }

    // Tab: completes the first word of `line` when the line has no blank yet (the arguments are not completed).
    // One object match gets its ".", one method match its blank; several get their longest common start.
    // At most `limit` candidates are returned (all of them still count for the common start).
    Result complete(const std::string& line, size_t limit) const;

    // The matches for the hint list, as complete() would find them, without changing the line.
    std::vector<std::string> matches(const std::string& line, size_t limit) const;

private:
    void candidates(const std::string& word, std::vector<std::string>& out, bool& methods) const;

    std::vector<std::string> m_names;       // sorted without case, one per name
    std::vector<std::string> m_objects;     // the object part of each, once, sorted without case
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_CONSOLECOMPLETION_H
