// Path:     [INFERRED] companion to the attested ResurrectCollisionComp.h.
// Contents: recovered from bf2.i64 (ELF 0x828A880..0x828B840) and cross-checked against
// bf2_w32ded.exe (0x5B57A0..0x5B5Bxx).

#include "ResurrectCollisionComp.h"

#include <sstream>
#include <string>

#include "../ProjectileCompTemplates.h"
#include "../../IObject.h"
#include "../../../IGameLogic.h"
#include "../../../../Core/Debug.h"
#include "../../../../Core/Ids.h"
#include "../../../../Core/System.h"
#include "../../../../IO/Stream/IStream.h"
#include "../../../../Math/Mat4.h"
#include "../../../../Physics/Material.h"
#include "../../../../Physics/MaterialManager.h"

namespace dice {
namespace hfe {
namespace world {

extern IBase* materialManager;

namespace {

inline MaterialManager* materials()
{
    return static_cast<MaterialManager*>(materialManager);
}

} // namespace

ResurrectCollisionComp::ResurrectCollisionComp(ResurrectCollisionCompTemplate* objectTemplate)
    : m_refCount(1)
    , m_template(objectTemplate)
    , m_dword1C(0)
{
    ProjectileComp::m_refCount = 1;
    m_object = 0;
    m_initObject = 0;
}

ResurrectCollisionComp::~ResurrectCollisionComp() {}

void ResurrectCollisionComp::addRef() { ++m_refCount; }
int ResurrectCollisionComp::getRef() { return m_refCount; }

int ResurrectCollisionComp::release()
{
    const int remaining = m_refCount - 1;
    const bool last = m_refCount == 1;
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

void* ResurrectCollisionComp::queryInterface(unsigned int iid) const
{
    ResurrectCollisionComp* const self = const_cast<ResurrectCollisionComp*>(this);
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return static_cast<IObjectComponent*>(self);
    if (iid == IID_IProjectileComp || iid == IID_IProjectileCollisionComp)
        return static_cast<IProjectileCollisionComp*>(self);
    return 0;
}

unsigned int ResurrectCollisionComp::getInterfaceID() const
{
    return IID_IProjectileCollisionComp;
}

bool ResurrectCollisionComp::hasUpdate() const { return false; }
void ResurrectCollisionComp::updateComponent(float, unsigned int) {}
void ResurrectCollisionComp::activate() {}
void ResurrectCollisionComp::resetProjectile(bool) {}
void ResurrectCollisionComp::preResetProjectile(bool) {}
void ResurrectCollisionComp::stick(IObject*, const Mat4&) {}
void ResurrectCollisionComp::stickAbsolute(const Mat4&) {}
IObject* ResurrectCollisionComp::getHostObject() const { return 0; }
float ResurrectCollisionComp::getHealEffect(Vec3&) const { return 0.0f; }
bool ResurrectCollisionComp::getDieAfterColl() { return false; }

bool ResurrectCollisionComp::hasCollisionEffect() const
{
    return m_template->getHasCollisionEffect();
}

bool ResurrectCollisionComp::getIsBouncing() const
{
    return m_template->getIsBouncing();
}

int ResurrectCollisionComp::getDefaultCollisionType(int collisionType) const
{
    if (!m_template->getIsBouncing())
        return 0;
    Material* const material =
        materials()->getMaterialPtr(static_cast<unsigned int>(collisionType));
    return material->getHasWaterPhysics() ? 0 : 1;
}

const Mat4& ResurrectCollisionComp::getStickTransformation() const
{
    Debug(DEBUGTYPE_6,
          "Game/Objects/Projectiles/CollisionComponents/ResurrectCollisionComp.h", 71, "")
        << "Dummy mat";
    return Mat4::getIdentity();
}

bool ResurrectCollisionComp::handleCollision(IObject* target, const Vec3& velocity,
                                             const Vec3& normal, const Vec3& collisionOffset,
                                             int collisionType, int b,
                                             int collisionTypeOverride,
                                             CollisionFaceInfo*, CollisionFaceInfo*)
{
    GenericProjectile* const projectile = static_cast<GenericProjectile*>(m_initObject);

    if (target)
    {
        IObject* const launcher = projectile->getLauncherVehicle();
        if (launcher && launcher == getRootParent(target) &&
            WorldPref::getWorldTime() - projectile->getWorldLaunchTime() < 1.0)
        {
            return false;
        }
    }

    int type = collisionType;
    if (collisionTypeOverride != -1)
        type = collisionTypeOverride;

    return gameLogic->handleCollisionForResurrecter(
        projectile, target, m_template->getResurrectDamage(), m_template->getRestoreHP(),
        velocity, normal, collisionOffset, type, b);
}

ResurrectCollisionCompTemplate::ResurrectCollisionCompTemplate()
    : m_refCount(1)
    , m_hasCollisionEffect(false)
    , m_isBouncing(false)
    , m_resurrectDamage(0.0f)
    , m_restoreHP(0.1f)
{
}

void ResurrectCollisionCompTemplate::addRef() { ++m_refCount; }
int ResurrectCollisionCompTemplate::getRef() { return m_refCount; }

int ResurrectCollisionCompTemplate::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        ::operator delete(this);
    return remaining;
}

void* ResurrectCollisionCompTemplate::queryInterface(unsigned int iid) const
{
    ResurrectCollisionCompTemplate* const self =
        const_cast<ResurrectCollisionCompTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent ||
        iid == IID_IProjectileCompTemplate)
        return self;
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int ResurrectCollisionCompTemplate::getClassID() const
{
    return CID_ResurrectCollisionCompTemplate;
}

unsigned int ResurrectCollisionCompTemplate::getInterfaceID()
{
    return IID_IProjectileCollisionCompTemplate;
}

IObjectComponent* ResurrectCollisionCompTemplate::createComponent()
{
    return new ResurrectCollisionComp(this);
}

void ResurrectCollisionCompTemplate::objectCreated(IObject*) {}
void ResurrectCollisionCompTemplate::objectDestroyed(IObject*) {}
void ResurrectCollisionCompTemplate::setTemplate(IObjectTemplate*) {}

void* ResurrectCollisionCompTemplate::queryImplementation(unsigned int classId)
{
    return classId == getClassID() ? this : 0;
}

bool ResurrectCollisionCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (m_hasCollisionEffect)
        script << "ObjectTemplate.collision.hasCollisionEffect " << m_hasCollisionEffect << std::endl;
    if (m_isBouncing)
        script << "ObjectTemplate.collision.bouncing " << m_isBouncing << std::endl;
    if (m_resurrectDamage != 0.0f)
        script << "ObjectTemplate.collision.resurrectDamage " << m_resurrectDamage << std::endl;
    if (m_restoreHP != 0.1f)
        script << "ObjectTemplate.collision.restoreHP " << m_restoreHP << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

int ResurrectCollisionCompTemplate::getResurrectCollisionType() const
{
    return 0;
}

#if defined(_M_IX86)
static_assert(sizeof(ResurrectCollisionComp) == 0x20,
              "ResurrectCollisionComp must match the retail x86 layout");
static_assert(sizeof(ResurrectCollisionCompTemplate) == 0x18,
              "ResurrectCollisionCompTemplate must match the retail x86 layout");
#endif

} // namespace world
} // namespace hfe
} // namespace dice
