// Reads and writes Editor/GroundProperties.con terrain detail-layer and global settings.
// Writing uses CRLF, indexed layer order, blank separators, and default stream float
// formatting. Parsing does not retain unrelated commands.
#ifndef BF2EDITOR_FRAMEWORK_GROUNDPROPERTIESFILE_H
#define BF2EDITOR_FRAMEWORK_GROUNDPROPERTIESFILE_H

#include "Framework/EditorConfig.h"

#include <map>
#include <string>
#include <vector>

namespace bf2editor
{

struct GroundLayer
{
    std::string detailTexture;          // "<relative path>.dds"; empty = not set
    std::string colorTexture;
    int         color[3]   = { 0, 0, 0 };
    std::string material;               // a material name; empty = none found
    float       topTiling  = 0.0f;
    float       sideTiling[2] = { 0.0f, 0.0f };
    float       yOffset    = 0.0f;
    int         planeMap   = 0;
    float       colorTiling[2] = { 0.0f, 0.0f };
    int         lowDetailType = 0;
    bool        envMap     = false;
    bool operator==(const GroundLayer& other) const;
};

struct GroundMisc
{
    float farTopTilingHi    = 0.0f;
    float farTopTilingLow   = 0.0f;
    float farSideTiling[2]  = { 0.0f, 0.0f };
    float farYOffset        = 0.0f;
    float lodSwitchDistance = 0.0f;
    bool operator==(const GroundMisc& other) const;
};

class GroundProperties
{
public:
    std::map<int, GroundLayer> layers;  // the layers present (the plugin's tree), by index
    GroundMisc                 misc;

    std::string render() const;
    // False with `error` naming the first line it could not read.
    bool parse(const std::string& text, std::string& error);

    // The settings by name, as `EditorGround.set` names them: detailTexture, colorTexture, color (r/g/b), material,
    // topTiling, sideTiling (x/y), yOffset, planeMap, colorTiling (x/y), lowDetailType, envMap (0/1); and the Misc ones:
    // farTopTilingHi, farTopTilingLow, farSideTiling (x/y), farYOffset, lodSwitchDistance. Values in the file's form.
    static const std::vector<std::string>& layerSettings();
    static const std::vector<std::string>& miscSettings();
    bool setLayer(int layer, const std::string& name, const std::string& value, std::string& error);
    bool getLayer(int layer, const std::string& name, std::string& value) const;
    bool setMisc(const std::string& name, const std::string& value, std::string& error);
    bool getMisc(const std::string& name, std::string& value) const;

    // The default ostream form of a float (as the plugin wrote it: "24", "0.5", "5/5").
    static std::string number(float value);
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_GROUNDPROPERTIESFILE_H
