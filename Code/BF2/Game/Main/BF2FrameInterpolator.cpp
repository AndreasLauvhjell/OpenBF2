// Client transform snapshots and temporary render interpolation.

#include "BF2FrameInterpolator.h"

#if !DEDICATED

#include <cmath>                            // std::fabs - the 4.0f teleport guard
#include <vector>

#include "BF2Console.h"
#include "BF2Engine.h"
#include "BF2ClientSeams.h"                 // hudManager (ds:0xA10890)

#include "../../Core/ClassManager.h"
#include "../../Core/Debug.h"               // the two DebugTypeError sites in update
#include "../../Core/IBase.h"
#include "../../Core/Ids.h"
#include "../../IO/Input/Input.h"           // io::IInputDeviceManager / IAdvancedInputDevice
#include "../../Math/Mat4.h"
#include "../../Math/Quat.h"                // fromMat / slerp / toMat - the interpolation itself
#include "../../Menu/BF2HudManager.h"       // hasActiveMenu (+0x30)
#include "../../RendMisc/IRenderView.h"     // dx9rend::IRenderView - the frustum the visible set comes from
#include "../../RendMisc/IRenderer.h"
#include "../../Scene/Object/ObjectManager.h"
#include "../../Scene/Object/ObjectPredicator.h"
#include "../../Scene/Object/PlayerManager.h"
#include "../IGameLogic.h"
#include "../Objects/ICameraObject.h"
#include "../Objects/IObject.h"
#include "../Objects/IObjectTemplate.h"
#include "../Objects/IPlayer.h"
#include "../Objects/ISoldier.h"

namespace dice {
namespace hfe {

extern ClassManager* classManager;          // Core/Globals.cpp; PE ds:0x9FF960
extern BF2Engine*    bf2Engine;             // Core/Globals.cpp; PE ds:0x986F48

namespace io { extern IBase* inputDeviceManager; }   // Core/Globals.cpp; PE ds:0x9FFCB0

namespace {

// : the shared float clamp, upper bound tested first, then the lower bound. Same body as the
// copies in Game/GameServerSettings.cpp and Game/Objects/Game/Engine.cpp; the retail image has one
// out-of-line helper and every caller reaches it.
inline float clampFloat(float value, float low, float high)
{
    float result = value;
    if (value >= high) result = high;
    if (result < low) return low;
    return result;
}

// : three floats, `out = a; out += (b - a) * t`, component by component. Writes the
// translation row of the interpolated matrix; `out` is `&Mat4::m[12]` at the one call site.
void lerpTranslation(float* out, const float* a, const float* b, float t)
{
    out[0] = a[0];
    out[1] = a[1];
    out[2] = a[2];
    out[0] = (b[0] - out[0]) * t + out[0];
    out[1] = (b[1] - out[1]) * t + out[1];
    out[2] = (b[2] - out[2]) * t + out[2];
}

// The attested __FILE__ the two Debug() sites carry (BF2.exe 0x89D770).
const char kSourcePath[] = "C:\\dice\\Projects\\BF2Branches\\Patch_1_50\\Code\\BF2\\Game\\Main\\BF2FrameInterpolator.cpp";

} // namespace

// BF2.exe 0x45BF10, 32 B. The shipped constructor writes a byte at +0, zeroes +4/+8, then
// default-constructs both vectors (their three pointer words are zero; each VC7.1 allocator word is
// left to its empty allocator). Member initialisation preserves those source-level operations while
// allowing the native compiler's STL representation to differ from retail VC7.1.
BF2FrameInterpolator::BF2FrameInterpolator()
    : m_enabled(true)
    , m_ticks(0)
    , m_fraction(0.0f)
    , m_objects()
    , m_entries()
{
}

// Interpolates the visible scene between simulation ticks, then the local vehicle and camera.
// Requires initialized rendering, object/player managers, game logic and input services.
// Only the optional player, vehicle, camera parent, Falcon device, console and HUD are null-checked.
void BF2FrameInterpolator::interpolate(float exactTime, float prevTickTime)
{
    if (!m_enabled)
        return;

    // : a std::string temporary built on the stack and destroyed immediately after the call.
    dx9rend::IRenderer* renderer =
        static_cast<dx9rend::IRenderer*>(classManager->getSingleton("Renderer"));

    // : the fraction is zero unless a positive tick count says otherwise, then clamped
    // slightly outside [0,1] so a frame that runs a little early or late still extrapolates.
    float fraction = 0.0f;
    if (m_ticks > 0)
        fraction = (exactTime - prevTickTime) / (m_ticks * 0.033333335f);   // ds:0x89D7C8, the 30 Hz tick
    fraction = clampFloat(fraction, -0.1f, 1.1f);
    m_fraction = fraction;                              // `fst`/`fstp` pair: local and member both get it

    // : the object the local player is riding, resolved to its root so the vehicle arm
    // below can exclude it from the generic loop and drive it separately.
    world::IObject* localVehicleRoot = 0;
    world::IPlayer* localPlayer = world::playerManager->getLocalHumanPlayer();
    if (localPlayer)
    {
        world::IObject* vehicle = localPlayer->getVehicle();
        if (vehicle)
            localVehicleRoot = world::getRootParent(vehicle);
    }

    // : the visible set is rebuilt from scratch every frame - visible root objects that
    // carry mobile physics, i.e. exactly the things whose transform moves between ticks. The mask
    //  is one of the thirteen the engine's predicators use.
    m_objects.resize(0);
    world::ObjectFlagPredicator predicator(world::IObject::OFIsVisible |
                                           world::IObject::OFHasMobilePhysics |
                                           world::IObject::OFIsRootParent);

    dx9rend::IRenderView* view = renderer->getRenderView();

    world::objectManager->getObjectsFromGrid(*view->getFrustumPtr(), m_objects, &predicator,
                                             true, 0.0f, 0.0f);

    // : everything the query returned except the local player's own vehicle.
    for (std::vector<world::IObject*>::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        if (*it != localVehicleRoot)
            interpolateObject(*it, fraction);
    }

    if (!localVehicleRoot)
        return;

    // : a free camera is not a vehicle - interpolate the camera's own object and stop.
    if (localVehicleRoot->getTemplate()->getClassID() == world::CID_FreeCameraTemplate)
    {
        world::ICameraObject* camera = localPlayer->getCamera();
        interpolateObject(static_cast<world::IObject*>(camera->queryInterface(world::IID_IObject)),
                          fraction);
        return;
    }

    // : otherwise the camera hangs off the soldier that owns it. Note that retail asks for
    // the Falcon device and its activity before testing the parent, even though `!falconActive` is
    // the last of the three conditions - the haptic device drives the view itself when it is live.
    world::ICameraObject* camera = localPlayer->getCamera();
    world::IObject* cameraObject = static_cast<world::IObject*>(camera->queryInterface(world::IID_IObject));
    world::IObject* cameraParent = cameraObject->getParent();

    io::IInputDevice* falcon =
        static_cast<io::IInputDeviceManager*>(io::inputDeviceManager)->getInputDevice(io::IDFFalcon);
    const bool falconActive = falcon ? falcon->isActive() : false;

    if (cameraParent
        && cameraParent->getTemplate()->getClassID() == world::CID_SoldierTemplate
        && !falconActive)
    {
        world::ISoldier* soldier =
            static_cast<world::ISoldier*>(cameraParent->queryInterface(world::IID_ISoldier));

        // : the soldier's transform is snapshotted before the vehicle is interpolated and
        // written back afterwards, so interpolating the vehicle cannot drag the view with it.
        const bool relative = (cameraParent->getParent() != 0);
        Mat4 transform(relative ? cameraParent->getRelativeTransformation()
                                : cameraParent->getAbsoluteTransformation());

        interpolateObject(localVehicleRoot, fraction);

        if (relative)
        {
            cameraParent->setRelativeTransformation(transform);
        }
        else
        {
            // : only the translation is taken from where the soldier ended up; the
            // orientation stays as it was snapshotted. (The PE loads m[14] and m[13] onto the x87
            // stack, moves m[12] as an integer, then stores all three - order is immaterial here.)
            const Mat4& current = cameraParent->getAbsoluteTransformation();
            transform.m[12] = current.m[12];
            transform.m[13] = current.m[13];
            transform.m[14] = current.m[14];
            cameraParent->setAbsoluteTransformation(transform);
        }

        // : the mouse delta this frame, applied to the soldier's camera. Retail does NOT
        // test the mouse pointer before dispatching on it.
        io::IAdvancedInputDevice* mouse = static_cast<io::IAdvancedInputDevice*>(
            static_cast<io::IInputDeviceManager*>(io::inputDeviceManager)->getInputDevice(io::IDFMouse));

        float deltaX = 0.0f;
        float deltaY = 0.0f;
        if (gameLogic->isServer() || gameLogic->getGameStatus() != 4)
            mouse->peakXYAxis(deltaX, deltaY);

        // : and it is thrown away again whenever something on screen owns the pointer.
        if (bf2Engine->getMenuActive()
            || (bf2Engine->bf2Console && bf2Engine->bf2Console->getConsoleActive())
            || (hudManager && hudManager->hasActiveMenu()))
        {
            deltaX = 0.0f;
            deltaY = 0.0f;
        }

        soldier->increaseSoldierCameraTransformation(deltaX, deltaY);
        return;
    }

    // : no soldier to keep still - interpolate the vehicle root itself. Retail re-resolves
    // the root even though localVehicleRoot already is one; getRootParent answers from the cache.
    interpolateObject(world::getRootParent(localVehicleRoot), fraction);
}

// Restores each object's simulation transform after rendering, then empties the
// snapshot list with resize(0), retaining capacity.
void BF2FrameInterpolator::postRender()
{
    for (std::vector<Entry>::iterator it = m_entries.begin(); it != m_entries.end(); ++it)
    {
        // The object pointer is dereferenced unguarded, exactly as retail does.
        if (it->object->getParent())
            it->object->setRelativeTransformation(it->transform);
        else
            it->object->setAbsoluteTransformation(it->transform);
    }
    m_entries.resize(0);
}

// Snapshots eligible visible objects before simulation for later frame interpolation.
// Requires initialized world/render services and a valid render view; missing view
// is fatal. Keep repeated vehicle lookups because they may clear expired references.
void BF2FrameInterpolator::update(int ticks)
{
    if (!m_enabled)
        return;

    m_ticks = ticks;
    m_objects.resize(0);

    world::IObject* localVehicleRoot = 0;
    world::IPlayer* localPlayer = world::playerManager->getLocalHumanPlayer();   // +0x30
    if (localPlayer && localPlayer->getVehicle())                       // +0x44, asked twice
        localVehicleRoot = world::getRootParent(localPlayer->getVehicle());

    world::ObjectFlagPredicator predicator(world::IObject::OFIsVisible |
                                           world::IObject::OFHasMobilePhysics |
                                           world::IObject::OFIsRootParent);

    dx9rend::IRenderer* renderer =
        static_cast<dx9rend::IRenderer*>(classManager->getSingleton("Renderer"));
    dx9rend::IRenderView* view = renderer->getRenderView();
    if (!view)
        Debug(DebugTypeError, kSourcePath, 83, "") << "no renderview";

    world::objectManager->getObjectsFromGrid(*view->getFrustumPtr(), m_objects, &predicator,
                                             true, 0.0f, 0.0f);         // +0x74, then +0x40

    for (std::vector<world::IObject*>::iterator it = m_objects.begin(); it != m_objects.end(); ++it)
    {
        if (*it != localVehicleRoot)
            snapshotObject(*it);
    }

    if (localPlayer)
    {
        if (localPlayer->getVehicle()
            && localPlayer->getVehicle()->getTemplate()->getClassID() == world::CID_FreeCameraTemplate)
        {
            world::ICameraObject* camera = localPlayer->getCamera();    // +0x17C
            world::IObject* cameraObject =
                static_cast<world::IObject*>(camera->queryInterface(world::IID_IObject));
            if (!cameraObject)
                Debug(DebugTypeError, kSourcePath, 109, "") << "no comp for camera";
            snapshotObject(cameraObject);
        }
        else if (localPlayer->getVehicle())                             // asked again
        {
            snapshotObject(world::getRootParent(localPlayer->getVehicle()));
        }
    }
}

// Snapshots sibling and child transforms for interpolation, allocating storage on
// first use. Roots use absolute transforms; only parented rotational bundles use
// relative transforms. A terminal non-camera leaf without geometry ends the walk.
void BF2FrameInterpolator::snapshotObject(world::IObject* object)
{
    for (world::IObject* current = object; current != 0; current = current->getSibling())
    {
        if (!current->getChild() && !current->getSibling() && !current->m_geometry)   // +0xEC, +0x2C, +0x38
        {
            const unsigned int classId = current->getTemplate()->getClassID();
            if (classId != world::CID_CameraTemplate && classId != world::CID_FreeCameraTemplate)
                return;                                                 // the whole walk
        }

        if (current->getParent())                                       // +0x28
        {
            if (const_cast<world::IObjectTemplate*>(current->getTemplate())
                    ->queryImplementation(world::CID_RotationalBundleTemplate))
            {
                if (!current->m_interpolationTransformation)
                    current->m_interpolationTransformation = new Mat4(Mat4::getIdentity());
                *current->m_interpolationTransformation = current->getRelativeTransformation();   // +0xE4
            }
        }
        else
        {
            if (!current->m_interpolationTransformation)
                current->m_interpolationTransformation = new Mat4(Mat4::getIdentity());
            *current->m_interpolationTransformation = current->getAbsoluteTransformation();       // +0x80
        }

        snapshotObject(current->getChild());
    }
}

// Temporarily interpolates eligible objects between their snapshot and current
// transforms; postRender restores the originals. Skips vertical changes of 4 units
// or more. Parented soldiers and rotational bundles use relative transforms.
void BF2FrameInterpolator::interpolateObject(world::IObject* object, float fraction)
{
    for (world::IObject* current = object; current != 0; current = current->getSibling())
    {
        const unsigned int classId = current->getTemplate()->getClassID();   // unconditional here
        if (!current->getChild() && !current->getSibling() && !current->m_geometry
            && classId != world::CID_CameraTemplate && classId != world::CID_FreeCameraTemplate)
            return;                                                     // the whole walk

        const Mat4* currentTransform;
        bool eligible = true;
        if (current->getParent())
        {
            currentTransform = &current->getRelativeTransformation();   // +0xE4, fetched first
            eligible = classId == world::CID_SoldierTemplate
                    || const_cast<world::IObjectTemplate*>(current->getTemplate())
                           ->queryImplementation(world::CID_RotationalBundleTemplate) != 0;
        }
        else
        {
            currentTransform = &current->getAbsoluteTransformation();   // +0x80
        }

        if (eligible)
        {
            const Mat4* previous = current->m_interpolationTransformation;   // +0x08
            if (previous && std::fabs(currentTransform->m[13] - previous->m[13]) < 4.0f)   // ds:0x89D7C4
            {
                Entry entry;                                            //  twice, then 0x45BF30
                entry.object = current;
                entry.transform = *currentTransform;
                m_entries.push_back(entry);

                Mat4 out = Mat4::getIdentity();
                Quat from = { 0.0f, 0.0f, 0.0f, 1.0f };
                from.fromMat(*previous);                                //  on the snapshot
                Quat to = { 0.0f, 0.0f, 0.0f, 1.0f };
                to.fromMat(*currentTransform);                          // : zero, then 0x6D9050
                Quat blended = { 0.0f, 0.0f, 0.0f, 1.0f };
                blended.slerp(from, to, fraction);
                blended.toMat(out);
                lerpTranslation(&out.m[12], &previous->m[12], &currentTransform->m[12], fraction);

                if (current->getParent())
                    current->setRelativeTransformation(out);            // +0xE8
                else
                    current->setAbsoluteTransformation(out);            // +0x88
            }
        }

        interpolateObject(current->getChild(), fraction);
    }
}

} // namespace hfe
} // namespace dice

#endif // !dedicated
