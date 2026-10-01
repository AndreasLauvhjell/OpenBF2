// Edits Init.con/Sky.con/Water.con settings by command, caller-defined index words and branch.
// Preserves untouched bytes and newline style. Missing settings follow the last matching command in
// the branch, or append unconditionally when none exists. Editor/game forms are tracked separately.
#ifndef BF2EDITOR_FRAMEWORK_LEVELSETTINGSFILE_H
#define BF2EDITOR_FRAMEWORK_LEVELSETTINGSFILE_H

#include "Framework/EditorConfig.h"

#include <cstddef>
#include <map>
#include <string>
#include <vector>

namespace bf2editor
{

class LevelSettingsFile
{
public:
    enum Branch
    {
        Always = 0,
        EditorBranch = 1,
        GameBranch = 2
    };

    LevelSettingsFile();
    void parse(const std::string& text);
    std::string render() const;
    bool hasChanges() const { return render() != m_text; }
    const std::string& text() const { return m_text; }      // as parsed

    // The value of the statement `command <index...> <value>` in `branch` (`index` is its index words,
    // space-separated, "" for none); false when the file has no such statement.
    bool value(const std::string& command, const std::string& index, Branch branch, std::string& out) const;
    // Sets it: the value rewritten in place (the command's spelling, its index words and the line's
    // indentation kept), or the statement added (see above).
    void setValue(const std::string& command, const std::string& index, Branch branch, const std::string& value);
    // A statement setValue added, taken out again; false when the file had it itself.
    bool removeAdded(const std::string& command, const std::string& index, Branch branch);
    // The edits made, back to none.
    void revert();

private:
    struct Line
    {
        std::string text;
        bool        crlf;
        Branch      branch;
        bool        found;      // a statement a setting can be (not in a conditional of another kind)
        std::string command;    // lower-case "object.command"
        std::string words;      // the rest of the statement, as written (trimmed)
    };
    // The line of the statement, npos when none.
    size_t find(const std::string& command, const std::string& index, Branch branch) const;
    static bool startsWithIndex(const std::string& words, const std::string& index, std::string& rest);

    std::vector<Line>                  m_lines;
    std::string                        m_text;
    bool                               m_finalNewline;
    bool                               m_crlf;
    // An edited line: its text up to the old value (command and index words as written), the new value.
    struct Edit
    {
        std::string prefix;
        std::string value;
    };
    std::map<size_t, Edit>             m_edits;
    // Statements added: after line (npos = the end) -> their lines, in the order they were added.
    std::map<size_t, std::vector<std::string> > m_added;
    struct Added
    {
        std::string command;
        std::string index;
        std::string value;
        Branch      branch;
        size_t      after;
        size_t      position;   // in m_added[after]
    };
    std::vector<Added>                 m_addedKeys;
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_LEVELSETTINGSFILE_H
