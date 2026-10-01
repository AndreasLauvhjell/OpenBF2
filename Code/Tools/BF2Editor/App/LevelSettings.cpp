// Code/Tools/BF2Editor/App/LevelSettings.cpp - see LevelSettings.h. Editor-only code (D3).
#include "Framework/EditorConfig.h"
#include "App/LevelSettings.h"

#include <cctype>
#include <cstdlib>
#include <cstring>

namespace bf2editor
{

namespace
{

typedef LevelSettingsFile LSF;

// The rows the Level settings panel shows, in its order. The labels are the editor's own.
const SettingSpec kSettings[] = {
    // Init.con: the teams
    { "team1.name", "Teams", "Team 1 name", "Init.con", "gameLogic.setTeamName", "1", LSF::Always, 0, SettingKind::Quoted, 0, false },
    { "team2.name", "Teams", "Team 2 name", "Init.con", "gameLogic.setTeamName", "2", LSF::Always, 0, SettingKind::Quoted, 0, false },
    { "team1.language", "Teams", "Team 1 language", "Init.con", "gameLogic.setTeamLanguage", "1", LSF::Always, 0, SettingKind::Quoted, 0, false },
    { "team2.language", "Teams", "Team 2 language", "Init.con", "gameLogic.setTeamLanguage", "2", LSF::Always, 0, SettingKind::Quoted, 0, false },
    { "flag0", "Teams", "Neutral flag", "Init.con", "gameLogic.setTeamFlag", "0", LSF::Always, 0, SettingKind::Quoted, 0, false },
    { "flag1", "Teams", "Team 1 flag", "Init.con", "gameLogic.setTeamFlag", "1", LSF::Always, 0, SettingKind::Quoted, 0, false },
    { "flag2", "Teams", "Team 2 flag", "Init.con", "gameLogic.setTeamFlag", "2", LSF::Always, 0, SettingKind::Quoted, 0, false },
    { "team1.dropVehicle", "Teams", "Team 1 drop vehicle", "Init.con", "gameLogic.setTeamDropVehicle", "1", LSF::Always, 0, SettingKind::Quoted, 0, false },
    { "team2.dropVehicle", "Teams", "Team 2 drop vehicle", "Init.con", "gameLogic.setTeamDropVehicle", "2", LSF::Always, 0, SettingKind::Quoted, 0, false },
    // Init.con: the kits (kit template, soldier template) per team and slot
    { "team1.kit0", "Kits", "Team 1 special ops", "Init.con", "gameLogic.setKit", "1 0", LSF::Always, 0, SettingKind::QuotedPair, 0, false },
    { "team1.kit1", "Kits", "Team 1 sniper", "Init.con", "gameLogic.setKit", "1 1", LSF::Always, 0, SettingKind::QuotedPair, 0, false },
    { "team1.kit2", "Kits", "Team 1 assault", "Init.con", "gameLogic.setKit", "1 2", LSF::Always, 0, SettingKind::QuotedPair, 0, false },
    { "team1.kit3", "Kits", "Team 1 support", "Init.con", "gameLogic.setKit", "1 3", LSF::Always, 0, SettingKind::QuotedPair, 0, false },
    { "team1.kit4", "Kits", "Team 1 engineer", "Init.con", "gameLogic.setKit", "1 4", LSF::Always, 0, SettingKind::QuotedPair, 0, false },
    { "team1.kit5", "Kits", "Team 1 medic", "Init.con", "gameLogic.setKit", "1 5", LSF::Always, 0, SettingKind::QuotedPair, 0, false },
    { "team1.kit6", "Kits", "Team 1 anti-tank", "Init.con", "gameLogic.setKit", "1 6", LSF::Always, 0, SettingKind::QuotedPair, 0, false },
    { "team2.kit0", "Kits", "Team 2 special ops", "Init.con", "gameLogic.setKit", "2 0", LSF::Always, 0, SettingKind::QuotedPair, 0, false },
    { "team2.kit1", "Kits", "Team 2 sniper", "Init.con", "gameLogic.setKit", "2 1", LSF::Always, 0, SettingKind::QuotedPair, 0, false },
    { "team2.kit2", "Kits", "Team 2 assault", "Init.con", "gameLogic.setKit", "2 2", LSF::Always, 0, SettingKind::QuotedPair, 0, false },
    { "team2.kit3", "Kits", "Team 2 support", "Init.con", "gameLogic.setKit", "2 3", LSF::Always, 0, SettingKind::QuotedPair, 0, false },
    { "team2.kit4", "Kits", "Team 2 engineer", "Init.con", "gameLogic.setKit", "2 4", LSF::Always, 0, SettingKind::QuotedPair, 0, false },
    { "team2.kit5", "Kits", "Team 2 medic", "Init.con", "gameLogic.setKit", "2 5", LSF::Always, 0, SettingKind::QuotedPair, 0, false },
    { "team2.kit6", "Kits", "Team 2 anti-tank", "Init.con", "gameLogic.setKit", "2 6", LSF::Always, 0, SettingKind::QuotedPair, 0, false },
    // Init.con: the tickets
    { "tickets16.team1", "Tickets", "16 players, team 1", "Init.con", "gameLogic.setDefaultNumberOfTicketsEx", "16 1", LSF::Always, 0, SettingKind::Integer, 0, false },
    { "tickets16.team2", "Tickets", "16 players, team 2", "Init.con", "gameLogic.setDefaultNumberOfTicketsEx", "16 2", LSF::Always, 0, SettingKind::Integer, 0, false },
    { "tickets32.team1", "Tickets", "32 players, team 1", "Init.con", "gameLogic.setDefaultNumberOfTicketsEx", "32 1", LSF::Always, 0, SettingKind::Integer, 0, false },
    { "tickets32.team2", "Tickets", "32 players, team 2", "Init.con", "gameLogic.setDefaultNumberOfTicketsEx", "32 2", LSF::Always, 0, SettingKind::Integer, 0, false },
    { "tickets64.team1", "Tickets", "64 players, team 1", "Init.con", "gameLogic.setDefaultNumberOfTicketsEx", "64 1", LSF::Always, 0, SettingKind::Integer, 0, false },
    { "tickets64.team2", "Tickets", "64 players, team 2", "Init.con", "gameLogic.setDefaultNumberOfTicketsEx", "64 2", LSF::Always, 0, SettingKind::Integer, 0, false },
    { "tickets128.team1", "Tickets", "128 players, team 1", "Init.con", "gameLogic.setDefaultNumberOfTicketsEx", "128 1", LSF::Always, 0, SettingKind::Integer, 0, false },
    { "tickets128.team2", "Tickets", "128 players, team 2", "Init.con", "gameLogic.setDefaultNumberOfTicketsEx", "128 2", LSF::Always, 0, SettingKind::Integer, 0, false },
    { "ticketLoss.team1", "Tickets", "Team 1 loss a minute", "Init.con", "gameLogic.setTicketLossPerMin", "1", LSF::Always, 0, SettingKind::Integer, 0, false },
    { "ticketLoss.team2", "Tickets", "Team 2 loss a minute", "Init.con", "gameLogic.setTicketLossPerMin", "2", LSF::Always, 0, SettingKind::Integer, 0, false },
    { "ticketLossAtEnd", "Tickets", "Loss a minute at the end", "Init.con", "gameLogic.setTicketLossAtEndPerMin", "", LSF::Always, 0, SettingKind::Integer, 0, false },
    { "aiWave", "Tickets", "Time to the next AI wave", "Init.con", "gameLogic.setDefaultTimeToNextAIWave", "", LSF::Always, 0, SettingKind::Integer, 0, false },
    // Init.con: the view
    { "spawnCamera", "View", "Camera before spawning", "Init.con", "gameLogic.setBeforeSpawnCamera", "", LSF::Always, 0, SettingKind::Camera, 0, false },
    { "viewDistance", "View", "Maximum view distance", "Init.con", "GameLogic.MaximumLevelViewDistance", "", LSF::Always, 0, SettingKind::Number, 0, false },
    { "textureSuffix", "View", "Texture suffix", "Init.con", "texturemanager.customTextureSuffix", "", LSF::GameBranch, "LevelSettings.CustomTextureSuffix", SettingKind::Quoted, 0, false },
    { "lod.staticMesh", "View", "Static mesh LOD distance scale", "Init.con", "renderer.globalStaticMeshLodDistanceScale", "", LSF::Always, 0, SettingKind::Number, 0, false },
    { "lod.bundleMesh", "View", "Bundle mesh LOD distance scale", "Init.con", "renderer.globalBundleMeshLodDistanceScale", "", LSF::Always, 0, SettingKind::Number, 0, false },
    { "lod.skinnedMesh", "View", "Skinned mesh LOD distance scale", "Init.con", "renderer.globalSkinnedMeshLodDistanceScale", "", LSF::Always, 0, SettingKind::Number, 0, false },
    // Sky.con: the light
    { "light.sky", "Light", "Sky colour", "Sky.con", "Lightmanager.skycolor", "", LSF::Always, 0, SettingKind::Color, 0, true },
    { "light.ambient", "Light", "Ambient colour", "Sky.con", "Lightmanager.ambientcolor", "", LSF::Always, 0, SettingKind::Color, 0, true },
    { "light.sun", "Light", "Sun colour", "Sky.con", "Lightmanager.sunColor", "", LSF::Always, 0, SettingKind::Color, 0, true },
    { "light.sunSpecular", "Light", "Sun specular colour", "Sky.con", "Lightmanager.sunSpecColor", "", LSF::Always, 0, SettingKind::Color, 0, true },
    { "light.sunDirection", "Light", "Sun direction", "Sky.con", "Lightmanager.sunDirection", "", LSF::Always, 0, SettingKind::Numbers, 3, true },
    { "light.sunEnabled", "Light", "Sun on (0/1)", "Sky.con", "Lightmanager.enableSun", "", LSF::Always, 0, SettingKind::Integer, 0, true },
    { "light.terrainSun", "Light", "Terrain sun colour", "Sky.con", "terrain.sunColor", "", LSF::GameBranch, "LightSettings.TerrainSunColor", SettingKind::Color, 0, true },
    { "light.terrainSky", "Light", "Terrain sky colour", "Sky.con", "terrain.GIColor", "", LSF::GameBranch, "LightSettings.TerrainSkyColor", SettingKind::Color, 0, true },
    { "light.staticSun", "Light", "Static sun colour", "Sky.con", "Lightmanager.staticSunColor", "", LSF::Always, 0, SettingKind::Color, 0, true },
    { "light.staticSpecular", "Light", "Static specular colour", "Sky.con", "Lightmanager.staticSpecularColor", "", LSF::Always, 0, SettingKind::Color, 0, true },
    { "light.staticSky", "Light", "Static sky colour", "Sky.con", "Lightmanager.staticSkyColor", "", LSF::Always, 0, SettingKind::Color, 0, true },
    { "light.singlePoint", "Light", "Single point colour", "Sky.con", "Lightmanager.singlePointColor", "", LSF::Always, 0, SettingKind::Color, 0, true },
    { "light.effectSun", "Light", "Effect sun colour", "Sky.con", "Lightmanager.effectSunColor", "", LSF::Always, 0, SettingKind::Color, 0, true },
    { "light.effectShadow", "Light", "Effect shadow colour", "Sky.con", "Lightmanager.effectShadowColor", "", LSF::Always, 0, SettingKind::Color, 0, true },
    { "light.effectAffection", "Light", "Effects' light affection", "Sky.con", "Lightmanager.defaultEffectLightAffectionFactor", "", LSF::Always, 0, SettingKind::Number, 0, true },
    { "light.hemiLerpBias", "Light", "Hemi lerp bias", "Sky.con", "Lightmanager.hemilerpbias", "", LSF::Always, 0, SettingKind::Number, 0, true },
    { "light.treeAmbient", "Light", "Tree ambient colour", "Sky.con", "Lightmanager.treeAmbientColor", "", LSF::Always, 0, SettingKind::Color, 0, true },
    { "light.treeSun", "Light", "Tree sun colour", "Sky.con", "Lightmanager.treeSunColor", "", LSF::Always, 0, SettingKind::Color, 0, true },
    { "light.treeSky", "Light", "Tree sky colour", "Sky.con", "Lightmanager.treeSkyColor", "", LSF::Always, 0, SettingKind::Color, 0, true },
    // Sky.con: the fog and the sky dome
    { "fog.color", "Fog and sky", "Fog colour", "Sky.con", "Renderer.fogColor", "", LSF::Always, 0, SettingKind::Color255, 0, true },
    { "fog.range", "Fog and sky", "Fog start / end / base / density", "Sky.con", "Renderer.fogStartEndAndBase", "", LSF::Always, 0, SettingKind::Numbers, 4, true },
    { "sky.texture", "Fog and sky", "Sky texture", "Sky.con", "Skydome.skyTexture", "", LSF::Always, 0, SettingKind::Word, 0, false },
    { "sky.rotation", "Fog and sky", "Dome rotation", "Sky.con", "Skydome.domeRotation", "", LSF::Always, 0, SettingKind::Number, 0, true },
    { "sky.cloudTexture", "Fog and sky", "Cloud texture", "Sky.con", "Skydome.cloudTexture", "", LSF::Always, 0, SettingKind::Word, 0, false },
    { "sky.cloudLayer", "Fog and sky", "Cloud layer (0/1)", "Sky.con", "Skydome.hasCloudLayer", "", LSF::Always, 0, SettingKind::Integer, 0, false },
    // The rest of the stock Sky.con's dome lines (all bound in the rebuilt engine's skyDome object): the
    // templates and the second layer are written for the next load, the scroll and fade numbers run live.
    { "sky.template", "Fog and sky", "Sky dome template", "Sky.con", "Skydome.skyTemplate", "", LSF::Always, 0, SettingKind::Word, 0, false },
    { "sky.cloudTemplate", "Fog and sky", "Cloud layer template", "Sky.con", "Skydome.cloudTemplate", "", LSF::Always, 0, SettingKind::Word, 0, false },
    { "sky.cloudLayer2", "Fog and sky", "Second cloud layer (0/1)", "Sky.con", "Skydome.hasCloudLayer2", "", LSF::Always, 0, SettingKind::Integer, 0, false },
    { "sky.cloudTexture2", "Fog and sky", "Second cloud texture", "Sky.con", "Skydome.cloudTexture2", "", LSF::Always, 0, SettingKind::Word, 0, false },
    { "sky.scroll", "Fog and sky", "Cloud scroll direction", "Sky.con", "Skydome.scrolldirection", "", LSF::Always, 0, SettingKind::Numbers, 2, true },
    { "sky.scroll2", "Fog and sky", "Second layer scroll direction", "Sky.con", "Skydome.scrolldirection2", "", LSF::Always, 0, SettingKind::Numbers, 2, true },
    { "sky.cloudFade", "Fog and sky", "Cloud fade distances", "Sky.con", "Skydome.fadeCloudsDistances", "", LSF::Always, 0, SettingKind::Numbers, 2, true },
    { "sky.cloudLerp", "Fog and sky", "Cloud lerp factors", "Sky.con", "Skydome.cloudLerpFactors", "", LSF::Always, 0, SettingKind::Numbers, 2, true },
    { "sky.flareTexture", "Fog and sky", "Flare texture", "Sky.con", "Skydome.flareTexture", "", LSF::Always, 0, SettingKind::Word, 0, false },
    { "sky.flareDirection", "Fog and sky", "Flare direction", "Sky.con", "Skydome.flareDirection", "", LSF::Always, 0, SettingKind::Numbers, 3, true },
    // Water.con
    { "water.color", "Water", "Water colour", "Water.con", "renderer.waterColor", "", LSF::Always, 0, SettingKind::Color, 0, false },
    { "water.specular", "Water", "Specular colour and strength", "Water.con", "renderer.waterSpecularColor", "", LSF::Always, 0, SettingKind::Numbers, 4, false },
    { "water.specularPower", "Water", "Specular power", "Water.con", "renderer.waterSpecularPower", "", LSF::Always, 0, SettingKind::Number, 0, false },
    { "water.fogColor", "Water", "Water fog colour", "Water.con", "renderer.waterFogColor", "", LSF::Always, 0, SettingKind::Color255, 0, true },
    { "water.fogRange", "Water", "Water fog start / end / base / density", "Water.con", "renderer.waterFogStartEndAndBase", "", LSF::Always, 0, SettingKind::Numbers, 4, false },
    { "water.scroll", "Water", "Scroll", "Water.con", "renderer.waterScroll", "", LSF::Always, 0, SettingKind::Numbers, 2, false },
    { "water.animSpeed", "Water", "Animation speed", "Water.con", "renderer.waterAnimSpeed", "", LSF::Always, 0, SettingKind::Number, 0, false },
    { "water.sunIntensity", "Water", "Sun intensity on the terrain's water", "Water.con", "terrain.waterSunIntensity", "", LSF::Always, 0, SettingKind::Number, 0, true },
};

std::string trimmed(const std::string& text)
{
    size_t begin = 0;
    size_t end = text.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])))
        ++begin;
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
        --end;
    return text.substr(begin, end - begin);
}

std::vector<std::string> words(const std::string& text)
{
    std::vector<std::string> out;
    std::string current;
    for (size_t i = 0; i < text.size(); ++i)
    {
        if (std::isspace(static_cast<unsigned char>(text[i])))
        {
            if (!current.empty())
                out.push_back(current);
            current.clear();
        }
        else
            current += text[i];
    }
    if (!current.empty())
        out.push_back(current);
    return out;
}

bool isNumber(const std::string& text)
{
    if (text.empty())
        return false;
    char* end = 0;
    std::strtod(text.c_str(), &end);
    return end && *end == 0;
}

// "a/b/c" with `count` numbers.
bool isNumbers(const std::string& text, int count)
{
    int parts = 0;
    size_t start = 0;
    for (;;)
    {
        const size_t slash = text.find('/', start);
        if (!isNumber(trimmed(text.substr(start, slash == std::string::npos ? std::string::npos : slash - start))))
            return false;
        ++parts;
        if (slash == std::string::npos)
            break;
        start = slash + 1;
    }
    return parts == count;
}

std::string unquote(const std::string& text)
{
    const std::string t = trimmed(text);
    if (t.size() >= 2 && t[0] == '"' && t[t.size() - 1] == '"')
        return t.substr(1, t.size() - 2);
    return t;
}

} // namespace

const std::vector<SettingSpec>& levelSettingSpecs()
{
    static const std::vector<SettingSpec> specs(kSettings, kSettings + sizeof(kSettings) / sizeof(kSettings[0]));
    return specs;
}

const SettingSpec* findLevelSetting(const std::string& key)
{
    const std::vector<SettingSpec>& specs = levelSettingSpecs();
    for (size_t i = 0; i < specs.size(); ++i)
        if (_stricmp(specs[i].key, key.c_str()) == 0)
            return &specs[i];
    return 0;
}

std::string settingFromFile(const SettingSpec& spec, const std::string& fileValue)
{
    if (spec.kind == SettingKind::Quoted)
        return unquote(fileValue);
    if (spec.kind == SettingKind::QuotedPair)
    {
        // "a" "b" -> a b
        std::string out;
        const std::vector<std::string> parts = words(fileValue);
        for (size_t i = 0; i < parts.size(); ++i)
            out += (out.empty() ? "" : " ") + unquote(parts[i]);
        return out;
    }
    return trimmed(fileValue);
}

std::string settingToFile(const SettingSpec& spec, const std::string& value)
{
    if (spec.kind == SettingKind::Quoted)
        return "\"" + trimmed(value) + "\"";
    if (spec.kind == SettingKind::QuotedPair)
    {
        std::string out;
        const std::vector<std::string> parts = words(value);
        for (size_t i = 0; i < parts.size(); ++i)
            out += (out.empty() ? "" : " ") + ("\"" + parts[i] + "\"");
        return out;
    }
    return trimmed(value);
}

bool validSetting(const SettingSpec& spec, const std::string& value, std::string& error)
{
    const std::string v = trimmed(value);
    switch (spec.kind)
    {
    case SettingKind::Quoted:
        if (v.find('"') != std::string::npos)
        {
            error = "the text cannot hold a double quote";
            return false;
        }
        return true;
    case SettingKind::QuotedPair:
        if (words(v).size() != 2 || v.find('"') != std::string::npos)
        {
            error = "a kit is two template names: the kit, then the soldier";
            return false;
        }
        return true;
    case SettingKind::Word:
        if (v.find_first_of(" \t\"") != std::string::npos)
        {
            error = "the value is one word (a path without spaces)";
            return false;
        }
        return true;
    case SettingKind::Integer:
    {
        char* end = 0;
        std::strtol(v.c_str(), &end, 10);
        if (v.empty() || !end || *end != 0)
        {
            error = "the value is a whole number";
            return false;
        }
        return true;
    }
    case SettingKind::Number:
        if (!isNumber(v))
        {
            error = "the value is a number";
            return false;
        }
        return true;
    case SettingKind::Numbers:
        if (!isNumbers(v, spec.components))
        {
            error = "the value is " + std::to_string(spec.components) + " numbers separated by /";
            return false;
        }
        return true;
    case SettingKind::Color:
    case SettingKind::Color255:
        if (!isNumbers(v, 3))
        {
            error = "a colour is red/green/blue";
            return false;
        }
        return true;
    case SettingKind::Camera:
    {
        const std::vector<std::string> parts = words(v);
        if (parts.size() != 2 || !isNumbers(parts[0], 3) || !isNumbers(parts[1], 3))
        {
            error = "the camera is x/y/z yaw/pitch/roll";
            return false;
        }
        return true;
    }
    }
    return false;
}

} // namespace bf2editor
