// Code/Tools/BF2Editor/Framework/GroundPropertiesFile.cpp - see GroundPropertiesFile.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/GroundPropertiesFile.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace bf2editor
{

namespace
{

const char* const kLayerPrefix = "groundproperties.";
const char* const kMiscPrefix = "miscgroundproperties.";

std::string lowered(std::string text)
{
    for (size_t i = 0; i < text.size(); ++i)
        text[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
    return text;
}

std::string trimmed(const std::string& text)
{
    const size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return std::string();
    return text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1);
}

bool parseFloat(const std::string& text, float& out)
{
    const std::string t = trimmed(text);
    char* end = 0;
    const double v = std::strtod(t.c_str(), &end);
    if (t.empty() || !end || *end != '\0')
        return false;
    out = static_cast<float>(v);
    return true;
}

bool parseInt(const std::string& text, int low, int high, int& out)
{
    const std::string t = trimmed(text);
    char* end = 0;
    const long v = std::strtol(t.c_str(), &end, 10);
    if (t.empty() || !end || *end != '\0' || v < low || v > high)
        return false;
    out = static_cast<int>(v);
    return true;
}

// "x/y" (the console's Vec2 form).
bool parsePair(const std::string& text, float out[2])
{
    const std::string t = trimmed(text);
    const size_t slash = t.find('/');
    return slash != std::string::npos && parseFloat(t.substr(0, slash), out[0]) && parseFloat(t.substr(slash + 1), out[1]);
}

std::string unquoted(const std::string& text)
{
    const std::string t = trimmed(text);
    if (t.size() >= 2 && t[0] == '"' && t[t.size() - 1] == '"')
        return t.substr(1, t.size() - 2);
    return t;
}

std::string pair(const float v[2])
{
    return GroundProperties::number(v[0]) + "/" + GroundProperties::number(v[1]);
}

} // namespace

bool GroundLayer::operator==(const GroundLayer& o) const
{
    return detailTexture == o.detailTexture && colorTexture == o.colorTexture && color[0] == o.color[0]
        && color[1] == o.color[1] && color[2] == o.color[2] && material == o.material && topTiling == o.topTiling
        && sideTiling[0] == o.sideTiling[0] && sideTiling[1] == o.sideTiling[1] && yOffset == o.yOffset
        && planeMap == o.planeMap && colorTiling[0] == o.colorTiling[0] && colorTiling[1] == o.colorTiling[1]
        && lowDetailType == o.lowDetailType && envMap == o.envMap;
}

bool GroundMisc::operator==(const GroundMisc& o) const
{
    return farTopTilingHi == o.farTopTilingHi && farTopTilingLow == o.farTopTilingLow
        && farSideTiling[0] == o.farSideTiling[0] && farSideTiling[1] == o.farSideTiling[1] && farYOffset == o.farYOffset
        && lodSwitchDistance == o.lodSwitchDistance;
}

std::string GroundProperties::number(float value)
{
    std::ostringstream out;
    out << value;
    return out.str();
}

std::string GroundProperties::render() const
{
    std::string text;
    for (std::map<int, GroundLayer>::const_iterator it = layers.begin(); it != layers.end(); ++it)
    {
        const std::string i = std::to_string(it->first);
        const GroundLayer& l = it->second;
        if (!l.detailTexture.empty())
            text += "GroundProperties.SetDetailTexture " + i + " \"" + l.detailTexture + "\"\r\n";
        if (!l.colorTexture.empty())
            text += "GroundProperties.SetColorTexture " + i + " \"" + l.colorTexture + "\"\r\n";
        text += "GroundProperties.SetColor " + i + " " + std::to_string(l.color[0]) + " " + std::to_string(l.color[1]) + " "
              + std::to_string(l.color[2]) + "\r\n";
        if (!l.material.empty())
            text += "GroundProperties.SetMaterial " + i + " " + l.material + "\r\n";
        text += "GroundProperties.SetTopTiling " + i + " " + number(l.topTiling) + "\r\n";
        text += "GroundProperties.SetSideTiling " + i + " " + pair(l.sideTiling) + "\r\n";
        text += "GroundProperties.SetYOffset " + i + " " + number(l.yOffset) + "\r\n";
        text += "GroundProperties.SetPlaneMap " + i + " " + std::to_string(l.planeMap) + "\r\n";
        text += "GroundProperties.SetColorTiling " + i + " " + pair(l.colorTiling) + "\r\n";
        text += "GroundProperties.SetLowDetailType " + i + " " + std::to_string(l.lowDetailType) + "\r\n";
        text += "GroundProperties.SetEnvMap " + i + " " + (l.envMap ? "1" : "0") + "\r\n";
    }
    text += "\r\n";
    text += "MiscGroundProperties.SetFarTopTilingHi " + number(misc.farTopTilingHi) + "\r\n";
    text += "MiscGroundProperties.SetFarTopTilingLow " + number(misc.farTopTilingLow) + "\r\n";
    text += "MiscGroundProperties.SetFarSideTiling " + pair(misc.farSideTiling) + "\r\n";
    text += "MiscGroundProperties.SetFarYOffset " + number(misc.farYOffset) + "\r\n";
    text += "MiscGroundProperties.SetLodSwitchDistance " + number(misc.lodSwitchDistance) + "\r\n";
    text += "\r\n";
    return text;
}

const std::vector<std::string>& GroundProperties::layerSettings()
{
    static const std::vector<std::string> names = { "detailTexture", "colorTexture", "color", "material", "topTiling",
                                                    "sideTiling", "yOffset", "planeMap", "colorTiling", "lowDetailType",
                                                    "envMap" };
    return names;
}

const std::vector<std::string>& GroundProperties::miscSettings()
{
    static const std::vector<std::string> names = { "farTopTilingHi", "farTopTilingLow", "farSideTiling", "farYOffset",
                                                    "lodSwitchDistance" };
    return names;
}

bool GroundProperties::setLayer(int layer, const std::string& name, const std::string& value, std::string& error)
{
    if (layer < 0 || layer > 5)
    {
        error = "a ground layer is 0..5";
        return false;
    }
    GroundLayer l = layers.count(layer) ? layers[layer] : GroundLayer();
    const std::string key = lowered(name);
    bool ok = true;
    if (key == "detailtexture")
        l.detailTexture = unquoted(value);
    else if (key == "colortexture")
        l.colorTexture = unquoted(value);
    else if (key == "color")
    {
        std::string v = trimmed(value);
        for (size_t i = 0; i < v.size(); ++i)
            if (v[i] == '/')
                v[i] = ' ';
        std::istringstream in(v);
        int c[3] = { -1, -1, -1 };
        std::string tail;
        ok = static_cast<bool>(in >> c[0] >> c[1] >> c[2]) && !(in >> tail);
        for (int i = 0; i < 3 && ok; ++i)
            ok = c[i] >= 0 && c[i] <= 255;
        if (ok)
            for (int i = 0; i < 3; ++i)
                l.color[i] = c[i];
        else
            error = "the colour is r/g/b, each 0..255";
    }
    else if (key == "material")
        l.material = trimmed(value);
    else if (key == "toptiling")
        ok = parseFloat(value, l.topTiling) || (error = "topTiling is a number", false);
    else if (key == "sidetiling")
        ok = parsePair(value, l.sideTiling) || (error = "sideTiling is x/y", false);
    else if (key == "yoffset")
        ok = parseFloat(value, l.yOffset) || (error = "yOffset is a number", false);
    else if (key == "planemap")
        ok = parseInt(value, 0, 255, l.planeMap) || (error = "planeMap is 0..255", false);
    else if (key == "colortiling")
        ok = parsePair(value, l.colorTiling) || (error = "colorTiling is x/y", false);
    else if (key == "lowdetailtype")
        ok = parseInt(value, 0, 255, l.lowDetailType) || (error = "lowDetailType is 0..255", false);
    else if (key == "envmap")
    {
        int on = 0;
        ok = parseInt(value, 0, 1, on) || (error = "envMap is 0 or 1", false);
        l.envMap = on != 0;
    }
    else
    {
        error = "no ground layer setting '" + name + "'";
        return false;
    }
    if (!ok)
        return false;
    layers[layer] = l;
    return true;
}

bool GroundProperties::getLayer(int layer, const std::string& name, std::string& value) const
{
    const std::map<int, GroundLayer>::const_iterator it = layers.find(layer);
    if (it == layers.end())
        return false;
    const GroundLayer& l = it->second;
    const std::string key = lowered(name);
    if (key == "detailtexture") value = l.detailTexture;
    else if (key == "colortexture") value = l.colorTexture;
    else if (key == "color") value = std::to_string(l.color[0]) + "/" + std::to_string(l.color[1]) + "/" + std::to_string(l.color[2]);
    else if (key == "material") value = l.material;
    else if (key == "toptiling") value = number(l.topTiling);
    else if (key == "sidetiling") value = pair(l.sideTiling);
    else if (key == "yoffset") value = number(l.yOffset);
    else if (key == "planemap") value = std::to_string(l.planeMap);
    else if (key == "colortiling") value = pair(l.colorTiling);
    else if (key == "lowdetailtype") value = std::to_string(l.lowDetailType);
    else if (key == "envmap") value = l.envMap ? "1" : "0";
    else
        return false;
    return true;
}

bool GroundProperties::setMisc(const std::string& name, const std::string& value, std::string& error)
{
    GroundMisc m = misc;
    const std::string key = lowered(name);
    bool ok = true;
    if (key == "fartoptilinghi")
        ok = parseFloat(value, m.farTopTilingHi);
    else if (key == "fartoptilinglow")
        ok = parseFloat(value, m.farTopTilingLow);
    else if (key == "fartoptiling")
        ok = parseFloat(value, m.farTopTilingHi) && parseFloat(value, m.farTopTilingLow);   // retail: both
    else if (key == "farsidetiling")
        ok = parsePair(value, m.farSideTiling);
    else if (key == "faryoffset")
        ok = parseFloat(value, m.farYOffset);
    else if (key == "lodswitchdistance")
        ok = parseFloat(value, m.lodSwitchDistance);
    else
    {
        error = "no ground setting '" + name + "'";
        return false;
    }
    if (!ok)
    {
        error = name + (key == "farsidetiling" ? " is x/y" : " is a number");
        return false;
    }
    misc = m;
    return true;
}

bool GroundProperties::getMisc(const std::string& name, std::string& value) const
{
    const std::string key = lowered(name);
    if (key == "fartoptilinghi") value = number(misc.farTopTilingHi);
    else if (key == "fartoptilinglow") value = number(misc.farTopTilingLow);
    else if (key == "farsidetiling") value = pair(misc.farSideTiling);
    else if (key == "faryoffset") value = number(misc.farYOffset);
    else if (key == "lodswitchdistance") value = number(misc.lodSwitchDistance);
    else
        return false;
    return true;
}

bool GroundProperties::parse(const std::string& text, std::string& error)
{
    std::istringstream in(text);
    std::string line;
    int number = 0;
    while (std::getline(in, line))
    {
        ++number;
        const std::string t = trimmed(line);
        if (t.empty() || lowered(t).compare(0, 3, "rem") == 0)
            continue;
        const size_t space = t.find_first_of(" \t");
        const std::string command = lowered(space == std::string::npos ? t : t.substr(0, space));
        const std::string rest = space == std::string::npos ? std::string() : trimmed(t.substr(space + 1));
        bool ok = false;
        std::string why;
        if (command.compare(0, std::string(kMiscPrefix).size(), kMiscPrefix) == 0)
        {
            const std::string setting = command.substr(std::string(kMiscPrefix).size());
            ok = setting.compare(0, 3, "set") == 0 && setMisc(setting.substr(3), rest, why);
        }
        else if (command.compare(0, std::string(kLayerPrefix).size(), kLayerPrefix) == 0)
        {
            const std::string setting = command.substr(std::string(kLayerPrefix).size());
            const size_t gap = rest.find_first_of(" \t");
            int layer = -1;
            if (setting.compare(0, 3, "set") == 0 && gap != std::string::npos && parseInt(rest.substr(0, gap), 0, 5, layer))
            {
                std::string value = trimmed(rest.substr(gap + 1));
                if (setting == "setcolor")
                {
                    for (size_t i = 0; i < value.size(); ++i)
                        if (value[i] == ' ' || value[i] == '\t')
                            value[i] = '/';
                    while (value.find("//") != std::string::npos)
                        value.erase(value.find("//"), 1);
                }
                ok = setLayer(layer, setting.substr(3), value, why);
            }
        }
        if (!ok)
        {
            char where[32];
            std::snprintf(where, sizeof(where), "line %d: ", number);
            error = where + t + (why.empty() ? std::string() : " (" + why + ")");
            return false;
        }
    }
    return true;
}

} // namespace bf2editor
