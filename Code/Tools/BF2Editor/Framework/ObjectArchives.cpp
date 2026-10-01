// Code/Tools/BF2Editor/Framework/ObjectArchives.cpp - see ObjectArchives.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/ObjectArchives.h"

#include "Framework/ZipWriter.h"

#include "unzip.h"

#include <windows.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>

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

struct LooseFile
{
    std::string        relative;    // '/' separators
    FILETIME           written;
};

void listFiles(const std::string& root, const std::string& relative, std::vector<LooseFile>& out)
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
            LooseFile f;
            f.relative = path;
            f.written = found.ftLastWriteTime;
            out.push_back(f);
        }
    } while (FindNextFileA(h, &found));
    FindClose(h);
}

bool byPath(const LooseFile& a, const LooseFile& b)
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

void makeFolders(const std::string& path)
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
}

bool exists(const std::string& path)
{
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool packInto(const std::string& source, const std::vector<const LooseFile*>& files, const std::string& target,
              unsigned long long& bytes, std::string& error)
{
    ZipWriter zip;
    if (!zip.open(target, error))
        return false;
    std::vector<unsigned char> data;
    for (size_t i = 0; i < files.size(); ++i)
    {
        const LooseFile& f = *files[i];
        unsigned dosTime = 0, dosDate = 0;
        dosTimeOf(f.written, dosTime, dosDate);
        if (!readWhole(source + "/" + f.relative, data)
            || !zip.add(f.relative, data.empty() ? 0 : &data[0], data.size(), dosTime, dosDate, error))
        {
            if (error.empty())
                error = "cannot read " + f.relative;
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

bool objectArchiveTakes(const std::string& path, bool server)
{
    const std::string low = lowered(path);
    const size_t slash = low.find_last_of("/\\");
    const std::string name = slash == std::string::npos ? low : low.substr(slash + 1);
    const size_t dot = name.rfind('.');
    const std::string ext = dot == std::string::npos ? std::string() : name.substr(dot + 1);
    // Always excluded (EditorEngine 0x1005A850's first list).
    static const char* const never[] = { "zip", "psd", "nfo", "bik", "samples" };
    for (size_t i = 0; i < sizeof(never) / sizeof(never[0]); ++i)
        if (ext == never[i])
            return false;
    if (!server)
        return true;
    // The client-only list, left out of a server zip.
    static const char* const clientOnly[] = { "font", "ttf", "dif", "difb", "fx", "dfx", "mfx", "mesh", "bundledmesh",
                                              "staticmesh", "skinnedmesh", "treemesh", "mp3", "dds", "tga", "gif", "jpg",
                                              "wav", "avi", "dat", "swf", "png", "ogg", "ani", "cur" };
    for (size_t i = 0; i < sizeof(clientOnly) / sizeof(clientOnly[0]); ++i)
        if (ext == clientOnly[i])
            return false;
    return true;
}

std::vector<std::string> archivesMountedAt(const std::string& scriptText, const std::string& mountPoint)
{
    std::vector<std::string> zips;
    std::istringstream in(scriptText);
    std::string line;
    while (std::getline(in, line))
    {
        std::istringstream words(line);
        std::string command, zip, point;
        if (!(words >> command >> zip >> point))
            continue;
        if (_stricmp(command.c_str(), "fileManager.mountArchive") == 0 && _stricmp(point.c_str(), mountPoint.c_str()) == 0)
            zips.push_back(zip);
    }
    return zips;
}

bool archiveContentSize(const std::string& zipPath, unsigned long long& bytes, std::string& error)
{
    bytes = 0;
    unzFile zip = unzOpen(zipPath.c_str());
    if (!zip)
    {
        error = "cannot open " + zipPath + " as a zip";
        return false;
    }
    for (int status = unzGoToFirstFile(zip); status == UNZ_OK; status = unzGoToNextFile(zip))
    {
        unz_file_info info;
        if (unzGetCurrentFileInfo(zip, &info, 0, 0, 0, 0, 0, 0) == UNZ_OK)
            bytes += info.uncompressed_size;
    }
    unzClose(zip);
    return true;
}

bool extractArchive(const std::string& zipPath, const std::string& folder, ExtractReport& report, std::string& error)
{
    unzFile zip = unzOpen(zipPath.c_str());
    if (!zip)
    {
        error = "cannot open " + zipPath + " as a zip";
        return false;
    }
    std::vector<char> buffer(1 << 16);
    bool ok = true;
    for (int status = unzGoToFirstFile(zip); ok && status == UNZ_OK; status = unzGoToNextFile(zip))
    {
        char name[1024];
        unz_file_info info;
        if (unzGetCurrentFileInfo(zip, &info, name, sizeof(name), 0, 0, 0, 0) != UNZ_OK)
            continue;
        std::string entry = name;
        if (entry.empty() || entry[entry.size() - 1] == '/' || entry[entry.size() - 1] == '\\'
            || entry.find("..") != std::string::npos || entry[0] == '/' || entry[0] == '\\' || entry.find(':') != std::string::npos)
            continue;                               // folders, and no path out of the folder
        const std::string target = folder + "/" + entry;
        if (exists(target))
        {
            ++report.kept;
            continue;
        }
        const size_t slash = target.find_last_of("/\\");
        if (slash != std::string::npos)
            makeFolders(target.substr(0, slash));
        if (unzOpenCurrentFile(zip) != UNZ_OK)
        {
            error = "cannot read " + entry + " from " + zipPath;
            ok = false;
            break;
        }
        std::FILE* out = std::fopen(target.c_str(), "wb");
        if (!out)
        {
            unzCloseCurrentFile(zip);
            error = "cannot write " + target;
            ok = false;
            break;
        }
        int got = 0;
        unsigned long long written = 0;
        while ((got = unzReadCurrentFile(zip, &buffer[0], static_cast<unsigned>(buffer.size()))) > 0)
        {
            if (std::fwrite(&buffer[0], 1, static_cast<size_t>(got), out) != static_cast<size_t>(got))
            {
                got = -1;
                break;
            }
            written += static_cast<unsigned long long>(got);
        }
        const bool closed = std::fclose(out) == 0;
        const bool crcOk = unzCloseCurrentFile(zip) == UNZ_OK;
        if (got < 0 || !closed || !crcOk)
        {
            DeleteFileA(target.c_str());
            error = "extracting " + entry + " from " + zipPath + " failed";
            ok = false;
            break;
        }
        ++report.files;
        report.bytes += written;
    }
    unzClose(zip);
    return ok;
}

bool packObjectArchives(const std::string& objects, const std::string& destination, ObjectPackReport& report,
                        std::string& error)
{
    report = ObjectPackReport();
    std::vector<LooseFile> found;
    listFiles(objects, std::string(), found);
    if (found.empty())
    {
        error = "no files under " + objects;
        return false;
    }
    std::sort(found.begin(), found.end(), byPath);
    std::vector<const LooseFile*> client, server;
    for (size_t i = 0; i < found.size(); ++i)
    {
        const bool toClient = objectArchiveTakes(found[i].relative, false);
        const bool toServer = objectArchiveTakes(found[i].relative, true);
        if (toClient)
            client.push_back(&found[i]);
        if (toServer)
            server.push_back(&found[i]);
        if (!toClient && !toServer)
            ++report.left;
    }
    makeFolders(destination);
    const std::string clientZip = destination + "/Objects_client.zip";
    const std::string serverZip = destination + "/Objects_server.zip";
    // Both written whole beside their final names first, then moved over them.
    if (!packInto(objects, client, clientZip + ".tmp", report.clientBytes, error)
        || !packInto(objects, server, serverZip + ".tmp", report.serverBytes, error))
    {
        DeleteFileA((clientZip + ".tmp").c_str());
        DeleteFileA((serverZip + ".tmp").c_str());
        return false;
    }
    if (!MoveFileExA((clientZip + ".tmp").c_str(), clientZip.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)
        || !MoveFileExA((serverZip + ".tmp").c_str(), serverZip.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        error = "cannot put the archives in place in " + destination;
        return false;
    }
    report.clientFiles = static_cast<int>(client.size());
    report.serverFiles = static_cast<int>(server.size());
    return true;
}

} // namespace bf2editor
