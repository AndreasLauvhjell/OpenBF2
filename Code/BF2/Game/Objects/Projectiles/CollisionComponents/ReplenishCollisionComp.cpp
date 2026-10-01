// Path:     [INFERRED] companion to the attested ReplenishCollisionComp.h.
// Contents: recovered from bf2.i64 (ELF 0x8289890..0x828A870) and cross-checked against
// bf2_w32ded.exe (0x5B5370..0x5B56xx).

#include "ReplenishCollisionComp.h"

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

ReplenishCollisionComp::ReplenishCollisionComp(ReplenishCollisionCompTemplate* objectTemplate)
    : m_refCount(1)
    , m_template(objectTemplate)
{
    ProjectileComp::m_refCount = 1;
    m_object = 0;
    m_initObject = 0;
}

ReplenishCollisionComp::~ReplenishCollisionComp() {}

void ReplenishCollisionComp::addRef() { ++m_refCount; }
int ReplenishCollisionComp::getRef() { return m_refCount; }

int ReplenishCollisionComp::release()
{
    const int remaining = m_refCount - 1;
    const bool last = m_refCount == 1;
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

void* ReplenishCollisionComp::queryInterface(unsigned int iid) const
{
    ReplenishCollisionComp* const self = const_cast<ReplenishCollisionComp*>(this);
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return static_cast<IObjectComponent*>(self);
    if (iid == IID_IProjectileComp || iid == IID_IProjectileCollisionComp)
        return static_cast<IProjectileCollisionComp*>(self);
    return 0;
}

unsigned int ReplenishCollisionComp::getInterfaceID() const
{
    return IID_IProjectileCollisionComp;
}

bool ReplenishCollisionComp::hasUpdate() const { return false; }
void ReplenishCollisionComp::updateComponent(float, unsigned int) {}
void ReplenishCollisionComp::activate() {}
void ReplenishCollisionComp::resetProjectile(bool) {}
void ReplenishCollisionComp::preResetProjectile(bool) {}
void ReplenishCollisionComp::stick(IObject*, const Mat4&) {}
void ReplenishCollisionComp::stickAbsolute(const Mat4&) {}
IObject* ReplenishCollisionComp::getHostObject() const { return 0; }

float ReplenishCollisionComp::getHealEffect(Vec3&) const
{
    return m_template->getReplenishingStrength();
}

bool ReplenishCollisionComp::hasCollisionEffect() const
{
    return m_template->getHasCollisionEffect();
}

bool ReplenishCollisionComp::getIsBouncing() const
{
    return m_template->getIsBouncing();
}

int ReplenishCollisionComp::getDefaultCollisionType(int collisionType) const
{
    if (!m_template->getIsBouncing())
        return 0;
    Material* const material =
        materials()->getMaterialPtr(static_cast<unsigned int>(collisionType));
    return material->getHasWaterPhysics() ? 0 : 1;
}

const Mat4& ReplenishCollisionComp::getStickTransformation() const
{
    Debug(DEBUGTYPE_6,
          "Game/Objects/Projectiles/CollisionComponents/ReplenishCollisionComp.h", 71, "")
        << "Dummy mat";
    return Mat4::getIdentity();
}

bool ReplenishCollisionComp::handleCollision(IObject* target, const Vec3& velocity,
                                             const Vec3& normal, const Vec3& collisionOffset,
                                             int collisionType, int b,
                                             int collisionTypeOverride,
                                             CollisionFaceInfo*, CollisionFaceInfo*)
{
    GenericProjectile* const projectile = static_cast<GenericProjectile*>(m_initObject);

    if (target)
    {
        IObject* const launcher = projectile->getLauncherVehicle();
        if (launcher && launcher == getRootParent(target))
        {
            // Both ports convert the unsigned tick values to float before adding/comparing.
            const float rejectUntil =
                static_cast<float>(static_cast<unsigned int>(projectile->m_dword1BC)) + 30.0f;
            const float now =
                static_cast<float>(static_cast<unsigned int>(WorldPref::getTickCount()));
            if (now < rejectUntil)
                return false;
        }
    }

    int type = collisionType;
    if (collisionTypeOverride != -1)
        type = collisionTypeOverride;

    return gameLogic->handleCollisionForReplenisher(
        static_cast<int>(m_template->getReplenishingType()) == 0,
        projectile, target, velocity, normal, collisionOffset, type, b);
}

ReplenishCollisionCompTemplate::ReplenishCollisionCompTemplate()
    : m_refCount(1)
    , m_hasCollisionEffect(false)
    , m_isBouncing(false)
    , m_replenishingStrength(0.0f)
    , m_replenishingType(static_cast<ReplenishingType>(0))
{
}

void ReplenishCollisionCompTemplate::addRef() { ++m_refCount; }
int ReplenishCollisionCompTemplate::getRef() { return m_refCount; }

int ReplenishCollisionCompTemplate::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        ::operator delete(this);
    return remaining;
}

void* ReplenishCollisionCompTemplate::queryInterface(unsigned int iid) const
{
    ReplenishCollisionCompTemplate* const self =
        const_cast<ReplenishCollisionCompTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent ||
        iid == IID_IProjectileCompTemplate)
        return self;
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int ReplenishCollisionCompTemplate::getClassID() const
{
    return CID_ReplenishCollisionCompTemplate;
}

unsigned int ReplenishCollisionCompTemplate::getInterfaceID()
{
    return IID_IProjectileCollisionCompTemplate;
}

IObjectComponent* ReplenishCollisionCompTemplate::createComponent()
{
    return new ReplenishCollisionComp(this);
}

void ReplenishCollisionCompTemplate::objectCreated(IObject*) {}
void ReplenishCollisionCompTemplate::objectDestroyed(IObject*) {}
void ReplenishCollisionCompTemplate::setTemplate(IObjectTemplate*) {}

void* ReplenishCollisionCompTemplate::queryImplementation(unsigned int classId)
{
    return classId == getClassID() ? this : 0;
}

bool ReplenishCollisionCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (m_hasCollisionEffect)
        script << "ObjectTemplate.collision.hasCollisionEffect " << m_hasCollisionEffect << std::endl;
    if (m_isBouncing)
        script << "ObjectTemplate.collision.bouncing " << m_isBouncing << std::endl;

    // Retail always writes the type, including its constructor default.
    script << "ObjectTemplate.collision.replenishingType " << m_replenishingType << std::endl;
    if (m_replenishingStrength != 0.0f)
        script << "ObjectTemplate.collision.replenishingStrength "
               << m_replenishingStrength << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

int ReplenishCollisionCompTemplate::getReplenishCollisionType() const
{
    return 0;
}

#if defined(_M_IX86)
static_assert(sizeof(ReplenishCollisionComp) == 0x1C,
              "ReplenishCollisionComp must match the retail x86 layout");
static_assert(sizeof(ReplenishCollisionCompTemplate) == 0x18,
              "ReplenishCollisionCompTemplate must match the retail x86 layout");
#endif

} // namespace world
} // namespace hfe
} // namespace dice
