// Code/Tools/BF2Editor/Framework/WizardRules.cpp - see WizardRules.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/WizardRules.h"

#include <windows.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace bf2editor
{

namespace
{

bool isBlank(char c)
{
    return c == ' ' || c == '\t';
}

std::string lower(std::string text)
{
    for (size_t i = 0; i < text.size(); ++i)
        if (text[i] >= 'A' && text[i] <= 'Z')
            text[i] = static_cast<char>(text[i] - 'A' + 'a');
    return text;
}

// The first occurrence of `token` replaced by `value` (retail's find + substr + value + substr).
std::string replaceFirst(const std::string& text, const std::string& token, const std::string& value)
{
    const size_t at = text.find(token);
    if (at == std::string::npos)
        return text;
    return text.substr(0, at) + value + text.substr(at + token.size());
}

bool truth(const std::string& word)
{
    const std::string w = lower(word);
    return !(w.empty() || w == "0" || w == "false");
}

// Argument counts the console checks for each command (minimum, maximum).
bool arity(const std::string& name, size_t& low, size_t& high)
{
    struct Row { const char* name; size_t low, high; };
    static const Row rows[] = {
        { "create", 1, 1 }, { "beginanalyzer", 0, 0 }, { "endanalyzer", 0, 0 }, { "beginresolution", 0, 0 },
        { "endresolution", 0, 0 }, { "warning", 0, 1 }, { "category", 0, 1 }, { "type", 0, 1 },
        { "affectall", 0, 1 }, { "modifyobjectlist", 2, 2 }, { "populate", 2, 2 }, { "recursefilter", 2, 2 },
        { "include", 2, 2 }, { "exclude", 2, 2 }, { "checkvalue", 3, 3 }, { "onlyflag", 0, 0 }, { "catchup", 0, 0 },
        { "break", 0, 0 }, { "getstring", 1, 1 }, { "getenum", 2, 2 }, { "getfloat", 2, 2 }, { "selectobject", 1, 1 },
    };
    for (size_t i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i)
        if (name == rows[i].name)
        {
            low = rows[i].low;
            high = rows[i].high;
            return true;
        }
    return false;
}

void splitLines(const std::string& text, std::vector<std::string>& out)
{
    size_t start = 0;
    while (start <= text.size())
    {
        size_t end = text.find('\n', start);
        if (end == std::string::npos)
            end = text.size();
        if (end > start)
            out.push_back(text.substr(start, end - start));
        start = end + 1;
    }
}

struct Entry
{
    std::string name;
    bool        folder;
};

bool byName(const Entry& a, const Entry& b)
{
    return _stricmp(a.name.c_str(), b.name.c_str()) < 0;
}

void walk(const std::string& root, const std::string& relative, std::vector<std::string>& files)
{
    WIN32_FIND_DATAA found;
    const std::string pattern = root + (relative.empty() ? std::string() : "/" + relative) + "/*";
    HANDLE h = FindFirstFileA(pattern.c_str(), &found);
    if (h == INVALID_HANDLE_VALUE)
        return;
    std::vector<Entry> entries;
    do
    {
        const std::string name = found.cFileName;
        if (name == "." || name == "..")
            continue;
        Entry entry;
        entry.name = name;
        entry.folder = (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        entries.push_back(entry);
    } while (FindNextFileA(h, &found));
    FindClose(h);
    std::sort(entries.begin(), entries.end(), byName);
    // Pass 1: the subfolders whose path holds Common; pass 2: the rest in order.
    for (int pass = 0; pass < 2; ++pass)
    {
        for (size_t i = 0; i < entries.size(); ++i)
        {
            const std::string path = relative.empty() ? entries[i].name : relative + "/" + entries[i].name;
            const bool common = lower(entries[i].name).find("common") != std::string::npos;
            if (entries[i].folder)
            {
                if ((pass == 0) == common)
                    walk(root, path, files);
            }
            else if (pass == 1 && entries[i].name.size() > 4
                     && _stricmp(entries[i].name.c_str() + entries[i].name.size() - 4, ".con") == 0)
                files.push_back(path);
        }
    }
}

} // namespace

std::vector<std::string> wizardArguments(const std::string& text)
{
    std::vector<std::string> words;
    size_t i = 0;
    while (i < text.size())
    {
        while (i < text.size() && isBlank(text[i]))
            ++i;
        if (i >= text.size())
            break;
        if (text[i] == '"')
        {
            const size_t close = text.find('"', i + 1);
            const size_t end = close == std::string::npos ? text.size() : close;
            words.push_back(text.substr(i + 1, end - i - 1));
            i = end == text.size() ? end : end + 1;
            continue;
        }
        size_t j = i;
        while (j < text.size() && !isBlank(text[j]))
            ++j;
        words.push_back(text.substr(i, j - i));
        i = j;
    }
    return words;
}

std::vector<std::string> wizardFileLines(const std::string& text)
{
    std::vector<std::string> lines;
    size_t start = 0;
    while (start < text.size())
    {
        const size_t end = text.find_first_of("\r\n", start);
        const size_t stop = end == std::string::npos ? text.size() : end;
        if (stop > start)
            lines.push_back(text.substr(start, stop - start));
        start = stop + 1;
    }
    return lines;
}

std::string wizardSubstitute(const std::string& line, const std::map<int, float>& floats, const std::string& answer,
                             const std::string& objectName)
{
    std::string out = line;
    for (int i = 0; i < 10; ++i)
    {
        char token[16];
        std::snprintf(token, sizeof(token), "#FLOAT%d", i);
        std::map<int, float>::const_iterator found = floats.find(i);
        char value[64];
        if (found == floats.end())
            std::snprintf(value, sizeof(value), "-1");
        else
            std::snprintf(value, sizeof(value), "%f", found->second);
        out = replaceFirst(out, token, value);
    }
    out = replaceFirst(out, "#STRING", answer);
    return replaceFirst(out, "#OBJECT", objectName);
}

int wizardFilterOperator(const std::string& token)
{
    return token == "IS" ? 1 : token == "ISNOT" ? 2 : token == "HASCHILD" ? 3 : 0;
}

int wizardCompareOperator(const std::string& token)
{
    return token == "EQUAL" ? 1 : token == "DIFFER" ? 2 : 0;
}

WizardRules::WizardRules()
    : m_current(-1)
    , m_recording(false)
{
}

void WizardRules::clear()
{
    m_rules.clear();
    m_current = -1;
    m_recording = false;
    m_terminator.clear();
    m_buffer.clear();
}

WizardRules::Taken WizardRules::feed(const std::string& line, std::string& command, std::vector<std::string>& arguments,
                                     std::string& message)
{
    command.clear();
    arguments.clear();
    message.clear();
    if (line.empty())
        return Empty;
    // The capture test: while recording, every line but the terminator is stored, not run.
    if (m_recording && _stricmp(line.c_str(), m_terminator.c_str()) != 0)
    {
        m_buffer += line + "\n";
        return Captured;
    }
    m_recording = false;
    m_terminator.clear();
    size_t begin = 0;
    while (begin < line.size() && isBlank(line[begin]))
        ++begin;
    size_t end = begin;
    while (end < line.size() && !isBlank(line[end]))
        ++end;
    const std::string word = line.substr(begin, end - begin);
    if (word.size() < 7 || _strnicmp(word.c_str(), "Wizard.", 7) != 0)
        return Console;
    const std::string name = lower(word.substr(7));
    arguments = wizardArguments(line.substr(end));
    size_t low = 0, high = 0;
    if (!arity(name, low, high))
    {
        message = word + ": Unknown object or method!";
        return Refused;
    }
    if (name != "create" && m_current < 0)
    {
        message = word + ": no rule is current yet (Wizard.create comes first)";
        return Refused;
    }
    if (arguments.size() < low || arguments.size() > high)
    {
        char text[96];
        std::snprintf(text, sizeof(text), ": takes %u to %u arguments", static_cast<unsigned>(low), static_cast<unsigned>(high));
        message = word + text;
        return Refused;
    }
    WizardRule* rule = currentRule();
    if (name == "create")
    {
        for (size_t i = 0; i < m_rules.size(); ++i)
            if (m_rules[i].title == arguments[0])
            {
                m_current = static_cast<int>(i);
                return Handled;
            }
        m_rules.push_back(WizardRule());
        m_rules.back().title = arguments[0];
        m_current = static_cast<int>(m_rules.size() - 1);
        return Handled;
    }
    if (name == "beginanalyzer" || name == "beginresolution")
    {
        m_recording = true;
        m_terminator = name == "beginanalyzer" ? "Wizard.endAnalyzer" : "Wizard.endResolution";
        return Handled;
    }
    if (name == "endanalyzer" || name == "endresolution")
    {
        std::vector<std::string> lines;
        splitLines(m_buffer, lines);
        m_buffer.clear();
        std::vector<std::string>& target = name == "endanalyzer" ? rule->analyzer : rule->resolution;
        target.insert(target.end(), lines.begin(), lines.end());
        return Handled;
    }
    if (name == "warning" || name == "category" || name == "type")
    {
        std::string& field = name == "warning" ? rule->warning : name == "category" ? rule->category : rule->type;
        if (arguments.empty())
            message = field;
        else
            field = arguments[0];
        return Handled;
    }
    if (name == "affectall")
    {
        if (arguments.empty())
            message = rule->affectAll ? "1" : "0";
        else
            rule->affectAll = truth(arguments[0]);
        return Handled;
    }
    if (name == "modifyobjectlist")
    {
        rule->listOn = true;
        rule->listDefault = truth(arguments[1]);
        rule->listTitle = arguments[0];
        return Handled;
    }
    command = name;
    return Command;
}

bool WizardRules::ruleFiles(const std::string& folder, std::vector<std::string>& files, std::string& error)
{
    files.clear();
    const DWORD attributes = GetFileAttributesA(folder.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        error = folder + " is not there (the retail editor's rule files; install the retail editor beside the game)";
        return false;
    }
    walk(folder, std::string(), files);
    return true;
}

} // namespace bf2editor
