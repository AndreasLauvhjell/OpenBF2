// Edits named Object.layer groups and their visibility, locking, disabling and mode/player filters.
// New layers take the lowest free ID below 20. Default layer 1 always has every filter and cannot be removed.
// Removal moves objects to Default or deletes them; combat areas are always deleted.
// Disabled layers remain editable but are omitted from game files. Layer and object edits have separate undo entries.
#include "Framework/EditorConfig.h"
#include "App/EditSession.h"

#include "App/EditorLevel.h"
#include "App/LevelDocument.h"
#include "Framework/EditorLog.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <set>

namespace bf2editor
{

namespace
{

std::string number(int value)
{
    char text[32];
    std::snprintf(text, sizeof(text), "%d", value);
    return text;
}

std::vector<std::string> wordsOf(const std::string& text)
{
    std::vector<std::string> words;
    size_t i = 0;
    while (i < text.size())
    {
        while (i < text.size() && (text[i] == ' ' || text[i] == '\t' || text[i] == ','))
            ++i;
        size_t j = i;
        while (j < text.size() && text[j] != ' ' && text[j] != '\t' && text[j] != ',')
            ++j;
        if (j > i)
            words.push_back(text.substr(i, j - i));
        i = j;
    }
    return words;
}

std::string joined(const std::vector<std::string>& words)
{
    std::string text;
    for (size_t i = 0; i < words.size(); ++i)
        text += (i ? " " : "") + words[i];
    return text;
}

GameplayLayer* findLayer(std::vector<GameplayLayer>& layers, int id)
{
    for (size_t i = 0; i < layers.size(); ++i)
        if (layers[i].id == id)
            return &layers[i];
    return 0;
}

std::string describeLayer(const GameplayLayer& l)
{
    std::string text = number(l.id) + " " + l.name + " (" + (l.id == 1 ? std::string("every game mode and size")
                                                                        : joined(l.modes) + "; " + joined(l.players)) + ")";
    if (l.disabled)
        text += " disabled";
    if (l.locked)
        text += " locked";
    if (!l.visible)
        text += " hidden";
    return text;
}

} // namespace

void EditSession::adoptLayerModel()
{
    if (!m_layerModel.active() || !m_document)
        return;
    for (size_t i = 0; i < m_layerModel.layers().size(); ++i)
    {
        const GameplayLayer& l = m_layerModel.layers()[i];
        if (!l.visible)
            m_hiddenLayers.insert(l.id);
        if (l.locked)
            m_lockedLayers.insert(l.id);
    }
    ++m_visibilityRevision;
    if (!m_hiddenLayers.empty())
        syncVisibility();
    // New objects go where the loaded game mode's own objects are: its one layer besides the Default, when it has one.
    char size[16];
    std::snprintf(size, sizeof(size), "%d", m_level.layerSize());
    int only = 0, count = 0;
    for (size_t i = 0; i < m_layerModel.layers().size(); ++i)
    {
        const GameplayLayer& l = m_layerModel.layers()[i];
        if (l.id != 1 && l.takes(m_level.gameMode(), size))
        {
            only = l.id;
            ++count;
        }
    }
    m_layerModel.activeLayer = count == 1 ? only : 1;
    m_countsStale = true;
}

bool EditSession::layersMeet(int a, int b) const
{
    if (a == b || !m_layerModel.active())
        return true;
    const GameplayLayer* la = m_layerModel.layer(a);
    const GameplayLayer* lb = m_layerModel.layer(b);
    if (!la || !lb)
        return true;
    for (size_t m = 0; m < la->modes.size(); ++m)
        for (size_t p = 0; p < la->players.size(); ++p)
            if (lb->takes(la->modes[m], la->players[p]))
                return true;
    return false;
}

int EditSession::newObjectLayer(size_t file) const
{
    size_t gameplay = 0;
    if (m_layerModel.active() && m_document && m_document->gameplayFile(gameplay) && gameplay == file)
        return m_layerModel.activeLayer;
    const int fileLayer = m_document ? m_document->fileLayer(file) : -1;
    return fileLayer >= 0 ? fileLayer : 1;
}

bool EditSession::recordLayerEdit(const std::vector<GameplayLayer>& before, const std::string& label, std::string& result)
{
    LayerEdit edit;
    edit.before = before;
    edit.after = m_layerModel.layers();
    m_layerModel.layers() = before;
    const int id = m_nextLayerEdit++;
    m_layerEdits[id] = edit;
    HistoryEntry entry;
    entry.label = label;
    char line[64];
    std::snprintf(line, sizeof(line), "EditorLayer.applyLayers %d after\n", id);
    entry.doScript = line;
    std::snprintf(line, sizeof(line), "EditorLayer.applyLayers %d before\n", id);
    entry.undoScript = line;
    if (!m_history.execute(entry, result))
    {
        m_layerEdits.erase(id);
        return false;
    }
    logLine(LogLevel::Info, "edit: %s", label.c_str());
    result = label;
    return true;
}

bool EditSession::layersCommand(const std::string& action, const std::vector<std::string>& args, std::string& result)
{
    const std::string a0 = args.size() > 0 ? args[0] : std::string();
    const std::string a1 = args.size() > 1 ? args[1] : std::string();
    const std::string a2 = args.size() > 2 ? args[2] : std::string();
    if (action == "applyLayers")
    {
        const std::map<int, LayerEdit>::const_iterator found = m_layerEdits.find(std::atoi(a0.c_str()));
        if (found == m_layerEdits.end() || (a1 != "before" && a1 != "after"))
        {
            result = "no such layer edit (EditorLayer.applyLayers <edit> <before|after>)";
            return false;
        }
        m_layerModel.layers() = a1 == "after" ? found->second.after : found->second.before;
        // The view follows the flags.
        for (size_t i = 0; i < m_layerModel.layers().size(); ++i)
        {
            const GameplayLayer& l = m_layerModel.layers()[i];
            if (l.visible)
                m_hiddenLayers.erase(l.id);
            else
                m_hiddenLayers.insert(l.id);
            if (l.locked)
                m_lockedLayers.insert(l.id);
            else
                m_lockedLayers.erase(l.id);
        }
        if (!m_layerModel.layer(m_layerModel.activeLayer))
            m_layerModel.activeLayer = 1;
        ++m_visibilityRevision;
        syncVisibility();
        m_countsStale = true;
        return true;
    }
    if (action == "status")
    {
        if (!m_layerModel.active())
        {
            result = "the gameplay layers are off: the loaded game-mode file is edited as it stands";
            return true;
        }
        result = "gameplay layers on: " + (m_document ? m_layerModel.editorPath().substr(m_document->levelDirectory().size())
                                                      : m_layerModel.editorPath())
               + (m_layerModel.editorFileExists() ? std::string(" (the level's)") : std::string(" (imported; written when a gameplay change is saved)"))
               + ", " + number(static_cast<int>(m_layerModel.layers().size())) + " layers, new objects to layer "
               + number(m_layerModel.activeLayer);
        if (!m_layerModel.unmanaged().empty())
        {
            result += "; left as they are:";
            for (size_t i = 0; i < m_layerModel.unmanaged().size(); ++i)
                result += (i ? ", " : " ") + m_layerModel.unmanaged()[i].first + " " + m_layerModel.unmanaged()[i].second;
        }
        return true;
    }
    if (!m_layerModel.active())
    {
        result = "the gameplay layers are off for this level (EditorLayer.status)";
        return false;
    }
    std::vector<GameplayLayer> layers = m_layerModel.layers();
    const std::vector<GameplayLayer> before = layers;
    if (action == "list")
    {
        result.clear();
        for (size_t i = 0; i < layers.size(); ++i)
            result += (i ? "; " : "") + describeLayer(layers[i]);
        return true;
    }
    const int id = std::atoi(a0.c_str());
    GameplayLayer* target = (action == "add") ? 0 : findLayer(layers, id);
    if (action != "add" && !target)
    {
        result = "no layer " + a0 + " (EditorLayer.list)";
        return false;
    }
    if (action == "active")
    {
        if (target->locked)
        {
            result = "layer " + a0 + " is locked: nothing is placed on it";
            return false;
        }
        m_layerModel.activeLayer = id;
        result = "new gameplay objects go to layer " + describeLayer(*target);
        return true;
    }
    std::string label;
    if (action == "add")
    {
        const int free = m_layerModel.freeLayerId();
        if (a0.empty() || a0.find('"') != std::string::npos || free < 0)
        {
            result = free < 0 ? "a level holds at most 19 layers (retail's limit)" : "EditorLayer.add <name>";
            return false;
        }
        for (size_t i = 0; i < layers.size(); ++i)
            if (_stricmp(layers[i].name.c_str(), a0.c_str()) == 0)
            {
                result = "the level has a layer named " + a0 + " already";
                return false;
            }
        GameplayLayer layer;
        layer.id = free;
        layer.name = a0;
        layer.modes = m_layerModel.filters().modes;          // every filter, as retail's new layer
        layer.players = m_layerModel.filters().players;
        layers.push_back(layer);
        label = "Add the layer " + number(free) + " " + a0;
    }
    else if (target->id == 1 && action != "disable")
    {
        result = "the Default layer keeps its name and every filter, and is not removed";
        return false;
    }
    else if (action == "rename")
    {
        if (a1.empty() || a1.find('"') != std::string::npos)
        {
            result = "EditorLayer.rename <id> <name>";
            return false;
        }
        label = "Rename the layer " + a0 + " to " + a1;
        target->name = a1;
    }
    else if (action == "filters")
    {
        const std::vector<std::string> modes = wordsOf(a1);
        const std::vector<std::string> players = wordsOf(a2);
        const GameplayFilters& known = m_layerModel.filters();
        for (size_t i = 0; i < modes.size(); ++i)
        {
            bool found = false;
            for (size_t k = 0; k < known.modes.size() && !found; ++k)
                found = _stricmp(known.modes[k].c_str(), modes[i].c_str()) == 0;
            if (!found)
            {
                result = "'" + modes[i] + "' is not one of the mod's game modes (" + joined(known.modes) + ")";
                return false;
            }
        }
        for (size_t i = 0; i < players.size(); ++i)
            if (std::find(known.players.begin(), known.players.end(), players[i]) == known.players.end())
            {
                result = "'" + players[i] + "' is not one of the mod's player counts (" + joined(known.players) + ")";
                return false;
            }
        target->modes = modes;
        target->players = players;
        label = "Filter the layer " + a0 + " to " + (modes.empty() ? std::string("no game mode") : joined(modes)) + "; "
              + (players.empty() ? std::string("no size") : joined(players));
    }
    else if (action == "disable")
    {
        if (a1 != "0" && a1 != "1")
        {
            result = "EditorLayer.disable <id> <0|1>";
            return false;
        }
        target->disabled = a1 == "1";
        label = std::string(target->disabled ? "Disable" : "Enable") + " the layer " + a0
              + (target->disabled ? " (left out of the game-mode files)" : "");
    }
    else if (action == "remove")
    {
        // Its objects first (their own undoable edits), then its combat areas, then the layer.
        std::string error;
        if (selectLayer(id, error) && !m_selection.empty())
        {
            if (a1 != "move" && a1 != "delete")
            {
                result = "the layer holds " + number(static_cast<int>(m_selection.size())) + " objects: EditorLayer.remove "
                       + a0 + " <move|delete> (to the Default layer, or deleted)";
                return false;
            }
            std::string done;
            if (!(a1 == "move" ? setSelectionLayer(1, done) : deleteSelection(done)))
            {
                result = done;
                return false;
            }
        }
        size_t file = 0;
        if (m_document && m_document->gameplayFile(file))
        {
            GameplaySet set;
            std::string parseError;
            if (set.parse(m_document->objects(file).render(), parseError))
                for (size_t i = 0; i < set.objects().size(); ++i)
                    if (set.objects()[i].section == GameplaySection::CombatArea && set.objects()[i].layer == id)
                    {
                        std::string areaResult;
                        if (!deleteCombatArea(set.objects()[i].name, areaResult))
                            logLine(LogLevel::Warning, "layers: the combat area %s was not deleted: %s", set.objects()[i].name.c_str(),
                                    areaResult.c_str());
                    }
        }
        layers = m_layerModel.layers();
        for (size_t i = 0; i < layers.size(); ++i)
            if (layers[i].id == id)
            {
                layers.erase(layers.begin() + static_cast<std::ptrdiff_t>(i));
                break;
            }
        if (m_layerModel.activeLayer == id)
            m_layerModel.activeLayer = 1;
        label = "Remove the layer " + a0;
        const std::vector<GameplayLayer> current = m_layerModel.layers();
        m_layerModel.layers() = layers;
        return recordLayerEdit(current, label, result);
    }
    else
    {
        result = "no layers action '" + action + "'";
        return false;
    }
    m_layerModel.layers() = layers;
    return recordLayerEdit(before, label, result);
}

} // namespace bf2editor
