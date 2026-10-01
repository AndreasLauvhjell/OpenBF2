// BSP wrapper and legacy tree support; Physics/Bsp.cpp is attested.
// Legacy walkers use shared query statics with [CHOSEN] names and are not reentrant.
// The normal load/build paths create BspV8 trees.

#include "Bsp.h"

#include <string>
#include <cmath>
#include <cstdio>

#include "../Core/Debug.h"
#include "../IO/Stream/IStream.h"
#include "CollisionGeometry.h"

namespace dice {
namespace hfe {
namespace world {

int Bsp::m_totalCheckedFaces = 0;                          // the TU's first static

namespace {
// The legacy statics (see the header block). Only the ones the written legacy code touches.
std::vector<const BspNode::Face*>* s_legacyResult = 0;
const Vec3* s_legacyP0     = 0;
Vec3        s_legacyDir    = { 0.0f, 0.0f, 0.0f };         // 0x8ABAF6C..0x8ABAF74 (init _0ii_397)
const Vec3* s_legacyP1     = 0;
float       s_legacyRadius = 0.0f;
int         s_legacyStamp  = 0;                            // 0x8ABAFC8 - ++ per legacy query
} // namespace

// BspNode - the legacy node.
// all zero.
BspNode::BspNode()
{
    faceCount = 0; faces = 0;
    planePoint.x = planePoint.y = planePoint.z = 0.0f;
    planeNormal.x = planeNormal.y = planeNormal.z = 0.0f;
    childNode1 = 0; childNode2 = 0;
}


void BspNode::destroy()
{
    if (faces)      { delete[] faces; faces = 0; }
    if (childNode1) { childNode1->destroy(); delete childNode1; childNode1 = 0; }
    if (childNode2) { childNode2->destroy(); delete childNode2; childNode2 = 0; }
}


void BspNode::addFace(Face* face)
{
    faces[faceCount++] = face;
}

// the legacy ray gather. A leaf (faceCount != 0) reports each unstamped face
// whose plane the direction crosses on the requested side (dot != 0 && frontFaces != (dot > 0));
// an internal node classifies p0 and p1 against its plane and descends into the children the
// segment touches. On a server the root is always the empty ctor node: faceCount 0, no children
// -> the internal arm runs with both children null and returns false. Written in full anyway.
bool BspNode::getCollidingFaces(bool frontFaces) const
{
    if (faceCount)
    {
        for (int i = 0; i < faceCount; ++i)
        {
            Face* f = faces[i];
            if (f->lastQueryStamp == s_legacyStamp)
                continue;
            f->lastQueryStamp = s_legacyStamp;
            const float dot = s_legacyDir.y * f->normal.y + s_legacyDir.x * f->normal.x
                            + s_legacyDir.z * f->normal.z;
            if (dot != 0.0f && frontFaces != (dot > 0.0f))
                s_legacyResult->push_back(f);
        }
        return true;
    }

    // side of p0 (bit 1) / p1 (bit 2): equal sides -> that side only, else both (3)
    const float s0 = (s_legacyP0->y - planePoint.y) * planeNormal.y
                   + (s_legacyP0->x - planePoint.x) * planeNormal.x
                   + (s_legacyP0->z - planePoint.z) * planeNormal.z;
    const float s1 = (s_legacyP1->y - planePoint.y) * planeNormal.y
                   + (s_legacyP1->x - planePoint.x) * planeNormal.x
                   + (s_legacyP1->z - planePoint.z) * planeNormal.z;
    int sides = 3;
    if ((s0 >= 0.0f) + 1 == (s1 >= 0.0f) + 1)
        sides = (s0 >= 0.0f) + 1;
    bool found = false;
    if ((sides & 1) && childNode1)
        found = childNode1->getCollidingFaces(frontFaces);
    if ((sides & 2) && childNode2)
        return childNode2->getCollidingFaces(frontFaces) | found;
    return found;
}

// ELF 0x838F140 / dedicated PE 0x6C2000. This no-argument const signature is attested by
// _ZNK4dice3hfe5world7BspNode30getCollidingFacesInsideCapsuleEv; Bsp fills the shared context.
bool BspNode::getCollidingFacesInsideCapsule() const
{
    if (faceCount)
    {
        for (unsigned int i = 0; i < static_cast<unsigned int>(faceCount); ++i)
        {
            Face* face = faces[i];
            if (face->lastQueryStamp == s_legacyStamp)
                continue;
            face->lastQueryStamp = s_legacyStamp;
            const float dot = s_legacyDir.y * face->normal.y + s_legacyDir.x * face->normal.x
                            + s_legacyDir.z * face->normal.z;
            if (dot < 0.0f)
                s_legacyResult->push_back(face);
        }
        return true; // A leaf reports true even if no face passed its facing/stamp tests.
    }

    const float s0 = (s_legacyP0->y - planePoint.y) * planeNormal.y
                   + (s_legacyP0->x - planePoint.x) * planeNormal.x
                   + (s_legacyP0->z - planePoint.z) * planeNormal.z;
    const float s1 = (s_legacyP1->y - planePoint.y) * planeNormal.y
                   + (s_legacyP1->x - planePoint.x) * planeNormal.x
                   + (s_legacyP1->z - planePoint.z) * planeNormal.z;
    if (std::fabs(s0) <= s_legacyRadius || std::fabs(s1) <= s_legacyRadius)
    {
        bool found = false;
        if (childNode1)
            found = childNode1->getCollidingFacesInsideCapsule();
        if (childNode2)
            return childNode2->getCollidingFacesInsideCapsule() | found;
        return found;
    }
    // Retail quirk: a crossing segment with both endpoints outside the radius still chooses
    // only child 1. Do not replace this with a geometric segment/plane intersection test.
    const BspNode* child = (s0 < 0.0f || s1 < 0.0f) ? childNode1 : childNode2;
    return child ? child->getCollidingFacesInsideCapsule() : false;
}

// ELF 0x838E820 / PE 0x6C0FA0. The mangled ELF name has no K: the old const declaration
// was wrong. Both retail bodies format into a shared buffer and produce no log/output.
void BspNode::printInfo(int depth)
{
    static char str[2048];
    if (faceCount)
    {
        std::sprintf(str, "%*d leafSz ", 2 * depth, faceCount);
        return;
    }
    std::sprintf(str, "%*d splitPlane", 2 * depth, depth);
    if (childNode1)
        childNode1->printInfo(depth + 1);
    if (childNode2)
        childNode2->printInfo(depth + 1);
}

// Bsp - the wrapper.

Bsp::Bsp()
{
    m_flag0 = 0;
    m_numFaces = 0; m_numNodes = 0;
    m_pool34 = 0; m_pool38 = 0;
    m_faceListSlots = 0;
    m_totalFaceRefs = 0;
    bspV8 = 0;
}

// Releases the owned BSP tree through destroy().
Bsp::~Bsp()
{
    destroy();
}


void Bsp::destroy()
{
    if (bspV8)
    {
        delete bspV8;
    }
    if (!m_flag0)
        bspNode.destroy();
    bspNode.faceCount = 0;
    bspNode.faces = 0;
    bspNode.childNode1 = 0;
    bspNode.childNode2 = 0;
    if (m_pool38) { delete[] static_cast<char*>(m_pool38); m_pool38 = 0; }
    if (m_pool34) { delete[] static_cast<char*>(m_pool34); m_pool34 = 0; }
    m_flag0 = 0;
    m_faceListSlots = 0;
    m_numFaces = 0;
    m_numNodes = 0;
    // bspV8 is not nulled after the delete, and m_totalFaceRefs is not reset - as shipped.
}


bool Bsp::load(io::IStream* stream, const std::vector<Vec3>* vertices,
               const std::vector<CollisionFace>* faces)
{
    BspV8* tree = new BspV8();
    bspV8 = tree;
    return tree->load(stream, vertices, faces);
}

// the tool path (CollisionMeshTemplate::save). BspV8::build now
// delegates to the recovered native BspBuilder; this wrapper is unchanged.
bool Bsp::build(const std::vector<Vec3>* vertices, const std::vector<CollisionFace>* faces, int a4)
{
    if (bspV8)
        return true;
    BspV8* tree = new BspV8();
    bspV8 = tree;
    return tree->build(vertices, faces, a4);
}


bool Bsp::getCollidingFaces(std::vector<const BspNode::Face*>& out, const Vec3& p0, const Vec3& p1,
                            bool frontFaces) const
{
    if (bspV8)
        return bspV8->getCollidingFaces(out, p0, p1, 0.0f, !frontFaces);

    ++s_legacyStamp;
    s_legacyResult = &out;
    s_legacyP0 = &p0;
    s_legacyP1 = &p1;
    s_legacyDir.x = p1.x - p0.x;
    s_legacyDir.y = p1.y - p0.y;
    s_legacyDir.z = p1.z - p0.z;
    bspNode.getCollidingFaces(frontFaces);
    m_totalCheckedFaces += static_cast<int>(out.size());
    return true;                                 // unconditionally, as shipped
}


bool Bsp::getCollidingFacesInsideCapsule(std::vector<const BspNode::Face*>& out, const Vec3& p0,
                                         const Vec3& p1, float radius) const
{
    if (bspV8)
        return bspV8->getCollidingFaces(out, p0, p1, radius, false);

    ++s_legacyStamp;
    s_legacyP0 = &p0;
    s_legacyP1 = &p1;
    s_legacyResult = &out;
    s_legacyDir.x = p1.x - p0.x;
    s_legacyDir.y = p1.y - p0.y;
    s_legacyDir.z = p1.z - p0.z;
    s_legacyRadius = radius;
    bspNode.getCollidingFacesInsideCapsule();
    m_totalCheckedFaces += static_cast<int>(out.size());
    return true;
}

// The legacy arm builds a BspNode::Context (0x38 B on the stack: a flag, the
// two end points, 1/dir, four bools and the three outputs) and calls
// BspNode::getClosestFace(ctx, box) - [FRONTIER] with the rest of the legacy walkers; on a server
// bspV8 is always set for a loaded Lod. Declared here so the arm is honest.
const BspNode::Face* Bsp::getClosestFace(Vec3* outPoint, float* outT, float* outPlaneDist,
                                         const Vec3& p0, const Vec3& dir,
                                         const LocalBoundingBox& /*box*/,
                                         const std::vector<Vec3>& /*verts*/,
                                         bool noFacingFilter, bool backFaces, bool firstHit) const
{
    if (bspV8)
        return bspV8->getClosestFace(outPoint, outT, outPlaneDist, p0, dir,
                                     noFacingFilter, backFaces, firstHit);
    // [FRONTIER] the legacy closest-face search (BspNode::getClosestFace 0x838EBA0, 1,056 B).
    // Its early-out is reproduced: a zero-length direction finds nothing.
    return 0;
}


int Bsp::getFaceIndex(const BspNode::Face* face) const
{
    const std::vector<BspNode::Face>& faces = bspV8->getFaceVector();
    return static_cast<int>(face - (faces.empty() ? static_cast<const BspNode::Face*>(0) : &faces[0]));
}


unsigned int Bsp::findFaceIndex(const BspNode::Face* face) const
{
    if (m_numFaces)
    {
        const std::vector<BspNode::Face>& faces = bspV8->getFaceVector();
        const BspNode::Face* base = faces.empty() ? 0 : &faces[0];
        for (unsigned int i = 0; i < static_cast<unsigned int>(m_numFaces); ++i)
            if (base + i == face)
                return i;
    }
    Debug(DEBUGTYPE_6, "Physics/Bsp.cpp", 289) << "notfound";
    return 0;
}


int Bsp::getApproxBspSize() const
{
    return 4 * (m_totalFaceRefs + 6 * m_numFaces + 10 * m_numNodes) + 84;
}

// > the legacy formatter (no output in the shipped images).
void Bsp::printInfo()
{
    bspNode.printInfo(0);
}

// the legacy builder's node factory.
BspNode* Bsp::allocateNewBspNode(std::vector<BspNode::Face*>& faces)
{
    ++m_numNodes;
    m_totalFaceRefs += static_cast<int>(faces.size());
    BspNode* node = new BspNode();
    node->faces = allocateNewFaceList(static_cast<int>(faces.size()));
    for (std::vector<BspNode::Face*>::iterator it = faces.begin(); it != faces.end(); ++it)
        node->addFace(*it);
    return node;
}


BspNode::Face** Bsp::allocateNewFaceList(int count)
{
    m_faceListSlots += count;
    return new BspNode::Face*[count];
}

} // namespace world
} // namespace hfe
} // namespace dice
