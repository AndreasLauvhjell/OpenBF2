// Code/Tools/BF2Editor/Framework/ObjectLibrary.cpp - see ObjectLibrary.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/ObjectLibrary.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

namespace bf2editor
{

namespace
{

const char* const kNewline = "\r\n";

bool sameText(const std::string& a, const std::string& b)
{
    return _stricmp(a.c_str(), b.c_str()) == 0;
}

std::string lineLabel(size_t index, const std::string& line)
{
    char number[32];
    std::snprintf(number, sizeof(number), "line %u", static_cast<unsigned>(index + 1));
    return std::string(number) + " '" + line + "'";
}

// The file's lines, CR and surrounding blanks taken off.
std::vector<std::string> splitLines(const std::string& text)
{
    std::vector<std::string> lines;
    std::string line;
    std::istringstream in(text);
    while (std::getline(in, line))
    {
        size_t begin = 0, end = line.size();
        while (begin < end && std::isspace(static_cast<unsigned char>(line[begin])))
            ++begin;
        while (end > begin && std::isspace(static_cast<unsigned char>(line[end - 1])))
            --end;
        lines.push_back(line.substr(begin, end - begin));
    }
    return lines;
}

std::vector<std::string> words(const std::string& line)
{
    std::vector<std::string> out;
    std::istringstream in(line);
    std::string word;
    while (in >> word)
        out.push_back(word);
    return out;
}

bool parseFloat(const std::string& text, float& out)
{
    if (text.empty())
        return false;
    char* end = 0;
    const double value = std::strtod(text.c_str(), &end);
    if (!end || *end != 0)
        return false;
    out = static_cast<float>(value);
    return true;
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

// "x/y/z", as the console reads a Vec3.
bool parseVec3(const std::string& text, float out[3])
{
    const size_t first = text.find('/');
    const size_t second = first == std::string::npos ? std::string::npos : text.find('/', first + 1);
    if (second == std::string::npos || text.find('/', second + 1) != std::string::npos)
        return false;
    return parseFloat(text.substr(0, first), out[0]) && parseFloat(text.substr(first + 1, second - first - 1), out[1])
        && parseFloat(text.substr(second + 1), out[2]);
}

std::string formatVec3(const float v[3], int decimals)
{
    char text[160];
    std::snprintf(text, sizeof(text), "%.*f/%.*f/%.*f", decimals, v[0], decimals, v[1], decimals, v[2]);
    return text;
}

void setVec3(float out[3], float x, float y, float z)
{
    out[0] = x;
    out[1] = y;
    out[2] = z;
}

// The precache block both writers open with, and the blank line after it.
std::string renderPreload(const std::vector<std::string>& preload)
{
    std::string text = std::string("console.allowMultipleFileLoad 0") + kNewline;
    for (size_t i = 0; i < preload.size(); ++i)
        text += preload[i] + kNewline;
    text += std::string("console.allowMultipleFileLoad 1") + kNewline;
    text += kNewline;
    return text;
}

// A line both formats skip or keep aside: blank, rem, the precache switches, a run line. True when it
// was one of those.
bool preloadLine(const std::vector<std::string>& w, const std::string& line, std::vector<std::string>& preload)
{
    if (w.empty() || sameText(w[0], "rem"))
        return true;
    if (sameText(w[0], "console.allowMultipleFileLoad"))
        return true;
    if (sameText(w[0], "run"))
    {
        if (std::find(preload.begin(), preload.end(), line) == preload.end())
            preload.push_back(line);
        return true;
    }
    return false;
}

bool makeFolders(const std::string& path)
{
    // Every folder along the path, the first ones allowed to exist already.
    for (size_t i = 1; i <= path.size(); ++i)
    {
        if (i < path.size() && path[i] != '/' && path[i] != '\\')
            continue;
        const std::string part = path.substr(0, i);
        if (part.empty() || part[part.size() - 1] == ':')
            continue;
        if (!CreateDirectoryA(part.c_str(), 0) && GetLastError() != ERROR_ALREADY_EXISTS)
            return false;
    }
    return true;
}

std::string folderOf(const std::string& path)
{
    const size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

// The .con files of a folder, without the extension, sorted without regard to case.
std::vector<std::string> conFiles(const std::string& folder)
{
    std::vector<std::string> names;
    WIN32_FIND_DATAA found;
    HANDLE search = FindFirstFileA((folder + "/*.con").c_str(), &found);
    if (search == INVALID_HANDLE_VALUE)
        return names;
    do
    {
        if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        const std::string name = found.cFileName;
        if (name.size() > 4 && _stricmp(name.c_str() + name.size() - 4, ".con") == 0)
            names.push_back(name.substr(0, name.size() - 4));
    } while (FindNextFileA(search, &found));
    FindClose(search);
    struct NoCase
    {
        bool operator()(const std::string& a, const std::string& b) const { return _stricmp(a.c_str(), b.c_str()) < 0; }
    };
    std::sort(names.begin(), names.end(), NoCase());
    return names;
}

} // namespace

// Bundles

BundleRecord::BundleRecord()
    : layer(1)
{
    setVec3(offset, 0.0f, 0.0f, 0.0f);
    setVec3(rotation, 0.0f, 0.0f, 0.0f);
}

bool ObjectBundleFile::parse(const std::string& text, std::string& error)
{
    preload.clear();
    records.clear();
    const std::vector<std::string> lines = splitLines(text);
    for (size_t i = 0; i < lines.size(); ++i)
    {
        const std::vector<std::string> w = words(lines[i]);
        if (preloadLine(w, lines[i], preload))
            continue;
        BundleRecord record;
        if (sameText(w[0], "ObjectBundle.CreateRootObject"))
        {
            if (w.size() != 4 || !parseVec3(w[2], record.rotation) || !parseInt(w[3], record.layer))
            {
                error = lineLabel(i, lines[i]) + ": CreateRootObject takes a template, a rotation x/y/z and a layer";
                return false;
            }
            if (!records.empty())
            {
                error = lineLabel(i, lines[i]) + ": a second root (a bundle file holds one bundle)";
                return false;
            }
        }
        else if (sameText(w[0], "ObjectBundle.CreateObject"))
        {
            if (w.size() != 5 || !parseVec3(w[2], record.offset) || !parseVec3(w[3], record.rotation)
                || !parseInt(w[4], record.layer))
            {
                error = lineLabel(i, lines[i]) + ": CreateObject takes a template, an offset x/y/z, a rotation x/y/z "
                                                 "and a layer";
                return false;
            }
            if (records.empty())
            {
                error = lineLabel(i, lines[i]) + ": a member before the root";
                return false;
            }
        }
        else
        {
            error = lineLabel(i, lines[i]) + ": not a line of a bundle file";
            return false;
        }
        if (record.layer < 0 || record.layer > 255)
        {
            error = lineLabel(i, lines[i]) + ": a layer is 0 to 255";
            return false;
        }
        record.templateName = w[1];
        records.push_back(record);
    }
    if (records.empty())
    {
        error = "no ObjectBundle.CreateRootObject line: not a bundle";
        return false;
    }
    return true;
}

std::string ObjectBundleFile::render() const
{
    std::string text = renderPreload(preload);
    char layer[16];
    for (size_t i = 0; i < records.size(); ++i)
    {
        const BundleRecord& r = records[i];
        std::snprintf(layer, sizeof(layer), "%d", r.layer);
        if (i == 0)
            text += "ObjectBundle.CreateRootObject " + r.templateName + " " + formatVec3(r.rotation, 2) + " " + layer;
        else
            text += "ObjectBundle.CreateObject " + r.templateName + " " + formatVec3(r.offset, 2) + " "
                  + formatVec3(r.rotation, 2) + " " + layer;
        text += kNewline;
    }
    return text;
}

// Brushes

LevelBrushItem::LevelBrushItem()
{
    setVec3(position, 0.0f, 0.0f, 0.0f);
    setVec3(rotation, 0.0f, 0.0f, 0.0f);
    setVec3(scale, 0.0f, 0.0f, 0.0f);
}

LevelBrushFile::LevelBrushFile()
    : spacing(10.0f)
    , snapToGround(true)
    , rotationRangesCopied(false)
{
    float* const ranges[] = { randomPositionMin, randomPositionMax, randomRotationMin, randomRotationMax,
                              randomScaleMin, randomScaleMax };
    for (size_t i = 0; i < sizeof(ranges) / sizeof(ranges[0]); ++i)
        setVec3(ranges[i], 0.0f, 0.0f, 0.0f);
}

bool LevelBrushFile::parse(const std::string& text, std::string& error)
{
    *this = LevelBrushFile();
    // The setters the console binds: LevelBrush.<range> and the item setters in both spellings (the
    // retail property grid's LevelBrushItem.Item* do what LevelBrush.SetItem* do).
    struct Range { const char* command; float* target; };
    const Range ranges[] = {
        { "LevelBrush.RandomPositionMin", randomPositionMin }, { "LevelBrush.RandomPositionMax", randomPositionMax },
        { "LevelBrush.RandomRotationMin", randomRotationMin }, { "LevelBrush.RandomRotationMax", randomRotationMax },
        { "LevelBrush.RandomScaleMin", randomScaleMin },       { "LevelBrush.RandomScaleMax", randomScaleMax },
    };
    const std::vector<std::string> lines = splitLines(text);
    for (size_t i = 0; i < lines.size(); ++i)
    {
        const std::vector<std::string> w = words(lines[i]);
        if (preloadLine(w, lines[i], preload))
            continue;
        const std::string& command = w[0];
        if (sameText(command, "LevelBrush.CreateBrush"))
        {
            if (w.size() != 2)
            {
                error = lineLabel(i, lines[i]) + ": CreateBrush takes a name";
                return false;
            }
            // The retail console makes a second name a second brush; a file here holds one.
            if (!name.empty() && !sameText(name, w[1]))
            {
                error = lineLabel(i, lines[i]) + ": a second brush (a brush file holds one brush)";
                return false;
            }
            name = w[1];
            continue;
        }
        if (name.empty())
        {
            error = lineLabel(i, lines[i]) + ": a brush setting before LevelBrush.CreateBrush";
            return false;
        }
        if (w.size() != 2)
        {
            error = lineLabel(i, lines[i]) + ": takes one value";
            return false;
        }
        const std::string& value = w[1];
        bool understood = false;
        for (size_t r = 0; r < sizeof(ranges) / sizeof(ranges[0]); ++r)
        {
            if (!sameText(command, ranges[r].command))
                continue;
            if (!parseVec3(value, ranges[r].target))
            {
                error = lineLabel(i, lines[i]) + ": the value is x/y/z";
                return false;
            }
            understood = true;
        }
        if (understood)
            continue;
        if (sameText(command, "LevelBrush.Spacing"))
        {
            if (!parseFloat(value, spacing))
            {
                error = lineLabel(i, lines[i]) + ": the spacing is a number";
                return false;
            }
            continue;
        }
        if (sameText(command, "LevelBrush.SnapToGround"))
        {
            int flag = 0;
            if (!parseInt(value, flag))
            {
                error = lineLabel(i, lines[i]) + ": SnapToGround is 0 or 1";
                return false;
            }
            snapToGround = flag != 0;
            continue;
        }
        if (sameText(command, "LevelBrush.AddItem"))
        {
            LevelBrushItem item;
            item.templateName = value;
            items.push_back(item);
            continue;
        }
        float* target = 0;
        if (!items.empty())
        {
            LevelBrushItem& item = items.back();
            if (sameText(command, "LevelBrush.SetItemPosition") || sameText(command, "LevelBrushItem.ItemPosition"))
                target = item.position;
            else if (sameText(command, "LevelBrush.SetItemRotation") || sameText(command, "LevelBrushItem.ItemRotation"))
                target = item.rotation;
            else if (sameText(command, "LevelBrush.SetItemScale") || sameText(command, "LevelBrushItem.ItemScale"))
                target = item.scale;
        }
        else if (_strnicmp(command.c_str(), "LevelBrush.SetItem", 18) == 0
                 || _strnicmp(command.c_str(), "LevelBrushItem.", 15) == 0)
        {
            error = lineLabel(i, lines[i]) + ": an item setting before any LevelBrush.AddItem";
            return false;
        }
        if (!target)
        {
            error = lineLabel(i, lines[i]) + ": not a line of a brush file";
            return false;
        }
        if (!parseVec3(value, target))
        {
            error = lineLabel(i, lines[i]) + ": the value is x/y/z";
            return false;
        }
    }
    if (name.empty())
    {
        error = "no LevelBrush.CreateBrush line: not a brush";
        return false;
    }
    bool nonZero = false;
    for (int k = 0; k < 3; ++k)
        nonZero = nonZero || randomPositionMin[k] != 0.0f || randomPositionMax[k] != 0.0f;
    rotationRangesCopied = nonZero && std::memcmp(randomRotationMin, randomPositionMin, sizeof(randomPositionMin)) == 0
                        && std::memcmp(randomRotationMax, randomPositionMax, sizeof(randomPositionMax)) == 0;
    return true;
}

std::string LevelBrushFile::render() const
{
    std::string text = renderPreload(preload);
    text += "rem *** " + name + " ***" + kNewline;
    text += "LevelBrush.CreateBrush " + name + kNewline;
    // The real rotation ranges, where the retail writer printed the position ranges again.
    text += "LevelBrush.RandomPositionMin " + formatVec3(randomPositionMin, 3) + kNewline;
    text += "LevelBrush.RandomPositionMax " + formatVec3(randomPositionMax, 3) + kNewline;
    text += "LevelBrush.RandomRotationMin " + formatVec3(randomRotationMin, 3) + kNewline;
    text += "LevelBrush.RandomRotationMax " + formatVec3(randomRotationMax, 3) + kNewline;
    text += "LevelBrush.RandomScaleMin " + formatVec3(randomScaleMin, 3) + kNewline;
    text += "LevelBrush.RandomScaleMax " + formatVec3(randomScaleMax, 3) + kNewline;
    char line[96];
    std::snprintf(line, sizeof(line), "LevelBrush.Spacing %f", spacing);
    text += std::string(line) + kNewline;
    std::snprintf(line, sizeof(line), "LevelBrush.SnapToGround %d", snapToGround ? 1 : 0);
    text += std::string(line) + kNewline;
    for (size_t i = 0; i < items.size(); ++i)
    {
        text += "LevelBrush.AddItem " + items[i].templateName + kNewline;
        text += "LevelBrush.SetItemPosition " + formatVec3(items[i].position, 3) + kNewline;
        text += "LevelBrush.SetItemRotation " + formatVec3(items[i].rotation, 3) + kNewline;
        text += "LevelBrush.SetItemScale " + formatVec3(items[i].scale, 3) + kNewline;
    }
    return text;
}

// Random numbers

LibraryRandom::LibraryRandom(unsigned seed)
    : m_state(1)
{
    this->seed(seed);
}

void LibraryRandom::seed(unsigned seed)
{
    m_state = seed % 2147483647u;
    if (m_state == 0)
        m_state = 1;
}

float LibraryRandom::next()
{
    m_state = static_cast<unsigned>((static_cast<unsigned long long>(m_state) * 48271ull) % 2147483647ull);
    // 1 .. 2^31 - 2 onto [0, 1), through a double so every state maps below 1.
    return static_cast<float>(static_cast<double>(m_state - 1) / 2147483646.0);
}

float LibraryRandom::between(float a, float b)
{
    return a + next() * (b - a);
}

// The folder

bool validLibraryName(const std::string& name)
{
    if (name.empty() || name.size() > 64)
        return false;
    for (size_t i = 0; i < name.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(name[i]);
        if (!std::isalnum(c) && c != '_' && c != '-')
            return false;
    }
    return true;
}

const char* ObjectLibrary::defaultFolder()
{
    return "bf2editor/Content/LevelEditor";
}

ObjectLibrary::ObjectLibrary()
    : m_folder(defaultFolder())
    , m_brushesLoaded(false)
{
}

void ObjectLibrary::setFolder(const std::string& folder)
{
    std::string f = folder.empty() ? std::string(defaultFolder()) : folder;
    while (f.size() > 1 && (f[f.size() - 1] == '/' || f[f.size() - 1] == '\\'))
        f.erase(f.size() - 1);
    m_folder = f;
    m_brushesLoaded = false;
}

std::string ObjectLibrary::bundleFolder() const
{
    return m_folder + "/ObjectBundle";
}

std::string ObjectLibrary::brushFolder() const
{
    return m_folder + "/LevelBrushes";
}

std::string ObjectLibrary::bundlePath(const std::string& name) const
{
    return bundleFolder() + "/" + name + ".con";
}

bool ObjectLibrary::readTextFile(const std::string& path, std::string& text, std::string& error)
{
    std::ifstream in(path.c_str(), std::ios::binary);
    if (!in)
    {
        error = "cannot read " + path;
        return false;
    }
    std::ostringstream all;
    all << in.rdbuf();
    text = all.str();
    return true;
}

bool ObjectLibrary::writeTextFile(const std::string& path, const std::string& text, std::string& error)
{
    const std::string folder = folderOf(path);
    if (!folder.empty() && !makeFolders(folder))
    {
        error = "cannot make the folder " + folder;
        return false;
    }
    const std::string temporary = path + ".tmp";
    {
        std::ofstream out(temporary.c_str(), std::ios::binary | std::ios::trunc);
        if (!out || !out.write(text.data(), static_cast<std::streamsize>(text.size())))
        {
            error = "cannot write " + temporary;
            return false;
        }
    }
    if (!MoveFileExA(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING))
    {
        DeleteFileA(temporary.c_str());
        error = "cannot replace " + path;
        return false;
    }
    return true;
}

std::vector<std::string> ObjectLibrary::bundleNames() const
{
    return conFiles(bundleFolder());
}

bool ObjectLibrary::bundleExists(const std::string& name) const
{
    const DWORD attributes = GetFileAttributesA(bundlePath(name).c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool ObjectLibrary::loadBundle(const std::string& name, ObjectBundleFile& out, std::string& error) const
{
    if (!validLibraryName(name))
    {
        error = "'" + name + "' is not a bundle name (letters, digits, _ and -)";
        return false;
    }
    std::string text;
    if (!bundleExists(name))
    {
        error = "no bundle named " + name + " in " + bundleFolder();
        return false;
    }
    if (!readTextFile(bundlePath(name), text, error))
        return false;
    std::string why;
    if (!out.parse(text, why))
    {
        error = bundlePath(name) + ": " + why;
        return false;
    }
    return true;
}

bool ObjectLibrary::saveBundle(const std::string& name, const ObjectBundleFile& bundle, bool overwrite,
                               std::string& error) const
{
    if (!validLibraryName(name))
    {
        error = "'" + name + "' is not a bundle name (letters, digits, _ and -)";
        return false;
    }
    if (!overwrite && bundleExists(name))
    {
        error = "a bundle named " + name + " exists already";
        return false;
    }
    return writeTextFile(bundlePath(name), bundle.render(), error);
}

bool ObjectLibrary::deleteBundle(const std::string& name, std::string& error) const
{
    if (!validLibraryName(name) || !bundleExists(name))
    {
        error = "no bundle named " + name;
        return false;
    }
    if (!DeleteFileA(bundlePath(name).c_str()))
    {
        error = "cannot delete " + bundlePath(name);
        return false;
    }
    return true;
}

void ObjectLibrary::loadBrushes()
{
    m_brushes.clear();
    m_brushFiles.clear();
    m_brushProblems.clear();
    m_brushesLoaded = true;
    const std::vector<std::string> files = conFiles(brushFolder());
    for (size_t i = 0; i < files.size(); ++i)
    {
        const std::string path = brushFolder() + "/" + files[i] + ".con";
        std::string text, error;
        LevelBrushFile brush;
        if (!readTextFile(path, text, error) || !brush.parse(text, error))
        {
            m_brushProblems.push_back(path + ": " + error);
            continue;
        }
        if (this->brush(brush.name))
        {
            m_brushProblems.push_back(path + ": another file defines the brush " + brush.name + " already");
            continue;
        }
        m_brushes.push_back(brush);
        m_brushFiles.push_back(path);
    }
}

const std::vector<LevelBrushFile>& ObjectLibrary::brushes()
{
    if (!m_brushesLoaded)
        loadBrushes();
    return m_brushes;
}

const std::vector<std::string>& ObjectLibrary::brushProblems()
{
    if (!m_brushesLoaded)
        loadBrushes();
    return m_brushProblems;
}

LevelBrushFile* ObjectLibrary::brush(const std::string& name)
{
    if (!m_brushesLoaded)
        loadBrushes();
    for (size_t i = 0; i < m_brushes.size(); ++i)
        if (sameText(m_brushes[i].name, name))
            return &m_brushes[i];
    return 0;
}

bool ObjectLibrary::addBrush(const LevelBrushFile& brush, std::string& error)
{
    if (!validLibraryName(brush.name))
    {
        error = "'" + brush.name + "' is not a brush name (letters, digits, _ and -)";
        return false;
    }
    if (this->brush(brush.name))
    {
        error = "a brush named " + brush.name + " exists already";
        return false;
    }
    const std::string path = brushFolder() + "/" + brush.name + ".con";
    const DWORD attributes = GetFileAttributesA(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES)
    {
        error = path + " exists already (a file the library could not read as a brush)";
        return false;
    }
    if (!writeTextFile(path, brush.render(), error))
        return false;
    m_brushes.push_back(brush);
    m_brushFiles.push_back(path);
    return true;
}

bool ObjectLibrary::writeBrush(const std::string& name, std::string& error)
{
    if (!m_brushesLoaded)
        loadBrushes();
    for (size_t i = 0; i < m_brushes.size(); ++i)
        if (sameText(m_brushes[i].name, name))
            return writeTextFile(m_brushFiles[i], m_brushes[i].render(), error);
    error = "no brush named " + name;
    return false;
}

bool ObjectLibrary::deleteBrush(const std::string& name, std::string& error)
{
    if (!m_brushesLoaded)
        loadBrushes();
    for (size_t i = 0; i < m_brushes.size(); ++i)
    {
        if (!sameText(m_brushes[i].name, name))
            continue;
        if (!DeleteFileA(m_brushFiles[i].c_str()))
        {
            error = "cannot delete " + m_brushFiles[i];
            return false;
        }
        m_brushes.erase(m_brushes.begin() + static_cast<std::ptrdiff_t>(i));
        m_brushFiles.erase(m_brushFiles.begin() + static_cast<std::ptrdiff_t>(i));
        return true;
    }
    error = "no brush named " + name;
    return false;
}

} // namespace bf2editor
