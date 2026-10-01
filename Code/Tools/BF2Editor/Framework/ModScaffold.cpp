// Code/Tools/BF2Editor/Framework/ModScaffold.cpp - see ModScaffold.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/ModScaffold.h"

#include "Framework/ObjectLibrary.h"

#include <windows.h>

#include <algorithm>
#include <cctype>

namespace bf2editor
{

namespace
{

const char* const kNewline = "\r\n";

void line(std::string& text, const std::string& content)
{
    text += content + kNewline;
}

// A little-endian 32-bit number, then the string's bytes (Ingame.mep's records).
void lengthPrefixed(std::string& out, const std::string& text)
{
    const unsigned n = static_cast<unsigned>(text.size());
    out += static_cast<char>(n & 0xFF);
    out += static_cast<char>((n >> 8) & 0xFF);
    out += static_cast<char>((n >> 16) & 0xFF);
    out += static_cast<char>((n >> 24) & 0xFF);
    out += text;
}

bool noCaseLess(const std::string& a, const std::string& b)
{
    return _stricmp(a.c_str(), b.c_str()) < 0;
}

} // namespace

std::vector<std::pair<std::string, std::string> > modScaffoldFiles(const std::string& name,
                                                                     const std::vector<std::string>& baseMods)
{
    std::vector<std::pair<std::string, std::string> > files;

    std::string client;
    line(client, "fileManager.mountArchive Objects_client.zip Objects");
    line(client, "fileManager.mountArchive Common_client.zip Common");
    line(client, "fileManager.mountArchive Menu_client.zip Menu");
    line(client, "fileManager.mountArchive Fonts_client.zip Fonts");
    for (size_t i = 0; i < baseMods.size(); ++i)
    {
        client += kNewline;
        line(client, "fileManager.mountArchive mods/" + baseMods[i] + "/Objects_client.zip Objects");
        line(client, "fileManager.mountArchive mods/" + baseMods[i] + "/Common_client.zip Common");
        line(client, "fileManager.mountArchive mods/" + baseMods[i] + "/Menu_client.zip Menu");
        line(client, "fileManager.mountArchive mods/" + baseMods[i] + "/Fonts_client.zip Fonts");
        if (baseMods[i] == "bf2")
            line(client, "fileManager.mountArchive mods/" + baseMods[i] + "/Shaders_client.zip Shaders");
    }
    files.push_back(std::make_pair(std::string("ClientArchives.con"), client));

    std::string server;
    line(server, "fileManager.mountArchive Objects_server.zip Objects");
    line(server, "fileManager.mountArchive Menu_server.zip Menu");
    line(server, "fileManager.mountArchive Common_server.zip Common");
    for (size_t i = 0; i < baseMods.size(); ++i)
    {
        server += kNewline;
        line(server, "fileManager.mountArchive mods/" + baseMods[i] + "/Objects_server.zip Objects");
        line(server, "fileManager.mountArchive mods/" + baseMods[i] + "/Menu_server.zip Menu");
        line(server, "fileManager.mountArchive mods/" + baseMods[i] + "/Common_server.zip Common");
    }
    files.push_back(std::make_pair(std::string("ServerArchives.con"), server));

    std::string gameLogic;
    line(gameLogic, "rem *** Put non level specific initializations here ***");
    gameLogic += kNewline;
    gameLogic += kNewline;
    line(gameLogic, "rem *** Load multiplayer free camera here");
    line(gameLogic, "run Common/FreeCamera/Object.con");
    gameLogic += kNewline;
    line(gameLogic, "gameLogic.setDefaultNumberOfTickets 1 250");
    line(gameLogic, "gameLogic.setDefaultNumberOfTickets 2 250");
    gameLogic += kNewline;
    line(gameLogic, "gameLogic.enableVoHelp 1");
    line(gameLogic, "gameLogic.supplyDropHeight 50");
    line(gameLogic, "gameLogic.supplyDropNumSecsToLive 300");
    files.push_back(std::make_pair(std::string("GameLogicInit.con"), gameLogic));

    std::string init;
    line(init, "rem ** gameLogic hasn't been initialized yet **");
    init += kNewline;
    init += kNewline;
    init += kNewline;
    line(init, "rem ** Intro Movies **");
    line(init, "bf2Engine.playMovie mods\\bf2\\Movies\\EA.bik 1");
    line(init, "bf2Engine.playMovie mods\\bf2\\Movies\\dice.bik 1");
    line(init, "bf2Engine.playMovie mods\\bf2\\Movies\\legal.bik 1");
    line(init, "bf2Engine.playMovie mods\\bf2\\Movies\\intro.bik 0");
    line(init, "bf2Engine.playMovie mods\\bf2\\Movies\\welcome.bik 0");
    files.push_back(std::make_pair(std::string("Init.con"), init));

    // The .mew as the retail writer's fputc sequence makes it (bf2's BF2.mew is the same 34 bytes).
    static const unsigned char mewHead[] = { 2, 0, 0, 0, '.', '/', 0, 0, 0, 0, 6, 0, 0, 0 };
    static const unsigned char mewMiddle[] = { 0, 0, 0, 0, 6, 0, 0, 0 };
    std::string mew(reinterpret_cast<const char*>(mewHead), sizeof(mewHead));
    mew += "Ingame";
    mew.append(reinterpret_cast<const char*>(mewMiddle), sizeof(mewMiddle));
    mew += "Ingame";
    files.push_back(std::make_pair(name + ".mew", mew));

    std::string mep;
    lengthPrefixed(mep, "../Content/Menu/");
    lengthPrefixed(mep, "../Content/Menu/HUD/");
    lengthPrefixed(mep, "../Content/Menu/HUD/Texture/");
    files.push_back(std::make_pair(std::string("Ingame.mep"), mep));

    std::string desc;
    line(desc, "<mod>");
    line(desc, "\t<title> " + name + " </title>");
    line(desc, "\t<desc> " + name + " </desc>");
    line(desc, "\t<url> </url>");
    line(desc, "\t<version> BF2 1.5 </version>");
    line(desc, "</mod>");
    files.push_back(std::make_pair(std::string("Mod.desc"), desc));
    return files;
}

bool validModName(const std::string& name)
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

bool createMod(const std::string& modsFolder, const std::string& name, const std::vector<std::string>& baseMods,
               std::string& error)
{
    if (!validModName(name))
    {
        error = "'" + name + "' is not a mod name (letters, digits, _ and -)";
        return false;
    }
    const std::string folder = modsFolder + "/" + name;
    if (GetFileAttributesA(folder.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        error = folder + " exists already";
        return false;
    }
    const std::vector<std::pair<std::string, std::string> > files = modScaffoldFiles(name, baseMods);
    for (size_t i = 0; i < files.size(); ++i)
        if (!ObjectLibrary::writeTextFile(folder + "/" + files[i].first, files[i].second, error))
            return false;
    return true;
}

std::vector<std::string> listMods(const std::string& modsFolder)
{
    std::vector<std::string> mods;
    WIN32_FIND_DATAA found;
    HANDLE h = FindFirstFileA((modsFolder + "/*").c_str(), &found);
    if (h == INVALID_HANDLE_VALUE)
        return mods;
    do
    {
        const std::string name = found.cFileName;
        if (name == "." || name == ".." || !(found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            continue;
        if (GetFileAttributesA((modsFolder + "/" + name + "/Mod.desc").c_str()) != INVALID_FILE_ATTRIBUTES)
            mods.push_back(name);
    } while (FindNextFileA(h, &found));
    FindClose(h);
    std::sort(mods.begin(), mods.end(), noCaseLess);
    return mods;
}

} // namespace bf2editor
