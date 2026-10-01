// Code/Tools/BF2Editor/Framework/GameplayLayers.cpp - see GameplayLayers.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/GameplayLayers.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>

namespace bf2editor
{

namespace
{

const char kSpawnerBanner[] = "rem ********** Object Spawner **********";
const char kSpawnPointBanner[] = "rem ********** Spawn Points **********";
const char kControlPointBanner[] = "rem ********** Control Points **********";
const char kHost[] = "if v_arg1 == host";
const char kEndIf[] = "endIf";

std::string lower(std::string text)
{
    for (size_t i = 0; i < text.size(); ++i)
        if (text[i] >= 'A' && text[i] <= 'Z')
            text[i] = static_cast<char>(text[i] - 'A' + 'a');
    return text;
}

std::string trimmed(const std::string& text)
{
    const size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return std::string();
    return text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1);
}

bool blank(const std::string& line)
{
    return trimmed(line).empty();
}

std::vector<std::string> wordsOf(const std::string& text)
{
    std::vector<std::string> words;
    size_t i = 0;
    while (i < text.size())
    {
        while (i < text.size() && (text[i] == ' ' || text[i] == '\t'))
            ++i;
        size_t j = i;
        while (j < text.size() && text[j] != ' ' && text[j] != '\t')
            ++j;
        if (j > i)
            words.push_back(text.substr(i, j - i));
        i = j;
    }
    return words;
}

// "rem [<kind>: <name>]" -> name ("" when the line is no such banner).
std::string bannerName(const std::string& line, const char* kind)
{
    const std::string text = trimmed(line);
    const std::string head = std::string("rem [") + kind + ": ";
    if (text.size() <= head.size() + 1 || _strnicmp(text.c_str(), head.c_str(), head.size()) != 0 || text[text.size() - 1] != ']')
        return std::string();
    return text.substr(head.size(), text.size() - head.size() - 1);
}

std::string unquoted(const std::string& value)
{
    if (value.size() >= 2 && value[0] == '"' && value[value.size() - 1] == '"')
        return value.substr(1, value.size() - 2);
    return value;
}

int layerOf(const std::vector<std::string>& lines, bool area)
{
    const char* key = area ? "CombatArea.layer" : "Object.layer";
    for (size_t i = 0; i < lines.size(); ++i)
    {
        const std::vector<std::string> words = wordsOf(lines[i]);
        if (words.size() >= 2 && _stricmp(words[0].c_str(), key) == 0)
            return std::atoi(words[1].c_str());
    }
    return 1;
}

std::string bucketName(const std::string& mode, const std::string& players)
{
    return mode + " " + players;
}

} // namespace

// Filters and Layer.con

GameplayFilters GameplayFilters::defaults()
{
    GameplayFilters f;
    const char* const modes[6][2] = { { "gpm_cq", "Conquest" }, { "gpm_sl", "SupplyLine" }, { "gpm_coop", "Coop" },
                                      { "sp1", "SP1" }, { "sp2", "SP2" }, { "sp3", "SP3" } };
    for (int i = 0; i < 6; ++i)
        f.addMode(modes[i][0], modes[i][1]);
    f.addPlayers("16");
    f.addPlayers("32");
    f.addPlayers("64");
    return f;
}

void GameplayFilters::addMode(const std::string& id, const std::string& label)
{
    for (size_t i = 0; i < modes.size(); ++i)
        if (_stricmp(modes[i].c_str(), id.c_str()) == 0)
            return;
    modes.push_back(id);
    modeLabels.push_back(label.empty() ? id : label);
}

void GameplayFilters::addPlayers(const std::string& count)
{
    if (std::find(players.begin(), players.end(), count) == players.end())
        players.push_back(count);
}

bool GameplayFilters::parse(const std::string& text)
{
    GameplayFilters f;
    size_t start = 0;
    while (start < text.size())
    {
        size_t end = text.find('\n', start);
        if (end == std::string::npos)
            end = text.size();
        const std::vector<std::string> words = wordsOf(trimmed(text.substr(start, end - start)));
        start = end + 1;
        if (words.size() >= 2 && _stricmp(words[0].c_str(), "LayerManager.CreateDefaultFilter") == 0)
            f.addMode(unquoted(words[1]), words.size() > 2 ? unquoted(words[2]) : std::string());
        else if (words.size() >= 2 && _stricmp(words[0].c_str(), "LayerManager.CreateDefaultPlayerFilter") == 0)
            f.addPlayers(unquoted(words[1]));
    }
    if (f.modes.empty() && f.players.empty())
        return false;
    *this = f;
    return true;
}

bool GameplayLayer::takes(const std::string& mode, const std::string& count) const
{
    bool modeHit = false, playersHit = false;
    for (size_t i = 0; i < modes.size() && !modeHit; ++i)
        modeHit = _stricmp(modes[i].c_str(), mode.c_str()) == 0;
    for (size_t i = 0; i < players.size() && !playersHit; ++i)
        playersHit = players[i] == count;
    return modeHit && playersHit;
}

bool parseLayerFile(const std::string& text, std::vector<GameplayLayer>& layers, std::string& error)
{
    layers.clear();
    GameplayLayer* current = 0;
    size_t start = 0;
    int lineNumber = 0;
    while (start < text.size())
    {
        size_t end = text.find('\n', start);
        if (end == std::string::npos)
            end = text.size();
        const std::string line = trimmed(text.substr(start, end - start));
        start = end + 1;
        ++lineNumber;
        if (line.empty() || _strnicmp(line.c_str(), "rem", 3) == 0)
            continue;
        // AddUserLayer takes a quoted name with spaces: the words after it.
        if (_strnicmp(line.c_str(), "LayerManager.AddUserLayer", 25) == 0)
        {
            const size_t open = line.find('"');
            const size_t close = open == std::string::npos ? std::string::npos : line.find('"', open + 1);
            const std::vector<std::string> rest = wordsOf(close == std::string::npos ? std::string() : line.substr(close + 1));
            if (close == std::string::npos || rest.size() < 3)
            {
                char text2[96];
                std::snprintf(text2, sizeof(text2), "Layer.con line %d: AddUserLayer \"<name>\" <mask> <id> <visible> <no filters>",
                              lineNumber);
                error = text2;
                return false;
            }
            GameplayLayer layer;
            layer.name = line.substr(open + 1, close - open - 1);
            layer.id = std::atoi(rest[1].c_str());
            layer.visible = rest[2] != "0";
            layers.push_back(layer);
            current = &layers.back();
            continue;
        }
        const std::vector<std::string> words = wordsOf(line);
        if (!current || words.size() < 2)
            continue;       // the retail loader skips what it cannot place
        if (_stricmp(words[0].c_str(), "LayerManager.SetLocked") == 0)
            current->locked = words[1] != "0";
        else if (_stricmp(words[0].c_str(), "LayerManager.SetDisabled") == 0)
            current->disabled = words[1] != "0";
        else if (_stricmp(words[0].c_str(), "LayerManager.AddGameModeFilter") == 0)
            current->modes.push_back(unquoted(words[1]));
        else if (_stricmp(words[0].c_str(), "LayerManager.AddPlayerFilter") == 0)
            current->players.push_back(unquoted(words[1]));
    }
    return true;
}

std::string renderLayerFile(const std::vector<GameplayLayer>& layers)
{
    std::vector<const GameplayLayer*> ordered;
    for (size_t i = 0; i < layers.size(); ++i)
        if (layers[i].id != 1)
            ordered.push_back(&layers[i]);
    std::sort(ordered.begin(), ordered.end(), [](const GameplayLayer* a, const GameplayLayer* b) { return a->id < b->id; });
    std::string out;
    for (size_t i = 0; i < ordered.size(); ++i)
    {
        const GameplayLayer& l = *ordered[i];
        char head[160];
        out += "rem ******** " + l.name + " ********\r\n";
        std::snprintf(head, sizeof(head), " %u %d %d 1\r\n", 1u << (l.id & 31), l.id, l.visible ? 1 : 0);
        out += "LayerManager.AddUserLayer \"" + l.name + "\"" + head;
        out += std::string("LayerManager.SetLocked ") + (l.locked ? "1" : "0") + "\r\n";
        out += std::string("LayerManager.SetDisabled ") + (l.disabled ? "1" : "0") + "\r\n";
        for (size_t k = 0; k < l.modes.size(); ++k)
            out += "LayerManager.AddGameModeFilter \"" + l.modes[k] + "\"\r\n";
        for (size_t k = 0; k < l.players.size(); ++k)
            out += "LayerManager.AddPlayerFilter \"" + l.players[k] + "\"\r\n";
        out += "\r\n";
    }
    return out;
}

// The gameplay file

int GameplaySet::find(GameplaySection section, const std::string& name) const
{
    for (size_t i = 0; i < m_objects.size(); ++i)
        if (m_objects[i].section == section && _stricmp(m_objects[i].name.c_str(), name.c_str()) == 0)
            return static_cast<int>(i);
    return -1;
}

bool GameplaySet::parse(const std::string& text, std::string& error)
{
    m_objects.clear();
    m_manager.clear();
    m_preamble.clear();
    for (int s = 0; s < 4; ++s)
    {
        templateOrder[s].clear();
        instanceOrder[s].clear();
    }
    newline = text.find("\r\n") != std::string::npos || text.empty() ? "\r\n" : "\n";
    std::vector<std::string> lines;
    size_t start = 0;
    while (start < text.size())
    {
        size_t end = text.find('\n', start);
        if (end == std::string::npos)
            end = text.size();
        std::string line = text.substr(start, end - start);
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        lines.push_back(line);
        start = end + 1;
    }
    size_t i = 0;
    char where[64];
    const auto fail = [&](const char* what) {
        std::snprintf(where, sizeof(where), " (line %u)", static_cast<unsigned>(i + 1));
        error = std::string(what) + where;
        return false;
    };
    const auto expect = [&](const char* line) {
        if (i >= lines.size() || trimmed(lines[i]) != line)
            return false;
        ++i;
        return true;
    };
    // A block: its lines up to the next blank line (which it consumes).
    const auto block = [&](std::vector<std::string>& out) {
        while (i < lines.size() && !blank(lines[i]))
            out.push_back(lines[i++]);
        if (i < lines.size())
            ++i;
    };
    const auto object = [&](GameplaySection section, const std::string& name) {
        int at = find(section, name);
        if (at < 0)
        {
            GameplayObject o;
            o.section = section;
            o.name = name;
            m_objects.push_back(o);
            at = static_cast<int>(m_objects.size()) - 1;
        }
        return at;
    };
    const char* const templateKind[3] = { "ObjectSpawnerTemplate", "SpawnPointTemplate", "ControlPointTemplate" };
    const char* const instanceKind[3] = { "ObjectSpawner", "SpawnPoint", "ControlPoint" };

    // Lines a level keeps above the sections (Gulf_of_Oman's gameLogic.setTeamDropVehicle), as they are.
    while (i < lines.size() && trimmed(lines[i]) != kSpawnerBanner)
        m_preamble.push_back(lines[i++]);
    // Object spawners: templates, then their instances inside the host block.
    if (!expect(kSpawnerBanner))
        return fail("no object spawner banner");
    for (int s = 0; s < 3; s += 2)
    {
        const GameplaySection section = static_cast<GameplaySection>(s);
        if (s == 2 && !expect(kControlPointBanner))
            return fail("no control point banner");
        while (i < lines.size() && trimmed(lines[i]) != kHost)
        {
            const std::string name = bannerName(lines[i], templateKind[s]);
            if (name.empty())
                return fail("a template block without its banner");
            const int at = object(section, name);
            block(m_objects[static_cast<size_t>(at)].templateLines);
            templateOrder[s].push_back(at);
        }
        if (!expect(kHost) || i >= lines.size() || !blank(lines[i]))
            return fail("no host block");
        ++i;
        while (i < lines.size() && trimmed(lines[i]) != kEndIf)
        {
            const std::string name = bannerName(lines[i], instanceKind[s]);
            if (name.empty())
                return fail("an instance block without its banner");
            const int at = object(section, name);
            block(m_objects[static_cast<size_t>(at)].instanceLines);
            instanceOrder[s].push_back(at);
        }
        if (!expect(kEndIf) || i >= lines.size() || !blank(lines[i]))
            return fail("no end of the host block");
        ++i;
        // Spawn points: template and instance pairs, between the spawner and control point sections.
        if (s == 0)
        {
            if (!expect(kSpawnPointBanner))
                return fail("no spawn point banner");
            while (i < lines.size() && trimmed(lines[i]) != kControlPointBanner)
            {
                const std::string templateName = bannerName(lines[i], templateKind[1]);
                if (!templateName.empty())
                {
                    const int at = object(GameplaySection::SpawnPoint, templateName);
                    block(m_objects[static_cast<size_t>(at)].templateLines);
                    templateOrder[1].push_back(at);
                    continue;
                }
                const std::string instanceName = bannerName(lines[i], instanceKind[1]);
                if (instanceName.empty())
                    return fail("a spawn point block without its banner");
                const int at = object(GameplaySection::SpawnPoint, instanceName);
                block(m_objects[static_cast<size_t>(at)].instanceLines);
                instanceOrder[1].push_back(at);
            }
        }
    }
    // The combat-area manager's lines, then the areas.
    while (i < lines.size() && _strnicmp(trimmed(lines[i]).c_str(), "CombatAreaManager.", 18) == 0)
        m_manager.push_back(lines[i++]);
    while (i < lines.size())
    {
        if (blank(lines[i]) && i + 1 == lines.size())
            break;      // the final line end
        const std::vector<std::string> words = wordsOf(lines[i]);
        if (words.size() < 2 || _stricmp(words[0].c_str(), "CombatArea.create") != 0)
            return fail("a line that is no combat area");
        const int at = object(GameplaySection::CombatArea, words[1]);
        block(m_objects[static_cast<size_t>(at)].instanceLines);
        templateOrder[3].push_back(at);
        instanceOrder[3].push_back(at);
    }
    for (size_t k = 0; k < m_objects.size(); ++k)
        m_objects[k].layer = layerOf(m_objects[k].instanceLines, m_objects[k].section == GameplaySection::CombatArea);
    return true;
}

std::string GameplaySet::render(const std::set<int>& include, bool all) const
{
    const auto in = [&](int index) {
        return all || include.count(m_objects[static_cast<size_t>(index)].layer) != 0;
    };
    std::vector<std::string> out(m_preamble);
    const auto lines = [&](const std::vector<std::string>& block) {
        out.insert(out.end(), block.begin(), block.end());
        out.push_back(std::string());
    };
    out.push_back(kSpawnerBanner);
    for (int s = 0; s < 3; s += 2)
    {
        if (s == 2)
            out.push_back(kControlPointBanner);
        for (size_t k = 0; k < templateOrder[s].size(); ++k)
            if (in(templateOrder[s][k]))
                lines(m_objects[static_cast<size_t>(templateOrder[s][k])].templateLines);
        out.push_back(kHost);
        out.push_back(std::string());
        for (size_t k = 0; k < instanceOrder[s].size(); ++k)
            if (in(instanceOrder[s][k]))
                lines(m_objects[static_cast<size_t>(instanceOrder[s][k])].instanceLines);
        out.push_back(kEndIf);
        out.push_back(std::string());
        if (s == 0)
        {
            out.push_back(kSpawnPointBanner);
            // Each spawn point's template, then its instance (the writer's pairs).
            for (size_t k = 0; k < templateOrder[1].size(); ++k)
            {
                const int at = templateOrder[1][k];
                if (!in(at))
                    continue;
                lines(m_objects[static_cast<size_t>(at)].templateLines);
                if (!m_objects[static_cast<size_t>(at)].instanceLines.empty())
                    lines(m_objects[static_cast<size_t>(at)].instanceLines);
            }
        }
    }
    out.insert(out.end(), m_manager.begin(), m_manager.end());
    for (size_t k = 0; k < templateOrder[3].size(); ++k)
        if (in(templateOrder[3][k]))
            lines(m_objects[static_cast<size_t>(templateOrder[3][k])].instanceLines);
    std::string text;
    for (size_t k = 0; k < out.size(); ++k)
        text += out[k] + newline;
    return text;
}

// The game-mode files

std::string renderBucket(const GameplaySet& set, const std::vector<GameplayLayer>& layers, const std::string& mode,
                         const std::string& players)
{
    std::set<int> include;
    for (size_t i = 0; i < layers.size(); ++i)
        if (!layers[i].disabled && layers[i].takes(mode, players))
            include.insert(layers[i].id);
    return set.render(include, false);
}

bool bucketWritten(const GameplaySet& set, const std::vector<GameplayLayer>& layers, const std::string& mode,
                   const std::string& players, bool exists)
{
    if (exists)
        return true;
    for (size_t i = 0; i < layers.size(); ++i)
    {
        if (layers[i].id == 1 || layers[i].disabled || !layers[i].takes(mode, players))
            continue;
        for (size_t k = 0; k < set.objects().size(); ++k)
            if (set.objects()[k].layer == layers[i].id)
                return true;
    }
    return false;
}

namespace
{

// The sequence `order` of `from`'s objects merged into `into`'s order for that section; false on a contradiction.
bool mergeOrder(std::vector<int>& into, const std::vector<int>& order, const std::vector<int>& remap)
{
    int last = -1;
    for (size_t k = 0; k < order.size(); ++k)
    {
        const int global = remap[static_cast<size_t>(order[k])];
        const std::vector<int>::iterator found = std::find(into.begin(), into.end(), global);
        int position = found == into.end() ? -1 : static_cast<int>(found - into.begin());
        if (position < 0)
        {
            position = last + 1;
            into.insert(into.begin() + position, global);
        }
        else if (position <= last)
            return false;
        last = position;
    }
    return true;
}

std::string firstDifference(const std::string& a, const std::string& b)
{
    size_t i = 0;
    while (i < a.size() && i < b.size() && a[i] == b[i])
        ++i;
    const size_t lineStart = a.rfind('\n', i == 0 ? 0 : i - 1);
    const size_t from = lineStart == std::string::npos ? 0 : lineStart + 1;
    int line = 1;
    for (size_t k = 0; k < from; ++k)
        line += a[k] == '\n';
    char text[64];
    std::snprintf(text, sizeof(text), "line %d", line);
    const size_t end = b.find_first_of("\r\n", from);
    return std::string(text) + ": " + (from < b.size() ? b.substr(from, end == std::string::npos ? std::string::npos : end - from)
                                                       : std::string("(end of file)"));
}

} // namespace

bool importBuckets(const std::vector<GameplayBucket>& buckets, const GameplayFilters& filters, GameplayImport& result)
{
    std::vector<GameplaySet> parsed(buckets.size());
    std::vector<bool> managed(buckets.size(), true);
    result = GameplayImport();
    for (size_t b = 0; b < buckets.size(); ++b)
    {
        std::string error;
        if (!parsed[b].parse(buckets[b].text, error))
        {
            managed[b] = false;
            result.notes.push_back(bucketName(buckets[b].mode, buckets[b].players) + ": not in the retail layout - " + error);
        }
    }
    // The filter universe: the mod's, plus what the files hold.
    GameplayFilters universe = filters;
    for (size_t b = 0; b < buckets.size(); ++b)
    {
        universe.addMode(buckets[b].mode, std::string());
        universe.addPlayers(buckets[b].players);
    }
    for (int pass = 0; pass < 64; ++pass)
    {
        GameplaySet set;
        bool changed = false;
        // The union, each object once; every copy must be the same.
        std::vector<std::vector<int> > remaps(buckets.size());
        std::map<int, std::set<size_t> > holders;       // global object -> the buckets holding it
        for (size_t b = 0; b < buckets.size() && !changed; ++b)
        {
            if (!managed[b])
                continue;
            const std::vector<GameplayObject>& objects = parsed[b].objects();
            remaps[b].assign(objects.size(), -1);
            for (size_t k = 0; k < objects.size() && !changed; ++k)
            {
                int at = set.find(objects[k].section, objects[k].name);
                if (at < 0)
                {
                    set.objects().push_back(objects[k]);
                    at = static_cast<int>(set.objects().size()) - 1;
                }
                else if (set.objects()[static_cast<size_t>(at)].templateLines != objects[k].templateLines
                         || set.objects()[static_cast<size_t>(at)].instanceLines != objects[k].instanceLines)
                {
                    managed[b] = false;
                    changed = true;
                    result.notes.push_back(bucketName(buckets[b].mode, buckets[b].players) + ": its " + objects[k].name
                                           + " differs from another game mode's (an older save)");
                    break;
                }
                remaps[b][k] = at;
                holders[at].insert(b);
            }
            if (changed)
                break;
            if (set.manager().empty() && set.preamble().empty())
            {
                set.manager() = parsed[b].manager();
                set.preamble() = parsed[b].preamble();
            }
            else if (set.manager() != parsed[b].manager() || set.preamble() != parsed[b].preamble())
            {
                managed[b] = false;
                changed = true;
                result.notes.push_back(bucketName(buckets[b].mode, buckets[b].players)
                                       + ": its combat-area manager lines (or the lines above its sections) differ from "
                                         "another game mode's");
                break;
            }
            set.newline = parsed[b].newline;
            for (int s = 0; s < 4 && !changed; ++s)
                if (!mergeOrder(set.templateOrder[s], parsed[b].templateOrder[s], remaps[b])
                    || !mergeOrder(set.instanceOrder[s], parsed[b].instanceOrder[s], remaps[b]))
                {
                    managed[b] = false;
                    changed = true;
                    result.notes.push_back(bucketName(buckets[b].mode, buckets[b].players)
                                           + ": its objects come in another order than another game mode's");
                }
        }
        if (changed)
            continue;
        size_t managedCount = 0;
        for (size_t b = 0; b < buckets.size(); ++b)
            managedCount += managed[b] ? 1 : 0;
        // The Default layer's objects are in every game mode's file: one that holds some the others lack is from
        // another save.
        for (std::map<int, std::set<size_t> >::const_iterator h = holders.begin(); h != holders.end() && !changed; ++h)
        {
            if (set.objects()[static_cast<size_t>(h->first)].layer != 1 || h->second.size() == managedCount)
                continue;
            for (std::set<size_t>::const_iterator b = h->second.begin(); b != h->second.end(); ++b)
            {
                managed[*b] = false;
                result.notes.push_back(bucketName(buckets[*b].mode, buckets[*b].players) + ": it holds Default-layer objects ("
                                       + set.objects()[static_cast<size_t>(h->first)].name + ") the other game modes lack");
            }
            changed = true;
        }
        if (changed)
            continue;
        // The layers: the Default with every filter; another with the game modes and player counts its objects
        // are in, which must be one filter set of files that exist.
        std::map<int, GameplayLayer> layers;
        for (std::map<int, std::set<size_t> >::const_iterator h = holders.begin(); h != holders.end(); ++h)
        {
            const int id = set.objects()[static_cast<size_t>(h->first)].layer;
            GameplayLayer& layer = layers[id];
            layer.id = id;
            for (std::set<size_t>::const_iterator b = h->second.begin(); b != h->second.end(); ++b)
            {
                if (std::find(layer.modes.begin(), layer.modes.end(), buckets[*b].mode) == layer.modes.end())
                    layer.modes.push_back(buckets[*b].mode);
                if (std::find(layer.players.begin(), layer.players.end(), buckets[*b].players) == layer.players.end())
                    layer.players.push_back(buckets[*b].players);
            }
        }
        for (std::map<int, GameplayLayer>::iterator l = layers.begin(); l != layers.end() && !changed; ++l)
        {
            GameplayLayer& layer = l->second;
            if (layer.id == 1)
            {
                layer.name = "Default";
                layer.modes = universe.modes;
                layer.players = universe.players;
                continue;
            }
            std::sort(layer.players.begin(), layer.players.end(),
                      [](const std::string& a, const std::string& b) { return std::atoi(a.c_str()) < std::atoi(b.c_str()); });
            layer.name.clear();
            for (size_t k = 0; k < layer.modes.size(); ++k)
                layer.name += (k ? " " : "") + layer.modes[k];
            for (size_t k = 0; k < layer.players.size(); ++k)
                layer.name += " " + layer.players[k];
            for (size_t m = 0; m < layer.modes.size() && !changed; ++m)
                for (size_t p = 0; p < layer.players.size() && !changed; ++p)
                {
                    bool present = false;
                    for (size_t b = 0; b < buckets.size(); ++b)
                        present = present || (managed[b] && buckets[b].mode == layer.modes[m] && buckets[b].players == layer.players[p]);
                    if (present)
                        continue;
                    // The filter set would make a file that does not exist: the layer's files are not one save.
                    for (size_t b = 0; b < buckets.size(); ++b)
                        if (managed[b] && layer.takes(buckets[b].mode, buckets[b].players))
                        {
                            managed[b] = false;
                            result.notes.push_back(bucketName(buckets[b].mode, buckets[b].players) + ": layer " + std::to_string(layer.id)
                                                   + "'s game modes and player counts are no one filter set");
                        }
                    changed = true;
                }
        }
        if (changed)
            continue;
        // Each file kept must come back byte for byte.
        std::vector<GameplayLayer> list;
        if (layers.find(1) == layers.end())
        {
            GameplayLayer defaults;
            defaults.name = "Default";
            defaults.modes = universe.modes;
            defaults.players = universe.players;
            list.push_back(defaults);
        }
        for (std::map<int, GameplayLayer>::const_iterator l = layers.begin(); l != layers.end(); ++l)
            list.push_back(l->second);
        for (size_t b = 0; b < buckets.size(); ++b)
        {
            if (!managed[b])
                continue;
            const std::string again = renderBucket(set, list, buckets[b].mode, buckets[b].players);
            if (again != buckets[b].text)
            {
                managed[b] = false;
                changed = true;
                result.notes.push_back(bucketName(buckets[b].mode, buckets[b].players) + ": regenerated it differs at "
                                       + firstDifference(again, buckets[b].text));
            }
        }
        if (changed)
            continue;
        result.set = set;
        result.layers = list;
        for (size_t b = 0; b < buckets.size(); ++b)
            (managed[b] ? result.managed : result.unmanaged).push_back(std::make_pair(buckets[b].mode, buckets[b].players));
        return !result.managed.empty();
    }
    result.notes.push_back("no stable set of game-mode files");
    return false;
}

} // namespace bf2editor
