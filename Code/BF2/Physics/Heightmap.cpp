// Terrain height, material and collision queries.
// Path attested by __FILE__; query bodies cross-checked against the dedicated PE.

#include "Heightmap.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>

#include "../Core/Debug.h"
#include "../Core/Ids.h"
#include "../Game/Objects/IObject.h"             // the water bake's object walk
#include "../Game/Objects/ObjectLayoutAccess.h"  // LayoutAccess::collisionMesh - the +0x40 read
#include "../Game/Objects/ObjectTemplate.h"      // getMaterialVector - the template's materials
#include "../IO/File/FileManager.h"              // io::fileManager - the terrain file reads
#include "../Scene/Object/ObjectManager.h"       // objectManager->getObjectsFromGrid
#include "../Scene/Object/ObjectPredicator.h"    // ObjectFlagPredicator(0x1000100)
#include "ICollisionMesh.h"                      // the per-object ray cast
#include "Material.h"                            // getHasWaterPhysics
#include "MaterialManager.h"                     // getMaterialPtr

namespace dice {
namespace hfe {
namespace world {

extern IBase* materialManager;                   // ds:0x8AD2820 (Core/Globals.cpp) - declared there
                                                 //   as an IBase* [CHOSEN]; the concrete manager is
                                                 //   the MaterialManager (queryInterface-free cast,
                                                 //   as the ELF's direct vcalls imply)

namespace {

// ELF 0x8ABAF1C/0x8ABAF20, PE 0x89A1C8/0x89A1CC. The symbols are anonymous in the ELF;
// these source names are [INFERRED]. The first counts the leaf's penetration projection arm and
// the second its ordinary segment/plane crossing arm.
int s_heightmapRayInsideCount = 0;
int s_heightmapRayCrossingCount = 0;

void normalizeHeightmapRayNormal(Vec3& value)
{
    const float lengthSq = value.x * value.x + value.y * value.y + value.z * value.z;
    if (fabsf(lengthSq - 1.0f) >= FLT_EPSILON)
    {
        if (fabsf(lengthSq) < FLT_EPSILON)
        {
            value.x = value.y = value.z = 0.0f;
        }
        else
        {
            const float inverseLength = 1.0f / sqrtf(lengthSq);
            value.x *= inverseLength;
            value.y *= inverseLength;
            value.z *= inverseLength;
        }
    }
}

float clampHeightmapRayCoordinate(float value, float high)
{
    if (value >= high)
        value = high;
    // PE 0x402500 uses fcmovb for the lower bound, so unordered also selects zero.
    if (!(value >= 0.0f))
        value = 0.0f;
    return value;
}

// [INFERRED] packaging of the query bodies' inlined Vec3 normalization. ELF 0x837F2E5,
// 0x837FC18, 0x83840AA and 0x838434B; PE calls the common normalizer at 0x45BFA0.
// Keep the ordered "already unit" / "near zero" tests: unordered values reach sqrt too.
void normalizeHeightmapQueryNormal(Vec3& value)
{
    const float lengthSq = value.x * value.x + value.y * value.y + value.z * value.z;
    if (fabsf(lengthSq - 1.0f) < FLT_EPSILON)
        return;
    if (fabsf(lengthSq) < FLT_EPSILON)
    {
        value.x = value.y = value.z = 0.0f;
        return;
    }
    const float inverseLength = 1.0f / sqrtf(lengthSq);
    value.x *= inverseLength;
    value.y *= inverseLength;
    value.z *= inverseLength;
}

// [INFERRED] spelling of the four inlined integer clamps in the min/max query.
int clampHeightmapQueryIndex(int value, int low, int high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

} // anonymous namespace

// Initializes empty terrain with one reference, no-sea level -10000, and material scale one.
Heightmap::Heightmap()
{
    m_refCount = 1;                              // +0x04
    m_heightData = 0;                            // +0x0C
    m_posX = 0; m_posZ = 0;                      // +0x10/+0x14
    m_sizeX = 0; m_sizeZ = 0;                    // +0x18/+0x1C
    m_bitResolution = 0;                         // +0x20
    m_halfSizeX = 0; m_halfSizeZ = 0;            // +0x24/+0x28
    m_scale = Vec3::getZero();                   // +0x2C
    m_invScale = Vec3::getZero();                // +0x38
    m_usePhongNormalInterpolation = false;       // +0x44 (byte store)
    m_seaWaterLevel = -10000.0f;                 // +0x48
    m_quadTreeLevels = 0;                        // +0x4C
    m_quadTree = 0;                              // +0x50
    for (int i = 0; i < 7; ++i) _unresolved54[i] = 0;
    m_rayHitPoint = Vec3::getZero();
    m_rayHitNormal = Vec3::getZero();
    m_rayWaterHitPoint = Vec3::getZero();
    m_rayWaterHitNormal = Vec3::getZero();
    m_rayHitFraction = 0.0f;
    m_rayDzOverDx = 0.0f;
    m_rayDxOverDz = 0.0f;
    m_rayDyOverDx = 0.0f;
    m_rayDyOverDz = 0.0f;
    m_rayZeroDeltaX = false;
    m_rayZeroDeltaZ = false;
    m_materialScale = 1;                         // +0xB8
    m_materialData = 0;                          // +0xBC
    m_waterQuadTree = 0;                         // +0xC0
    m_waterData = 0;                             // +0xC4
    m_rayHitKind = 0;
}

// ELF D1 0x837E040 / PE 0x6A8290. Free height samples, both trees' levels, both pointer
// arrays, water samples, then materials; the string and IHeightmap destructors follow normally.
Heightmap::~Heightmap()
{
    delete[] static_cast<unsigned char*>(m_heightData);
    if (m_quadTree)
    {
        for (int i = 0; i < m_quadTreeLevels; ++i)
            delete[] m_quadTree[i];
    }
    if (m_waterQuadTree)
    {
        for (int i = 0; i < m_quadTreeLevels - 1; ++i)
            delete[] m_waterQuadTree[i];
    }
    delete[] m_quadTree;
    delete[] m_waterQuadTree;
    delete[] m_waterData;
    delete[] m_materialData;
}

// the registered factory (ModuleRegistry.cpp:720). Aggregation refused; qi or die.
IBase* Heightmap::create(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;
    Heightmap* self = new Heightmap();
    void* out = self->queryInterface(iid);
    if (!out)
        self->release();
    return static_cast<IBase*>(out);
}

void Heightmap::addRef()  { ++m_refCount; }
int  Heightmap::getRef()  { return m_refCount; }
int  Heightmap::release()
{
    const int r = --m_refCount;
    if (r == 0) delete this;
    return r;
}

void* Heightmap::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase)             return const_cast<Heightmap*>(this);
    if (iid == world::IID_IHeightmap) return const_cast<Heightmap*>(this);
    return 0;
}

// Loads the raw height grid. Size and bit depth must already be set, and the file must
// open successfully. Clamps 16-bit samples to 0xFF00, builds the height tree immediately,
// and lazily allocates a zeroed material grid if none was loaded.
bool Heightmap::loadHeightData(const std::string& name)
{
    if (m_heightData)
        delete[] static_cast<unsigned char*>(m_heightData);
    m_name.assign(name);

    io::IStream* f = io::fileManager->open(name, (io::IFileSystem::FileMode)2,
                                           (io::IFileSystem::FileAccess)0, 0);
    const unsigned int bytes =
        static_cast<unsigned int>(m_bitResolution * m_sizeX * m_sizeZ) >> 3;
    m_heightData = new unsigned char[bytes];
    f->read(m_heightData, bytes);                // +0x10 - unguarded, as shipped
    f->release();                                // +0x08

    if (m_bitResolution == 16)
    {
        unsigned short* samples = static_cast<unsigned short*>(m_heightData);
        const int count = static_cast<int>(bytes) / 2;
        for (int i = 0; i < count; ++i)
            if (samples[i] > 0xFF00u)
                samples[i] = 0xFF00u;
    }

    createQuadTree(m_sizeX);

    if (!m_materialData)
    {
        const unsigned int matBytes = m_materialScale * m_materialScale * m_sizeX * m_sizeZ;
        m_materialData = new unsigned char[matBytes];
        memset(m_materialData, 0, matBytes);
    }
    return true;                                 // [CHOSEN] - the ELF returns a live pointer here
}

bool Heightmap::saveHeightData(const std::string&) const { return false; }   // 7 B shipped stub

// the zeroed twin of loadHeightData: same allocation, memset instead of read,
// and it drops the material grid rather than lazily filling one.
bool Heightmap::createEmpty(const std::string& name)
{
    if (m_heightData)
        delete[] static_cast<unsigned char*>(m_heightData);
    m_name.assign(name);

    const unsigned int bytes =
        static_cast<unsigned int>(m_bitResolution * m_sizeX * m_sizeZ) >> 3;
    m_heightData = new unsigned char[bytes];
    memset(m_heightData, 0, bytes);

    if (m_materialData)
    {
        delete[] m_materialData;
        m_materialData = 0;
    }
    return true;                                 // [CHOSEN] - same materialised-value situation
}

// The modified-flag API intentionally reports false and ignores writes.
bool Heightmap::getModified() const   { return false; }
void Heightmap::setModified(bool /*modified*/) {}

// / 0x837E5F0, 12 B - the +0x44 byte.
void Heightmap::setUsePhongNormalInterpolation(bool on) { m_usePhongNormalInterpolation = on; }
bool Heightmap::getUsePhongNormalInterpolation() const  { return m_usePhongNormalInterpolation; }

// Returns the material under a centred grid cell, or zero without data/outside the grid.
// Material resolution retains repeated lookups and aborts for an invalid resolved ID or a water material.
unsigned char Heightmap::getMaterialFromGrid(int x, int z) const
{
    if (!m_materialData)
        return 0;
    const unsigned int gx = static_cast<unsigned int>(m_halfSizeX + x);
    const unsigned int gz = static_cast<unsigned int>(m_halfSizeZ + z);
    if (gx < static_cast<unsigned int>(m_sizeX) && gz < static_cast<unsigned int>(m_sizeZ))
    {
        const unsigned char material =
            m_materialData[m_materialScale * gz * m_materialScale * m_sizeX + m_materialScale * gx];
        MaterialManager* const materials = static_cast<MaterialManager*>(materialManager);
        if ((materials->getMaterialPtr(0u) && !materials->getMaterialPtr(material)) ||
            (materials->getMaterialPtr(material) &&
             materials->getMaterialPtr(material)->getHasWaterPhysics()))
            abort();
        return material;
    }
    return 0;
}

// ELF 0x8384710 / PE 0x6A9070: truncation toward zero, not the floor used by height queries.
unsigned char Heightmap::getMaterialInWorldCoords(float x, float z) const
{
    const int ix = static_cast<int>((x - static_cast<float>(m_posX)) * m_invScale.x);
    const int iz = static_cast<int>((z - static_cast<float>(m_posZ)) * m_invScale.z);
    return getMaterialFromGrid(ix, iz);
}

// ELF 0x837E7E0 / PE 0x6A85B0. Only the material-buffer null guard is shipped; no bounds check.
void Heightmap::setMaterialInGrid(int x, int z, unsigned char material)
{
    if (m_materialData)
        m_materialData[m_materialScale * (m_halfSizeZ + z) * m_sizeX * m_materialScale
                       + m_materialScale * (m_halfSizeX + x)] = material;
}

// ELF 0x8384780 / PE 0x6A90D0. Both scans deliberately ignore materialScale and have no
// material-buffer or manager null guard for a nonempty grid. getMaterialPtr retains its fallback.
bool Heightmap::getUsingValidMaterials() const
{
    const int count = m_sizeX * m_sizeZ;
    for (int i = 0; i < count; ++i)
        if (!static_cast<MaterialManager*>(materialManager)->getMaterialPtr(m_materialData[i]))
            return false;
    return true;
}

// ELF 0x83847E0 / PE 0x6A9120: -1 unseen, -2 unmapped, otherwise the original id. This is
// a void mutator, not a replacement count; only the low byte of the int argument is stored.
void Heightmap::replaceNonMappedMaterials(int material)
{
    int mapped[256];
    memset(mapped, 0xff, sizeof(mapped));
    const int count = m_sizeX * m_sizeZ;
    for (int i = 0; i < count; ++i)
    {
        const unsigned char id = m_materialData[i];
        if (mapped[id] < 0)
        {
            if (mapped[id] == -1)
                mapped[id] = static_cast<MaterialManager*>(materialManager)->getMaterialPtr(id)
                           ? static_cast<int>(id) : -2;
            if (mapped[id] == -2)
                m_materialData[i] = static_cast<unsigned char>(material);
        }
    }
}

// ELF 0x837EA60 / PE 0x6A9380. Upper bounds are exclusive, all four lower clamps use sizeX,
// including Z, and an empty/reversed rectangle leaves the unsigned sentinels UINT_MAX and zero.
void Heightmap::getMinMaxHeightInLocalCoords(float x0, float z0, float x1, float z1,
                                            float& minY, float& maxY)
{
    const float gx0 = x0 * m_invScale.x, gz0 = z0 * m_invScale.z;
    const float gx1 = x1 * m_invScale.x, gz1 = z1 * m_invScale.z;
    const int ix0 = static_cast<int>(floorf(gx0));
    const int iz0 = static_cast<int>(floorf(gz0));
    const int ix1 = static_cast<int>(ceilf(gx1));
    const int iz1 = static_cast<int>(ceilf(gz1));
    const int firstX = clampHeightmapQueryIndex(ix0, -m_sizeX / 2, m_sizeX / 2);
    const int firstZ = clampHeightmapQueryIndex(iz0, -m_sizeX / 2, m_sizeZ / 2);
    const int lastX = clampHeightmapQueryIndex(ix1, -m_sizeX / 2, m_sizeX / 2);
    const int lastZ = clampHeightmapQueryIndex(iz1, -m_sizeX / 2, m_sizeZ / 2);
    unsigned int low = ~0u, high = 0;
    for (int z = firstZ; z < lastZ; ++z)
        for (int x = firstX; x < lastX; ++x)
        {
            const unsigned int height = static_cast<unsigned int>(getHeightFromGrid(x, z));
            if (height < low) low = height;
            if (height > high) high = height;
        }
    minY = static_cast<float>(low) * m_scale.y;
    maxY = static_cast<float>(high) * m_scale.y;
}

// ELF 0x837E9E0 / PE 0x6A86E0: world position is subtracted directly, without another scale.
void Heightmap::getMinMaxHeightInWorldCoords(float x0, float z0, float x1, float z1,
                                            float& minY, float& maxY)
{
    getMinMaxHeightInLocalCoords(x0 - static_cast<float>(m_posX), z0 - static_cast<float>(m_posZ),
                                x1 - static_cast<float>(m_posX), z1 - static_cast<float>(m_posZ),
                                minY, maxY);
}

// ELF 0x837F0B0,0x837F100 / PE 0x6A89F0,0x6A8A20. The scalar overload dispatches to the
// local height query; the Vec3 overload dispatches to this scalar overload, as shipped.
float Heightmap::getWorldScaledHeightInWorldCoords(float x, float z) const
{
    return getHeightInLocalCoords(x - static_cast<float>(m_posX), z - static_cast<float>(m_posZ));
}
float Heightmap::getWorldScaledHeightInWorldCoords(const Vec3& p) const
{
    return getWorldScaledHeightInWorldCoords(p.x, p.z);
}

// ELF 0x837F130 / PE 0x6A8A40. The two off-diagonal samples precede triangle selection.
// Both slope components use invScale.x * scale.y, even when scale.x differs from scale.z.
// This query always produces a face normal, independently of the Phong switch.
void Heightmap::getNormalInLocalCoords(float x, float z, Vec3& normal) const
{
    const float gx = x * m_invScale.x, gz = z * m_invScale.z;
    const int ix = static_cast<int>(floorf(gx)), iz = static_cast<int>(floorf(gz));
    const float fx = gx - static_cast<float>(ix), fz = gz - static_cast<float>(iz);
    const float h10 = static_cast<float>(getHeightFromGrid(ix + 1, iz));
    const float h01 = static_cast<float>(getHeightFromGrid(ix, iz + 1));
    const float slopeScale = m_invScale.x * m_scale.y;
    if (fx + fz < 1.0f)
    {
        const float h00 = static_cast<float>(getHeightFromGrid(ix, iz));
        normal.x = -((h10 - h00) * slopeScale);
        normal.z = -((h01 - h00) * slopeScale);
    }
    else
    {
        const float h11 = static_cast<float>(getHeightFromGrid(ix + 1, iz + 1));
        normal.x = (h01 - h11) * slopeScale;
        normal.z = (h10 - h11) * slopeScale;
    }
    normal.y = 1.0f;
    normalizeHeightmapQueryNormal(normal);
}


// All three dispatch directly to the scalar local query (including the world Vec3 overload).
void Heightmap::getNormalInWorldCoords(float x, float z, Vec3& normal) const
{
    getNormalInLocalCoords(x - static_cast<float>(m_posX), z - static_cast<float>(m_posZ), normal);
}
void Heightmap::getNormalInLocalCoords(const Vec3& p, Vec3& normal) const
{
    getNormalInLocalCoords(p.x, p.z, normal);
}
void Heightmap::getNormalInWorldCoords(const Vec3& p, Vec3& normal) const
{
    getNormalInLocalCoords(p.x - static_cast<float>(m_posX), p.z - static_cast<float>(m_posZ), normal);
}

// ELF 0x837F9A0 / PE 0x6A9560. Output order is normal, v0, v1, v2, material. The square's
// diagonal selects (00,10,01) or (01,10,11), with equality in the latter. No Phong interpolation.
void Heightmap::getTriangleAndNormalAndMaterialInLocalCoords(
        float x, float z, Vec3& normal, Vec3& v0, Vec3& v1, Vec3& v2,
        unsigned char& material) const
{
    const float gx = x * m_invScale.x, gz = z * m_invScale.z;
    const float bx = floorf(gx), bz = floorf(gz);
    const int ix = static_cast<int>(bx), iz = static_cast<int>(bz);
    material = getMaterialFromGrid(ix, iz);
    if (1.0f - (gx - bx) > gz - bz)
    {
        v0 = Vec3{bx * m_scale.x, static_cast<float>(getHeightFromGrid(ix, iz)) * m_scale.y,
                  bz * m_scale.z};
        v1 = Vec3{(bx + 1.0f) * m_scale.x, static_cast<float>(getHeightFromGrid(ix + 1, iz)) * m_scale.y,
                  bz * m_scale.z};
        v2 = Vec3{bx * m_scale.x, static_cast<float>(getHeightFromGrid(ix, iz + 1)) * m_scale.y,
                  (bz + 1.0f) * m_scale.z};
    }
    else
    {
        v0 = Vec3{bx * m_scale.x, static_cast<float>(getHeightFromGrid(ix, iz + 1)) * m_scale.y,
                  (bz + 1.0f) * m_scale.z};
        v1 = Vec3{(bx + 1.0f) * m_scale.x, static_cast<float>(getHeightFromGrid(ix + 1, iz)) * m_scale.y,
                  bz * m_scale.z};
        v2 = Vec3{(bx + 1.0f) * m_scale.x,
                  static_cast<float>(getHeightFromGrid(ix + 1, iz + 1)) * m_scale.y,
                  (bz + 1.0f) * m_scale.z};
    }
    const Vec3 edge1 = {v1.x - v0.x, v1.y - v0.y, v1.z - v0.z};
    const Vec3 edge2 = {v2.x - v0.x, v2.y - v0.y, v2.z - v0.z};
    normal.x = edge2.y * edge1.z - edge2.z * edge1.y;
    normal.y = edge2.z * edge1.x - edge2.x * edge1.z;
    normal.z = edge2.x * edge1.y - edge2.y * edge1.x;
    normalizeHeightmapQueryNormal(normal);
}

// ELF 0x837FDF0 / PE 0x6A8C30: translate only the three vertices after the virtual local query.
void Heightmap::getTriangleAndNormalAndMaterialInWorldCoords(
        float x, float z, Vec3& normal, Vec3& v0, Vec3& v1, Vec3& v2,
        unsigned char& material) const
{
    getTriangleAndNormalAndMaterialInLocalCoords(x - static_cast<float>(m_posX),
        z - static_cast<float>(m_posZ), normal, v0, v1, v2, material);
    v0.x += static_cast<float>(m_posX); v0.z += static_cast<float>(m_posZ);
    v1.x += static_cast<float>(m_posX); v1.z += static_cast<float>(m_posZ);
    v2.x += static_cast<float>(m_posX); v2.z += static_cast<float>(m_posZ);
}

// ELF 0x837FEE0 / PE 0x6A8CB0. All four corners are written in 00,10,01,11 order; flipped
// is the query point's strict lower-triangle selector, not an alternating cell-parity flag.
void Heightmap::getSquareInLocalCoords(float x, float z, Vec3* corners, bool& flipped) const
{
    const float gx = x * m_invScale.x, gz = z * m_invScale.z;
    const float bx = floorf(gx), bz = floorf(gz);
    const int ix = static_cast<int>(bx), iz = static_cast<int>(bz);
    flipped = 1.0f - (gx - bx) > gz - bz;
    corners[0] = Vec3{bx * m_scale.x, static_cast<float>(getHeightFromGrid(ix, iz)) * m_scale.y,
                      bz * m_scale.z};
    corners[1] = Vec3{(bx + 1.0f) * m_scale.x, static_cast<float>(getHeightFromGrid(ix + 1, iz)) * m_scale.y,
                      bz * m_scale.z};
    corners[2] = Vec3{bx * m_scale.x, static_cast<float>(getHeightFromGrid(ix, iz + 1)) * m_scale.y,
                      (bz + 1.0f) * m_scale.z};
    corners[3] = Vec3{(bx + 1.0f) * m_scale.x,
                      static_cast<float>(getHeightFromGrid(ix + 1, iz + 1)) * m_scale.y,
                      (bz + 1.0f) * m_scale.z};
}

// ELF 0x83800D0 / PE 0x6A8E50: the local square helper is nonvirtual.
void Heightmap::getSquareInWorldCoords(float x, float z, Vec3* corners, bool& flipped) const
{
    getSquareInLocalCoords(x - static_cast<float>(m_posX), z - static_cast<float>(m_posZ), corners, flipped);
    for (int i = 0; i < 4; ++i)
    {
        corners[i].x += static_cast<float>(m_posX);
        corners[i].z += static_cast<float>(m_posZ);
    }
}

// ELF 0x8383FC0 / PE 0x6AB1A0. These are forward-difference face normals, not a central
// difference or an average of adjacent faces. Both horizontal edge lengths use scale.x.
// [PLATFORM] The conditional captures below preserve PE/ELF operand-evaluation ordering.
void Heightmap::internalPhongGetNormal(int x, int z, Vec3& normal) const
{
    const float height = static_cast<float>(getHeightFromGrid(x, z));
#if !defined(_WIN32)
    const float edgeX = m_scale.x; // ELF evaluates this operand before the next virtual call.
#endif
    const float heightX = static_cast<float>(getHeightFromGrid(x + 1, z));
#if defined(_WIN32)
    const float edgeX = m_scale.x; // PE evaluates it after that call.
#endif
    const float dyX = (heightX - height) * m_scale.y;
    const float heightZ = static_cast<float>(getHeightFromGrid(x, z + 1));
#if defined(_WIN32)
    const float edgeZ = edgeX; // PE reuses the captured X scale; ELF rereads it after the call.
#else
    const float edgeZ = m_scale.x;
#endif
    const float dyZ = (heightZ - height) * m_scale.y;
    normal.x = dyZ * 0.0f - edgeZ * dyX;
    normal.y = edgeZ * edgeX - 0.0f * 0.0f;
    normal.z = dyX * 0.0f - dyZ * edgeX;
    normalizeHeightmapQueryNormal(normal);
}

// ELF 0x8384160 / PE 0x6AB250. Normalize each X interpolation before the final normalized
// Z interpolation. The four helper calls (twelve height reads) are unconditional, even at a vertex.
void Heightmap::getPhongInterpolation(float x, float z, Vec3& normal) const
{
    const float gx = x * m_invScale.x, gz = z * m_invScale.z;
    const int ix = static_cast<int>(floorf(gx)), iz = static_cast<int>(floorf(gz));
    const float fx = gx - static_cast<float>(ix), fz = gz - static_cast<float>(iz);
    Vec3 corners[4] = {};
    internalPhongGetNormal(ix, iz, corners[0]);
    internalPhongGetNormal(ix + 1, iz, corners[1]);
    internalPhongGetNormal(ix, iz + 1, corners[2]);
    internalPhongGetNormal(ix + 1, iz + 1, corners[3]);
    Vec3 rows[2] = {};
    rows[0] = Vec3{corners[0].x * (1.0f - fx) + corners[1].x * fx,
                   corners[0].y * (1.0f - fx) + corners[1].y * fx,
                   corners[0].z * (1.0f - fx) + corners[1].z * fx};
    normalizeHeightmapQueryNormal(rows[0]);
    rows[1] = Vec3{corners[2].x * (1.0f - fx) + corners[3].x * fx,
                   corners[2].y * (1.0f - fx) + corners[3].y * fx,
                   corners[2].z * (1.0f - fx) + corners[3].z * fx};
    normalizeHeightmapQueryNormal(rows[1]);
    normal = Vec3{rows[0].x * (1.0f - fz) + rows[1].x * fz,
                  rows[0].y * (1.0f - fz) + rows[1].y * fz,
                  rows[0].z * (1.0f - fz) + rows[1].z * fz};
    normalizeHeightmapQueryNormal(normal);
}

// Returns scaled terrain height and writes the cell material and face/phong normal.
// Preserve the adjacent-cell reads before triangle selection and the normalization thresholds.
float Heightmap::getHeightAndNormalInLocalCoords(float x, float z, Vec3& normal,
                                                 unsigned char& material) const
{
    const float gx = x * m_invScale.x;
    const float gz = z * m_invScale.z;
    const int ix = static_cast<int>(floorf(gx));
    const int iz = static_cast<int>(floorf(gz));
    material = getMaterialFromGrid(ix, iz);                          // vtable +0x7C
    const float fx = gx - static_cast<float>(ix);
    const float fz = gz - static_cast<float>(iz);
    const float h10 = static_cast<float>(getHeightFromGrid(ix + 1, iz));   // vtable +0x88
    const float h01 = static_cast<float>(getHeightFromGrid(ix, iz + 1));
    const float s = m_invScale.x * m_scale.y;

    float height;
    if (fx <= 0.0f)
    {
        const float h00 = static_cast<float>(getHeightFromGrid(ix, iz));
        const float dz = h01 - h00;
        if (fz <= 0.0f)
            height = h00;
        else
            height = fz * dz + h00;
        normal.y = 1.0f;
        normal.x = -((h10 - h00) * s);
        normal.z = -(dz * s);
    }
    else if (fz <= 0.0f)
    {
        const float h00 = static_cast<float>(getHeightFromGrid(ix, iz));
        const float dx = h10 - h00;
        height = fx * dx + h00;
        normal.x = -(dx * s);
        normal.y = 1.0f;
        normal.z = -((h01 - h00) * s);
    }
    else if (fz + fx < 1.0f)
    {
        const float h00 = static_cast<float>(getHeightFromGrid(ix, iz));
        const float dx = h10 - h00;
        const float dz = h01 - h00;
        normal.y = 1.0f;
        normal.x = -(dx * s);
        height = fz * dz + fx * dx + h00;
        normal.z = -(dz * s);
    }
    else
    {
        const float h11 = static_cast<float>(getHeightFromGrid(ix + 1, iz + 1));
        const float dx = h01 - h11;
        const float dz = h10 - h11;
        normal.y = 1.0f;
        normal.x = dx * s;
        height = (1.0f - fx) * dx + h11 + (1.0f - fz) * dz;
        normal.z = dz * s;
    }

    if (m_usePhongNormalInterpolation)                               // +0x44
    {
        getPhongInterpolation(x, z, normal);
    }
    else
    {
        const float nn = normal.y * normal.y + normal.x * normal.x + normal.z * normal.z;
        if (fabsf(nn - 1.0f) >= FLT_EPSILON)
        {
            if (fabsf(nn) >= FLT_EPSILON)
            {
                const float inv = 1.0f / sqrtf(nn);
                normal.x = normal.x * inv;
                normal.y = normal.y * inv;
                normal.z = inv * normal.z;
            }
            else
            {
                normal.x = normal.y = normal.z = 0.0f;
            }
        }
    }
    return height * m_scale.y;
}

// world = local + pos, re-entered virtually (+0xB4).
float Heightmap::getHeightAndNormalInWorldCoords(float x, float z, Vec3& normal,
                                                 unsigned char& material) const
{
    return getHeightAndNormalInLocalCoords(x - static_cast<float>(m_posX),
                                           z - static_cast<float>(m_posZ), normal, material);
}

// Traverses terrain segments with the original split-plane comparisons. A point on a
// plane stays in its arrival half; when both planes are crossed, Z at the X midpoint
// determines traversal order. Preserve these rules at cell corners.
bool Heightmap::intersectRayInWorldCoords(const Vec3& from, const Vec3& to, Vec3& hitPoint,
                                          float& hitFraction, Vec3& hitNormal,
                                          unsigned char& material, bool twoSided)
{
    Vec3 localFrom;
    localFrom.x = from.x - static_cast<float>(m_posX);
    localFrom.y = from.y;
    localFrom.z = from.z - static_cast<float>(m_posZ);
    Vec3 localTo;
    localTo.x = to.x - static_cast<float>(m_posX);
    localTo.y = to.y;
    localTo.z = to.z - static_cast<float>(m_posZ);

    if (!intersectRayInLocalCoords(localFrom, localTo, hitPoint, hitFraction,
                                   hitNormal, material, twoSided))
        return false;

    hitPoint.x += static_cast<float>(m_posX);
    hitPoint.z += static_cast<float>(m_posZ);
    return true;
}

bool Heightmap::intersectRayInLocalCoords(const Vec3& from, const Vec3& to, Vec3& hitPoint,
                                          float& hitFraction, Vec3& hitNormal,
                                          unsigned char& material, bool twoSided)
{
    const float fromX = from.x * m_invScale.x + static_cast<float>(m_halfSizeX);
    const float fromY = from.y;
    const float fromZ = from.z * m_invScale.z + static_cast<float>(m_halfSizeZ);
    const float toX = to.x * m_invScale.x + static_cast<float>(m_halfSizeX);
    const float toY = to.y;
    const float toZ = to.z * m_invScale.z + static_cast<float>(m_halfSizeZ);

    const float deltaX = toX - fromX;
    const float deltaY = toY - fromY;
    const float deltaZ = toZ - fromZ;
    if (fabsf(deltaX) > FLT_EPSILON)
    {
        const float inverseDeltaX = 1.0f / deltaX;
        m_rayZeroDeltaX = false;
        m_rayDzOverDx = inverseDeltaX * deltaZ;
        m_rayDyOverDx = inverseDeltaX * deltaY;
    }
    else
    {
        m_rayZeroDeltaX = true;
    }
    if (fabsf(deltaZ) > FLT_EPSILON)
    {
        const float inverseDeltaZ = 1.0f / deltaZ;
        m_rayZeroDeltaZ = false;
        m_rayDxOverDz = inverseDeltaZ * deltaX;
        m_rayDyOverDz = inverseDeltaZ * deltaY;
    }
    else
    {
        m_rayZeroDeltaZ = true;
    }
    if (m_rayZeroDeltaX && m_rayZeroDeltaZ && fabsf(deltaY) <= FLT_EPSILON)
        return false;

    if (!intersectRay(0, 0, 0, fromX, fromY, fromZ, toX, toY, toZ, twoSided))
        return false;

    if (m_rayHitKind == 1 && m_rayWaterHitPoint.y > m_rayHitPoint.y)
    {
        m_rayHitPoint = m_rayWaterHitPoint;
        m_rayHitNormal = m_rayWaterHitNormal;
    }
    else
    {
        m_rayHitKind = -1;
    }

    hitPoint.x = clampHeightmapRayCoordinate(m_rayHitPoint.x,
                                              static_cast<float>(m_sizeX - 1))
                     - static_cast<float>(m_halfSizeX);
    hitPoint.y = m_rayHitPoint.y;
    hitPoint.z = clampHeightmapRayCoordinate(m_rayHitPoint.z,
                                              static_cast<float>(m_sizeZ - 1))
                     - static_cast<float>(m_halfSizeZ);

    if (m_rayHitKind == -1)
        material = getMaterialFromGrid(static_cast<int>(floorf(hitPoint.x)),
                                       static_cast<int>(floorf(hitPoint.z)));
    else
        material = static_cast<unsigned char>(m_rayHitKind);

    hitPoint.x *= m_scale.x;
    hitPoint.y *= m_scale.x;
    hitPoint.z *= m_scale.x;
    hitFraction = m_rayHitFraction;
    hitNormal = m_rayHitNormal;
    return true;
}

bool Heightmap::splitVerticalRay(float fromX, float fromY, float fromZ, float splitZ,
                                 float& outX, float& outY, float& outZ) const
{
    const float delta = splitZ - fromZ;
    outY = m_rayDyOverDz * delta + fromY;
    outZ = splitZ;
    outX = fromX;
    if (!m_rayZeroDeltaX)
        outX += m_rayDxOverDz * delta;
    return fabsf(delta) > FLT_EPSILON;
}

bool Heightmap::splitHorizontalRay(float fromX, float fromY, float fromZ, float splitX,
                                   float& outX, float& outY, float& outZ) const
{
    const float delta = splitX - fromX;
    outY = m_rayDyOverDx * delta + fromY;
    outX = splitX;
    outZ = fromZ;
    if (!m_rayZeroDeltaZ)
        outZ += m_rayDzOverDx * delta;
    return fabsf(delta) > FLT_EPSILON;
}

bool Heightmap::intersectRay(int level, int cellX, int cellZ,
                             float fromX, float fromY, float fromZ,
                             float toX, float toY, float toZ, bool twoSided)
{
    if (level != m_quadTreeLevels)
    {
        const int dimension = 1 << level;
        const int nodeIndex = cellX + dimension * cellZ;
        float maximumHeight = static_cast<float>(m_quadTree[level][nodeIndex] << 8);
        if (m_waterQuadTree)
        {
            int waterMaximum;
            if (level == m_quadTreeLevels - 1)
            {
                const int waterDimension = dimension / 2;
                waterMaximum = m_waterQuadTree[level - 1]
                    [cellX / 2 + waterDimension * (cellZ / 2)] << 8;
            }
            else
            {
                waterMaximum = m_waterQuadTree[level][nodeIndex] << 8;
            }
            const float waterMaximumHeight = static_cast<float>(waterMaximum);
            if (maximumHeight <= waterMaximumHeight)
                maximumHeight = waterMaximumHeight;
        }
        maximumHeight *= m_scale.y;
        if (fromY > maximumHeight && toY > maximumHeight)
            return false;

        const int span = 1 << (m_quadTreeLevels - level);
        const float midX = (static_cast<float>(cellX) + 0.5f) * static_cast<float>(span);
        const float midZ = (static_cast<float>(cellZ) + 0.5f) * static_cast<float>(span);
        const int nextLevel = level + 1;
        const int childX = 2 * cellX;
        const int childZ = 2 * cellZ;
        float splitX1, splitY1, splitZ1;
        float splitX2, splitY2, splitZ2;

        if (fromX >= midX)
        {
            if (toX >= midX)
            {
                if (fromZ >= midZ)
                {
                    if (toZ < midZ)
                    {
                        if (splitVerticalRay(fromX, fromY, fromZ, midZ,
                                             splitX1, splitY1, splitZ1) &&
                            intersectRay(nextLevel, childX + 1, childZ + 1,
                                         fromX, fromY, fromZ,
                                         splitX1, splitY1, splitZ1, twoSided))
                            return true;
                        return intersectRay(nextLevel, childX + 1, childZ,
                                            splitX1, splitY1, splitZ1,
                                            toX, toY, toZ, twoSided);
                    }
                    return intersectRay(nextLevel, childX + 1, childZ + 1,
                                        fromX, fromY, fromZ, toX, toY, toZ, twoSided);
                }

                if (toZ > midZ)
                {
                    splitVerticalRay(fromX, fromY, fromZ, midZ,
                                     splitX1, splitY1, splitZ1);
                    if (intersectRay(nextLevel, childX + 1, childZ,
                                     fromX, fromY, fromZ,
                                     splitX1, splitY1, splitZ1, twoSided))
                        return true;
                    return intersectRay(nextLevel, childX + 1, childZ + 1,
                                        splitX1, splitY1, splitZ1,
                                        toX, toY, toZ, twoSided);
                }
                return intersectRay(nextLevel, childX + 1, childZ,
                                    fromX, fromY, fromZ, toX, toY, toZ, twoSided);
            }

            if (fromZ >= midZ)
            {
                if (toZ >= midZ)
                {
                    if (splitHorizontalRay(fromX, fromY, fromZ, midX,
                                           splitX1, splitY1, splitZ1) &&
                        intersectRay(nextLevel, childX + 1, childZ + 1,
                                     fromX, fromY, fromZ,
                                     splitX1, splitY1, splitZ1, twoSided))
                        return true;
                    return intersectRay(nextLevel, childX, childZ + 1,
                                        splitX1, splitY1, splitZ1,
                                        toX, toY, toZ, twoSided);
                }

                const float zAtMidX = (midX - fromX) * m_rayDzOverDx + fromZ;
                if (zAtMidX >= midZ)
                {
                    if (splitHorizontalRay(fromX, fromY, fromZ, midX,
                                           splitX1, splitY1, splitZ1) &&
                        intersectRay(nextLevel, childX + 1, childZ + 1,
                                     fromX, fromY, fromZ,
                                     splitX1, splitY1, splitZ1, twoSided))
                        return true;
                    if (splitVerticalRay(splitX1, splitY1, splitZ1, midZ,
                                         splitX2, splitY2, splitZ2) &&
                        intersectRay(nextLevel, childX, childZ + 1,
                                     splitX1, splitY1, splitZ1,
                                     splitX2, splitY2, splitZ2, twoSided))
                        return true;
                    return intersectRay(nextLevel, childX, childZ,
                                        splitX2, splitY2, splitZ2,
                                        toX, toY, toZ, twoSided);
                }

                if (splitVerticalRay(fromX, fromY, fromZ, midZ,
                                     splitX1, splitY1, splitZ1) &&
                    intersectRay(nextLevel, childX + 1, childZ + 1,
                                 fromX, fromY, fromZ,
                                 splitX1, splitY1, splitZ1, twoSided))
                    return true;
                if (splitHorizontalRay(splitX1, splitY1, splitZ1, midX,
                                       splitX2, splitY2, splitZ2) &&
                    intersectRay(nextLevel, childX + 1, childZ,
                                 splitX1, splitY1, splitZ1,
                                 splitX2, splitY2, splitZ2, twoSided))
                    return true;
                return intersectRay(nextLevel, childX, childZ,
                                    splitX2, splitY2, splitZ2,
                                    toX, toY, toZ, twoSided);
            }

            if (toZ <= midZ)
            {
                if (splitHorizontalRay(fromX, fromY, fromZ, midX,
                                       splitX1, splitY1, splitZ1) &&
                    intersectRay(nextLevel, childX + 1, childZ,
                                 fromX, fromY, fromZ,
                                 splitX1, splitY1, splitZ1, twoSided))
                    return true;
                return intersectRay(nextLevel, childX, childZ,
                                    splitX1, splitY1, splitZ1,
                                    toX, toY, toZ, twoSided);
            }

            const float zAtMidX = (midX - fromX) * m_rayDzOverDx + fromZ;
            if (zAtMidX >= midZ)
            {
                splitVerticalRay(fromX, fromY, fromZ, midZ,
                                 splitX1, splitY1, splitZ1);
                if (intersectRay(nextLevel, childX + 1, childZ,
                                 fromX, fromY, fromZ,
                                 splitX1, splitY1, splitZ1, twoSided))
                    return true;
                if (splitHorizontalRay(splitX1, splitY1, splitZ1, midX,
                                       splitX2, splitY2, splitZ2) &&
                    intersectRay(nextLevel, childX + 1, childZ + 1,
                                 splitX1, splitY1, splitZ1,
                                 splitX2, splitY2, splitZ2, twoSided))
                    return true;
                return intersectRay(nextLevel, childX, childZ + 1,
                                    splitX2, splitY2, splitZ2,
                                    toX, toY, toZ, twoSided);
            }

            if (splitHorizontalRay(fromX, fromY, fromZ, midX,
                                   splitX1, splitY1, splitZ1) &&
                intersectRay(nextLevel, childX + 1, childZ,
                             fromX, fromY, fromZ,
                             splitX1, splitY1, splitZ1, twoSided))
                return true;
            splitVerticalRay(splitX1, splitY1, splitZ1, midZ,
                             splitX2, splitY2, splitZ2);
            if (intersectRay(nextLevel, childX, childZ,
                             splitX1, splitY1, splitZ1,
                             splitX2, splitY2, splitZ2, twoSided))
                return true;
            return intersectRay(nextLevel, childX, childZ + 1,
                                splitX2, splitY2, splitZ2,
                                toX, toY, toZ, twoSided);
        }

        if (toX <= midX)
        {
            if (fromZ >= midZ)
            {
                if (toZ < midZ)
                {
                    if (splitVerticalRay(fromX, fromY, fromZ, midZ,
                                         splitX1, splitY1, splitZ1) &&
                        intersectRay(nextLevel, childX, childZ + 1,
                                     fromX, fromY, fromZ,
                                     splitX1, splitY1, splitZ1, twoSided))
                        return true;
                    return intersectRay(nextLevel, childX, childZ,
                                        splitX1, splitY1, splitZ1,
                                        toX, toY, toZ, twoSided);
                }
                return intersectRay(nextLevel, childX, childZ + 1,
                                    fromX, fromY, fromZ, toX, toY, toZ, twoSided);
            }

            if (toZ > midZ)
            {
                splitVerticalRay(fromX, fromY, fromZ, midZ,
                                 splitX1, splitY1, splitZ1);
                if (intersectRay(nextLevel, childX, childZ,
                                 fromX, fromY, fromZ,
                                 splitX1, splitY1, splitZ1, twoSided))
                    return true;
                return intersectRay(nextLevel, childX, childZ + 1,
                                    splitX1, splitY1, splitZ1,
                                    toX, toY, toZ, twoSided);
            }
            return intersectRay(nextLevel, childX, childZ,
                                fromX, fromY, fromZ, toX, toY, toZ, twoSided);
        }

        if (fromZ >= midZ)
        {
            if (toZ >= midZ)
            {
                splitHorizontalRay(fromX, fromY, fromZ, midX,
                                   splitX1, splitY1, splitZ1);
                if (intersectRay(nextLevel, childX, childZ + 1,
                                 fromX, fromY, fromZ,
                                 splitX1, splitY1, splitZ1, twoSided))
                    return true;
                return intersectRay(nextLevel, childX + 1, childZ + 1,
                                    splitX1, splitY1, splitZ1,
                                    toX, toY, toZ, twoSided);
            }

            const float zAtMidX = (midX - fromX) * m_rayDzOverDx + fromZ;
            if (zAtMidX >= midZ)
            {
                splitHorizontalRay(fromX, fromY, fromZ, midX,
                                   splitX1, splitY1, splitZ1);
                if (intersectRay(nextLevel, childX, childZ + 1,
                                 fromX, fromY, fromZ,
                                 splitX1, splitY1, splitZ1, twoSided))
                    return true;
                if (splitVerticalRay(splitX1, splitY1, splitZ1, midZ,
                                     splitX2, splitY2, splitZ2) &&
                    intersectRay(nextLevel, childX + 1, childZ + 1,
                                 splitX1, splitY1, splitZ1,
                                 splitX2, splitY2, splitZ2, twoSided))
                    return true;
                return intersectRay(nextLevel, childX + 1, childZ,
                                    splitX2, splitY2, splitZ2,
                                    toX, toY, toZ, twoSided);
            }

            if (splitVerticalRay(fromX, fromY, fromZ, midZ,
                                 splitX1, splitY1, splitZ1) &&
                intersectRay(nextLevel, childX, childZ + 1,
                             fromX, fromY, fromZ,
                             splitX1, splitY1, splitZ1, twoSided))
                return true;
            splitHorizontalRay(splitX1, splitY1, splitZ1, midX,
                               splitX2, splitY2, splitZ2);
            if (intersectRay(nextLevel, childX, childZ,
                             splitX1, splitY1, splitZ1,
                             splitX2, splitY2, splitZ2, twoSided))
                return true;
            return intersectRay(nextLevel, childX + 1, childZ,
                                splitX2, splitY2, splitZ2,
                                toX, toY, toZ, twoSided);
        }

        if (toZ <= midZ)
        {
            splitHorizontalRay(fromX, fromY, fromZ, midX,
                               splitX1, splitY1, splitZ1);
            if (intersectRay(nextLevel, childX, childZ,
                             fromX, fromY, fromZ,
                             splitX1, splitY1, splitZ1, twoSided))
                return true;
            return intersectRay(nextLevel, childX + 1, childZ,
                                splitX1, splitY1, splitZ1,
                                toX, toY, toZ, twoSided);
        }

        const float zAtMidX = (midX - fromX) * m_rayDzOverDx + fromZ;
        if (zAtMidX >= midZ)
        {
            splitVerticalRay(fromX, fromY, fromZ, midZ,
                             splitX1, splitY1, splitZ1);
            if (intersectRay(nextLevel, childX, childZ,
                             fromX, fromY, fromZ,
                             splitX1, splitY1, splitZ1, twoSided))
                return true;
            splitHorizontalRay(splitX1, splitY1, splitZ1, midX,
                               splitX2, splitY2, splitZ2);
            if (intersectRay(nextLevel, childX, childZ + 1,
                             splitX1, splitY1, splitZ1,
                             splitX2, splitY2, splitZ2, twoSided))
                return true;
            return intersectRay(nextLevel, childX + 1, childZ + 1,
                                splitX2, splitY2, splitZ2,
                                toX, toY, toZ, twoSided);
        }

        splitHorizontalRay(fromX, fromY, fromZ, midX,
                           splitX1, splitY1, splitZ1);
        if (intersectRay(nextLevel, childX, childZ,
                         fromX, fromY, fromZ,
                         splitX1, splitY1, splitZ1, twoSided))
            return true;
        splitVerticalRay(splitX1, splitY1, splitZ1, midZ,
                         splitX2, splitY2, splitZ2);
        if (intersectRay(nextLevel, childX + 1, childZ,
                         splitX1, splitY1, splitZ1,
                         splitX2, splitY2, splitZ2, twoSided))
            return true;
        return intersectRay(nextLevel, childX + 1, childZ + 1,
                            splitX2, splitY2, splitZ2,
                            toX, toY, toZ, twoSided);
    }

    m_rayHitKind = -1;
    if (twoSided && m_waterData)
    {
        const int waterIndex = cellX / 2 + (cellZ / 2) * ((m_sizeX - 1) / 2);
        const float waterLevel = m_waterData[waterIndex];
        if ((fromY > waterLevel && toY <= waterLevel) ||
            (fromY < waterLevel && toY >= waterLevel))
        {
            const float fraction = 1.0f - (toY - waterLevel) / (toY - fromY);
            m_rayWaterHitPoint.x = fromX + (toX - fromX) * fraction;
            m_rayWaterHitPoint.y = m_invScale.x * waterLevel;
            m_rayWaterHitPoint.z = fromZ + (toZ - fromZ) * fraction;
            m_rayWaterHitNormal.x = 0.0f;
            m_rayWaterHitNormal.y = 1.0f;
            m_rayWaterHitNormal.z = 0.0f;
            m_rayHitKind = 1;
            m_rayHitPoint.y = -FLT_MAX;
        }
    }

    int h00 = 0;
    int h10 = 0;
    int h11 = 0;
    int h01 = 0;
    const int heightIndex = cellX + m_sizeX * cellZ;
    if (m_bitResolution == 8)
    {
        const unsigned char* heights = static_cast<const unsigned char*>(m_heightData) + heightIndex;
        h00 = heights[0];
        h10 = heights[1];
        h11 = heights[m_sizeX + 1];
        h01 = heights[m_sizeX];
    }
    else if (m_bitResolution == 16)
    {
        const unsigned short* heights = static_cast<const unsigned short*>(m_heightData) + heightIndex;
        h00 = heights[0];
        h10 = heights[1];
        h11 = heights[m_sizeX + 1];
        h01 = heights[m_sizeX];
    }
    else
    {
        Debug(DEBUGTYPE_6, "Physics/Heightmap.cpp", 1217, __FUNCTION__)
            << "Heightmap bit depth of " << m_bitResolution << " is not supported.";
    }

    const float cellOriginX = static_cast<float>(cellX);
    const float cellOriginZ = static_cast<float>(cellZ);
    const float localX = fromX - cellOriginX;
    const float localZ = fromZ - cellOriginZ;
    const float heightScale = m_scale.y * m_invScale.x;
    const float terrainFromY = fromY * m_invScale.x;
    const float terrainToY = toY * m_invScale.x;
    const float deltaX = toX - fromX;
    const float deltaY = terrainToY - terrainFromY;
    const float deltaZ = toZ - fromZ;
    const float endSum = (toX - cellOriginX) + (toZ - cellOriginZ);

    const float height00 = static_cast<float>(h00) * heightScale;
    const float height10 = static_cast<float>(h10) * heightScale;
    const float height11 = static_cast<float>(h11) * heightScale;
    const float height01 = static_cast<float>(h01) * heightScale;

    if (localX + localZ < 1.0f)
    {
        m_rayHitNormal.x = height00 - height10;
        m_rayHitNormal.y = 1.0f;
        m_rayHitNormal.z = height00 - height01;
        normalizeHeightmapRayNormal(m_rayHitNormal);
        m_rayHitFraction = (toX - (cellOriginX + 1.0f)) * m_rayHitNormal.x
                         + (terrainToY - height10) * m_rayHitNormal.y
                         + (toZ - cellOriginZ) * m_rayHitNormal.z;
        if (m_rayHitFraction <= 0.0f)
        {
            const float deltaDot = deltaX * m_rayHitNormal.x
                                 + deltaY * m_rayHitNormal.y
                                 + deltaZ * m_rayHitNormal.z;
            if (m_rayHitFraction <= deltaDot)
            {
                m_rayHitPoint.x = fromX;
                m_rayHitPoint.y = height00
                                + (height10 - height00) * localX
                                + (height01 - height00) * localZ;
                m_rayHitPoint.z = fromZ;
                ++s_heightmapRayInsideCount;
            }
            else
            {
                const float fraction = m_rayHitFraction / deltaDot;
                m_rayHitPoint.x = toX - deltaX * fraction;
                m_rayHitPoint.y = terrainToY - deltaY * fraction;
                m_rayHitPoint.z = toZ - deltaZ * fraction;
                ++s_heightmapRayCrossingCount;
            }
            if ((m_rayHitPoint.x - cellOriginX) +
                (m_rayHitPoint.z - cellOriginZ) <= 1.0f)
                return true;
        }
        if (endSum < 1.0f)
            return m_rayHitKind == 1;

        m_rayHitNormal.x = height01 - height11;
        m_rayHitNormal.y = 1.0f;
        m_rayHitNormal.z = height10 - height11;
        normalizeHeightmapRayNormal(m_rayHitNormal);
        m_rayHitFraction = (toX - (cellOriginX + 1.0f)) * m_rayHitNormal.x
                         + (terrainToY - height11) * m_rayHitNormal.y
                         + (toZ - (cellOriginZ + 1.0f)) * m_rayHitNormal.z;
        if (m_rayHitFraction > 0.0f)
            return m_rayHitKind == 1;

        const float deltaDot = deltaX * m_rayHitNormal.x
                             + deltaY * m_rayHitNormal.y
                             + deltaZ * m_rayHitNormal.z;
        if (m_rayHitFraction > deltaDot)
        {
            const float fraction = m_rayHitFraction / deltaDot;
            m_rayHitPoint.x = toX - deltaX * fraction;
            m_rayHitPoint.y = terrainToY - deltaY * fraction;
            m_rayHitPoint.z = toZ - deltaZ * fraction;
            ++s_heightmapRayCrossingCount;
            return true;
        }
        m_rayHitPoint.x = fromX;
        m_rayHitPoint.y = height11
                        + (height10 - height11) * (1.0f - localZ)
                        + (height01 - height11) * (1.0f - localX);
        m_rayHitPoint.z = fromZ;
        ++s_heightmapRayInsideCount;
        return true;
    }

    m_rayHitNormal.x = height01 - height11;
    m_rayHitNormal.y = 1.0f;
    m_rayHitNormal.z = height10 - height11;
    normalizeHeightmapRayNormal(m_rayHitNormal);
    m_rayHitFraction = (toX - (cellOriginX + 1.0f)) * m_rayHitNormal.x
                     + (terrainToY - height11) * m_rayHitNormal.y
                     + (toZ - (cellOriginZ + 1.0f)) * m_rayHitNormal.z;
    if (m_rayHitFraction <= 0.0f)
    {
        const float deltaDot = deltaX * m_rayHitNormal.x
                             + deltaY * m_rayHitNormal.y
                             + deltaZ * m_rayHitNormal.z;
        if (m_rayHitFraction <= deltaDot)
        {
            m_rayHitPoint.x = fromX;
            m_rayHitPoint.y = height11
                            + (height10 - height11) * (1.0f - localZ)
                            + (height01 - height11) * (1.0f - localX);
            m_rayHitPoint.z = fromZ;
            ++s_heightmapRayInsideCount;
        }
        else
        {
            const float fraction = m_rayHitFraction / deltaDot;
            m_rayHitPoint.x = toX - deltaX * fraction;
            m_rayHitPoint.y = terrainToY - deltaY * fraction;
            m_rayHitPoint.z = toZ - deltaZ * fraction;
            ++s_heightmapRayCrossingCount;
        }
        if ((m_rayHitPoint.x - cellOriginX) +
            (m_rayHitPoint.z - cellOriginZ) >= 1.0f)
            return true;
    }
    if (endSum >= 1.0f)
        return m_rayHitKind == 1;

    m_rayHitNormal.x = height00 - height10;
    m_rayHitNormal.y = 1.0f;
    m_rayHitNormal.z = height00 - height01;
    normalizeHeightmapRayNormal(m_rayHitNormal);
    m_rayHitFraction = (toX - (cellOriginX + 1.0f)) * m_rayHitNormal.x
                     + (terrainToY - height10) * m_rayHitNormal.y
                     + (toZ - cellOriginZ) * m_rayHitNormal.z;
    if (m_rayHitFraction > 0.0f)
        return m_rayHitKind == 1;

    const float deltaDot = deltaX * m_rayHitNormal.x
                         + deltaY * m_rayHitNormal.y
                         + deltaZ * m_rayHitNormal.z;
    if (m_rayHitFraction > deltaDot)
    {
        const float fraction = m_rayHitFraction / deltaDot;
        m_rayHitPoint.x = toX - deltaX * fraction;
        m_rayHitPoint.y = terrainToY - deltaY * fraction;
        m_rayHitPoint.z = toZ - deltaZ * fraction;
        ++s_heightmapRayCrossingCount;
    }
    else
    {
        m_rayHitPoint.x = fromX;
        m_rayHitPoint.y = height00
                        + (height10 - height00) * localX
                        + (height01 - height00) * localZ;
        m_rayHitPoint.z = fromZ;
        ++s_heightmapRayInsideCount;
    }
    return true;
}

void Heightmap::setSize(int x, int z)
{
    m_sizeX = x;
    m_halfSizeX = x / 2;
    m_sizeZ = z;
    m_halfSizeZ = z / 2;
}
void Heightmap::getSize(int& x, int& z) const { x = m_sizeX; z = m_sizeZ; }
void Heightmap::setPos(int x, int z) { m_posX = x; m_posZ = z; }
void Heightmap::getPos(int& x, int& z) const { x = m_posX; z = m_posZ; }

// (pos - half) * scale / (pos + half) * scale, per axis - raw-read shape.
float Heightmap::getLowBoundaryX() const
{ return (static_cast<float>(m_posX) - static_cast<float>(m_halfSizeX)) * m_scale.x; }
float Heightmap::getHighBoundaryX() const
{ return (static_cast<float>(m_posX) + static_cast<float>(m_halfSizeX)) * m_scale.x; }
float Heightmap::getLowBoundaryZ() const
{ return (static_cast<float>(m_posZ) - static_cast<float>(m_halfSizeZ)) * m_scale.z; }
float Heightmap::getHighBoundaryZ() const
{ return (static_cast<float>(m_posZ) + static_cast<float>(m_halfSizeZ)) * m_scale.z; }

void Heightmap::setBitResolution(int bits) { m_bitResolution = bits; }
int  Heightmap::getBitResolution() const   { return m_bitResolution; }

// stores the scale and the per-axis inverse, each zero-guarded to 0.
void Heightmap::setScale(const Vec3& scale)
{
    m_scale = scale;
    m_invScale.x = (m_scale.x == 0.0f) ? 0.0f : 1.0f / m_scale.x;
    m_invScale.y = (m_scale.y == 0.0f) ? 0.0f : 1.0f / m_scale.y;
    m_invScale.z = (m_scale.z == 0.0f) ? 0.0f : 1.0f / m_scale.z;
}
const Vec3& Heightmap::getScale() const { return m_scale; }

void Heightmap::setSeaWaterLevel(float level) { m_seaWaterLevel = level; }
float Heightmap::getSeaWaterLevel() { return m_seaWaterLevel; }

int  Heightmap::getMaterialScale() const  { return m_materialScale; }
void Heightmap::setMaterialScale(int scale) { m_materialScale = scale; }

// the material twin of loadHeightData: materialScale^2 * sizeX * sizeZ bytes,
// one read, same unguarded open.
bool Heightmap::loadMaterialData(const std::string& name)
{
    if (m_materialData)
        delete[] m_materialData;

    io::IStream* f = io::fileManager->open(name, (io::IFileSystem::FileMode)2,
                                           (io::IFileSystem::FileAccess)0, 0);
    const unsigned int bytes = m_materialScale * m_materialScale * m_sizeX * m_sizeZ;
    m_materialData = new unsigned char[bytes];
    f->read(m_materialData, bytes);
    f->release();
    return true;                                 // [CHOSEN] - the close result materialises here
}

bool Heightmap::saveMaterialData(const std::string&) const { return false; }   // 7 B shipped stub

// Reads an 8- or 16-bit sample from the center-origin grid. Other bit depths emit
// the original process-fatal diagnostic.
int Heightmap::getHeightFromGrid(int x, int z) const
{
    const int index = m_halfSizeX + x + (m_halfSizeZ + z) * m_sizeX;
    if (m_bitResolution == 8)
        return static_cast<const unsigned char*>(m_heightData)[index];
    if (m_bitResolution == 16)
        return static_cast<const unsigned short*>(m_heightData)[index];

    Debug(DEBUGTYPE_6, "Physics/Heightmap.cpp", 462, __FUNCTION__)
        << "Heightmap bit depth of " << m_bitResolution << " is not supported.";
    return 0;
}

// Samples the cell's two triangles according to 1-fx <= fz, collapsing exact edges and
// corners to fewer samples. Uses virtual getHeightFromGrid for editable overrides and
// scales the result by scale.y.
float Heightmap::getHeightInLocalCoords(float x, float z) const
{
    const float gx = m_invScale.x * x;
    const float gz = m_invScale.z * z;
    const int ix = static_cast<int>(floorf(gx));
    const int iz = static_cast<int>(floorf(gz));
    const float fx = gx - static_cast<float>(ix);
    const float fz = gz - static_cast<float>(iz);

    float base, blend;
    if (fx <= 0.0f)
    {
        if (fz <= 0.0f)
        {
            base = static_cast<float>(getHeightFromGrid(ix, iz));
            blend = 0.0f;
        }
        else
        {
            base = static_cast<float>(getHeightFromGrid(ix, iz));
            blend = (static_cast<float>(getHeightFromGrid(ix, iz + 1)) - base) * fz;
        }
    }
    else if (fz <= 0.0f)
    {
        base = static_cast<float>(getHeightFromGrid(ix, iz));
        blend = (static_cast<float>(getHeightFromGrid(ix + 1, iz)) - base) * fx;
    }
    else
    {
        const float gxInv = 1.0f - fx;
        if (gxInv <= fz)
        {
            const float fzInv = 1.0f - fz;
            base = static_cast<float>(getHeightFromGrid(ix + 1, iz + 1));
            blend = (static_cast<float>(getHeightFromGrid(ix + 1, iz)) - base) * fzInv
                  + (static_cast<float>(getHeightFromGrid(ix, iz + 1)) - base) * gxInv;
        }
        else
        {
            base = static_cast<float>(getHeightFromGrid(ix, iz));
            blend = (static_cast<float>(getHeightFromGrid(ix, iz + 1)) - base) * fz
                  + (static_cast<float>(getHeightFromGrid(ix + 1, iz)) - base) * fx;
        }
    }
    return (blend + base) * m_scale.y;
}

// world = local + pos: the world wrappers subtract the tile offset and re-enter virtually (the
// local variant is a vtable slot; the Editable chain depends on the dispatch).
float Heightmap::getHeightInWorldCoords(float x, float z) const
{
    return getHeightInLocalCoords(x - static_cast<float>(m_posX),
                                  z - static_cast<float>(m_posZ));
}
float Heightmap::getHeightInLocalCoords(const Vec3& p) const
{ return getHeightInLocalCoords(p.x, p.z); }
float Heightmap::getHeightInWorldCoords(const Vec3& p) const
{ return getHeightInWorldCoords(p.x, p.z); }

// The water chain - quarter-resolution float grid, or the sea level when none is baked yet.
// getWaterLevelFromGrid (0x83846B0, 88 B): width is (sizeX-1)/2, the incoming grid coords are
// halved half-sizes offsets - the exact index arithmetic as read.
float Heightmap::getWaterLevelFromGrid(int x, int z) const
{
    const int width = (m_sizeX - 1) / 2;
    return m_waterData[width * (z + m_halfSizeZ / 2) + x + m_halfSizeX / 2];
}

float Heightmap::getWaterLevelInLocalCoords(float x, float z) const
{
    if (!m_waterData)
        return m_seaWaterLevel;
    const int gx = static_cast<int>(m_invScale.x * x);   // truncation, as the raw shows
    const int gz = static_cast<int>(m_invScale.z * z);
    return getWaterLevelFromGrid(gx / 2, gz / 2);
}
float Heightmap::getWaterLevelInWorldCoords(float x, float z) const
{
    return getWaterLevelInLocalCoords(x - static_cast<float>(m_posX),
                                      z - static_cast<float>(m_posZ));
}
float Heightmap::getWaterLevelInLocalCoords(const Vec3& p) const
{ return getWaterLevelInLocalCoords(p.x, p.z); }
float Heightmap::getWaterLevelInWorldCoords(const Vec3& p) const
{ return getWaterLevelInWorldCoords(p.x, p.z); }

// Builds a reusable byte-valued maximum-height pyramid. Finest cells cover a 3x3 sample
// stencil; 16-bit heights round upward when narrowed. Coarser cells take the maximum
// of four children for conservative ray-query rejection.
int Heightmap::createQuadTree(int size)
{
    m_quadTreeLevels = 0;
    int span = size - 1;
    if (span > 1)
    {
        int levels = 0;
        do { span >>= 1; ++levels; } while (span > 1);
        m_quadTreeLevels = levels;
    }

    if (!m_quadTree)
    {
        m_quadTree = new unsigned char*[m_quadTreeLevels];
        for (int i = 0; i < m_quadTreeLevels; ++i)
            m_quadTree[i] = 0;
    }

    int result = m_quadTreeLevels - 1;
    for (int level = m_quadTreeLevels - 1; level >= 0; --level)
    {
        const int dim = 1 << level;
        if (!m_quadTree[level])
            m_quadTree[level] = new unsigned char[dim * dim];

        for (int cz = 0; cz < dim; ++cz)
        {
            for (int cx = 0; cx < dim; ++cx)
            {
                unsigned char cell;
                if (level == m_quadTreeLevels - 1)
                {
                    unsigned int best = 0;
                    for (int k = 0; k <= 8; ++k)
                    {
                        const unsigned int h = static_cast<unsigned int>(
                            getHeightFromGrid(2 * cx + k % 3 - m_halfSizeX,
                                              2 * cz + k / 3 - m_halfSizeZ));
                        if (h >= best)
                            best = h;
                    }
                    cell = (m_bitResolution > 8)
                        ? static_cast<unsigned char>(
                              (best + (1u << (m_bitResolution - 8)) - 1) >> (m_bitResolution - 8))
                        : static_cast<unsigned char>(best);
                }
                else
                {
                    unsigned char best = 0;
                    for (int m = 0; m <= 3; ++m)
                    {
                        const unsigned char h =
                            m_quadTree[level + 1][2 * cx + m % 2 + 2 * dim * (2 * cz + m / 2)];
                        if (h > best)
                            best = h;
                    }
                    cell = best;
                }
                m_quadTree[level][cz * dim + cx] = cell;
                result = cz * dim + cx;          // the raw's running value, materialised
            }
        }
    }
    return result;
}

// Bakes collision-mesh water surfaces into this tile's half-resolution water grid.
// Resets cells to sea level, casts downward through up to 100 surfaces and collects each participating
// water object once for the cluster. Registered materials and collision LOD 0 are required.
// Raises eight neighboring cells using the retail flat-index bounds test, without a column-wrap guard,
// then rebuilds the water quadtree. Preserve the ray-stepping arithmetic below non-water hits.
void Heightmap::compileWaterMeshesIntoHeightmap(std::vector<IObject*>& waterObjects)
{
    const int n = m_sizeX - 1;
    const int waterCells = n * (n / 2) / 2;
    const float maxHeight = 65280.0f * m_scale.y;
    if (!m_waterData)
        m_waterData = new float[waterCells];
    for (int i = 0; i < waterCells; ++i)
        m_waterData[i] = m_seaWaterLevel;

    std::vector<IObject*> objects;
    const int half = n / 2;                       // v49 / 2 - signed division, as shipped
    const int lo   = ((n > 0) - n) >> 1;          // -n/2 for the even sizes the content ships
    for (int z = lo; z < half; z += 2)
    {
        for (int x = lo; x < half; x += 2)
        {
            Vec3 start;
            start.x = static_cast<float>(x + 1) * m_scale.x;
            start.y = maxHeight;
            start.z = static_cast<float>(z + 1) * m_scale.z;
            Vec3 dir;
            dir.x = 0.0f;
            dir.y = 0.0f - maxHeight;
            dir.z = 0.0f;

            objects.resize(0);
            ObjectFlagPredicator predicator(0x1000100u);
            objectManager->getObjectsFromGrid(start, dir, objects, &predicator, false, 0, false);

            for (unsigned int o = 0; o < objects.size(); ++o)
            {
                IObject* object = objects[o];
                IObjectTemplate* objectTemplate = static_cast<IObjectTemplate*>(
                    object->getTemplate()->queryInterface(IID_IObjectTemplate));   // +0x14 -> qi
                if (!objectTemplate)
                    continue;

                // the first water-physics material in the template's material vector
                std::vector<std::pair<std::string, unsigned int> >* materials =
                    static_cast<ObjectTemplate*>(objectTemplate)->getMaterialVector();
                bool namesWater = false;
                for (unsigned int k = 0; k < materials->size(); ++k)
                {
                    Material* material = static_cast<MaterialManager*>(materialManager)
                                             ->getMaterialPtr((*materials)[k].second);
                    if (material->getHasWaterPhysics() == 1)          // null material = shipped crash
                    {
                        namesWater = true;
                        break;
                    }
                }
                if (!namesWater)
                    continue;
                if (!object->hasCollisionLod(0))
                    continue;

                ICollisionMesh* mesh = static_cast<ICollisionMesh*>(
                    object_detail::LayoutAccess::collisionMesh(object));      // the raw +0x40 read
                Vec3 hitNormal;   hitNormal.x = hitNormal.y = hitNormal.z = 0.0f;
                Vec3 hitPosition; hitPosition.x = hitPosition.y = hitPosition.z = 0.0f;
                for (int attempt = 0; attempt <= 99; ++attempt)
                {
                    const Mat4& worldToObject = object->getInvAbsoluteTransformation();   // +0x88
                    const Mat4& objectToWorld = object->getAbsoluteTransformation();      // +0x84
                    float planeDist = 0.0f, along = 0.0f;
                    int hitMaterial = 0;
                    if (mesh->getDistance(0, false, objectToWorld, worldToObject, start, dir,
                                          hitNormal, hitPosition, planeDist, along, &hitMaterial,
                                          0, true, false) != true)
                        break;
                    Material* material = static_cast<MaterialManager*>(materialManager)
                                             ->getMaterialPtr(static_cast<unsigned int>(hitMaterial));
                    if (material->getHasWaterPhysics() == 1)
                    {
                        // remember the object (once)
                        bool known = false;
                        for (unsigned int w = 0; w < waterObjects.size() && !known; ++w)
                            if (waterObjects[w] == object)
                                known = true;
                        if (!known)
                            waterObjects.push_back(object);

                        // the cell, then its eight neighbours raised to at least this level
                        const int cell = half * ((m_halfSizeZ + z) / 2) + (m_halfSizeX + x) / 2;
                        m_waterData[cell] = hitPosition.y;
                        for (int row = -half; row <= half; row += half)
                        {
                            for (int col = -1; col <= 1; ++col)
                            {
                                const int j = cell + row + col;
                                if (j >= 0 && j < waterCells && m_waterData[cell] > m_waterData[j])
                                    m_waterData[j] = m_waterData[cell];
                            }
                        }
                        break;
                    }
                    // not water: step the probe to just below the hit and cast again
                    const float newDirY = dir.y - (start.y - hitPosition.y);
                    start.y = hitPosition.y - 0.001f;
                    dir.y = newDirY;
                }
            }
        }
    }
    createWaterQuadTree();
}

// Builds a reusable maximum-water-height pyramid, clamping negative water to zero and
// rounding positive heights upward to byte units. Coarser cells take four-child maxima.
// The original return type is unproven; intptr_t preserves its unused pointer/index value across ports.
std::intptr_t Heightmap::createWaterQuadTree()
{
    const int levels = m_quadTreeLevels - 1;
    if (!m_waterQuadTree)
    {
        m_waterQuadTree = new unsigned char*[levels];
        for (int i = 0; i < levels; ++i)
            m_waterQuadTree[i] = 0;
    }
    std::intptr_t result = reinterpret_cast<std::intptr_t>(m_waterQuadTree);
    const int finest = levels - 1;
    for (int level = levels - 1; level >= 0; --level)
    {
        const int dim = 1 << level;
        if (!m_waterQuadTree[level])
            m_waterQuadTree[level] = new unsigned char[dim * dim];
        for (int cz = 0; cz < dim; ++cz)
        {
            for (int cx = 0; cx < dim; ++cx)
            {
                unsigned char cell;
                if (level == finest)
                {
                    float best = 0.0f;
                    for (int m = 0; m <= 3; ++m)
                    {
                        const float w = getWaterLevelFromGrid(2 * cx + m % 2 - m_halfSizeX / 2,
                                                              2 * cz + m / 2 - m_halfSizeZ / 2);
                        if (best <= w)
                            best = w;
                    }
                    int q = static_cast<int>(best * m_invScale.y);        // cvttss2si
                    if (q < 0) q = 0;
                    if (q > 65280) q = 65280;
                    const int rounded = q ? q + 256 : 0;
                    cell = static_cast<unsigned char>(rounded >> 8);
                }
                else
                {
                    unsigned char best = 0;
                    for (int m = 0; m <= 3; ++m)
                    {
                        const unsigned char h =
                            m_waterQuadTree[level + 1][2 * cx + m % 2 + 2 * dim * (2 * cz + m / 2)];
                        if (h >= best)
                            best = h;
                    }
                    cell = best;
                }
                m_waterQuadTree[level][cz * dim + cx] = cell;
                result = cz * dim + cx;
            }
        }
    }
    return result;
}

} // namespace world
} // namespace hfe
} // namespace dice
