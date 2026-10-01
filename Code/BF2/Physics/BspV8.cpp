// Version-8 BSP construction and collision traversal. Preserve floating-point branch order
// when deciding near/far split-plane searches. Source path is attested by anonymous-namespace symbols.

#include "Bsp.h"
#include "BspBuilder.h"

#include <cmath>
#include <cstring>
#include <float.h>

#include "../IO/Stream/IStream.h"
#include "../Math/Intersect.h"
#include "CollisionGeometry.h"
#include "GeomUtil.h"

namespace dice {
namespace hfe {
namespace world {

namespace {
inline void readBytes(io::IStream* s, void* dst, unsigned int n) { s->read(dst, n); }
} // namespace

// every word but +0x3C zeroed (the ELF stores nothing to m_vertices; load and
// build both assign it before any read).
BspV8::BspV8()
    : m_vertices(0), m_queryCounter(0)
{
    m_min.x = m_min.y = m_min.z = 0.0f;
    m_max.x = m_max.y = m_max.z = 0.0f;
}

// the three vectors' storage. Nothing else is owned.
BspV8::~BspV8()
{
}

// copyFaces - 0x8395310, 564 B.
void BspV8::copyFaces(const std::vector<CollisionFace>* faces)
{
    const unsigned int n = static_cast<unsigned int>(faces->size());
    BspNode::Face blank;
    blank.lastQueryStamp = 0;
    blank.normal.x = blank.normal.y = blank.normal.z = 0.0f;
    blank.material = 0;
    blank.v0 = blank.v1 = blank.v2 = 0;
    m_faces.resize(n, blank);                    // grow with blanks / shrink, as the ELF's
                                                 //   _M_fill_insert / erase pair
    const Vec3* verts = m_vertices->empty() ? 0 : &(*m_vertices)[0];
    for (unsigned int i = 0; i < n; ++i)         // the ELF re-reads faces->size() per iteration
    {
        const CollisionFace& src = (*faces)[i];
        BspNode::Face& f = m_faces[i];
        f.v0 = static_cast<unsigned short>(src.v0);
        f.v1 = static_cast<unsigned short>(src.v1);
        f.v2 = static_cast<unsigned short>(src.v2);
        f.material = src.material;

        const Vec3& a = verts[f.v0];
        const Vec3& b = verts[f.v1];
        const Vec3& c = verts[f.v2];
        // e1 = c - b, e2 = b - a; normal = cross(e1, e2)
        const float e1x = c.x - b.x, e1y = c.y - b.y, e1z = c.z - b.z;
        const float e2x = b.x - a.x, e2y = b.y - a.y, e2z = b.z - a.z;
        f.normal.x = e1y * e2z - e1z * e2y;
        f.normal.y = e2x * e1z - e2z * e1x;
        f.normal.z = e2y * e1x - e2x * e1y;
        const float len2 = f.normal.y * f.normal.y + f.normal.x * f.normal.x + f.normal.z * f.normal.z;
        if (fabsf(len2 - 1.0f) >= 1.1920929e-7f)             // already unit? leave it
        {
            if (fabsf(len2) >= 1.1920929e-7f)
            {
                const float inv = 1.0f / sqrtf(len2);
                f.normal.x *= inv;
                f.normal.y *= inv;
                f.normal.z *= inv;
            }
            else
            {
                f.normal.x = f.normal.y = f.normal.z = 0.0f;  // degenerate face
            }
        }
        f.lastQueryStamp = 0;
    }
}

// load - 0x8394D40, 463 B. See Bsp.h for the byte format.
bool BspV8::load(io::IStream* stream, const std::vector<Vec3>* vertices,
                 const std::vector<CollisionFace>* faces)
{
    m_vertices = vertices;
    copyFaces(faces);

    readBytes(stream, &m_min, 12);
    readBytes(stream, &m_max, 12);

    unsigned int count = 0;
    readBytes(stream, &count, 4);
    Node blank;
    memset(&blank, 0, sizeof(blank));
    m_nodes.resize(count, blank);
    if (count)
        readBytes(stream, &m_nodes[0], count * sizeof(Node));   // (end - begin) & ~15

    readBytes(stream, &count, 4);
    m_faceIndices.resize(count, 0);
    if (count)
        readBytes(stream, &m_faceIndices[0], count * 2);        // (end - begin) & ~1
    return true;
}

// ELF 83948F0 / dedicated PE 6C38A0. Native builder pointers are consumed here;
// only the fixed 16-byte Node and u16 face indices enter the stored/query format.
bool BspV8::build(const std::vector<Vec3>* vertices, const std::vector<CollisionFace>* faces, int mode)
{
    BspBuilder builder(*vertices, *faces, mode);
    m_vertices = vertices;
    copyFaces(faces);
    m_min = builder.getMins();
    m_max = builder.getMaxs();
    const std::vector<BspSimpleNode>& nodes = builder.getNodes();
    if (nodes.empty()) return false;
    const Node blank = {};
    m_nodes.resize(nodes.size(), blank);
    // Retail does not clear this pool on a second build; offsets append to the old pool.
    m_faceIndices.reserve(builder.getNumFaceReferences());
    for (unsigned int i = 0; i < nodes.size(); ++i)
    {
        m_nodes[i].split = nodes[i].split;
        m_nodes[i].flags = static_cast<unsigned int>(nodes[i].axis) & 3;
        for (unsigned int side = 0; side < 2; ++side)
        {
            if (nodes[i].children[side]) m_nodes[i].child[side] = nodes[i].children[side];
            else
            {
                m_nodes[i].flags |= nodes[i].faceCount[side] << (16 + 8 * side);
                m_nodes[i].flags |= 4u << side;
                m_nodes[i].child[side] = static_cast<unsigned int>(m_faceIndices.size());
                for (unsigned int j = 0; j < nodes[i].faceCount[side]; ++j)
                    m_faceIndices.push_back(static_cast<unsigned short>(nodes[i].faceIndices[side][j]));
            }
        }
    }
    return true;
}

// Collects collision faces along the requested segment.
bool BspV8::getCollidingFaces(std::vector<const BspNode::Face*>& out, const Vec3& p0,
                              const Vec3& p1, float radius, bool backFaces) const
{
    ++m_queryCounter;                            // inc [eax+40h] - before the empty test
    if (!m_nodes.empty())
    {
        SearchContext ctx;
        ctx.collectAll = 1;
        ctx.done = false;
        ctx.origin = p0;
        ctx.dir.x = p1.x - p0.x;
        ctx.dir.y = p1.y - p0.y;
        ctx.dir.z = p1.z - p0.z;
        ctx.radius = radius;
        ctx.backFaces = backFaces;
        ctx.noFacingFilter = false;
        ctx.firstHit = false;
        ctx.result = &out;
        ctx.closestFace = 0;                     // (the ELF leaves +0x30 unset in this mode)
        ctx.closestPoint.x = ctx.closestPoint.y = ctx.closestPoint.z = 0.0f;
        ctx.closestT = 0.0f;
        ctx.closestPlaneDist = 0.0f;

        const float len2 = ctx.dir.z * ctx.dir.z + ctx.dir.y * ctx.dir.y + ctx.dir.x * ctx.dir.x;
        if (len2 <= 1.4210855e-14f)              // a point: the sphere walk
        {
            ctx.noFacingFilter = true;
            ctx.epsilon = 0.001f;
            if (sphereIntersectsAABB(p0, radius, m_min, m_max))
                getSphereCollidingFaces(m_nodes[0], &ctx);
        }
        else                                     // the swept ray
        {
            ctx.epsilon = 0.000099999997f / sqrtf(len2);        // 1e-4 in units of |dir|
            float tMin, tMax;
            if (rayRadiusIntersectsAABB(p0, ctx.dir, radius, m_min, m_max, tMin, tMax))
                getCollidingFaces(m_nodes[0], tMin, tMax, &ctx);
        }
    }
    return !out.empty();
}

// Traverses near then far children using a padded, clamped segment window. Near-child
// recursion retains the original window; far traversal starts at the radius-offset split
// and is skipped when unreachable or a first-hit query has finished.
bool BspV8::getCollidingFaces(const Node& node, float tMin, float tMax, SearchContext* ctx) const
{
    bool result = false;
    const unsigned int flags = node.flags;
    const int axis = static_cast<int>(flags & 3);

    float t0 = tMin - ctx->epsilon;  if (t0 <= 0.0f) t0 = 0.0f;
    float t1 = ctx->epsilon + tMax;  if (t1 >  1.0f) t1 = 1.0f;
    if (!(t0 < t1))
        return result;

    const float* dir = &ctx->dir.x;
    const float* org = &ctx->origin.x;
    float p[3], seg[3];
    for (int i = 0; i < 3; ++i)
    {
        p[i]   = dir[i] * t0 + org[i];
        seg[i] = dir[i] * (t1 - t0);
    }
    const float split = node.split;
    const unsigned int near = (p[axis] >= split) ? 1u : 0u;

    // The near child, with the original window
    if (flags & (4u << near))
    {
        const unsigned int count = (flags >> (8 * near + 16)) & 0xFF;
        result = ctx->collectAll
               ? copyCollidingFaces(ctx, node.child[near], count)
               : findClosestFace(node, near, ctx, node.child[near], count);
    }
    else
    {
        result = getCollidingFaces(m_nodes[node.child[near]], tMin, tMax, ctx);
    }

    if (ctx->done)
        return result;

    // The far child
    float tSplit = tMin;
    if (near == 0)
    {
        const float plane = split - ctx->radius;
        if (plane > p[axis])                     // p is short of the offset plane
        {
            if (dir[axis] <= 0.0f)  return result;          // moving away from it
            if (plane - p[axis] > seg[axis]) return result; // segment does not reach it
            tSplit = (plane - org[axis]) / dir[axis];
        }
    }
    else
    {
        const float plane = ctx->radius + split;
        if (!(p[axis] < plane))                  // p is beyond the offset plane
        {
            if (!(dir[axis] < 0.0f)) return result;
            if (!(seg[axis] < plane - p[axis])) return result;
            tSplit = (plane - org[axis]) / dir[axis];
        }
    }

    const unsigned int far = 1u - near;
    if (flags & (4u << far))
    {
        const unsigned int count = (flags >> (8 * far + 16)) & 0xFF;
        result = ctx->collectAll
               ? copyCollidingFaces(ctx, node.child[far], count)
               : findClosestFace(node, far, ctx, node.child[far], count);
    }
    else
    {
        result = getCollidingFaces(m_nodes[node.child[far]], tSplit, tMax, ctx);
    }
    return result;
}

// getSphereCollidingFaces - 0x8395860, 348 B. Both children whose slab the sphere overlaps.
bool BspV8::getSphereCollidingFaces(const Node& node, SearchContext* ctx) const
{
    bool result = false;
    const unsigned int flags = node.flags;
    const int axis = static_cast<int>(flags & 3);
    const float c = (&ctx->origin.x)[axis];
    const float split = node.split;
    const unsigned int near = (c >= split) ? 1u : 0u;

    if (flags & (4u << near))
        result = copyCollidingFaces(ctx, node.child[near], (flags >> (8 * near + 16)) & 0xFF);
    else
        result = getSphereCollidingFaces(m_nodes[node.child[near]], ctx);

    const bool overlapsFar = near ? (c <= ctx->radius + split) : (split - ctx->radius <= c);
    if (overlapsFar)
    {
        const unsigned int far = 1u - near;
        if (flags & (4u << far))
            result = copyCollidingFaces(ctx, node.child[far], (flags >> (8 * far + 16)) & 0xFF);
        else
            result = getSphereCollidingFaces(m_nodes[node.child[far]], ctx);
    }
    return result;
}

// copyCollidingFaces - 0x83959C0, 243 B. A leaf's faces: each unstamped face is stamped, and
// pushed if (noFacingFilter || (dot(dir, n) >= 0) == backFaces).
bool BspV8::copyCollidingFaces(SearchContext* ctx, unsigned int start, unsigned int count) const
{
    bool pushed = false;
    for (unsigned int i = start; i < start + count; ++i)
    {
        BspNode::Face& f = m_faces[m_faceIndices[i]];             // m_faces is mutable
        if (f.lastQueryStamp == m_queryCounter)
            continue;
        if (ctx->noFacingFilter
            || ((ctx->dir.x * f.normal.x + ctx->dir.y * f.normal.y + ctx->dir.z * f.normal.z >= 0.0f)
                == ctx->backFaces))
        {
            ctx->result->push_back(&f);
            pushed = true;
        }
        f.lastQueryStamp = m_queryCounter;
    }
    return pushed;
}

// getClosestFace (public) - 0x8395100, 522 B.
const BspNode::Face* BspV8::getClosestFace(Vec3* outPoint, float* outT, float* outPlaneDist,
                                           const Vec3& p0, const Vec3& dir,
                                           bool noFacingFilter, bool backFaces, bool firstHit) const
{
    if (m_nodes.empty())
        return 0;

    SearchContext ctx;
    ctx.collectAll = 0;
    ctx.done = false;
    ctx.origin = p0;
    ctx.dir = dir;
    ctx.radius = 0.0f;
    ctx.backFaces = backFaces;
    ctx.noFacingFilter = noFacingFilter;
    ctx.firstHit = firstHit;
    ctx.result = 0;
    ctx.closestFace = 0;
    ctx.closestPoint.x = ctx.closestPoint.y = ctx.closestPoint.z = 0.0f;
    ctx.closestT = FLT_MAX;
    ctx.closestPlaneDist = 0.0f;

    const float len2 = dir.z * dir.z + dir.y * dir.y + dir.x * dir.x;
    if (len2 <= 1.4210855e-14f)
        return 0;
    ctx.epsilon = 0.000099999997f / sqrtf(len2);

    float tMin = 0.0f, tMax = 1.0f;
    if (rayIntersectsAABB(p0, dir, m_min, m_max, tMin, tMax))
    {
        // clamp both to [0, 1] - the ELF's exact sequence
        float a = 0.0f;
        if (tMin >= 0.0f) { a = tMin; if (tMin > 1.0f) a = 1.0f; }
        float b = tMax;
        if (tMax < 0.0f) b = 0.0f; else if (b > 1.0f) b = 1.0f;
        getCollidingFaces(m_nodes[0], a, b, &ctx);
    }

    if (ctx.closestFace)
    {
        if (outPoint)     *outPoint     = ctx.closestPoint;
        if (outT)         *outT         = ctx.closestT;
        if (outPlaneDist) *outPlaneDist = ctx.closestPlaneDist;
    }
    return ctx.closestFace;
}

// Keeps the closest leaf-face collision after optional facing checks, reversing normals
// for back-face queries. Accepts only hits on this node's side within 1e-4 tolerance;
// first-hit queries stop immediately.
bool BspV8::findClosestFace(const Node& node, unsigned int side, SearchContext* ctx,
                            unsigned int start, unsigned int count) const
{
    bool result = false;
    const int axis = static_cast<int>(node.flags & 3);
    Vec3 end;
    end.x = ctx->origin.x + ctx->dir.x;
    end.y = ctx->origin.y + ctx->dir.y;
    end.z = ctx->origin.z + ctx->dir.z;
    Vec3 hit;
    hit.x = hit.y = hit.z = 0.0f;
    const Vec3* verts = m_vertices->empty() ? 0 : &(*m_vertices)[0];

    for (unsigned int i = start; i < start + count; ++i)
    {
        const BspNode::Face& f = m_faces[m_faceIndices[i]];
        if (!ctx->noFacingFilter)
        {
            const float dot = f.normal.y * ctx->dir.y + ctx->dir.x * f.normal.x + f.normal.z * ctx->dir.z;
            if (ctx->backFaces != (dot >= 0.0f))
                continue;
        }
        Vec3 n = f.normal;
        if (ctx->backFaces) { n.x = -n.x; n.y = -n.y; n.z = -n.z; }

        float along = 0.0f, planeDist = 0.0f;
        result = checkFaceAndEdgeCollision(verts[f.v0], verts[f.v1], verts[f.v2], n,
                                           ctx->origin, end, hit, planeDist, along,
                                           ctx->noFacingFilter, 0.0f);
        if (!result)
            continue;
        if (!(ctx->closestT > along))
            continue;

        // on this node's side of the split?
        const double h = (&hit.x)[axis];
        const bool onSide = side ? (h > static_cast<double>(node.split) - 0.0001)
                                 : (static_cast<double>(node.split) + 0.0001 > h);
        if (!onSide)
            continue;

        ctx->closestFace      = &f;
        ctx->closestT         = along;
        ctx->closestPlaneDist = planeDist;
        ctx->closestPoint     = hit;
        if (ctx->firstHit)
        {
            ctx->done = true;
            return result;
        }
    }
    return result;
}

} // namespace world
} // namespace hfe
} // namespace dice
