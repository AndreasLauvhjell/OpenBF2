// Path:     [INFERRED] beside the attested FireComp.cpp. LPFireComp emits no independent __FILE__
// string in either retail image.
// Contents: complete LPFireComp runtime/template family, recovered from bf2.i64
// (0x8266A10..0x826799F) and cross-checked in bf2_w32ded.exe.

#include "LPFireComp.h"

#include <sstream>
#include <string>

#include "../GenericFireArm.h"
#include "../../IObjectComponent.h"
#include "../../IPlayer.h"
#include "../../../IGame.h"
#include "../../../../Core/Ids.h"
#include "../../../../Core/System.h"
#include "../../../../IO/Stream/IStream.h"
#include "../../../../Scene/Object/PlayerManager.h"

namespace dice {
namespace hfe {
namespace world {

// ELF C2/C1 0x8266A10/0x8266AA0. All three Vec3 objects are explicitly zeroed; the first has no
// recovered reader but occupies real state between the radio latch and the painted line.
LPFireComp::LPFireComp(LPFireCompTemplate* objectTemplate)
    : FireComp(objectTemplate)
    , m_refCount(1)
    , m_radioMessageStartedAtTick(0)
    , m_radioMessagePending(false)
    , m_unusedPosition{0.0f, 0.0f, 0.0f}
    , m_startPosition{0.0f, 0.0f, 0.0f}
    , m_endPosition{0.0f, 0.0f, 0.0f}
    , m_lpTemplate(objectTemplate)
{
}

LPFireComp::~LPFireComp() {}

void LPFireComp::addRef() { ++m_refCount; }
int LPFireComp::getRef() { return m_refCount; }

int LPFireComp::release()
{
    const int remaining = m_refCount - 1;
    const bool last = m_refCount == 1;
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

void* LPFireComp::queryInterface(unsigned int iid) const
{
    LPFireComp* const self = const_cast<LPFireComp*>(this);
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return static_cast<IObjectComponent*>(self);
    if (iid == IID_IWeaponComp || iid == IID_IWeaponFireComp)
        return static_cast<IWeaponFireComp*>(self);
    return 0;
}

unsigned int LPFireComp::getInterfaceID() const
{
    return IID_IWeaponFireComp;
}

// . A blocked-update flag leaves the pending message armed. The original repeats the
// local-player lookup for the radio call after using a first lookup in the comparison.
void LPFireComp::updateComponent(float, unsigned int flags)
{
    IPlayer* const player = firearm()->getPlayerByForce();
    if (m_radioMessagePending && !(flags & 4))
    {
        const unsigned int readyAt = m_radioMessageStartedAtTick
            + static_cast<unsigned int>(m_lpTemplate->getRadioMessageDelayInTicks());
        if (readyAt < static_cast<unsigned int>(WorldPref::getTickCount()))
        {
            if (player == playerManager->getLocalHumanPlayer())
            {
                game->radioMessage(
                    TCLocalAirVehicle,
                    "laserpaint",
                    playerManager->getLocalHumanPlayer(),
                    1,
                    true,
                    false);
            }
            m_radioMessagePending = false;
        }
    }
}

bool LPFireComp::requestFire(bool released, bool)
{
    m_fireStartDelay = !released;
    return true;
}

int LPFireComp::fire(bool, int)
{
    if (m_fireStartDelay)
    {
        m_radioMessagePending = true;
        m_radioMessageStartedAtTick = static_cast<unsigned int>(WorldPref::getTickCount());
    }

    m_firedProjectiles.clear();
    launchProjectile(-1);
    m_firedProjectiles.push_back(0);
    setFireState(static_cast<FireState>(4));
    return 1;
}

void LPFireComp::fireStop(bool)
{
    setFireState(static_cast<FireState>(0));
    m_radioMessagePending = false;
    m_signalComplete = true;
}

// . The player lookup is a retained side effect. The painted segment begins at the
// projectile-origin translation and extends exactly 100 units down its forward row.
void LPFireComp::launchProjectile(int)
{
    firearm()->getPlayerByForce();
    const Mat4& origin = getProjectileOrigin();

    m_startPosition.x = origin.m[12];
    m_startPosition.y = origin.m[13];
    m_startPosition.z = origin.m[14];

    m_endPosition.x = origin.m[12] + origin.m[8] * 100.0f;
    m_endPosition.y = origin.m[13] + origin.m[9] * 100.0f;
    m_endPosition.z = origin.m[14] + origin.m[10] * 100.0f;
    m_fireAtTick = static_cast<unsigned int>(WorldPref::getTickCount());
}

bool LPFireComp::busy() { return false; }
Vec3 LPFireComp::getStartPos() const { return m_startPosition; }
Vec3 LPFireComp::getEndPos() const { return m_endPosition; }
bool LPFireComp::launchesProjectiles(int) const { return true; }
std::vector<IObject*>* LPFireComp::getFiredProjectiles() { return &m_firedProjectiles; }
void LPFireComp::postReload() {}

// Template

LPFireCompTemplate::LPFireCompTemplate()
    : FireCompTemplate()
    , m_refCount(1)
{
    // ELF calls this out-of-line setter even for the default, preserving the cached conversion.
    setRadioMessageDelay(0.0f);
}

LPFireCompTemplate::~LPFireCompTemplate() {}

void LPFireCompTemplate::addRef() { ++m_refCount; }
int LPFireCompTemplate::getRef() { return m_refCount; }

int LPFireCompTemplate::release()
{
    const int remaining = m_refCount - 1;
    const bool last = m_refCount == 1;
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

void* LPFireCompTemplate::queryInterface(unsigned int iid) const
{
    LPFireCompTemplate* const self = const_cast<LPFireCompTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent || iid == IID_IWeaponCompTemplate)
        return static_cast<IWeaponCompTemplate*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int LPFireCompTemplate::getClassID() const { return CID_LPFireCompTemplate; }

IObjectComponent* LPFireCompTemplate::createComponent()
{
    return new LPFireComp(this);
}

void* LPFireCompTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_LPFireCompTemplate || classId == CID_FireCompTemplate)
        return this;
    return 0;
}

void LPFireCompTemplate::setRadioMessageDelay(float seconds)
{
    m_radioMessageDelay = seconds;
    m_radioMessageDelayTimes30 = static_cast<int>(seconds * 30.0f);
}

// . Unlike most optional leaf properties, radioMessageDelay is emitted
// even at its zero default. The second stream performs one final write after the base writer.
bool LPFireCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (!FireCompTemplate::makeScript(stream))
        return false;

    script << "ObjectTemplate.fire.radioMessageDelay " << m_radioMessageDelay << std::endl;
    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
