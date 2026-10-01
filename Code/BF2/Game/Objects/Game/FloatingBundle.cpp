// Floating-bundle physics and lifecycle.

#include "FloatingBundle.h"

#include "../../../Core/Debug.h"
#include "../../../Core/IBase.h"
#include "../../../Core/Ids.h"
#include "../../../Physics/PhysicsFloatingBundle.h"
#include "../GameLogicComp/IArmorGLComp.h"                 // getHitPoints, slot 15 / +0x3C
#include "../ObjectLayoutAccess.h"                          // +0x44 physicsNode
#include "../RotationalBundleLeaves.h"                        // FloatingBundleTemplate (a RotationalBundleTemplate)

namespace dice {
namespace hfe {
namespace world {

// RotationalBundle(template); the vptrs; +0x1B0 = 0.
FloatingBundle::FloatingBundle(const FloatingBundleTemplate* objectTemplate)
    : RotationalBundle(objectTemplate)
    , m_floaterAngle(0.0f)                                                           // +0x1B0
{
}

FloatingBundle::~FloatingBundle() {}                                                 // ~RotationalBundle

// / 0x82A6550, 38 B / 0x82A6580, 26 B - the angle cleared around the base calls.
void FloatingBundle::init()
{
    RotationalBundle::init();
    m_floaterAngle = 0.0f;
}

void FloatingBundle::enableCachedObject()
{
    RotationalBundle::enableCachedObject();
    m_floaterAngle = 0.0f;
}

void FloatingBundle::disableCachedObject()
{
    m_floaterAngle = 0.0f;
    RotationalBundle::disableCachedObject();
}

// Updates the floating-physics angle from rotation; destroyed parent armor forces the angle to -50.
// Requires a physics node; absence triggers a fatal diagnostic.
void FloatingBundle::handleUpdate(float dt, unsigned int)
{
    PhysicsFloatingBundle* node = static_cast<PhysicsFloatingBundle*>(
        static_cast<IPhysicsNode*>(object_detail::LayoutAccess::physicsNode(this)));   // +0x44
    if (!node)
    {
        Debug(DEBUGTYPE_6, "Game/Objects/Vehicle/Floatingbundle.cpp", 93) << "No physicsNodeComp!";
        return;
    }
    if (m_parent)                                                                    // +0x28
    {
        const RotationalBundleTemplate* t = static_cast<const RotationalBundleTemplate*>(m_template);
        if (t->m_maxRotation.y > 0.0f)                                               // template +0x18C
        {
            calculateAndClipAngle(t, 1, dt);                                         // [FRONTIER]
            m_floaterAngle = -m_currentRotation.y;                                   // +0x144 sign flipped
        }
        // `call [edx+5Ch]` = IObject::queryComponent (slot 23), then 0x82A65F7
        // `mov eax,[eax] / mov [esp],edx / call [eax+3Ch]` - the returned pointer is re-used as
        // `this` with NO adjustment, because Object +0x3C already stores a
        // queryInterface(IID_IArmorGLComp) result (same reading as SpawnPoint.cpp's +0x3C walk).
        // +0x3C on that sub-table is slot 15, getHitPoints (IArmorGLComp.h) - an x87 float return
        // the ELF then tests `fldz / fucomip / jb`, i.e. skip while 0 < hitPoints.
        IArmorGLComp* armor = static_cast<IArmorGLComp*>(
            m_parent->queryComponent(IID_IArmorGLComp, IID_IArmorGLComp));           // slot 23
        if (armor && armor->getHitPoints() <= 0.0f)                                  // +0x3C
            m_floaterAngle = -50.0f;
    }
    node->m_floaterValue = m_floaterAngle;                                           // node +0x234
}

// see the header: nothing with a node, the Debug without one.
void FloatingBundle::handleMessage(TemplateMessage, IPlayer*, bool)
{
    if (!object_detail::LayoutAccess::physicsNode(this))
        Debug(DEBUGTYPE_6, "Game/Objects/Vehicle/Floatingbundle.cpp", 129) << "No pshyicsNodeComp!";
}

} // namespace world
} // namespace hfe
} // namespace dice
