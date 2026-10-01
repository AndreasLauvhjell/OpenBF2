// Code/Tools/BF2Editor/App/ObjectWizard.cpp - see ObjectWizard.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/ObjectWizard.h"

#include "App/EditSession.h"
#include "Framework/EditorLog.h"
#include "Framework/ObjectLibrary.h"

#include "Game/Objects/IObject.h"
#include "Game/Objects/IObjectTemplate.h"
#include "IO/Console/Console.h"
#include "Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace bf2editor
{

namespace
{

const char kRuleFolder[] = "bf2editor/Content/ObjectEditor/Wizards";

bool sameText(const std::string& a, const std::string& b)
{
    return _stricmp(a.c_str(), b.c_str()) == 0;
}

// One engine console line with full access; its answer, trimmed (EditSession.cpp's engineLine).
std::string engineLine(const std::string& line)
{
    std::string answer;
    dice::hfe::io::mainConsole->executeLine(line + "\n", answer, false, false, false, false, true);
    const size_t begin = answer.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return std::string();
    return answer.substr(begin, answer.find_last_not_of(" \t\r\n") - begin + 1);
}

int groupOf(const std::string& type)
{
    return sameText(type, "ErrorCheck") ? 0 : sameText(type, "WarningCheck") ? 1 : sameText(type, "Utility") ? 2 : -1;
}

std::vector<std::string> splitChoices(const std::string& text)
{
    std::vector<std::string> out;
    size_t start = 0;
    while (start <= text.size())
    {
        size_t end = text.find('/', start);
        if (end == std::string::npos)
            end = text.size();
        if (end > start)
            out.push_back(text.substr(start, end - start));
        start = end + 1;
    }
    return out;
}

} // namespace

ObjectWizard::ObjectWizard(EditSession& session)
    : m_session(session)
    , m_loaded(false)
    , m_root(0)
    , m_rootId(0)
    , m_executing(false)
    , m_pass(0)
    , m_next(0)
    , m_item(-1)
    , m_objectAt(0)
    , m_lineAt(0)
    , m_listAsked(false)
    , m_wait(Wait::None)
    , m_floatIndex(0)
    , m_batchOpen(false)
    , m_edits(0)
{
}

void ObjectWizard::forget()
{
    closeBatch();
    m_items.clear();
    m_results.assign(m_rules.rules().size(), std::vector<Object*>());
    m_target.assign(m_rules.rules().size(), 0);
    m_root = 0;
    m_rootId = 0;
    m_executing = false;
    m_wait = Wait::None;
    m_item = -1;
}

bool ObjectWizard::load(std::string& result)
{
    if (m_executing)
    {
        result = "Execute is waiting for an answer: finish or cancel it first";
        return false;
    }
    forget();
    m_rules.clear();
    m_results.clear();
    m_target.clear();
    m_loaded = false;
    std::vector<std::string> files;
    std::string error;
    if (!WizardRules::ruleFiles(kRuleFolder, files, error))
    {
        result = error;
        return false;
    }
    if (!dice::hfe::io::mainConsole)
    {
        result = "the engine console is not up";
        return false;
    }
    size_t lines = 0;
    for (size_t f = 0; f < files.size(); ++f)
    {
        std::string text;
        if (!ObjectLibrary::readTextFile(std::string(kRuleFolder) + "/" + files[f], text, error))
        {
            logLine(LogLevel::Warning, "wizard: %s", error.c_str());
            continue;
        }
        const std::vector<std::string> fileLines = wizardFileLines(text);
        for (size_t i = 0; i < fileLines.size(); ++i)
            issue(fileLines[i], false);
        lines += fileLines.size();
    }
    m_results.resize(m_rules.rules().size());
    m_target.resize(m_rules.rules().size(), 0);
    m_loaded = true;
    char text[200];
    std::snprintf(text, sizeof(text), "%u rules from %u files (%u lines) in %s - loaded once a session (retail ran them "
                  "again on each ObjectEditor activation, appending every block)%s",
                  static_cast<unsigned>(m_rules.rules().size()), static_cast<unsigned>(files.size()), static_cast<unsigned>(lines),
                  kRuleFolder, m_rules.recording() ? "; a block is left open: it takes every later line" : "");
    result = text;
    logLine(LogLevel::Info, "wizard: %s", result.c_str());
    return true;
}

std::string ObjectWizard::engine(const std::string& line)
{
    return dice::hfe::io::mainConsole ? engineLine(line) : std::string();
}

std::string ObjectWizard::issue(const std::string& line, bool resolution)
{
    std::string name, message;
    std::vector<std::string> arguments;
    switch (m_rules.feed(line, name, arguments, message))
    {
    case WizardRules::Empty:
    case WizardRules::Captured:
        return std::string();
    case WizardRules::Handled:
        if (m_results.size() < m_rules.rules().size())
        {
            m_results.resize(m_rules.rules().size());
            m_target.resize(m_rules.rules().size(), 0);
        }
        return message;
    case WizardRules::Refused:
        logLine(LogLevel::Warning, "wizard: %s", message.c_str());
        return message;
    case WizardRules::Command:
        command(name, arguments, resolution);
        return std::string();
    case WizardRules::Console:
        if (resolution)
        {
            translate(line);
            return std::string();
        }
        return engine(line);
    }
    return std::string();
}

std::string ObjectWizard::templateName(Object* node) const
{
    return node && node->getTemplate() ? node->getTemplate()->getName() : std::string();
}

// Analysis (§4)

bool ObjectWizard::filter(int rule, int op, const std::string& className, Object* node)
{
    if (op == 0)
        return true;
    // getClassIDFromTemplateClassName: case-insensitive, 0 on a miss - and no template has class 0.
    const unsigned int id = dice::hfe::world::objectTemplateManager
        ? dice::hfe::world::objectTemplateManager->getClassIDFromTemplateClassName(className) : 0;
    if (op == 1 || op == 2)
    {
        const bool same = node->getTemplate() && id != 0 && node->getTemplate()->getClassID() == id;
        return op == 1 ? same : !same;
    }
    // haschild: a node collected from its children (the recurse filter applied) has the class.
    std::vector<Object*> below;
    for (Object* child = node->getChild(); child; child = child->getSibling())
        collect(rule, child, below, true);
    for (size_t i = 0; i < below.size(); ++i)
        if (below[i]->getTemplate() && id != 0 && below[i]->getTemplate()->getClassID() == id)
            return true;
    return false;
}

void ObjectWizard::collect(int rule, Object* node, std::vector<Object*>& out, bool test)
{
    const WizardRule& r = m_rules.rules()[static_cast<size_t>(rule)];
    if (test && !filter(rule, r.recurseOp, r.recurseClass, node))
        return;
    out.push_back(node);
    for (Object* child = node->getChild(); child; child = child->getSibling())
        collect(rule, child, out, true);
}

void ObjectWizard::analyze(int rule, Object* root)
{
    WizardRule& r = m_rules.rules()[static_cast<size_t>(rule)];
    m_results[static_cast<size_t>(rule)].clear();
    m_target[static_cast<size_t>(rule)] = root;
    r.floats.clear();
    if (!root)
        return;
    // Verbatim, no substitution; the recurse filter, flag mode, flags, break and #string carry over (Q12).
    const std::vector<std::string> lines = r.analyzer;
    for (size_t i = 0; i < lines.size(); ++i)
        issue(lines[i], false);
}

void ObjectWizard::command(const std::string& name, const std::vector<std::string>& arguments, bool resolution)
{
    const int current = m_rules.current();
    if (current < 0)
        return;
    WizardRule& rule = m_rules.rules()[static_cast<size_t>(current)];
    if (m_results.size() <= static_cast<size_t>(current))
    {
        m_results.resize(m_rules.rules().size());
        m_target.resize(m_rules.rules().size(), 0);
    }
    std::vector<Object*>& results = m_results[static_cast<size_t>(current)];
    Object* target = m_target[static_cast<size_t>(current)];
    if (name == "populate")
    {
        const int op = wizardFilterOperator(arguments[0]);
        if (op == 0)
            logLine(LogLevel::Warning, "wizard: '%s' is not IS, ISNOT or HASCHILD", arguments[0].c_str());
        if (!target)
            return;
        std::vector<Object*> all;
        collect(current, target, all, false);
        for (size_t i = 0; i < all.size(); ++i)
            if (filter(current, op, arguments[1], all[i]))
                results.push_back(all[i]);              // appended: no clear, no dedupe (Q7)
        return;
    }
    if (name == "recursefilter")
    {
        rule.recurseOp = wizardFilterOperator(arguments[0]);
        rule.recurseClass = arguments[1];
        return;
    }
    if (name == "include" || name == "exclude")
    {
        const int op = wizardFilterOperator(arguments[0]);
        const bool include = name == "include";
        if (include)
            logLine(LogLevel::Warning, "wizard: Wizard.include keeps the nodes that pass (its evident intent): retail's "
                                       "erase path used freed memory (census Q9)");
        std::vector<Object*> kept;
        for (size_t i = 0; i < results.size(); ++i)
        {
            if (rule.flagMode && i < rule.flags.size())
                rule.flags[i] = false;
            const bool passes = filter(current, op, arguments[1], results[i]);
            if (passes == include)
            {
                kept.push_back(results[i]);
                if (rule.flagMode && i < rule.flags.size())
                    rule.flags[i] = true;
            }
        }
        results.swap(kept);                             // both erase even in flag mode (Q11)
        rule.recurseOp = 0;
        return;
    }
    if (name == "checkvalue")
    {
        // For each node: activate its template, read, write the compare value, read it back, write the old
        // text back; the two reads compared without case. The engine only: no file sees the probe.
        const std::string property = arguments[0];
        const int op = wizardCompareOperator(arguments[1]);
        const std::string value = arguments[2].empty() ? std::string("\"\"") : arguments[2];
        // Each probe line passes the capture test like every line the wizard issues; nothing here holds a
        // reference into the rule list across them.
        const std::vector<Object*> nodes = results;
        std::vector<bool> keep(nodes.size(), false);
        std::vector<bool> flagged(nodes.size(), false);
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            issue("ObjectTemplate.active " + templateName(nodes[i]), false);
            const std::string old = issue("ObjectTemplate." + property, false);
            issue("ObjectTemplate." + property + " " + value, false);
            const std::string now = issue("ObjectTemplate." + property, false);
            issue("ObjectTemplate." + property + (old.empty() ? std::string() : " " + old), false);
            const bool same = sameText(old, now);
            flagged[i] = op == 1 ? !same : op == 2 ? same : true;
            keep[i] = flagged[i];
        }
        WizardRule& checked = m_rules.rules()[static_cast<size_t>(current)];
        std::vector<Object*> kept;
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            if (flagged[i] && checked.flagMode && i < checked.flags.size())
                checked.flags[i] = true;
            if (keep[i] || checked.flagMode)            // flag mode leaves a node it does not keep alone
                kept.push_back(nodes[i]);
        }
        m_results[static_cast<size_t>(current)].swap(kept);
        return;
    }
    if (name == "onlyflag")
    {
        rule.flags.assign(results.size(), false);
        rule.flagMode = true;
        return;
    }
    if (name == "catchup")
    {
        if (!rule.flagMode)
            return;
        std::vector<Object*> kept;
        for (size_t i = 0; i < results.size(); ++i)
            if (i < rule.flags.size() && rule.flags[i])
                kept.push_back(results[i]);
        results.swap(kept);
        rule.flags.clear();
        rule.flagMode = false;
        return;
    }
    if (name == "break")
    {
        rule.breakSet = true;
        return;
    }
    // The prompts: only while Execute runs a resolution.
    if (!resolution || !m_executing)
    {
        logLine(LogLevel::Warning, "wizard: Wizard.%s asks only while Execute runs a resolution", name.c_str());
        return;
    }
    if (name == "getfloat")
    {
        const int index = std::atoi(arguments[0].c_str());
        if (rule.floats.count(index))
            return;                                     // answered: kept until the next analysis (Q14)
        m_floatIndex = index;
        m_promptTitle = "Enter value";
        m_promptText = arguments[1];
        m_promptPreset.clear();
        m_choices.clear();
        m_wait = Wait::Float;
        return;
    }
    if (rule.affectAll && rule.answer != "0")
        return;                                         // asked once in an affectAll loop (Q14)
    m_promptText = arguments[0];
    m_choices.clear();
    if (name == "getstring")
    {
        m_promptTitle = "Enter string";
        m_promptPreset = templateName(target) + "_";
        m_wait = Wait::Text;
    }
    else if (name == "getenum")
    {
        m_promptTitle = "Choose";
        m_promptPreset.clear();
        m_choices = splitChoices(arguments[1]);
        m_wait = Wait::Choice;
    }
    else if (name == "selectobject")
    {
        m_promptTitle = "Select";
        m_promptPreset.clear();
        for (size_t i = 0; i < results.size(); ++i)
            m_choices.push_back(templateName(results[i]));
        m_wait = Wait::Choice;
    }
}

// The items

bool ObjectWizard::run(const std::string& category, std::string& result)
{
    if (m_executing)
    {
        result = "Execute is waiting for an answer: finish or cancel it first";
        return false;
    }
    if (!m_loaded && !load(result))
        return false;
    const unsigned id = m_session.selection().primary();
    Object* root = EditSession::object(id);
    if (!root || !root->getTemplate())
    {
        result = "select an object: the wizard looks at its tree";
        return false;
    }
    forget();
    m_root = root;
    m_rootId = id;
    m_rootName = templateName(root);
    m_category = category;
    m_report.clear();
    std::vector<WizardRule>& rules = m_rules.rules();
    int counts[3] = { 0, 0, 0 };
    for (size_t r = 0; r < rules.size(); ++r)
    {
        if (!sameText(category, rules[r].category) && !sameText("General", rules[r].category))
            continue;
        m_rules.setCurrent(static_cast<int>(r));
        analyze(static_cast<int>(r), root);
        std::vector<Object*> entries;
        if (rules[r].affectAll)
            entries.push_back(root);
        else
            entries = m_results[r];
        for (size_t e = 0; e < entries.size(); ++e)
        {
            // The label: #object the entry's template, #string "" (the label code sets it), floats -1.
            rules[r].answer.clear();
            const std::string label = wizardSubstitute(rules[r].warning, std::map<int, float>(), "", templateName(entries[e]));
            const int group = groupOf(rules[r].type);
            bool merged = false;
            for (size_t i = 0; i < m_items.size() && !merged; ++i)
                if (m_items[i].group == group && sameText(m_items[i].label, label))
                {
                    m_items[i].rule = static_cast<int>(r);  // bound to the newer entry (Q21)
                    m_items[i].object = entries[e];
                    m_items[i].readOnly = rules[r].resolution.empty() || group < 0;
                    merged = true;
                }
            if (merged)
                continue;
            Item item;
            item.rule = static_cast<int>(r);
            item.object = entries[e];
            item.label = label;
            item.group = group;
            item.ticked = false;
            item.readOnly = rules[r].resolution.empty() || group < 0;
            m_items.push_back(item);
            if (group >= 0)
                ++counts[group];
        }
    }
    char text[200];
    std::snprintf(text, sizeof(text), "%u items for %s (%s): %d errors, %d warnings, %d utilities",
                  static_cast<unsigned>(m_items.size()), m_rootName.c_str(), category.c_str(), counts[0], counts[1], counts[2]);
    result = text;
    logLine(LogLevel::Info, "wizard: %s", result.c_str());
    return true;
}

bool ObjectWizard::tick(size_t index, bool on, std::string& result)
{
    if (index >= m_items.size())
    {
        result = "no such item";
        return false;
    }
    if (m_items[index].readOnly && on)
    {
        result = m_items[index].label + ": read-only (nothing to run, or run already)";
        return false;
    }
    m_items[index].ticked = on;
    result = m_items[index].label + (on ? ": ticked" : ": not ticked");
    return true;
}

bool ObjectWizard::objectsAlive() const
{
    return m_root && m_rootId && EditSession::object(m_rootId) == m_root;
}

bool ObjectWizard::stillThere(std::string& result)
{
    if (objectsAlive())
        return true;
    m_item = -1;                                        // first: closing a batch names the item's object
    closeBatch();
    m_executing = false;
    m_wait = Wait::None;
    forget();
    result = "the object the wizard looked at is gone: Execute stopped (run the wizard again)";
    logLine(LogLevel::Warning, "wizard: %s", result.c_str());
    return false;
}

// Execute (§6, §7)

bool ObjectWizard::execute(std::string& result)
{
    if (m_executing)
    {
        result = "Execute is waiting for an answer";
        return false;
    }
    if (m_items.empty())
    {
        result = "nothing to execute: run the wizard first";
        return false;
    }
    if (!objectsAlive())
    {
        forget();
        result = "the object the wizard looked at is gone: run it again";
        return false;
    }
    m_executing = true;
    m_pass = 0;
    m_next = 0;
    m_item = -1;
    m_report.clear();
    step();
    if (m_wait != Wait::None)
        result = "waiting for an answer: " + m_promptTitle + " - " + m_promptText;
    else
    {
        char text[64];
        std::snprintf(text, sizeof(text), "executed %u item%s", static_cast<unsigned>(m_report.size()),
                      m_report.size() == 1 ? "" : "s");
        result = text;
        for (size_t i = 0; i < m_report.size(); ++i)
            result += (i ? "; " : ": ") + m_report[i];
    }
    return true;
}

bool ObjectWizard::startItem(size_t index)
{
    const Item& item = m_items[index];
    m_rules.setCurrent(item.rule);
    WizardRule& rule = m_rules.rules()[static_cast<size_t>(item.rule)];
    m_target[static_cast<size_t>(item.rule)] = item.object;
    rule.answer = "0";
    rule.breakSet = false;
    m_item = static_cast<int>(index);
    m_objectAt = 0;
    m_lineAt = 0;
    m_listAsked = false;
    m_edits = 0;
    m_pendingClass.clear();
    m_pendingName.clear();
    m_pendingLines.clear();
    m_childParent.clear();
    m_childName.clear();
    // The template lines act on: whatever the engine has active now - the analysis's last CheckValue, or an
    // earlier resolution (census Q16).
    const dice::hfe::world::IObjectTemplate* active =
        dice::hfe::world::objectTemplateManager ? dice::hfe::world::objectTemplateManager->getActiveObjectTemplate() : 0;
    m_active = active ? active->getName() : std::string();
    m_objects.clear();
    if (!item.object && !rule.affectAll)
    {
        finishItem(false);
        return false;
    }
    if (rule.affectAll)
        m_objects = m_results[static_cast<size_t>(item.rule)];
    else
        m_objects.push_back(item.object);
    return true;
}

void ObjectWizard::finishItem(bool ran)
{
    flushPending();
    Item& item = m_items[static_cast<size_t>(m_item)];
    const WizardRule& rule = m_rules.rules()[static_cast<size_t>(item.rule)];
    closeBatch();
    item.ticked = false;
    if (item.group == 0 || item.group == 1)
    {
        item.readOnly = true;
        item.status = ran ? "done" : "not run";
    }
    else if (item.group == 2)
        item.status = "Util Completed Successfully!";    // whatever happened (Q17)
    char text[64];
    std::snprintf(text, sizeof(text), " (%d edit%s)", m_edits, m_edits == 1 ? "" : "s");
    m_report.push_back(rule.title + ": " + item.label + text);
    m_item = -1;
}

void ObjectWizard::step()
{
    while (m_executing && m_wait == Wait::None)
    {
        if (m_item < 0)
        {
            if (m_pass > 3)
            {
                m_executing = false;
                logLine(LogLevel::Info, "wizard: Execute finished");
                return;
            }
            // Errors, Errors again (every item it ran is no longer ticked), Warnings, Utils.
            const int group = m_pass <= 1 ? 0 : m_pass == 2 ? 1 : 2;
            while (m_next < m_items.size() && !(m_items[m_next].group == group && m_items[m_next].ticked))
                ++m_next;
            if (m_next >= m_items.size())
            {
                ++m_pass;
                m_next = 0;
                continue;
            }
            startItem(m_next++);
            continue;
        }
        const int r = m_items[static_cast<size_t>(m_item)].rule;
        WizardRule& rule = m_rules.rules()[static_cast<size_t>(r)];
        if (rule.affectAll && rule.listOn && !m_listAsked)
        {
            // ModifyObjectList: a checklist of the results first.
            m_promptTitle = rule.listTitle;
            m_promptText = rule.title;
            m_choices.clear();
            for (size_t i = 0; i < m_objects.size(); ++i)
                m_choices.push_back(templateName(m_objects[i]));
            m_checklist.assign(m_objects.size(), rule.listDefault);
            m_wait = Wait::Checklist;
            closeBatch();
            return;
        }
        if (m_objectAt >= m_objects.size())
        {
            finishItem(true);
            continue;
        }
        if (m_lineAt == 0)
            m_target[static_cast<size_t>(r)] = m_objects[m_objectAt];
        // Break, or a cancelled prompt, ends the object - and under affectAll every later one (Q15).
        if (rule.breakSet || m_lineAt >= rule.resolution.size())
        {
            flushPending();
            ++m_objectAt;
            m_lineAt = 0;
            continue;
        }
        const std::string line = wizardSubstitute(rule.resolution[m_lineAt++], rule.floats, rule.answer,
                                                  templateName(m_target[static_cast<size_t>(r)]));
        issue(line, true);
        if (m_wait != Wait::None)
            closeBatch();                               // no other edit joins the item's while it waits
    }
}

bool ObjectWizard::answer(const std::string& text, std::string& result)
{
    if (m_wait == Wait::None || m_wait == Wait::Checklist || m_item < 0)
    {
        result = m_wait == Wait::Checklist ? "the checklist wants EditorWizard.check / checklistOk" : "nothing is asked";
        return false;
    }
    if (!stillThere(result))
        return false;
    WizardRule& rule = m_rules.rules()[static_cast<size_t>(m_items[static_cast<size_t>(m_item)].rule)];
    if (m_wait == Wait::Choice)
    {
        size_t pick = m_choices.size();
        for (size_t i = 0; i < m_choices.size() && pick == m_choices.size(); ++i)
            if (sameText(m_choices[i], text))
                pick = i;
        if (pick == m_choices.size())
        {
            result = "'" + text + "' is not one of the choices";
            return false;
        }
        rule.answer = m_choices[pick];
    }
    else if (m_wait == Wait::Float)
        rule.floats[m_floatIndex] = static_cast<float>(std::atof(text.c_str()));
    else
        rule.answer = text;
    m_wait = Wait::None;
    result = "answered " + text;
    step();
    if (m_wait != Wait::None)
        result += "; now waiting for: " + m_promptTitle + " - " + m_promptText;
    else if (!m_executing)
        result += "; Execute finished";
    return true;
}

bool ObjectWizard::cancel(std::string& result)
{
    if (m_wait == Wait::None || m_item < 0)
    {
        result = "nothing is asked";
        return false;
    }
    if (!stillThere(result))
        return false;
    WizardRule& rule = m_rules.rules()[static_cast<size_t>(m_items[static_cast<size_t>(m_item)].rule)];
    const bool checklist = m_wait == Wait::Checklist;
    m_wait = Wait::None;
    if (checklist)
    {
        // Cancel on the checklist runs nothing, and the item is still marked done (Q18).
        m_objects.clear();
        m_listAsked = true;
        finishItem(false);
    }
    else
        rule.breakSet = true;                           // a cancelled prompt ends the object (and later ones)
    result = checklist ? "the checklist was cancelled: nothing run" : "cancelled";
    step();
    return true;
}

bool ObjectWizard::checklistDone(std::string& result)
{
    if (m_wait != Wait::Checklist)
    {
        result = "no checklist is shown";
        return false;
    }
    if (!stillThere(result))
        return false;
    std::vector<Object*> ticked;
    for (size_t i = 0; i < m_objects.size() && i < m_checklist.size(); ++i)
        if (m_checklist[i])
            ticked.push_back(m_objects[i]);
    m_objects.swap(ticked);
    m_listAsked = true;
    m_wait = Wait::None;
    char text[64];
    std::snprintf(text, sizeof(text), "%u object%s ticked", static_cast<unsigned>(m_objects.size()),
                  m_objects.size() == 1 ? "" : "s");
    result = text;
    step();
    return true;
}

// The translator (§10.8)

void ObjectWizard::openBatch()
{
    if (m_batchOpen)
        return;
    m_session.beginTemplateBatch();
    m_batchOpen = true;
}

void ObjectWizard::closeBatch()
{
    if (!m_batchOpen)
        return;
    std::string label = "Wizard";
    if (m_item >= 0)
    {
        const Item& item = m_items[static_cast<size_t>(m_item)];
        label += ": " + m_rules.rules()[static_cast<size_t>(item.rule)].title + " (" + templateName(item.object) + ")";
    }
    m_session.endTemplateBatch(label);
    m_batchOpen = false;
}

void ObjectWizard::flushPending()
{
    if (m_pendingName.empty())
        return;
    logLine(LogLevel::Warning, "wizard: the resolution created %s (%s) but never added it as a child, so it is not kept: "
                               "the object editor makes templates in files", m_pendingName.c_str(), m_pendingClass.c_str());
    m_pendingName.clear();
    m_pendingClass.clear();
    m_pendingLines.clear();
}

void ObjectWizard::translate(const std::string& line)
{
    size_t begin = line.find_first_not_of(" \t");
    if (begin == std::string::npos)
        return;
    size_t end = line.find_first_of(" \t", begin);
    if (end == std::string::npos)
        end = line.size();
    const std::string word = line.substr(begin, end - begin);
    std::string rest = end < line.size() ? line.substr(end) : std::string();
    const size_t restBegin = rest.find_first_not_of(" \t");
    rest = restBegin == std::string::npos ? std::string() : rest.substr(restBegin);
    const size_t restEnd = rest.find_last_not_of(" \t");
    rest = restEnd == std::string::npos ? std::string() : rest.substr(0, restEnd + 1);
    if (sameText(word, "rem"))
        return;
    if (word.size() <= 15 || _strnicmp(word.c_str(), "ObjectTemplate.", 15) != 0)
    {
        const std::string answer = engine(line);
        if (!answer.empty())
            logLine(LogLevel::Info, "wizard: %s -> %s", line.c_str(), answer.c_str());
        return;
    }
    const std::string cmd = word.substr(15);
    const std::vector<std::string> args = wizardArguments(rest);
    std::string r;
    bool ok = true;
    dice::hfe::Vec3 zero;
    zero.x = zero.y = zero.z = 0.0f;
    if (sameText(cmd, "active") || sameText(cmd, "activeSafe"))
    {
        m_active = sameText(cmd, "active") ? (args.empty() ? std::string() : args[0]) : (args.size() >= 2 ? args[1] : std::string());
        engine(line);                                   // the engine's cursor follows, as retail's line moved it
        return;
    }
    if (sameText(cmd, "create") && args.size() >= 2)
    {
        flushPending();
        m_active = args[1];
        if (dice::hfe::world::objectTemplateManager && dice::hfe::world::objectTemplateManager->getTemplate(args[1]))
        {
            logLine(LogLevel::Warning, "wizard: %s exists already: the resolution's create is not made again", args[1].c_str());
            return;
        }
        m_pendingClass = args[0];
        m_pendingName = args[1];
        m_pendingLines.clear();
        return;
    }
    if (!m_pendingName.empty() && sameText(m_active, m_pendingName))
    {
        // A line for the template this resolution creates, before it is added somewhere: kept for it.
        if (!sameText(cmd, "createdInEditor"))
            m_pendingLines.push_back(std::make_pair(cmd, rest));
        return;
    }
    openBatch();
    if (sameText(cmd, "addTemplate") && !args.empty())
    {
        if (!m_pendingName.empty() && sameText(args[0], m_pendingName))
        {
            ok = m_session.newTemplateChild(m_active, m_pendingClass, m_pendingName, zero, r);
            const std::string made = m_pendingName;
            const std::vector<std::pair<std::string, std::string> > lines = m_pendingLines;
            m_pendingName.clear();
            m_pendingClass.clear();
            m_pendingLines.clear();
            if (ok)
                for (size_t i = 0; i < lines.size(); ++i)
                {
                    std::string rr;
                    if (m_session.setObjectTemplateProperty(made, lines[i].first, lines[i].second, rr))
                        ++m_edits;
                    else
                        logLine(LogLevel::Warning, "wizard: %s %s: %s", lines[i].first.c_str(), lines[i].second.c_str(), rr.c_str());
                }
        }
        else
            ok = m_session.addTemplateChild(m_active, args[0], zero, r);
        if (ok)
        {
            m_childParent = m_active;
            m_childName = args[0];
        }
    }
    else if ((sameText(cmd, "setPosition") || sameText(cmd, "setRotation")) && !m_childName.empty()
             && sameText(m_childParent, m_active))
    {
        // The child the last addTemplate added: the last of its name in the engine's list, placed.
        std::vector<EditSession::TemplateChild> children;
        std::string error;
        ok = m_session.templateChildren(m_active, children, error);
        int occurrence = -1;
        dice::hfe::Vec3 position = zero, rotation = zero;
        for (size_t i = 0; ok && i < children.size(); ++i)
            if (sameText(children[i].name, m_childName))
            {
                ++occurrence;
                position = children[i].position;
                rotation = children[i].rotation;
            }
        dice::hfe::Vec3 value = zero;
        std::sscanf(rest.c_str(), "%f/%f/%f", &value.x, &value.y, &value.z);
        if (sameText(cmd, "setPosition"))
            position = value;
        else
            rotation = value;
        char spec[160];
        std::snprintf(spec, sizeof(spec), "%s#%d", m_childName.c_str(), occurrence < 0 ? 0 : occurrence);
        ok = ok && occurrence >= 0 && m_session.moveTemplateChild(m_active, spec, position, rotation, r);
        if (!ok && r.empty())
            r = error.empty() ? m_childName + " is not a child of " + m_active : error;
    }
    else if (sameText(cmd, "createComponent") && !args.empty())
    {
        // A component the template has is made fresh, as createComponent made it.
        std::vector<std::string> components;
        std::string error;
        m_session.templateComponents(m_active, components, error);
        for (size_t i = 0; i < components.size(); ++i)
            if (sameText(components[i], args[0]))
            {
                std::string removed;
                if (m_session.removeTemplateComponent(m_active, components[i], removed))
                    ++m_edits;
                break;
            }
        ok = m_session.addTemplateComponent(m_active, args[0], r);
    }
    else if (sameText(cmd, "deleteComponent") && !args.empty())
        ok = m_session.removeTemplateComponent(m_active, args[0], r);
    else
        ok = m_session.setObjectTemplateProperty(m_active, cmd, rest, r);
    if (ok)
        ++m_edits;
    else
        logLine(LogLevel::Warning, "wizard: %s (on %s): %s", line.c_str(), m_active.c_str(), r.c_str());
}

} // namespace bf2editor
