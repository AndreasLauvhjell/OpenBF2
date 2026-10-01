// Code/Tools/BF2Editor/Framework/LightSetupFile.cpp - see LightSetupFile.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "Framework/LightSetupFile.h"

#include <cstdlib>
#include <cstring>

namespace bf2editor
{

namespace
{

struct Setting
{
    const char* name;
    int         kind;
    const char* fallback;       // the retail bundle's currentLight.con
};
const Setting kSettings[] = {
    { "GILightsBaseDir", 3, "0/0.5/-0.5" }, { "GILightsIntensity", 0, "-1" }, { "GILightsShadowIntensity", 0, "-1" },
    { "GILightsFov", 0, "-1" }, { "UseGITopLight", 2, "0" }, { "GITopLightIntensity", 0, "0.1" },
    { "GILightsViewDistance", 0, "50" }, { "skyWhite", 0, "0" }, { "numberOfGILights", 1, "0" },
    { "sunLightShadowIntensity", 0, "-1" }, { "sunLightFov", 0, "8" }, { "sunLightIntensity", 0, "-1" },
};
const size_t kCount = sizeof(kSettings) / sizeof(kSettings[0]);

bool number(const std::string& text, bool integer)
{
    if (text.empty())
        return false;
    char* end = 0;
    if (integer)
        std::strtol(text.c_str(), &end, 10);
    else
        std::strtod(text.c_str(), &end);
    return end && *end == 0;
}

std::string trimmed(const std::string& text)
{
    const size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return std::string();
    return text.substr(begin, text.find_last_not_of(" \t\r\n") - begin + 1);
}

} // namespace

LightSetup::LightSetup()
{
    for (size_t i = 0; i < kCount; ++i)
        values.push_back(kSettings[i].fallback);
}

const std::vector<std::string>& LightSetup::settingNames()
{
    static std::vector<std::string> names;
    if (names.empty())
        for (size_t i = 0; i < kCount; ++i)
            names.push_back(kSettings[i].name);
    return names;
}

int LightSetup::settingKind(size_t index)
{
    return index < kCount ? kSettings[index].kind : 0;
}

bool LightSetup::set(const std::string& name, const std::string& value, std::string& error)
{
    for (size_t i = 0; i < kCount; ++i)
    {
        if (_stricmp(kSettings[i].name, name.c_str()) != 0)
            continue;
        const std::string v = trimmed(value);
        bool ok = false;
        switch (kSettings[i].kind)
        {
        case 0: ok = number(v, false); break;
        case 1: ok = number(v, true); break;
        case 2: ok = v == "0" || v == "1"; break;
        default:
        {
            const size_t a = v.find('/');
            const size_t b = a == std::string::npos ? std::string::npos : v.find('/', a + 1);
            ok = b != std::string::npos && v.find('/', b + 1) == std::string::npos && number(v.substr(0, a), false)
              && number(v.substr(a + 1, b - a - 1), false) && number(v.substr(b + 1), false);
        }
        }
        if (!ok)
        {
            static const char* const kinds[] = { "a number", "a whole number", "0 or 1", "x/y/z" };
            error = std::string(kSettings[i].name) + " takes " + kinds[kSettings[i].kind];
            return false;
        }
        values[i] = v;
        return true;
    }
    std::string list;
    for (size_t i = 0; i < kCount; ++i)
        list += (i ? ", " : "") + std::string(kSettings[i].name);
    error = "'" + name + "' is not a lightmap light setting (" + list + ")";
    return false;
}

bool LightSetup::get(const std::string& name, std::string& value) const
{
    for (size_t i = 0; i < kCount; ++i)
        if (_stricmp(kSettings[i].name, name.c_str()) == 0)
        {
            value = values[i];
            return true;
        }
    return false;
}

std::string LightSetup::render() const
{
    std::string text = "rem ******** Lightmap Generation Settings ********\r\nif v_arg1 == BF2Editor\r\n\r\nLightMapGeneration.init\r\n";
    for (size_t i = 0; i < kCount; ++i)
        text += std::string("LightMapGeneration.") + kSettings[i].name + " " + values[i] + "\r\n";
    return text + "\r\nendIf\r\n";
}

bool LightSetup::parse(const std::string& text)
{
    *this = LightSetup();
    bool any = false;
    size_t start = 0;
    while (start < text.size())
    {
        size_t end = text.find('\n', start);
        if (end == std::string::npos)
            end = text.size();
        const std::string line = trimmed(text.substr(start, end - start));
        start = end + 1;
        if (_strnicmp(line.c_str(), "LightMapGeneration.", 19) != 0)
            continue;
        const size_t space = line.find_first_of(" \t");
        if (space == std::string::npos)
            continue;
        std::string error;
        if (set(line.substr(19, space - 19), trimmed(line.substr(space + 1)), error))
            any = true;
    }
    return any;
}

} // namespace bf2editor
