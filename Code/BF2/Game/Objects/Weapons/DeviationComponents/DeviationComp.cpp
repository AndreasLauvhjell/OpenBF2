// Path:     [INFERRED] beside the attested DeviationComp.h.
// Contents: common deviation runtime, recovered from the BF2 1.50 Linux ELF and cross-read against
// the Win32 dedicated-server image.

#include "DeviationComp.h"

#include <string>

#include "../GenericFireArm.h"
#include "../../ObjectLayoutAccess.h"
#include "../../IObject.h"
#include "../../IPlayer.h"
#include "../../../../Core/IBase.h"
#include "../../../../Core/Ids.h"
#include "../../../../Core/StringUtil.h"
#include "../../../IGame.h"
#include "../../../GameServer/ServerSettings.h"
#include "../../../../Physics/IPhysicsNode.h"

namespace dice {
namespace hfe {
namespace world {

namespace {

bool g_normalTableInitialized = false;
float g_normalTable[1024];

} // anonymous namespace

DeviationComp::DeviationComp(DeviationCompTemplate* objectTemplate)
    : m_deviation(0.0f)
    , m_fireDeviation(0.0f)
    , m_cachedDeviationX(0.0f)
    , m_cachedDeviationY(0.0f)
    , m_random()
    , m_template(objectTemplate)
{
    // WeaponComp has no emitted constructor. As in the other weapon-component bases, only the
    // reference count is initialized here; setObject/init publish the two object pointers later.
    m_refCount = 1;

    // Retail builds this process-wide normal table lazily from the first DeviationComp's own RNG.
    // There is deliberately no lock around the guard.
    if (!g_normalTableInitialized)
    {
        m_random.m_seed = 135150u;
        for (int i = 0; i != 1024; ++i)
            g_normalTable[i] = m_random.getNormal();
        g_normalTableInitialized = true;
    }
}

DeviationComp::~DeviationComp()
{
}

unsigned int DeviationComp::getInterfaceID() const { return IID_IWeaponDeviationComp; }
void DeviationComp::initComponent(WeaponInitEvent&) {}
bool DeviationComp::preUpdate() { return false; }
void DeviationComp::updateComponent(float, unsigned int) {}
bool DeviationComp::isSelectable() const { return true; }
void DeviationComp::fireStop(bool) {}
bool DeviationComp::requestFire(bool, bool) { return true; }

// . The component perturbs the projectile velocity along the firearm transform's
// first two axes, then grows the accumulated fire deviation. The normal-table index deliberately
// includes the current input index so prediction consumes the same samples as retail.
void DeviationComp::postFire(WeaponFireEvent& event)
{
    IObject* const projectile = event.projectile;
    if (!projectile)
        return;

    const Mat4 transform = projectile->getAbsoluteTransformation();

    // The shipped body performs this query and discards the answer before reaching the raw physics
    // node field. Keep the observable virtual call even though the typed result is not needed.
    (void)projectile->queryInterface(IID_IProjectileObject);

    IPhysicsNode* const node = static_cast<IPhysicsNode*>(
        object_detail::LayoutAccess::physicsNode(projectile));
    if (!node)
        return;

    Vec3 speed = node->getPositionalSpeed();
    float deviation = event.dummyProjectile ? m_template->getSubProjectileDev() : m_deviation;

    std::string gameMode;
    if (game)
        gameMode = game->getCurrentMapGameMode();
    if (compareIgnoreCase(gameMode, std::string("gpm_coop")) == 0)
    {
        GenericFireArm* const weapon = firearm();
        if (weapon)
        {
            IPlayer* const player = weapon->getPlayer();
            if (player && player->getIsAIPlayer())
                deviation = (100.0f - serverSettings->getCoopBotDifficulty()) / 10.0f;
        }
    }

    if (deviation > 0.01f)
    {
        const int tableIndex =
            (event.projectileIndex + game->getCurrentInputIndex()) & 0x3FF;
        float deviationX = deviation * g_normalTable[tableIndex];
        float deviationY = deviation * g_normalTable[(tableIndex + 1) & 0x3FF];

        if (event.dummyProjectile)
        {
            m_cachedDeviationX = deviationX;
            m_cachedDeviationY = deviationY;
        }
        else
        {
            deviationX += m_cachedDeviationX;
            deviationY += m_cachedDeviationY;
        }

        // Retail deliberately dispatches getDevMod twice rather than caching it: once for each
        // transform axis contribution. Preserve both observable virtual calls.
        const float deviationXScale = firearm()->getDevMod() * deviationX;
        speed.x += transform.m[4] * deviationXScale;
        speed.y += transform.m[5] * deviationXScale;
        speed.z += transform.m[6] * deviationXScale;

        const float deviationYScale = firearm()->getDevMod() * deviationY;
        speed.x += transform.m[0] * deviationYScale;
        speed.y += transform.m[1] * deviationYScale;
        speed.z += transform.m[2] * deviationYScale;
    }

    m_fireDeviation += m_template->getFireDevAdd();
    if (m_fireDeviation > m_template->getFireDevMax())
        m_fireDeviation = m_template->getFireDevMax();

    node->setPositionalSpeed(speed);
}

void DeviationComp::fireComplete(unsigned int) {}
void DeviationComp::postSwitchFireRate(FireRate) {}
bool DeviationComp::requestReload(bool) { return true; }
void DeviationComp::reload(ReloadState) {}
void DeviationComp::postReload() {}
void DeviationComp::outOfAmmo(bool) {}
void DeviationComp::stoppedTracking() {}
void DeviationComp::postChangeTarget(IObject*) {}
void DeviationComp::projectileWasReset(IObject*) {}
void DeviationComp::enable() {}
void DeviationComp::disable() {}
void DeviationComp::stop() {}
bool DeviationComp::busy() { return false; }
void DeviationComp::exit() {}
void DeviationComp::destroy() {}
void DeviationComp::pickup() {}
void DeviationComp::reset() {}
void DeviationComp::updateDeviation(io::PlayerInput, bool, bool) {}
void DeviationComp::setZoom(int) {}
Vec3 DeviationComp::deviatePosition(Vec3&, int) { return Vec3::getZero(); }
void DeviationComp::setProjectileDeviationFixed(Vec3&, int) {}
float DeviationComp::getMinDeviation() const { return m_template->getMinDev(); }
float DeviationComp::getMaxDeviation() const { return 0.0f; }
float DeviationComp::getDeviation() const { return m_deviation; }

} // namespace world
} // namespace hfe
} // namespace dice
