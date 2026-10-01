// [FRONTIER] Remaining methods stop by name until implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining AerialHeightMap operations.

#include "AerialHeightMap.h"

#include "../Main/AIDllFrontier.h"
#include "../Main/CommonSingletonServer.h"
#include "../../../AIInterface/BFKnowledge/BFGame.h"             // IAIGame::getLevelPath
#include "../../../AIInterface/BFKnowledge/BFTerrain.h"          // IAITerrain, the world bounds
#include "../../../../IO/File/File.h"

namespace dice {
namespace hfe {
namespace ai {

AerialHeightMap* AerialHeightMap::m_instance = 0;

namespace {

// The cache's name, spelled dice's way in both directions. "AerialHeighMap" is missing a 't' and
// that is not a transcription slip - the reader and the writer share the literal, so a rebuilt
// server that corrects the spelling stops finding the file a shipped level ships with.
const char* const kHeightMapSuffix = "/AIPathFinding/AerialHeighMap.ahm";

} // namespace

// Starts with cell size 16, zero origin, and no height buffer.
// Construction unconditionally replaces the singleton.
AerialHeightMap::AerialHeightMap()
    : m_cellSize(16.0f)
    , m_heights(0)
{
    m_origin.x = 0.0f;
    m_origin.y = 0.0f;

    // m_width and m_height deliberately not initialised - the binary steps over both. See the
    // header; `m_width` is genuinely read before anything is guaranteed to have written it.
    m_instance = this;
}

// it nulls the static unconditionally, without testing `== this`, so destroying a displaced
// instance clears the pointer to the live one. Reproduced.
AerialHeightMap::~AerialHeightMap()
{
    delete[] m_heights;
    m_instance = 0;
}

// Samples height above ground. Grid lookup requires a position inside the configured bounds.
// An unloaded map returns the missing-height sentinel 9999.
float AerialHeightMap::getAltitude(const Vec3& position)
{
    return position.y - getGroundHeight(position);
}

float AerialHeightMap::getGroundHeight(const Vec3& position)
{
    return getGroundHeight(position.x, position.z);
}

float AerialHeightMap::getGroundHeight(float x, float z)
{
    if (!m_heights)
        return 9999.0f;

    const int column = static_cast<int>((x - m_origin.x) / m_cellSize);
    const int row    = static_cast<int>((z - m_origin.y) / m_cellSize);

    return static_cast<float>(m_heights[column + m_width * row]);
}

// Loads the level's cached height grid. Missing cache data leaves the map empty.
void AerialHeightMap::readHeightMapFromFile()
{
    const std::string levelPath =
        static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getLevelPath();

    io::File file;

    std::string path(levelPath);
    path.append(kHeightMapSuffix);

    if (!file.open(path, io::FILEMODE_READ, static_cast<io::FileAccess>(0), 0))
        return;

    file.read(m_cellSize);
    file.read(m_width);
    file.read(m_height);
    file.read(m_origin);

    if (!m_heights)
        m_heights = new unsigned short[m_width * m_height];

    file.read(m_heights, 2 * m_width * m_height);
    file.close();
}

// The exact mirror of the reader, field for field and in the same order. It shares the reader's
// silence on failure, and it writes the array unconditionally - a save on an object whose
// `m_heights` is still null passes a null pointer and a zero size to `File::write`.
void AerialHeightMap::saveHeightMapToFile()
{
    const std::string levelPath =
        static_cast<IAIGame*>(CommonSingletonServer::m_iaiGame)->getLevelPath();

    io::File file;

    std::string path(levelPath);
    path.append(kHeightMapSuffix);

    if (file.open(path, static_cast<io::FileMode>(0), io::FILEACCESS_1, 0))
    {
        file.write(m_cellSize);
        file.write(m_width);
        file.write(m_height);
        file.write(m_origin);
        file.write(m_heights, 2 * m_width * m_height);
        file.close();
    }
}

// Builds the aerial height grid from terrain bounds and saves it to the level cache.
void AerialHeightMap::createHeightMap(int samplesPerAxis)
{
    IAITerrain* const terrain = static_cast<IAITerrain*>(CommonSingletonServer::m_iaiTerrain);

    m_width  = static_cast<int>(static_cast<float>(terrain->getWorldSizeX()) / m_cellSize);
    m_height = static_cast<int>(static_cast<float>(terrain->getWorldSizeZ()) / m_cellSize);

    m_origin.x = static_cast<float>(terrain->getWorldMinX());
    m_origin.y = static_cast<float>(terrain->getWorldMinZ());

    if (m_heights)
        delete[] m_heights;

    m_heights = new unsigned short[m_width * m_height];

    for (int z = 0; z < m_height; ++z)
        for (int x = 0; x < m_width; ++x)
            m_heights[x + m_width * z] = computeHeightAt(x, z, samplesPerAxis);

    saveHeightMapToFile();
}

// [FRONTIER] Computes a cell height from a square sample grid once the required collision query is implemented.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
unsigned short AerialHeightMap::computeHeightAt(int, int, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::AerialHeightMap::computeHeightAt (AIObjectPredicateNoVehicles: IAIObject +0x70)");
}

} // namespace ai
} // namespace hfe
} // namespace dice
