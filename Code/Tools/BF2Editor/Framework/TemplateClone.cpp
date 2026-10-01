// Code/Tools/BF2Editor/Framework/TemplateClone.cpp - see TemplateClone.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/TemplateClone.h"

#include <cstring>
#include <map>

namespace bf2editor
{

namespace
{

const char kPrefix[] = "objecttemplate.";

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

struct Line
{
    std::string text;
    std::string end;
};

std::vector<Line> linesOf(const std::string& text)
{
    std::vector<Line> lines;
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
        lines.push_back(line);
    }
    return lines;
}

// The words of a line with where each begins.
void wordsOf(const std::string& text, std::vector<std::string>& words, std::vector<size_t>& at)
{
    words.clear();
    at.clear();
    size_t i = 0;
    while (i < text.size())
    {
        while (i < text.size() && isBlank(text[i]))
            ++i;
        size_t j = i;
        while (j < text.size() && !isBlank(text[j]))
            ++j;
        if (j > i)
        {
            words.push_back(text.substr(i, j - i));
            at.push_back(i);
        }
        i = j;
    }
}

// The line with each word from `first` on that the map renames replaced in place, the blanks kept.
std::string renamed(const std::string& text, size_t first, const std::map<std::string, std::string>& names)
{
    std::vector<std::string> words;
    std::vector<size_t> at;
    wordsOf(text, words, at);
    std::string out = text;
    for (size_t k = words.size(); k-- > first;)
    {
        std::map<std::string, std::string>::const_iterator found = names.find(lower(words[k]));
        if (found != names.end())
            out.replace(at[k], words[k].size(), found->second);
    }
    return out;
}

bool startsWith(const std::string& text, const char* head)
{
    const size_t n = std::strlen(head);
    return text.size() >= n && lower(text.substr(0, n)) == head;
}

// A sound template named after one of `owners` (lower-case old name -> new name): S_<owner>_<suffix>, the
// longest owner that fits, becomes S_<new owner>_<suffix>. Empty when the name is no such sound.
std::string soundRename(const std::string& name, const std::map<std::string, std::string>& owners)
{
    const std::string key = lower(name);
    if (key.size() < 3 || key.compare(0, 2, "s_") != 0)
        return std::string();
    std::map<std::string, std::string>::const_iterator best = owners.end();
    for (std::map<std::string, std::string>::const_iterator i = owners.begin(); i != owners.end(); ++i)
    {
        const std::string head = "s_" + i->first + "_";
        if (key.size() > head.size() && key.compare(0, head.size(), head) == 0
            && (best == owners.end() || i->first.size() > best->first.size()))
            best = i;
    }
    if (best == owners.end())
        return std::string();
    return "S_" + best->second + "_" + name.substr(best->first.size() + 3);
}

} // namespace

std::string clonedChildName(const std::string& child, const std::string& source, const std::string& target)
{
    if (child.size() > source.size() && lower(child.substr(0, source.size())) == lower(source))
        return target + child.substr(source.size());
    return target + "_" + child;
}

bool cloneTemplateFiles(const std::string& conText, const std::string& tweakText, const std::string& conStem,
                        const std::string& source, const std::string& target, TemplateClone& out, std::string& error)
{
    out = TemplateClone();
    if (target.empty() || target.find_first_of(" \t\r\n/\\\"") != std::string::npos)
    {
        error = "'" + target + "' is not a template name";
        return false;
    }
    if (lower(source) == lower(target))
    {
        error = "a clone needs a name of its own";
        return false;
    }
    const std::vector<Line> files[2] = { linesOf(conText), linesOf(tweakText) };
    // The templates the two files create: the source, and the children cloned with it.
    std::map<std::string, std::string> names;
    names[lower(source)] = target;
    out.renames.push_back(TemplateCloneRename());
    out.renames.back().from = source;
    out.renames.back().to = target;
    bool defined = false;
    std::vector<std::string> words;
    std::vector<size_t> at;
    // Every template name the files open a block of or add as a child, in order.
    std::vector<std::string> mentioned;
    std::vector<bool> created;
    for (int f = 0; f < 2; ++f)
    {
        for (size_t i = 0; i < files[f].size(); ++i)
        {
            wordsOf(files[f][i].text, words, at);
            if (words.empty() || !startsWith(words[0], kPrefix))
                continue;
            const std::string command = lower(words[0].substr(sizeof(kPrefix) - 1));
            const std::string name = (command == "active" || command == "addtemplate") && words.size() >= 2 ? words[1]
                                   : (command == "create" || command == "activesafe") && words.size() >= 3 ? words[2]
                                   : std::string();
            if (name.empty())
                continue;
            if (lower(name) == lower(source))
            {
                defined = defined || command != "addtemplate";
                continue;
            }
            mentioned.push_back(name);
            created.push_back(command == "create");
        }
    }
    if (!defined)
    {
        error = "the files do not open a block of " + source;
        return false;
    }
    // The children the files create (an S_ name waits for its owner, below).
    for (size_t i = 0; i < mentioned.size(); ++i)
    {
        if (!created[i] || names.count(lower(mentioned[i])) || lower(mentioned[i]).compare(0, 2, "s_") == 0)
            continue;
        const std::string clone = clonedChildName(mentioned[i], source, target);
        names[lower(mentioned[i])] = clone;
        out.renames.push_back(TemplateCloneRename());
        out.renames.back().from = mentioned[i];
        out.renames.back().to = clone;
    }
    // The sounds named after a renamed owner, made by the engine or by the files; an S_ template the files
    // create that is nobody's sound is cloned as a child.
    const std::map<std::string, std::string> owners = names;
    for (size_t i = 0; i < mentioned.size(); ++i)
    {
        if (names.count(lower(mentioned[i])) || lower(mentioned[i]).compare(0, 2, "s_") != 0)
            continue;
        std::string clone = soundRename(mentioned[i], owners);
        if (clone.empty() && !created[i])
            continue;
        if (clone.empty())
            clone = clonedChildName(mentioned[i], source, target);
        else
            ++out.sounds;
        names[lower(mentioned[i])] = clone;
        out.renames.push_back(TemplateCloneRename());
        out.renames.back().from = mentioned[i];
        out.renames.back().to = clone;
    }
    // Each line: geometry and collision lines left out, the include renamed, template names followed.
    std::string texts[2];
    for (int f = 0; f < 2; ++f)
    {
        for (size_t i = 0; i < files[f].size(); ++i)
        {
            const Line& line = files[f][i];
            wordsOf(line.text, words, at);
            std::string text = line.text;
            if (!words.empty() && (startsWith(words[0], "geometrytemplate.") || startsWith(words[0], "collisionmanager.")))
            {
                ++out.droppedLines;
                continue;
            }
            if (words.size() >= 2 && lower(words[0]) == "include" && lower(words[1]) == lower(conStem + ".tweak"))
                text.replace(at[1], words[1].size(), target + ".tweak");
            else if (!words.empty() && startsWith(words[0], kPrefix))
            {
                const std::string command = lower(words[0].substr(sizeof(kPrefix) - 1));
                const bool opening = command == "create" || command == "activesafe";
                if (command == "active")
                    text = renamed(text, 1, names);
                else if (opening)
                    text = renamed(text, 2, names);
                else if (command != "geometry" && command != "collisionmesh" && command != "setcollisionmesh")
                    text = renamed(text, 1, names);
            }
            texts[f] += text + line.end;
        }
    }
    out.con = texts[0];
    out.tweak = texts[1];
    return true;
}

} // namespace bf2editor
