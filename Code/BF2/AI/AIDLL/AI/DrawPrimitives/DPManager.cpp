// [RETIRE-WHEN: aidll-02-skeleton] Complete and validate the remaining DPManager operations.

#include "DPManager.h"

#include <string>

#include "../../../../Core/ClassManager.h"
#include "../Main/CommonSingletonServer.h"
#include "../Main/AIDllFrontier.h"

namespace dice {
namespace hfe {

extern ClassManager* classManager;

namespace ai {

// IDPManager
IDPManager::IDPManager()
{
    // the guard reads the roster static, not the registry. Reproduced.
    if (!CommonSingletonServer::m_iDPManager)
        classManager->registerSimpleSingleton("IDPManager", this);
}

IDPManager::~IDPManager()
{
    if (CommonSingletonServer::m_iDPManager == this)
        classManager->unregisterSimpleSingleton("IDPManager");
}

// IDPStaticCreator
IDPStaticCreator::IDPStaticCreator()
{
    // the guard reads the roster static, not the registry. Reproduced.
    if (!AISingletonServer::m_iDPDynamicCreator)
        classManager->registerSimpleSingleton("IDPStaticCreator", this);
}

IDPStaticCreator::~IDPStaticCreator()
{
    if (AISingletonServer::m_iDPDynamicCreator == this)
        classManager->unregisterSimpleSingleton("IDPStaticCreator");
}

// IDPDynamicCreator
IDPDynamicCreator::IDPDynamicCreator()
{
    // the guard reads the roster static, not the registry. Reproduced.
    if (!AISingletonServer::m_iDPDynamicCreator)
        classManager->registerSimpleSingleton("IDPDynamicCreator", this);
}

IDPDynamicCreator::~IDPDynamicCreator()
{
    if (AISingletonServer::m_iDPDynamicCreator == this)
        classManager->unregisterSimpleSingleton("IDPDynamicCreator");
}

// IDPAIObjectCreator
IDPAIObjectCreator::IDPAIObjectCreator()
{
    // the guard reads the roster static, not the registry. Reproduced.
    if (!AISingletonServer::m_iDPDynamicCreator)
        classManager->registerSimpleSingleton("IDPAIObjectCreator", this);
}

IDPAIObjectCreator::~IDPAIObjectCreator()
{
    if (AISingletonServer::m_iDPDynamicCreator == this)
        classManager->unregisterSimpleSingleton("IDPAIObjectCreator");
}

// Initializes primitive collections and their four interface bases.
DPManager::DPManager()
    : m_textGamma(1.0f)
{
}

// - seven tree teardowns, which is what ~DPManager emits on its own.
DPManager::~DPManager()
{
}

int DPManager::deleteCircle(DrawPrimitive *&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::deleteCircle");
}

int DPManager::getCircleAttributes(DrawPrimitive *, float &, unsigned int &, Vec3 &, float &, Vec3 &, bool &, int &)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::getCircleAttributes");
}

int DPManager::setCircleAttributes(DrawPrimitive *, float, unsigned int, const Vec3&, float, const Vec3&, bool, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::setCircleAttributes");
}

int DPManager::deleteLine(DrawPrimitive *&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::deleteLine");
}

int DPManager::getLineAttributes(DrawPrimitive *, float &, unsigned int &, std::vector<Vec3> &, float &)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::getLineAttributes");
}

int DPManager::setLineAttributes(DrawPrimitive *, float, unsigned int, const std::vector<Vec3>&, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::setLineAttributes");
}

int DPManager::delete2dText(DrawPrimitive *&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::delete2dText");
}

int DPManager::get2dTextAttributes(DrawPrimitive *, float &, unsigned int &, Vec2 &, std::vector<std::string> &, int &, AITextAlignmentX &, AITextAlignmentY &)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::get2dTextAttributes");
}

int DPManager::set2dTextAttributes(DrawPrimitive *, float, unsigned int, const Vec2&, const std::vector<std::string>&, int, AITextAlignmentX, AITextAlignmentY)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::set2dTextAttributes");
}

int DPManager::delete3dText(DrawPrimitive *&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::delete3dText");
}

int DPManager::get3dTextAttributes(DrawPrimitive *, float &, unsigned int &, Vec3 &, std::vector<std::string> &, int &, AITextAlignmentX &, AITextAlignmentY &)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::get3dTextAttributes");
}

int DPManager::set3dTextAttributes(DrawPrimitive *, float, unsigned int, const Vec3&, const std::vector<std::string>&, int, AITextAlignmentX, AITextAlignmentY)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::set3dTextAttributes");
}

int DPManager::deleteTriangle(DrawPrimitive *&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::deleteTriangle");
}

int DPManager::getTriangleAttributes(DrawPrimitive *, float &, std::vector<Vec3> &, std::vector<unsigned int>&, bool &)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::getTriangleAttributes");
}

int DPManager::setTriangleAttributes(DrawPrimitive *, float &, const std::vector<Vec3>&, const std::vector<unsigned int>&, bool &)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::setTriangleAttributes");
}

int DPManager::deletePolygon(DrawPrimitive *&)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::deletePolygon");
}

int DPManager::getPolygonAttributes(DrawPrimitive *, float &, bool &)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::getPolygonAttributes");
}

int DPManager::setPolygonAttributes(DrawPrimitive *, float &, bool &)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::setPolygonAttributes");
}

// Deletes a known mesh and clears the caller's handle. An unknown handle returns false.
// [FRONTIER] Known mesh deletion requires DPStaticMesh destruction.
bool DPManager::deleteMesh(DrawPrimitive *& primitive)
{
    const std::map<DrawPrimitive*, DPStaticMesh*>::iterator found = m_meshes.find(primitive);
    if (found == m_meshes.end())
        return false;
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::deleteMesh [DPStaticMesh's destructor]");
}

void DPManager::setTextGamma(float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::setTextGamma");
}

float DPManager::getTextGamma()
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::getTextGamma");
}

void DPManager::drawAll()
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::drawAll");
}

void DPManager::nodrawUpdate()
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::nodrawUpdate");
}

int DPManager::deleteAll()
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::deleteAll");
}

DPStaticCircle * DPManager::createStaticCircle(float, unsigned int, const Vec3&, float, const Vec3&, bool, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createStaticCircle");
}

DPStaticLine * DPManager::createStaticLine(float, unsigned int, const Vec3&, const Vec3&, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createStaticLine");
}

int DPManager::createStaticLine(float, unsigned int, const std::vector<Vec3>&, float)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createStaticLine");
}

DrawPrimitive * DPManager::createStatic2dText(float, unsigned int, const Vec2&, const std::string&, int, AITextAlignmentX, AITextAlignmentY)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createStatic2dText");
}

DrawPrimitive * DPManager::createStatic2dText(float, unsigned int, const Vec2&, const std::vector<std::string>&, int, AITextAlignmentX, AITextAlignmentY)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createStatic2dText");
}

int DPManager::createStatic3dText(float, unsigned int, const Vec3&, const std::string&, int, AITextAlignmentX, AITextAlignmentY)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createStatic3dText");
}

int DPManager::createStatic3dText(float, unsigned int, const Vec3&, const std::vector<std::string>&, int, AITextAlignmentX, AITextAlignmentY)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createStatic3dText");
}

int DPManager::createStaticTriangle(float, const std::vector<Vec3>&, const std::vector<unsigned int>&, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createStaticTriangle");
}

int DPManager::createStaticPolygon(float, const std::vector<Vec3>&, const std::vector<unsigned int>&, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createStaticPolygon");
}

DPStaticMesh * DPManager::createStaticMesh(float, unsigned int *, Vec3 *, unsigned int *, int, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createStaticMesh");
}

int DPManager::createStaticMesh(float, unsigned int *, Polygon *, Vec3 *, unsigned int *, int, bool)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createStaticMesh");
}

int DPManager::testStatics(Mat4 &)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::testStatics");
}

DPStaticCircle * DPManager::createDPDynamicCircle(float, unsigned int, const Vec3&, float, Bot *, DPDynamicTypes, const Vec3&, bool, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createDPDynamicCircle");
}

DPStaticCircle * DPManager::createDPDynamicCircle(float, unsigned int, const Vec3&, float, unsigned int, DPDynamicTypes, const Vec3&, bool, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createDPDynamicCircle");
}

DPStaticLine * DPManager::createDynamicLine(float, unsigned int, const Vec3&, const Vec3&, float, Bot *, DPDynamicTypes)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createDynamicLine");
}

DPStaticLine * DPManager::createDynamicLine(float, unsigned int, const Vec3&, const Vec3&, float, unsigned int, DPDynamicTypes)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createDynamicLine");
}

int DPManager::createDynamicLine(float, unsigned int, const std::vector<Vec3>&, float, Bot *, DPDynamicTypes)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createDynamicLine");
}

int DPManager::createDynamicLine(float, unsigned int, const std::vector<Vec3>&, float, unsigned int, DPDynamicTypes)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createDynamicLine");
}

int DPManager::createDynamic3dText(float, unsigned int, const Vec3&, const std::string&, Bot *, DPDynamicTypes, int, AITextAlignmentX, AITextAlignmentY)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createDynamic3dText");
}

int DPManager::createDynamic3dText(float, unsigned int, const Vec3&, const std::vector<std::string>&, Bot *, DPDynamicTypes, int, AITextAlignmentX, AITextAlignmentY)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createDynamic3dText");
}

int DPManager::createDynamic3dText(float, unsigned int, const Vec3&, const std::string&, unsigned int, DPDynamicTypes, int, AITextAlignmentX, AITextAlignmentY)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createDynamic3dText");
}

int DPManager::createDynamic3dText(float, unsigned int, const Vec3&, const std::vector<std::string>&, unsigned int, DPDynamicTypes, int, AITextAlignmentX, AITextAlignmentY)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createDynamic3dText");
}

DPStaticCircle * DPManager::createDPAIObjectCircle(float, unsigned int, const Vec3&, float, unsigned int, DPDynamicTypes, const Vec3&, bool, int)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createDPAIObjectCircle");
}

DPStaticLine * DPManager::createAIObjectLine(float, unsigned int, const Vec3&, const Vec3&, float, unsigned int, DPDynamicTypes)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createAIObjectLine");
}

int DPManager::createAIObjectLine(float, unsigned int, const std::vector<Vec3>&, float, unsigned int, DPDynamicTypes)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createAIObjectLine");
}

int DPManager::createAIObject3dText(float, unsigned int, const Vec3&, const std::string&, unsigned int, DPDynamicTypes, int, AITextAlignmentX, AITextAlignmentY)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createAIObject3dText");
}

int DPManager::createAIObject3dText(float, unsigned int, const Vec3&, const std::vector<std::string>&, unsigned int, DPDynamicTypes, int, AITextAlignmentX, AITextAlignmentY)
{
    BF2_AI_FRONTIER("dice::hfe::ai::DPManager::createAIObject3dText");
}

} // namespace ai
} // namespace hfe
} // namespace dice
