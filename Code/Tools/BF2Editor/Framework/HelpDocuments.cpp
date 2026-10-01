// Code/Tools/BF2Editor/Framework/HelpDocuments.cpp - see HelpDocuments.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/HelpDocuments.h"

#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>

namespace bf2editor
{

namespace
{

std::string lowerText(std::string text)
{
    for (size_t i = 0; i < text.size(); ++i)
        if (text[i] >= 'A' && text[i] <= 'Z')
            text[i] = static_cast<char>(text[i] - 'A' + 'a');
    return text;
}

bool isFile(const std::string& path)
{
    const DWORD attributes = GetFileAttributesA(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

// The *.doc files of a folder, by name.
std::vector<std::string> documentsIn(const std::string& folder)
{
    std::vector<std::string> names;
    WIN32_FIND_DATAA found;
    HANDLE h = FindFirstFileA((folder + "/*.doc").c_str(), &found);
    if (h == INVALID_HANDLE_VALUE)
        return names;
    do
    {
        if (!(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            names.push_back(found.cFileName);
    } while (FindNextFileA(h, &found));
    FindClose(h);
    std::sort(names.begin(), names.end());
    return names;
}

void appendUtf8(std::string& out, unsigned code)
{
    if (code < 0x80)
        out += static_cast<char>(code);
    else if (code < 0x800)
    {
        out += static_cast<char>(0xC0 | (code >> 6));
        out += static_cast<char>(0x80 | (code & 0x3F));
    }
    else if (code < 0x10000)
    {
        out += static_cast<char>(0xE0 | (code >> 12));
        out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (code & 0x3F));
    }
    else
    {
        out += static_cast<char>(0xF0 | (code >> 18));
        out += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
        out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        out += static_cast<char>(0x80 | (code & 0x3F));
    }
}

unsigned readU32(const std::string& bytes, size_t at)
{
    return static_cast<unsigned char>(bytes[at]) | (static_cast<unsigned char>(bytes[at + 1]) << 8)
           | (static_cast<unsigned char>(bytes[at + 2]) << 16) | (static_cast<unsigned>(static_cast<unsigned char>(bytes[at + 3])) << 24);
}

} // namespace

std::vector<HelpDocument> helpDocuments(const std::string& helpFolder)
{
    std::vector<HelpDocument> documents;
    const struct { const char* label; const char* file; } fixed[] = {
        { "User guide", "UserGuide.doc" },
        { "Command list", "CommandDescriptions.xls" },
        { "Object editor help", "ObjectEditor_Help.xls" },
    };
    for (size_t i = 0; i < sizeof(fixed) / sizeof(fixed[0]); ++i)
    {
        const std::string path = helpFolder + "/" + fixed[i].file;
        if (!isFile(path))
            continue;
        HelpDocument document;
        document.label = fixed[i].label;
        document.path = path;
        documents.push_back(document);
    }
    const struct { const char* kind; const char* folder; } groups[] = { { "Workshop", "Workshop" }, { "Tutorial", "Tutorial" } };
    for (size_t g = 0; g < sizeof(groups) / sizeof(groups[0]); ++g)
    {
        const std::string folder = helpFolder + "/" + groups[g].folder;
        const std::vector<std::string> names = documentsIn(folder);
        for (size_t i = 0; i < names.size(); ++i)
        {
            HelpDocument document;
            document.label = std::string(groups[g].kind) + ": " + names[i].substr(0, names[i].size() - 4);
            document.path = folder + "/" + names[i];
            documents.push_back(document);
        }
    }
    return documents;
}

const HelpDocument* findHelpDocument(const std::vector<HelpDocument>& documents, const std::string& name)
{
    const std::string wanted = lowerText(name);
    for (size_t i = 0; i < documents.size(); ++i)
    {
        const std::string& path = documents[i].path;
        const size_t slash = path.find_last_of("/\\");
        const std::string file = slash == std::string::npos ? path : path.substr(slash + 1);
        if (lowerText(documents[i].label) == wanted || lowerText(file) == wanted)
            return &documents[i];
    }
    return 0;
}

bool openHelpDocument(const HelpDocument& document, std::string& result)
{
    char full[MAX_PATH];
    if (!GetFullPathNameA(document.path.c_str(), MAX_PATH, full, 0))
    {
        result = "bad path " + document.path;
        return false;
    }
    const HINSTANCE code = ShellExecuteA(0, "open", full, 0, 0, SW_SHOWNORMAL);
    if (reinterpret_cast<INT_PTR>(code) <= 32)
    {
        char text[64];
        std::snprintf(text, sizeof(text), " (shell error %d)", static_cast<int>(reinterpret_cast<INT_PTR>(code)));
        result = "no program opened " + document.path + text;
        return false;
    }
    result = "opened " + document.path;
    return true;
}

bool CommandCatalogue::parse(const std::string& bytes, std::string& error)
{
    m_rows.clear();
    m_byName.clear();
    if (bytes.size() < 8)
    {
        error = "too short for a command catalogue";
        return false;
    }
    const unsigned rows = readU32(bytes, 0);
    const unsigned columns = readU32(bytes, 4);
    if (rows == 0 || columns < 2 || rows > 100000 || columns > 64)
    {
        error = "not a command catalogue (its row and column counts)";
        return false;
    }
    size_t at = 8;
    std::vector<std::string> cells;
    cells.reserve(static_cast<size_t>(rows) * columns);
    for (unsigned cell = 0; cell < rows * columns; ++cell)
    {
        std::string text;
        for (;;)
        {
            if (at + 2 > bytes.size())
            {
                error = "the command catalogue ends inside its table";
                m_rows.clear();
                return false;
            }
            unsigned code = static_cast<unsigned char>(bytes[at]) | (static_cast<unsigned char>(bytes[at + 1]) << 8);
            at += 2;
            if (code == 0)
                break;
            // A surrogate pair is one character; a lone surrogate becomes U+FFFD.
            if (code >= 0xD800 && code < 0xDC00 && at + 2 <= bytes.size())
            {
                const unsigned low = static_cast<unsigned char>(bytes[at]) | (static_cast<unsigned char>(bytes[at + 1]) << 8);
                if (low >= 0xDC00 && low < 0xE000)
                {
                    at += 2;
                    code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                }
                else
                    code = 0xFFFD;
            }
            else if (code >= 0xD800 && code < 0xE000)
                code = 0xFFFD;
            appendUtf8(text, code);
        }
        cells.push_back(text);
    }
    // Row 0 holds the headings.
    for (unsigned row = 1; row < rows; ++row)
    {
        const std::string& name = cells[static_cast<size_t>(row) * columns];
        if (name.empty())
            continue;
        m_byName[lowerText(name)] = m_rows.size();
        m_rows.push_back(std::make_pair(name, cells[static_cast<size_t>(row) * columns + 1]));
    }
    return true;
}

bool CommandCatalogue::load(const std::string& path, std::string& error)
{
    std::ifstream in(path.c_str(), std::ios::in | std::ios::binary);
    if (!in)
    {
        error = "cannot read " + path;
        return false;
    }
    std::ostringstream bytes;
    bytes << in.rdbuf();
    if (!parse(bytes.str(), error))
    {
        error = path + ": " + error;
        return false;
    }
    return true;
}

const std::string* CommandCatalogue::describe(const std::string& command) const
{
    const std::map<std::string, size_t>::const_iterator it = m_byName.find(lowerText(command));
    return it == m_byName.end() ? 0 : &m_rows[it->second].second;
}

} // namespace bf2editor
