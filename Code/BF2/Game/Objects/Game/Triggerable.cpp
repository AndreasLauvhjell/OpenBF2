// [INFERRED] path beside Trigger.cpp; both dedicated images contain this runtime pair.
// ELF81C4E60..81C5CDC / PE545E20,544810,5449D0,5450E0,5464B0.
#include "Triggerable.h"
#include "TriggerableManager.h"
#include "../TemplateLeaves.h"
#include "../IPlayer.h"
#include "../../../Core/Ids.h"
#include "../../../Core/System.h"
#include "../../../IO/Input/PlayerInput.h"
#include "../../../Scene/Object/PlayerManager.h"
#include "../../../Scene/Object/ObjectManager.h"
#include "../../../Sound/NullSound.h"
#include <cmath>

namespace dice { namespace hfe { namespace world {
namespace {
// ELF8AADC94 / PE884ED8: process-lifetime counter, not reset by the manager.
int nextTriggerableId = 0;
float distance(const Vec3& a, const Vec3& b)
{
    const float x = b.x - a.x, y = b.y - a.y, z = b.z - a.z;
    return std::sqrt(x*x + y*y + z*z);
}
bool usePressed(const io::PlayerInput& input)
{
    return (input.mappingFlags & (1ULL << 10)) != 0 && input.inputs[10] > 0.5f;
}
}

Triggerable::Triggerable(const TriggerableTemplate* objectTemplate)
    : Bundle(objectTemplate), m_refCount(1), m_template(objectTemplate),
      m_id(nextTriggerableId++), m_float14C(5.0f),
      m_startupSound(0), m_finishedSound(0), m_bool16C(false)
{
    // The two tick words at +150/+154 are intentionally not initialized by retail.
    updateFlags(0x4000, 0);
    m_startupSound = static_cast<ISound*>(objectTemplate->createSoundFromTemplate("Startup", this));
    if (m_startupSound) m_startupSound->setParent(this);
    m_finishedSound = static_cast<ISound*>(objectTemplate->createSoundFromTemplate("Finished", this));
    if (m_finishedSound) m_finishedSound->setParent(this);
    Object::setName(objectTemplate->getName());
    m_layer = 1;
}
Triggerable::~Triggerable()
{
    if (triggerableManager) triggerableManager->removeTriggerable(this);
    if (m_startupSound) m_startupSound->release();
    if (m_finishedSound) m_finishedSound->release();
}
void Triggerable::addRef() { ++m_refCount; }
int Triggerable::getRef() { return m_refCount; }
int Triggerable::release()
{
    const int result = --m_refCount;
    if (!result) delete this;
    return result;
}
void* Triggerable::queryInterface(unsigned int iid) const
{
    Triggerable* self = const_cast<Triggerable*>(this);
    if (iid == IID_IBase || iid == IID_IObject) return static_cast<IObject*>(self);
    if (iid == io::IID_IConsoleSaveable) return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}
void Triggerable::init()
{
    Bundle::init();
    m_createdOnLoad = true;
    if (triggerableManager) triggerableManager->addTriggerable(this);
}
void Triggerable::destroy(bool value) { SimpleObject::destroy(value); }
void Triggerable::handleMessage(TemplateMessage message, IPlayer* player, bool recursive)
{
    Bundle::handleMessage(message, player, recursive);
}
const Mat4& Triggerable::getTransform() { return getAbsoluteTransformation(); }
const Vec3* Triggerable::getVelocity() { return 0; }
float Triggerable::getRadius() const { return m_template->m_radius; }

bool Triggerable::checkTriggerableArea()
{
    const std::list<IPlayer*>& players = playerManager->getPlayers();
    for (auto it = players.begin(); it != players.end(); ++it)
    {
        IPlayer* player = *it;
        if (!player->getIsAlive()) continue;
        IObject* vehicle = player->getDefaultVehicle();
        if (!vehicle) continue;
        if (vehicle->getParent() && getRootParent(vehicle) != getRootParent(this)) continue;
#if defined(_MSC_VER)
        const Vec3& vehiclePosition = vehicle->getAbsolutePosition();
        const Vec3& position = getAbsolutePosition();
#else
        const Vec3& position = getAbsolutePosition();
        const Vec3& vehiclePosition = vehicle->getAbsolutePosition();
#endif
        if (distance(position, vehiclePosition) > m_template->m_radius) continue;
        if (getTypes() == 1)
        {
            const io::PlayerInput input = player->getPlayerInput();
            if (!usePressed(input))
            {
                if (!player->getIsAIPlayer()) continue;
                if (!usePressed(*static_cast<const io::PlayerInput*>(player->getAIPlayerInput()))) continue;
            }
        }
        else if (getTypes() != 3) continue;
        playActivateSound();
        return true;
    }
    return false;
}
bool Triggerable::isOccupied()
{
    const std::list<IPlayer*>& players = playerManager->getPlayers();
    for (auto it = players.begin(); it != players.end(); ++it)
    {
        IPlayer* player = *it;
        if (!player->getIsAlive()) continue;
        IObject* vehicle = player->getDefaultVehicle();
        if (!vehicle) continue;
        if (vehicle->getParent() && getRootParent(vehicle) != getRootParent(this)) continue;
#if defined(_MSC_VER)
        const Vec3& vehiclePosition = vehicle->getAbsolutePosition();
        const Vec3& position = getAbsolutePosition();
#else
        const Vec3& position = getAbsolutePosition();
        const Vec3& vehiclePosition = vehicle->getAbsolutePosition();
#endif
        if (distance(position, vehiclePosition) <= m_template->m_radius) return true;
    }
    return false;
}
bool Triggerable::isInArea(const Vec3& position)
{
    return distance(getAbsolutePosition(), position) < m_template->m_radius;
}
void Triggerable::updateOnTime(float deltaTime)
{
    if (!m_unresolved150[0]) m_unresolved150[0] = WorldPref::getTickCount();
    const float elapsedTicks = static_cast<float>(WorldPref::getTickCount() - m_unresolved150[1]);
    if (m_template->m_float198 <= WorldPref::getTickTime() * elapsedTicks)
    {
        // Retail repeats the whole target update for each entry; the stored pointer is unused.
        for (auto it = m_objectsInArea.begin(); it != m_objectsInArea.end(); ++it)
            handleTrigger(deltaTime);
        m_unresolved150[0] = WorldPref::getTickCount();
    }
}
bool Triggerable::handleTrigger(float deltaTime)
{
    bool finished = true;
    TriggerableManager* manager = triggerableManager;
    for (auto it = m_template->m_targetNames.begin(); it != m_template->m_targetNames.end(); ++it)
    {
        TriggerableTarget* target = manager->getTriggerableTargetByName(std::string(it->c_str()));
        if (target)
        {
            target->handleUpdate(target, this, deltaTime);
            if (target->m_state != 5) finished = false;
        }
    }
    if (finished) playDeActivateSound();
    return finished;
}
void Triggerable::resetTriggerTargets()
{
    TriggerableManager* manager = triggerableManager;
    for (auto it = m_template->m_targetNames.begin(); it != m_template->m_targetNames.end(); ++it)
    {
        TriggerableTarget* target = manager->getTriggerableTargetByName(std::string(it->c_str()));
        if (target) target->m_state = 6;
    }
}
void Triggerable::refresh()
{
    TriggerableManager* manager = triggerableManager;
    for (auto it = m_template->m_targetNames.begin(); it != m_template->m_targetNames.end(); ++it)
    {
        TriggerableTarget* target = manager->getTriggerableTargetByName(std::string(it->c_str()));
        if (target) target->refresh();
    }
}
bool Triggerable::allTargetsStopped()
{
    bool stopped = true;
    TriggerableManager* manager = triggerableManager;
    for (auto it = m_template->m_targetNames.begin(); it != m_template->m_targetNames.end(); ++it)
    {
        TriggerableTarget* target = manager->getTriggerableTargetByName(std::string(it->c_str()));
        if (target && static_cast<unsigned int>(target->m_state - 5) > 1u && target->m_state != 1)
            stopped = false;
    }
    return stopped;
}
void Triggerable::playActivateSound() { if (m_startupSound) m_startupSound->play(); }
void Triggerable::playDeActivateSound() { if (m_finishedSound) m_finishedSound->play(); }
int Triggerable::getTargetMovementType()
{
    // Retail dereferences begin even for an empty list; retain its missing guard.
    TriggerableTarget* target = triggerableManager->getTriggerableTargetByName(
        std::string(m_template->m_targetNames.begin()->c_str()));
    return target ? target->m_template->m_movementType : -1;
}
} } }
