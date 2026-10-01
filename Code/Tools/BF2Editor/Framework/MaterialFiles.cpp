// Code/Tools/BF2Editor/Framework/MaterialFiles.cpp - see MaterialFiles.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/MaterialFiles.h"

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

bool sameText(const std::string& a, const std::string& b)
{
    return _stricmp(a.c_str(), b.c_str()) == 0;
}

bool startsWithText(const std::string& text, const char* head)
{
    const size_t n = std::strlen(head);
    return text.size() >= n && _strnicmp(text.c_str(), head, n) == 0;
}

std::string trimmed(const std::string& text)
{
    size_t begin = 0;
    size_t end = text.size();
    while (begin < end && isBlank(text[begin]))
        ++begin;
    while (end > begin && isBlank(text[end - 1]))
        --end;
    return text.substr(begin, end - begin);
}

bool parseInt(const std::string& text, int& out)
{
    if (text.empty())
        return false;
    char* end = 0;
    const long value = std::strtol(text.c_str(), &end, 10);
    if (!end || *end != 0)
        return false;
    out = static_cast<int>(value);
    return true;
}

bool parseFloat(const std::string& text, float& out)
{
    if (text.empty())
        return false;
    char* end = 0;
    const float value = std::strtof(text.c_str(), &end);
    if (!end || *end != 0)
        return false;
    out = value;
    return true;
}

const char kMaterialPrefix[] = "Material.";
const char kCellPrefix[] = "MaterialManager.";

// The define file's properties: name, its kind, its default (the text the writer leaves the line out for),
// whether the writer always writes it.
enum class PropertyKind { Name, Integer, Number, Flag };
struct PropertyRow
{
    const char*  name;
    PropertyKind kind;
    const char*  defaultText;
    bool         always;
};
const PropertyRow kProperties[] = {
    { "name", PropertyKind::Name, "", true },
    { "type", PropertyKind::Integer, "1", true },
    { "friction", PropertyKind::Number, "1", true },
    { "elasticity", PropertyKind::Number, "0", true },
    { "resistance", PropertyKind::Number, "0.01", true },
    { "projectileCollisionHardness", PropertyKind::Number, "0", false },
    { "damageLoss", PropertyKind::Number, "-1", false },
    { "minDamageLoss", PropertyKind::Number, "0", false },
    { "maxDamageLoss", PropertyKind::Number, "100000", false },
    { "penetrationDeviation", PropertyKind::Number, "0.1", false },
    { "overrideNeverPenetrate", PropertyKind::Flag, "0", false },
    { "hasWaterPhysics", PropertyKind::Flag, "0", false },
    { "isBarbwire", PropertyKind::Flag, "0", false },
    { "isOneSided", PropertyKind::Flag, "0", false },
    { "isSeeThrough", PropertyKind::Flag, "0", false },
};
const size_t kPropertyCount = sizeof(kProperties) / sizeof(kProperties[0]);

int propertyIndex(const std::string& property)
{
    for (size_t i = 0; i < kPropertyCount; ++i)
        if (sameText(property, kProperties[i].name))
            return static_cast<int>(i);
    return -1;
}

// A define-file line's property ("Material.friction 0.8" -> friction); empty for any other line.
std::string definePropertyOf(const std::string& text, std::string& value)
{
    const std::string line = trimmed(text);
    if (!startsWithText(line, kMaterialPrefix))
        return std::string();
    size_t end = 0;
    while (end < line.size() && !isBlank(line[end]))
        ++end;
    value = trimmed(line.substr(end));
    return line.substr(sizeof(kMaterialPrefix) - 1, end - (sizeof(kMaterialPrefix) - 1));
}

std::string unquoted(const std::string& text)
{
    if (text.size() >= 2 && text[0] == '"' && text[text.size() - 1] == '"')
        return text.substr(1, text.size() - 2);
    return text;
}

} // namespace

std::string materialNumber(float value)
{
    char text[64];
    std::snprintf(text, sizeof(text), "%g", static_cast<double>(value));
    return text;
}

// MaterialTextLines

MaterialTextLines::MaterialTextLines()
    : m_dirty(true)
{
}

void MaterialTextLines::parse(const std::string& text)
{
    m_lines.clear();
    m_original = text;
    m_dirty = true;
    size_t start = 0;
    while (start < text.size())
    {
        const size_t newlineAt = text.find('\n', start);
        Line line;
        if (newlineAt == std::string::npos)
        {
            line.text = text.substr(start);
            start = text.size();
        }
        else
        {
            const bool crlf = newlineAt > start && text[newlineAt - 1] == '\r';
            line.text = text.substr(start, newlineAt - start - (crlf ? 1 : 0));
            line.end = crlf ? "\r\n" : "\n";
            start = newlineAt + 1;
        }
        m_lines.push_back(line);
    }
}

std::string MaterialTextLines::render() const
{
    std::string text;
    for (size_t i = 0; i < m_lines.size(); ++i)
        text += m_lines[i].text + m_lines[i].end;
    return text;
}

std::vector<std::string> MaterialTextLines::wordsOf(const std::string& text)
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

std::string MaterialTextLines::restAfter(const std::string& text, size_t count)
{
    size_t i = 0;
    for (size_t w = 0; w < count; ++w)
    {
        while (i < text.size() && isBlank(text[i]))
            ++i;
        while (i < text.size() && !isBlank(text[i]))
            ++i;
    }
    return trimmed(text.substr(i < text.size() ? i : text.size()));
}

std::string MaterialTextLines::newline() const
{
    for (size_t i = 0; i < m_lines.size(); ++i)
        if (!m_lines[i].end.empty())
            return m_lines[i].end;
    return "\r\n";
}

void MaterialTextLines::insert(size_t at, const std::string& text)
{
    m_dirty = true;
    Line line;
    line.text = text;
    line.end = newline();
    if (at >= m_lines.size())
    {
        if (!m_lines.empty() && m_lines.back().end.empty())
            m_lines.back().end = line.end;      // the old last line needs one now
        m_lines.push_back(line);
        return;
    }
    m_lines.insert(m_lines.begin() + static_cast<std::ptrdiff_t>(at), line);
}

void MaterialTextLines::erase(size_t at)
{
    if (at >= m_lines.size())
        return;
    m_dirty = true;
    // The last line keeps the file's final line end (or its lack of one).
    if (at == m_lines.size() - 1 && at > 0)
        m_lines[at - 1].end = m_lines[at].end;
    m_lines.erase(m_lines.begin() + static_cast<std::ptrdiff_t>(at));
}

// MaterialDefineFile

const std::vector<std::string>& MaterialDefineFile::properties()
{
    static std::vector<std::string> names;
    if (names.empty())
        for (size_t i = 0; i < kPropertyCount; ++i)
            names.push_back(kProperties[i].name);
    return names;
}

bool MaterialDefineFile::isProperty(const std::string& property)
{
    return propertyIndex(property) >= 0;
}

void MaterialDefineFile::index()
{
    m_blocks.clear();
    m_byId.clear();
    m_dirty = false;
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        const std::vector<std::string> words = wordsOf(m_lines[i].text);
        int id = 0;
        if (words.size() >= 2 && sameText(words[0], "Material.active") && parseInt(words[1], id))
        {
            if (!m_blocks.empty())
                m_blocks.back().end = i;
            Block block;
            block.id = id;
            block.first = i;
            block.end = m_lines.size();
            if (!m_byId.count(id))
                m_byId[id] = m_blocks.size();
            m_blocks.push_back(block);
        }
    }
}

int MaterialDefineFile::blockOf(int id) const
{
    ensureIndexed();
    const std::map<int, size_t>::const_iterator found = m_byId.find(id);
    return found == m_byId.end() ? -1 : static_cast<int>(found->second);
}

size_t MaterialDefineFile::lineOf(size_t b, const std::string& property) const
{
    for (size_t i = m_blocks[b].first + 1; i < m_blocks[b].end; ++i)
    {
        std::string value;
        if (sameText(definePropertyOf(m_lines[i].text, value), property))
            return i;
    }
    return std::string::npos;
}

std::vector<int> MaterialDefineFile::ids() const
{
    ensureIndexed();
    std::vector<int> out;
    for (size_t b = 0; b < m_blocks.size(); ++b)
        out.push_back(m_blocks[b].id);
    return out;
}

bool MaterialDefineFile::has(int id) const
{
    ensureIndexed();
    return blockOf(id) >= 0;
}

bool MaterialDefineFile::value(int id, const std::string& property, std::string& out) const
{
    ensureIndexed();
    const int p = propertyIndex(property);
    const int b = blockOf(id);
    if (p < 0 || b < 0)
        return false;
    const size_t line = lineOf(static_cast<size_t>(b), kProperties[p].name);
    if (line == std::string::npos)
    {
        out = kProperties[p].defaultText;
        return true;
    }
    std::string value;
    definePropertyOf(m_lines[line].text, value);
    out = kProperties[p].kind == PropertyKind::Name ? unquoted(value) : value;
    return true;
}

bool MaterialDefineFile::setValue(int id, const std::string& property, const std::string& value, std::string& error)
{
    index();
    const int p = propertyIndex(property);
    const int b = blockOf(id);
    if (p < 0)
    {
        error = "'" + property + "' is not a material property";
        return false;
    }
    if (b < 0)
    {
        char text[64];
        std::snprintf(text, sizeof(text), "no material %d", id);
        error = text;
        return false;
    }
    const PropertyRow& row = kProperties[p];
    // The value as the writer prints it.
    std::string text;
    switch (row.kind)
    {
    case PropertyKind::Name:
    {
        const std::string name = unquoted(trimmed(value));
        if (name.empty() || name.find_first_of("\"\r\n") != std::string::npos)
        {
            error = "a material name is not empty and holds no quote (the writer prints it between quotes, unescaped)";
            return false;
        }
        text = "\"" + name + "\"";
        break;
    }
    case PropertyKind::Integer:
    {
        int number = 0;
        if (!parseInt(trimmed(value), number))
        {
            error = "'" + value + "' is not a whole number";
            return false;
        }
        char digits[32];
        std::snprintf(digits, sizeof(digits), "%d", number);
        text = digits;
        break;
    }
    case PropertyKind::Number:
    {
        float number = 0.0f;
        if (!parseFloat(trimmed(value), number))
        {
            error = "'" + value + "' is not a number";
            return false;
        }
        text = materialNumber(number);
        break;
    }
    case PropertyKind::Flag:
    {
        const std::string flag = trimmed(value);
        if (flag != "0" && flag != "1")
        {
            error = "a flag is 0 or 1";
            return false;
        }
        text = flag;
        break;
    }
    }
    // minDamageLoss / maxDamageLoss only count while damageLoss is on.
    std::string damageLoss;
    this->value(id, "damageLoss", damageLoss);
    const bool lossOn = materialNumber(static_cast<float>(std::atof(damageLoss.c_str()))) != "-1";
    if ((sameText(row.name, "minDamageLoss") || sameText(row.name, "maxDamageLoss")) && !lossOn)
    {
        error = std::string(row.name) + " counts only while damageLoss is on (set damageLoss first)";
        return false;
    }
    const bool keep = row.always || text != row.defaultText;
    size_t line = lineOf(static_cast<size_t>(b), row.name);
    if (!keep)
    {
        if (line != std::string::npos)
            erase(line);
        index();
        if (sameText(row.name, "damageLoss"))
        {
            // damageLoss off: the writer drops its two companions too.
            const char* const companions[] = { "minDamageLoss", "maxDamageLoss" };
            for (int c = 0; c < 2; ++c)
            {
                const int bb = blockOf(id);
                const size_t l = lineOf(static_cast<size_t>(bb), companions[c]);
                if (l != std::string::npos)
                {
                    erase(l);
                    index();
                }
            }
        }
        return true;
    }
    if (line != std::string::npos)
    {
        // In place: the indentation and the command as spelt kept.
        const std::string& old = m_lines[line].text;
        size_t start = 0;
        while (start < old.size() && isBlank(old[start]))
            ++start;
        size_t end = start;
        while (end < old.size() && !isBlank(old[end]))
            ++end;
        m_lines[line].text = old.substr(0, end) + " " + text;
        return true;
    }
    // A new line after the last line that comes before it in the writer's order.
    const Block block = m_blocks[static_cast<size_t>(b)];
    size_t at = block.first + 1;
    for (size_t i = block.first + 1; i < block.end; ++i)
    {
        std::string ignored;
        const int q = propertyIndex(definePropertyOf(m_lines[i].text, ignored));
        if (q >= 0 && q < p)
            at = i + 1;
    }
    insert(at, std::string(kMaterialPrefix) + row.name + " " + text);
    index();
    return true;
}

bool MaterialDefineFile::add(int id, const std::string& name, int type, std::string& error)
{
    index();
    if (id < 0 || id >= 10000)
    {
        error = "a material id is 0 to 9999 (the engine stops the process on 10000 or more)";
        return false;
    }
    if (blockOf(id) >= 0)
    {
        char text[64];
        std::snprintf(text, sizeof(text), "material %d is there already", id);
        error = text;
        return false;
    }
    const std::string clean = unquoted(trimmed(name));
    if (clean.empty() || clean.find_first_of("\"\r\n") != std::string::npos)
    {
        error = "a material name is not empty and holds no quote";
        return false;
    }
    size_t at = m_lines.size();
    for (size_t b = 0; b < m_blocks.size(); ++b)
        if (m_blocks[b].id > id)
        {
            at = m_blocks[b].first;
            break;
        }
    char head[64], kind[64];
    std::snprintf(head, sizeof(head), "Material.active %d", id);
    std::snprintf(kind, sizeof(kind), "Material.type %d", type);
    const std::string lines[] = {
        head, "Material.name \"" + clean + "\"", kind, "Material.friction 1", "Material.elasticity 0",
        "Material.resistance 0.01", "",
    };
    for (size_t i = 0; i < sizeof(lines) / sizeof(lines[0]); ++i)
        insert(at + i, lines[i]);
    index();
    return true;
}

bool MaterialDefineFile::remove(int id, std::string& error)
{
    index();
    const int b = blockOf(id);
    if (b < 0)
    {
        char text[64];
        std::snprintf(text, sizeof(text), "no material %d", id);
        error = text;
        return false;
    }
    const Block block = m_blocks[static_cast<size_t>(b)];
    for (size_t i = block.end; i-- > block.first;)
        erase(i);
    index();
    return true;
}

std::vector<std::string> MaterialDefineFile::violations() const
{
    ensureIndexed();
    std::vector<std::string> out;
    char text[200];
    if (!m_blocks.empty() && m_blocks[0].first != 0)
        out.push_back("lines before the first material block");
    for (size_t b = 0; b < m_blocks.size(); ++b)
    {
        const Block& block = m_blocks[b];
        if (b > 0 && block.id <= m_blocks[b - 1].id)
        {
            std::snprintf(text, sizeof(text), "material %d comes after %d", block.id, m_blocks[b - 1].id);
            out.push_back(text);
        }
        int last = -1;
        bool lossOn = false;
        std::vector<bool> seen(kPropertyCount, false);
        for (size_t i = block.first + 1; i < block.end; ++i)
        {
            const std::string& line = m_lines[i].text;
            if (i == block.end - 1)
            {
                if (!line.empty())
                {
                    std::snprintf(text, sizeof(text), "material %d does not end with a blank line", block.id);
                    out.push_back(text);
                }
                if (line.empty())
                    continue;
            }
            std::string value;
            const std::string property = definePropertyOf(line, value);
            const int p = propertyIndex(property);
            if (p < 0)
            {
                std::snprintf(text, sizeof(text), "material %d: '%s' is no property line", block.id, line.c_str());
                out.push_back(text);
                continue;
            }
            const PropertyRow& row = kProperties[p];
            seen[static_cast<size_t>(p)] = true;
            if (p <= last)
            {
                std::snprintf(text, sizeof(text), "material %d: %s out of the writer's order", block.id, row.name);
                out.push_back(text);
            }
            last = p;
            if (line != std::string(kMaterialPrefix) + row.name + " " + value)
            {
                std::snprintf(text, sizeof(text), "material %d: %s not as the writer spells it", block.id, row.name);
                out.push_back(text);
            }
            if (row.kind == PropertyKind::Number)
            {
                float number = 0.0f;
                if (!parseFloat(value, number) || materialNumber(number) != value)
                {
                    std::snprintf(text, sizeof(text), "material %d: %s %s not as the writer prints it", block.id, row.name,
                                  value.c_str());
                    out.push_back(text);
                }
            }
            if (row.kind == PropertyKind::Name && (value.size() < 2 || value[0] != '"' || value[value.size() - 1] != '"'))
            {
                std::snprintf(text, sizeof(text), "material %d: its name is not quoted", block.id);
                out.push_back(text);
            }
            if (!row.always && value == row.defaultText)
            {
                std::snprintf(text, sizeof(text), "material %d: %s written at its default", block.id, row.name);
                out.push_back(text);
            }
            if (row.kind == PropertyKind::Flag && value != "1")
            {
                std::snprintf(text, sizeof(text), "material %d: flag %s is not 1", block.id, row.name);
                out.push_back(text);
            }
            if (sameText(row.name, "damageLoss"))
                lossOn = value != "-1";
            if ((sameText(row.name, "minDamageLoss") || sameText(row.name, "maxDamageLoss")) && !lossOn)
            {
                std::snprintf(text, sizeof(text), "material %d: %s while damageLoss is off", block.id, row.name);
                out.push_back(text);
            }
        }
        for (size_t p = 0; p < kPropertyCount; ++p)
            if (kProperties[p].always && !seen[p])
            {
                std::snprintf(text, sizeof(text), "material %d has no %s line", block.id, kProperties[p].name);
                out.push_back(text);
            }
    }
    return out;
}

// MaterialCellFile

const char* MaterialCellFile::kindName(Kind kind)
{
    return kind == Effect ? "effect" : kind == Decal ? "decal" : "sound";
}

bool MaterialCellFile::kindFromName(const std::string& name, Kind& kind)
{
    if (sameText(name, "effect"))
        kind = Effect;
    else if (sameText(name, "decal"))
        kind = Decal;
    else if (sameText(name, "sound"))
        kind = Sound;
    else
        return false;
    return true;
}

namespace
{

const char* const kTemplateCommands[] = { "MaterialManager.setEffectTemplate", "MaterialManager.setDecalTemplate",
                                          "MaterialManager.setSoundTemplate" };

} // namespace

void MaterialCellFile::index()
{
    m_blocks.clear();
    m_byPair.clear();
    m_dirty = false;
    for (size_t i = 0; i < m_lines.size(); ++i)
    {
        const std::vector<std::string> words = wordsOf(m_lines[i].text);
        int a = 0, b = 0;
        if (words.size() >= 3 && sameText(words[0], "MaterialManager.createCell") && parseInt(words[1], a)
            && parseInt(words[2], b))
        {
            if (!m_blocks.empty())
                m_blocks.back().end = i;
            Block block;
            block.a = a;
            block.b = b;
            block.first = i;
            block.end = m_lines.size();
            if (!m_byPair.count(std::make_pair(a, b)))
                m_byPair[std::make_pair(a, b)] = m_blocks.size();
            m_blocks.push_back(block);
        }
    }
}

int MaterialCellFile::blockOf(int a, int b) const
{
    ensureIndexed();
    const std::map<std::pair<int, int>, size_t>::const_iterator found = m_byPair.find(std::make_pair(a, b));
    return found == m_byPair.end() ? -1 : static_cast<int>(found->second);
}

long MaterialCellFile::lineKey(size_t line) const
{
    const std::vector<std::string> words = wordsOf(m_lines[line].text);
    if (words.empty())
        return -1;
    if (sameText(words[0], "MaterialManager.damageMod"))
        return 0;
    for (int k = 0; k < 3; ++k)
    {
        int index = 0;
        if (sameText(words[0], kTemplateCommands[k]) && words.size() >= 2 && parseInt(words[1], index))
            return static_cast<long>(k + 1) * 100000L + index;
    }
    return -1;
}

std::vector<std::pair<int, int> > MaterialCellFile::cells() const
{
    ensureIndexed();
    std::vector<std::pair<int, int> > out;
    for (size_t i = 0; i < m_blocks.size(); ++i)
        out.push_back(std::make_pair(m_blocks[i].a, m_blocks[i].b));
    return out;
}

bool MaterialCellFile::has(int a, int b) const
{
    ensureIndexed();
    return blockOf(a, b) >= 0;
}

bool MaterialCellFile::damageMod(int a, int b, std::string& out) const
{
    ensureIndexed();
    const int block = blockOf(a, b);
    if (block < 0)
        return false;
    out.clear();
    for (size_t i = m_blocks[static_cast<size_t>(block)].first + 1; i < m_blocks[static_cast<size_t>(block)].end; ++i)
        if (lineKey(i) == 0)
            out = restAfter(m_lines[i].text, 1);
    return true;
}

bool MaterialCellFile::templateAt(int a, int b, Kind kind, int index, std::string& out) const
{
    ensureIndexed();
    const int block = blockOf(a, b);
    if (block < 0)
        return false;
    out.clear();
    const long key = static_cast<long>(kind + 1) * 100000L + index;
    for (size_t i = m_blocks[static_cast<size_t>(block)].first + 1; i < m_blocks[static_cast<size_t>(block)].end; ++i)
        if (lineKey(i) == key)
            out = restAfter(m_lines[i].text, 2);
    return true;
}

std::vector<int> MaterialCellFile::indexes(int a, int b, Kind kind) const
{
    ensureIndexed();
    std::vector<int> out;
    const int block = blockOf(a, b);
    if (block < 0)
        return out;
    for (size_t i = m_blocks[static_cast<size_t>(block)].first + 1; i < m_blocks[static_cast<size_t>(block)].end; ++i)
    {
        const long key = lineKey(i);
        if (key / 100000L == static_cast<long>(kind + 1) && key > 0)
            out.push_back(static_cast<int>(key % 100000L));
    }
    return out;
}

bool MaterialCellFile::add(int a, int b, std::string& error)
{
    index();
    if (a < 0 || a >= 10000 || b < 0 || b >= 10000)
    {
        error = "material ids are 0 to 9999";
        return false;
    }
    if (blockOf(a, b) >= 0)
    {
        char text[64];
        std::snprintf(text, sizeof(text), "the cell %d %d is there already", a, b);
        error = text;
        return false;
    }
    size_t at = m_lines.size();
    for (size_t i = 0; i < m_blocks.size(); ++i)
        if (m_blocks[i].a > a || (m_blocks[i].a == a && m_blocks[i].b > b))
        {
            at = m_blocks[i].first;
            break;
        }
    char line[96];
    std::snprintf(line, sizeof(line), "MaterialManager.createCell %d %d", a, b);
    insert(at, line);
    index();
    return true;
}

bool MaterialCellFile::remove(int a, int b, std::string& error)
{
    index();
    const int block = blockOf(a, b);
    if (block < 0)
    {
        char text[64];
        std::snprintf(text, sizeof(text), "no cell %d %d", a, b);
        error = text;
        return false;
    }
    const Block found = m_blocks[static_cast<size_t>(block)];
    for (size_t i = found.end; i-- > found.first;)
        erase(i);
    index();
    return true;
}

int MaterialCellFile::removeCellsOf(int id)
{
    index();
    int removed = 0;
    for (size_t i = m_blocks.size(); i-- > 0;)
    {
        if (m_blocks[i].a != id && m_blocks[i].b != id)
            continue;
        const Block found = m_blocks[i];
        for (size_t l = found.end; l-- > found.first;)
            erase(l);
        ++removed;
        index();
    }
    return removed;
}

bool MaterialCellFile::setDamageMod(int a, int b, const std::string& value, std::string& error)
{
    index();
    const int block = blockOf(a, b);
    if (block < 0)
    {
        char text[64];
        std::snprintf(text, sizeof(text), "no cell %d %d (add it first)", a, b);
        error = text;
        return false;
    }
    float number = 0.0f;
    if (!parseFloat(trimmed(value), number))
    {
        error = "'" + value + "' is not a number";
        return false;
    }
    const Block found = m_blocks[static_cast<size_t>(block)];
    size_t line = std::string::npos;
    for (size_t i = found.first + 1; i < found.end; ++i)
        if (lineKey(i) == 0)
            line = i;
    if (number < 0.0f)
    {
        // Unset: the writer prints damageMod only when it is 0 or more.
        if (line != std::string::npos)
            erase(line);
        index();
        return true;
    }
    const std::string text = "MaterialManager.damageMod " + materialNumber(number);
    if (line != std::string::npos)
        m_lines[line].text = text;
    else
        insert(found.first + 1, text);
    index();
    return true;
}

bool MaterialCellFile::setTemplate(int a, int b, Kind kind, int index, const std::string& name, std::string& error)
{
    this->index();
    const int block = blockOf(a, b);
    if (block < 0)
    {
        char text[64];
        std::snprintf(text, sizeof(text), "no cell %d %d (add it first)", a, b);
        error = text;
        return false;
    }
    if (index < 0 || index >= 100000)
    {
        error = "a template index is 0 or more";
        return false;
    }
    const std::string clean = trimmed(name);
    if (clean.find_first_of(" \t\"") != std::string::npos)
    {
        error = "'" + clean + "' is not a template name";
        return false;
    }
    const Block found = m_blocks[static_cast<size_t>(block)];
    const long key = static_cast<long>(kind + 1) * 100000L + index;
    size_t line = std::string::npos;
    size_t at = found.first + 1;
    for (size_t i = found.first + 1; i < found.end; ++i)
    {
        const long k = lineKey(i);
        if (k == key)
            line = i;
        if (k >= 0 && k < key)
            at = i + 1;
    }
    if (clean.empty())
    {
        if (line != std::string::npos)
            erase(line);
        this->index();
        return true;
    }
    char head[96];
    std::snprintf(head, sizeof(head), "%s %d ", kTemplateCommands[kind], index);
    if (line != std::string::npos)
        m_lines[line].text = head + clean;
    else
        insert(at, head + clean);
    this->index();
    return true;
}

std::vector<std::string> MaterialCellFile::violations() const
{
    ensureIndexed();
    std::vector<std::string> out;
    char text[200];
    for (size_t b = 0; b < m_blocks.size(); ++b)
    {
        const Block& block = m_blocks[b];
        if (b > 0 && (block.a < m_blocks[b - 1].a || (block.a == m_blocks[b - 1].a && block.b <= m_blocks[b - 1].b)))
        {
            std::snprintf(text, sizeof(text), "cell %d %d comes after %d %d", block.a, block.b, m_blocks[b - 1].a,
                          m_blocks[b - 1].b);
            out.push_back(text);
        }
        long last = -1;
        for (size_t i = block.first + 1; i < block.end; ++i)
        {
            const long key = lineKey(i);
            if (key < 0)
            {
                std::snprintf(text, sizeof(text), "cell %d %d: '%s' is no cell line", block.a, block.b, m_lines[i].text.c_str());
                out.push_back(text);
                continue;
            }
            if (key <= last)
            {
                std::snprintf(text, sizeof(text), "cell %d %d: lines out of the writer's order", block.a, block.b);
                out.push_back(text);
            }
            last = key;
            if (key == 0)
            {
                const std::string value = restAfter(m_lines[i].text, 1);
                float number = 0.0f;
                if (!parseFloat(value, number) || number < 0.0f || materialNumber(number) != value)
                {
                    std::snprintf(text, sizeof(text), "cell %d %d: damageMod %s not as the writer prints it", block.a,
                                  block.b, value.c_str());
                    out.push_back(text);
                }
            }
            else if (restAfter(m_lines[i].text, 2).empty())
            {
                std::snprintf(text, sizeof(text), "cell %d %d: a template line without a name", block.a, block.b);
                out.push_back(text);
            }
        }
    }
    return out;
}

} // namespace bf2editor
