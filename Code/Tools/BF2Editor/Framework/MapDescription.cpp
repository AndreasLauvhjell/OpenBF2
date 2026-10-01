// Code/Tools/BF2Editor/Framework/MapDescription.cpp - see MapDescription.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/MapDescription.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace bf2editor
{

namespace
{

const size_t npos = std::string::npos;

std::string lowered(const std::string& text)
{
    std::string out(text);
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

// The value of `name="..."` in a tag's text; "" when it has none.
std::string attribute(const std::string& tag, const std::string& name)
{
    const std::string lower = lowered(tag);
    size_t at = 0;
    while ((at = lower.find(name, at)) != npos)
    {
        const bool starts = at == 0 || std::isspace(static_cast<unsigned char>(lower[at - 1]));
        size_t i = at + name.size();
        while (i < lower.size() && std::isspace(static_cast<unsigned char>(lower[i])))
            ++i;
        if (starts && i < lower.size() && lower[i] == '=')
        {
            ++i;
            while (i < lower.size() && std::isspace(static_cast<unsigned char>(lower[i])))
                ++i;
            if (i < tag.size() && (tag[i] == '"' || tag[i] == '\''))
            {
                const char quote = tag[i];
                const size_t end = tag.find(quote, i + 1);
                if (end != npos)
                    return tag.substr(i + 1, end - i - 1);
            }
        }
        at += name.size();
    }
    return std::string();
}

// A mode element: its opening tag's start and end (one past '>'), its closing tag's start, and the
// maptype tags inside (each its start).
struct ModeElement
{
    std::string         type;
    size_t              open;
    size_t              openEnd;
    size_t              close;
    std::vector<size_t> maptypes;
};

std::vector<ModeElement> modeElements(const std::string& xml)
{
    std::vector<ModeElement> modes;
    const std::string lower = lowered(xml);
    size_t at = 0;
    while ((at = lower.find("<mode", at)) != npos)
    {
        const char next = at + 5 < lower.size() ? lower[at + 5] : 0;
        if (!std::isspace(static_cast<unsigned char>(next)) && next != '>')
        {
            at += 5;                                        // <modes
            continue;
        }
        ModeElement mode;
        mode.open = at;
        const size_t tagEnd = lower.find('>', at);
        if (tagEnd == npos)
            break;
        mode.openEnd = tagEnd + 1;
        mode.type = attribute(xml.substr(at, tagEnd - at), "type");
        mode.close = lower.find("</mode>", mode.openEnd);
        if (mode.close == npos)
            break;
        for (size_t m = lower.find("<maptype", mode.openEnd); m != npos && m < mode.close; m = lower.find("<maptype", m + 8))
            mode.maptypes.push_back(m);
        modes.push_back(mode);
        at = mode.close;
    }
    return modes;
}

size_t lineStart(const std::string& text, size_t at)
{
    while (at > 0 && text[at - 1] != '\n')
        --at;
    return at;
}

size_t lineEnd(const std::string& text, size_t at)          // one past the line's newline
{
    const size_t newline = text.find('\n', at);
    return newline == npos ? text.size() : newline + 1;
}

// A maptype line with its players set and without ai="1".
std::string retargeted(const std::string& line, int players)
{
    std::string out = line;
    const std::string lower = lowered(out);
    const size_t at = lower.find("players=\"");
    if (at != npos)
    {
        const size_t end = out.find('"', at + 9);
        char text[16];
        std::snprintf(text, sizeof(text), "%d", players);
        out.replace(at + 9, end - at - 9, text);
    }
    const std::string lower2 = lowered(out);
    const size_t ai = lower2.find(" ai=\"1\"");
    if (ai != npos)
        out.erase(ai, 7);
    return out;
}

} // namespace

std::vector<std::pair<std::string, int> > mapDescriptionLayers(const std::string& xml)
{
    std::vector<std::pair<std::string, int> > layers;
    const std::vector<ModeElement> modes = modeElements(xml);
    for (size_t i = 0; i < modes.size(); ++i)
    {
        for (size_t k = 0; k < modes[i].maptypes.size(); ++k)
        {
            const size_t start = modes[i].maptypes[k];
            const size_t end = xml.find('>', start);
            const std::string players = attribute(xml.substr(start, end - start), "players");
            if (!players.empty())
                layers.push_back(std::make_pair(modes[i].type, std::atoi(players.c_str())));
        }
    }
    return layers;
}

bool addMapDescriptionLayer(std::string& xml, const std::string& mode, int players)
{
    const std::vector<std::pair<std::string, int> > listed = mapDescriptionLayers(xml);
    for (size_t i = 0; i < listed.size(); ++i)
        if (lowered(listed[i].first) == lowered(mode) && listed[i].second == players)
            return false;
    const std::vector<ModeElement> modes = modeElements(xml);
    for (size_t i = 0; i < modes.size(); ++i)
    {
        if (lowered(modes[i].type) != lowered(mode) || modes[i].maptypes.empty())
            continue;
        // After the mode's last maptype line, a copy of it.
        const size_t last = modes[i].maptypes.back();
        const size_t start = lineStart(xml, last);
        const size_t end = lineEnd(xml, last);
        std::string line = xml.substr(start, end - start);
        if (line.empty() || line[line.size() - 1] != '\n')
            line += "\r\n";
        xml.insert(end, retargeted(line, players));
        return true;
    }
    // A mode of its own, before the modes element closes: the last mode's opening line and first
    // maptype line, retargeted.
    const size_t closeModes = lowered(xml).find("</modes>");
    if (closeModes == npos || modes.empty() || modes.back().maptypes.empty())
        return false;
    const ModeElement& model = modes.back();
    std::string open = xml.substr(lineStart(xml, model.open), lineEnd(xml, model.open) - lineStart(xml, model.open));
    std::string maptype = xml.substr(lineStart(xml, model.maptypes[0]),
                                     lineEnd(xml, model.maptypes[0]) - lineStart(xml, model.maptypes[0]));
    std::string close = xml.substr(lineStart(xml, model.close), lineEnd(xml, model.close) - lineStart(xml, model.close));
    const size_t typeAt = lowered(open).find("type=\"");
    if (typeAt == npos)
        return false;
    open.replace(typeAt + 6, open.find('"', typeAt + 6) - typeAt - 6, mode);
    xml.insert(lineStart(xml, closeModes), open + retargeted(maptype, players) + close);
    return true;
}

} // namespace bf2editor
