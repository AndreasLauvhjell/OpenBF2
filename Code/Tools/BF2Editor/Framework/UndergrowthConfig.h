// Undergrowth.cfg system/material/plant settings, edited without the engine.
// Writes ordered system and material blocks with CRLF and ostream-style float formatting.
// Meshless textured types use CrossSize. The paired Undergrowth.raw has one material-ID byte per
// primary-heightmap sample, with 0 meaning no undergrowth.
#ifndef BF2EDITOR_FRAMEWORK_UNDERGROWTHCONFIG_H
#define BF2EDITOR_FRAMEWORK_UNDERGROWTHCONFIG_H

#include "Framework/EditorConfig.h"

#include <string>
#include <utility>
#include <vector>

namespace bf2editor
{

struct UndergrowthType
{
    std::string name;
    std::string mesh;               // "" for a cross of two quads (CrossSize)
    std::string texture;
    float       crossSize[2];
    float       randomSizeScale[2];
    float       density;
    float       variation;
    float       terrainColorScale;
    float       typeSwayScale;
    bool        skew;
    UndergrowthType();
};

struct UndergrowthMaterial
{
    std::string                  name;
    int                          id;             // the density map's value for it (1..255)
    float                        generalHeight;
    std::vector<UndergrowthType> types;
    UndergrowthMaterial() : id(0), generalHeight(0.0f) {}
};

class UndergrowthConfig
{
public:
    // The system settings in the file's order: ViewDistance, ViewDistanceFadeScale,
    // ViewDistanceHeightScale, ViewDistanceHeight2Scale, ViewDistanceStreamingScale, PatchSubdivide,
    // SwayScale, LightingScale, AlphaRef.
    std::vector<std::pair<std::string, float> > settings;
    std::vector<UndergrowthMaterial>            materials;

    // False with `error` (and the line) for a line it does not understand.
    bool parse(const std::string& text, std::string& error);
    std::string render() const;

    UndergrowthMaterial* material(const std::string& name);
    UndergrowthMaterial* materialById(int id);
    // The lowest id no material has (1..255); 0 when all are taken.
    int freeId() const;
};

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_UNDERGROWTHCONFIG_H
