// Code/Tools/BF2Editor/Framework/UndergrowthConfig.cpp - see UndergrowthConfig.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/UndergrowthConfig.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace bf2editor
{

namespace
{

std::string trimmed(const std::string& text)
{
    const size_t begin = text.find_first_not_of(" \t\r");
    if (begin == std::string::npos)
        return std::string();
    return text.substr(begin, text.find_last_not_of(" \t\r") - begin + 1);
}

// A float as an ostream prints it with its default precision (6 significant digits, no trailing zeros).
std::string number(float value)
{
    char text[64];
    std::snprintf(text, sizeof(text), "%g", static_cast<double>(value));
    return text;
}

bool toFloat(const std::string& text, float& out)
{
    char* end = 0;
    const double v = std::strtod(text.c_str(), &end);
    if (text.empty() || !end || *end != '\0')
        return false;
    out = static_cast<float>(v);
    return true;
}

// "Key a b ..." split on blanks.
std::vector<std::string> words(const std::string& line)
{
    std::vector<std::string> out;
    std::istringstream in(line);
    std::string w;
    while (in >> w)
        out.push_back(w);
    return out;
}

const char* const kSettings[] = { "ViewDistance", "ViewDistanceFadeScale", "ViewDistanceHeightScale",
                                  "ViewDistanceHeight2Scale", "ViewDistanceStreamingScale", "PatchSubdivide",
                                  "SwayScale", "LightingScale", "AlphaRef" };

} // namespace

UndergrowthType::UndergrowthType()
    : density(1.0f), variation(0.0f), terrainColorScale(1.0f), typeSwayScale(1.0f), skew(false)
{
    crossSize[0] = crossSize[1] = 1.0f;
    randomSizeScale[0] = randomSizeScale[1] = 1.0f;
}

bool UndergrowthConfig::parse(const std::string& text, std::string& error)
{
    settings.clear();
    materials.clear();
    std::istringstream in(text);
    std::string raw;
    int lineNumber = 0;
    // 0 settings, 1 after "Material" (expecting "{"), 2 in a material, 3 after "Type" (expecting "{"),
    // 4 in a type.
    int state = 0;
    while (std::getline(in, raw))
    {
        ++lineNumber;
        const std::string line = trimmed(raw);
        if (line.empty())
            continue;
        const std::vector<std::string> w = words(line);
        char where[48];
        std::snprintf(where, sizeof(where), "line %d: ", lineNumber);
        float a = 0.0f, b = 0.0f;
        switch (state)
        {
        case 0:
        case 2:
            if (w[0] == "Material" && w.size() == 3 && state != 1)
            {
                UndergrowthMaterial m;
                m.name = w[1];
                m.id = std::atoi(w[2].c_str());
                materials.push_back(m);
                state = 1;
                continue;
            }
            if (state == 0 && w.size() == 2 && toFloat(w[1], a))
            {
                settings.push_back(std::make_pair(w[0], a));
                continue;
            }
            if (state == 2 && w[0] == "}")
            {
                state = 0;
                continue;
            }
            if (state == 2 && w[0] == "GeneralHeight" && w.size() == 2 && toFloat(w[1], a))
            {
                materials.back().generalHeight = a;
                continue;
            }
            if (state == 2 && w[0] == "Type" && w.size() == 2)
            {
                UndergrowthType t;
                t.name = w[1];
                materials.back().types.push_back(t);
                state = 3;
                continue;
            }
            break;
        case 1:
            if (w[0] == "{")
            {
                state = 2;
                continue;
            }
            break;
        case 3:
            if (w[0] == "{")
            {
                state = 4;
                continue;
            }
            break;
        case 4:
        {
            UndergrowthType& t = materials.back().types.back();
            if (w[0] == "}")
            {
                state = 2;
                continue;
            }
            if (w[0] == "Mesh" && w.size() == 2)
            {
                t.mesh = w[1];
                continue;
            }
            if (w[0] == "Texture" && w.size() == 2)
            {
                t.texture = w[1];
                continue;
            }
            if ((w[0] == "CrossSize" || w[0] == "RandomSizeScale") && w.size() == 3 && toFloat(w[1], a) && toFloat(w[2], b))
            {
                float* pair = w[0] == "CrossSize" ? t.crossSize : t.randomSizeScale;
                pair[0] = a;
                pair[1] = b;
                continue;
            }
            if (w.size() == 2 && toFloat(w[1], a))
            {
                if (w[0] == "Density") { t.density = a; continue; }
                if (w[0] == "Variation") { t.variation = a; continue; }
                if (w[0] == "TerrainColorScale") { t.terrainColorScale = a; continue; }
                if (w[0] == "TypeSwayScale") { t.typeSwayScale = a; continue; }
                if (w[0] == "Skew") { t.skew = a != 0.0f; continue; }
            }
            break;
        }
        }
        error = where + std::string("not understood: ") + line;
        return false;
    }
    if (state != 0)
    {
        error = "the file ends inside a block";
        return false;
    }
    return true;
}

std::string UndergrowthConfig::render() const
{
    const std::string nl = "\r\n";
    std::string out;
    // The settings in the writer's order; any the file had that the writer does not know follow them.
    std::vector<bool> written(settings.size(), false);
    for (size_t k = 0; k < sizeof(kSettings) / sizeof(kSettings[0]); ++k)
    {
        for (size_t i = 0; i < settings.size(); ++i)
        {
            if (!written[i] && settings[i].first == kSettings[k])
            {
                out += settings[i].first + " " + number(settings[i].second) + nl;
                written[i] = true;
                break;
            }
        }
    }
    for (size_t i = 0; i < settings.size(); ++i)
        if (!written[i])
            out += settings[i].first + " " + number(settings[i].second) + nl;
    out += nl;
    for (size_t m = 0; m < materials.size(); ++m)
    {
        const UndergrowthMaterial& mat = materials[m];
        char id[16];
        std::snprintf(id, sizeof(id), "%d", mat.id);
        out += "Material " + mat.name + " " + id + nl + "{" + nl;
        out += "\tGeneralHeight " + number(mat.generalHeight) + nl + nl;
        for (size_t t = 0; t < mat.types.size(); ++t)
        {
            const UndergrowthType& type = mat.types[t];
            out += "\tType " + type.name + nl + "\t{" + nl;
            if (!type.mesh.empty())
                out += "\t\tMesh " + type.mesh + nl;
            else if (!type.texture.empty())
                out += "\t\tCrossSize " + number(type.crossSize[0]) + " " + number(type.crossSize[1]) + nl;
            out += "\t\tTexture " + type.texture + nl;
            out += "\t\tRandomSizeScale " + number(type.randomSizeScale[0]) + " " + number(type.randomSizeScale[1]) + nl;
            out += "\t\tDensity " + number(type.density) + nl;
            out += "\t\tVariation " + number(type.variation) + nl;
            out += "\t\tTerrainColorScale " + number(type.terrainColorScale) + nl;
            out += "\t\tTypeSwayScale " + number(type.typeSwayScale) + nl;
            out += std::string("\t\tSkew ") + (type.skew ? "1" : "0") + nl;
            out += "\t}" + nl;
        }
        out += "}" + nl + nl;
    }
    return out;
}

UndergrowthMaterial* UndergrowthConfig::material(const std::string& name)
{
    for (size_t i = 0; i < materials.size(); ++i)
        if (materials[i].name == name)
            return &materials[i];
    return 0;
}

UndergrowthMaterial* UndergrowthConfig::materialById(int id)
{
    for (size_t i = 0; i < materials.size(); ++i)
        if (materials[i].id == id)
            return &materials[i];
    return 0;
}

int UndergrowthConfig::freeId() const
{
    for (int id = 1; id <= 255; ++id)
    {
        bool taken = false;
        for (size_t i = 0; i < materials.size() && !taken; ++i)
            taken = materials[i].id == id;
        if (!taken)
            return id;
    }
    return 0;
}

} // namespace bf2editor
