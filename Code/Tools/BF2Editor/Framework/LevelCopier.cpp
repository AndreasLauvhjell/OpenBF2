// Code/Tools/BF2Editor/Framework/LevelCopier.cpp - see LevelCopier.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/LevelCopier.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace bf2editor
{

namespace
{

std::string lowered(const std::string& text)
{
    std::string out(text);
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

bool readWhole(const std::string& path, std::vector<unsigned char>& data)
{
    data.clear();
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f)
        return false;
    unsigned char buffer[65536];
    size_t n = 0;
    while ((n = std::fread(buffer, 1, sizeof(buffer), f)) > 0)
        data.insert(data.end(), buffer, buffer + n);
    const bool ok = !std::ferror(f);
    std::fclose(f);
    return ok;
}

bool writeWhole(const std::string& path, const std::vector<unsigned char>& data)
{
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f)
        return false;
    const bool ok = data.empty() || std::fwrite(&data[0], 1, data.size(), f) == data.size();
    return std::fclose(f) == 0 && ok;
}

void listFiles(const std::string& root, const std::string& relative, std::vector<std::string>& out)
{
    WIN32_FIND_DATAA found;
    const std::string pattern = root + "/" + (relative.empty() ? std::string() : relative + "/") + "*";
    HANDLE h = FindFirstFileA(pattern.c_str(), &found);
    if (h == INVALID_HANDLE_VALUE)
        return;
    do
    {
        const std::string name = found.cFileName;
        if (name == "." || name == "..")
            continue;
        const std::string path = relative.empty() ? name : relative + "/" + name;
        if (found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            listFiles(root, path, out);
        else
            out.push_back(path);
    } while (FindNextFileA(h, &found));
    FindClose(h);
}

bool makeFolders(const std::string& path)
{
    std::string walked;
    for (size_t i = 0; i <= path.size(); ++i)
    {
        if (i == path.size() || path[i] == '/' || path[i] == '\\')
        {
            if (!walked.empty() && walked[walked.size() - 1] != ':')
                CreateDirectoryA(walked.c_str(), 0);
        }
        if (i < path.size())
            walked += path[i];
    }
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

void removeTree(const std::string& root)
{
    std::vector<std::string> files;
    listFiles(root, std::string(), files);
    for (size_t i = 0; i < files.size(); ++i)
        DeleteFileA((root + "/" + files[i]).c_str());
    // Folders deepest first.
    std::vector<std::string> folders;
    for (size_t i = 0; i < files.size(); ++i)
    {
        std::string f = files[i];
        for (size_t slash = f.rfind('/'); slash != std::string::npos; slash = f.rfind('/'))
        {
            f = f.substr(0, slash);
            folders.push_back(f);
        }
    }
    std::sort(folders.begin(), folders.end());
    folders.erase(std::unique(folders.begin(), folders.end()), folders.end());
    for (size_t i = folders.size(); i-- > 0;)
        RemoveDirectoryA((root + "/" + folders[i]).c_str());
    RemoveDirectoryA(root.c_str());
}

bool isTextFile(const std::string& relative)
{
    const std::string low = lowered(relative);
    const size_t dot = low.rfind('.');
    const std::string ext = dot == std::string::npos ? std::string() : low.substr(dot);
    return ext == ".con" || ext == ".emi" || ext == ".tai" || ext == ".desc" || ext == ".txt" || ext == ".ai"
        || ext == ".cfg";
}

// The map description's display name, <name> ... </name>, with the new level's name.
std::string renameDisplay(const std::string& text, const std::string& to)
{
    const std::string low = lowered(text);
    const size_t open = low.find("<name>");
    const size_t close = low.find("</name>");
    if (open == std::string::npos || close == std::string::npos || close < open)
        return text;
    std::string display = to;
    std::replace(display.begin(), display.end(), '_', ' ');
    return text.substr(0, open + 6) + " " + display + " " + text.substr(close);
}

} // namespace

bool validLevelName(const std::string& name)
{
    if (name.empty() || name.size() > 64)
        return false;
    for (size_t i = 0; i < name.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(name[i]);
        if (!std::isalnum(c) && c != '_')
            return false;
    }
    return true;
}

std::string retargetLevelPaths(const std::string& text, const std::string& from, const std::string& to, int& count)
{
    count = 0;
    const std::string low = lowered(text);
    const std::string needle = "levels";
    const std::string fromLow = lowered(from);
    std::string out;
    size_t copied = 0;
    for (size_t at = low.find(needle); at != std::string::npos; at = low.find(needle, at + 1))
    {
        const size_t slash = at + needle.size();
        if (slash >= low.size() || (low[slash] != '/' && low[slash] != '\\'))
            continue;
        const size_t name = slash + 1;
        const size_t end = name + fromLow.size();
        if (end >= low.size() || low.compare(name, fromLow.size(), fromLow) != 0 || (low[end] != '/' && low[end] != '\\'))
            continue;
        // A word boundary before "Levels" ("MyLevels/..." is not a level path).
        if (at > 0 && (std::isalnum(static_cast<unsigned char>(low[at - 1])) || low[at - 1] == '_'))
            continue;
        out += text.substr(copied, name - copied) + to;
        copied = end;
        ++count;
    }
    out += text.substr(copied);
    return out;
}

bool retargetTerrainData(std::vector<unsigned char>& data, const std::string& from, const std::string& to, int& count)
{
    count = 0;
    const size_t header = 57;
    size_t end = header;
    for (int line = 0; line < 4; ++line)
    {
        while (end < data.size() && data[end] != '\n')
            ++end;
        if (end >= data.size())
            return false;
        ++end;
    }
    const std::string lines(data.begin() + header, data.begin() + end);
    const std::string changed = retargetLevelPaths(lines, from, to, count);
    std::vector<unsigned char> out(data.begin(), data.begin() + header);
    out.insert(out.end(), changed.begin(), changed.end());
    out.insert(out.end(), data.begin() + end, data.end());
    data.swap(out);
    return true;
}

bool copyLevel(const std::string& source, const std::string& target, const std::string& from, const std::string& to,
               LevelCopyReport& report, std::string& error)
{
    report = LevelCopyReport();
    if (!validLevelName(to))
    {
        error = "'" + to + "' is not a level name (letters, digits and underscores)";
        return false;
    }
    if (GetFileAttributesA(target.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        error = "a level named " + to + " exists already";
        return false;
    }
    std::vector<std::string> files;
    listFiles(source, std::string(), files);
    if (files.empty())
    {
        error = "no level files in " + source;
        return false;
    }
    const std::string staging = target + ".tmp";
    removeTree(staging);
    std::vector<unsigned char> data;
    const std::string fromLow = lowered(from);
    for (size_t i = 0; i < files.size(); ++i)
    {
        const std::string& relative = files[i];
        if (!readWhole(source + "/" + relative, data))
        {
            removeTree(staging);
            error = "cannot read " + relative;
            return false;
        }
        // The name: a leading old level name becomes the new one (Info/<old>.desc, the menu pictures).
        std::string targetRelative = relative;
        const size_t slash = relative.rfind('/');
        const size_t base = slash == std::string::npos ? 0 : slash + 1;
        if (lowered(relative.substr(base, from.size())) == fromLow)
        {
            const std::string old = relative.substr(base, from.size());
            const std::string replacement = old == lowered(old) && lowered(to) != to ? lowered(to) : to;
            targetRelative = relative.substr(0, base) + replacement + relative.substr(base + from.size());
            ++report.renamed;
        }
        int references = 0;
        const std::string low = lowered(relative);
        if (low == "terraindata.raw")
        {
            if (!retargetTerrainData(data, from, to, references))
            {
                removeTree(staging);
                error = "terraindata.raw is too short for its header";
                return false;
            }
        }
        else if (isTextFile(relative))
        {
            const std::string text(data.begin(), data.end());
            std::string changed = retargetLevelPaths(text, from, to, references);
            if (low.compare(0, 5, "info/") == 0 && low.size() > 5 && low.substr(low.size() - 5) == ".desc")
            {
                const std::string renamed = renameDisplay(changed, to);
                if (renamed != changed)
                    ++references;
                changed = renamed;
            }
            data.assign(changed.begin(), changed.end());
        }
        if (references)
        {
            ++report.rewritten;
            report.references += references;
        }
        const std::string out = staging + "/" + targetRelative;
        const size_t cut = out.rfind('/');
        if (!makeFolders(out.substr(0, cut)) || !writeWhole(out, data))
        {
            removeTree(staging);
            error = "cannot write " + targetRelative;
            return false;
        }
        ++report.files;
    }
    if (!MoveFileExA(staging.c_str(), target.c_str(), 0))
    {
        removeTree(staging);
        error = "cannot move the copy into " + target;
        return false;
    }
    return true;
}

} // namespace bf2editor
