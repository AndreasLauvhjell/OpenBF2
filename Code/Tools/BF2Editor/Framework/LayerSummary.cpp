// Code/Tools/BF2Editor/Framework/LayerSummary.cpp - see LayerSummary.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/LayerSummary.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

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

std::vector<std::string> split(const std::string& text, char separator)
{
    std::vector<std::string> parts;
    size_t start = 0;
    for (;;)
    {
        const size_t at = text.find(separator, start);
        parts.push_back(text.substr(start, at == std::string::npos ? std::string::npos : at - start));
        if (at == std::string::npos)
            break;
        start = at + 1;
    }
    return parts;
}

// A layer's own word in a name: a number ("64"), or a short tag with one ("SP16").
bool isLayerToken(const std::string& token)
{
    size_t letters = 0;
    while (letters < token.size() && std::isalpha(static_cast<unsigned char>(token[letters])))
        ++letters;
    if (letters == token.size() || letters > 4)
        return false;
    for (size_t i = letters; i < token.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(token[i])))
            return false;
    return true;
}

const LevelObjectFile::TemplateLine* line(const std::vector<LevelObjectFile::TemplateLine>& lines, const std::string& key)
{
    for (size_t i = 0; i < lines.size(); ++i)
        if (lines[i].key == key)
            return &lines[i];
    return 0;
}

int integerLine(const std::vector<LevelObjectFile::TemplateLine>& lines, const std::string& key, int fallback)
{
    const LevelObjectFile::TemplateLine* found = line(lines, key);
    return found ? std::atoi(found->value.c_str()) : fallback;
}

float numberLine(const std::vector<LevelObjectFile::TemplateLine>& lines, const std::string& key, float fallback)
{
    const LevelObjectFile::TemplateLine* found = line(lines, key);
    return found ? static_cast<float>(std::atof(found->value.c_str())) : fallback;
}

std::string labelOf(const std::string& place)
{
    const std::vector<std::string> tokens = split(place, '_');
    if (tokens.size() > 2 && tokens[0] == "cpname")
    {
        std::string label;
        for (size_t i = 2; i < tokens.size(); ++i)
            label += (label.empty() ? "" : "_") + tokens[i];
        return label;
    }
    return place;
}

bool rowBefore(const ControlPointRow& a, const ControlPointRow& b)
{
    return a.label < b.label;
}

} // namespace

std::string controlPointPlace(const std::string& name)
{
    const std::vector<std::string> tokens = split(lowered(name), '_');
    std::string place;
    for (size_t i = 0; i < tokens.size(); ++i)
        if (!tokens[i].empty() && !isLayerToken(tokens[i]))
            place += (place.empty() ? "" : "_") + tokens[i];
    return place.empty() ? lowered(name) : place;
}

LayerSummary summarizeLayer(const LevelObjectFile& file, const std::string& mode, int size)
{
    LayerSummary summary;
    summary.mode = mode;
    summary.size = size;
    for (size_t r = 0; r < file.recordCount(); ++r)
    {
        if (file.isDeleted(r))
            continue;
        const ObjectPlacement& p = file.placement(r);
        std::string className;
        if (!file.findTemplate(p.templateName, className) || file.isTemplateDeleted(p.templateName))
            continue;
        const std::vector<LevelObjectFile::TemplateLine> lines = file.templateLines(p.templateName);
        const std::string kind = lowered(className);
        if (kind == "controlpoint")
        {
            LayerControlPoint point;
            point.name = p.templateName;
            point.place = controlPointPlace(p.templateName);
            point.id = integerLine(lines, "controlpointid", -1);
            point.team = integerLine(lines, "team", 0);
            point.radius = numberLine(lines, "radius", 1.0f);           // ControlPointTemplate's default
            for (int k = 0; k < 3; ++k)
                point.position[k] = p.position[k];
            summary.controlPoints.push_back(point);
        }
        else if (kind == "objectspawner")
        {
            LayerSpawner spawner;
            spawner.name = p.templateName;
            for (int team = 0; team < 2; ++team)
            {
                // "setObjectTemplate 1 RUTNK_T90": the template is the second word.
                const LevelObjectFile::TemplateLine* vehicle = line(lines, team == 0 ? "setobjecttemplate 1" : "setobjecttemplate 2");
                if (vehicle)
                {
                    const size_t space = vehicle->value.find_first_of(" \t");
                    spawner.vehicle[team] = space == std::string::npos ? std::string() : vehicle->value.substr(space + 1);
                }
            }
            spawner.controlPointId = p.hasControlPointId ? p.controlPointId : -1;
            for (int k = 0; k < 3; ++k)
                spawner.position[k] = p.position[k];
            summary.spawners.push_back(spawner);
        }
        else if (kind == "spawnpoint")
        {
            summary.spawnPoints.push_back(std::vector<float>(p.position, p.position + 3));
        }
    }
    for (size_t a = 0; a < file.combatAreaCount(); ++a)
        if (!file.isCombatAreaDeleted(a))
            summary.combatAreas.push_back(file.combatArea(a));
    return summary;
}

std::vector<ControlPointRow> matchControlPoints(const std::vector<LayerSummary>& layers, float mergeDistance)
{
    std::vector<ControlPointRow> rows;
    for (size_t l = 0; l < layers.size(); ++l)
    {
        for (size_t c = 0; c < layers[l].controlPoints.size(); ++c)
        {
            const LayerControlPoint& point = layers[l].controlPoints[c];
            int row = -1;
            // The same place by name, in a row this layer has no point in yet...
            for (size_t r = 0; r < rows.size() && row < 0; ++r)
                if (rows[r].place == point.place && rows[r].point[l] < 0)
                    row = static_cast<int>(r);
            // ...or by where it stands.
            for (size_t r = 0; r < rows.size() && row < 0; ++r)
            {
                if (rows[r].point[l] >= 0)
                    continue;
                for (size_t k = 0; k < layers.size() && row < 0; ++k)
                {
                    if (rows[r].point[k] < 0)
                        continue;
                    const LayerControlPoint& other = layers[k].controlPoints[rows[r].point[k]];
                    const float dx = other.position[0] - point.position[0], dz = other.position[2] - point.position[2];
                    if (dx * dx + dz * dz <= mergeDistance * mergeDistance)
                        row = static_cast<int>(r);
                }
            }
            if (row < 0)
            {
                ControlPointRow added;
                added.place = point.place;
                added.label = labelOf(point.place);
                added.point.assign(layers.size(), -1);
                rows.push_back(added);
                row = static_cast<int>(rows.size()) - 1;
            }
            rows[row].point[l] = static_cast<int>(c);
        }
    }
    std::stable_sort(rows.begin(), rows.end(), rowBefore);
    return rows;
}

} // namespace bf2editor
