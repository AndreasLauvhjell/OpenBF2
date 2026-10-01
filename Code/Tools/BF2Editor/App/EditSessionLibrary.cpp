// Places bundles and paints/erases level brushes, with one undo entry per placement or stroke.
// Offsets are relative to the bundle root; selection brushes retain object rotations and relative offsets.
// One seeded generator supplies each stamp. Ground snapping uses each object's anchor after rotation.
// These editor choices intentionally differ from the retail placement and undo behavior.
#include "Framework/EditorConfig.h"
#include "App/EditSession.h"

#include "App/EditorLevel.h"
#include "App/LevelDocument.h"
#include "Framework/EditorLog.h"
#include "Framework/Matrix4.h"

#include "Game/Objects/IObject.h"
#include "Game/Objects/IObjectTemplate.h"
#include "Scene/Object/ObjectManager.h"
#include "Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>

using dice::hfe::Mat4;
using dice::hfe::Vec3;
using dice::hfe::world::IObject;

namespace bf2editor
{

namespace
{

std::string lowerName(const std::string& text)
{
    std::string out(text);
    for (size_t i = 0; i < out.size(); ++i)
        out[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(out[i])));
    return out;
}

bool sameName(const std::string& a, const std::string& b)
{
    return _stricmp(a.c_str(), b.c_str()) == 0;
}

std::string vectorText(const float v[3], int decimals)
{
    char text[128];
    std::snprintf(text, sizeof(text), "%.*f/%.*f/%.*f", decimals, v[0], decimals, v[1], decimals, v[2]);
    return text;
}

// "x/y/z", as the console reads a Vec3.
bool parseVector(const std::string& text, float out[3])
{
    const size_t first = text.find('/');
    const size_t second = first == std::string::npos ? std::string::npos : text.find('/', first + 1);
    if (second == std::string::npos || text.find('/', second + 1) != std::string::npos)
        return false;
    const std::string parts[3] = { text.substr(0, first), text.substr(first + 1, second - first - 1),
                                   text.substr(second + 1) };
    for (int i = 0; i < 3; ++i)
    {
        char* end = 0;
        if (parts[i].empty())
            return false;
        out[i] = static_cast<float>(std::strtod(parts[i].c_str(), &end));
        if (!end || *end != 0)
            return false;
    }
    return true;
}

Vec3 vec3(const float v[3])
{
    Vec3 out;
    out.x = v[0];
    out.y = v[1];
    out.z = v[2];
    return out;
}

Mat4 identity()
{
    Mat4 m;
    for (int i = 0; i < 16; ++i)
        m.m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    return m;
}

// A rotation as Object.rotation takes it (yaw / pitch / roll, degrees), no translation.
Mat4 rotationMatrix(const float yawPitchRoll[3])
{
    Mat4 m = identity();
    EditSession::setRotation(m, vec3(yawPitchRoll));
    return m;
}

// out = the 3x3 block of a times the 3x3 block of b (row vectors: first a, then b); no translation.
Mat4 rotationProduct(const Mat4& a, const Mat4& b)
{
    Mat4 out;
    matrixMultiply(a.m, b.m, out.m);
    out.m[12] = out.m[13] = out.m[14] = 0.0f;
    return out;
}

// v (a row vector) through the 3x3 block of m.
void rotateVector(const float v[3], const Mat4& m, float out[3])
{
    for (int i = 0; i < 3; ++i)
        out[i] = v[0] * m.m[i] + v[1] * m.m[4 + i] + v[2] * m.m[8 + i];
}

bool templateExists(const std::string& name)
{
    return dice::hfe::world::objectTemplateManager && dice::hfe::world::objectTemplateManager->getTemplate(name);
}

// The templates of a list no level template has, as "a, b".
std::string missingTemplates(const std::vector<std::string>& names)
{
    std::string missing;
    std::set<std::string> seen;
    for (size_t i = 0; i < names.size(); ++i)
    {
        if (templateExists(names[i]) || !seen.insert(lowerName(names[i])).second)
            continue;
        missing += (missing.empty() ? "" : ", ") + names[i];
    }
    return missing;
}

std::string brushTemplatesText(const LevelBrushFile& brush)
{
    std::string text;
    for (size_t i = 0; i < brush.items.size(); ++i)
        text += (i ? ", " : "") + brush.items[i].templateName;
    return text.empty() ? std::string("no items") : text;
}

} // namespace

// The library

std::vector<std::string> EditSession::preloadLines(const std::vector<std::string>& templates,
                                                   const std::vector<std::string>& keep)
{
    // What the retail writers print: "run <file>" when the template's file starts with '/', else
    // "run /<file>", each file once. A template this level does not load keeps the line the file had.
    std::vector<std::string> lines;
    std::set<std::string> done;
    for (size_t i = 0; i < templates.size(); ++i)
    {
        if (!done.insert(lowerName(templates[i])).second)
            continue;
        std::string line;
        std::string path = dice::hfe::world::objectTemplateManager
                               ? dice::hfe::world::objectTemplateManager->getWorkingFolderAndFile(templates[i])
                               : std::string();
        if (!path.empty())
        {
            for (size_t k = 0; k < path.size(); ++k)
                if (path[k] == '\\')
                    path[k] = '/';
            line = "run " + (path[0] == '/' ? path : "/" + path);
        }
        else
        {
            const std::string tail = lowerName("/" + templates[i] + ".con");
            for (size_t k = 0; k < keep.size() && line.empty(); ++k)
            {
                const std::string kept = lowerName(keep[k]);
                if (kept.size() >= tail.size() && kept.compare(kept.size() - tail.size(), tail.size(), tail) == 0)
                    line = keep[k];
            }
        }
        if (!line.empty() && std::find(lines.begin(), lines.end(), line) == lines.end())
            lines.push_back(line);
    }
    return lines;
}

bool EditSession::libraryInfo(std::string& result)
{
    const std::vector<std::string> bundles = m_library.bundleNames();
    const std::vector<LevelBrushFile>& brushes = m_library.brushes();
    char text[160];
    std::snprintf(text, sizeof(text), ": %u bundles, %u brushes", static_cast<unsigned>(bundles.size()),
                  static_cast<unsigned>(brushes.size()));
    result = "library " + m_library.folder() + text;
    if (!m_activeBrush.empty())
        result += "; the active brush is " + m_activeBrush;
    const std::vector<std::string>& problems = m_library.brushProblems();
    for (size_t i = 0; i < problems.size(); ++i)
        result += "; not read: " + problems[i];
    return true;
}

bool EditSession::setLibraryFolder(const std::string& folder, std::string& result)
{
    if (folder.empty())
    {
        result = "EditorLibrary.folder takes a folder, or 'default' for " + std::string(ObjectLibrary::defaultFolder());
        return false;
    }
    if (m_brushStroke)
        endBrushStroke();
    m_library.setFolder(sameName(folder, "default") ? std::string() : folder);
    m_activeBrush.clear();
    m_armedBundle.clear();
    return libraryInfo(result);
}

// Bundles

bool EditSession::listBundles(std::string& result)
{
    const std::vector<std::string> names = m_library.bundleNames();
    if (names.empty())
    {
        result = "no bundles in " + m_library.folder() + "/ObjectBundle";
        return true;
    }
    result.clear();
    for (size_t i = 0; i < names.size(); ++i)
        result += (i ? ", " : "") + names[i];
    return true;
}

bool EditSession::bundleInfo(const std::string& name, std::string& result)
{
    ObjectBundleFile bundle;
    if (!m_library.loadBundle(name, bundle, result))
        return false;
    char text[64];
    std::snprintf(text, sizeof(text), "%s: %u objects; root ", name.c_str(), static_cast<unsigned>(bundle.records.size()));
    result = text;
    for (size_t i = 0; i < bundle.records.size(); ++i)
    {
        const BundleRecord& r = bundle.records[i];
        std::snprintf(text, sizeof(text), " layer %d", r.layer);
        result += (i ? "; " : "") + r.templateName + (i ? " at " + vectorText(r.offset, 2) : std::string()) + " rotation "
                + vectorText(r.rotation, 2) + text;
    }
    std::vector<std::string> templates;
    for (size_t i = 0; i < bundle.records.size(); ++i)
        templates.push_back(bundle.records[i].templateName);
    const std::string missing = missingTemplates(templates);
    if (!missing.empty())
        result += "; this level has no template " + missing;
    return true;
}

bool EditSession::saveBundle(const std::string& name, bool overwrite, std::string& result)
{
    if (!validLibraryName(name))
    {
        result = "'" + name + "' is not a bundle name (letters, digits, _ and -)";
        return false;
    }
    if (m_selection.size() < 2)
    {
        result = m_selection.empty() ? "nothing is selected: a bundle is two or more objects"
                                     : "one object is selected: a bundle is two or more objects";
        return false;
    }
    if (!overwrite && m_library.bundleExists(name))
    {
        result = "a bundle named " + name + " exists already (EditorBundle.saveOver replaces it)";
        return false;
    }
    // The primary object is the root; the others follow in the order they were selected.
    const unsigned rootId = m_selection.primary();
    std::vector<unsigned> ids(1, rootId);
    for (size_t i = 0; i < m_selection.ids().size(); ++i)
        if (m_selection.ids()[i] != rootId)
            ids.push_back(m_selection.ids()[i]);
    Mat4 root;
    float inverse[16];
    if (!transformOf(rootId, root) || !matrixInvertAffine(root.m, inverse))
    {
        result = describe(rootId) + " has no usable matrix";
        return false;
    }
    ObjectBundleFile bundle;
    std::vector<std::string> templates;
    for (size_t i = 0; i < ids.size(); ++i)
    {
        // Placing a bundle makes StaticObjects.con objects, so only those can go into one.
        if (!copyable(ids[i], result))
            return false;
        IObject* o = object(ids[i]);
        const Mat4& m = o->getAbsoluteTransformation();
        BundleRecord record;
        record.templateName = o->getTemplate()->getName();
        if (i > 0)
            for (int k = 0; k < 3; ++k)
                record.offset[k] = m.m[12] * inverse[k] + m.m[13] * inverse[4 + k] + m.m[14] * inverse[8 + k]
                                 + inverse[12 + k];
        const Vec3 angles = rotationOf(m);
        record.rotation[0] = angles.x;
        record.rotation[1] = angles.y;
        record.rotation[2] = angles.z;
        record.layer = static_cast<int>(o->getLayer());
        bundle.records.push_back(record);
        templates.push_back(record.templateName);
    }
    bundle.preload = preloadLines(templates, std::vector<std::string>());
    std::string error;
    if (!m_library.saveBundle(name, bundle, overwrite, error))
    {
        result = error;
        return false;
    }
    char text[96];
    std::snprintf(text, sizeof(text), "saved the bundle %s: %u objects, root ", name.c_str(),
                  static_cast<unsigned>(bundle.records.size()));
    result = text + bundle.records[0].templateName + ", in " + m_library.bundlePath(name);
    logLine(LogLevel::Info, "bundle: %s", result.c_str());
    return true;
}

int EditSession::freeGroup() const
{
    std::set<int> used;
    if (dice::hfe::world::objectManager)
    {
        const std::map<unsigned int, IObject*>& roots = dice::hfe::world::objectManager->getAllObjectsFromGrid();
        for (std::map<unsigned int, IObject*>::const_iterator it = roots.begin(); it != roots.end(); ++it)
            if (it->second)
                used.insert(static_cast<int>(it->second->getGroup()));
    }
    for (int group = 1; group <= 255; ++group)
        if (!used.count(group))
            return group;
    return 0;
}

bool EditSession::placeBundle(const std::string& name, const Vec3& at, float yaw, std::string& result)
{
    if (!m_document || !m_document->isOpen())
    {
        result = "the level's objects file is not open, so a bundle could not be saved with it: refused";
        return false;
    }
    ObjectBundleFile bundle;
    if (!m_library.loadBundle(name, bundle, result))
        return false;
    std::vector<std::string> templates;
    for (size_t i = 0; i < bundle.records.size(); ++i)
        templates.push_back(bundle.records[i].templateName);
    const std::string missing = missingTemplates(templates);
    if (!missing.empty())
    {
        result = "the bundle " + name + " needs templates this level does not load: " + missing + "; nothing was placed";
        return false;
    }
    // The root at the point, turned by its saved rotation and then by `yaw` about the vertical; each member
    // at the root's offset carried through both turns, with its own rotation turned by `yaw` too.
    const float turnAngles[3] = { yaw, 0.0f, 0.0f };
    const Mat4 turn = rotationMatrix(turnAngles);
    const Mat4 rootRotation = rotationMatrix(bundle.records[0].rotation);
    const Mat4 rootFrame = rotationProduct(rootRotation, turn);
    std::vector<NewObject> objects;
    for (size_t i = 0; i < bundle.records.size(); ++i)
    {
        const BundleRecord& record = bundle.records[i];
        NewObject item;
        item.templateName = record.templateName;
        item.layer = record.layer;
        item.transform = rotationProduct(rotationMatrix(record.rotation), turn);
        float offset[3];
        rotateVector(record.offset, rootFrame, offset);
        item.transform.m[12] = at.x + offset[0];
        item.transform.m[13] = at.y + offset[1];
        item.transform.m[14] = at.z + offset[2];
        objects.push_back(item);
    }
    std::vector<unsigned> made;
    std::string error;
    if (!makeObjects(objects, made, error))
    {
        result = "placing the bundle " + name + ": " + error;
        return false;
    }
    int group = 0;
    std::string groupText;
    if (m_bundleGrouping && made.size() >= 2)
    {
        group = freeGroup();
        if (group == 0)
            groupText = "; not grouped: every group 1 to 255 is in use";
        for (size_t i = 0; group != 0 && i < made.size(); ++i)
        {
            ObjectProperties p = propertiesOf(object(made[i]));
            p.group = group;
            if (!runConsoleScript(propertiesScript(made[i], p), error))
                logLine(LogLevel::Warning, "bundle: grouping %s: %s", describe(made[i]).c_str(), error.c_str());
        }
    }
    HistoryEntry entry;
    madeScripts(made, entry.doScript, entry.undoScript);
    char label[160];
    std::snprintf(label, sizeof(label), "Place the bundle %s (%u objects)", name.c_str(), static_cast<unsigned>(made.size()));
    entry.label = label;
    // Made already (their ids were needed for the undo), so the history records the placing as done.
    m_history.record(entry);
    m_selection.clear();
    for (size_t i = made.size(); i-- > 1;)
        m_selection.add(made[i]);
    m_selection.add(made[0]);                   // the root is the primary
    if (group)
    {
        char text[48];
        std::snprintf(text, sizeof(text), "; group %d", group);
        groupText = text;
    }
    logLine(LogLevel::Info, "edit: %s%s", entry.label.c_str(), groupText.c_str());
    result = entry.label + groupText;
    return true;
}

bool EditSession::placeBundleHere(const std::string& name, float yaw, std::string& result)
{
    float point[3];
    pointAtViewCentre(point);
    Vec3 at;
    at.x = point[0];
    at.y = point[1];
    at.z = point[2];
    return placeBundle(name, at, yaw, result);
}

bool EditSession::deleteBundle(const std::string& name, std::string& result)
{
    if (!m_library.deleteBundle(name, result))
        return false;
    if (sameName(m_armedBundle, name))
        m_armedBundle.clear();
    result = "deleted the bundle " + name;
    return true;
}

bool EditSession::setBundleGroup(bool group, std::string& result)
{
    m_bundleGrouping = group;
    result = group ? "a placed bundle gets a group of its own" : "a placed bundle is not grouped";
    return true;
}

bool EditSession::groupSelection(std::string& result)
{
    if (m_selection.empty())
    {
        result = "nothing is selected";
        return false;
    }
    const int group = freeGroup();
    if (group == 0)
    {
        result = "every group 1 to 255 is in use: refused";
        return false;
    }
    char value[16];
    std::snprintf(value, sizeof(value), "%d", group);
    return setSelectionProperty("group", value, result);
}

bool EditSession::ungroupSelection(std::string& result)
{
    if (m_selection.empty())
    {
        result = "nothing is selected";
        return false;
    }
    return setSelectionProperty("group", "0", result);
}

bool EditSession::selectGroup(std::string& result)
{
    IObject* primary = object(m_selection.primary());
    if (!primary)
    {
        result = "nothing is selected";
        return false;
    }
    const int group = static_cast<int>(primary->getGroup());
    if (group == 0)
    {
        result = describe(m_selection.primary()) + " is in no group";
        return false;
    }
    const std::map<unsigned int, IObject*>& roots = dice::hfe::world::objectManager->getAllObjectsFromGrid();
    const unsigned keep = m_selection.primary();
    m_selection.clear();
    for (std::map<unsigned int, IObject*>::const_iterator it = roots.begin(); it != roots.end(); ++it)
        if (it->second && static_cast<int>(it->second->getGroup()) == group && it->first != keep)
            m_selection.add(it->first);
    m_selection.add(keep);
    char text[96];
    std::snprintf(text, sizeof(text), "selected the %u objects of group %d", static_cast<unsigned>(m_selection.size()), group);
    result = text;
    return true;
}

// Brushes

LevelBrushFile* EditSession::activeBrushFile()
{
    return m_activeBrush.empty() ? 0 : m_library.brush(m_activeBrush);
}

bool EditSession::writeBrush(LevelBrushFile& brush, std::string& error)
{
    std::vector<std::string> templates;
    for (size_t i = 0; i < brush.items.size(); ++i)
        templates.push_back(brush.items[i].templateName);
    brush.preload = preloadLines(templates, brush.preload);
    return m_library.writeBrush(brush.name, error);
}

bool EditSession::listBrushes(std::string& result)
{
    const std::vector<LevelBrushFile>& brushes = m_library.brushes();
    result.clear();
    for (size_t i = 0; i < brushes.size(); ++i)
    {
        char count[48];
        std::snprintf(count, sizeof(count), " (%u items)", static_cast<unsigned>(brushes[i].items.size()));
        result += (i ? ", " : "") + std::string(sameName(brushes[i].name, m_activeBrush) ? "*" : "") + brushes[i].name + count;
    }
    if (brushes.empty())
        result = "no brushes in " + m_library.folder() + "/LevelBrushes";
    const std::vector<std::string>& problems = m_library.brushProblems();
    for (size_t i = 0; i < problems.size(); ++i)
        result += "; not read: " + problems[i];
    return true;
}

bool EditSession::brushInfo(const std::string& name, std::string& result)
{
    const LevelBrushFile* brush = m_library.brush(name.empty() ? m_activeBrush : name);
    if (!brush)
    {
        result = name.empty() ? std::string("no active brush") : "no brush named " + name;
        return false;
    }
    char text[160];
    std::snprintf(text, sizeof(text), ": spacing %g m, snap to the ground %d; random position ", brush->spacing,
                  brush->snapToGround ? 1 : 0);
    result = brush->name + text + vectorText(brush->randomPositionMin, 2) + " to " + vectorText(brush->randomPositionMax, 2)
           + ", rotation " + vectorText(brush->randomRotationMin, 2) + " to " + vectorText(brush->randomRotationMax, 2);
    if (brush->rotationRangesCopied)
        result += " (they repeat the position ranges: a file of the retail editor, whose save wrote them so)";
    for (size_t i = 0; i < brush->items.size(); ++i)
    {
        std::snprintf(text, sizeof(text), "; item %u ", static_cast<unsigned>(i));
        result += text + brush->items[i].templateName + " at " + vectorText(brush->items[i].position, 2) + " rotation "
                + vectorText(brush->items[i].rotation, 2);
    }
    std::vector<std::string> templates;
    for (size_t i = 0; i < brush->items.size(); ++i)
        templates.push_back(brush->items[i].templateName);
    const std::string missing = missingTemplates(templates);
    if (!missing.empty())
        result += "; this level has no template " + missing;
    return true;
}

bool EditSession::selectBrush(const std::string& name, std::string& result)
{
    const LevelBrushFile* brush = m_library.brush(name);
    if (!brush)
    {
        result = "no brush named " + name;
        return false;
    }
    if (m_brushStroke)
        endBrushStroke();
    m_activeBrush = brush->name;
    result = "the active brush is " + m_activeBrush + " (" + brushTemplatesText(*brush) + ")";
    return true;
}

bool EditSession::defineBrush(const std::string& name, std::string& result)
{
    if (m_selection.empty())
    {
        result = "nothing is selected: a brush is made from the selected objects";
        return false;
    }
    if (!validLibraryName(name))
    {
        result = "'" + name + "' is not a brush name (letters, digits, _ and -)";
        return false;
    }
    if (m_library.brush(name))
    {
        result = "a brush named " + name + " exists already";
        return false;
    }
    // The primary object first, at offset 0; each object keeps its offset from it (world axes, as a stamp
    // applies them) and its own rotation.
    const unsigned primaryId = m_selection.primary();
    Mat4 primary;
    if (!transformOf(primaryId, primary))
    {
        result = "nothing is selected";
        return false;
    }
    std::vector<unsigned> ids(1, primaryId);
    for (size_t i = 0; i < m_selection.ids().size(); ++i)
        if (m_selection.ids()[i] != primaryId)
            ids.push_back(m_selection.ids()[i]);
    LevelBrushFile brush;
    brush.name = name;
    for (size_t i = 0; i < ids.size(); ++i)
    {
        if (!copyable(ids[i], result))
            return false;
        IObject* o = object(ids[i]);
        const Mat4& m = o->getAbsoluteTransformation();
        LevelBrushItem item;
        item.templateName = o->getTemplate()->getName();
        for (int k = 0; k < 3; ++k)
            item.position[k] = m.m[12 + k] - primary.m[12 + k];
        const Vec3 angles = rotationOf(m);
        item.rotation[0] = angles.x;
        item.rotation[1] = angles.y;
        item.rotation[2] = angles.z;
        brush.items.push_back(item);
    }
    std::vector<std::string> templates;
    for (size_t i = 0; i < brush.items.size(); ++i)
        templates.push_back(brush.items[i].templateName);
    brush.preload = preloadLines(templates, std::vector<std::string>());
    std::string error;
    if (!m_library.addBrush(brush, error))
    {
        result = error;
        return false;
    }
    m_activeBrush = name;
    char text[128];
    std::snprintf(text, sizeof(text), "defined the brush %s with %u items (the active brush now)", name.c_str(),
                  static_cast<unsigned>(brush.items.size()));
    result = text;
    logLine(LogLevel::Info, "brush: %s", result.c_str());
    return true;
}

bool EditSession::addSelectionToBrush(std::string& result)
{
    LevelBrushFile* brush = activeBrushFile();
    if (!brush)
    {
        result = "no active brush: define or select one";
        return false;
    }
    if (m_selection.empty())
    {
        result = "nothing is selected";
        return false;
    }
    Mat4 primary;
    transformOf(m_selection.primary(), primary);
    std::vector<LevelBrushItem> added;
    for (size_t i = 0; i < m_selection.ids().size(); ++i)
    {
        const unsigned id = m_selection.ids()[i];
        if (!copyable(id, result))
            return false;
        IObject* o = object(id);
        const Mat4& m = o->getAbsoluteTransformation();
        LevelBrushItem item;
        item.templateName = o->getTemplate()->getName();
        for (int k = 0; k < 3; ++k)
            item.position[k] = m.m[12 + k] - primary.m[12 + k];
        const Vec3 angles = rotationOf(m);
        item.rotation[0] = angles.x;
        item.rotation[1] = angles.y;
        item.rotation[2] = angles.z;
        added.push_back(item);
    }
    const LevelBrushFile before = *brush;
    brush->items.insert(brush->items.end(), added.begin(), added.end());
    std::string error;
    if (!writeBrush(*brush, error))
    {
        *brush = before;
        result = error;
        return false;
    }
    char text[128];
    std::snprintf(text, sizeof(text), "added %u items to the brush %s (%u now)", static_cast<unsigned>(added.size()),
                  brush->name.c_str(), static_cast<unsigned>(brush->items.size()));
    result = text;
    return true;
}

bool EditSession::addBrushItem(const std::string& templateName, std::string& result)
{
    LevelBrushFile* brush = activeBrushFile();
    if (!brush)
    {
        result = "no active brush: define or select one";
        return false;
    }
    if (!templateExists(templateName))
    {
        result = "no template named '" + templateName + "' in this level";
        return false;
    }
    const LevelBrushFile before = *brush;
    LevelBrushItem item;
    item.templateName = dice::hfe::world::objectTemplateManager->getTemplate(templateName)->getName();
    brush->items.push_back(item);
    std::string error;
    if (!writeBrush(*brush, error))
    {
        *brush = before;
        result = error;
        return false;
    }
    result = "added " + item.templateName + " to the brush " + brush->name;
    return true;
}

bool EditSession::setBrushProperty(const std::string& key, const std::string& value, std::string& result)
{
    LevelBrushFile* brush = activeBrushFile();
    if (!brush)
    {
        result = "no active brush: define or select one";
        return false;
    }
    const LevelBrushFile before = *brush;
    struct Range { const char* key; float* target; };
    const Range ranges[] = {
        { "randomPositionMin", brush->randomPositionMin }, { "randomPositionMax", brush->randomPositionMax },
        { "randomRotationMin", brush->randomRotationMin }, { "randomRotationMax", brush->randomRotationMax },
        { "randomScaleMin", brush->randomScaleMin },       { "randomScaleMax", brush->randomScaleMax },
    };
    bool known = false;
    for (size_t i = 0; i < sizeof(ranges) / sizeof(ranges[0]); ++i)
    {
        if (!sameName(key, ranges[i].key))
            continue;
        if (!parseVector(value, ranges[i].target))
        {
            result = key + " is x/y/z";
            return false;
        }
        known = true;
    }
    if (sameName(key, "spacing"))
    {
        char* end = 0;
        const double spacing = std::strtod(value.c_str(), &end);
        if (value.empty() || !end || *end != 0 || !(spacing >= 0.1 && spacing <= 1000.0))
        {
            result = "the spacing is 0.1 to 1000 metres";
            return false;
        }
        brush->spacing = static_cast<float>(spacing);
        known = true;
    }
    else if (sameName(key, "snapToGround"))
    {
        if (value != "0" && value != "1")
        {
            result = "snapToGround is 0 or 1";
            return false;
        }
        brush->snapToGround = value == "1";
        known = true;
    }
    if (!known)
    {
        result = "no brush setting '" + key + "': spacing, snapToGround, randomPositionMin, randomPositionMax, "
                 "randomRotationMin, randomRotationMax, randomScaleMin or randomScaleMax";
        return false;
    }
    brush->rotationRangesCopied = false;
    std::string error;
    if (!writeBrush(*brush, error))
    {
        *brush = before;
        result = error;
        return false;
    }
    result = brush->name + ": " + key + " " + value;
    return true;
}

bool EditSession::setBrushItem(int index, const std::string& key, const std::string& value, std::string& result)
{
    LevelBrushFile* brush = activeBrushFile();
    if (!brush)
    {
        result = "no active brush: define or select one";
        return false;
    }
    if (index < 0 || index >= static_cast<int>(brush->items.size()))
    {
        char text[96];
        std::snprintf(text, sizeof(text), "the brush %s has items 0 to %d", brush->name.c_str(),
                      static_cast<int>(brush->items.size()) - 1);
        result = brush->items.empty() ? "the brush " + brush->name + " has no items" : std::string(text);
        return false;
    }
    const LevelBrushFile before = *brush;
    LevelBrushItem& item = brush->items[static_cast<size_t>(index)];
    float* target = sameName(key, "position") ? item.position
                  : sameName(key, "rotation") ? item.rotation
                  : sameName(key, "scale") ? item.scale : 0;
    if (target)
    {
        if (!parseVector(value, target))
        {
            result = key + " is x/y/z";
            return false;
        }
    }
    else if (sameName(key, "template"))
    {
        if (!templateExists(value))
        {
            result = "no template named '" + value + "' in this level";
            return false;
        }
        item.templateName = dice::hfe::world::objectTemplateManager->getTemplate(value)->getName();
    }
    else
    {
        result = "an item has a position, rotation, scale (x/y/z) and template";
        return false;
    }
    std::string error;
    if (!writeBrush(*brush, error))
    {
        *brush = before;
        result = error;
        return false;
    }
    char text[64];
    std::snprintf(text, sizeof(text), ": item %d ", index);
    result = brush->name + text + key + " " + value;
    return true;
}

bool EditSession::removeBrushItem(int index, std::string& result)
{
    LevelBrushFile* brush = activeBrushFile();
    if (!brush)
    {
        result = "no active brush: define or select one";
        return false;
    }
    if (index < 0 || index >= static_cast<int>(brush->items.size()))
    {
        result = "no such item";
        return false;
    }
    const LevelBrushFile before = *brush;
    const std::string removed = brush->items[static_cast<size_t>(index)].templateName;
    brush->items.erase(brush->items.begin() + index);
    std::string error;
    if (!writeBrush(*brush, error))
    {
        *brush = before;
        result = error;
        return false;
    }
    result = "removed " + removed + " from the brush " + brush->name;
    return true;
}

bool EditSession::deleteBrush(const std::string& name, std::string& result)
{
    const LevelBrushFile* brush = m_library.brush(name);
    if (!brush)
    {
        result = "no brush named " + name;
        return false;
    }
    const std::string actual = brush->name;
    const bool active = sameName(actual, m_activeBrush);
    if (active)
        setBrushToolActive(false);          // ends a stroke under way
    if (!m_library.deleteBrush(actual, result))
        return false;
    if (active)
        m_activeBrush.clear();
    result = "deleted the brush " + actual + " and its file";
    return true;
}

bool EditSession::seedBrush(unsigned seed, std::string& result)
{
    m_random.seed(seed);
    char text[64];
    std::snprintf(text, sizeof(text), "the brushes' random numbers start from %u", m_random.state());
    result = text;
    return true;
}

void EditSession::setBrushToolActive(bool active)
{
    if (!active && m_brushStroke)
        endBrushStroke();
    // One tool has the mouse at a time: the terrain brush and the undergrowth tool go off when the object
    // brush comes on.
    if (active && m_terrainTool)
        setTerrainToolActive(false);
    if (active && m_undergrowthTool)
        setUndergrowthToolActive(false);
    m_brushTool = active;
}

bool EditSession::setBrushTool(bool active, std::string& result)
{
    if (active && !activeBrushFile())
    {
        result = "no active brush: define one from the selection or select one";
        return false;
    }
    setBrushToolActive(active);
    result = active ? "the object brush " + m_activeBrush + " is on: the left button paints, Shift+left erases"
                    : std::string("the object brush is off");
    return true;
}

bool EditSession::setBrushEraseRadius(float radius, std::string& result)
{
    if (!(radius >= 0.5f && radius <= 500.0f))
    {
        result = "the erase radius is 0.5 to 500 metres";
        return false;
    }
    m_brushEraseRadius = radius;
    char text[64];
    std::snprintf(text, sizeof(text), "the erase radius is %g m", radius);
    result = text;
    return true;
}

bool EditSession::beginBrushStroke(bool erase, std::string& error)
{
    if (m_brushStroke)
        endBrushStroke();
    if (!m_document || !m_document->isOpen())
    {
        error = "the level's objects file is not open, so painted objects could not be saved: refused";
        return false;
    }
    LevelBrushFile* brush = activeBrushFile();
    if (!brush)
    {
        error = "no active brush: define one from the selection or select one";
        return false;
    }
    if (brush->items.empty())
    {
        error = "the brush " + brush->name + " has no items";
        return false;
    }
    std::vector<std::string> templates;
    for (size_t i = 0; i < brush->items.size(); ++i)
        templates.push_back(brush->items[i].templateName);
    const std::string missing = missingTemplates(templates);
    if (!erase && !missing.empty())
    {
        error = "the brush " + brush->name + " needs templates this level does not load: " + missing;
        return false;
    }
    m_brushStroke = true;
    m_brushStrokeErase = erase;
    m_haveLastStamp = false;
    m_strokeMade.clear();
    m_strokeRemoved.clear();
    m_strokeRecreate.clear();
    m_strokeSkipped = 0;
    return true;
}

bool EditSession::stampBrush(const float at[3], std::vector<unsigned>& made, std::string& error)
{
    LevelBrushFile* brush = activeBrushFile();
    if (!brush)
    {
        error = "no active brush";
        return false;
    }
    std::vector<NewObject> objects;
    for (size_t i = 0; i < brush->items.size(); ++i)
    {
        const LevelBrushItem& item = brush->items[i];
        // Six numbers per item, in the retail order: the position's x, y, z, then the rotation's.
        float randomPosition[3], randomRotation[3];
        for (int k = 0; k < 3; ++k)
            randomPosition[k] = m_random.between(brush->randomPositionMin[k], brush->randomPositionMax[k]);
        for (int k = 0; k < 3; ++k)
            randomRotation[k] = m_random.between(brush->randomRotationMin[k], brush->randomRotationMax[k]);
        float position[3], rotation[3];
        for (int k = 0; k < 3; ++k)
        {
            position[k] = at[k] + item.position[k] + randomPosition[k];
            rotation[k] = item.rotation[k] + randomRotation[k];
        }
        if (brush->snapToGround && !m_level.groundHeight(position[0], position[2], position[1]))
        {
            ++m_strokeSkipped;          // off the heightmap: nothing to stand on
            continue;
        }
        if (position[1] == 0.0f)
        {
            ++m_strokeSkipped;          // the engine's culler never takes an object at exactly y = 0
            continue;
        }
        NewObject object;
        object.templateName = item.templateName;
        object.layer = 1;
        object.transform = rotationMatrix(rotation);
        object.transform.m[12] = position[0];
        object.transform.m[13] = position[1];
        object.transform.m[14] = position[2];
        objects.push_back(object);
    }
    if (objects.empty())
        return true;
    std::vector<unsigned> newly;
    if (!makeObjects(objects, newly, error))
        return false;
    // Each object stood on the ground by its anchor, now that it exists (the snap reads its template's
    // anchor, or its bounding box, from the object).
    for (size_t i = 0; brush->snapToGround && i < newly.size(); ++i)
    {
        IObject* o = object(newly[i]);
        float anchor[3], ground = 0.0f;
        if (!o || !worldAnchorOf(newly[i], anchor) || !m_level.groundHeight(anchor[0], anchor[2], ground))
            continue;
        Mat4 m = o->getAbsoluteTransformation();
        const float y = snapPivotHeight(o, ground);
        if (y == m.m[13] || y == 0.0f)
            continue;
        m.m[13] = y;
        std::string why;
        if (!setTransformNow(newly[i], m, why))
            logLine(LogLevel::Warning, "brush: the snap of %s did not apply: %s", describe(newly[i]).c_str(), why.c_str());
    }
    made.insert(made.end(), newly.begin(), newly.end());
    return true;
}

bool EditSession::eraseBrush(const float at[3], std::string& error)
{
    LevelBrushFile* brush = activeBrushFile();
    if (!brush || !dice::hfe::world::objectManager)
    {
        error = "no active brush";
        return false;
    }
    std::set<std::string> templates;
    for (size_t i = 0; i < brush->items.size(); ++i)
        templates.insert(lowerName(brush->items[i].templateName));
    // The objects first, then their removal: removing changes the map being walked.
    std::vector<unsigned> doomed;
    const std::map<unsigned int, IObject*>& roots = dice::hfe::world::objectManager->getAllObjectsFromGrid();
    for (std::map<unsigned int, IObject*>::const_iterator it = roots.begin(); it != roots.end(); ++it)
    {
        IObject* o = it->second;
        if (!o || !o->getTemplate() || !templates.count(lowerName(o->getTemplate()->getName())) || !canPick(o))
            continue;
        const Mat4& m = o->getAbsoluteTransformation();
        const float dx = m.m[12] - at[0], dz = m.m[14] - at[2];
        if (dx * dx + dz * dz > m_brushEraseRadius * m_brushEraseRadius)
            continue;
        std::string notDeletable;
        if (deletable(it->first, notDeletable))
            doomed.push_back(it->first);
    }
    for (size_t i = 0; i < doomed.size(); ++i)
    {
        IObject* o = object(doomed[i]);
        if (!o)
            continue;
        const std::string recreate = recreateLine(doomed[i], o->getTemplate()->getName(), o->getAbsoluteTransformation(),
                                                  static_cast<int>(o->getLayer()), propertiesOf(o));
        if (!removeObject(doomed[i], error))
            return false;
        m_strokeRemoved.push_back(doomed[i]);
        m_strokeRecreate.push_back(recreate);
    }
    return true;
}

bool EditSession::brushStrokeTick(const float at[3], std::string& error)
{
    if (!m_brushStroke)
    {
        error = "no brush stroke under way";
        return false;
    }
    LevelBrushFile* brush = activeBrushFile();
    if (!brush)
    {
        error = "no active brush";
        return false;
    }
    if (m_brushStrokeErase)
        return eraseBrush(at, error);
    // Across the ground only, as retail measured it: a climb does not count.
    if (m_haveLastStamp)
    {
        const float dx = at[0] - m_lastStamp[0], dz = at[2] - m_lastStamp[1];
        if (std::sqrt(dx * dx + dz * dz) <= brush->spacing)
            return true;
    }
    m_lastStamp[0] = at[0];
    m_lastStamp[1] = at[2];
    m_haveLastStamp = true;
    return stampBrush(at, m_strokeMade, error);
}

void EditSession::endBrushStroke()
{
    if (!m_brushStroke)
        return;
    m_brushStroke = false;
    const std::string brushName = m_activeBrush.empty() ? std::string("the brush") : "the brush " + m_activeBrush;
    char label[160];
    HistoryEntry entry;
    if (m_brushStrokeErase && !m_strokeRemoved.empty())
    {
        char line[48];
        for (size_t i = 0; i < m_strokeRemoved.size(); ++i)
        {
            std::snprintf(line, sizeof(line), "EditorObject.remove %u\n", m_strokeRemoved[i]);
            entry.doScript += line;
        }
        for (size_t i = m_strokeRecreate.size(); i-- > 0;)
            entry.undoScript += m_strokeRecreate[i];
        std::snprintf(label, sizeof(label), "Erase %u objects with %s", static_cast<unsigned>(m_strokeRemoved.size()),
                      brushName.c_str());
        entry.label = label;
        m_history.record(entry);
        logLine(LogLevel::Info, "edit: %s", entry.label.c_str());
    }
    else if (!m_brushStrokeErase)
    {
        std::vector<unsigned> made;
        for (size_t i = 0; i < m_strokeMade.size(); ++i)
            if (object(m_strokeMade[i]))
                made.push_back(m_strokeMade[i]);
        if (!made.empty())
        {
            madeScripts(made, entry.doScript, entry.undoScript);
            std::snprintf(label, sizeof(label), "Paint %u objects with %s", static_cast<unsigned>(made.size()),
                          brushName.c_str());
            entry.label = label;
            // Made already, stamp by stamp, so the history records the stroke as done.
            m_history.record(entry);
            logLine(LogLevel::Info, "edit: %s", entry.label.c_str());
        }
    }
    if (m_strokeSkipped)
        logLine(LogLevel::Info, "brush: %d objects were left out (no ground under them)", m_strokeSkipped);
}

bool EditSession::brushStamp(const Vec3& at, std::string& result)
{
    if (!beginBrushStroke(false, result))
        return false;
    const float point[3] = { at.x, at.y, at.z };
    std::string error;
    const bool ok = brushStrokeTick(point, error);
    const size_t made = m_strokeMade.size();
    const int skipped = m_strokeSkipped;
    endBrushStroke();
    if (!ok)
    {
        result = error;
        return false;
    }
    char text[160];
    std::snprintf(text, sizeof(text), "painted %u objects with the brush %s", static_cast<unsigned>(made),
                  m_activeBrush.c_str());
    result = text;
    if (skipped)
    {
        std::snprintf(text, sizeof(text), "; %d left out (no ground under them)", skipped);
        result += text;
    }
    return true;
}

bool EditSession::brushErase(const Vec3& at, std::string& result)
{
    if (!beginBrushStroke(true, result))
        return false;
    const float point[3] = { at.x, at.y, at.z };
    std::string error;
    const bool ok = brushStrokeTick(point, error);
    const size_t removed = m_strokeRemoved.size();
    endBrushStroke();
    if (!ok)
    {
        result = error;
        return false;
    }
    char text[160];
    std::snprintf(text, sizeof(text), "erased %u objects of the brush %s within %g m", static_cast<unsigned>(removed),
                  m_activeBrush.c_str(), m_brushEraseRadius);
    result = text;
    return true;
}

} // namespace bf2editor
