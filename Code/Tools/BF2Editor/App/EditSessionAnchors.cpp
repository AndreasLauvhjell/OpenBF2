// Editor anchor operations use the engine's anchor values, including compression/offset behavior.
// Snap uses world anchor height minus local anchor Y, falling back to bounds for a zero anchor.
// [CHOSEN, not recovered] bounds include children. Template edits are undoable and require writable
// files; reset restores saved text. Rotating/stamping anchors leaves unrelated template state intact.
#include "Framework/EditorConfig.h"
#include "App/EditSession.h"

#include "App/EditorLevel.h"
#include "Framework/EditorLog.h"

#include "Game/Objects/IObject.h"
#include "Game/Objects/IObjectTemplate.h"
#include "Math/LocalBoundingBox.h"
#include "Framework/TemplateFile.h"

#include <cmath>

#include <cstdio>

using dice::hfe::Mat4;
using dice::hfe::Vec3;
using dice::hfe::world::IObject;

namespace bf2editor
{

namespace
{

std::string vectorText(const float v[3])
{
    char text[128];
    std::snprintf(text, sizeof(text), "%.3f/%.3f/%.3f", v[0], v[1], v[2]);
    return text;
}

std::string vectorText(const Vec3& v)
{
    const float f[3] = { v.x, v.y, v.z };
    return vectorText(f);
}

} // namespace

bool EditSession::anchorPosOf(IObject* o, float out[3])
{
    out[0] = out[1] = out[2] = 0.0f;
    const dice::hfe::world::IObjectTemplate* objectTemplate = o ? o->getTemplate() : 0;
    if (!objectTemplate)
        return false;
    const Vec3 anchor = objectTemplate->getAnchorPos();
    out[0] = anchor.x;
    out[1] = anchor.y;
    out[2] = anchor.z;
    return true;
}

bool EditSession::worldAnchorOf(unsigned id, float out[3])
{
    IObject* o = object(id);
    if (!o)
        return false;
    float a[3];
    anchorPosOf(o, a);
    const Mat4& m = o->getAbsoluteTransformation();
    for (int i = 0; i < 3; ++i)
        out[i] = a[0] * m.m[i] + a[1] * m.m[4 + i] + a[2] * m.m[8 + i] + m.m[12 + i];
    return true;
}

float EditSession::snapPivotHeight(IObject* o, float ground)
{
    float a[3];
    if (!o || !anchorPosOf(o, a))
        return ground;
    if (a[0] != 0.0f || a[1] != 0.0f || a[2] != 0.0f)
        return ground - a[1];
    dice::hfe::LocalBoundingBox* box = o->getFullBoundingBox();
    if (box && box->m_max.y > box->m_min.y)
        return ground - box->m_min.y;
    return ground;
}

bool EditSession::anchorInfo(std::string& result)
{
    const unsigned id = m_selection.primary();
    IObject* o = object(id);
    if (!o || !o->getTemplate())
    {
        result = "nothing is selected";
        return false;
    }
    const dice::hfe::world::IObjectTemplate* objectTemplate = o->getTemplate();
    float position[3], world[3];
    anchorPosOf(o, position);
    worldAnchorOf(id, world);
    result = describe(id) + ": anchor " + vectorText(objectTemplate->getAnchor()) + ", offset "
           + vectorText(objectTemplate->getAnchorOffset()) + ", anchor position " + vectorText(position) + ", world anchor "
           + vectorText(world);
    const Vec3 anchor = objectTemplate->getAnchor();
    const Vec3 offset = objectTemplate->getAnchorOffset();
    if (position[0] != anchor.x + offset.x || position[1] != anchor.y + offset.y || position[2] != anchor.z + offset.z)
        result += " (the mesh quantises it, or it is a SkinnedMesh, whose anchor the 1.50 engine answers as 1/1/1)";
    if (m_alignStamp)
        result += "; the stamp is " + describe(resolve(m_alignStamp));
    return true;
}

bool EditSession::setAlignStamp(std::string& result)
{
    const unsigned id = m_selection.primary();
    if (!object(id))
    {
        result = "nothing is selected";
        return false;
    }
    m_alignStamp = id;
    float world[3];
    worldAnchorOf(id, world);
    result = "the stamp is " + describe(id) + " (world anchor " + vectorText(world) + ")";
    return true;
}

bool EditSession::alignToStamp(std::string& result)
{
    const unsigned stamp = resolve(m_alignStamp);
    float target[3];
    if (!m_alignStamp || !worldAnchorOf(stamp, target))
    {
        result = "no stamp: select the object to align to and stamp it first";
        return false;
    }
    std::vector<TransformChange> changes;
    for (size_t i = 0; i < m_selection.ids().size(); ++i)
    {
        TransformChange change;
        change.id = m_selection.ids()[i];
        float anchor[3];
        if (change.id == stamp || !transformOf(change.id, change.before) || !worldAnchorOf(change.id, anchor))
            continue;
        change.after = change.before;
        for (int k = 0; k < 3; ++k)
            change.after.m[12 + k] += target[k] - anchor[k];
        if (change.after.m[12] != change.before.m[12] || change.after.m[13] != change.before.m[13]
            || change.after.m[14] != change.before.m[14])
            changes.push_back(change);
    }
    if (changes.empty())
    {
        result = m_selection.empty() ? "nothing is selected" : "the selection's anchors are on the stamp's already";
        return !m_selection.empty();
    }
    const std::string label = (changes.size() == 1 ? "Align " + describe(changes[0].id) : labelFor("Align"))
                            + " to the anchor of " + describe(stamp);
    if (!applyTransforms(changes, label, result))
        return false;
    result = label;
    return true;
}

bool EditSession::raiseByAnchor(std::string& result)
{
    std::vector<TransformChange> changes;
    for (size_t i = 0; i < m_selection.ids().size(); ++i)
    {
        TransformChange change;
        change.id = m_selection.ids()[i];
        float anchor[3];
        if (!transformOf(change.id, change.before) || !anchorPosOf(object(change.id), anchor) || anchor[1] == 0.0f)
            continue;
        change.after = change.before;
        change.after.m[13] -= anchor[1];
        changes.push_back(change);
    }
    if (changes.empty())
    {
        result = m_selection.empty() ? "nothing is selected" : "no selected object has an anchor above or below its pivot";
        return !m_selection.empty();
    }
    const std::string label = (changes.size() == 1 ? "Raise " + describe(changes[0].id) : labelFor("Raise"))
                            + " by the anchor";
    if (!applyTransforms(changes, label, result))
        return false;
    result = label;
    return true;
}

bool EditSession::setGizmoPivot(int pivot, std::string& result)
{
    if (pivot != 0 && pivot != 1)
    {
        result = "the rotate gizmo's pivot is 0 (the object's) or 1 (its anchor)";
        return false;
    }
    m_rotateAroundAnchor = pivot == 1;
    result = m_rotateAroundAnchor ? "the rotate gizmo turns about the primary object's anchor"
                                  : "the rotate gizmo turns about the primary object's pivot";
    return true;
}

namespace
{

// One anchor component as the stock files write it: up to four decimals, zero as 0 (`0/-1.25/0`).
std::string anchorNumber(float value)
{
    if (std::fabs(value) < 0.00005f)
        return "0";
    char text[32];
    std::snprintf(text, sizeof(text), "%.4f", value);
    std::string out = text;
    while (!out.empty() && out[out.size() - 1] == '0')
        out.erase(out.size() - 1);
    if (!out.empty() && out[out.size() - 1] == '.')
        out.erase(out.size() - 1);
    return out;
}

} // namespace

bool EditSession::setAnchor(const Vec3& anchor, std::string& result)
{
    IObject* o = object(m_selection.primary());
    if (!o || !o->getTemplate())
    {
        result = "nothing is selected";
        return false;
    }
    const std::string value = anchorNumber(anchor.x) + "/" + anchorNumber(anchor.y) + "/" + anchorNumber(anchor.z);
    return setObjectTemplateProperty(o->getTemplate()->getName(), "anchor", value, result);
}

bool EditSession::anchorToBox(const std::string& which, std::string& result)
{
    IObject* o = object(m_selection.primary());
    dice::hfe::LocalBoundingBox* box = o ? o->getFullBoundingBox() : 0;
    if (!o || !box)
    {
        result = o ? describe(m_selection.primary()) + " has no bounding box" : std::string("nothing is selected");
        return false;
    }
    Vec3 point;
    point.x = point.y = point.z = 0.0f;
    if (_stricmp(which.c_str(), "bottom") == 0)
    {
        point.x = (box->m_min.x + box->m_max.x) * 0.5f;
        point.y = box->m_min.y;
        point.z = (box->m_min.z + box->m_max.z) * 0.5f;
    }
    else if (_strnicmp(which.c_str(), "corner", 6) == 0 && which.size() == 7 && which[6] >= '0' && which[6] <= '7')
    {
        // Corner n: bit 0 picks x, bit 1 y, bit 2 z (0 the minimum, 1 the maximum).
        const int n = which[6] - '0';
        point.x = (n & 1) ? box->m_max.x : box->m_min.x;
        point.y = (n & 2) ? box->m_max.y : box->m_min.y;
        point.z = (n & 4) ? box->m_max.z : box->m_min.z;
    }
    else if (_stricmp(which.c_str(), "origin") != 0)
    {
        result = "the anchor goes to bottom, origin or corner0 to corner7 of the bounding box";
        return false;
    }
    return setAnchor(point, result);
}

bool EditSession::setAnchorMoveMode(bool on, std::string& result)
{
    m_anchorMoveMode = on;
    result = on ? "the move gizmo moves the primary object's anchor (its template's, for every object of it)"
                : "the move gizmo moves the selection";
    return true;
}

} // namespace bf2editor
