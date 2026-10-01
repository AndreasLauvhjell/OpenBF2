// Code/Tools/BF2Editor/Framework/LevelObjectFile.cpp - see LevelObjectFile.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/LevelObjectFile.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>

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

// "Object.absolutePosition 1/2/3" -> true, with `rest` = "1/2/3". The console ignores case in names.
bool startsWithCommand(const std::string& line, const char* command, std::string& rest)
{
    const std::string text = trimmed(line);
    const size_t length = std::strlen(command);
    if (text.size() < length || lowered(text.substr(0, length)) != lowered(command))
        return false;
    if (text.size() > length && !std::isspace(static_cast<unsigned char>(text[length])))
        return false;
    rest = trimmed(text.substr(length));
    return true;
}

bool isObjectPropertyLine(const std::string& line)
{
    const std::string text = lowered(trimmed(line));
    return text.compare(0, 7, "object.") == 0 && text.compare(0, 14, "object.create ") != 0
        && text != "object.create";
}

bool isRemBannerFor(const std::string& line)
{
    // "rem * name *", as the retail writer puts before each record of StaticObjects.con, and
    // "rem [<class>: name]" before each of the gameplay layers' and AmbientObjects.con's.
    const std::string text = trimmed(line);
    if (text.size() < 8 || lowered(text.substr(0, 4)) != "rem ")
        return false;
    if (text.find("***") != std::string::npos)
        return true;
    const std::string rest = trimmed(text.substr(4));
    return rest.size() >= 3 && rest[0] == '[' && rest[rest.size() - 1] == ']';
}

// Angles compared as printed with `decimals` digits, modulo 360: -180.0 and 180.0 are the same
// rotation.
bool anglesDiffer(float a, float b, int decimals)
{
    float difference = std::fmod(std::fabs(a - b), 360.0f);
    if (difference > 180.0f)
        difference = 360.0f - difference;
    return difference > 0.5f * static_cast<float>(std::pow(10.0, -decimals));
}

void formatNumber(char* out, size_t size, const char* format, float value)
{
    std::snprintf(out, size, format, value);
    // Never write "-0.0" / "-0.000": the retail files hold none.
    if (out[0] == '-')
    {
        bool allZero = true;
        for (const char* c = out + 1; *c; ++c)
            if (*c != '0' && *c != '.')
                allZero = false;
        if (allZero)
            std::memmove(out, out + 1, std::strlen(out));
    }
}

std::string formatTriple(const float values[3], int decimals)
{
    char format[8];
    std::snprintf(format, sizeof(format), "%%.%df", decimals < 0 ? 0 : (decimals > 9 ? 9 : decimals));
    char parts[3][48];
    for (int i = 0; i < 3; ++i)
        formatNumber(parts[i], sizeof(parts[i]), format, values[i]);
    return std::string(parts[0]) + "/" + parts[1] + "/" + parts[2];
}

// The digits after the point in a triple's first number: "90.000/0.000/0.000" -> 3.
int decimalsOf(const std::string& triple)
{
    const size_t point = triple.find('.');
    const size_t end = triple.find('/');
    if (point == std::string::npos || (end != std::string::npos && point > end))
        return 0;
    int digits = 0;
    for (size_t i = point + 1; i < triple.size() && std::isdigit(static_cast<unsigned char>(triple[i])); ++i)
        ++digits;
    return digits;
}

// "ObjectTemplate.<property> <value>" -> property and value (the value trimmed, quotes kept).
bool splitTemplateLine(const std::string& line, std::string& property, std::string& value)
{
    const std::string text = trimmed(line);
    const std::string prefix = "objecttemplate.";
    if (text.size() <= prefix.size() || lowered(text.substr(0, prefix.size())) != prefix)
        return false;
    const std::string rest = text.substr(prefix.size());
    const size_t space = rest.find_first_of(" \t");
    property = space == std::string::npos ? rest : rest.substr(0, space);
    value = space == std::string::npos ? std::string() : trimmed(rest.substr(space));
    return !property.empty();
}

// A whole decimal integer.
bool parseInteger(const std::string& text, long& out)
{
    if (text.empty())
        return false;
    char* end = 0;
    out = std::strtol(text.c_str(), &end, 10);
    return end && *end == 0 && end != text.c_str();
}

// "Object.property value" -> its three parts, trimmed (false for any other line).
bool splitStatement(const std::string& line, std::string& object, std::string& property, std::string& value)
{
    const std::string text = trimmed(line);
    const size_t dot = text.find('.');
    if (dot == std::string::npos || dot == 0)
        return false;
    const size_t space = text.find_first_of(" \t", dot);
    object = text.substr(0, dot);
    if (object.find_first_of(" \t") != std::string::npos)
        return false;
    property = text.substr(dot + 1, space == std::string::npos ? std::string::npos : space - dot - 1);
    value = space == std::string::npos ? std::string() : trimmed(text.substr(space));
    return !property.empty();
}

bool parseFloat(const std::string& text, float& out)
{
    if (text.empty())
        return false;
    char* end = 0;
    const double value = std::strtod(text.c_str(), &end);
    if (!end || *end != 0 || end == text.c_str())
        return false;
    out = static_cast<float>(value);
    return true;
}

// "x/z" -> two floats.
bool parsePair(const std::string& text, float out[2])
{
    const size_t slash = text.find('/');
    return slash != std::string::npos && parseFloat(trimmed(text.substr(0, slash)), out[0])
        && parseFloat(trimmed(text.substr(slash + 1)), out[1]);
}

// The whitespace a line starts with.
std::string indentOf(const std::string& line)
{
    size_t n = 0;
    while (n < line.size() && (line[n] == ' ' || line[n] == '\t'))
        ++n;
    return line.substr(0, n);
}

} // namespace

ObjectPlacement::ObjectPlacement()
    : layer(0)
    , hasLayer(false)
    , group(0)
    , disableChildren(false)
    , notInAI(false)
    , isOvergrowth(false)
    , lightSourceMask(0)
    , hasLightSourceMask(false)
    , controlPointId(0)
    , hasControlPointId(false)
    , hostOnly(false)
    , omitZeroRotation(false)
{
    for (int i = 0; i < 3; ++i)
        position[i] = rotation[i] = 0.0f;
}

LevelObjectFile::CombatAreaData::CombatAreaData()
    : team(0)
    , vehicles(4)
    , layer(1)
    , usedByPathFinding(false)
    , inverted(false)
{
    minimum[0] = minimum[1] = maximum[0] = maximum[1] = 0.0f;
}

LevelObjectFile::CombatAreaSettings::CombatAreaSettings()
    : hasUse(false)
    , use(false)
    , hasDamage(false)
    , damage(20.0f)
    , hasTimeAllowedOutside(false)
    , timeAllowedOutside(10.0f)
{
}

LevelObjectFile::LevelObjectFile()
    : m_finalNewline(true)
    , m_crlf(true)
    , m_preloadBegin(npos)
    , m_preloadEnd(npos)
{
    m_settingLine[0] = m_settingLine[1] = m_settingLine[2] = npos;
}

bool LevelObjectFile::preloads(const std::string& runLine) const
{
    if (!hasPreloadBlock())
        return false;
    const std::string wanted = lowered(trimmed(runLine));
    for (size_t i = m_preloadBegin + 1; i < m_preloadEnd; ++i)
        if (lowered(trimmed(m_lines[i].text)) == wanted)
            return true;
    for (size_t i = 0; i < m_extraPreload.size(); ++i)
        if (lowered(trimmed(m_extraPreload[i])) == wanted)
            return true;
    return false;
}

std::string LevelObjectFile::formatPosition(const float position[3], int decimals)
{
    return formatTriple(position, decimals);
}

std::string LevelObjectFile::formatRotation(const float rotation[3], int decimals)
{
    return formatTriple(rotation, decimals);
}

bool LevelObjectFile::rotationPrintsAsZero(const float rotation[3], int decimals)
{
    const float zero[3] = { 0.0f, 0.0f, 0.0f };
    return formatRotation(rotation, decimals) == formatRotation(zero, decimals);
}

bool LevelObjectFile::parseTriple(const std::string& text, float out[3])
{
    const char* p = text.c_str();
    for (int i = 0; i < 3; ++i)
    {
        char* end = 0;
        const double value = std::strtod(p, &end);
        if (end == p)
            return false;
        out[i] = static_cast<float>(value);
        p = end;
        if (i < 2)
        {
            if (*p != '/')
                return false;
            ++p;
        }
    }
    while (*p && std::isspace(static_cast<unsigned char>(*p)))
        ++p;
    return *p == 0;
}

void LevelObjectFile::parse(const std::string& text)
{
    m_lines.clear();
    m_records.clear();
    m_extraPreload.clear();
    m_lineOverride.clear();
    m_preloadBegin = m_preloadEnd = npos;
    m_style = Style();
    bool positionStyleSeen = false;
    bool rotationStyleSeen = false;
    size_t start = 0;
    int crlfLines = 0;
    while (start < text.size())
    {
        const size_t newline = text.find('\n', start);
        Line line;
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

    // The preload block: the first `if v_arg1 == BF2Editor` whose body is only run, console and rem
    // lines, up to its endIf.
    for (size_t i = 0; i < m_lines.size() && m_preloadBegin == npos; ++i)
    {
        const std::string text2 = lowered(trimmed(m_lines[i].text));
        std::string compact;
        for (size_t k = 0; k < text2.size(); ++k)
            if (!std::isspace(static_cast<unsigned char>(text2[k])))
                compact += text2[k];
        if (compact != "ifv_arg1==bf2editor")
            continue;
        for (size_t j = i + 1; j < m_lines.size(); ++j)
        {
            const std::string body = lowered(trimmed(m_lines[j].text));
            if (body == "endif")
            {
                m_preloadBegin = i;
                m_preloadEnd = j;
                break;
            }
            if (!body.empty() && body.compare(0, 4, "run ") != 0 && body.compare(0, 8, "console.") != 0
                && body.compare(0, 3, "rem") != 0)
                break;                                      // not a plain preload block
        }
    }

    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        std::string rest;
        if (!startsWithCommand(m_lines[i].text, "Object.create", rest) || rest.empty())
            continue;
        Record record;
        record.createLine = i;
        record.firstLine = (i > 0 && isRemBannerFor(m_lines[i - 1].text)
                            && (m_records.empty() || m_records.back().endLine <= i - 1)) ? i - 1 : i;
        for (int s = 0; s < SlotCount; ++s)
            record.slotLine[s] = npos;
        record.isNew = false;
        record.deleted = false;
        record.loaded.templateName = trimmed(rest.substr(0, rest.find_first_of(" \t")));
        size_t j = i + 1;
        for (; j < m_lines.size() && isObjectPropertyLine(m_lines[j].text); ++j)
        {
            std::string value;
            float triple[3];
            long number = 0;
            if (startsWithCommand(m_lines[j].text, "Object.absolutePosition", value) && parseTriple(value, triple))
            {
                record.slotLine[SlotPosition] = j;
                for (int k = 0; k < 3; ++k)
                    record.loaded.position[k] = triple[k];
                if (!positionStyleSeen)
                {
                    // Never fewer digits than the retail writer's: a hand-written "0/0/0" must not
                    // round every later save to whole metres.
                    m_style.positionDecimals = std::max(decimalsOf(value), 3);
                    positionStyleSeen = true;
                }
            }
            else if (startsWithCommand(m_lines[j].text, "Object.rotation", value) && parseTriple(value, triple))
            {
                record.slotLine[SlotRotation] = j;
                for (int k = 0; k < 3; ++k)
                    record.loaded.rotation[k] = triple[k];
                if (!rotationStyleSeen)
                {
                    m_style.rotationDecimals = std::max(decimalsOf(value), 1);
                    rotationStyleSeen = true;
                }
                // A file that writes a zero rotation keeps writing them.
                if (triple[0] == 0.0f && triple[1] == 0.0f && triple[2] == 0.0f)
                    m_style.keepZeroRotation = true;
            }
            else if (startsWithCommand(m_lines[j].text, "Object.setControlPointId", value) && parseInteger(value, number))
            {
                record.slotLine[SlotControlPointId] = j;
                record.loaded.controlPointId = static_cast<int>(number);
                record.loaded.hasControlPointId = true;
            }
            else if (startsWithCommand(m_lines[j].text, "Object.layer", value) && parseInteger(value, number))
            {
                record.slotLine[SlotLayer] = j;
                record.loaded.layer = static_cast<int>(number);
                record.loaded.hasLayer = true;
            }
            else if (startsWithCommand(m_lines[j].text, "Object.group", value) && parseInteger(value, number))
            {
                record.slotLine[SlotGroup] = j;
                record.loaded.group = static_cast<int>(number);
            }
            else if (startsWithCommand(m_lines[j].text, "Object.disableChildren", value) && parseInteger(value, number))
            {
                record.slotLine[SlotDisableChildren] = j;
                record.loaded.disableChildren = number != 0;
            }
            else if (startsWithCommand(m_lines[j].text, "Object.notInAI", value) && parseInteger(value, number))
            {
                record.slotLine[SlotNotInAI] = j;
                record.loaded.notInAI = number != 0;
            }
            else if (startsWithCommand(m_lines[j].text, "Object.isOvergrowth", value) && parseInteger(value, number))
            {
                record.slotLine[SlotIsOvergrowth] = j;
                record.loaded.isOvergrowth = number != 0;
            }
            else if (startsWithCommand(m_lines[j].text, "Object.setLightSourceMask", value) && parseInteger(value, number))
            {
                record.slotLine[SlotLightSourceMask] = j;
                record.loaded.lightSourceMask = static_cast<int>(number);
                record.loaded.hasLightSourceMask = true;
            }
        }
        record.endLine = j;
        record.current = record.loaded;
        m_records.push_back(record);
        i = j - 1;
    }

    // Template blocks: an ObjectTemplate.create line and the ObjectTemplate.* lines right after it.
    m_templates.clear();
    m_templateByName.clear();
    m_templateEdits.clear();
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        std::string property, value;
        if (!splitTemplateLine(m_lines[i].text, property, value) || lowered(property) != "create")
            continue;
        const size_t space = value.find_first_of(" \t");
        if (space == std::string::npos)
            continue;
        TemplateBlock block;
        block.className = value.substr(0, space);
        block.name = trimmed(value.substr(space));
        block.fileName = block.name;
        block.createLine = block.lastLine = i;
        // The block runs on over `rem` lines - the retail writer separates a control point's parts with
        // "rem -----" inside its block - and ends at a blank line or any other statement.
        size_t j = i + 1;
        for (; j < m_lines.size(); ++j)
        {
            if (!splitTemplateLine(m_lines[j].text, property, value))
            {
                if (lowered(trimmed(m_lines[j].text)).compare(0, 3, "rem") == 0)
                    continue;
                break;
            }
            const std::string low = lowered(property);
            if (low == "create")
                break;
            block.lastLine = j;
            if (low != "activesafe")
                block.propertyLines.push_back(j);
        }
        if (m_templateByName.find(lowered(block.name)) == m_templateByName.end())
            m_templateByName[lowered(block.name)] = m_templates.size();
        m_templates.push_back(block);
        i = j - 1;
    }

    // Each ambient sound's `Sound.addTrigger <name>` line belongs to its template block.
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        std::string rest;
        if (!startsWithCommand(m_lines[i].text, "Sound.addTrigger", rest) || rest.empty())
            continue;
        std::map<std::string, size_t>::const_iterator block = m_templateByName.find(lowered(rest));
        if (block != m_templateByName.end() && m_templates[block->second].triggerLine == npos)
            m_templates[block->second].triggerLine = i;
    }

    // Combat areas: a CombatArea.create line and the CombatArea.* lines right after it; and the
    // manager's lines wherever they stand.
    m_areas.clear();
    m_areaSettings = CombatAreaSettings();
    m_settingLine[0] = m_settingLine[1] = m_settingLine[2] = npos;
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        std::string object, property, value;
        if (!splitStatement(m_lines[i].text, object, property, value))
            continue;
        const std::string lowObject = lowered(object);
        const std::string lowProperty = lowered(property);
        if (lowObject == "combatareamanager")
        {
            long number = 0;
            float real = 0.0f;
            if (lowProperty == "use" && parseInteger(value, number))
            {
                m_areaSettings.hasUse = true;
                m_areaSettings.use = number != 0;
                m_settingLine[0] = i;
            }
            else if (lowProperty == "damage" && parseFloat(value, real))
            {
                m_areaSettings.hasDamage = true;
                m_areaSettings.damage = real;
                m_settingLine[1] = i;
            }
            else if (lowProperty == "timeallowedoutside" && parseFloat(value, real))
            {
                m_areaSettings.hasTimeAllowedOutside = true;
                m_areaSettings.timeAllowedOutside = real;
                m_settingLine[2] = i;
            }
            continue;
        }
        if (lowObject != "combatarea" || lowProperty != "create" || value.empty())
            continue;
        AreaBlock block;
        block.createLine = i;
        block.isNew = false;
        block.deleted = false;
        block.writable = value.find_first_of(" \t") == std::string::npos;
        block.loaded.name = value;
        size_t j = i + 1;
        for (; j < m_lines.size(); ++j)
        {
            if (!splitStatement(m_lines[j].text, object, property, value) || lowered(object) != "combatarea")
                break;
            const std::string p = lowered(property);
            if (p == "create")
                break;
            long number = 0;
            float pair[2];
            bool understood = true;
            if (p == "min")
                understood = parsePair(value, block.loaded.minimum);
            else if (p == "max")
                understood = parsePair(value, block.loaded.maximum);
            else if (p == "addareapoint" && parsePair(value, pair))
                block.loaded.points.push_back(std::make_pair(pair[0], pair[1]));
            else if (p == "team" && parseInteger(value, number))
                block.loaded.team = static_cast<int>(number);
            else if (p == "vehicles" && parseInteger(value, number))
                block.loaded.vehicles = static_cast<int>(number);
            else if (p == "layer" && parseInteger(value, number))
                block.loaded.layer = static_cast<int>(number);
            else if (p == "usedbypathfinding" && parseInteger(value, number))
                block.loaded.usedByPathFinding = number != 0;
            else if (p == "inverted" && parseInteger(value, number))
                block.loaded.inverted = number != 0;
            else
                understood = false;
            block.writable = block.writable && understood;
        }
        block.endLine = j;
        block.current = block.loaded;
        m_areas.push_back(block);
        i = j - 1;
    }
    m_loadedAreaSettings = m_areaSettings;
}

std::string LevelObjectFile::templateKey(const std::string& property, const std::string& value)
{
    const std::string low = lowered(property);
    // The indexed properties: the first word of the value is part of the key.
    if (low == "setobjecttemplate" || low == "mapmaterial")
    {
        const std::string first = value.substr(0, value.find_first_of(" \t"));
        return low + " " + first;
    }
    // A control point's supplyGroupNeeded <team> <group> <needed>: one line per team and group (1.50's
    // addSupplyGroupNeeded stops the game on a second line of the same pair).
    if (low == "supplygroupneeded")
    {
        std::string words[2];
        size_t at = 0;
        for (int w = 0; w < 2; ++w)
        {
            at = value.find_first_not_of(" \t", at);
            if (at == std::string::npos)
                break;
            const size_t end = value.find_first_of(" \t", at);
            words[w] = value.substr(at, end == std::string::npos ? std::string::npos : end - at);
            at = end;
        }
        return low + " " + words[0] + " " + words[1];
    }
    return low;
}

const LevelObjectFile::TemplateBlock* LevelObjectFile::templateBlock(const std::string& name) const
{
    std::map<std::string, size_t>::const_iterator found = m_templateByName.find(lowered(name));
    return found == m_templateByName.end() ? 0 : &m_templates[found->second];
}

bool LevelObjectFile::findTemplate(const std::string& name, std::string& className) const
{
    const TemplateBlock* block = templateBlock(name);
    if (!block)
        return false;
    className = block->className;
    return true;
}

std::vector<LevelObjectFile::TemplateLine> LevelObjectFile::templateLines(const std::string& name) const
{
    std::vector<TemplateLine> lines;
    const TemplateBlock* block = templateBlock(name);
    if (!block)
        return lines;
    if (block->isNew)
    {
        for (size_t i = 0; i < block->newLines.size(); ++i)
        {
            if (block->newLines[i].first.empty())
                continue;                               // a line written as it is, not a property
            TemplateLine line;
            line.property = block->newLines[i].first;
            line.value = block->newLines[i].second;
            line.key = templateKey(line.property, line.value);
            lines.push_back(line);
        }
        return lines;
    }
    std::map<std::string, std::map<std::string, std::pair<std::string, std::string> > >::const_iterator edits =
        m_templateEdits.find(lowered(name));
    std::map<std::string, bool> seen;
    for (size_t i = 0; i < block->propertyLines.size(); ++i)
    {
        TemplateLine line;
        splitTemplateLine(m_lines[block->propertyLines[i]].text, line.property, line.value);
        line.key = templateKey(line.property, line.value);
        seen[line.key] = true;
        if (edits != m_templateEdits.end())
        {
            std::map<std::string, std::pair<std::string, std::string> >::const_iterator edit = edits->second.find(line.key);
            if (edit != edits->second.end())
            {
                if (edit->second.second.empty())
                    continue;                                   // removed
                line.value = edit->second.second;
            }
        }
        lines.push_back(line);
    }
    if (edits != m_templateEdits.end())
    {
        for (std::map<std::string, std::pair<std::string, std::string> >::const_iterator it = edits->second.begin();
             it != edits->second.end(); ++it)
        {
            if (seen.count(it->first) || it->second.second.empty())
                continue;
            TemplateLine line;
            line.key = it->first;
            line.property = it->second.first;
            line.value = it->second.second;
            lines.push_back(line);
        }
    }
    return lines;
}

bool LevelObjectFile::setTemplateValue(const std::string& name, const std::string& property, const std::string& value)
{
    const TemplateBlock* block = templateBlock(name);
    if (!block || property.empty())
        return false;
    // Removing takes the line's key ("setobjecttemplate 2"); setting, the property and its value.
    const std::string key = value.empty() ? lowered(property) : templateKey(property, value);
    if (block->isNew)
    {
        // A new block holds its lines itself.
        TemplateBlock& owned = m_templates[m_templateByName.find(lowered(name))->second];
        for (size_t i = 0; i < owned.newLines.size(); ++i)
        {
            if (owned.newLines[i].first.empty() || templateKey(owned.newLines[i].first, owned.newLines[i].second) != key)
                continue;
            if (value.empty())
                owned.newLines.erase(owned.newLines.begin() + static_cast<std::ptrdiff_t>(i));
            else
                owned.newLines[i].second = value;
            return true;
        }
        if (!value.empty())
            owned.newLines.push_back(std::make_pair(property, value));
        return true;
    }
    // The value the file holds for this key: an edit back to it is no edit.
    std::string original;
    bool present = false;
    std::string written = property;
    for (size_t i = 0; i < block->propertyLines.size() && !present; ++i)
    {
        std::string p, v;
        splitTemplateLine(m_lines[block->propertyLines[i]].text, p, v);
        if (templateKey(p, v) == key)
        {
            present = true;
            original = v;
            written = p;
        }
    }
    std::map<std::string, std::pair<std::string, std::string> >& edits = m_templateEdits[lowered(name)];
    if ((present && value == original) || (!present && value.empty()))
        edits.erase(key);
    else
        edits[key] = std::make_pair(written, value);
    return true;
}

int LevelObjectFile::editedTemplates() const
{
    int count = 0;
    for (std::map<std::string, std::map<std::string, std::pair<std::string, std::string> > >::const_iterator it =
             m_templateEdits.begin(); it != m_templateEdits.end(); ++it)
    {
        const TemplateBlock* block = templateBlock(it->first);
        if (!it->second.empty() && block && !block->isNew && !block->deleted)
            ++count;
    }
    // New blocks written and old ones left out count as edits of their templates too, and so do renamed
    // blocks without other edits.
    for (size_t i = 0; i < m_templates.size(); ++i)
    {
        const TemplateBlock& block = m_templates[i];
        if (block.isNew != block.deleted)
            ++count;
        else if (!block.isNew && !block.deleted && block.name != block.fileName)
        {
            std::map<std::string, std::map<std::string, std::pair<std::string, std::string> > >::const_iterator edits =
                m_templateEdits.find(lowered(block.name));
            if (edits == m_templateEdits.end() || edits->second.empty())
                ++count;
        }
    }
    return count;
}

bool LevelObjectFile::appendTemplate(const std::string& className, const std::string& name,
                                     const std::vector<std::pair<std::string, std::string> >& lines, bool addTrigger)
{
    if (className.empty() || name.empty() || templateBlock(name))
        return false;
    TemplateBlock block;
    block.className = className;
    block.name = name;
    block.isNew = true;
    block.newLines = lines;
    block.addTrigger = addTrigger;
    m_templateByName[lowered(name)] = m_templates.size();
    m_templates.push_back(block);
    return true;
}

std::vector<std::string> LevelObjectFile::templateNamesOf(const std::string& className) const
{
    std::vector<std::string> names;
    for (size_t b = 0; b < m_templates.size(); ++b)
        if (!m_templates[b].deleted && lowered(m_templates[b].className) == lowered(className))
            names.push_back(m_templates[b].name);
    return names;
}

bool LevelObjectFile::templateHasTrigger(const std::string& name) const
{
    const TemplateBlock* block = templateBlock(name);
    return block && (block->isNew ? block->addTrigger : block->triggerLine != npos);
}

size_t LevelObjectFile::blockEnd(const TemplateBlock& block) const
{
    if (block.isNew || block.triggerLine == npos || block.triggerLine < block.lastLine)
        return block.lastLine;
    for (size_t j = block.lastLine + 1; j < block.triggerLine; ++j)
        if (!trimmed(m_lines[j].text).empty())
            return block.lastLine;                          // something stands between them
    return block.triggerLine;
}

bool LevelObjectFile::setTemplateDeleted(const std::string& name, bool deleted)
{
    std::map<std::string, size_t>::const_iterator found = m_templateByName.find(lowered(name));
    if (found == m_templateByName.end())
        return false;
    m_templates[found->second].deleted = deleted;
    return true;
}

bool LevelObjectFile::isTemplateDeleted(const std::string& name) const
{
    const TemplateBlock* block = templateBlock(name);
    return block && block->deleted;
}

namespace
{

// `text` with the first whole-name occurrence of `from` (case ignored) replaced by `to`. A name there is
// bounded by the line's ends, blanks, ':', '[' and ']' (the banners "rem [<class>: <name>]").
std::string replaceName(const std::string& text, const std::string& from, const std::string& to)
{
    if (from.empty())
        return text;
    const std::string low = lowered(text);
    const std::string lowFrom = lowered(from);
    for (size_t at = low.find(lowFrom); at != std::string::npos; at = low.find(lowFrom, at + 1))
    {
        const size_t end = at + from.size();
        const bool opens = at == 0 || std::strchr(" \t:[", text[at - 1]) != 0;
        const bool closes = end == text.size() || std::strchr(" \t]", text[end]) != 0;
        if (opens && closes)
            return text.substr(0, at) + to + text.substr(end);
    }
    return text;
}

} // namespace

bool LevelObjectFile::renameTemplate(const std::string& from, const std::string& to)
{
    std::map<std::string, size_t>::iterator found = m_templateByName.find(lowered(from));
    if (found == m_templateByName.end() || to.empty())
        return false;
    const size_t index = found->second;
    if (lowered(from) != lowered(to) && m_templateByName.count(lowered(to)))
        return false;
    TemplateBlock& block = m_templates[index];
    const std::string oldName = block.name;
    block.name = to;
    m_templateByName.erase(found);
    m_templateByName[lowered(to)] = index;
    std::map<std::string, std::map<std::string, std::pair<std::string, std::string> > >::iterator edits =
        m_templateEdits.find(lowered(oldName));
    if (edits != m_templateEdits.end() && lowered(oldName) != lowered(to))
    {
        m_templateEdits[lowered(to)].swap(edits->second);
        m_templateEdits.erase(lowered(oldName));
    }
    for (size_t r = 0; r < m_records.size(); ++r)
        if (lowered(m_records[r].current.templateName) == lowered(oldName))
            m_records[r].current.templateName = to;
    updateRenamedLines();
    return true;
}

size_t LevelObjectFile::renameObjectRecords(const std::string& from, const std::string& to)
{
    size_t count = 0;
    for (size_t r = 0; r < m_records.size(); ++r)
        if (lowered(m_records[r].current.templateName) == lowered(from))
        {
            m_records[r].current.templateName = to;
            ++count;
        }
    if (count)
        updateRenamedLines();
    return count;
}

void LevelObjectFile::updateRenamedLines()
{
    m_lineOverride.clear();
    const size_t last = m_lines.size();
    for (size_t b = 0; b < m_templates.size(); ++b)
    {
        const TemplateBlock& block = m_templates[b];
        if (block.isNew || block.name == block.fileName)
            continue;
        std::vector<size_t> lines;
        lines.push_back(block.createLine);
        if (block.createLine > 0 && lowered(trimmed(m_lines[block.createLine - 1].text)).compare(0, 5, "rem [") == 0)
            lines.push_back(block.createLine - 1);
        for (size_t j = block.createLine + 1; j <= block.lastLine && j < last; ++j)
        {
            std::string property, value;
            if (splitTemplateLine(m_lines[j].text, property, value) && lowered(property) == "activesafe")
                lines.push_back(j);
        }
        if (block.triggerLine != npos)
            lines.push_back(block.triggerLine);
        for (size_t k = 0; k < lines.size(); ++k)
        {
            const std::string text = replaceName(m_lines[lines[k]].text, block.fileName, block.name);
            if (text != m_lines[lines[k]].text)
                m_lineOverride[lines[k]] = text;
        }
    }
    for (size_t r = 0; r < m_records.size(); ++r)
    {
        const Record& record = m_records[r];
        if (record.isNew || record.current.templateName == record.loaded.templateName)
            continue;
        const size_t lines[2] = { record.createLine, record.firstLine };
        for (int k = 0; k < 2; ++k)
        {
            if (k == 1 && record.firstLine == record.createLine)
                continue;
            const std::string text =
                replaceName(m_lines[lines[k]].text, record.loaded.templateName, record.current.templateName);
            if (text != m_lines[lines[k]].text)
                m_lineOverride[lines[k]] = text;
        }
    }
}

const std::string& LevelObjectFile::lineText(size_t line) const
{
    std::map<size_t, std::string>::const_iterator found = m_lineOverride.find(line);
    return found == m_lineOverride.end() ? m_lines[line].text : found->second;
}

bool LevelObjectFile::lineOfTemplate(size_t line, const TemplateBlock*& block, TemplateLine& parsed) const
{
    // Only asked for lines inside a block with edits, so a scan of those blocks is enough.
    for (std::map<std::string, std::map<std::string, std::pair<std::string, std::string> > >::const_iterator it =
             m_templateEdits.begin(); it != m_templateEdits.end(); ++it)
    {
        if (it->second.empty())
            continue;
        const TemplateBlock* candidate = templateBlock(it->first);
        if (!candidate || line < candidate->createLine || line > candidate->lastLine)
            continue;
        block = candidate;
        if (!splitTemplateLine(m_lines[line].text, parsed.property, parsed.value))
            return false;
        parsed.key = templateKey(parsed.property, parsed.value);
        return true;
    }
    return false;
}

bool LevelObjectFile::slotDiffers(int slot, const ObjectPlacement& a, const ObjectPlacement& b) const
{
    switch (slot)
    {
    case SlotPosition:
        return formatPosition(a.position, m_style.positionDecimals) != formatPosition(b.position, m_style.positionDecimals);
    case SlotRotation:
        for (int i = 0; i < 3; ++i)
            if (anglesDiffer(a.rotation[i], b.rotation[i], m_style.rotationDecimals))
                return true;
        return false;
    case SlotControlPointId:
        return a.hasControlPointId != b.hasControlPointId || (b.hasControlPointId && a.controlPointId != b.controlPointId);
    case SlotLayer:
        return a.hasLayer != b.hasLayer || (b.hasLayer && a.layer != b.layer);
    case SlotGroup:
        return a.group != b.group;
    case SlotDisableChildren:
        return a.disableChildren != b.disableChildren;
    case SlotNotInAI:
        return a.notInAI != b.notInAI;
    case SlotIsOvergrowth:
        return a.isOvergrowth != b.isOvergrowth;
    case SlotLightSourceMask:
        return a.hasLightSourceMask != b.hasLightSourceMask || (b.hasLightSourceMask && a.lightSourceMask != b.lightSourceMask);
    default:
        return false;
    }
}

std::string LevelObjectFile::slotText(int slot, const ObjectPlacement& p, bool lineExists) const
{
    char text[64];
    switch (slot)
    {
    case SlotPosition:
        return "Object.absolutePosition " + formatPosition(p.position, m_style.positionDecimals);
    case SlotRotation:
    {
        // A zero rotation: a file that writes zeros keeps the record's rotation line, but no line is
        // added to a record that has none (the stock gameplay layers write no rotation for 750 of their
        // records - control points - while writing "0.000/0.000/0.000" for others).
        const bool zero = rotationPrintsAsZero(p.rotation, m_style.rotationDecimals);
        const bool shown = lineExists ? (m_style.keepZeroRotation || !zero) : !zero;
        return shown ? "Object.rotation " + formatRotation(p.rotation, m_style.rotationDecimals) : std::string();
    }
    case SlotControlPointId:
        if (!p.hasControlPointId)
            return std::string();
        std::snprintf(text, sizeof(text), "Object.setControlPointId %d", p.controlPointId);
        return text;
    case SlotLayer:
        if (!p.hasLayer)
            return std::string();
        std::snprintf(text, sizeof(text), "Object.layer %d", p.layer);
        return text;
    case SlotGroup:
        if (p.group == 0)
            return std::string();
        std::snprintf(text, sizeof(text), "Object.group %d", p.group);
        return text;
    case SlotDisableChildren:
        return p.disableChildren ? std::string("Object.disableChildren 1") : std::string();
    case SlotNotInAI:
        return p.notInAI ? std::string("Object.notInAI 1") : std::string();
    case SlotIsOvergrowth:
        return p.isOvergrowth ? std::string("Object.isOvergrowth 1") : std::string();
    case SlotLightSourceMask:
        if (!p.hasLightSourceMask)
            return std::string();
        std::snprintf(text, sizeof(text), "Object.setLightSourceMask %d", p.lightSourceMask);
        return text;
    default:
        return std::string();
    }
}

bool LevelObjectFile::placementDiffers(const Record& record) const
{
    for (int s = 0; s < SlotCount; ++s)
        if (slotDiffers(s, record.loaded, record.current))
            return true;
    return false;
}

bool LevelObjectFile::isEdited(size_t record) const
{
    return !m_records[record].isNew && placementDiffers(m_records[record]);
}

bool LevelObjectFile::hasChanges() const
{
    if (hasPreloadBlock() && !m_extraPreload.empty())
        return true;
    if (!m_lineOverride.empty())
        return true;
    if (editedTemplates() > 0 || editedCombatAreas() > 0 || combatAreaSettingsChanged())
        return true;
    for (size_t i = 0; i < m_records.size(); ++i)
    {
        const Record& record = m_records[i];
        if (record.isNew ? !record.deleted : (record.deleted || placementDiffers(record)))
            return true;
    }
    return false;
}

void LevelObjectFile::setPlacement(size_t record, const ObjectPlacement& placement)
{
    Record& target = m_records[record];
    const std::string templateName = target.current.templateName;
    target.current = placement;
    target.current.templateName = templateName;
}

size_t LevelObjectFile::appendRecord(const ObjectPlacement& placement)
{
    Record record;
    record.firstLine = record.createLine = record.endLine = npos;
    for (int s = 0; s < SlotCount; ++s)
        record.slotLine[s] = npos;
    record.loaded = placement;
    record.current = placement;
    record.isNew = true;
    record.deleted = false;
    m_records.push_back(record);
    return m_records.size() - 1;
}

void LevelObjectFile::setDeleted(size_t record, bool deleted)
{
    m_records[record].deleted = deleted;
}

void LevelObjectFile::appendLine(std::string& out, const std::string& text, bool crlf) const
{
    out += text;
    out += crlf ? "\r\n" : "\n";
}

void LevelObjectFile::renderRecord(const Record& record, std::string& out) const
{
    const ObjectPlacement& p = record.current;
    const bool crlf = m_lines[record.createLine].crlf;
    const std::string indent = indentOf(m_lines[record.createLine].text);

    // What each slot prints as now ("" to leave it out), and where a missing one goes: after the last
    // line (in the file) of the slots before it in the retail order, or after the create line - so a
    // record whose lines stand in another order never gets a line ahead of one that precedes it.
    std::string text[SlotCount];
    size_t after[SlotCount];
    size_t latest = record.createLine;
    for (int s = 0; s < SlotCount; ++s)
    {
        text[s] = slotText(s, p, record.slotLine[s] != npos);
        after[s] = latest;
        if (record.slotLine[s] != npos && record.slotLine[s] > latest)
            latest = record.slotLine[s];
    }
    for (size_t j = record.firstLine; j < record.endLine; ++j)
    {
        const Line& line = m_lines[j];
        int slot = -1;
        for (int s = 0; s < SlotCount && slot < 0; ++s)
            if (record.slotLine[s] == j)
                slot = s;
        if (slot < 0 || !slotDiffers(slot, record.loaded, p))
            appendLine(out, lineText(j), line.crlf);                // not the editor's, or unchanged
        else if (!text[slot].empty())
            appendLine(out, indentOf(line.text) + text[slot], line.crlf);
        // else: the value changed to one this file does not write, and the line goes

        // A line the record lacks is added only for a value that changed.
        for (int s = 0; s < SlotCount; ++s)
            if (record.slotLine[s] == npos && after[s] == j && !text[s].empty()
                && slotDiffers(s, record.loaded, p))
                appendLine(out, indent + text[s], crlf);
    }
}

std::string LevelObjectFile::render(std::vector<size_t>* order) const
{
    std::string out;
    if (order)
        order->clear();
    const std::string newline = m_crlf ? "\r\n" : "\n";
    // Which records start where.
    std::map<size_t, size_t> startsAt;
    for (size_t r = 0; r < m_records.size(); ++r)
        if (!m_records[r].isNew)
            startsAt[m_records[r].firstLine] = r;

    const size_t last = m_lines.size();
    // Deleted template blocks: first line (the banner when there is one) -> the line after the block.
    std::map<size_t, size_t> droppedBlocks;
    for (size_t b = 0; b < m_templates.size(); ++b)
    {
        const TemplateBlock& block = m_templates[b];
        if (block.isNew || !block.deleted)
            continue;
        size_t first = block.createLine;
        if (first > 0 && lowered(trimmed(m_lines[first - 1].text)).compare(0, 5, "rem [") == 0)
            --first;
        size_t end = blockEnd(block) + 1;
        if (end < last && trimmed(m_lines[end].text).empty())
            ++end;
        droppedBlocks[first] = end;
        if (block.triggerLine != npos && blockEnd(block) != block.triggerLine)
            droppedBlocks[block.triggerLine] = block.triggerLine + 1;
    }

    // New templates and records: into the sections of their class where the file has them, else at
    // the end (see the header).
    std::vector<Insertion> inserts;
    std::vector<Insertion> tail;
    planNewBlocks(inserts, tail);

    // Combat areas: where each existing block starts, and where new ones go - after the last block, or
    // at the end with the manager lines the file lacks.
    std::map<size_t, size_t> areaStartsAt;
    size_t lastArea = npos;
    std::map<size_t, std::string> pendingSettings;      // manager lines to write before a line
    for (size_t a = 0; a < m_areas.size(); ++a)
    {
        if (m_areas[a].isNew)
            continue;
        areaStartsAt[m_areas[a].createLine] = a;
        if (lastArea == npos || m_areas[lastArea].createLine < m_areas[a].createLine)
            lastArea = a;
    }
    {
        Insertion areas;
        areas.at = lastArea == npos ? npos : lineAfter(m_areas[lastArea].endLine - 1);
        for (size_t a = 0; a < m_areas.size(); ++a)
            if (m_areas[a].isNew && !m_areas[a].deleted)
                areas.text += formatCombatArea(m_areas[a].current, newline) + newline;
        // Manager lines the file lacks: before the first manager line that follows them in the stock
        // order, else after the last that precedes them, else before the first block, else at the end.
        std::string settingsText;
        for (int s = 0; s < 3; ++s)
        {
            const std::string line = settingLine(s, m_areaSettings);
            if (m_settingLine[s] != npos || line.empty())
                continue;
            size_t before = npos;
            for (int k = s + 1; k < 3 && before == npos; ++k)
                before = m_settingLine[k];
            size_t after = npos;
            for (int k = s - 1; k >= 0 && after == npos; --k)
                after = m_settingLine[k];
            const size_t at = before != npos ? before
                            : after != npos ? after + 1
                            : lastArea != npos ? m_areas[areaStartsAt.begin()->second].createLine : npos;
            if (at == npos)
                settingsText += line + newline;
            else
                pendingSettings[at] += line + newline;
        }
        if (!settingsText.empty() || (!areas.text.empty() && areas.at == npos))
        {
            areas.text = settingsText + areas.text;
            areas.at = npos;
            if (!areas.text.empty())
                tail.push_back(areas);
        }
        else if (!areas.text.empty())
        {
            inserts.push_back(areas);
            std::stable_sort(inserts.begin(), inserts.end(), insertionBefore);
        }
    }
    size_t next = 0;

    for (size_t i = 0; i < last;)
    {
        while (next < inserts.size() && inserts[next].at <= i)
            writeInsertion(inserts[next++], out, order);
        std::map<size_t, std::string>::iterator pending = pendingSettings.find(i);
        if (pending != pendingSettings.end())
        {
            out += pending->second;
            pendingSettings.erase(pending);
        }
        // A combat area block: left out, written again, or kept below line by line.
        std::map<size_t, size_t>::const_iterator area = areaStartsAt.find(i);
        if (area != areaStartsAt.end())
        {
            const AreaBlock& block = m_areas[area->second];
            if (block.deleted)
            {
                i = block.endLine;
                if (i < last && trimmed(m_lines[i].text).empty())
                    ++i;
                continue;
            }
            if (block.writable && areaChanged(block))
            {
                const std::string indent = indentOf(m_lines[i].text);
                const std::string blockNewline = m_lines[i].crlf ? "\r\n" : "\n";
                const std::string text = formatCombatArea(block.current, blockNewline);
                for (size_t at = 0; at < text.size();)
                {
                    const size_t end = text.find('\n', at);
                    out += indent + text.substr(at, end + 1 - at);
                    at = end + 1;
                }
                i = block.endLine;
                if (i == last && !m_finalNewline)
                    out.erase(out.size() - blockNewline.size());
                continue;
            }
        }
        // A manager line whose value changed: written again, or left out when it is no longer set.
        bool settingDone = false;
        for (int s = 0; s < 3 && !settingDone; ++s)
        {
            if (m_settingLine[s] != i)
                continue;
            const std::string now = settingLine(s, m_areaSettings);
            if (now == settingLine(s, m_loadedAreaSettings))
                break;
            if (!now.empty())
            {
                out += indentOf(m_lines[i].text) + now;
                if (i + 1 < last || m_finalNewline)
                    out += m_lines[i].crlf ? "\r\n" : "\n";
            }
            settingDone = true;
        }
        if (settingDone)
        {
            ++i;
            continue;
        }
        if (i == m_preloadEnd)
            for (size_t k = 0; k < m_extraPreload.size(); ++k)
                appendLine(out, m_extraPreload[k], m_lines[i].crlf);
        std::map<size_t, size_t>::const_iterator found = startsAt.find(i);
        if (found != startsAt.end())
        {
            const Record& record = m_records[found->second];
            if (record.deleted)
            {
                i = record.endLine;
                if (i < last && trimmed(m_lines[i].text).empty())
                    ++i;                                    // its blank separator goes with it
                continue;
            }
            if (order)
                order->push_back(found->second);
            if (placementDiffers(record))
            {
                renderRecord(record, out);
                i = record.endLine;
                // The file's last line keeps its missing final newline.
                if (i == last && !m_finalNewline)
                    out.erase(out.size() - (m_lines[i - 1].crlf ? 2 : 1));
                continue;
            }
        }
        // A template block left out: its banner, its lines and one blank line after it.
        std::map<size_t, size_t>::const_iterator dropped = droppedBlocks.find(i);
        if (dropped != droppedBlocks.end())
        {
            i = dropped->second;
            continue;
        }
        const Line& line = m_lines[i];
        const TemplateBlock* block = 0;
        TemplateLine parsed;
        if (!m_templateEdits.empty() && lineOfTemplate(i, block, parsed))
        {
            const std::map<std::string, std::pair<std::string, std::string> >& edits =
                m_templateEdits.find(lowered(block->name))->second;
            std::map<std::string, std::pair<std::string, std::string> >::const_iterator edit = edits.find(parsed.key);
            const bool lineEnds = i + 1 < last || m_finalNewline;
            if (edit != edits.end() && parsed.property.size() && lowered(parsed.property) != "create"
                && lowered(parsed.property) != "activesafe")
            {
                // The edited line, at its own indentation; an empty value drops it.
                if (!edit->second.second.empty())
                {
                    out += indentOf(line.text) + "ObjectTemplate." + parsed.property + " " + edit->second.second;
                    if (lineEnds)
                        out += line.crlf ? "\r\n" : "\n";
                }
            }
            else
            {
                out += lineText(i);
                if (lineEnds)
                    out += line.crlf ? "\r\n" : "\n";
            }
            // New lines of the block follow its last one.
            if (i == block->lastLine)
            {
                std::map<std::string, bool> present;
                for (size_t k = 0; k < block->propertyLines.size(); ++k)
                {
                    std::string p, v;
                    splitTemplateLine(m_lines[block->propertyLines[k]].text, p, v);
                    present[templateKey(p, v)] = true;
                }
                for (std::map<std::string, std::pair<std::string, std::string> >::const_iterator it = edits.begin();
                     it != edits.end(); ++it)
                    if (!present.count(it->first) && !it->second.second.empty())
                        appendLine(out, indentOf(m_lines[block->createLine].text) + "ObjectTemplate." + it->second.first
                                            + " " + it->second.second, line.crlf);
            }
            ++i;
            continue;
        }
        out += lineText(i);
        if (i + 1 < last || m_finalNewline)
            out += line.crlf ? "\r\n" : "\n";
        ++i;
    }

    // What goes at the end: the sections that close the file first, then the blocks of classes the
    // file had none of.
    if (next == inserts.size() && tail.empty())
        return out;
    if (!out.empty() && out[out.size() - 1] != '\n')
        out += newline;
    while (next < inserts.size())
        writeInsertion(inserts[next++], out, order);
    for (size_t k = 0; k < tail.size(); ++k)
        writeInsertion(tail[k], out, order);
    return out;
}

namespace
{

// The last line written is empty (or nothing is written yet).
bool endsWithBlankLine(const std::string& out)
{
    size_t n = out.size();
    if (n == 0)
        return true;
    if (out[n - 1] != '\n')
        return false;
    --n;
    if (n > 0 && out[n - 1] == '\r')
        --n;
    return n == 0 || out[n - 1] == '\n';
}

} // namespace

void LevelObjectFile::writeInsertion(const Insertion& insertion, std::string& out, std::vector<size_t>* order) const
{
    // Each block is written with the blank line after it, so one before it is needed only where the
    // file has none.
    if (!endsWithBlankLine(out))
        out += m_crlf ? "\r\n" : "\n";
    out += insertion.text;
    if (order)
        order->insert(order->end(), insertion.records.begin(), insertion.records.end());
}

size_t LevelObjectFile::lineAfter(size_t line) const
{
    // The line after `line`, past one blank separator.
    size_t next = line + 1;
    if (next < m_lines.size() && trimmed(m_lines[next].text).empty())
        ++next;
    return next;
}

void LevelObjectFile::planNewBlocks(std::vector<Insertion>& inserts, std::vector<Insertion>& tail) const
{
    // Where each class's blocks are in the file: its last template block and its last record, by line.
    struct Layout
    {
        size_t lastBlock;       // index into m_templates; npos when none
        size_t lastRecord;      // index into m_records
        bool   pairs;           // each record right after its own template (the spawn points)
        Layout() : lastBlock(npos), lastRecord(npos), pairs(false) {}
    };
    std::map<std::string, Layout> layout;
    for (size_t b = 0; b < m_templates.size(); ++b)
    {
        const TemplateBlock& block = m_templates[b];
        if (block.isNew)
            continue;
        Layout& l = layout[lowered(block.className)];
        if (l.lastBlock == npos || m_templates[l.lastBlock].createLine < block.createLine)
            l.lastBlock = b;
    }
    for (size_t r = 0; r < m_records.size(); ++r)
    {
        const Record& record = m_records[r];
        const TemplateBlock* block = record.isNew ? 0 : templateBlock(record.loaded.templateName);
        if (!block || block->isNew)
            continue;
        Layout& l = layout[lowered(block->className)];
        if (l.lastRecord == npos || m_records[l.lastRecord].createLine < record.createLine)
            l.lastRecord = r;
    }
    // Pairs: every template block of the class is followed by its own object's record (the first
    // statement after it creates that template), and the last record comes after the last block.
    std::map<std::string, bool> followedByOwn;
    for (size_t b = 0; b < m_templates.size(); ++b)
    {
        const TemplateBlock& block = m_templates[b];
        if (block.isNew)
            continue;
        bool own = false;
        for (size_t j = blockEnd(block) + 1; j < m_lines.size(); ++j)
        {
            const std::string text = lowered(trimmed(m_lines[j].text));
            if (text.empty() || text.compare(0, 3, "rem") == 0)
                continue;
            std::string rest;
            own = startsWithCommand(m_lines[j].text, "Object.create", rest)
               && lowered(trimmed(rest.substr(0, rest.find_first_of(" \t")))) == lowered(block.name);
            break;
        }
        const std::string key = lowered(block.className);
        followedByOwn[key] = (followedByOwn.count(key) ? followedByOwn[key] : true) && own;
    }
    for (std::map<std::string, Layout>::iterator it = layout.begin(); it != layout.end(); ++it)
    {
        Layout& l = it->second;
        l.pairs = l.lastBlock != npos && l.lastRecord != npos && followedByOwn[it->first]
               && m_records[l.lastRecord].createLine > m_templates[l.lastBlock].lastLine;
    }
    const std::string newline = m_crlf ? "\r\n" : "\n";

    // New templates, each where its class's templates end (or, for pairs, after the class's last
    // record, with its own new records after it); in the order they were made.
    std::vector<bool> placed(m_records.size(), false);
    std::map<std::string, size_t> templateAnchor;      // lower-case template name -> its line (npos = end)
    Insertion tailTemplates;
    tailTemplates.at = npos;
    for (size_t b = 0; b < m_templates.size(); ++b)
    {
        const TemplateBlock& block = m_templates[b];
        if (!block.isNew || block.deleted)
            continue;
        std::map<std::string, Layout>::const_iterator l = layout.find(lowered(block.className));
        size_t at = npos;
        if (l != layout.end() && l->second.pairs)
            at = m_records[l->second.lastRecord].endLine - 1;
        else if (l != layout.end() && l->second.lastBlock != npos)
            at = blockEnd(m_templates[l->second.lastBlock]);
        Insertion insertion;
        insertion.at = at == npos ? npos : lineAfter(at);
        // The banner as the class's last block has it.
        std::string bannerPrefix;
        if (l != layout.end() && l->second.lastBlock != npos && m_templates[l->second.lastBlock].createLine > 0)
        {
            const std::string previous = trimmed(m_lines[m_templates[l->second.lastBlock].createLine - 1].text);
            const size_t colon = previous.find(':');
            if (lowered(previous).compare(0, 5, "rem [") == 0 && colon != std::string::npos)
                bannerPrefix = previous.substr(0, colon);
        }
        renderNewTemplate(block, insertion.text, bannerPrefix);
        if (l != layout.end() && l->second.pairs)
        {
            const Record& neighbour = m_records[l->second.lastRecord];
            const std::string indent = indentOf(m_lines[neighbour.createLine].text);
            for (size_t r = 0; r < m_records.size(); ++r)
            {
                const Record& record = m_records[r];
                if (record.isNew && !record.deleted && lowered(record.current.templateName) == lowered(block.name))
                {
                    renderNewRecord(record, indent, insertion.text, neighbour.firstLine != neighbour.createLine);
                    insertion.records.push_back(r);
                    placed[r] = true;
                }
            }
        }
        templateAnchor[lowered(block.name)] = insertion.at;
        if (insertion.at == npos)
        {
            tailTemplates.text += insertion.text;
            tailTemplates.records.insert(tailTemplates.records.end(), insertion.records.begin(), insertion.records.end());
        }
        else
            inserts.push_back(insertion);
    }

    // New records: after their class's last record where the file has one (inside its host block, at
    // its indentation); else at the end - inside a new `if v_arg1 == host` block for host-only ones.
    Insertion tailRecords, tailHost;
    tailRecords.at = tailHost.at = npos;
    for (size_t r = 0; r < m_records.size(); ++r)
    {
        const Record& record = m_records[r];
        if (!record.isNew || record.deleted || placed[r])
            continue;
        const TemplateBlock* block = templateBlock(record.current.templateName);
        std::map<std::string, Layout>::const_iterator l =
            block ? layout.find(lowered(block->className)) : layout.end();
        size_t at = npos;
        if (l != layout.end() && l->second.lastRecord != npos)
        {
            at = lineAfter(m_records[l->second.lastRecord].endLine - 1);
            // Never before its own template.
            std::map<std::string, size_t>::const_iterator own = templateAnchor.find(lowered(block->name));
            if (own != templateAnchor.end() && (own->second == npos || own->second > at))
                at = npos;
        }
        if (at != npos)
        {
            Insertion insertion;
            insertion.at = at;
            const Record& neighbour = m_records[l->second.lastRecord];
            renderNewRecord(record, indentOf(m_lines[neighbour.createLine].text), insertion.text,
                            neighbour.firstLine != neighbour.createLine);
            insertion.records.push_back(r);
            inserts.push_back(insertion);
        }
        else if (record.current.hostOnly)
        {
            renderNewRecord(record, "   ", tailHost.text);
            tailHost.records.push_back(r);
        }
        else
        {
            renderNewRecord(record, std::string(), tailRecords.text);
            tailRecords.records.push_back(r);
        }
    }
    // Stable: blocks for one line keep the order they were made in.
    std::stable_sort(inserts.begin(), inserts.end(), insertionBefore);
    if (!tailTemplates.text.empty())
        tail.push_back(tailTemplates);
    if (!tailRecords.text.empty())
        tail.push_back(tailRecords);
    if (!tailHost.text.empty())
    {
        tailHost.text = "if v_arg1 == host" + newline + newline + tailHost.text + "endIf" + newline + newline;
        tail.push_back(tailHost);
    }
}

bool LevelObjectFile::insertionBefore(const Insertion& a, const Insertion& b)
{
    return a.at < b.at;
}

void LevelObjectFile::renderNewTemplate(const TemplateBlock& block, std::string& out, const std::string& bannerPrefix) const
{
    appendLine(out, (bannerPrefix.empty() ? "rem [" + block.className + "Template" : bannerPrefix) + ": " + block.name + "]",
               m_crlf);
    appendLine(out, "ObjectTemplate.create " + block.className + " " + block.name, m_crlf);
    appendLine(out, "ObjectTemplate.activeSafe " + block.className + " " + block.name, m_crlf);
    for (size_t i = 0; i < block.newLines.size(); ++i)
    {
        // A line with no property is written as it is (the "rem -----" separators of a control point).
        if (block.newLines[i].first.empty())
            appendLine(out, block.newLines[i].second, m_crlf);
        else
            appendLine(out, "ObjectTemplate." + block.newLines[i].first + " " + block.newLines[i].second, m_crlf);
    }
    out += m_crlf ? "\r\n" : "\n";
    if (block.addTrigger)
    {
        appendLine(out, "Sound.addTrigger " + block.name, m_crlf);
        out += m_crlf ? "\r\n" : "\n";
    }
}

void LevelObjectFile::renderNewRecord(const Record& record, const std::string& indent, std::string& out, bool banner) const
{
    const ObjectPlacement& p = record.current;
    // The banner the file's own records of that class have: "rem [<class>: <name>]" where the file
    // defines the template, else StaticObjects.con's "rem * <template> *"; none where they have none
    // (the ambient sounds' records).
    const TemplateBlock* block = templateBlock(p.templateName);
    if (banner)
        appendLine(out, indent + (block ? "rem [" + block->className + ": " + p.templateName + "]"
                                        : "rem *** " + p.templateName + " ***"), m_crlf);
    appendLine(out, indent + "Object.create " + p.templateName, m_crlf);
    for (int s = SlotPosition; s < SlotCount; ++s)
    {
        std::string text;
        if (s == SlotRotation)
        {
            const bool zero = rotationPrintsAsZero(p.rotation, m_style.rotationDecimals);
            if (!zero || (m_style.keepZeroRotation && !p.omitZeroRotation))
                text = "Object.rotation " + formatRotation(p.rotation, m_style.rotationDecimals);
        }
        else
            text = slotText(s, p, false);
        if (!text.empty())
            appendLine(out, indent + text, m_crlf);
    }
    out += m_crlf ? "\r\n" : "\n";
}

int LevelObjectFile::dominantLayer() const
{
    std::map<int, int> counts;
    for (size_t r = 0; r < m_records.size(); ++r)
        if (!m_records[r].isNew && m_records[r].loaded.hasLayer)
            ++counts[m_records[r].loaded.layer];
    int best = -1, most = 0;
    for (std::map<int, int>::const_iterator it = counts.begin(); it != counts.end(); ++it)
        if (it->second > most)
        {
            best = it->first;
            most = it->second;
        }
    return best;
}

bool LevelObjectFile::findCombatArea(const std::string& name, size_t& area) const
{
    for (size_t i = 0; i < m_areas.size(); ++i)
    {
        if (lowered(m_areas[i].current.name) == lowered(name))
        {
            area = i;
            return true;
        }
    }
    return false;
}

bool LevelObjectFile::setCombatArea(size_t area, const CombatAreaData& data)
{
    AreaBlock& block = m_areas[area];
    if (!block.writable)
        return false;
    const std::string name = block.current.name;
    block.current = data;
    block.current.name = name;
    return true;
}

size_t LevelObjectFile::appendCombatArea(const CombatAreaData& data)
{
    AreaBlock block;
    block.createLine = block.endLine = npos;
    block.loaded = data;
    block.current = data;
    block.isNew = true;
    block.deleted = false;
    block.writable = true;
    m_areas.push_back(block);
    return m_areas.size() - 1;
}

void LevelObjectFile::setCombatAreaDeleted(size_t area, bool deleted)
{
    m_areas[area].deleted = deleted;
}

std::string LevelObjectFile::formatCombatArea(const CombatAreaData& data, const std::string& newline)
{
    char line[160];
    std::string out = "CombatArea.create " + data.name + newline;
    std::snprintf(line, sizeof(line), "CombatArea.min %f/%f", data.minimum[0], data.minimum[1]);
    out += line + newline;
    std::snprintf(line, sizeof(line), "CombatArea.max %f/%f", data.maximum[0], data.maximum[1]);
    out += line + newline;
    for (size_t i = 0; i < data.points.size(); ++i)
    {
        std::snprintf(line, sizeof(line), "CombatArea.addAreaPoint %f/%f", data.points[i].first, data.points[i].second);
        out += line + newline;
    }
    std::snprintf(line, sizeof(line), "CombatArea.team %d", data.team);
    out += line + newline;
    std::snprintf(line, sizeof(line), "CombatArea.vehicles %d", data.vehicles);
    out += line + newline;
    std::snprintf(line, sizeof(line), "CombatArea.layer %d", data.layer);
    out += line + newline;
    if (data.usedByPathFinding)
        out += "CombatArea.usedByPathFinding 1" + newline;
    if (data.inverted)
        out += "CombatArea.inverted 1" + newline;
    return out;
}

bool LevelObjectFile::areaChanged(const AreaBlock& block) const
{
    return formatCombatArea(block.loaded, "\n") != formatCombatArea(block.current, "\n");
}

int LevelObjectFile::editedCombatAreas() const
{
    int count = 0;
    for (size_t i = 0; i < m_areas.size(); ++i)
    {
        const AreaBlock& block = m_areas[i];
        if (block.isNew ? !block.deleted : (block.deleted || areaChanged(block)))
            ++count;
    }
    return count;
}

std::string LevelObjectFile::settingLine(int setting, const CombatAreaSettings& s) const
{
    char line[96];
    switch (setting)
    {
    case 0:
        if (!s.hasUse)
            return std::string();
        std::snprintf(line, sizeof(line), "CombatAreaManager.use %d", s.use ? 1 : 0);
        return line;
    case 1:
        if (!s.hasDamage)
            return std::string();
        std::snprintf(line, sizeof(line), "CombatAreaManager.damage %f", s.damage);
        return line;
    default:
        if (!s.hasTimeAllowedOutside)
            return std::string();
        std::snprintf(line, sizeof(line), "CombatAreaManager.timeAllowedOutside %f", s.timeAllowedOutside);
        return line;
    }
}

bool LevelObjectFile::combatAreaSettingsChanged() const
{
    for (int s = 0; s < 3; ++s)
        if (settingLine(s, m_loadedAreaSettings) != settingLine(s, m_areaSettings))
            return true;
    return false;
}

std::vector<unsigned> bindRecords(const LevelObjectFile& file, const std::vector<SceneObject>& objects,
                                  float tolerance)
{
    std::map<std::string, std::vector<size_t> > byTemplate;
    for (size_t i = 0; i < objects.size(); ++i)
        byTemplate[lowered(objects[i].templateName)].push_back(i);
    std::vector<bool> used(objects.size(), false);
    std::vector<unsigned> ids(file.recordCount(), 0);
    const float limit = tolerance * tolerance;
    for (size_t r = 0; r < file.recordCount(); ++r)
    {
        if (file.isNew(r) || file.isDeleted(r))
            continue;
        const ObjectPlacement& p = file.loadedPlacement(r);
        std::map<std::string, std::vector<size_t> >::const_iterator candidates = byTemplate.find(lowered(p.templateName));
        if (candidates == byTemplate.end())
            continue;
        size_t best = objects.size();
        float bestDistance = limit;
        for (size_t c = 0; c < candidates->second.size(); ++c)
        {
            const size_t index = candidates->second[c];
            if (used[index])
                continue;
            float distance = 0.0f;
            for (int k = 0; k < 3; ++k)
            {
                const float d = objects[index].position[k] - p.position[k];
                distance += d * d;
            }
            if (distance <= bestDistance)
            {
                bestDistance = distance;
                best = index;
            }
        }
        if (best < objects.size())
        {
            used[best] = true;
            ids[r] = objects[best].id;
        }
    }
    return ids;
}

} // namespace bf2editor
