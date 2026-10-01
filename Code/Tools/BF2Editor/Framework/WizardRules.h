// Parses wizard console scripts without trimming lines; CR/LF separators skip empty tokens.
// Recording appends commands until an exact case-insensitive end line; unterminated blocks consume later commands.
// Rule titles are case-sensitive and reused. Commands requiring a current rule fail before create.
// Substitutes the first #FLOAT0..9, then #string and #object, case-sensitively and without quoting.
// Unanswered floats are -1; answered floats use %f. Rule execution state persists here.
#ifndef BF2EDITOR_FRAMEWORK_WIZARDRULES_H
#define BF2EDITOR_FRAMEWORK_WIZARDRULES_H

#include "Framework/EditorConfig.h"

#include <map>
#include <string>
#include <vector>

namespace bf2editor
{

struct WizardRule
{
    std::string              title;
    std::string              warning;
    std::string              category;
    std::string              type;
    std::vector<std::string> analyzer;          // appended by each Wizard.endAnalyzer
    std::vector<std::string> resolution;        // appended by each Wizard.endResolution
    bool                     affectAll;
    bool                     listOn;            // Wizard.modifyObjectList
    bool                     listDefault;
    std::string              listTitle;
    // Run state, kept on the rule as retail keeps it (§3.1).
    int                      recurseOp;         // 0 none, 1 is, 2 isnot, 3 haschild
    std::string              recurseClass;
    std::string              answer;            // #string, "0" until a prompt answers
    bool                     flagMode;
    std::vector<bool>        flags;
    std::map<int, float>     floats;            // GetFloat answers, cleared by each analysis
    bool                     breakSet;
    WizardRule() : affectAll(false), listOn(false), listDefault(false), recurseOp(0), answer("0"), flagMode(false),
                   breakSet(false) {}
};

// The console's argument split: blank-separated words, a "..." group one word without its quotes ("" is an
// empty word).
std::vector<std::string> wizardArguments(const std::string& text);

// A rule file's lines: split on CR or LF, blank tokens skipped, nothing trimmed (§2.3).
std::vector<std::string> wizardFileLines(const std::string& text);

// §6.2.
std::string wizardSubstitute(const std::string& line, const std::map<int, float>& floats, const std::string& answer,
                             const std::string& objectName);

// The filter operator is / isnot / haschild -> 1 / 2 / 3, anything else 0; equal / differ -> 1 / 2, else 0.
// Case-sensitive, as retail compared the token with std::string ==.
int wizardFilterOperator(const std::string& token);
int wizardCompareOperator(const std::string& token);

class WizardRules
{
public:
    WizardRules();

    // One line, through the capture test (§2.4). Captured: stored by the recorder. Handled: a load-time
    // Wizard command ran here. Command: another Wizard.* command (an analysis or resolution command) for the
    // caller to run, with `command` its lower-case name and `arguments` its words. Console: any other line,
    // for the engine console. Refused: a Wizard command the console would refuse (unknown, or before the
    // first create), with `message`. Empty: a blank line.
    enum Taken { Empty, Captured, Handled, Command, Console, Refused };
    Taken feed(const std::string& line, std::string& command, std::vector<std::string>& arguments, std::string& message);

    std::vector<WizardRule>& rules() { return m_rules; }
    const std::vector<WizardRule>& rules() const { return m_rules; }
    // The current rule (`cur`); -1 before the first create.
    int current() const { return m_current; }
    void setCurrent(int index) { m_current = index; }
    WizardRule* currentRule() { return m_current >= 0 ? &m_rules[static_cast<size_t>(m_current)] : 0; }
    bool recording() const { return m_recording; }
    void clear();

    // The rule files under `folder` in retail's walk order (§2.2): each folder's subfolders whose name holds
    // "Common" first, then its entries by name (case ignored), a subfolder walked where it is met, *.con files
    // listed. Paths are folder-relative; false with `error` when the folder is not there.
    static bool ruleFiles(const std::string& folder, std::vector<std::string>& files, std::string& error);

private:
    std::vector<WizardRule> m_rules;
    int                     m_current;
    bool                    m_recording;
    std::string             m_terminator;
    std::string             m_buffer;
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_WIZARDRULES_H
