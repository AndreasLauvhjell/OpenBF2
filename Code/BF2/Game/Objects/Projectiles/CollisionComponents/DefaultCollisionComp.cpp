// Routes projectile collisions to ServerGameLogic for resolution.

#include "DefaultCollisionComp.h"

#include <cstddef>
#include <cfloat>                                    // FLT_EPSILON
#include <sstream>                                   // makeScript's ostringstream
#include <string>

#include "../ProjectileCompTemplates.h"
#include "../../../../IO/Stream/IStream.h"           // makeScript's sink
#include "../GenericProjectile.h"
#include "../../IObject.h"
#include "../../IObjectTemplate.h"
#include "../../Game/Soldier.h"
#include "../../../IGameLogic.h"
#include "../../../../Core/Debug.h"
#include "../../../../Core/Ids.h"
#include "../../../../Core/System.h"
#include "../../../../Math/Mat4.h"
#include "../../../../Physics/Material.h"
#include "../../../../Physics/MaterialManager.h"

namespace dice {
namespace hfe {
namespace world {

// Already inside namespace world - the global is declared, not defined, here. The tree also
// declares this as an opaque `IBase*` in three other TUs; MaterialManager.h gives it a real
// type, which is what lets getDefaultCollisionType below dispatch without a raw slot.
// THE global IS defined AS `IBase*` in Core/Globals.cpp, and that spelling is the one the
// linker sees - MSVC encodes a data symbol's type into its decorated name, so declaring it as
// `MaterialManager*` here would silently be a different symbol. Declare it as it is defined
// and downcast at the use site; MaterialManager.h gives the downcast a real type to land on.
extern IBase* materialManager;                       // ds:0x8AD2820 - Core/Globals.cpp

inline MaterialManager* materials()
{
    return static_cast<MaterialManager*>(materialManager);
}

// Initializes separate base and leaf reference counts; only the leaf count is decremented.
DefaultCollisionComp::DefaultCollisionComp(DefaultCollisionCompTemplate* objectTemplate)
    : m_refCount(1)
    , m_template(objectTemplate)
    , m_dword1C(0)
    , m_collisionHandled(false)
{
    m_refCount = 1;
    m_object = 0;
    m_initObject = 0;
    ProjectileComp::m_refCount = 1;
}

DefaultCollisionComp::~DefaultCollisionComp() {}

void DefaultCollisionComp::addRef() { ++m_refCount; }
int  DefaultCollisionComp::getRef() { return m_refCount; }

// . Decrements the leaf count and self-deletes through the deleting-destructor slot.
int DefaultCollisionComp::release()
{
    const int remaining = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

// . TWO IDS return THE primary pointer, TWO return THE +0x10 face.
void* DefaultCollisionComp::queryInterface(unsigned int iid) const
{
    DefaultCollisionComp* const self = const_cast<DefaultCollisionComp*>(this);

    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return static_cast<IObjectComponent*>(self);
    if (iid == IID_IProjectileComp || iid == IID_IProjectileCollisionComp)
        return static_cast<IProjectileCollisionComp*>(self);
    return 0;
}

unsigned int DefaultCollisionComp::getInterfaceID() const
{
    return IID_IProjectileCollisionComp;
}

// the inert half
// ALL seven ARE genuinely empty IN THE shipped image (5-7 bytes each), not unrecovered. A default
// collision component has no per-tick work, no reset state, and never sticks to anything.
bool DefaultCollisionComp::hasUpdate() const { return false; }
void DefaultCollisionComp::updateComponent(float, unsigned int) {}
void DefaultCollisionComp::activate() {}
void DefaultCollisionComp::preResetProjectile(bool) {}
void DefaultCollisionComp::resetProjectile(bool) {}
float DefaultCollisionComp::getHealEffect(Vec3&) const { return 0.0f; }
void DefaultCollisionComp::stick(IObject*, const Mat4&) {}
void DefaultCollisionComp::stickAbsolute(const Mat4&) {}
IObject* DefaultCollisionComp::getHostObject() const { return 0; }

// . NOT A stub - A diagnosed refusal. Reaching this means something asked a
// non-sticky projectile where it is stuck; dice logs "Dummy mat" against DefaultCollisionComp.h:70
// and hands back the identity. StickyCollisionComp is the class that answers it for real.
const Mat4& DefaultCollisionComp::getStickTransformation() const
{
    Debug(DEBUGTYPE_6,
          "Game/Objects/Projectiles/CollisionComponents/DefaultCollisionComp.h", 70, "")
        << "Dummy mat";
    return Mat4::getIdentity();
}

// the template-backed getters
bool DefaultCollisionComp::hasCollisionEffect() const
{
    return m_template->getHasCollisionEffect();
}

bool DefaultCollisionComp::getIsBouncing() const
{
    return m_template->getIsBouncing();
}

// Returns the configured bounce collision type only for a bouncing projectile hitting non-water material.
// The material pointer is unchecked; getMaterialPtr requires material 0 as its fallback.
int DefaultCollisionComp::getDefaultCollisionType(int collisionType) const
{
    if (!m_template->getIsBouncing())
        return 0;

    Material* const material =
        materials()->getMaterialPtr(static_cast<unsigned int>(collisionType));
    return material->getHasWaterPhysics() ? 0 : 1;
}

// Routes collisions using velocity, surface normal and projectile-relative collisionOffset.
// Windows return their own collision result; UAVs receive the call and still continue through normal handling.
// Suppresses hits against the firing root for one second, normalizes the surface normal and latches
// the one-shot soldier damage animation before forwarding to game logic.
bool DefaultCollisionComp::handleCollision(IObject* target, const Vec3& velocity,
                                           const Vec3& normal, const Vec3& collisionOffset,
                                           int collisionType, int b, int collisionTypeOverride,
                                           CollisionFaceInfo* face1, CollisionFaceInfo* face2)
{
    IObject* const projectile = m_initObject;

    if (target)
    {
        const IObjectTemplate* const targetTemplate = target->getTemplate();
        if (targetTemplate &&
            (targetTemplate->getClassID() == CID_DestroyableWindowTemplate ||
             targetTemplate->getClassID() == CID_DestroyableWindowsBundleTemplate))
        {
            return target->handleCollision(projectile, velocity, normal, collisionOffset,
                                           collisionType, b, face1, face2);
        }

        if (target->getTemplate() &&
            target->getTemplate()->getClassID() == CID_UAVVehicleTemplate)
        {
            // Result deliberately discarded - the UAV also takes the normal path below.
            target->handleCollision(projectile, velocity, normal, collisionOffset,
                                    collisionType, b, face1, face2);
        }

        IObject* const firingRoot =
            static_cast<GenericProjectile*>(projectile)->getLauncherVehicle();   // +0x1CC
        if (firingRoot &&
            firingRoot == getRootParent(target) &&
            WorldPref::getWorldTime() -
                static_cast<GenericProjectile*>(projectile)->getWorldLaunchTime() < 1.0)
        {
            return false;
        }
    }

    int type = collisionType;
    if (collisionTypeOverride != -1)
        type = collisionTypeOverride;

    Vec3 unitNormal = normal;
    const float lengthSq = unitNormal.x * unitNormal.x +
                           unitNormal.y * unitNormal.y +
                           unitNormal.z * unitNormal.z;
    if (fabsf(lengthSq - 1.0f) >= FLT_EPSILON)
    {
        if (fabsf(lengthSq) >= FLT_EPSILON)
        {
            const float inverse = 1.0f / sqrtf(lengthSq);
            unitNormal.x *= inverse;
            unitNormal.y *= inverse;
            unitNormal.z *= inverse;
        }
        else
        {
            unitNormal.x = 0.0f;
            unitNormal.y = 0.0f;
            unitNormal.z = 0.0f;
        }
    }

    if (!m_collisionHandled &&
        static_cast<GenericProjectile*>(projectile)->getProjectileDamage() > 0 &&
        target &&
        target->getTemplate()->getClassID() == CID_SoldierTemplate)
    {
        static_cast<Soldier*>(target)->playDamageAnimations();
    }

    m_collisionHandled = true;

    // and the answer is a constant. handleCollisionForProjectile was read in
    // full (0x81018C0): every path leaves 1 in eax, so this returns true unconditionally once the
    // collision has been handed to GameLogic. The earlier [CHOSEN] `false` was the wrong guess.
    // [RETIRE-WHEN: gamelogic-handlecollision-return] is closed.
    return gameLogic->handleCollisionForProjectile(projectile, target, velocity, unitNormal,
                                                   collisionOffset, type, b, face1, face2);
}


// DefaultCollisionCompTemplate::DefaultCollisionCompTemplate - 0x8288A40, 36 B. Five stores: the two
// vptrs, the reference count, and the two behaviour bytes both starting false. A projectile template
// that never writes `collisionEffect` or `isBouncing` therefore gets a non-bouncing round with no
// impact effect, and getDefaultCollisionType short-circuits to 0 for it.
// Historical staging note: at constructor recovery time the twelve template virtuals were still
// declaration-only. They are implemented immediately below; this paragraph records why the
// constructor originally landed separately rather than describing the current state.
DefaultCollisionCompTemplate::DefaultCollisionCompTemplate()
    : m_refCount(1)
    , m_hasCollisionEffect(false)
    , m_isBouncing(false)
{
}

// Default collision template lifecycle. objectCreated, objectDestroyed and setTemplate intentionally do nothing;
// the template owns no per-object state.

void DefaultCollisionCompTemplate::addRef() { ++m_refCount; }
int  DefaultCollisionCompTemplate::getRef() { return m_refCount; }

// . Returns the decremented count, and deletes at zero before returning it - the value
// is read out of the register the decrement left it in, so the read after `delete this` is the
// shipped shape, not a use-after-free of the object.
int DefaultCollisionCompTemplate::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        ::operator delete(this);
    return remaining;
}

// . Three ids answer with the primary address; IConsoleSaveable answers with the
// second base at +4. Anything else is null.
void* DefaultCollisionCompTemplate::queryInterface(unsigned int iid) const
{
    DefaultCollisionCompTemplate* const self =
        const_cast<DefaultCollisionCompTemplate*>(this);

    if (iid == IID_IBase
        || iid == IID_IObjectTemplateComponent
        || iid == IID_IProjectileCompTemplate)
    {
        return self;
    }
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int DefaultCollisionCompTemplate::getClassID() const
{
    return CID_DefaultCollisionCompTemplate;
}

unsigned int DefaultCollisionCompTemplate::getInterfaceID()
{
    return IID_IProjectileCollisionCompTemplate;
}

// . `operator new(0x24)` - the component's proven 36 bytes - then its constructor with
// this template. THE ONE slot RW-6b WAS waiting ON.
IObjectComponent* DefaultCollisionCompTemplate::createComponent()
{
    return new DefaultCollisionComp(this);
}

void DefaultCollisionCompTemplate::objectCreated(IObject*) {}               // empty
void DefaultCollisionCompTemplate::objectDestroyed(IObject*) {}            // empty
void DefaultCollisionCompTemplate::setTemplate(IObjectTemplate*) {}        // empty

// . It asks itself for its class id through THE vtable (`call [eax+10h]`) rather than
// comparing a constant, so a subclass that overrides getClassID answers for its own id here and
// NOT for this one. Reproduced as the virtual call it is.
void* DefaultCollisionCompTemplate::queryImplementation(unsigned int classId)
{
    return classId == getClassID() ? this : 0;
}

// . THE TWO property names ARE attested string literals, and the second one is worth
// noting: the console name is `bouncing`, not `isBouncing` as the accessor is spelled.
// Each line is emitted only when the byte is true - a template left at its constructor defaults
// saves nothing at all, which is why both defaults are false.
bool DefaultCollisionCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (m_hasCollisionEffect)
        script << "ObjectTemplate.collision.hasCollisionEffect " << m_hasCollisionEffect << std::endl;
    if (m_isBouncing)
        script << "ObjectTemplate.collision.bouncing " << m_isBouncing << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

// `xor eax, eax; retn`. The template's own answer is always 0; the component's
// getDefaultCollisionType(int) is the one that varies, and it short-circuits on m_isBouncing.
int DefaultCollisionCompTemplate::getDefaultCollisionType() const
{
    return 0;
}

} // namespace world
} // namespace hfe
} // namespace dice
