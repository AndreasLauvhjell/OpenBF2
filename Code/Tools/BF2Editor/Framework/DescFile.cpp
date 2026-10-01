// Code/Tools/BF2Editor/Framework/DescFile.cpp - see DescFile.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/DescFile.h"

#include <cstring>

namespace bf2editor
{

namespace
{

enum Found { Missing, Element, EmptyTag };

// Where <element ...> opens (the '<'), and where its text begins and ends.
Found findElement(const std::string& xml, const std::string& element, size_t& open, size_t& textBegin, size_t& textEnd)
{
    size_t at = 0;
    while ((at = xml.find("<" + element, at)) != std::string::npos)
    {
        const size_t after = at + 1 + element.size();
        if (after < xml.size() && (xml[after] == '>' || xml[after] == ' ' || xml[after] == '\t' || xml[after] == '/'
                                   || xml[after] == '\r' || xml[after] == '\n'))
        {
            const size_t close = xml.find('>', after);
            if (close == std::string::npos)
                return Missing;
            if (xml[close - 1] == '/')
                return EmptyTag;                    // <element/>: no text to read or set in place
            const size_t end = xml.find("</" + element, close + 1);
            if (end == std::string::npos)
                return Missing;
            open = at;
            textBegin = close + 1;
            textEnd = end;
            return Element;
        }
        at = after;
    }
    return Missing;
}

std::string escaped(const std::string& text)
{
    std::string out;
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '&')
            out += "&amp;";
        else if (text[i] == '<')
            out += "&lt;";
        else if (text[i] == '>')
            out += "&gt;";
        else
            out += text[i];
    }
    return out;
}

std::string unescaped(const std::string& text)
{
    std::string out;
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (text[i] == '&')
        {
            static const char* const names[] = { "&amp;", "&lt;", "&gt;", "&quot;", "&apos;" };
            static const char values[] = { '&', '<', '>', '"', '\'' };
            bool known = false;
            for (int k = 0; k < 5 && !known; ++k)
                if (text.compare(i, std::strlen(names[k]), names[k]) == 0)
                {
                    out += values[k];
                    i += std::strlen(names[k]) - 1;
                    known = true;
                }
            if (known)
                continue;
        }
        out += text[i];
    }
    return out;
}

bool isBlank(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

} // namespace

bool descText(const std::string& xml, const std::string& element, std::string& text)
{
    size_t open = 0, begin = 0, end = 0;
    if (findElement(xml, element, open, begin, end) != Element)
        return false;
    while (begin < end && isBlank(xml[begin]))
        ++begin;
    while (end > begin && isBlank(xml[end - 1]))
        --end;
    text = unescaped(xml.substr(begin, end - begin));
    return true;
}

bool setDescText(std::string& xml, const std::string& root, const std::string& element, const std::string& text)
{
    size_t open = 0, begin = 0, end = 0;
    const Found found = findElement(xml, element, open, begin, end);
    if (found == EmptyTag)
        return false;
    if (found == Element)
    {
        // The blanks the old text had inside the tags stay (the stock files write "<title> X </title>").
        size_t first = begin, last = end;
        while (first < last && isBlank(xml[first]))
            ++first;
        while (last > first && isBlank(xml[last - 1]))
            --last;
        std::string before, after;
        if (first == last)
        {
            // No text yet: a blank on each side when the tags held blanks, none when they touched.
            before = after = end > begin ? " " : "";
        }
        else
        {
            before = xml.substr(begin, first - begin);
            after = xml.substr(last, end - last);
        }
        xml = xml.substr(0, begin) + before + escaped(text) + after + xml.substr(end);
        return true;
    }
    // A new element, before </root>, indented as the root's first child is.
    const size_t close = xml.rfind("</" + root);
    const size_t rootOpen = xml.find("<" + root);
    if (close == std::string::npos || rootOpen == std::string::npos || rootOpen > close)
        return false;
    const size_t rootEnd = xml.find('>', rootOpen);
    std::string indent = "\t";
    std::string lineEnd = xml.find("\r\n") != std::string::npos ? "\r\n" : "\n";
    if (rootEnd != std::string::npos)
    {
        size_t child = rootEnd + 1;
        while (child < close && (xml[child] == '\r' || xml[child] == '\n'))
            ++child;
        size_t text0 = child;
        while (text0 < close && (xml[text0] == ' ' || xml[text0] == '\t'))
            ++text0;
        if (text0 > child && text0 < close && xml[text0] == '<')
            indent = xml.substr(child, text0 - child);
    }
    // The closing tag on its own line: the new element goes on a line of its own before it.
    size_t lineStart = close;
    while (lineStart > 0 && (xml[lineStart - 1] == ' ' || xml[lineStart - 1] == '\t'))
        --lineStart;
    const std::string line = indent + "<" + element + "> " + escaped(text) + " </" + element + ">" + lineEnd;
    if (lineStart > 0 && xml[lineStart - 1] == '\n')
        xml.insert(lineStart, line);
    else
        xml.insert(close, lineEnd + line);
    return true;
}

bool removeDescElement(std::string& xml, const std::string& element)
{
    size_t open = 0, begin = 0, end = 0;
    if (findElement(xml, element, open, begin, end) != Element)
        return false;
    size_t stop = xml.find('>', end);
    if (stop == std::string::npos)
        return false;
    ++stop;
    // Alone on its line: the line goes, its indent and its line end with it.
    size_t lineStart = open;
    while (lineStart > 0 && (xml[lineStart - 1] == ' ' || xml[lineStart - 1] == '\t'))
        --lineStart;
    size_t lineEnd = stop;
    while (lineEnd < xml.size() && (xml[lineEnd] == ' ' || xml[lineEnd] == '\t'))
        ++lineEnd;
    if ((lineStart == 0 || xml[lineStart - 1] == '\n') && (lineEnd == xml.size() || xml[lineEnd] == '\r' || xml[lineEnd] == '\n'))
    {
        if (lineEnd < xml.size() && xml[lineEnd] == '\r')
            ++lineEnd;
        if (lineEnd < xml.size() && xml[lineEnd] == '\n')
            ++lineEnd;
        xml.erase(lineStart, lineEnd - lineStart);
        return true;
    }
    xml.erase(open, stop - open);
    return true;
}

} // namespace bf2editor
