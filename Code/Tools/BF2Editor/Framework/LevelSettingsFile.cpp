// Code/Tools/BF2Editor/Framework/LevelSettingsFile.cpp - see LevelSettingsFile.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/LevelSettingsFile.h"

#include <cctype>

namespace bf2editor
{

namespace
{

const size_t npos = static_cast<size_t>(-1);

std::string trimmed(const std::string& text)
{
    size_t begin = 0;
    size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])))
        ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
        --end;
    return text.substr(begin, end - begin);
}

std::string lowered(const std::string& text)
{
    std::string out(text);
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

std::string compacted(const std::string& text)
{
    std::string out;
    for (size_t i = 0; i < text.size(); ++i)
        if (!std::isspace(static_cast<unsigned char>(text[i])))
            out += static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
    return out;
}

std::string indentOf(const std::string& line)
{
    size_t n = 0;
    while (n < line.size() && (line[n] == ' ' || line[n] == '\t'))
        ++n;
    return line.substr(0, n);
}

// The first word of `text` and the rest after it (trimmed).
std::string firstWord(const std::string& text, std::string& rest)
{
    const std::string t = trimmed(text);
    const size_t space = t.find_first_of(" \t");
    rest = space == std::string::npos ? std::string() : trimmed(t.substr(space));
    return space == std::string::npos ? t : t.substr(0, space);
}

} // namespace

LevelSettingsFile::LevelSettingsFile()
    : m_finalNewline(true)
    , m_crlf(true)
{
}

void LevelSettingsFile::revert()
{
    m_edits.clear();
    m_added.clear();
    m_addedKeys.clear();
}

void LevelSettingsFile::parse(const std::string& text)
{
    m_text = text;
    m_lines.clear();
    revert();
    size_t start = 0;
    int crlfLines = 0;
    while (start < text.size())
    {
        const size_t newline = text.find('\n', start);
        Line line;
        line.branch = Always;
        line.found = false;
        if (newline == std::string::npos)
        {
            line.text = text.substr(start);
            line.crlf = false;
            m_lines.push_back(line);
            break;
        }
        size_t end = newline;
        line.crlf = end > start && text[end - 1] == '\r';
        if (line.crlf)
        {
            --end;
            ++crlfLines;
        }
        line.text = text.substr(start, end - start);
        m_lines.push_back(line);
        start = newline + 1;
    }
    m_finalNewline = text.empty() || text[text.size() - 1] == '\n';
    m_crlf = crlfLines > 0 || m_lines.empty();

    // Branches: `if v_arg1 == BF2Editor` opens the editor branch, `else` the game branch. A statement in
    // any other conditional is found in none (a stack keeps the nesting).
    std::vector<int> stack;         // 1: an editor if in its editor branch, 2: in its game branch, 3: other
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        Line& line = m_lines[i];
        const std::string t = trimmed(line.text);
        const std::string lower = lowered(t);
        const std::string compact = compacted(t);
        if (lower.compare(0, 2, "if") == 0 && (lower.size() == 2 || std::isspace(static_cast<unsigned char>(lower[2]))))
        {
            stack.push_back(compact == "ifv_arg1==bf2editor" ? 1 : 3);
            continue;
        }
        if (lower == "else")
        {
            if (!stack.empty() && stack.back() == 1)
                stack.back() = 2;
            continue;
        }
        if (lower == "endif")
        {
            if (!stack.empty())
                stack.pop_back();
            continue;
        }
        bool other = false;
        int branch = 0;
        for (size_t k = 0; k < stack.size(); ++k)
        {
            if (stack[k] == 3)
                other = true;
            else
                branch = stack[k];
        }
        line.branch = static_cast<Branch>(branch);
        if (other || t.empty() || lower.compare(0, 3, "rem") == 0)
            continue;
        std::string rest;
        const std::string head = firstWord(t, rest);
        if (head.find('.') == std::string::npos)
            continue;                                       // run, include and the like
        line.command = lowered(head);
        line.words = rest;
        line.found = true;
    }
}

bool LevelSettingsFile::startsWithIndex(const std::string& words, const std::string& index, std::string& rest)
{
    std::string remaining = trimmed(words);
    std::string indexRest = trimmed(index);
    while (!indexRest.empty())
    {
        std::string afterIndex, afterWords;
        const std::string want = firstWord(indexRest, afterIndex);
        const std::string have = firstWord(remaining, afterWords);
        if (lowered(want) != lowered(have))
            return false;
        indexRest = afterIndex;
        remaining = afterWords;
    }
    rest = remaining;
    return true;
}

size_t LevelSettingsFile::find(const std::string& command, const std::string& index, Branch branch) const
{
    const std::string wanted = lowered(command);
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        std::string rest;
        if (m_lines[i].found && m_lines[i].command == wanted && m_lines[i].branch == branch
            && startsWithIndex(m_lines[i].words, index, rest))
            return i;
    }
    return npos;
}

bool LevelSettingsFile::value(const std::string& command, const std::string& index, Branch branch, std::string& out) const
{
    const size_t line = find(command, index, branch);
    if (line != npos)
    {
        std::map<size_t, Edit>::const_iterator edit = m_edits.find(line);
        if (edit != m_edits.end())
        {
            out = edit->second.value;
            return true;
        }
        return startsWithIndex(m_lines[line].words, index, out);
    }
    for (size_t i = 0; i < m_addedKeys.size(); ++i)
    {
        const Added& added = m_addedKeys[i];
        if (lowered(added.command) == lowered(command) && lowered(trimmed(added.index)) == lowered(trimmed(index))
            && added.branch == branch)
        {
            out = added.value;
            return true;
        }
    }
    return false;
}

void LevelSettingsFile::setValue(const std::string& command, const std::string& index, Branch branch,
                                 const std::string& value)
{
    const size_t line = find(command, index, branch);
    if (line != npos)
    {
        std::string original;
        startsWithIndex(m_lines[line].words, index, original);
        if (original == value)
        {
            m_edits.erase(line);
            return;
        }
        // The line up to its old value - the command and index words as written - stays.
        const std::string t = trimmed(m_lines[line].text);
        Edit edit;
        edit.prefix = trimmed(t.substr(0, t.size() - original.size()));
        edit.value = value;
        m_edits[line] = edit;
        return;
    }
    const std::string statement = command + (trimmed(index).empty() ? "" : " " + trimmed(index)) + " " + value;
    for (size_t i = 0; i < m_addedKeys.size(); ++i)
    {
        Added& added = m_addedKeys[i];
        if (lowered(added.command) == lowered(command) && lowered(trimmed(added.index)) == lowered(trimmed(index))
            && added.branch == branch)
        {
            m_added[added.after][added.position] = statement;
            added.value = value;
            return;
        }
    }
    // After the last statement of the same command in the branch; else after the branch's last line;
    // else, for an unconditional one, at the end.
    size_t after = npos;
    const std::string wanted = lowered(command);
    for (size_t i = 0; i < m_lines.size(); ++i)
        if (m_lines[i].found && m_lines[i].command == wanted && m_lines[i].branch == branch)
            after = i;
    for (size_t i = 0; i < m_lines.size() && branch != Always; ++i)
        if (after == npos && m_lines[i].branch == branch && m_lines[i].found)
            after = i;
    Added added;
    added.command = command;
    added.index = index;
    added.value = value;
    added.branch = branch;
    added.after = after;
    added.position = m_added[after].size();
    m_added[after].push_back(statement);
    m_addedKeys.push_back(added);
}

bool LevelSettingsFile::removeAdded(const std::string& command, const std::string& index, Branch branch)
{
    for (size_t i = 0; i < m_addedKeys.size(); ++i)
    {
        const Added added = m_addedKeys[i];
        if (lowered(added.command) != lowered(command) || lowered(trimmed(added.index)) != lowered(trimmed(index))
            || added.branch != branch)
            continue;
        std::vector<std::string>& lines = m_added[added.after];
        lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(added.position));
        if (lines.empty())
            m_added.erase(added.after);
        m_addedKeys.erase(m_addedKeys.begin() + static_cast<std::ptrdiff_t>(i));
        for (size_t k = 0; k < m_addedKeys.size(); ++k)
            if (m_addedKeys[k].after == added.after && m_addedKeys[k].position > added.position)
                --m_addedKeys[k].position;
        return true;
    }
    return false;
}

std::string LevelSettingsFile::render() const
{
    std::string out;
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        const Line& line = m_lines[i];
        const std::string newline = line.crlf ? "\r\n" : "\n";
        std::map<size_t, Edit>::const_iterator edit = m_edits.find(i);
        if (edit != m_edits.end())
            out += indentOf(line.text) + edit->second.prefix + (edit->second.value.empty() ? "" : " ") + edit->second.value;
        else
            out += line.text;
        if (i + 1 < m_lines.size() || m_finalNewline)
            out += newline;
        std::map<size_t, std::vector<std::string> >::const_iterator added = m_added.find(i);
        if (added != m_added.end())
            for (size_t k = 0; k < added->second.size(); ++k)
                out += indentOf(line.text) + added->second[k] + newline;
    }
    std::map<size_t, std::vector<std::string> >::const_iterator tail = m_added.find(npos);
    if (tail != m_added.end() && !tail->second.empty())
    {
        const std::string newline = m_crlf ? "\r\n" : "\n";
        if (!out.empty() && out[out.size() - 1] != '\n')
            out += newline;
        for (size_t k = 0; k < tail->second.size(); ++k)
            out += tail->second[k] + newline;
    }
    return out;
}

} // namespace bf2editor
