// Wheel/track ground effects, instantiated only by client factories.
// Shared behavior uses an image-specific frame clock. Source placement is [INFERRED].
#include "WheelEffectBundle.h"
#include "WheelEffectBundleTemplate.h"
#include "EffectBundle.h"
#include "../ObjectLayoutAccess.h"
#include "../../../Physics/IPhysicsNode.h"
#include "../../../Physics/MaterialManager.h"
#if !DEDICATED
#include "../../Main/BF2Engine.h"                   // bf2Engine->bf2Render (+0x18), a friend there
#include "../../Main/BF2ClientSeams.h"              // BF2Render::getRenderer (+0x10)
#include "../../../RendMisc/IRenderer.h"            // IRenderer::getFrameOrdinal (+0x54)
#endif
#include <cmath>

namespace dice { namespace hfe {
#if !DEDICATED
extern BF2Engine* bf2Engine;                        // Core/Globals.cpp; PE ds:0x986F48
#endif
namespace world {
extern IBase* materialManager;
namespace { typedef object_detail::LayoutAccess L; }

// Returns renderer frame ordinal + 100 on the client, zero on dedicated builds.
// Requires bf2Render and its renderer; there are no null guards.
unsigned int getCurrentFrame()
{
#if !DEDICATED
    return bf2Engine->bf2Render->getRenderer()->getFrameOrdinal() + 100;
#else
    return 0;
#endif
}

// ELF 8215210 / PE 5FCC50 / BF2.exe 0x6456C0 (247 B, same stores). m_unknown (PE +0x160) is
// deliberately not initialized - no image stores it. Both PEs zero-fill m_matrix and write the
// diagonal (the retail Mat4::Mat4()); the tree's aggregate Mat4 reaches the same values through
// getIdentity (see Math/Mat4.h).
WheelEffectBundle::WheelEffectBundle(const WheelEffectBundleTemplate* objectTemplate)
    : SimpleObject(objectTemplate), m_material(-1), m_contactOffset{0, 0, 0},
      m_rotationMatrix(0), m_currentEffect(0), m_playing(false), m_expiryFrame(0)
{
    m_matrix = Mat4::getIdentity();
}
// ELF 82153C0 / PE 5FCAD0 / BF2.exe 0x645540 (84 B; deleting 0x6457C0): map nodes, then base;
// no destroy/delete of the pointer values (destroy/disableCachedObject own them).
WheelEffectBundle::~WheelEffectBundle() {}
// ELF8215670 / PE5FC3F0 / BF2.exe 0x5D3DB0 (a folded `jmp 0x628090`, SimpleObject::init).
void WheelEffectBundle::init() { SimpleObject::init(); }
// ELF 82155D0 / PE 5FC3E0 / BF2.exe 0x6447D0 (`jmp 0x628130`, SimpleObject::enableCachedObject).
void WheelEffectBundle::enableCachedObject() { SimpleObject::enableCachedObject(); }

// ELF 8215500 / PE 5FC960 / BF2.exe 0x645070 (213 B): clear current alias before callbacks, then
// clear cache before base.
void WheelEffectBundle::disableCachedObject()
{
    m_currentEffect = 0;
    for (std::map<int, EffectBundle*>::iterator i = m_effects.begin(); i != m_effects.end(); ++i)
        i->second->destroy(true);
    m_effects.clear();
    SimpleObject::disableCachedObject();
}
// ELF 82155E0 / PE 5FCA40 / BF2.exe 0x645150 (131 B): unlike disableCachedObject, the map
// itself is retained - its destroyed values stay in it.
void WheelEffectBundle::destroy(bool force)
{
    m_currentEffect = 0;
    for (std::map<int, EffectBundle*>::iterator i = m_effects.begin(); i != m_effects.end(); ++i)
        i->second->destroy(true);
    SimpleObject::destroy(force);
}
// ELF 8215690 / PE 5FC550 / BF2.exe 0x6448D0 (92 B). No base forwarding and no action for other
// messages. Message 4 is posted after a player enters the vehicle (Player.cpp's enter sequence)
// and 5 before one leaves (PlayerControlObject.cpp): only occupied vehicles carry flag 0x1000,
// the flag ObjectDrawer's visual pass tests before it calls handleVisualUpdate.
void WheelEffectBundle::handleMessage(TemplateMessage message, IPlayer*, bool recursive)
{
    if (!recursive)
    {
        if (message == 4) updateFlags(0x1000, 0);
        if (message == 5)
        {
            updateFlags(0, 0x1000);
            stopEffect();
        }
    }
}
// ELF8215710 / PE5FC930 / BF2.exe 0x644EB0 (53 B, getCurrentFrame inlined: `add eax, 82h`).
void WheelEffectBundle::handleVisualUpdate(float, float)
{
    updateCurrentState();
    m_expiryFrame = getCurrentFrame() + 30;
    updateFlags(0x800, 0);
}
// ELF8215750; PE5FC5B0 folds to ret8; BF2.exe 0x644930 (79 B, getCurrentFrame inlined as
// `add eax, 64h`, then `cmp eax, [esi+1BCh]; jbe` - unsigned, as the ELF's `ja` at 8215767).
// stopEffect is inlined there; m_playing clears before the updateFlags call, as written here.
void WheelEffectBundle::handleUpdate(float, unsigned int)
{
    if (getCurrentFrame() > m_expiryFrame)
    {
        stopEffect();
        updateFlags(0, 0x800);
    }
}
// ELF 82157A0 / PE 5FC860 / BF2.exe 0x644C70 (205 B): the playing latch becomes true even
// without a current effect.
void WheelEffectBundle::updateCurrentState()
{
    if (m_material == -1)
    {
        if (m_playing) stopEffect();
        return;
    }
    if (m_currentEffect)
    {
        if (m_rotationMatrix) m_currentEffect->setAbsoluteTransformation(*m_rotationMatrix);
        const Vec3& position = getAbsolutePosition();
        const Vec3 effectPosition = {m_contactOffset.x + position.x,
            m_contactOffset.y + position.y, m_contactOffset.z + position.z};
        m_currentEffect->setAbsolutePosition(effectPosition);
        updateEffectProperties();
    }
    if (!m_playing)
    {
        if (m_currentEffect) m_currentEffect->handleMessage(static_cast<TemplateMessage>(0), 0, false);
        m_playing = true;
    }
}
// ELF 82158A0 / PE 5FC5C0 / BF2.exe 0x644980 (642 B; 200 instructions, the same as the dedicated
// PE's, with the same 1.0 / -1.0 / 0.0 constants). Only Speed and SpeedInDof participate.
// The sixth property is evaluated but never dispatched; emitDirectionZFromSpeedDof is not read
// by either body. m_currentEffect is dereferenced unguarded (updateCurrentState's test is the
// only guard).
void WheelEffectBundle::updateEffectProperties()
{
    Vec3 speed = {0, 0, 0};
    float forward = 0;
    float direction = 1;
    IObject* root = getRootParent(this);
    if (root && root != this && L::physicsNode(root))
    {
        speed = static_cast<IPhysicsNode*>(L::physicsNode(root))->getTangentSpeed(getAbsolutePosition());
        const Mat4& transform = root->getAbsoluteTransformation();
#if defined(_WIN32)
        forward = speed.z * transform.m[10] + speed.y * transform.m[9] + speed.x * transform.m[8];
#else
        forward = speed.x * transform.m[8];
        forward = speed.y * transform.m[9] + forward;
        forward = speed.z * transform.m[10] + forward;
#endif
        // !(forward >= 0), NOT forward < 0: a NaN forward speed selects
        // -1 in both images. BF2.exe 0x644A14 `fcomp 0.0; fnstsw ax; test ah, 1; jne -> -1.0`
        // (C0 = less OR unordered) and ELF 0x8215956 `fucomi; jnb` past the -1 store (CF = less
        // OR unordered). The clamp in the loop below is the ieee `<` in both images (BF2.exe
        // ..0x644B41 `fcomp 0.0; test ah, 5; jp`, ELF 0x8215AC0 `fucomi; fcmovbe`): a
        // NaN survives it.
        if (!(forward >= 0)) direction = -1;
    }
    const float magnitude = std::sqrt(speed.z*speed.z + speed.y*speed.y + speed.x*speed.x);
    m_currentEffect->setEmitDirectionScaleFactorForAllEffects(Vec3{1, 1, direction});
    m_currentEffect->m_hasSpeed = true;
    m_currentEffect->m_speed = speed;
    WheelEffectBundleTemplate* t = const_cast<WheelEffectBundleTemplate*>(
        static_cast<const WheelEffectBundleTemplate*>(L::objectTemplate(this)));
    for (unsigned int i = 0; i < 6; ++i)
    {
        float speedFactor = 1, forwardFactor = 1;
        bool used = false;
        if (t->m_property[i][EA_Speed].x != 0 || t->m_property[i][EA_Speed].y != 0)
        {
            speedFactor = t->updateVar(magnitude, t->getProperty(static_cast<EffectProperty>(i), EA_Speed));
            used = true;
        }
        if (t->m_property[i][EA_SpeedInDof].x != 0 || t->m_property[i][EA_SpeedInDof].y != 0)
        {
            if (forward < 0) forward = 0;
            forwardFactor = t->updateVar(forward, t->getProperty(static_cast<EffectProperty>(i), EA_SpeedInDof));
            used = true;
        }
#if defined(_WIN32)
        const float value = (t->m_range[i].y - t->m_range[i].x) * forwardFactor * speedFactor + t->m_range[i].x;
#else
        const float value = t->m_range[i].x + forwardFactor * ((t->m_range[i].y - t->m_range[i].x) * speedFactor);
#endif
        if (used)
            switch (i)
            {
            case 0: m_currentEffect->setAlphaIntensityForAllEffects(value); break;
            case 1: m_currentEffect->setEmitScaleFactorForAllEffects(value); break;
            case 2: m_currentEffect->setEmitSpeedScaleFactorForAllEffects(value); break;
            case 3: m_currentEffect->setAgeScaleFactorForAllEffects(value); break;
            case 4: m_currentEffect->setStartAgeForAllEffects(value); break;
            }
    }
}
// ELF 8215C00 / PE 5FCB30 / BF2.exe 0x6455A0 (277 B; the client's Spring::handleUpdate 0x683780
// calls it at 0x68382F and 0x6838EA). The contact offset updates even when material is
// unchanged, and the new effect's init() runs after overRideTimeToLive(true), as shipped.
// The effect comes from the material cell (the wheel template's material, the ground's), index 0:
// stock materialManagerSettings.con fills those cells with the 44 e_web_* EffectBundles, whose
// particle children are 73 Sprite (type 0), 4 Mesh (type 1, geometry mp_rockshard_l) and one
// NonScreenAligned (type 4, water cavitations) systems.
void WheelEffectBundle::setCurrentMaterial(int material, const Vec3& contactOffset, bool recursive)
{
    if (recursive) return;
    m_contactOffset = contactOffset;
    if (material == m_material) return;
    if (m_currentEffect)
    {
        m_playing = false;
        m_currentEffect->handleMessage(static_cast<TemplateMessage>(1), 0, false);
        m_currentEffect->updateFlags(1, 0);
        m_currentEffect = 0;
    }
    m_material = material;
    if (material == -1) return;
    const int sourceMaterial = L::objectTemplate(this)->getMaterial();
    std::map<int, EffectBundle*>::iterator i = m_effects.find(m_material);
    if (i != m_effects.end())
    {
        EffectBundle* effect = i->second;
        effect->updateFlags(0, 1);
        m_currentEffect = effect;
    }
    else
    {
        IObjectTemplate* t = static_cast<IObjectTemplate*>(static_cast<MaterialManager*>(materialManager)
            ->getEffectTemplate(sourceMaterial, m_material, 0));
        if (t)
        {
            EffectBundle* effect = static_cast<EffectBundle*>(t->createObject());
            if (effect)
            {
                m_effects[m_material] = effect;
                m_currentEffect = effect;
                effect->overRideTimeToLive(true);
                effect->init();
            }
        }
    }
}
// ELF8215E40 / PE5FC400 / BF2.exe 0x6447E0 (23 B, tail-jumps EffectBundle::setCollisionPlane).
void WheelEffectBundle::setCollisionPlane(const Plane3& plane)
{
    if (m_currentEffect) m_currentEffect->setCollisionPlane(plane);
}
// ELF82164B0 / PE5FD450 / BF2.exe 0x4BDB80 (16 B, an identical-code-folded store).
void WheelEffectBundle::setRotationMatrixPtr(const Mat4* matrix)
{
    m_rotationMatrix = matrix;
}
// ELF8215E60 / inlined PE5FC588/5FC909 and BF2.exe 0x64494E/0x644908/0x644D19.
void WheelEffectBundle::stopEffect()
{
    if (m_currentEffect) m_currentEffect->handleMessage(static_cast<TemplateMessage>(1), 0, false);
    m_playing = false;
}
// ELF8215EB0 / PE5FC450 / BF2.exe 0x546760 (11 B, folded with SimpleObject's identical body).
int WheelEffectBundle::getMaterial() const
{
    return L::objectTemplate(this)->getMaterial();
}
} } }
