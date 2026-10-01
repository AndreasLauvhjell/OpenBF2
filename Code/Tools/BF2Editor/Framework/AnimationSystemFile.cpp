// Code/Tools/BF2Editor/Framework/AnimationSystemFile.cpp - see AnimationSystemFile.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/AnimationSystemFile.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>

namespace bf2editor
{

namespace
{

bool isBlank(char c)
{
    return c == ' ' || c == '\t';
}

bool same(const std::string& a, const char* b)
{
    return _stricmp(a.c_str(), b) == 0;
}

std::string lowerText(std::string text)
{
    for (size_t i = 0; i < text.size(); ++i)
        if (text[i] >= 'A' && text[i] <= 'Z')
            text[i] = static_cast<char>(text[i] - 'A' + 'a');
    return text;
}

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

// The line's first word and the rest after its blanks.
void splitLine(const std::string& text, std::string& command, std::string& rest)
{
    size_t i = 0;
    while (i < text.size() && isBlank(text[i]))
        ++i;
    size_t j = i;
    while (j < text.size() && !isBlank(text[j]))
        ++j;
    command = text.substr(i, j - i);
    while (j < text.size() && isBlank(text[j]))
        ++j;
    rest = text.substr(j);
    while (!rest.empty() && isBlank(rest[rest.size() - 1]))
        rest.erase(rest.size() - 1);
}

bool startsNoCase(const std::string& text, const char* prefix)
{
    return _strnicmp(text.c_str(), prefix, std::strlen(prefix)) == 0;
}

bool blankLine(const std::string& text)
{
    return text.find_first_not_of(" \t") == std::string::npos;
}

// The line with its value replaced, its command and the blanks before the value kept.
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

const AnimationKind kKinds[4] = { AnimationKind::Animation, AnimationKind::Bundle, AnimationKind::Trigger,
                                  AnimationKind::ValueHolder };

// The writer's order inside an object (census §6.2): a bundle writes its settings, then its animations (each
// with its start time), then its events; a trigger its fadeInTime, its children, its bundles, then its type's
// settings (idleTime, message, valueHolder ...). Animations and value holders hold settings only.
int rankOf(AnimationKind kind, const std::string& property)
{
    if (kind == AnimationKind::Bundle)
    {
        if (same(property, "addAnimation") || same(property, "setAnimationStartTime") || same(property, "setAnimationLength"))
            return 1;
        if (same(property, "addEvent") || same(property, "setEventStartTime"))
            return 2;
        return 0;
    }
    if (kind == AnimationKind::Trigger)
    {
        if (same(property, "fadeInTime"))
            return 0;
        if (same(property, "addChild"))
            return 1;
        if (same(property, "addBundle"))
            return 2;
        return 3;
    }
    return 0;
}

// The lines of one bundle item: its add line and the set lines that name it (start time, length).
struct ItemLines
{
    int add = -1;
    int start = -1;
    int length = -1;
};

ItemLines itemLines(const std::vector<AnimationProperty>& lines, bool event, const std::string& name)
{
    ItemLines found;
    const char* add = event ? "addEvent" : "addAnimation";
    const char* start = event ? "setEventStartTime" : "setAnimationStartTime";
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (same(lines[i].property, add) && _stricmp(lines[i].value.c_str(), name.c_str()) == 0)
            found.add = static_cast<int>(lines[i].line);
        const bool isStart = same(lines[i].property, start);
        const bool isLength = !event && same(lines[i].property, "setAnimationLength");
        if (!isStart && !isLength)
            continue;
        const std::vector<std::string> words = wordsOf(lines[i].value);
        if (words.empty() || _stricmp(words[0].c_str(), name.c_str()) != 0)
            continue;
        (isStart ? found.start : found.length) = static_cast<int>(lines[i].line);
    }
    return found;
}

} // namespace

const char* AnimationSystemFile::createCommand(AnimationKind kind)
{
    switch (kind)
    {
    case AnimationKind::Animation:   return "animationSystem.createAnimation";
    case AnimationKind::Bundle:      return "animationSystem.createBundle";
    case AnimationKind::Trigger:     return "animationSystem.createTrigger";
    default:                         return "AnimationSystem.createValueHolder";
    }
}

const char* AnimationSystemFile::prefix(AnimationKind kind)
{
    switch (kind)
    {
    case AnimationKind::Animation:   return "animationManager.";
    case AnimationKind::Bundle:      return "animationBundle.";
    case AnimationKind::Trigger:     return "animationTrigger.";
    default:                         return "AnimationValueHolder.";
    }
}

const char* AnimationSystemFile::kindName(AnimationKind kind)
{
    switch (kind)
    {
    case AnimationKind::Animation:   return "animation";
    case AnimationKind::Bundle:      return "bundle";
    case AnimationKind::Trigger:     return "trigger";
    default:                         return "valueholder";
    }
}

bool AnimationSystemFile::kindFromName(const std::string& name, AnimationKind& kind)
{
    for (int k = 0; k < 4; ++k)
        if (same(name, kindName(kKinds[k])))
        {
            kind = kKinds[k];
            return true;
        }
    if (same(name, "holder") || same(name, "value"))
    {
        kind = AnimationKind::ValueHolder;
        return true;
    }
    return false;
}

std::string AnimationSystemFile::number(float value)
{
    char text[32];
    std::snprintf(text, sizeof(text), "%g", value);
    return text;
}

void AnimationSystemFile::parse(const std::string& text)
{
    m_lines.clear();
    size_t start = 0;
    while (start < text.size())
    {
        size_t end = text.find('\n', start);
        Line line;
        if (end == std::string::npos)
        {
            line.text = text.substr(start);
            start = text.size();
        }
        else
        {
            const bool cr = end > start && text[end - 1] == '\r';
            line.text = text.substr(start, end - start - (cr ? 1 : 0));
            line.end = cr ? "\r\n" : "\n";
            start = end + 1;
        }
        m_lines.push_back(line);
    }
    m_changed = false;
    index();
}

std::string AnimationSystemFile::render() const
{
    std::string text;
    for (size_t i = 0; i < m_lines.size(); ++i)
        text += m_lines[i].text + m_lines[i].end;
    return text;
}

std::string AnimationSystemFile::newline() const
{
    for (size_t i = 0; i < m_lines.size(); ++i)
        if (!m_lines[i].end.empty())
            return m_lines[i].end;
    return "\r\n";      // the stock systems are all CRLF
}

void AnimationSystemFile::index()
{
    m_objects.clear();
    m_owner.assign(m_lines.size(), -1);
    int current[4] = { -1, -1, -1, -1 };
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        std::string command, rest;
        splitLine(m_lines[i].text, command, rest);
        if (command.empty())
            continue;
        bool created = false;
        for (int k = 0; k < 4 && !created; ++k)
        {
            if (_stricmp(command.c_str(), createCommand(kKinds[k])) != 0)
                continue;
            created = true;
            AnimationObject o;
            o.kind = kKinds[k];
            if (o.kind == AnimationKind::Trigger)
            {
                const std::vector<std::string> words = wordsOf(rest);
                o.type = words.size() > 0 ? words[0] : std::string();
                o.name = words.size() > 1 ? words[1] : std::string();
            }
            else
                o.name = rest;
            o.createLine = i;
            o.lastLine = i;
            current[k] = static_cast<int>(m_objects.size());
            m_owner[i] = current[k];
            m_objects.push_back(o);
        }
        if (created)
            continue;
        for (int k = 0; k < 4; ++k)
        {
            if (!startsNoCase(command, prefix(kKinds[k])) || current[k] < 0)
                continue;
            m_owner[i] = current[k];
            m_objects[static_cast<size_t>(current[k])].lastLine = i;
            break;
        }
    }
}

const AnimationObject* AnimationSystemFile::find(AnimationKind kind, const std::string& name) const
{
    for (size_t i = m_objects.size(); i-- > 0;)
        if (m_objects[i].kind == kind && _stricmp(m_objects[i].name.c_str(), name.c_str()) == 0)
            return &m_objects[i];
    return 0;
}

std::vector<AnimationProperty> AnimationSystemFile::properties(AnimationKind kind, const std::string& name) const
{
    std::vector<AnimationProperty> out;
    const AnimationObject* o = find(kind, name);
    if (!o)
        return out;
    const int owner = ownerOf(o);
    const size_t prefixLength = std::strlen(prefix(kind));
    for (size_t i = o->createLine + 1; i <= o->lastLine && i < m_lines.size(); ++i)
    {
        if (m_owner[i] != owner)
            continue;
        std::string command, rest;
        splitLine(m_lines[i].text, command, rest);
        AnimationProperty p;
        p.property = command.substr(prefixLength);
        p.value = rest;
        p.line = i;
        out.push_back(p);
    }
    return out;
}

std::vector<AnimationProperty> AnimationSystemFile::systemLines() const
{
    std::vector<AnimationProperty> out;
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        std::string command, rest;
        splitLine(m_lines[i].text, command, rest);
        if (!startsNoCase(command, "animationSystem.") || m_owner[i] >= 0)
            continue;
        bool create = false;
        for (int k = 0; k < 4; ++k)
            create = create || _stricmp(command.c_str(), createCommand(kKinds[k])) == 0;
        if (create)
            continue;
        AnimationProperty p;
        p.property = command.substr(std::strlen("animationSystem."));
        p.value = rest;
        p.line = i;
        out.push_back(p);
    }
    return out;
}

bool AnimationSystemFile::insertAt(size_t position, const std::string& text)
{
    Line line;
    line.text = text;
    line.end = newline();
    if (position > m_lines.size())
        position = m_lines.size();
    if (position == m_lines.size() && !m_lines.empty() && m_lines.back().end.empty())
    {
        m_lines.back().end = line.end;      // the old last line needs one now
        line.end.clear();                   // the file keeps its lack of a final line end
    }
    m_lines.insert(m_lines.begin() + static_cast<std::ptrdiff_t>(position), line);
    m_changed = true;
    index();
    return true;
}

size_t AnimationSystemFile::insertionLine(AnimationKind kind, const std::string& name, const std::string& property) const
{
    const AnimationObject* o = find(kind, name);
    if (!o)
        return m_lines.size();
    const int owner = ownerOf(o);
    const int rank = rankOf(kind, property);
    const size_t prefixLength = std::strlen(prefix(kind));
    size_t position = o->createLine + 1;
    for (size_t i = o->createLine + 1; i <= o->lastLine && i < m_lines.size(); ++i)
    {
        if (m_owner[i] != owner)
            continue;
        std::string command, rest;
        splitLine(m_lines[i].text, command, rest);
        if (rankOf(kind, command.substr(prefixLength)) <= rank)
            position = i + 1;
    }
    return position;
}

bool AnimationSystemFile::setProperty(AnimationKind kind, const std::string& name, const std::string& property,
                                      const std::string& value, int which, std::string& error)
{
    const AnimationObject* o = find(kind, name);
    if (!o)
    {
        error = std::string("no ") + createCommand(kind) + " " + name + " in the file";
        return false;
    }
    if (property.empty() || property.find_first_of(" \t") != std::string::npos || value.find_first_of("\r\n") != std::string::npos)
    {
        error = "'" + property + "' is not a property, or the value is more than one line";
        return false;
    }
    std::vector<size_t> matches;
    const std::vector<AnimationProperty> lines = properties(kind, name);
    for (size_t i = 0; i < lines.size(); ++i)
        if (_stricmp(lines[i].property.c_str(), property.c_str()) == 0)
            matches.push_back(lines[i].line);
    if (which < 0 && matches.size() > 1)
    {
        error = property + " appears more than once: name which";
        return false;
    }
    if (which >= 0 && static_cast<size_t>(which) >= matches.size())
    {
        error = "the object has fewer lines of " + property;
        return false;
    }
    if (!matches.empty())
    {
        Line& line = m_lines[matches[which < 0 ? 0 : static_cast<size_t>(which)]];
        line.text = withValue(line.text, value);
        m_changed = true;
        return true;
    }
    return insertAt(insertionLine(kind, name, property), std::string(prefix(kind)) + property + (value.empty() ? "" : " " + value));
}

bool AnimationSystemFile::removeProperty(AnimationKind kind, const std::string& name, const std::string& property, int which,
                                         std::string& error)
{
    std::vector<size_t> matches;
    const std::vector<AnimationProperty> lines = properties(kind, name);
    for (size_t i = 0; i < lines.size(); ++i)
        if (_stricmp(lines[i].property.c_str(), property.c_str()) == 0)
            matches.push_back(lines[i].line);
    if (matches.empty() || (which < 0 && matches.size() > 1) || (which >= 0 && static_cast<size_t>(which) >= matches.size()))
    {
        error = matches.empty() ? "no line of " + property + " to take out" : property + " appears more than once: name which";
        return false;
    }
    m_lines.erase(m_lines.begin() + static_cast<std::ptrdiff_t>(matches[which < 0 ? 0 : static_cast<size_t>(which)]));
    m_changed = true;
    index();
    return true;
}

bool AnimationSystemFile::setSystemProperty(const std::string& property, const std::string& value, std::string& error)
{
    if (property.empty() || property.find_first_of(" \t") != std::string::npos || value.find_first_of("\r\n") != std::string::npos)
    {
        error = "'" + property + "' is not a property, or the value is more than one line";
        return false;
    }
    const std::vector<AnimationProperty> lines = systemLines();
    for (size_t i = 0; i < lines.size(); ++i)
        if (_stricmp(lines[i].property.c_str(), property.c_str()) == 0)
        {
            m_lines[lines[i].line].text = withValue(m_lines[lines[i].line].text, value);
            m_changed = true;
            return true;
        }
    const std::string text = "animationSystem." + property + (value.empty() ? "" : " " + value);
    // After the system's last line; the first one at the top, where the stock files keep the camera spring.
    return insertAt(lines.empty() ? 0 : lines.back().line + 1, text);
}

bool AnimationSystemFile::removeSystemProperty(const std::string& property, std::string& error)
{
    const std::vector<AnimationProperty> lines = systemLines();
    for (size_t i = 0; i < lines.size(); ++i)
        if (_stricmp(lines[i].property.c_str(), property.c_str()) == 0)
        {
            m_lines.erase(m_lines.begin() + static_cast<std::ptrdiff_t>(lines[i].line));
            m_changed = true;
            index();
            return true;
        }
    error = "no animationSystem." + property + " line to take out";
    return false;
}

bool AnimationSystemFile::addLine(AnimationKind kind, const std::string& name, const std::string& property,
                                  const std::string& value, std::string& error)
{
    const AnimationObject* o = find(kind, name);
    if (!o)
    {
        error = std::string("no ") + createCommand(kind) + " " + name + " in the file";
        return false;
    }
    if (property.empty() || property.find_first_of(" \t") != std::string::npos || value.find_first_of("\r\n") != std::string::npos)
    {
        error = "'" + property + "' is not a property, or the value is more than one line";
        return false;
    }
    return insertAt(insertionLine(kind, name, property), std::string(prefix(kind)) + property + (value.empty() ? "" : " " + value));
}

std::vector<BundleItem> AnimationSystemFile::bundleAnimations(const std::string& bundle) const
{
    std::vector<BundleItem> items;
    std::map<std::string, float> starts, lengths;
    const std::vector<AnimationProperty> lines = properties(AnimationKind::Bundle, bundle);
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (same(lines[i].property, "addAnimation"))
        {
            BundleItem item;
            item.name = lines[i].value;
            item.start = item.length = 0.0f;
            item.startSet = item.lengthSet = false;
            items.push_back(item);
            continue;
        }
        const bool isStart = same(lines[i].property, "setAnimationStartTime");
        if (!isStart && !same(lines[i].property, "setAnimationLength"))
            continue;
        const std::vector<std::string> words = wordsOf(lines[i].value);
        if (words.size() >= 2)
            (isStart ? starts : lengths)[lowerText(words[0])] = static_cast<float>(std::atof(words[1].c_str()));
    }
    for (size_t i = 0; i < items.size(); ++i)
    {
        const std::string key = lowerText(items[i].name);
        std::map<std::string, float>::const_iterator it = starts.find(key);
        if (it != starts.end())
        {
            items[i].start = it->second;
            items[i].startSet = true;
        }
        it = lengths.find(key);
        if (it != lengths.end())
        {
            items[i].length = it->second;
            items[i].lengthSet = true;
        }
    }
    return items;
}

std::vector<BundleItem> AnimationSystemFile::bundleEvents(const std::string& bundle) const
{
    std::vector<BundleItem> items;
    std::map<std::string, float> starts;
    const std::vector<AnimationProperty> lines = properties(AnimationKind::Bundle, bundle);
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (same(lines[i].property, "addEvent"))
        {
            BundleItem item;
            item.name = lines[i].value;
            item.start = item.length = 0.0f;
            item.startSet = item.lengthSet = false;
            items.push_back(item);
        }
        else if (same(lines[i].property, "setEventStartTime"))
        {
            const std::vector<std::string> words = wordsOf(lines[i].value);
            if (words.size() >= 2)
                starts[lowerText(words[0])] = static_cast<float>(std::atof(words[1].c_str()));
        }
    }
    for (size_t i = 0; i < items.size(); ++i)
    {
        const std::map<std::string, float>::const_iterator it = starts.find(lowerText(items[i].name));
        if (it != starts.end())
        {
            items[i].start = it->second;
            items[i].startSet = true;
        }
    }
    return items;
}

bool AnimationSystemFile::setAnimationStart(const std::string& bundle, const std::string& path, float start, std::string& error)
{
    const ItemLines at = itemLines(properties(AnimationKind::Bundle, bundle), false, path);
    if (at.add < 0)
    {
        error = "the bundle " + bundle + " has no animation " + path;
        return false;
    }
    if (!(start >= 0.0f && start < 3600.0f))
    {
        error = "a start time is in seconds, 0 or more";
        return false;
    }
    const std::string value = path + " " + number(start);
    if (at.start >= 0)
    {
        m_lines[static_cast<size_t>(at.start)].text = withValue(m_lines[static_cast<size_t>(at.start)].text, value);
        m_changed = true;
        return true;
    }
    // Right after its addAnimation line, where the stock files put it.
    return insertAfter(static_cast<size_t>(at.add), std::string(prefix(AnimationKind::Bundle)) + "setAnimationStartTime " + value);
}

bool AnimationSystemFile::setEventStart(const std::string& bundle, const std::string& event, float start, std::string& error)
{
    const ItemLines at = itemLines(properties(AnimationKind::Bundle, bundle), true, event);
    if (at.add < 0)
    {
        error = "the bundle " + bundle + " has no event " + event;
        return false;
    }
    if (!(start >= 0.0f && start < 3600.0f))
    {
        error = "a start time is in seconds, 0 or more";
        return false;
    }
    const std::string value = event + " " + number(start);
    if (at.start >= 0)
    {
        m_lines[static_cast<size_t>(at.start)].text = withValue(m_lines[static_cast<size_t>(at.start)].text, value);
        m_changed = true;
        return true;
    }
    return insertAfter(static_cast<size_t>(at.add), std::string(prefix(AnimationKind::Bundle)) + "setEventStartTime " + value);
}

bool AnimationSystemFile::setAnimationLength(const std::string& bundle, const std::string& path, float length, std::string& error)
{
    const ItemLines at = itemLines(properties(AnimationKind::Bundle, bundle), false, path);
    if (at.add < 0)
    {
        error = "the bundle " + bundle + " has no animation " + path;
        return false;
    }
    if (!(length > 0.0f && length < 3600.0f))
    {
        error = "a length is in seconds and more than 0 (the engine divides by it)";
        return false;
    }
    const std::string value = path + " " + number(length);
    if (at.length >= 0)
    {
        m_lines[static_cast<size_t>(at.length)].text = withValue(m_lines[static_cast<size_t>(at.length)].text, value);
        m_changed = true;
        return true;
    }
    // After its start-time line when it has one, else after its addAnimation line.
    const int after = at.start > at.add ? at.start : at.add;
    return insertAfter(static_cast<size_t>(after), std::string(prefix(AnimationKind::Bundle)) + "setAnimationLength " + value);
}

bool AnimationSystemFile::removeBundleItem(const std::string& bundle, const std::string& name, bool event, std::string& error)
{
    const ItemLines at = itemLines(properties(AnimationKind::Bundle, bundle), event, name);
    if (at.add < 0)
    {
        error = "the bundle " + bundle + " has no " + (event ? "event " : "animation ") + name;
        return false;
    }
    std::vector<int> doomed;
    doomed.push_back(at.add);
    if (at.start >= 0)
        doomed.push_back(at.start);
    if (at.length >= 0)
        doomed.push_back(at.length);
    std::sort(doomed.begin(), doomed.end());
    for (size_t i = doomed.size(); i-- > 0;)        // the later lines first, so the earlier indexes stay right
        m_lines.erase(m_lines.begin() + doomed[i]);
    m_changed = true;
    index();
    return true;
}

std::vector<std::string> AnimationSystemFile::triggerChildren(const std::string& trigger) const
{
    std::vector<std::string> out;
    const std::vector<AnimationProperty> lines = properties(AnimationKind::Trigger, trigger);
    for (size_t i = 0; i < lines.size(); ++i)
        if (same(lines[i].property, "addChild") && !lines[i].value.empty())
            out.push_back(lines[i].value);
    return out;
}

std::vector<std::string> AnimationSystemFile::triggerBundles(const std::string& trigger) const
{
    std::vector<std::string> out;
    const std::vector<AnimationProperty> lines = properties(AnimationKind::Trigger, trigger);
    for (size_t i = 0; i < lines.size(); ++i)
        if (same(lines[i].property, "addBundle") && !lines[i].value.empty())
            out.push_back(lines[i].value);
    return out;
}

std::vector<std::string> AnimationSystemFile::triggerRoots() const
{
    std::set<std::string> children;
    for (size_t i = 0; i < m_objects.size(); ++i)
    {
        if (m_objects[i].kind != AnimationKind::Trigger)
            continue;
        const std::vector<std::string> named = triggerChildren(m_objects[i].name);
        for (size_t k = 0; k < named.size(); ++k)
            children.insert(lowerText(named[k]));
    }
    std::vector<std::string> roots;
    std::set<std::string> seen;
    for (size_t i = 0; i < m_objects.size(); ++i)
    {
        if (m_objects[i].kind != AnimationKind::Trigger)
            continue;
        const std::string key = lowerText(m_objects[i].name);
        if (children.count(key) == 0 && seen.insert(key).second)
            roots.push_back(m_objects[i].name);
    }
    return roots;
}

std::vector<std::string> AnimationSystemFile::triggerSubtree(const std::string& trigger) const
{
    std::vector<std::string> out;
    if (!find(AnimationKind::Trigger, trigger))
        return out;
    std::set<std::string> seen;
    std::vector<std::string> pending(1, trigger);
    while (!pending.empty())
    {
        const std::string name = pending.back();
        pending.pop_back();
        const AnimationObject* o = find(AnimationKind::Trigger, name);
        if (!o || !seen.insert(lowerText(name)).second)
            continue;
        out.push_back(o->name);
        const std::vector<std::string> children = triggerChildren(name);
        for (size_t i = children.size(); i-- > 0;)
            pending.push_back(children[i]);
    }
    return out;
}

bool AnimationSystemFile::createdBefore(AnimationKind kind, const std::string& name, size_t at) const
{
    for (size_t i = 0; i < m_objects.size(); ++i)
        if (m_objects[i].kind == kind && m_objects[i].createLine < at && _stricmp(m_objects[i].name.c_str(), name.c_str()) == 0)
            return true;
    return false;
}

int AnimationSystemFile::removeReferences(AnimationKind ownerKind, const std::string& property, const std::string& value)
{
    const std::string command = std::string(prefix(ownerKind)) + property;
    std::vector<size_t> doomed;
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        if (m_owner[i] < 0 || m_objects[static_cast<size_t>(m_owner[i])].kind != ownerKind)
            continue;
        std::string c, rest;
        splitLine(m_lines[i].text, c, rest);
        if (_stricmp(c.c_str(), command.c_str()) == 0 && _stricmp(rest.c_str(), value.c_str()) == 0)
            doomed.push_back(i);
    }
    for (size_t i = doomed.size(); i-- > 0;)
        m_lines.erase(m_lines.begin() + static_cast<std::ptrdiff_t>(doomed[i]));
    if (!doomed.empty())
    {
        m_changed = true;
        index();
    }
    return static_cast<int>(doomed.size());
}

bool AnimationSystemFile::addObject(AnimationKind kind, const std::string& type, const std::string& name, std::string& error,
                                    size_t before)
{
    if (name.empty() || name.find_first_of(" \t\r\n") != std::string::npos
        || (kind == AnimationKind::Trigger && (type.empty() || type.find_first_of(" \t\r\n") != std::string::npos)))
    {
        error = kind == AnimationKind::Trigger ? "a trigger needs a type and a name, each one word" : "the name is one word";
        return false;
    }
    if (find(kind, name))
    {
        error = name + " is in the file already";
        return false;
    }
    const std::string text = std::string(createCommand(kind)) + " " + (kind == AnimationKind::Trigger ? type + " " : std::string()) + name;
    if (m_lines.empty())
        return insertAt(0, text);
    // The writer's layout (census §6.2): bundles and triggers are blocks with a blank line between them;
    // animations and value holders follow one another. Animations, bundles and value holders go in name order
    // (no case) - above the first whose name sorts after the new one; a trigger, or a name that sorts last,
    // after the last object of its kind; with none of its kind, at the end of the file.
    const bool spaced = kind == AnimationKind::Bundle || kind == AnimationKind::Trigger;
    int above = -1;
    int last = -1;
    for (size_t i = 0; i < m_objects.size(); ++i)
    {
        if (m_objects[i].kind != kind)
            continue;
        last = static_cast<int>(m_objects[i].lastLine);
        if (kind != AnimationKind::Trigger && above < 0 && _stricmp(m_objects[i].name.c_str(), name.c_str()) > 0)
            above = static_cast<int>(m_objects[i].createLine);
    }
    size_t position = above >= 0 ? static_cast<size_t>(above) : last >= 0 ? static_cast<size_t>(last) + 1 : m_lines.size();
    bool placeAbove = above >= 0;
    if (before < m_lines.size() && position > before)
    {
        // Above line `before` instead, so the block that will name the new object finds it at load.
        position = before;
        placeAbove = true;
    }
    if (placeAbove)
    {
        insertAt(position, text);
        return spaced ? insertAt(position + 1, std::string()) : true;
    }
    if (!spaced && last >= 0)
        return insertAt(position, text);
    insertAt(position, std::string());
    return insertAt(position + 1, text);
}

bool AnimationSystemFile::removeObject(AnimationKind kind, const std::string& name, std::string& error)
{
    const AnimationObject* o = find(kind, name);
    if (!o)
    {
        error = std::string("no ") + createCommand(kind) + " " + name + " in the file";
        return false;
    }
    const int owner = ownerOf(o);
    const size_t first = o->createLine;
    const size_t last = o->lastLine;
    std::vector<size_t> doomed;
    for (size_t i = first; i <= last && i < m_lines.size(); ++i)
        if (m_owner[i] == owner)
            doomed.push_back(i);
    // A bundle or trigger block takes the blank line before its create line with it (the separator addObject
    // wrote); when there is none (the first block under a header), the blank line after it. Animations and value
    // holders have no separators.
    const bool spaced = kind == AnimationKind::Bundle || kind == AnimationKind::Trigger;
    if (spaced && first > 0 && blankLine(m_lines[first - 1].text) && m_owner[first - 1] < 0)
        doomed.insert(doomed.begin(), first - 1);
    else if (spaced && last + 1 < m_lines.size() && blankLine(m_lines[last + 1].text) && m_owner[last + 1] < 0)
        doomed.push_back(last + 1);
    for (size_t i = doomed.size(); i-- > 0;)
        m_lines.erase(m_lines.begin() + static_cast<std::ptrdiff_t>(doomed[i]));
    m_changed = true;
    index();
    return true;
}

} // namespace bf2editor
