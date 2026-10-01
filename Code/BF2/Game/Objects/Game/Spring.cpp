// Spring object behavior and wheel-effect integration.

#include "Spring.h"

#include <string>

#include "../../../Core/Ids.h"
#include "../../../Physics/IPhysicsNode.h"                   // getUnderWater (+0x10C)
#include "../../../Physics/PhysicsSpring.h"                  // setRelationToParentPosition
#include "../../../Scene/Object/ObjectUtil.h"                // setNetworkableComp
#include "../../../Sound/WheelSound.h"
#include "../IObjectTemplate.h"
#include "../ObjectLayoutAccess.h"                          // the +0x2C sibling, +0x14 template, +0x44 node, +0x48 response physics
#include "../ObjectTemplate.h"                               // getClassID on the read template
#include "../TemplateLeaves.h"                               // SpringTemplate
#include "../Fx/WheelEffectBundle.h"

namespace dice {
namespace hfe {
namespace world {

namespace {
    typedef object_detail::LayoutAccess L;

}

bool isEffectBundle(const IObjectTemplate* objectTemplate)
{
    if (!objectTemplate)
        return false;
#if defined(_WIN32)
    // Dedicated PE 62D307..323 repeats the virtual query on the non-EffectBundle arm.
    return objectTemplate->getClassID() == CID_EffectBundleTemplate
        || objectTemplate->getClassID() == CID_WheelEffectBundleTemplate;
#else
    const unsigned int cid = objectTemplate->getClassID();
    return cid == CID_EffectBundleTemplate || cid == CID_WheelEffectBundleTemplate;
#endif
}

// The constructor - 0x82AD620, 290 B: Bundle(template); the vptrs; the own block (materials -1,
// the rest 0); flag 0x800 (the spring ticks from birth - handleUpdate below); then the WheelSound
// (a fresh one always - the replace-if-different dance the ELF spells is over a null +0x168).
Spring::Spring(const SpringTemplate* objectTemplate)
    : Bundle(objectTemplate)
{
    m_dword140 = 0;                                                                  // +0x140
    m_material = -1;                                                                 // +0x144
    m_prevMaterial = -1;                                                             // +0x148
    m_hasContact = false; m_byte13D = false; m_occupied = false;                     // +0x13C..+0x13E
    m_contactPosition.x = m_contactPosition.y = m_contactPosition.z = 0.0f;         // +0x14C
    m_contactNormal.x = m_contactNormal.y = m_contactNormal.z = 0.0f;               // +0x158
    m_rotationSpeed = 0.0f;                                                          // +0x164
    m_wheelSound = 0;                                                                // +0x168
    updateFlags(0x800u, 0u);
    WheelSound* sound = new WheelSound(objectTemplate, this);
    if (sound != m_wheelSound)
    {
        if (m_wheelSound)
            delete m_wheelSound;
        m_wheelSound = sound;
    }
}

// / 0x82ADF20: the engine weak pointer's block released (WeakPtr's dtor), the sound
// deleted, ~Bundle.
Spring::~Spring()
{
    if (m_wheelSound)
        delete m_wheelSound;
}

// init - 0x82AD880, 214 B: Bundle::init(); the physics-node component (a PhysicsSpring - the
// template's setPhysicsNodeComponent built it) learns the spring's relative position as its
// relation to the parent; then every direct child that is an effect bundle gets message 1 and,
// when the spring has a parent, a wheel effect bundle also gets the parent's absolute
// transformation (its slot 97).
void Spring::init()
{
    Bundle::init();
    if (IBase* component = queryComponent(IID_IPhysicsNode, IID_IPhysicsNode))     // slot 23
        static_cast<PhysicsSpring*>(static_cast<IPhysicsNode*>(component))->setRelationToParentPosition(getRelativePosition());   // slot 56
    for (IObject* child = getChild(); child; child = L::sibling(child))              // +0xF0 +0x2C
    {
        const IObjectTemplate* t = L::objectTemplate(child);                         // +0x14
        if (!isEffectBundle(t))
            continue;
        child->handleMessage(static_cast<TemplateMessage>(1), 0, false);            // slot 65
        if (m_parent && t->getClassID() == CID_WheelEffectBundleTemplate)
            static_cast<WheelEffectBundle*>(child)->setRotationMatrixPtr(&m_parent->getAbsoluteTransformation());
    }
}

// getEngine - 0x82AD960, 200 B: the cached engine, or - only when the spring has response
// physics (+0x48) - the first ancestor whose template class is EngineTemplate, cached as a weak
// pointer (IObject::getWeakPtr, +0x1C). 0 when there is none.
IObject* Spring::getEngine() const
{
    IObject* engine = m_engine.get();                                                // +0x16C
    if (engine)
        return engine;
    if (L::responsePhysics(const_cast<Spring*>(this)))                               // +0x48
    {
        IObject* walk = m_parent;                                                    // +0x28
        while (walk)
        {
            if (L::objectTemplate(walk)->getClassID() == CID_EngineTemplate)
            {
                m_engine = walk->getWeakPtr();
                break;
            }
            walk = walk->getParent();
        }
    }
    return m_engine.get();
}

void Spring::initNetworkableComp()
{
    if (m_template && m_template->getNetworkableInfo().compare("") != 0)
        setNetworkableComp(this, CID_SpringNetworkable);                             // 39004
}

// handleUpdate - 0x82ADA90, 533 B. Skipped entirely under flag bit 4 (the second argument). With
// children: a fresh contact (handleCollision raised +0x13C) whose material differs from the last
// one is pushed to every wheel effect child - setCurrentMaterial(material, contact - my absolute
// position, flag & 4) and the world contact plane through setCollisionPlane - and remembered; the contact flag
// then clears. No contact but a remembered material -> every wheel effect child gets material -1
// at Vec3::getZero() and both materials reset to -1. Then the wheel sound updates.
void Spring::handleUpdate(float dt, unsigned int flags)
{
    if (flags & 4u)
        return;
    if (getChild())
    {
        if (m_hasContact)
        {
            if (m_material != m_prevMaterial)
            {
                for (IObject* child = getChild(); child; child = L::sibling(child))
                {
                    if (L::objectTemplate(child)->getClassID() != CID_WheelEffectBundleTemplate)
                        continue;
                    const Vec3& pos = getAbsolutePosition();                         // slot 27
                    Vec3 rel;
                    rel.x = m_contactPosition.x - pos.x;
                    rel.y = m_contactPosition.y - pos.y;
                    rel.z = m_contactPosition.z - pos.z;
                    WheelEffectBundle* wheel = static_cast<WheelEffectBundle*>(child);
                    wheel->setCurrentMaterial(m_material, rel, (flags & 4u) != 0);
                    Plane3 plane;
                    plane.normal = m_contactNormal;
                    // PE 62D6F8..745 constructs all four plane scalars. ELF groups
                    // x/y before z; the dedicated PE groups y/z before x.
#if defined(_WIN32)
                    plane.d = -(m_contactPosition.y * m_contactNormal.y
                        + m_contactPosition.z * m_contactNormal.z
                        + m_contactPosition.x * m_contactNormal.x);
#else
                    plane.d = -(m_contactPosition.y * m_contactNormal.y
                        + m_contactPosition.x * m_contactNormal.x
                        + m_contactPosition.z * m_contactNormal.z);
#endif
                    wheel->setCollisionPlane(plane);
                }
                m_prevMaterial = m_material;
            }
            m_hasContact = false;
        }
        else if (m_prevMaterial != -1)
        {
            for (IObject* child = getChild(); child; child = L::sibling(child))
            {
                if (L::objectTemplate(child)->getClassID() != CID_WheelEffectBundleTemplate)
                    continue;
                static_cast<WheelEffectBundle*>(child)->setCurrentMaterial(-1, Math::Vec3::getZero(), (flags & 4u) != 0);
            }
            m_material = -1;
            m_prevMaterial = -1;
        }
    }
    m_wheelSound->update(dt);
}

// 2 (entered) -> occupied; 3 (left) -> not occupied, rotation speed 0; then
// Bundle::handleMessage for every message.
void Spring::handleMessage(TemplateMessage message, IPlayer* player, bool flag)
{
    const int m = static_cast<int>(message);
    if (m == 2)
        m_occupied = true;                                                           // +0x13E
    else if (m == 3)
    {
        m_occupied = false;
        m_rotationSpeed = 0.0f;                                                      // +0x164
    }
    Bundle::handleMessage(message, player, flag);
}

// the contact flag; the material is the second int - or 1 when the physics
// node reports water (getUnderWater > 0); the normal is the second vector; the contact position is
// my absolute position plus the third vector; then Bundle::handleCollision.
bool Spring::handleCollision(IObject* other, const Vec3& v1, const Vec3& v2, const Vec3& v3,
                             int i1, int i2, CollisionFaceInfo* f1, CollisionFaceInfo* f2)
{
    m_hasContact = true;                                                             // +0x13C
    IPhysicsNode* node = static_cast<IPhysicsNode*>(L::physicsNode(this));           // +0x44
    m_material = i2;                                                                 // +0x144
    if (node->getUnderWater() > 0.0f)                                                // +0x10C
        m_material = 1;
    m_contactNormal = v2;                                                            // +0x158
    const Vec3& pos = getAbsolutePosition();                                         // slot 27
    m_contactPosition.x = pos.x + v3.x;                                              // +0x14C
    m_contactPosition.y = pos.y + v3.y;
    m_contactPosition.z = pos.z + v3.z;
    return Bundle::handleCollision(other, v1, v2, v3, i1, i2, f1, f2);
}

} // namespace world
} // namespace hfe
} // namespace dice
