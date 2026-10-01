// Compares gameplay layers by reading their scripts without executing them.
// Matches control points by names normalized without count/SP16-style tokens, then by proximity
// within mergeDistance of an existing match.
#ifndef BF2EDITOR_FRAMEWORK_LAYERSUMMARY_H
#define BF2EDITOR_FRAMEWORK_LAYERSUMMARY_H

#include "Framework/EditorConfig.h"

#include "Framework/LevelObjectFile.h"

#include <string>
#include <vector>

namespace bf2editor
{

struct LayerControlPoint
{
    std::string name;           // its template
    std::string place;          // the name without the layer's words (see above)
    int         id;             // ObjectTemplate.controlPointId; -1 when the template gives none
    int         team;           // ObjectTemplate.team; 0 = neutral
    float       radius;
    float       position[3];
};

struct LayerSpawner
{
    std::string name;
    std::string vehicle[2];     // setObjectTemplate 1 / 2 ("" for none)
    int         controlPointId; // Object.setControlPointId; -1 when none
    float       position[3];
};

struct LayerSummary
{
    std::string                                   mode;
    int                                           size;
    std::vector<LayerControlPoint>                controlPoints;
    std::vector<LayerSpawner>                     spawners;
    std::vector<std::vector<float> >              spawnPoints;    // x, y, z each
    std::vector<LevelObjectFile::CombatAreaData>  combatAreas;
};

// The layer as `file` holds it now (deleted records and blocks left out).
LayerSummary summarizeLayer(const LevelObjectFile& file, const std::string& mode, int size);

// "CPNAME_SK_64_gasstation" -> "cpname_sk_gasstation".
std::string controlPointPlace(const std::string& name);

// One place across the layers: for each layer (in `layers`' order), the index of its control point there,
// -1 when it has none. `label` is the place without the naming prefix ("gasstation").
struct ControlPointRow
{
    std::string      place;
    std::string      label;
    std::vector<int> point;
};
std::vector<ControlPointRow> matchControlPoints(const std::vector<LayerSummary>& layers, float mergeDistance);

} // namespace bf2editor

#endif // BF2EDITOR_FRAMEWORK_LAYERSUMMARY_H
