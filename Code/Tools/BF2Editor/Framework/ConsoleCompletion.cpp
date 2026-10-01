// Code/Tools/BF2Editor/Framework/ConsoleCompletion.cpp - see ConsoleCompletion.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/ConsoleCompletion.h"

#include <algorithm>
#include <set>

namespace bf2editor
{

namespace
{

std::string lowerText(std::string text)
{
    for (size_t i = 0; i < text.size(); ++i)
        if (text[i] >= 'A' && text[i] <= 'Z')
            text[i] = static_cast<char>(text[i] - 'A' + 'a');
    return text;
}

bool lessNoCase(const std::string& a, const std::string& b)
{
    const std::string la = lowerText(a), lb = lowerText(b);
    return la < lb || (la == lb && a < b);
}

bool startsNoCase(const std::string& text, const std::string& prefix)
{
    if (prefix.size() > text.size())
        return false;
    for (size_t i = 0; i < prefix.size(); ++i)
    {
        char a = text[i], b = prefix[i];
        if (a >= 'A' && a <= 'Z')
            a = static_cast<char>(a - 'A' + 'a');
        if (b >= 'A' && b <= 'Z')
            b = static_cast<char>(b - 'A' + 'a');
        if (a != b)
            return false;
    }
    return true;
}

// The longest start every candidate shares, compared without case, spelled as the first one has it.
std::string commonStart(const std::vector<std::string>& names)
{
    if (names.empty())
        return std::string();
    size_t length = names[0].size();
    for (size_t i = 1; i < names.size(); ++i)
    {
        size_t k = 0;
        while (k < length && k < names[i].size() && startsNoCase(names[i].substr(k, 1), names[0].substr(k, 1)))
            ++k;
        length = k;
    }
    return names[0].substr(0, length);
}

} // namespace

void ConsoleCompletion::setNames(const std::vector<std::string>& names)
{
    m_names.clear();
    m_objects.clear();
    std::set<std::string> seen, objects;
    for (size_t i = 0; i < names.size(); ++i)
    {
        const size_t dot = names[i].find('.');
        if (dot == std::string::npos || dot == 0 || dot + 1 >= names[i].size())
            continue;
        if (!seen.insert(lowerText(names[i])).second)
            continue;
        m_names.push_back(names[i]);
        const std::string object = names[i].substr(0, dot);
        if (objects.insert(lowerText(object)).second)
            m_objects.push_back(object);
    }
    std::sort(m_names.begin(), m_names.end(), lessNoCase);
    std::sort(m_objects.begin(), m_objects.end(), lessNoCase);
}

void ConsoleCompletion::candidates(const std::string& word, std::vector<std::string>& out, bool& methods) const
{
    out.clear();
    const size_t dot = word.find('.');
    methods = dot != std::string::npos;
    if (!methods)
    {
        for (size_t i = 0; i < m_objects.size(); ++i)
            if (startsNoCase(m_objects[i], word))
                out.push_back(m_objects[i] + ".");
        return;
    }
    const std::string object = word.substr(0, dot);
    for (size_t i = 0; i < m_names.size(); ++i)
    {
        const size_t nameDot = m_names[i].find('.');
        if (nameDot == object.size() && startsNoCase(m_names[i], object) && startsNoCase(m_names[i], word))
            out.push_back(m_names[i]);
    }
}

ConsoleCompletion::Result ConsoleCompletion::complete(const std::string& line, size_t limit) const
{
    Result result;
    result.text = line;
    result.completed = false;
    // Only the command word: once a blank follows it, the line is in its arguments.
    const size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos || line.find_first_of(" \t", start) != std::string::npos)
        return result;
    const std::string word = line.substr(start);
    std::vector<std::string> found;
    bool methods = false;
    candidates(word, found, methods);
    if (found.empty())
        return result;
    std::string replacement;
    if (found.size() == 1)
        replacement = methods ? found[0] + " " : found[0];
    else
    {
        replacement = commonStart(found);
        if (replacement.size() < word.size())
            replacement = word;
    }
    result.text = line.substr(0, start) + replacement;
    result.completed = result.text != line;
    for (size_t i = 0; i < found.size() && i < limit; ++i)
        result.candidates.push_back(found[i]);
    return result;
}

std::vector<std::string> ConsoleCompletion::matches(const std::string& line, size_t limit) const
{
    std::vector<std::string> out;
    const size_t start = line.find_first_not_of(" \t");
    if (start == std::string::npos)
        return out;
    const size_t end = line.find_first_of(" \t", start);
    const std::string word = line.substr(start, end == std::string::npos ? std::string::npos : end - start);
    std::vector<std::string> found;
    bool methods = false;
    candidates(word, found, methods);
    // In the arguments, only the exact command (its arguments' hint).
    if (end != std::string::npos)
    {
        for (size_t i = 0; i < found.size(); ++i)
            if (found[i].size() == word.size())
                out.push_back(found[i]);
        return out;
    }
    for (size_t i = 0; i < found.size() && i < limit; ++i)
        out.push_back(found[i]);
    return out;
}

} // namespace bf2editor
