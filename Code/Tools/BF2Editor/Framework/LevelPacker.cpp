// Code/Tools/BF2Editor/Framework/LevelPacker.cpp - see LevelPacker.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/LevelPacker.h"

#include "Framework/ZipWriter.h"

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

std::string forwardSlashes(const std::string& path)
{
    std::string out(path);
    std::replace(out.begin(), out.end(), '\\', '/');
    return out;
}

// No trailing separator, one kind of slash, lower case: for comparing two folders.
std::string folderKey(const std::string& path)
{
    std::string key = lowered(forwardSlashes(path));
    while (!key.empty() && key[key.size() - 1] == '/')
        key.erase(key.size() - 1);
    return key;
}

struct FoundFile
{
    std::string relative;       // '/' separators, as on disk
    FILETIME    written;
    unsigned long long size;
};

void listFiles(const std::string& root, const std::string& relative, std::vector<FoundFile>& out)
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
        {
            FoundFile f;
            f.relative = path;
            f.written = found.ftLastWriteTime;
            f.size = (static_cast<unsigned long long>(found.nFileSizeHigh) << 32) | found.nFileSizeLow;
            out.push_back(f);
        }
    } while (FindNextFileA(h, &found));
    FindClose(h);
}

bool byPath(const FoundFile& a, const FoundFile& b)
{
    return lowered(a.relative) < lowered(b.relative);
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

void dosTimeOf(const FILETIME& written, unsigned& dosTime, unsigned& dosDate)
{
    FILETIME local;
    WORD date = 0, time = 0;
    if (FileTimeToLocalFileTime(&written, &local) && FileTimeToDosDateTime(&local, &date, &time))
    {
        dosTime = time;
        dosDate = date;
    }
    else
        dosTime = dosDate = 0;
}

bool makeFolders(const std::string& path)
{
    // Every folder along the path, from the root down.
    std::string walked;
    const std::string p = forwardSlashes(path);
    for (size_t i = 0; i <= p.size(); ++i)
    {
        if (i == p.size() || p[i] == '/')
        {
            if (!walked.empty() && walked[walked.size() - 1] != ':')
                CreateDirectoryA(walked.c_str(), 0);
        }
        if (i < p.size())
            walked += p[i];
    }
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY);
}

bool packInto(const std::string& source, const std::vector<const FoundFile*>& files, const std::string& target,
              unsigned long long& bytes, std::string& error)
{
    ZipWriter zip;
    if (!zip.open(target, error))
        return false;
    std::vector<unsigned char> data;
    for (size_t i = 0; i < files.size(); ++i)
    {
        const FoundFile& f = *files[i];
        if (!readWhole(source + "/" + f.relative, data))
        {
            std::string ignored;
            zip.close(ignored);
            DeleteFileA(target.c_str());
            error = "cannot read " + f.relative;
            return false;
        }
        unsigned dosTime = 0, dosDate = 0;
        dosTimeOf(f.written, dosTime, dosDate);
        if (!zip.add(f.relative, data.empty() ? 0 : &data[0], data.size(), dosTime, dosDate, error))
        {
            std::string ignored;
            zip.close(ignored);
            DeleteFileA(target.c_str());
            return false;
        }
    }
    if (!zip.close(error))
    {
        DeleteFileA(target.c_str());
        return false;
    }
    bytes = zip.bytesWritten();
    return true;
}

} // namespace

LevelArchive levelArchiveFor(const std::string& relativePath)
{
    const std::string low = lowered(forwardSlashes(relativePath));
    const size_t slash = low.find('/');
    const std::string folder = slash == std::string::npos ? std::string() : low.substr(0, slash);
    const size_t last = low.rfind('/');
    const std::string name = last == std::string::npos ? low : low.substr(last + 1);
    const size_t dot = name.rfind('.');
    const std::string ext = dot == std::string::npos ? std::string() : name.substr(dot);
    if (folder == "info" || folder == "editor" || name == "archive.md5" || name == "client.zip" || name == "server.zip"
        || ext == ".bak" || ext == ".old" || name.empty())
        return LevelArchive::None;
    static const char* const serverExtensions[] = { ".con", ".tweak", ".ai", ".ahm", ".clb", ".qtr", ".cls", ".qti",
                                                    ".vbf", ".gts", ".bin", ".mtl", ".obj", ".txt", ".mat", ".desc",
                                                    ".emi", ".collisionmesh" };
    for (size_t i = 0; i < sizeof(serverExtensions) / sizeof(serverExtensions[0]); ++i)
        if (ext == serverExtensions[i])
            return LevelArchive::Server;
    if (ext == ".raw")
        return (name == "terraindata.raw" || name == "undergrowth.raw") ? LevelArchive::Client : LevelArchive::Server;
    if (ext == ".tai")
        return folder == "lightmaps" ? LevelArchive::Client : LevelArchive::Server;
    return LevelArchive::Client;
}

const char* levelArchiveName(LevelArchive archive)
{
    switch (archive)
    {
    case LevelArchive::Client: return "client.zip";
    case LevelArchive::Server: return "server.zip";
    default:                   return "";
    }
}

bool packLevel(const std::string& source, const std::string& destination, LevelPackReport& report, std::string& error)
{
    report = LevelPackReport();
    if (folderKey(source) == folderKey(destination))
    {
        error = "the destination is the level's own folder: the game reads archives before loose files, so the "
                "editor would load the pack instead of the edits - pack into another mod folder";
        return false;
    }
    std::vector<FoundFile> found;
    listFiles(source, std::string(), found);
    std::sort(found.begin(), found.end(), byPath);
    std::vector<const FoundFile*> client, server, info;
    for (size_t i = 0; i < found.size(); ++i)
    {
        const LevelArchive archive = levelArchiveFor(found[i].relative);
        if (archive == LevelArchive::Client)
            client.push_back(&found[i]);
        else if (archive == LevelArchive::Server)
            server.push_back(&found[i]);
        else if (lowered(found[i].relative).compare(0, 5, "info/") == 0)
            info.push_back(&found[i]);
        else
            report.skipped.push_back(found[i].relative);
    }
    if (client.empty() && server.empty())
    {
        error = "no level files in " + source;
        return false;
    }
    if (!makeFolders(destination))
    {
        error = "cannot make the folder " + destination;
        return false;
    }
    const std::string clientTmp = destination + "/client.zip.tmp";
    const std::string serverTmp = destination + "/server.zip.tmp";
    if (!packInto(source, client, clientTmp, report.clientBytes, error))
        return false;
    if (!packInto(source, server, serverTmp, report.serverBytes, error))
    {
        DeleteFileA(clientTmp.c_str());
        return false;
    }
    // Both whole: into place.
    if (!MoveFileExA(clientTmp.c_str(), (destination + "/client.zip").c_str(), MOVEFILE_REPLACE_EXISTING)
        || !MoveFileExA(serverTmp.c_str(), (destination + "/server.zip").c_str(), MOVEFILE_REPLACE_EXISTING))
    {
        DeleteFileA(clientTmp.c_str());
        DeleteFileA(serverTmp.c_str());
        error = "cannot move the archives into " + destination;
        return false;
    }
    report.clientFiles = static_cast<int>(client.size());
    report.serverFiles = static_cast<int>(server.size());
    for (size_t i = 0; i < info.size(); ++i)
    {
        const std::string from = source + "/" + info[i]->relative;
        const std::string to = destination + "/" + info[i]->relative;
        const size_t slash = to.rfind('/');
        makeFolders(to.substr(0, slash));
        if (!CopyFileA(from.c_str(), to.c_str(), FALSE))
        {
            error = "cannot copy " + info[i]->relative;
            return false;
        }
        ++report.infoFiles;
    }
    return true;
}

} // namespace bf2editor
