// Path:     attested - this exact relative path is the Debug __FILE__ at line 69 in both ports.
// Contents: recovered from bf2.i64 (ELF 0x8193CD0 / 0x8194530) and
// bf2_w32ded.exe.i64 (PE 0x588080 / 0x588560).

#include "OrderMarker.h"

#include <cmath>
#include <string>

#include "../../../Core/Debug.h"
#include "../../../Core/SettingsRepostitory.h"
#include "../../../IO/Network/NetworkManager.h"
#include "../../../Physics/HeightmapCluster.h"
#include "../../../Scene/Object/ObjectManager.h"
#include "../../../Scene/Object/ObjectPredicator.h"
#include "../../GameLogic/SquadManager/Squad.h"
#include "../../IGame.h"
#include "../../SingletonServer.h"
#include "../IObject.h"
#include "../IObjectTemplate.h"
#include "../../../Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"

namespace dice {
namespace hfe {

namespace world {
extern IBase* heightmapCluster;                         // ds:0x8ABAF24, Core/Globals.cpp
}

namespace world {

// The source uses named virtuals throughout.  Their physical slots differ between the PE and ELF
// class layouts (most visibly the two HeightmapCluster queries and IObject's post-destructor
// methods); keep those platform details in the real interfaces rather than baking raw offsets into
// this body.
OrderMarker::OrderMarker(SquadOrder& order, int markerId)
    : m_object()
    , m_order(&order)
    , m_markerId(markerId)
{
    Vec3 touchPosition = { 0.0f, 0.0f, 0.0f };

    if (order.targetNetId != 0)
    {
        IObject* const target = io::networkManager->getObject(order.targetNetId);
        touchPosition.x = 0.0f;
        touchPosition.y = 0.0f;
        touchPosition.z = 0.0f;
        SingletonServer::m_iNametagManager->addTouchItem(&order, markerId, target);
        return;
    }

    const char* markerTemplateName = 0;
    switch (order.orderId)
    {
        case 4:
            markerTemplateName = "ordermarker_move";
            break;
        case 5:
        case 8:
        case 9:
            markerTemplateName = "ordermarker_attack";
            break;
        case 6:
        case 7:
            markerTemplateName = "ordermarker_defend";
            break;
        default:
            // Shipped quirk: the text says order *type*, but the value printed is the constructor's
            // marker id (zero for squad orders, one for commander orders), not order.orderId.
            Debug(DEBUGTYPE_6, "Game/Objects/Game/OrderMarker.cpp", 69)
                << "No valid orderMarker for ordertype " << markerId;
            return;
    }

    IObjectTemplate* const markerTemplate =
        objectTemplateManager->getTemplate(std::string(markerTemplateName));
    IObject* const markerObject = markerTemplate->createObject();

    // Both settings reads are present in retail even though neither resulting value is consumed.
    int worldSizeX = 0;
    settings->intGet(std::string("GLSWorldSizeX"), worldSizeX);
    int worldSizeZ = 0;
    settings->intGet(std::string("GLSWorldSizeZ"), worldSizeZ);

    Vec3 requestedPosition = order.points[0];
    requestedPosition.y += 1.0f;

    IHeightmapCluster* const cluster = static_cast<IHeightmapCluster*>(heightmapCluster);
    const float terrainHeight = cluster->getHeightInWorldCoords(requestedPosition);
    float surfaceHeight = cluster->getWaterLevelInWorldCoords(requestedPosition);
    if (surfaceHeight <= terrainHeight)
        surfaceHeight = terrainHeight;

    IObject* hitObject = 0;
    Vec3 hitPoint = { 0.0f, 0.0f, 0.0f };
    Vec3 hitNormal = { 0.0f, 0.0f, 0.0f };
    // Retail leaves this output word uninitialised before intersectLine; the callee supplies it on
    // a hit.  Do not turn it into an input-side default here.
    int hitPart;
    const Vec3 down = { 0.0f, -100.0f, 0.0f };
    ObjectFlagPredicatorEx predicator(0x1000100u, 8u);

    float markerSurface = surfaceHeight;
    if (objectManager->intersectLine(hitObject, hitPoint, hitNormal, hitPart,
                                     requestedPosition, down, &predicator,
                                     true, false, false, true, 0))
    {
        markerSurface = hitPoint.y;
        if (markerSurface <= surfaceHeight)
            markerSurface = surfaceHeight;
    }

    Vec3 markerPosition = requestedPosition;
    markerPosition.y = markerSurface + markerObject->getVisualBoundingRadius() * 0.5f;

    Mat4 transform = Mat4::getIdentity();
    setRotationZXY(transform, -90.0f, 0.0f, 0.0f);
    transform.m[12] = markerPosition.x;
    transform.m[13] = markerPosition.y;
    transform.m[14] = markerPosition.z;
    markerObject->setAbsoluteTransformation(transform);
    markerObject->init();
    m_object = markerObject->getWeakPtr();

    const float dx = requestedPosition.x - markerPosition.x;
    const float dy = requestedPosition.y - markerPosition.y;
    const float dz = requestedPosition.z - markerPosition.z;
    if (std::sqrt(dx * dx + dy * dy + dz * dz) >= 1.5f)
    {
        touchPosition = requestedPosition;
    }
    else
    {
        touchPosition = markerPosition;
        touchPosition.y += 1.5f;
    }

    SingletonServer::m_iNametagManager->addTouchItem(&order, markerId, touchPosition);
}

OrderMarker::~OrderMarker()
{
    if (m_object.get())
        game->destroyObject(m_object.get());

    SingletonServer::m_iNametagManager->removeItem(1, m_order, m_markerId);
}

} // namespace world
} // namespace hfe
} // namespace dice
