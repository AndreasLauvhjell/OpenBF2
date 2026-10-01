// Edits control-point supply groups and dependencies in both the file and live template as one undo entry.
// New groups use max ID + 1 and copy the first other member's nonempty supply lists to the primary.
// Ungrouping sets -1 but retains dependencies. Duplicate dependencies are skipped to avoid an engine abort.
// Supply -1 clears one team's dependencies; disconnect clears both. Group -1 is omitted from the file.
#include "Framework/EditorConfig.h"
#include "App/EditSession.h"

#include "App/LevelDocument.h"
#include "Framework/EditorLog.h"

#include "Core/Ids.h"
#include "Game/Objects/IControlPointTemplate.h"
#include "Game/Objects/IObject.h"
#include "Game/Objects/IObjectTemplate.h"
#include "Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <map>
#include <set>

namespace bf2editor
{

namespace
{

dice::hfe::world::IControlPointTemplate* controlPointTemplate(const std::string& name)
{
    using namespace dice::hfe::world;
    IObjectTemplate* t = objectTemplateManager ? objectTemplateManager->getTemplate(name) : 0;
    return t ? static_cast<IControlPointTemplate*>(t->queryInterface(IID_IControlPointTemplate)) : 0;
}

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

bool parseInt(const std::string& text, int& value)
{
    if (text.empty())
        return false;
    char* end = 0;
    const long parsed = std::strtol(text.c_str(), &end, 10);
    if (!end || *end != 0)
        return false;
    value = static_cast<int>(parsed);
    return true;
}

std::string groupList(const std::vector<int>& groups)
{
    std::string text;
    for (size_t i = 0; i < groups.size(); ++i)
        text += (i ? " " : "") + number(groups[i]);
    return text;
}

// One replay line each way; the undo lines run in the opposite order.
struct Script
{
    std::string doLines;
    std::vector<std::string> undoLines;
    int count = 0;
    void add(const std::string& doLine, const std::string& undoLine)
    {
        doLines += doLine + "\n";
        undoLines.push_back(undoLine + "\n");
        ++count;
    }
    std::string undo() const
    {
        std::string text;
        for (size_t i = undoLines.size(); i-- > 0;)
            text += undoLines[i];
        return text;
    }
};

std::string groupLine(const std::string& name, int group)
{
    return "EditorGameplay.applyGroup " + name + " " + number(group);
}

std::string supplyLine(const std::string& name, int team, int group, int needed, bool add)
{
    return "EditorGameplay.applySupply " + name + " \"" + number(team) + " " + number(group) + " " + number(needed)
         + (add ? " add\"" : " remove\"");
}

} // namespace

std::vector<EditSession::SupplyPoint> EditSession::supplyPoints() const
{
    std::vector<SupplyPoint> points;
    if (!m_document || !m_document->isOpen())
        return points;
    std::set<std::string> seen;
    for (size_t f = 0; f < m_document->fileCount(); ++f)
    {
        const LevelObjectFile& objects = m_document->objects(f);
        for (size_t r = 0; r < objects.recordCount(); ++r)
        {
            if (objects.isDeleted(r))
                continue;
            const std::string& name = objects.placement(r).templateName;
            std::string recordClass;
            if (!objects.findTemplate(name, recordClass) || _stricmp(recordClass.c_str(), "ControlPoint") != 0)
                continue;
            std::string key = name;
            for (size_t i = 0; i < key.size(); ++i)
                key[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(key[i])));
            if (!seen.insert(key).second)
                continue;
            SupplyPoint p;
            p.name = name;
            p.group = -1;
            LevelDocument::RecordRef ref;
            ref.file = f;
            ref.record = r;
            p.object = m_document->idOf(ref);
            if (dice::hfe::world::IObject* o = p.object ? object(p.object) : 0)
                p.layer = static_cast<int>(o->getLayer());
            const std::vector<LevelObjectFile::TemplateLine> lines = objects.templateLines(name);
            for (size_t i = 0; i < lines.size(); ++i)
            {
                if (lines[i].key == "supplygroupid")
                    parseInt(wordsOf(lines[i].value).empty() ? std::string() : wordsOf(lines[i].value)[0], p.group);
                else if (lines[i].key.compare(0, 18, "supplygroupneeded ") == 0)
                {
                    const std::vector<std::string> words = wordsOf(lines[i].value);
                    int team = 0, group = 0, needed = 0;
                    if (words.size() < 2 || !parseInt(words[0], team) || !parseInt(words[1], group) || team < 1 || team > 2)
                        continue;
                    if (words.size() > 2)
                        parseInt(words[2], needed);
                    p.needs[team - 1].push_back(group);
                    p.needed[team - 1].push_back(needed);
                }
            }
            points.push_back(p);
        }
    }
    return points;
}

bool EditSession::supplyCommand(const std::string& action, const std::string& argument, std::string& result)
{
    using namespace dice::hfe::world;
    const std::vector<std::string> args = wordsOf(argument);

    // The replays: the level file and the running template, kept in step
    if (action == "applyGroup" || action == "applySupply")
    {
        size_t file = 0;
        std::string className;
        if (args.empty() || !m_document || !m_document->templateFile(args[0], file, className)
            || _stricmp(className.c_str(), "ControlPoint") != 0)
        {
            result = "no control point template '" + (args.empty() ? std::string() : args[0]) + "' in the level files";
            return false;
        }
        const std::string& name = args[0];
        IControlPointTemplate* cp = controlPointTemplate(name);
        if (action == "applyGroup")
        {
            int group = 0;
            if (args.size() != 2 || !parseInt(args[1], group) || group < -1)
            {
                result = "EditorGameplay.applyGroup <control point template> <group, -1 for none>";
                return false;
            }
            m_document->setTemplateValue(file, name, "supplyGroupId", group == -1 ? std::string() : number(group));
            if (cp)
                cp->setGroupId(group);
            return true;
        }
        int team = 0, group = 0, needed = 0;
        const bool add = args.size() == 5 && args[4] == "add";
        if (args.size() != 5 || !parseInt(args[1], team) || !parseInt(args[2], group) || !parseInt(args[3], needed)
            || team < 1 || team > 2 || group < 0 || (needed != 0 && needed != 1) || (!add && args[4] != "remove"))
        {
            result = "EditorGameplay.applySupply <control point template> \"<team 1|2> <group> <needed 0|1> <add|remove>\"";
            return false;
        }
        if (add)
        {
            m_document->setTemplateValue(file, name, "supplyGroupNeeded", number(team) + " " + number(group) + " " + number(needed));
            if (cp)
            {
                // Never a second entry: the engine stops the game on one.
                const std::list<int> held = cp->getSupplyGroupNeeded(team);
                if (std::find(held.begin(), held.end(), group) == held.end())
                    cp->addSupplyGroupNeeded(team, group, needed != 0);
            }
        }
        else
        {
            m_document->setTemplateValue(file, name, "supplyGroupNeeded " + number(team) + " " + number(group), std::string());
            if (cp)
                cp->removeSupplyGroupNeeded(team, group);
        }
        return true;
    }

    const std::vector<SupplyPoint> points = supplyPoints();
    if (action == "supplyGroups")
    {
        result.clear();
        for (size_t i = 0; i < points.size(); ++i)
        {
            const SupplyPoint& p = points[i];
            if (p.group < 0 && p.needs[0].empty() && p.needs[1].empty())
                continue;
            result += (result.empty() ? "" : "; ") + p.name + ": " + (p.group >= 0 ? "group " + number(p.group) : "no group");
            for (int t = 0; t < 2; ++t)
                if (!p.needs[t].empty())
                    result += ", team " + number(t + 1) + " needs " + groupList(p.needs[t]);
        }
        if (result.empty())
            result = "no control point has a supply group (" + number(static_cast<int>(points.size())) + " control points)";
        return true;
    }

    // The selected control points, in selection order (retail's SM+0x20).
    std::vector<const SupplyPoint*> selected;
    const std::vector<unsigned>& ids = m_selection.ids();
    for (size_t i = 0; i < ids.size(); ++i)
        for (size_t k = 0; k < points.size(); ++k)
            if (points[k].object == ids[i] && points[k].object != 0)
                selected.push_back(&points[k]);
    Script script;
    std::string label;
    if (action == "groupControlPoints" || action == "ungroupControlPoints")
    {
        if (selected.empty())
        {
            result = "select control points first (each one's template takes the group)";
            return false;
        }
        if (action == "ungroupControlPoints")
        {
            for (size_t i = 0; i < selected.size(); ++i)
                if (selected[i]->group != -1)
                    script.add(groupLine(selected[i]->name, -1), groupLine(selected[i]->name, selected[i]->group));
            if (script.count == 0)
            {
                result = "none of the selected control points is in a supply group";
                return false;
            }
            label = "Ungroup " + number(script.count) + " control point" + (script.count == 1 ? "" : "s");
        }
        else
        {
            int highest = 0;
            for (size_t i = 0; i < points.size(); ++i)
                highest = std::max(highest, points[i].group);
            const int group = highest + 1;
            for (size_t i = 0; i < selected.size(); ++i)
                script.add(groupLine(selected[i]->name, group), groupLine(selected[i]->name, selected[i]->group));
            // The primary (the last selected) takes the supply lists of the first other member that has any, and
            // none when no member has (retail's 0x100285B0).
            const SupplyPoint* primary = selected.back();
            const SupplyPoint* donor = 0;
            for (size_t i = 0; i < selected.size() && !donor; ++i)
                if (selected[i] != primary && (!selected[i]->needs[0].empty() || !selected[i]->needs[1].empty()))
                    donor = selected[i];
            for (int t = 0; t < 2; ++t)
                for (size_t k = 0; k < primary->needs[t].size(); ++k)
                {
                    const bool kept = donor && std::find(donor->needs[t].begin(), donor->needs[t].end(), primary->needs[t][k])
                                                   != donor->needs[t].end();
                    if (!kept)
                        script.add(supplyLine(primary->name, t + 1, primary->needs[t][k], primary->needed[t][k], false),
                                   supplyLine(primary->name, t + 1, primary->needs[t][k], primary->needed[t][k], true));
                }
            if (donor)
                for (int t = 0; t < 2; ++t)
                    for (size_t k = 0; k < donor->needs[t].size(); ++k)
                        if (std::find(primary->needs[t].begin(), primary->needs[t].end(), donor->needs[t][k]) == primary->needs[t].end())
                            script.add(supplyLine(primary->name, t + 1, donor->needs[t][k], donor->needed[t][k], true),
                                       supplyLine(primary->name, t + 1, donor->needs[t][k], donor->needed[t][k], false));
            label = "Group " + number(static_cast<int>(selected.size())) + " control point" + (selected.size() == 1 ? "" : "s")
                  + " as supply group " + number(group);
        }
    }
    else if (action == "connectSupply" || action == "disconnectSupply")
    {
        int team = 0, supply = -1, group = 0;      // Disconnect: supply -1 for both teams
        const bool connect = action == "connectSupply";
        if (connect ? (args.size() != 3 || !parseInt(args[0], team) || !parseInt(args[1], supply) || !parseInt(args[2], group)
                       || team < 1 || team > 2 || supply < -1)
                    : (args.size() != 1 || !parseInt(args[0], group)))
        {
            result = connect ? "EditorGameplay.connectSupply <team 1|2> <supply group, -1 to disconnect the team> <group>"
                             : "EditorGameplay.disconnectSupply <group>";
            return false;
        }
        bool groupExists = false, supplyExists = supply == -1;
        for (size_t i = 0; i < points.size(); ++i)
        {
            groupExists = groupExists || points[i].group == group;
            supplyExists = supplyExists || points[i].group == supply;
        }
        if (!groupExists || !supplyExists)
        {
            result = "no control point is in supply group " + number(groupExists ? supply : group);
            return false;
        }
        for (int t = 0; t < 2; ++t)
        {
            if (connect && t != team - 1)
                continue;
            for (size_t i = 0; i < points.size(); ++i)
            {
                const SupplyPoint& p = points[i];
                if (p.group != group)
                    continue;
                if (!connect || supply == -1)
                {
                    for (size_t k = 0; k < p.needs[t].size(); ++k)
                        script.add(supplyLine(p.name, t + 1, p.needs[t][k], p.needed[t][k], false),
                                   supplyLine(p.name, t + 1, p.needs[t][k], p.needed[t][k], true));
                }
                else if (std::find(p.needs[t].begin(), p.needs[t].end(), supply) == p.needs[t].end())
                    script.add(supplyLine(p.name, t + 1, supply, 0, true), supplyLine(p.name, t + 1, supply, 0, false));
            }
        }
        if (script.count == 0)
        {
            result = "no change";
            return true;
        }
        label = !connect ? "Disconnect the supply lines of group " + number(group)
              : supply == -1 ? "Disconnect team " + number(team) + "'s supply lines of group " + number(group)
                             : "Team " + number(team) + " of group " + number(group) + " needs group " + number(supply);
    }
    else
    {
        result = "no supply action '" + action + "'";
        return false;
    }
    HistoryEntry entry;
    entry.label = label;
    entry.doScript = script.doLines;
    entry.undoScript = script.undo();
    if (!m_history.execute(entry, result))
        return false;
    logLine(LogLevel::Info, "edit: %s", label.c_str());
    result = label;
    return true;
}

} // namespace bf2editor
