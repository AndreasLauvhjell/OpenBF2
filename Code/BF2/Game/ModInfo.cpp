// Mod and map metadata discovery through the mounted file system and XML descriptors.
// [INFERRED] File placement.

#include "ModInfo.h"

#include <cstring>                       // strlen - the ELF's append() calls pass an explicit length
#include <list>

#include "../Core/IBase.h"
#include "../Core/Ids.h"
#include "../Core/SettingsRepostitory.h"
#include "../Core/StringUtil.h"
#include "../IO/File/IFileSystem.h"      // io::FileInfo - findFiles' element type
#include "../IO/File/FileManager.h"      // io::fileManager, FileManager::findFiles
#include "Utility/XMLDocument.h"         // XMLDocument/XMLNode - the two .desc readers

namespace dice {
namespace hfe {

// Creates an uninitialized instance with one caller-owned reference; rejects
// aggregation and unsupported interfaces. Engine startup calls init after the
// file-system search paths are ready.
IBase* ModInfo::create(unsigned int iid, IBase* outer)
{
    if (outer != 0)
        return 0;

    ModInfo* self = new ModInfo();                      // operator new(0x24u)
    void* itf = self->queryInterface(iid);              // virtual - call [edx+0Ch]
    if (itf == 0)
        self->release();                                // virtual - call [eax+08h]
    return static_cast<IBase*>(itf);
}

// Starts with one reference and empty mod storage.
ModInfo::ModInfo()
    : m_refCount(1)
    , m_mods()                  //  / 0x82F98AA / 0x82F98B1 - three zeroed pointers
    , m_initialised(false)      // byte store
    , m_gameModeNames()         //  onwards - _M_header = 0, allocate(24), _M_empty_initialize
{
}

// Member containers own and release all mod metadata.
ModInfo::~ModInfo()
{
}

void ModInfo::addRef()
{
    ++m_refCount;
}

// ModInfo::getRef - ELF 0x82FF730, 11 bytes. Vtable +0x04.
int ModInfo::getRef()
{
    return m_refCount;
}

// Drops a reference and deletes at zero, returning the saved remaining count.
int ModInfo::release()
{
    //  preserves EBX; PE 628BD0 preserves ESI across deletion.
    const int remaining = --m_refCount;

    if (remaining == 0)
        delete this;            // call [eax+58h]

    return remaining;          // : saved value, no freed-member read
}

// Answers IBase and IModInfo interface requests.
void* ModInfo::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)
        return const_cast<ModInfo*>(this);

    if (iid == IID_IModInfo)                                // cmovz
        return const_cast<ModInfo*>(this);

    return 0;
}

// Discovers mods and their own or imported levels using nonrecursive directory scans.
// Returns false when already initialized or when no descriptor was accepted.
// Failed discovery leaves initialization clear and the mod-list accessor unavailable.
bool ModInfo::init()
{
    if (m_initialised)
        return false;

    // 1. every directory under mods/
    std::string modsPath("@ROOT@/");
    modsPath.append("mods/", strlen("mods/"));

    std::list<io::FileInfo> modDirs;
    io::fileManager->findFiles(modsPath, false, modDirs);               // call [edx+34h]

    for (std::list<io::FileInfo>::iterator it = modDirs.begin(); it != modDirs.end(); ++it)
    {
        io::FileInfo entry = *it;                                       //  onwards

        if (!entry.isDirectory)
            continue;

        Mod mod;                                                        // Mod::Mod()
        std::string path("mods/");                                      // 0x82F9F9x
        path += entry.name;
        mod.path = path;                                                // std::string::assign

        if (parseModDesc(&mod))                                         // direct call
            m_mods.push_back(mod);                                      // 0x82F9FCx / _M_insert_aux
    }

    // 2. every level directory of every mod, twice
    for (unsigned int i = 0; i < m_mods.size(); ++i)
    {
        for (int pass = 0; pass <= 1; ++pass)
        {
            Mod& mod = m_mods[i];

            if (pass == 1 && mod.importLevels.empty())                  // 0x82FA0xx
                continue;

            std::string levelsPath("@ROOT@/");
            std::string base(pass == 0 ? mod.path : mod.importLevels);
            base.append("/Levels/", strlen("/Levels/"));
            levelsPath += base;

            // [4] above: built, filled, never read, destroyed at the end of the pass.
            std::vector<std::string> foundLevelDirs;

            std::list<io::FileInfo> levelDirs;
            io::fileManager->findFiles(levelsPath, false, levelDirs);   // call [edx+34h]

            for (std::list<io::FileInfo>::iterator jt = levelDirs.begin();
                 jt != levelDirs.end(); ++jt)
            {
                if (!jt->isDirectory)
                    continue;

                foundLevelDirs.push_back(jt->name);

                Map map;                                                // constructed inline
                map.path = jt->name;                                    // std::string::assign

                if (parseMapDesc(&mod, levelsPath, &map))               // virtual, call [eax+50h]
                    mod.maps.push_back(map);
            }
        }
    }

    if (m_mods.empty())                                                 // 0x82FA6xx
        return false;

    m_initialised = true;                                               // 0x82FA6xx, byte store
    return true;
}

// Read mod.desc into mod; null or a parse failure returns false. Derive the mod
// name from its path and set gulf_of_oman as the single-player startup default
// before reading XML. Tag matching is case-insensitive and later values replace
// earlier values of the same tag.
bool ModInfo::parseModDesc(Mod* mod)
{
    if (!mod)
        return false;

    std::string file("@ROOT@/");
    file += mod->path;
    file += "/";
    file += "mod.desc";

    std::string name(mod->path);
    std::string::size_type i = name.find('/');
    if (i != std::string::npos) { name.erase(0, i + 1); setLowerCase(name); }
    i = name.find('\\');
    if (i != std::string::npos) { name.erase(0, i + 1); setLowerCase(name); }
    mod->name = name;                                                   // 0x82FA8Ax
    mod->spStartupMap = "gulf_of_oman";                                 // 0x82FA8Bx - before the XML

    XMLDocument doc;
    if (!doc.parse(file))
        return false;                       // a mod whose mod.desc will not parse is rejected

    // The document adopts the root element, so nodes are direct children of mod.
    // Preserve ordered first-match tag dispatch.
    for (unsigned int n = 0; n < doc.nodes.size(); ++n)
    {
        const XMLNode* e = doc.nodes[n];
        if      (!compareIgnoreCase(e->name.c_str(), "title"))          mod->title        = e->value;
        else if (!compareIgnoreCase(e->name.c_str(), "url"))            mod->url          = e->value;
        else if (!compareIgnoreCase(e->name.c_str(), "music"))          mod->music        = e->value;
        else if (!compareIgnoreCase(e->name.c_str(), "logo"))           mod->logo         = e->value;
        else if (!compareIgnoreCase(e->name.c_str(), "icon"))           mod->icon         = e->value;
        else if (!compareIgnoreCase(e->name.c_str(), "desc"))           mod->desc         = e->value;
        else if (!compareIgnoreCase(e->name.c_str(), "version"))        mod->version      = e->value;
        else if (!compareIgnoreCase(e->name.c_str(), "importlevels"))   mod->importLevels = e->value;
        else if (!compareIgnoreCase(e->name.c_str(), "sp_startup_map")) mod->spStartupMap = e->value;
    }
    return true;
}

// Loads a level descriptor into the supplied map record. Requires mod and map;
// returns false when the descriptor cannot be accepted, causing discovery to skip it.
bool ModInfo::parseMapDesc(Mod* mod, const std::string& levelsPath, Map* map)
{
    if (!map || !mod)                                       // both, and map first
        return false;

    // Parsing defaults differ from construction: initialize night-map to false and GameSpy ID to -1.
    map->isNightMap = false;                                //   +0x25
    map->gameSpyId  = -1;                                   //   +0x20

    // levelsPath + <dir> + "/Info/" + <dir> + ".desc" - the level's directory name appears twice,
    // e.g. mods/bf2/levels/strike_at_karkand/Info/strike_at_karkand.desc. map->path is already set
    // to that directory by the caller in init().
    const std::string file = levelsPath + map->path + "/Info/" + map->path + ".desc";

    XMLDocument doc;
    if (!doc.parse(file))                                   // a map that will not parse
        return false;                                       //   is dropped by init()

    // Document attribute names are case-sensitive, unlike element names below.
    for (unsigned int i = 0; i < doc.attributes.size(); ++i)
    {
        if (doc.attributes[i]->name.compare("gsid") == 0)
            map->gameSpyId = atoi(doc.getAttribute("gsid").c_str());
        else if (doc.attributes[i]->name.compare("plasma") == 0)
            map->plasma = (atoi(doc.getAttribute("plasma").c_str()) == 1);   // == 1, not != 0
    }

    // the elements under the root
    for (unsigned int j = 0; j < doc.nodes.size(); ++j)
    {
        const XMLNode* node = doc.nodes[j];

        if (!compareIgnoreCase(node->name.c_str(), "name"))
        {
            map->name.assign(node->value);
        }
        else if (!compareIgnoreCase(node->name.c_str(), "briefing"))
        {
            map->briefing.assign(node->value);
            map->briefingLocId.assign(node->getAttribute("locid"));
        }
        else if (!compareIgnoreCase(node->name.c_str(), "music"))
        {
            map->music.assign(node->value);
        }
        else if (!compareIgnoreCase(node->name.c_str(), "isnightmap"))
        {
            // Accept true and yes case-insensitively; otherwise parse the value numerically.
            if (compareIgnoreCase(node->value.c_str(), "true")
                && compareIgnoreCase(node->value.c_str(), "yes"))
                map->isNightMap = (atoi(node->value.c_str()) != 0);
            else
                map->isNightMap = true;
        }
        else if (!compareIgnoreCase(node->name.c_str(), "modes"))
        {
            for (unsigned int k = 0; k < node->nodes.size(); ++k)
            {
                const XMLNode* modeNode = node->nodes[k];
                if (compareIgnoreCase(modeNode->name.c_str(), "mode"))
                    continue;

                GameMode gameMode;
                gameMode.name.assign(modeNode->getAttribute("type"));

                // Accumulate distinct mode names across the mod, matching case-insensitively.
                bool known = false;
                for (unsigned int m = 0; m < mod->gameModes.size(); ++m)
                {
                    if (!compareIgnoreCase(mod->gameModes[m].c_str(), gameMode.name.c_str()))
                    {
                        known = true;
                        break;
                    }
                }
                if (!known)
                    mod->gameModes.push_back(gameMode.name);

                // Deduplication applies only to the mod-wide list; always append the map's own mode and variants.
                for (unsigned int n = 0; n < modeNode->nodes.size(); ++n)
                {
                    const XMLNode* typeNode = modeNode->nodes[n];
                    if (compareIgnoreCase(typeNode->name.c_str(), "maptype"))
                        continue;

                    MapType mapType;
                    mapType.players = atoi(typeNode->getAttribute("players").c_str());
                    mapType.type.assign(typeNode->getAttribute("type"));
                    mapType.locId.assign(typeNode->getAttribute("locid"));

                    // Missing and empty ai attributes leave the default false; a nonempty value is converted separately.
                    if (typeNode->getAttribute("ai").compare("") != 0)
                        mapType.ai = (atoi(typeNode->getAttribute("ai").c_str()) != 0);
                    else
                        mapType.ai = false;

                    mapType.description.assign(typeNode->value);
                    gameMode.mapTypes.push_back(mapType);
                }

                map->gameModes.push_back(gameMode);
            }
        }
    }
    return true;
}

// Returns the mod list only after successful initialization. [UNRESOLVED] Exact
// original return type; a mutable vector pointer is [CHOSEN].
std::vector<IModInfo::Mod>* ModInfo::getModList()
{
    if (m_initialised)
        return &m_mods;

    return 0;
}

// Finds a map in the current mod. Requires a valid current mod; unlike findMapFromCurrentMod, it does not null-check.
const IModInfo::Map* ModInfo::getMap(const std::string& map) const
{
    const Mod* mod = getCurrentMod();                       // virtual

    for (unsigned int i = 0; i < mod->maps.size(); ++i)
    {
        if (compareIgnoreCase(map, mod->maps[i].path) == 0) // 0x82FBC2x, string overload
            return &mod->maps[i];
    }

    return 0;
}

// Resolves GSModDirectory on every call, defaulting to mods/bf2, and scans mod
// paths case-insensitively. The current mod is not cached between calls.
const IModInfo::Mod* ModInfo::getCurrentMod() const
{
    std::string current = settings->stringGet("GSModDirectory", "mods/bf2");   // 0x82FBCAx, +0x40

    for (unsigned int i = 0; i < m_mods.size(); ++i)        // 0x82FBCCx
    {
        // c_str() on both operands, then the (const char*, const char*) overload - see the section
        // note above.
        if (compareIgnoreCase(m_mods[i].path.c_str(), current.c_str()) == 0)   // 0x82FBD0x
            return const_cast<Mod*>(&m_mods[i]);
    }

    return 0;
}

// Returns the configured mod directory without checking discovery state or filesystem existence.
std::string ModInfo::getCurrentModPath()
{
    return settings->stringGet("GSModDirectory", "mods/bf2");   // 0x82FBEAx, virtual +0x40
}

// ModInfo::findMapFromCurrentMod - ELF 0x82FC110, 64 bytes. Vtable +0x24.
// Both calls are virtual (call [eax+1Ch], then call [eax+28h]). Unlike getMap, this one checks.
const IModInfo::Map* ModInfo::findMapFromCurrentMod(const std::string& map)
{
    const Mod* mod = getCurrentMod();           // virtual

    if (mod == 0)
        return 0;

    return findMapFromMod(mod, map);            // 0x82FC13x, virtual
}

// ModInfo::findMapFromMod - ELF 0x82FC150, 176 bytes. Vtable +0x28.
// Searches a given mod's maps on Map::path and returns the Map. The null guard is real (0x82FC157).
const IModInfo::Map* ModInfo::findMapFromMod(const Mod* mod, const std::string& map)
{
    if (mod == 0)
        return 0;

    for (unsigned int i = 0; i < mod->maps.size(); ++i)                 // 0x82FC16x
    {
        if (compareIgnoreCase(mod->maps[i].path.c_str(), map.c_str()) == 0)
            return &mod->maps[i];                                       // 0x82FC1Cx
    }

    return 0;
}

// ModInfo::getModFromName - ELF 0x82FC280, 160 bytes. Vtable +0x2C. Searches on Mod::name (+0x04).
const IModInfo::Mod* ModInfo::getModFromName(const std::string& name)
{
    for (unsigned int i = 0; i < m_mods.size(); ++i)                    // 0x82FC28x
    {
        if (compareIgnoreCase(m_mods[i].name.c_str(), name.c_str()) == 0)   // 0x82FC2Ex
            return &m_mods[i];
    }

    return 0;
}

// Finds a mod by its full directory path.
const IModInfo::Mod* ModInfo::getModFromPath(const std::string& path)
{
    for (unsigned int i = 0; i < m_mods.size(); ++i)                    // 0x82FC32x
    {
        if (compareIgnoreCase(m_mods[i].path.c_str(), path.c_str()) == 0)   // 0x82FC38x
            return &m_mods[i];
    }

    return 0;
}

// Returns the first map with the requested GameSpy ID across all mods. IDs need not be globally unique.
const IModInfo::Map* ModInfo::getMapFromGameSpyId(int id)
{
    for (unsigned int i = 0; i < m_mods.size(); ++i)                    // 0x82FC21x
    {
        for (unsigned int j = 0; j < m_mods[i].maps.size(); ++j)        // 0x82FC22x
        {
            if (m_mods[i].maps[j].gameSpyId == id)                      // 0x82FC24x
                return &m_mods[i].maps[j];
        }
    }

    return 0;
}

// Converts map paths and names within the selected mod. Missing matches or a null
// mod return the argument unchanged; the one-argument overload resolves the mod
// through virtual calls.
std::string ModInfo::getMapNameFromMap(const std::string& map)
{
    return getMapNameFromMap(getCurrentMod(), map);      // 0x82FBEFx / 0x82FBF0x, both virtual
}

// ModInfo::getMapPathFromMap(const std::string&) - ELF 0x82FBF30, 80 bytes. Vtable +0x3C.
// The same shape, delegating to [eax+44h].
std::string ModInfo::getMapPathFromMap(const std::string& map)
{
    return getMapPathFromMap(getCurrentMod(), map);      // 0x82FBF4x / 0x82FBF5x, both virtual
}

// ModInfo::getMapNameFromMap(const Mod*, const std::string&) - ELF 0x82FBF80, 192 bytes. Vtable +0x40.
// Searches mod->maps on `path` (+0x00) and returns `name` (+0x04).
std::string ModInfo::getMapNameFromMap(const Mod* mod, const std::string& map)
{
    if (mod != 0)                                                       // 0x82FBF8x
    {
        for (unsigned int i = 0; i < mod->maps.size(); ++i)             // 0x82FBF9x
        {
            if (compareIgnoreCase(mod->maps[i].path.c_str(), map.c_str()) == 0)
                return mod->maps[i].name;                               // 0x82FC00x, returns +0x04
        }
    }

    return map;                                                         // the unchanged argument
}

// ModInfo::getMapPathFromMap(const Mod*, const std::string&) - ELF 0x82FC040, 208 bytes. Vtable +0x44.
// The mirror: searches on `name` (+0x04) and returns `path` (+0x00).
std::string ModInfo::getMapPathFromMap(const Mod* mod, const std::string& map)
{
    if (mod != 0)                                                       // 0x82FC04x
    {
        for (unsigned int i = 0; i < mod->maps.size(); ++i)             // 0x82FC06x
        {
            if (compareIgnoreCase(mod->maps[i].name.c_str(), map.c_str()) == 0)
                return mod->maps[i].path;                               // 0x82FC0Dx, returns +0x00
        }
    }

    return map;                                                         // the unchanged argument
}

// Stores a game-mode display name with the key's case unchanged. The getter lowercases
// its lookup key, so callers should supply lowercase keys. [UNRESOLVED] Original
// return type; void is [CHOSEN].
void ModInfo::addNameForGameMode(const std::string& gameMode, const std::string& name)
{
    m_gameModeNames[gameMode] = name;       //  lower_bound ... 0x82FC56x assign
}

// Returns the display name for a game mode, or an empty string when absent.
std::string ModInfo::getNameFromGameMode(const std::string& gameMode)
{
    std::string key(gameMode);                                          // 0x82FC58x
    setLowerCase(key);                                                  // see the note

    std::map<std::string, std::string>::const_iterator it = m_gameModeNames.find(key);

    if (it == m_gameModeNames.end())                                    // 0x82FC5Cx
        return std::string("");                                         // 0x82FC5Dx, the "" literal

    return it->second;                                                  // node + 20
}

} // namespace hfe
} // namespace dice
