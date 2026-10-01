// Path:     [INFERRED] beside the recovered ZoomComp/DefaultZoomComp family.
// Contents: ToggleCameraComp and its template recovered from ELF 0x829D780..0x829EA8A and
// cross-checked against the Win32 dedicated-server PE.

#include "ToggleCameraComp.h"

#include <cstddef>
#include <map>
#include <new>
#include <sstream>

#include "../GenericFireArm.h"
#include "../../ICameraObject.h"
#include "../../IObject.h"
#include "../../IObjectComponent.h"
#include "../../IWeaponHudComp.h"
#include "../../WeaponHudCompTemplate.h"
#include "../../ObjectLayoutAccess.h"
#include "../../IPlayer.h"
#include "../../Projectiles/GenericProjectile.h"
#include "../../../../Core/EventManager.h"
#include "../../../../Core/Ids.h"
#include "../../../../IO/Stream/IStream.h"
#include "../../../../Scene/Object/ObjectTypes/Game/IPlayerControlObject.h"

namespace dice {
namespace hfe {
namespace world {

namespace {

IPlayerControlObject* findPco(IObject* object)
{
    for (; object; object = object->getParent())
    {
        IPlayerControlObject* const result = static_cast<IPlayerControlObject*>(
            object->queryInterface(IID_IPlayerControlObject));
        if (result)
            return result;
    }
    return 0;
}

WeakPtr<IProjectileObject> getProjectileWeakPtr(IProjectileObject* projectile)
{
    if (!projectile->m_weakControl)
    {
        typedef WeakPtr<IProjectileObject>::Ctrl Ctrl;
        Ctrl* const control = static_cast<Ctrl*>(::operator new(sizeof(Ctrl)));
        control->refCount = 1;
        control->target = projectile;
        projectile->m_weakControl = control;
    }

    WeakPtr<IProjectileObject> result;
    result.m_ctrl = projectile->m_weakControl;
    ++result.m_ctrl->refCount;
    return result;
}

GenericProjectile* projectileObject(IProjectileObject* projectile)
{
    return projectile ? static_cast<GenericProjectile*>(projectile) : 0;
}

} // namespace

ToggleCameraComp::ToggleCameraComp(ToggleCameraCompTemplate* objectTemplate)
    : ZoomComp(objectTemplate)
    , m_leafRefCount(1)
    , m_prevCameraId(0)
    , m_projectile()
    , m_toggleTemplate(objectTemplate)
{
}

ToggleCameraComp::~ToggleCameraComp()
{
}

void ToggleCameraComp::addRef() { ++m_leafRefCount; }
int ToggleCameraComp::getRef() { return m_leafRefCount; }

int ToggleCameraComp::release()
{
    const int remaining = --m_leafRefCount;
    if (remaining == 0 && this)
        delete this;
    return remaining;
}

void* ToggleCameraComp::queryInterface(unsigned int iid) const
{
    ToggleCameraComp* const self = const_cast<ToggleCameraComp*>(this);
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return self;
    if (iid == IID_IWeaponComp || iid == IID_IWeaponZoomComp)
        return static_cast<IWeaponZoomComp*>(self);
    return 0;
}

unsigned int ToggleCameraComp::getInterfaceID() const
{
    return IID_IWeaponZoomComp;
}

void ToggleCameraComp::enable()
{
    if (m_toggleTemplate->getStartCameraId() == -1)
        return;

    IPlayerControlObject* const pco = findPco(firearm());
    if (!pco)
        return;

    pco->setActiveCamera(m_toggleTemplate->getStartCameraId());
    if (m_toggleTemplate->getStartCameraViewMode() != -1)
    {
        pco->getActiveCamera()->setViewMode(
            static_cast<CameraViewMode>(m_toggleTemplate->getStartCameraViewMode()), 0, false);
    }
    if (is1P())
        eventManager->postEvent(ECHud, 31, this, 0.0f);
}

void ToggleCameraComp::toggle(TemplateMessage message)
{
    if (message != static_cast<TemplateMessage>(7))
        return;

    GenericProjectile* const projectile = projectileObject(m_projectile.get());
    if (projectile && projectile->isInProjectileCamera())
    {
        IPlayerControlObject* const pco = findPco(firearm());
        pco->setActiveCamera(m_prevCameraId);
        m_prevCameraId = 0;
    }
    else if (m_toggleTemplate->getChangeCameraId() != -1)
    {
        IPlayerControlObject* const pco = findPco(firearm());
        if (pco)
        {
            if (m_toggleTemplate->getChangeCameraId()
                != pco->getActiveCamera()->getCameraId())
            {
                m_prevCameraId = pco->getActiveCamera()->getCameraId();
                pco->setActiveCamera(m_toggleTemplate->getChangeCameraId());
                if (m_toggleTemplate->getChangeCameraViewMode() != 0)
                {
                    pco->getActiveCamera()->setViewMode(
                        static_cast<CameraViewMode>(
                            m_toggleTemplate->getChangeCameraViewMode()),
                        0, false);
                }
            }
            else
            {
                pco->setActiveCamera(m_prevCameraId);
                pco->getActiveCamera()->setViewMode(
                    static_cast<CameraViewMode>(1), 0, false);
                m_prevCameraId = 0;
            }
        }
    }

    IWeaponHudComp* const weaponHud = static_cast<IWeaponHudComp*>(
        object_detail::LayoutAccess::weaponHudComponent(firearm()));
    if (is1P() && weaponHud && weaponHud->getHudInfo()->altGuiIndex != -1)
        eventManager->postEvent(ECHud, 31, this, 0.0f);
}

void ToggleCameraComp::changeTarget(int targetId)
{
    const int cameraId = getChangeCameraId();
    if (cameraId == -1)
        return;

    IPlayerControlObject* const pco = findPco(firearm());
    std::map<int, ICameraObject*>* const cameras = pco->getCameras();
    std::map<int, ICameraObject*>::iterator camera = cameras->find(cameraId);
    if (camera != cameras->end())
    {
        camera->second->setTraceObjectId(static_cast<unsigned int>(targetId));
        camera->second->setViewMode(static_cast<CameraViewMode>(6), 0, false);
    }
}

void ToggleCameraComp::postFire(WeaponFireEvent& event)
{
    if (!event.projectile)
        return;

    IProjectileObject* const projectile = static_cast<IProjectileObject*>(
        event.projectile->queryInterface(IID_IProjectileObject));
    if (projectile)
        m_projectile = getProjectileWeakPtr(projectile);
}

bool ToggleCameraComp::requestFire(bool, bool)
{
    GenericProjectile* const projectile = projectileObject(m_projectile.get());
    if (projectile && projectile->isInProjectileCamera())
        return false;
    return fireAllowed();
}

void ToggleCameraComp::reset()
{
    m_projectile = WeakPtr<IProjectileObject>();
}

int ToggleCameraComp::getChangeCameraId() const
{
    GenericProjectile* const projectile = projectileObject(m_projectile.get());
    if (!projectile || !projectile->isInProjectileCamera())
        return m_toggleTemplate->getChangeCameraId();

    IPlayerControlObject* const pco = findPco(firearm());
    if (!pco)
        return m_toggleTemplate->getChangeCameraId();
    return pco->getActiveCamera()->getCameraId();
}

bool ToggleCameraComp::fireAllowed() const
{
    bool allowed = false;
    if (m_toggleTemplate->getChangeCameraId() == -1)
    {
        allowed = true;
    }
    else
    {
        IPlayerControlObject* const pco = findPco(firearm());
        if (pco)
        {
            IPlayer* const player = pco->getOccupingPlayer();
            if (m_toggleTemplate->getChangeCameraId()
                    == pco->getActiveCamera()->getCameraId()
                || (player && player->getIsAIPlayer()))
            {
                allowed = true;
            }
        }
    }
    return allowed || m_toggleTemplate->getAllowUntoggledFire();
}

bool ToggleCameraComp::allowTargetSeek() const
{
    return fireAllowed();
}

int ToggleCameraComp::getCameraNr() const
{
    return 0;
}

int ToggleCameraComp::getPrevCameraId() const
{
    return m_prevCameraId;
}

void ToggleCameraComp::changeViewMode()
{
}

ToggleCameraCompTemplate::ToggleCameraCompTemplate()
    : ZoomCompTemplate()
    , m_refCount(1)
    , m_changeCameraId(-1)
    , m_changeCameraViewMode(2)
    , m_startCameraId(-1)
    , m_startCameraViewMode(2)
    , m_useProjectileCamera(false)
    , m_allowUntoggledFire(true)
{
    // +0x26/+0x27 are deliberately untouched, matching both retail constructors.
}

void ToggleCameraCompTemplate::addRef() { ++m_refCount; }
int ToggleCameraCompTemplate::getRef() { return m_refCount; }

int ToggleCameraCompTemplate::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        delete this;
    return remaining;
}

void* ToggleCameraCompTemplate::queryInterface(unsigned int iid) const
{
    ToggleCameraCompTemplate* const self = const_cast<ToggleCameraCompTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent || iid == IID_IWeaponCompTemplate)
        return static_cast<IObjectTemplateComponent*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int ToggleCameraCompTemplate::getClassID() const
{
    return CID_ToggleCameraCompTemplate;
}

IObjectComponent* ToggleCameraCompTemplate::createComponent()
{
    return new ToggleCameraComp(this);
}

void* ToggleCameraCompTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_ToggleCameraCompTemplate || classId == CID_ZoomCompTemplate)
        return this;
    return 0;
}

void ToggleCameraCompTemplate::setChangeCameraId(int value)
{
    m_changeCameraId = value;
}

bool ToggleCameraCompTemplate::makeScript(io::IStream* stream)
{
    if (!ZoomCompTemplate::makeScript(stream))
        return false;

    std::ostringstream script;
    if (m_changeCameraId != -1)
    {
        script << "ObjectTemplate.zoom.changeCameraId " << m_changeCameraId << std::endl;
        if (m_changeCameraViewMode != 2)
        {
            script << "ObjectTemplate.zoom.changeCameraViewMode "
                   << m_changeCameraViewMode << std::endl;
        }
    }
    if (m_startCameraId != -1)
    {
        script << "ObjectTemplate.zoom.startCameraId " << m_startCameraId << std::endl;
        if (m_startCameraViewMode != 2)
        {
            script << "ObjectTemplate.zoom.startCameraViewMode "
                   << m_startCameraViewMode << std::endl;
        }
    }
    if (m_useProjectileCamera)
        script << "ObjectTemplate.zoom.useProjectileCamera " << m_useProjectileCamera << std::endl;
    if (!m_allowUntoggledFire)
        script << "ObjectTemplate.zoom.allowUntoggledFire " << m_allowUntoggledFire << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
