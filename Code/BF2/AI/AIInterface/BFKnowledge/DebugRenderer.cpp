// [INFERRED] Source placement.
// [FRONTIER] Remaining methods stop by name until implemented.
// [TEMP] Temporary implementation boundaries remain.
// [CHOSEN, not recovered] Temporary implementation boundaries remain.
// [RETIRE-WHEN: aidll-01-adapters] Complete and validate the remaining DebugRenderer operations.

#include "DebugRenderer.h"

#include "AIFrontier.h"

#include "../../AIDLL/AI/Main/CommonSingletonServer.h"
#include "../SingletonServer.h"     // (!) its roster static is BFSingletonServer's, not Common's
#include "../../../Core/ClassManager.h"

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// First wins in, exact match out.
IDebugRenderer::IDebugRenderer()
{
    if (!BFSingletonServer::m_iDebugRenderer)
        classManager->registerSimpleSingleton(std::string("IDebugRenderer"), this);
}

IDebugRenderer::~IDebugRenderer()
{
    if (BFSingletonServer::m_iDebugRenderer == this)
        classManager->unregisterSimpleSingleton(std::string("IDebugRenderer"));
}

DebugRenderer::DebugRenderer()
{
}

DebugRenderer::~DebugRenderer()
{
}

// 5 bytes - a bare ret. Empty in the dedicated image.
void DebugRenderer::drawAll()
{
    BF2_AI_FRONTIER("dice::hfe::ai::DebugRenderer::drawAll");
}

// - likewise empty.
void DebugRenderer::reset()
{
    BF2_AI_FRONTIER("dice::hfe::ai::DebugRenderer::reset");
}

int DebugRenderer::drawLine(const std::vector<Vec3>& points, unsigned int colour, float width) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::DebugRenderer::drawLine");
}

int DebugRenderer::drawCircle(const Vec3& centre, const Vec3& normal, float radius, unsigned int colour, bool filled, int segments) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::DebugRenderer::drawCircle");
}

int DebugRenderer::drawScreenText(const Vec2& position, const std::vector<std::string>& lines, int size, unsigned int colour, AITextAlignmentX ax, AITextAlignmentY ay) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::DebugRenderer::drawScreenText");
}

int DebugRenderer::draw3dText(const Vec3& position, const std::vector<std::string>& lines, int size, unsigned int colour, AITextAlignmentX ax, AITextAlignmentY ay) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::DebugRenderer::draw3dText");
}

int DebugRenderer::drawTriangle(const std::vector<Vec3>& vertices, const std::vector<unsigned int>& indices, bool filled) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::DebugRenderer::drawTriangle");
}

int DebugRenderer::drawMesh(Vec3* vertices, unsigned int* indices, int count, unsigned int* colours, bool filled) const
{
    BF2_AI_FRONTIER("dice::hfe::ai::DebugRenderer::drawMesh");
}

} // namespace ai
} // namespace hfe
} // namespace dice
