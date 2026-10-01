// Code/Tools/BF2Editor/Framework/TemplateFile.cpp - see TemplateFile.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/TemplateFile.h"

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstring>

namespace bf2editor
{

namespace
{

const char kPrefix[] = "ObjectTemplate.";
const size_t kPrefixLength = sizeof(kPrefix) - 1;

bool sameText(const std::string& a, const std::string& b)
{
    return _stricmp(a.c_str(), b.c_str()) == 0;
}

bool isBlank(char c)
{
    return c == ' ' || c == '\t';
}

// The line's words, split at blanks.
std::vector<std::string> wordsOf(const std::string& text)
{
    std::vector<std::string> words;
    size_t i = 0;
    while (i < text.size())
    {
        while (i < text.size() && isBlank(text[i]))
            ++i;
        size_t j = i;
        while (j < text.size() && !isBlank(text[j]))
            ++j;
        if (j > i)
            words.push_back(text.substr(i, j - i));
        i = j;
    }
    return words;
}

std::string indentationOf(const std::string& text)
{
    size_t i = 0;
    while (i < text.size() && isBlank(text[i]))
        ++i;
    return text.substr(0, i);
}

// "rem ---BeginComp:<name> ---" gives the component's name; "rem ---EndComp ---" ends it.
bool beginComponent(const std::string& text, std::string& component)
{
    const std::vector<std::string> words = wordsOf(text);
    if (words.size() < 2 || !sameText(words[0], "rem") || _strnicmp(words[1].c_str(), "---BeginComp:", 13) != 0)
        return false;
    component = words[1].substr(13);
    return true;
}

bool endComponent(const std::string& text)
{
    const std::vector<std::string> words = wordsOf(text);
    return words.size() >= 2 && sameText(words[0], "rem") && _strnicmp(words[1].c_str(), "---EndComp", 10) == 0;
}

// The component a property belongs to: the part before its first '.', or "".
std::string componentOf(const std::string& property)
{
    const size_t dot = property.find('.');
    return dot == std::string::npos ? std::string() : property.substr(0, dot);
}

// The property line with its value replaced: the indentation, the command as spelt and the blanks before
// the value kept.
std::string withValue(const std::string& text, const std::string& value)
{
    size_t i = 0;
    while (i < text.size() && isBlank(text[i]))
        ++i;
    while (i < text.size() && !isBlank(text[i]))
        ++i;
    size_t j = i;
    while (j < text.size() && isBlank(text[j]))
        ++j;
    const std::string separator = j > i ? text.substr(i, j - i) : std::string(" ");
    return text.substr(0, i) + (value.empty() ? std::string() : separator + value);
}

} // namespace

TemplateFile::TemplateFile()
    : m_changed(false)
{
}

bool TemplateFile::splitPropertyLine(const std::string& text, std::string& property, std::string& value)
{
    size_t i = 0;
    while (i < text.size() && isBlank(text[i]))
        ++i;
    if (text.size() - i <= kPrefixLength || _strnicmp(text.c_str() + i, kPrefix, kPrefixLength) != 0)
        return false;
    size_t j = i + kPrefixLength;
    while (j < text.size() && !isBlank(text[j]))
        ++j;
    property = text.substr(i + kPrefixLength, j - i - kPrefixLength);
    if (property.empty())
        return false;
    while (j < text.size() && isBlank(text[j]))
        ++j;
    size_t end = text.size();
    while (end > j && isBlank(text[end - 1]))
        --end;
    value = text.substr(j, end - j);
    return true;
}

void TemplateFile::parse(const std::string& text)
{
    m_lines.clear();
    m_changed = false;
    size_t start = 0;
    while (start < text.size())
    {
        const size_t newline = text.find('\n', start);
        Line line;
        if (newline == std::string::npos)
        {
            line.text = text.substr(start);
            start = text.size();
        }
        else
        {
            const bool crlf = newline > start && text[newline - 1] == '\r';
            line.text = text.substr(start, newline - start - (crlf ? 1 : 0));
            line.end = crlf ? "\r\n" : "\n";
            start = newline + 1;
        }
        m_lines.push_back(line);
    }
    m_original = text;
    index();
}

void TemplateFile::index()
{
    m_blocks.clear();
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        const std::vector<std::string> words = wordsOf(m_lines[i].text);
        if (words.empty() || _strnicmp(words[0].c_str(), kPrefix, kPrefixLength) != 0)
            continue;
        const std::string command = words[0].substr(kPrefixLength);
        TemplateBlock block;
        if ((sameText(command, "create") || sameText(command, "activeSafe")) && words.size() >= 3)
        {
            block.className = words[1];
            block.name = words[2];
        }
        else if (sameText(command, "active") && words.size() >= 2)
            block.name = words[1];
        else
            continue;
        block.opening = command;
        block.first = i;
        block.end = m_lines.size();
        if (!m_blocks.empty())
            m_blocks.back().end = i;
        m_blocks.push_back(block);
    }
}

std::string TemplateFile::render() const
{
    std::string text;
    for (size_t i = 0; i < m_lines.size(); ++i)
        text += m_lines[i].text + m_lines[i].end;
    return text;
}

bool TemplateFile::hasChanges() const
{
    return m_changed && render() != m_original;
}

std::vector<size_t> TemplateFile::blocksOf(const std::string& name) const
{
    std::vector<size_t> found;
    for (size_t i = 0; i < m_blocks.size(); ++i)
        if (sameText(m_blocks[i].name, name))
            found.push_back(i);
    return found;
}

int TemplateFile::lastBlockOf(const std::string& name) const
{
    for (size_t i = m_blocks.size(); i-- > 0;)
        if (sameText(m_blocks[i].name, name))
            return static_cast<int>(i);
    return -1;
}

std::vector<TemplatePropertyLine> TemplateFile::properties(const std::string& name) const
{
    std::vector<TemplatePropertyLine> lines;
    const int b = lastBlockOf(name);
    if (b < 0)
        return lines;
    const TemplateBlock& block = m_blocks[static_cast<size_t>(b)];
    for (size_t i = block.first + 1; i < block.end; ++i)
    {
        TemplatePropertyLine line;
        line.line = i;
        if (splitPropertyLine(m_lines[i].text, line.property, line.value))
            lines.push_back(line);
    }
    return lines;
}

bool TemplateFile::value(const std::string& name, const std::string& property, std::string& out) const
{
    const std::vector<TemplatePropertyLine> lines = properties(name);
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (sameText(lines[i].property, property))
        {
            out = lines[i].value;
            return true;
        }
    }
    return false;
}

std::string TemplateFile::newline() const
{
    size_t crlf = 0, lf = 0;
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        if (m_lines[i].end == "\r\n")
            ++crlf;
        else if (m_lines[i].end == "\n")
            ++lf;
    }
    return lf > crlf ? "\n" : "\r\n";
}

size_t TemplateFile::insertionPoint(const TemplateBlock& block, const std::string& property) const
{
    const std::string component = componentOf(property);
    std::string property2, value;
    if (!component.empty())
    {
        // After the block's last line of the same component.
        size_t last = 0;
        bool found = false;
        for (size_t i = block.first + 1; i < block.end; ++i)
        {
            if (splitPropertyLine(m_lines[i].text, property2, value) && sameText(componentOf(property2), component))
            {
                last = i;
                found = true;
            }
        }
        if (found)
            return last + 1;
        // Inside the component's section, before its end.
        for (size_t i = block.first + 1; i < block.end; ++i)
        {
            std::string name;
            if (!beginComponent(m_lines[i].text, name) || !sameText(name, component))
                continue;
            for (size_t j = i + 1; j < block.end; ++j)
                if (endComponent(m_lines[j].text))
                    return j;
            return i + 1;
        }
    }
    // After the block's last property line outside the component sections (right after the opening
    // line when there is none).
    size_t last = block.first;
    bool inComponent = false;
    for (size_t i = block.first + 1; i < block.end; ++i)
    {
        std::string name;
        if (beginComponent(m_lines[i].text, name))
            inComponent = true;
        else if (endComponent(m_lines[i].text))
            inComponent = false;
        else if (!inComponent && splitPropertyLine(m_lines[i].text, property2, value))
            last = i;
    }
    return last + 1;
}

bool TemplateFile::setProperty(const std::string& name, const std::string& property, const std::string& value, int which,
                               std::string& error)
{
    const int b = lastBlockOf(name);
    if (b < 0)
    {
        error = "this file has no lines of the template " + name;
        return false;
    }
    if (property.empty() || property.find_first_of(" \t") != std::string::npos
        || value.find_first_of("\r\n") != std::string::npos)
    {
        error = "'" + property + "' is not a property, or the value is more than one line";
        return false;
    }
    const TemplateBlock block = m_blocks[static_cast<size_t>(b)];
    std::vector<size_t> matches;
    std::string p, v;
    for (size_t i = block.first + 1; i < block.end; ++i)
        if (splitPropertyLine(m_lines[i].text, p, v) && sameText(p, property))
            matches.push_back(i);
    if (which < 0 && matches.size() > 1)
    {
        char text[160];
        std::snprintf(text, sizeof(text), " appears %u times in the block of %s: name which (0 to %u)",
                      static_cast<unsigned>(matches.size()), name.c_str(), static_cast<unsigned>(matches.size() - 1));
        error = property + text;
        return false;
    }
    if (which >= 0 && static_cast<size_t>(which) >= matches.size())
    {
        char text[160];
        std::snprintf(text, sizeof(text), " has %u lines of %s", static_cast<unsigned>(matches.size()), property.c_str());
        error = "the block of " + name + text;
        return false;
    }
    if (!matches.empty())
    {
        // In place: the indentation, the command as spelt and the blanks before the value kept.
        Line& line = m_lines[matches[which < 0 ? 0 : static_cast<size_t>(which)]];
        line.text = withValue(line.text, value);
        m_changed = true;
        return true;
    }
    Line line;
    line.text = indentationOf(m_lines[block.first].text) + kPrefix + property + (value.empty() ? "" : " " + value);
    line.end = newline();
    const size_t at = insertionPoint(block, property);
    if (at == m_lines.size() && !m_lines.empty() && m_lines.back().end.empty())
    {
        m_lines.back().end = line.end;      // the old last line needs one now
        line.end.clear();                   // the file keeps its lack of a final line end
    }
    m_lines.insert(m_lines.begin() + static_cast<std::ptrdiff_t>(at), line);
    m_changed = true;
    index();
    return true;
}

bool TemplateFile::addProperty(const std::string& name, const std::string& property, const std::string& value,
                               std::string& error)
{
    const int b = lastBlockOf(name);
    if (b < 0)
    {
        error = "this file has no lines of the template " + name;
        return false;
    }
    if (property.empty() || property.find_first_of(" \t") != std::string::npos
        || value.find_first_of("\r\n") != std::string::npos)
    {
        error = "'" + property + "' is not a property, or the value is more than one line";
        return false;
    }
    const TemplateBlock block = m_blocks[static_cast<size_t>(b)];
    size_t at = 0;
    bool found = false;
    std::string p, v;
    for (size_t i = block.first + 1; i < block.end; ++i)
        if (splitPropertyLine(m_lines[i].text, p, v) && sameText(p, property))
        {
            at = i + 1;
            found = true;
        }
    if (!found)
        at = insertionPoint(block, property);
    Line line;
    line.text = indentationOf(m_lines[found ? at - 1 : block.first].text) + kPrefix + property + (value.empty() ? "" : " " + value);
    line.end = newline();
    if (at == m_lines.size() && !m_lines.empty() && m_lines.back().end.empty())
    {
        m_lines.back().end = line.end;      // the old last line needs one now
        line.end.clear();                   // the file keeps its lack of a final line end
    }
    m_lines.insert(m_lines.begin() + static_cast<std::ptrdiff_t>(at), line);
    m_changed = true;
    index();
    return true;
}

namespace
{

const char kGeometryPrefix[] = "GeometryTemplate.";
const size_t kGeometryPrefixLength = sizeof(kGeometryPrefix) - 1;

// "GeometryTemplate.<property> <value>" -> property, value.
bool splitGeometryLine(const std::string& text, std::string& property, std::string& value)
{
    size_t i = 0;
    while (i < text.size() && isBlank(text[i]))
        ++i;
    if (text.size() - i <= kGeometryPrefixLength || _strnicmp(text.c_str() + i, kGeometryPrefix, kGeometryPrefixLength) != 0)
        return false;
    size_t j = i + kGeometryPrefixLength;
    while (j < text.size() && !isBlank(text[j]))
        ++j;
    property = text.substr(i + kGeometryPrefixLength, j - i - kGeometryPrefixLength);
    while (j < text.size() && isBlank(text[j]))
        ++j;
    value = text.substr(j);
    while (!value.empty() && isBlank(value[value.size() - 1]))
        value.erase(value.size() - 1);
    return !property.empty();
}

} // namespace

std::vector<TemplatePropertyLine> TemplateFile::geometryLines() const
{
    std::vector<TemplatePropertyLine> lines;
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        TemplatePropertyLine line;
        if (splitGeometryLine(m_lines[i].text, line.property, line.value))
            lines.push_back(line);
    }
    return lines;
}

bool TemplateFile::setGeometryProperty(const std::string& property, const std::string& value, int which, std::string& error)
{
    if (property.empty() || property.find_first_of(" \t") != std::string::npos || value.find_first_of("\r\n") != std::string::npos)
    {
        error = "'" + property + "' is not a geometry property, or the value is more than one line";
        return false;
    }
    std::vector<size_t> matches;
    size_t last = 0;
    bool any = false;
    std::string p, v;
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        if (!splitGeometryLine(m_lines[i].text, p, v))
            continue;
        last = i;
        any = true;
        if (sameText(p, property))
            matches.push_back(i);
    }
    if (which < 0 && matches.size() > 1)
    {
        char text[160];
        std::snprintf(text, sizeof(text), " appears %u times: name which (0 to %u)", static_cast<unsigned>(matches.size()),
                      static_cast<unsigned>(matches.size() - 1));
        error = "GeometryTemplate." + property + text;
        return false;
    }
    if (which >= 0 && static_cast<size_t>(which) >= matches.size())
    {
        char text[96];
        std::snprintf(text, sizeof(text), "the file has %u lines of GeometryTemplate.%s", static_cast<unsigned>(matches.size()),
                      property.c_str());
        error = text;
        return false;
    }
    if (!matches.empty())
    {
        Line& line = m_lines[matches[which < 0 ? 0 : static_cast<size_t>(which)]];
        line.text = withValue(line.text, value);
        m_changed = true;
        return true;
    }
    // A new line: after the last geometry line, or after the leading rem lines (the writer's header).
    size_t at = 0;
    if (any)
        at = last + 1;
    else
        while (at < m_lines.size() && sameText(wordsOf(m_lines[at].text).empty() ? std::string() : wordsOf(m_lines[at].text)[0], "rem"))
            ++at;
    Line line;
    line.text = std::string(kGeometryPrefix) + property + (value.empty() ? "" : " " + value);
    line.end = newline();
    if (at == m_lines.size() && !m_lines.empty() && m_lines.back().end.empty())
    {
        m_lines.back().end = line.end;
        line.end.clear();
    }
    m_lines.insert(m_lines.begin() + static_cast<std::ptrdiff_t>(at), line);
    m_changed = true;
    index();
    return true;
}

bool TemplateFile::removeGeometryProperty(const std::string& property, int which, std::string& error)
{
    std::vector<size_t> matches;
    std::string p, v;
    for (size_t i = 0; i < m_lines.size(); ++i)
        if (splitGeometryLine(m_lines[i].text, p, v) && sameText(p, property))
            matches.push_back(i);
    if (matches.empty() || (which < 0 && matches.size() > 1) || (which >= 0 && static_cast<size_t>(which) >= matches.size()))
    {
        error = matches.empty() ? "the file has no GeometryTemplate." + property + " line"
                                : "GeometryTemplate." + property + " appears more than once: name which";
        return false;
    }
    m_lines.erase(m_lines.begin() + static_cast<std::ptrdiff_t>(matches[which < 0 ? 0 : static_cast<size_t>(which)]));
    m_changed = true;
    index();
    return true;
}

bool TemplateFile::removeProperty(const std::string& name, const std::string& property, int which, std::string& error)
{
    const int b = lastBlockOf(name);
    if (b < 0)
    {
        error = "this file has no lines of the template " + name;
        return false;
    }
    const TemplateBlock block = m_blocks[static_cast<size_t>(b)];
    std::vector<size_t> matches;
    std::string p, v;
    for (size_t i = block.first + 1; i < block.end; ++i)
        if (splitPropertyLine(m_lines[i].text, p, v) && sameText(p, property))
            matches.push_back(i);
    if (matches.empty() || (which < 0 && matches.size() > 1) || (which >= 0 && static_cast<size_t>(which) >= matches.size()))
    {
        error = matches.empty() ? "the block of " + name + " has no line of " + property
                                : property + " appears more than once: name which";
        return false;
    }
    const size_t at = matches[which < 0 ? 0 : static_cast<size_t>(which)];
    // The last line keeps the file's final line end (or its lack of one).
    if (at == m_lines.size() - 1 && at > 0)
        m_lines[at - 1].end = m_lines[at].end;
    m_lines.erase(m_lines.begin() + static_cast<std::ptrdiff_t>(at));
    m_changed = true;
    index();
    return true;
}

namespace
{

bool childLine(const std::string& property)
{
    return sameText(property, "addTemplate") || sameText(property, "setPosition") || sameText(property, "setRotation");
}

} // namespace

bool TemplateFile::addChildLines(const std::string& name, const std::string& child, const std::string& position,
                                 std::string& error)
{
    const int b = lastBlockOf(name);
    if (b < 0)
    {
        error = "this file has no lines of the template " + name;
        return false;
    }
    const TemplateBlock block = m_blocks[static_cast<size_t>(b)];
    size_t at = 0;
    bool found = false;
    std::string property, value;
    for (size_t i = block.first + 1; i < block.end; ++i)
    {
        if (splitPropertyLine(m_lines[i].text, property, value) && childLine(property))
        {
            at = i + 1;
            found = true;
        }
    }
    if (!found)
        at = insertionPoint(block, "addTemplate");
    const std::string indentation = indentationOf(m_lines[block.first].text);
    std::vector<Line> lines(1);
    lines[0].text = indentation + kPrefix + "addTemplate " + child;
    if (!position.empty())
    {
        lines.push_back(Line());
        lines[1].text = indentation + kPrefix + "setPosition " + position;
    }
    const std::string end = newline();
    for (size_t i = 0; i < lines.size(); ++i)
        lines[i].end = end;
    if (at == m_lines.size() && !m_lines.empty() && m_lines.back().end.empty())
    {
        m_lines.back().end = end;
        lines.back().end.clear();
    }
    m_lines.insert(m_lines.begin() + static_cast<std::ptrdiff_t>(at), lines.begin(), lines.end());
    m_changed = true;
    index();
    return true;
}

bool TemplateFile::removeChildLines(const std::string& name, const std::string& child, std::string& error)
{
    if (lastBlockOf(name) < 0)
    {
        error = "this file has no lines of the template " + name;
        return false;
    }
    const std::vector<TemplateChildLines> list = children(name);
    for (size_t i = 0; i < list.size(); ++i)
        if (sameText(list[i].child, child))
            return removeChildAt(name, i, error);
    error = "the block of " + name + " adds no child " + child;
    return false;
}

std::vector<TemplateChildLines> TemplateFile::children(const std::string& name) const
{
    std::vector<TemplateChildLines> list;
    const int b = lastBlockOf(name);
    if (b < 0)
        return list;
    const TemplateBlock& block = m_blocks[static_cast<size_t>(b)];
    std::string property, value;
    for (size_t i = block.first + 1; i < block.end; ++i)
    {
        if (!splitPropertyLine(m_lines[i].text, property, value) || !sameText(property, "addTemplate"))
            continue;
        TemplateChildLines c;
        c.line = i;
        const std::vector<std::string> w = wordsOf(value);
        c.child = w.empty() ? std::string() : w[0];
        c.positionLine = -1;
        c.rotationLine = -1;
        size_t j = i + 1;
        for (; j < block.end && splitPropertyLine(m_lines[j].text, property, value); ++j)
        {
            if (sameText(property, "setPosition"))
            {
                c.positionLine = static_cast<int>(j);
                c.position = value;
            }
            else if (sameText(property, "setRotation"))
            {
                c.rotationLine = static_cast<int>(j);
                c.rotation = value;
            }
            else
                break;
        }
        c.end = j;
        list.push_back(c);
        i = j - 1;
    }
    return list;
}

bool TemplateFile::removeChildAt(const std::string& name, size_t which, std::string& error)
{
    const std::vector<TemplateChildLines> list = children(name);
    if (which >= list.size())
    {
        char text[96];
        std::snprintf(text, sizeof(text), " adds %u children", static_cast<unsigned>(list.size()));
        error = "the block of " + name + text;
        return false;
    }
    const size_t first = list[which].line;
    const size_t end = list[which].end;
    if (end == m_lines.size() && first > 0)
        m_lines[first - 1].end = m_lines[end - 1].end;     // the file keeps its last line end (or lack of one)
    m_lines.erase(m_lines.begin() + static_cast<std::ptrdiff_t>(first), m_lines.begin() + static_cast<std::ptrdiff_t>(end));
    m_changed = true;
    index();
    return true;
}

bool TemplateFile::setChildTransformAt(const std::string& name, size_t which, const std::string& position,
                                       const std::string& rotation, std::string& error)
{
    const std::vector<TemplateChildLines> list = children(name);
    if (which >= list.size())
    {
        char text[96];
        std::snprintf(text, sizeof(text), " adds %u children", static_cast<unsigned>(list.size()));
        error = "the block of " + name + text;
        return false;
    }
    if (position.find_first_of(" \t\r\n") != std::string::npos || rotation.find_first_of(" \t\r\n") != std::string::npos)
    {
        error = "a position or a rotation is one x/y/z word";
        return false;
    }
    const TemplateChildLines c = list[which];
    if (c.positionLine >= 0 && !position.empty())
        m_lines[static_cast<size_t>(c.positionLine)].text = withValue(m_lines[static_cast<size_t>(c.positionLine)].text, position);
    if (c.rotationLine >= 0 && !rotation.empty())
        m_lines[static_cast<size_t>(c.rotationLine)].text = withValue(m_lines[static_cast<size_t>(c.rotationLine)].text, rotation);
    // The rotation goes after the position, so it is added first: adding the position then moves it down.
    const std::string indentation = indentationOf(m_lines[c.line].text);
    if (c.rotationLine < 0 && !rotation.empty())
        insertLine((c.positionLine >= 0 ? static_cast<size_t>(c.positionLine) : c.line) + 1,
                   indentation + kPrefix + "setRotation " + rotation);
    if (c.positionLine < 0 && !position.empty())
        insertLine(c.line + 1, indentation + kPrefix + "setPosition " + position);
    m_changed = true;
    index();
    return true;
}

namespace
{

// The lines ObjectTemplate::makeScript writes before a template's components.
bool headerLine(const std::string& property)
{
    const char* const header[] = { "creator", "modifiedByUser", "setNetworkableInfo", "saveInSeparateFile",
                                   "createNotInGrid", "createInvisible", "createVisibleInEditor", "isNotSaveable",
                                   "createdInEditor", "isDestructable", "castsDynamicShadow", "hasRelativeOffset",
                                   "cullRadiusScale" };
    for (size_t i = 0; i < sizeof(header) / sizeof(header[0]); ++i)
        if (sameText(property, header[i]))
            return true;
    return false;
}

} // namespace

std::vector<TemplateComponentSection> TemplateFile::componentSections(const std::string& name) const
{
    std::vector<TemplateComponentSection> list;
    const int b = lastBlockOf(name);
    if (b < 0)
        return list;
    const TemplateBlock& block = m_blocks[static_cast<size_t>(b)];
    for (size_t i = block.first + 1; i < block.end; ++i)
    {
        std::string component;
        if (beginComponent(m_lines[i].text, component))
        {
            TemplateComponentSection section;
            section.component = component;
            section.begin = i;
            section.end = 0;
            list.push_back(section);
        }
        else if (endComponent(m_lines[i].text) && !list.empty() && list.back().end == 0)
            list.back().end = i + 1;
    }
    return list;
}

bool TemplateFile::addComponentSection(const std::string& name, const std::string& component, std::string& error)
{
    const int b = lastBlockOf(name);
    if (b < 0)
    {
        error = "this file has no lines of the template " + name;
        return false;
    }
    if (component.empty() || component.find_first_of(" \t\r\n") != std::string::npos)
    {
        error = "'" + component + "' is not a component name";
        return false;
    }
    const TemplateBlock block = m_blocks[static_cast<size_t>(b)];
    const std::vector<TemplateComponentSection> sections = componentSections(name);
    size_t at = block.first + 1;
    if (!sections.empty())
    {
        if (sections.back().end == 0)
        {
            error = "the " + sections.back().component + " section of " + name + " has no end line: refused";
            return false;
        }
        at = sections.back().end;
    }
    else
    {
        std::string property, value;
        for (size_t i = block.first + 1; i < block.end; ++i)
        {
            if (!splitPropertyLine(m_lines[i].text, property, value))
                continue;
            if (!headerLine(property))
                break;
            at = i + 1;
        }
    }
    insertComponentLines(at, indentationOf(m_lines[block.first].text), component);
    index();
    return true;
}

bool TemplateFile::removeComponentSection(const std::string& name, const std::string& component,
                                          std::vector<std::string>& lines, std::string& error)
{
    size_t at = 0;
    if (!takeComponentSection(name, component, lines, at, error))
        return false;
    index();
    return true;
}

bool TemplateFile::replaceComponentSection(const std::string& name, const std::string& oldComponent,
                                           const std::string& newComponent, std::vector<std::string>& lines,
                                           std::string& error)
{
    if (newComponent.empty() || newComponent.find_first_of(" \t\r\n") != std::string::npos)
    {
        error = "'" + newComponent + "' is not a component name";
        return false;
    }
    const int b = lastBlockOf(name);
    const std::string indentation = b < 0 ? std::string() : indentationOf(m_lines[m_blocks[static_cast<size_t>(b)].first].text);
    size_t at = 0;
    if (!takeComponentSection(name, oldComponent, lines, at, error))
        return false;
    insertComponentLines(at, indentation, newComponent);
    index();
    return true;
}

bool TemplateFile::takeComponentSection(const std::string& name, const std::string& component,
                                        std::vector<std::string>& lines, size_t& at, std::string& error)
{
    lines.clear();
    const std::vector<TemplateComponentSection> sections = componentSections(name);
    for (size_t s = 0; s < sections.size(); ++s)
    {
        if (!sameText(sections[s].component, component))
            continue;
        if (sections[s].end == 0)
        {
            error = "the " + component + " section of " + name + " has no end line: refused";
            return false;
        }
        const size_t first = sections[s].begin;
        const size_t end = sections[s].end;
        std::string property, value;
        for (size_t i = first + 1; i + 1 < end; ++i)
            if (splitPropertyLine(m_lines[i].text, property, value) && !sameText(property, "createComponent"))
                lines.push_back(std::string(kPrefix) + property + (value.empty() ? "" : " " + value));
        if (end == m_lines.size() && first > 0)
            m_lines[first - 1].end = m_lines[end - 1].end;     // the file keeps its last line end (or lack of one)
        m_lines.erase(m_lines.begin() + static_cast<std::ptrdiff_t>(first), m_lines.begin() + static_cast<std::ptrdiff_t>(end));
        at = first;
        m_changed = true;
        return true;
    }
    error = "the block of " + name + " has no " + component + " section";
    return false;
}

void TemplateFile::insertComponentLines(size_t at, const std::string& indentation, const std::string& component)
{
    // Inserted last line first, each before `at`, so a file without a final line end keeps its lack of one.
    insertLine(at, indentation + "rem ---EndComp ---");
    insertLine(at, indentation + kPrefix + "createComponent " + component);
    insertLine(at, indentation + "rem ---BeginComp:" + component + " ---");
    m_changed = true;
}

void TemplateFile::insertLine(size_t at, const std::string& text)
{
    Line line;
    line.text = text;
    line.end = newline();
    if (at == m_lines.size() && !m_lines.empty() && m_lines.back().end.empty())
    {
        m_lines.back().end = line.end;      // the old last line needs one now
        line.end.clear();                   // the file keeps its lack of a final line end
    }
    m_lines.insert(m_lines.begin() + static_cast<std::ptrdiff_t>(at), line);
}

void TemplateFile::appendCreateBlock(const std::string& className, const std::string& name,
                                     const std::vector<std::string>& properties)
{
    const std::string end = newline();
    if (!m_lines.empty() && m_lines.back().end.empty())
        m_lines.back().end = end;
    if (!m_lines.empty() && !m_lines.back().text.empty())
    {
        Line blank;
        blank.end = end;
        m_lines.push_back(blank);
    }
    Line line;
    line.text = std::string(kPrefix) + "create " + className + " " + name;
    line.end = end;
    m_lines.push_back(line);
    for (size_t i = 0; i < properties.size(); ++i)
    {
        Line property;
        property.text = std::string(kPrefix) + properties[i];
        property.end = end;
        m_lines.push_back(property);
    }
    m_changed = true;
    index();
}

bool TemplateFile::setBlockClass(const std::string& name, const std::string& className)
{
    bool changed = false;
    for (size_t b = 0; b < m_blocks.size(); ++b)
    {
        const TemplateBlock& block = m_blocks[b];
        if (!sameText(block.name, name) || block.className.empty())
            continue;
        // The opening's third word is the class: ObjectTemplate.<opening> <class> <name>.
        const std::string& text = m_lines[block.first].text;
        size_t at = 0, word = 0, begin = 0, end = 0;
        while (at < text.size() && word < 2)
        {
            while (at < text.size() && isBlank(text[at]))
                ++at;
            begin = at;
            while (at < text.size() && !isBlank(text[at]))
                ++at;
            end = at;
            if (end > begin)
                ++word;
        }
        if (word != 2 || text.substr(begin, end - begin) == className)
            continue;
        m_lines[block.first].text = text.substr(0, begin) + className + text.substr(end);
        changed = true;
    }
    if (changed)
    {
        m_changed = true;
        index();
    }
    return changed;
}

bool TemplateFile::renameTemplateLines(const std::string& from, const std::string& to)
{
    bool changed = false;
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        std::string property, value;
        if (!splitPropertyLine(m_lines[i].text, property, value))
            continue;
        // The line's words: the command first, so the value's k-th word is the line's (k+1)-th.
        size_t which = 0;
        if (sameText(property, "create") || sameText(property, "activeSafe"))
            which = 3;
        else if (sameText(property, "active") || sameText(property, "addTemplate"))
            which = 2;
        else
            continue;
        const std::string& text = m_lines[i].text;
        size_t at = 0, word = 0, begin = 0, end = 0;
        while (at < text.size() && word < which)
        {
            while (at < text.size() && isBlank(text[at]))
                ++at;
            begin = at;
            while (at < text.size() && !isBlank(text[at]))
                ++at;
            end = at;
            if (end > begin)
                ++word;
        }
        if (word != which || !sameText(text.substr(begin, end - begin), from))
            continue;
        m_lines[i].text = text.substr(0, begin) + to + text.substr(end);
        changed = true;
    }
    if (changed)
    {
        m_changed = true;
        index();
    }
    return changed;
}

void TemplateFile::appendBlock(const std::string& className, const std::string& name)
{
    const std::string end = newline();
    if (!m_lines.empty() && m_lines.back().end.empty())
        m_lines.back().end = end;
    if (!m_lines.empty() && !m_lines.back().text.empty())
    {
        Line blank;
        blank.end = end;
        m_lines.push_back(blank);
    }
    Line line;
    line.text = std::string(kPrefix) + "activeSafe " + className + " " + name;
    line.end = end;
    m_lines.push_back(line);
    m_changed = true;
    index();
}

} // namespace bf2editor
