// Wing and floating-bundle template behavior.

#include "RotationalBundleLeaves.h"
#include "../../IO/Stream/IStream.h"
#include <sstream>
#include <cmath>

#include "../../Core/Ids.h"
#include "../../Physics/PhysicsFloatingBundle.h"   // FloatingBundleTemplate::setPhysicsNodeComponent
#include "../../Physics/PhysicsWing.h"             // WingTemplate::setPhysicsNodeComponent
#include "Game/Wing.h"                           // WingTemplate::createObject product
#include "../../Physics/PhysicsNodeManager.h"     // physicsNodeManager->addPhysicsNode
#include "Game/FloatingBundle.h"                 // FloatingBundleTemplate::createObject product

namespace dice {
namespace hfe {
namespace world {

extern PhysicsNodeManager* physicsNodeManager;   // ds:0x8B27844 - Core/Globals.cpp

// WingTemplate

// the base, the three vptr stores, then nine own fields. No updateFlags.
WingTemplate::WingTemplate()
{
    m_wingLift = 1.0f;
    m_flapLift = 0.1f;
    m_pitchOffset = 0.0f;
    m_positionOffset.x = 0.0f; m_positionOffset.y = 0.0f; m_positionOffset.z = 0.0f;
    m_liftRegulated = 0;
    m_regulateToLift = 4.91f;
    m_wingToRegulatorRatio = 1.0f;
    m_startDelay = 0.0f;
}

WingTemplate::~WingTemplate() {}

// enabling regulation is a SET side effect, even for zero.
void WingTemplate::setRegulateToLift(float value)
{
    m_liftRegulated = true;
    m_regulateToLift = value;
}

// unlike setRegulateToLift, this does not enable regulation.
void WingTemplate::setWingToRegulatorRatio(float value)
{
    m_wingToRegulatorRatio = value;
}

unsigned int WingTemplate::getClassID() const { return CID_WingTemplate; }

void* WingTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_WingTemplate)
        return this;
    return RotationalBundleTemplate::queryImplementation(classId);
}

// Creates Wing; physics setup attaches and registers its node without changing sleep state.
IObject* WingTemplate::createObject() const { return new Wing(this); }

void WingTemplate::setPhysicsNodeComponent(IObject* object) const
{
    PhysicsWing* node = new PhysicsWing(this, object);
    node->setObject(object);                                                    // +0x28
    object->setComponent(IID_IPhysicsNode, node);                               // slot 22
    node->release();                                                            // +0x08
    physicsNodeManager->addPhysicsNode(node);                                   // +0x1C
}

// FloatingBundleTemplate

// the base, the three vptr stores, then six own fields. No updateFlags.
FloatingBundleTemplate::FloatingBundleTemplate()
{
    m_waterHeight = 0.0f;
    m_hullHeight = 1.0f;
    m_floatMaxLift = 10.0f;
    m_floatMinLift = 10.0f;
    m_dragModifier = 400.0f;
    m_sinkingSpeedMod = 1.0f;
}

FloatingBundleTemplate::~FloatingBundleTemplate() {}

unsigned int FloatingBundleTemplate::getClassID() const { return CID_FloatingBundleTemplate; }

void* FloatingBundleTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_FloatingBundleTemplate)
        return this;
    return RotationalBundleTemplate::queryImplementation(classId);
}

// Creates FloatingBundle; physics setup attaches and registers its node without changing sleep state.
IObject* FloatingBundleTemplate::createObject() const { return new FloatingBundle(this); }

void FloatingBundleTemplate::setPhysicsNodeComponent(IObject* object) const
{
    PhysicsFloatingBundle* node = new PhysicsFloatingBundle(this, object);
    node->setObject(object);                                                    // +0x28
    object->setComponent(IID_IPhysicsNode, node);                               // slot 22
    node->release();                                                            // +0x08
    physicsNodeManager->addPhysicsNode(node);                                   // +0x1C
}

// ELF 82A6A90 / dedicated PE 60AB60: float epsilon comparisons, including NaN suppression.
bool FloatingBundleTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream out;
    if (!RotationalBundleTemplate::makeScript(stream)) return false;
    if (std::fabs(m_waterHeight - 0.0f) > 1.1920929e-7f)
        out << "ObjectTemplate.setWaterHeight " << m_waterHeight << std::endl;
    if (std::fabs(m_hullHeight - 1.0f) > 1.1920929e-7f)
        out << "ObjectTemplate.setHullHeight " << m_hullHeight << std::endl;
    if (std::fabs(m_floatMaxLift - 10.0f) > 1.1920929e-7f)
        out << "ObjectTemplate.setFloatMaxLift " << m_floatMaxLift << std::endl;
    if (std::fabs(m_floatMinLift - 10.0f) > 1.1920929e-7f)
        out << "ObjectTemplate.setFloatMinLift " << m_floatMinLift << std::endl;
    if (std::fabs(m_dragModifier - 400.0f) > 1.1920929e-7f)
        out << "ObjectTemplate.setDragModifier " << m_dragModifier << std::endl;
    if (std::fabs(m_sinkingSpeedMod - 1.0f) > 1.1920929e-7f)
        out << "ObjectTemplate.setSinkingSpeedMod " << m_sinkingSpeedMod << std::endl;
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

// ELF 82AF820 / dedicated PE 60B310: positionOffset is unconditional.
bool WingTemplate::makeScript(io::IStream* stream)
{
    if (!RotationalBundleTemplate::makeScript(stream)) return false;
    std::ostringstream out;
    if (m_pitchOffset != 0.0f) out << "ObjectTemplate.setPitchOffset " << m_pitchOffset << std::endl;
    out << "ObjectTemplate.setPositionOffset " << m_positionOffset.x << '/' << m_positionOffset.y << '/' << m_positionOffset.z << std::endl;
    if (std::fabs(m_wingLift - 1.0f) > 1.1920929e-7f) out << "ObjectTemplate.setWingLift " << m_wingLift << std::endl;
    if (std::fabs(m_flapLift - 0.1f) > 1.1920929e-7f) out << "ObjectTemplate.setFlapLift " << m_flapLift << std::endl;
    if (m_liftRegulated) out << "ObjectTemplate.setLiftRegulated " << (m_liftRegulated != 0) << std::endl;
    if (std::fabs(m_regulateToLift - 4.91f) > 1.1920929e-7f) out << "ObjectTemplate.setRegulateToLift " << m_regulateToLift << std::endl;
    if (std::fabs(m_wingToRegulatorRatio - 1.0f) > 1.1920929e-7f) out << "ObjectTemplate.setWingToRegulatorRatio " << m_wingToRegulatorRatio << std::endl;
    if (m_startDelay != 0.0f) out << "ObjectTemplate.startDelay " << m_startDelay << std::endl;
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

} // namespace world
} // namespace hfe
} // namespace dice
