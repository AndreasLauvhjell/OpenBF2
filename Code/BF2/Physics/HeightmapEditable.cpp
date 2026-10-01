// Editable heightmap operations shared by engine images and instantiated by the editor.
// Game cluster creation strips the Editable suffix. Source path is attested by __FILE__.

#include "HeightmapEditable.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

#include "../Core/Debug.h"
#include "../Core/EventManager.h"
#include "../Core/Ids.h"
#include "../Game/IEventListener.h"
#include "../Geom/HeightmapEventNode.h"
#include "../IO/File/FileManager.h"
#include "../IO/File/IFileSystem.h"
#include "../IO/Stream/IStream.h"

namespace dice {
namespace hfe {
namespace world {

// lifetime

// ELF 0x838BA70 (C1) / 0x838BA20 (C2), 70 B; The base, the two extra vptrs,
// then +0xD4 = 0 and the +0xD8 byte = 0.
HeightmapEditable::HeightmapEditable()
    : m_editBuffer(0)
    , m_modified(false)
{
}

// ELF 0x838BB10 (D1) / 0x838BB60 (D0); PE 0x6ACD80. Nothing of its own: the vptrs are reset and
// the bases torn down. m_editBuffer is not freed - neither image deletes it here. Reproduced.
HeightmapEditable::~HeightmapEditable()
{
}

// ; PE 0x691840. The module-registry shape: refuse aggregation, allocate
// (0xDC ELF / 0xF4 PE), hand back queryInterface(iid) or release and answer null. No addRef - the
// constructor's reference is the one returned.
IBase* HeightmapEditable::create(unsigned int iid, IBase* outer)
{
    if (outer != 0)
        return 0;

    HeightmapEditable* self = new HeightmapEditable();
    IBase* result = static_cast<IBase*>(self->queryInterface(iid));
    if (!result)
        self->release();
    return result;
}

// IBase - the refcount is Heightmap's (+0x04).

void HeightmapEditable::addRef() { ++m_refCount; }
int  HeightmapEditable::getRef() { return m_refCount; }

// decrement, and on the last reference the deleting destructor through the
// primary table (`call [vptr+14h]`), null-tested.
int HeightmapEditable::release()
{
    const int refs = --m_refCount;
    if (refs == 0)
        delete this;
    return refs;
}

// ; PE 0x6ACC60 (a switch). IHeightmapEditable -> +0xCC, IConsoleSaveable ->
// +0xD0, IBase and IHeightmap -> this; anything else null.
void* HeightmapEditable::queryInterface(unsigned int iid) const
{
    HeightmapEditable* self = const_cast<HeightmapEditable*>(this);
    if (iid == IID_IHeightmapEditable)
        return static_cast<IHeightmapEditable*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    if (iid == IID_IBase)
        return static_cast<IHeightmap*>(self);
    if (iid == IID_IHeightmap)
        return static_cast<IHeightmap*>(self);
    return 0;
}

// persistence

// ; The base's parser, then the edit buffer reallocated at
// the grid's byte size (signed division: `(n + 7) >> 3` for a negative n, as `/ 8` compiles).
bool HeightmapEditable::loadHeightData(const std::string& name)
{
    Heightmap::loadHeightData(name);

    if (m_editBuffer)
        delete[] m_editBuffer;
    m_editBuffer = new unsigned char[m_bitResolution * m_sizeX * m_sizeZ / 8];
    return true;    // [CHOSEN] carrier, as Heightmap::loadHeightData: the binary leaves the new[]
                    // pointer in the return register and no caller reads it
}

// ; No grid -> false. An empty name falls back to the
// name the grid was loaded under (+0x08). The file opens for writing (mode 0, access 2 - the
// Demo::openFileForWriting arguments); a failed open answers false; otherwise one write of the
// whole grid and true only if every byte went out.
bool HeightmapEditable::saveHeightData(const std::string& name) const
{
    if (!m_heightData)
        return false;

    std::string fileName(name);
    if (fileName.length() == 0)
        fileName.assign(m_name);

    io::IStream* stream = io::fileManager->open(fileName, (io::IFileSystem::FileMode)0,
                                                (io::IFileSystem::FileAccess)2, 0);
    if (!stream)
        return false;

    const unsigned int bytes = m_bitResolution * m_sizeX * m_sizeZ / 8;
    const unsigned int written = stream->write(m_heightData, bytes);
    stream->release();
    return written == bytes;
}

// ; The material twin: no grid -> false, no name
// fallback. The byte count is materialScale * sizeX * sizeZ - not squared (ELF [+B8h] * [+18h]
// * [+1Ch]; PE [+30h] * [+34h] * [+D0h]) - while the loader reads materialScale^2 * sizeX *
// sizeZ. The two agree only for the stock scale of 1. Reproduced.
bool HeightmapEditable::saveMaterialData(const std::string& name) const
{
    if (!m_materialData)
        return false;

    io::IStream* stream = io::fileManager->open(name, (io::IFileSystem::FileMode)0,
                                                (io::IFileSystem::FileAccess)2, 0);
    if (!stream)
        return false;

    const unsigned int bytes = m_materialScale * m_sizeX * m_sizeZ;
    const unsigned int written = stream->write(m_materialData, bytes);
    stream->release();
    return written == bytes;
}

// ; A fresh flat grid: both buffers freed and reallocated
// (grid bytes each; either null -> false), every cell set to height / scale.y (the float quotient
// truncated), each cell's edit byte zeroed, the quadtree rebuilt. Any depth but 8 or 16 is
// Physics/HeightmapEditable.cpp:90, fatal, and the loop carries on past it in the binary.
bool HeightmapEditable::createHeightData(int height)
{
    if (m_heightData)
        delete[] static_cast<unsigned char*>(m_heightData);
    if (m_editBuffer)
        delete[] m_editBuffer;

    const unsigned int bytes = m_bitResolution * m_sizeX * m_sizeZ / 8;
    m_heightData = new unsigned char[bytes];
    m_editBuffer = new unsigned char[bytes];
    if (!m_heightData || !m_editBuffer)
        return false;

    const unsigned short value =
        static_cast<unsigned short>(static_cast<int>(static_cast<float>(height) / m_scale.y));
    for (int i = 0; i < m_sizeX * m_sizeZ; ++i)
    {
        if (m_bitResolution == 8)
            static_cast<unsigned char*>(m_heightData)[i] = static_cast<unsigned char>(value);
        else if (m_bitResolution == 16)
            static_cast<unsigned short*>(m_heightData)[i] = value;
        else
            Debug(DEBUGTYPE_6, "Physics/HeightmapEditable.cpp", 90, __FUNCTION__)
                << "Heightmap bit depth of " << m_bitResolution << " is not supported.";
        m_editBuffer[i] = 0;
    }
    createQuadTree(m_sizeX);
    return true;
}

// ; materialScale * sizeX * sizeZ bytes (not squared -
// the same count saveMaterialData writes), all set to the given material.
bool HeightmapEditable::createMaterialData(int material)
{
    if (m_materialData)
        delete[] m_materialData;

    const unsigned int bytes = m_materialScale * m_sizeX * m_sizeZ;
    m_materialData = new unsigned char[bytes];
    memset(m_materialData, material, bytes);
    return true;
}

// ; PE 0x6AC600. Writes nothing, answers false.
bool HeightmapEditable::makeScript(io::IStream*)
{
    return false;
}

// the dirty flag (+0xD8) and the RAW grid

bool  HeightmapEditable::getModified() const          { return m_modified; }
void  HeightmapEditable::setModified(bool modified)    { m_modified = modified; }
void* HeightmapEditable::getHeightData()               { return m_heightData; }

// ; Replace the grid with a copy of `data` (grid bytes),
// rebuild the quadtree, then announce the whole tile as modified: the area is the grid's
// centre-origin extent (1 - halfX, 1 - halfZ, halfX - 1, halfZ - 1) in grid units, as floats.
// The edit buffer is not touched.
void HeightmapEditable::setHeightData(void* data)
{
    if (m_heightData)
        delete[] static_cast<unsigned char*>(m_heightData);

    const unsigned int bytes = m_bitResolution * m_sizeX * m_sizeZ / 8;
    m_heightData = new unsigned char[bytes];
    memcpy(m_heightData, data, bytes);
    createQuadTree(m_sizeX);

    geom::HeightmapEventNode* node = new geom::HeightmapEventNode(100);
    Vec4 area;
    area.x = static_cast<float>(1 - m_halfSizeX);
    area.y = static_cast<float>(1 - m_halfSizeZ);
    area.z = static_cast<float>(m_halfSizeX - 1);
    area.w = static_cast<float>(m_halfSizeZ - 1);
    node->setAffectedArea(area);
    eventManager->postEvent(ECEditor, node, 0.0f);
}

// ; The base's sampler with a bounds check the base lacks:
// outside -half..+half on either axis the answer is 0.
int HeightmapEditable::getHeightFromGrid(int x, int z) const
{
    if (x < -m_halfSizeX || x > m_halfSizeX)
        return 0;
    if (z < -m_halfSizeZ || z > m_halfSizeZ)
        return 0;

    const int index = m_halfSizeX + x + m_sizeX * (m_halfSizeZ + z);
    if (m_bitResolution == 8)
        return static_cast<const unsigned char*>(m_heightData)[index];
    if (m_bitResolution == 16)
        return static_cast<const unsigned short*>(m_heightData)[index];

    Debug(DEBUGTYPE_6, "Physics/HeightmapEditable.cpp", 181, __FUNCTION__)
        << "Heightmap bit depth of " << m_bitResolution << " is not supported.";
    return 0;
}

// writing samples

// ; Store one raw sample (centre-origin grid coords, no
// bounds check) under the rise/fall flags; a store refreshes the quadtree column and marks the
// tile modified. Any depth but 8/16 is Physics/HeightmapEditable.cpp:221, fatal - and in the
// binary that arm falls through to the quadtree refresh and the flag.
void HeightmapEditable::setHeight(int x, int z, unsigned short height, int flags)
{
    const int index = m_halfSizeX + x + m_sizeX * (m_halfSizeZ + z);
    if (m_bitResolution == 8)
    {
        unsigned char* sample = static_cast<unsigned char*>(m_heightData) + index;
        const unsigned char value = static_cast<unsigned char>(height);
        if (((flags & 1) != 0 || value <= *sample) && ((flags & 2) != 0 || value >= *sample))
        {
            *sample = value;
            updateQuadtreeAt(x, z, height);
            m_modified = true;
        }
    }
    else if (m_bitResolution == 16)
    {
        unsigned short* sample = static_cast<unsigned short*>(m_heightData) + index;
        if (((flags & 1) != 0 || height <= *sample) && ((flags & 2) != 0 || height >= *sample))
        {
            *sample = height;
            updateQuadtreeAt(x, z, height);
            m_modified = true;
        }
    }
    else
    {
        Debug(DEBUGTYPE_6, "Physics/HeightmapEditable.cpp", 221, __FUNCTION__)
            << "Heightmap bit depth of " << m_bitResolution << " is not supported.";
        updateQuadtreeAt(x, z, height);
        m_modified = true;
    }
}

// ; PE 0x6AC980 (the IHeightmapEditable thunk target). Both flags.
void HeightmapEditable::setHeightRaw(int x, int z, unsigned short height)
{
    setHeight(x, z, height, 3);
}

// ; PE 0x6ACB60 (reached by no vtable and no caller in either image). The
// current sample (virtual getHeightFromGrid, +0x84 PE) plus delta, clamped to 0..255 (8-bit) or
// 0..0xFF00 (16-bit - the quadtree's ceiling), stored, and the tile marked modified. Other depths:
// line 135, fatal, then the clamp runs against 0.
void HeightmapEditable::addHeight(int x, int z, int delta, int flags)
{
    int value = getHeightFromGrid(x, z) + delta;
    int ceiling = 0;
    if (m_bitResolution == 8)
        ceiling = 255;
    else if (m_bitResolution == 16)
        ceiling = 0xFF00;
    else
        Debug(DEBUGTYPE_6, "Physics/HeightmapEditable.cpp", 135, __FUNCTION__)
            << "Heightmap bit depth of " << m_bitResolution << " is not supported.";

    if (value <= 0)
        value = 0;
    if (value > ceiling)
        value = ceiling;
    setHeight(x, z, static_cast<unsigned short>(value), flags);
    m_modified = true;
}

// ; Raise the max-height pyramid over one sample: the
// sample reduced to the pyramid's 8 bits by a ceiling shift ((height + 2^(bits-8) - 1) >>
// (bits-8)), then every level's cell containing (x, z) - level L is 2^L cells across, a cell
// spans 2^(levels-L) samples - takes it if larger. Only ever raises; a lowered sample leaves the
// old maximum in place, as shipped.
void HeightmapEditable::updateQuadtreeAt(int x, int z, unsigned short height)
{
    const int shift = m_bitResolution - 8;
    const unsigned char value =
        static_cast<unsigned char>(((1 << shift) + height - 1) >> shift);
    for (int level = 0; level < m_quadTreeLevels; ++level)
    {
        const int span = 1 << (m_quadTreeLevels - level);
        unsigned char* cell = m_quadTree[level]
                              + (m_halfSizeX + x) / span
                              + (1 << level) * ((m_halfSizeZ + z) / span);
        if (value > *cell)
            *cell = value;
    }
}

// ; PE 0x6AC6A0 (IHeightmapEditable thunk target). World to local by the tile
// position, then setHeightInLocalCoords with both flags - through the vtable.
void HeightmapEditable::setHeightInWorldCoords(float x, float z, float height)
{
    setHeightInLocalCoords(x - static_cast<float>(m_posX), z - static_cast<float>(m_posZ), height, 3);
}

// ; PE 0x6AC6D0. The world-scaled twin: the height is first converted to grid
// units by the member at ELF +0x3C / PE +0x54 (the IHeightmapEditable this - 0x90), which is
// m_invScale.y; then as above, with the caller's flags.
void HeightmapEditable::setWorldScaledHeightInWorldCoords(float x, float z, float height, int flags)
{
    const float gridHeight = height * m_invScale.y;
    setHeightInLocalCoords(x - static_cast<float>(m_posX), z - static_cast<float>(m_posZ),
                           gridHeight, flags);
}

// Writes only points within 0.01 of a grid corner. Rounds height with +0.5, clamps to
// the bit-depth range, and narrows to u16; NaN selects the low bound. Other positions do nothing.
void HeightmapEditable::setHeightInLocalCoords(float x, float z, float height, int flags)
{
    const float gx = x * m_invScale.x;
    const float gz = z * m_invScale.z;
    const int ix = static_cast<int>(floorf(gx));
    const int iz = static_cast<int>(floorf(gz));
    const float fx = fabsf(gx - static_cast<float>(ix));
    const float fz = fabsf(gz - static_cast<float>(iz));
    const float ceiling = static_cast<float>((1 << getBitResolution()) - 1);
    const float value = std::max(0.0f, std::min(height + 0.5f, ceiling));
    const unsigned short sample = static_cast<unsigned short>(static_cast<int>(value));

    if (fx < 0.01f && fz < 0.01f)
        setHeight(ix, iz, sample, flags);
    else if (fx > 0.99f && fz < 0.01f)
        setHeight(ix + 1, iz, sample, flags);
    else if (fx < 0.01f && fz > 0.99f)
        setHeight(ix, iz + 1, sample, flags);
    else if (fx > 0.99f && fz > 0.99f)
        setHeight(ix + 1, iz + 1, sample, flags);
}

} // namespace world
} // namespace hfe
} // namespace dice
