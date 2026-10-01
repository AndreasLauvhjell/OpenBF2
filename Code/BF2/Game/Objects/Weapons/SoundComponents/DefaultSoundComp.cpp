// Path:     attested. Game/Objects/Weapons/SoundComponents/DefaultSoundComp.cpp.
// Contents: recovered from the symbolised Linux ELF (0x8270E90..0x8272FCF) and cross-read at the
// Win32 dedicated-server component/factory boundary. The authored sound names, template
// setup order, reload sample state machine and burst suppression are all retail data.

#include "DefaultSoundComp.h"

#include <sstream>
#include <string>

#include "../GenericFireArm.h"
#include "../../Base/SimpleObjectTemplate.h"
#include "../../GenericFireArmTemplate.h"
#include "../../ICameraObject.h"
#include "../../IObject.h"
#include "../../IObjectTemplate.h"
#include "../../IPlayer.h"
#include "../../../../Core/Debug.h"
#include "../../../../Core/Ids.h"
#include "../../../../Core/System.h"
#include "../../../../IO/Stream/IStream.h"
#include "../../../../Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../../../../Sound/BfSound.h"
#include "../../../../Sound/NullSound.h"
#include "../../../../Sound/ObjectSoundNode.h"
#include "../../../../Sound/SoundObjectTemplate.h"

namespace dice {
namespace hfe {
namespace world {

namespace {

const char* const kSoundNames[DefaultSoundComp::SoundCount] =
{
    "Fire1P",
    "Fire1P_Outdoor",
    "Fire3P",
    "BoltClick",
    "TriggerClick",
    "SwitchFireRate",
    "Reload1P",
    "Reload3P",
    "Deploy1P",
    "Deploy3P",
    "Zoom"
};

SoundObjectTemplate* addSound(SimpleObjectTemplate* objectTemplate, const char* name)
{
    return static_cast<SoundObjectTemplate*>(
        objectTemplate->addSoundTemplate(std::string(name)));
}

} // namespace

DefaultSoundComp::DefaultSoundComp(DefaultSoundCompTemplate* objectTemplate)
    : m_refCount(1)
    , m_template(objectTemplate)
    , m_alternateParent(0)
    , m_soundNode(0)
    , m_soundsUntilNextPlay(0)
    , m_reloadSampleState(0)
    , m_playBoltClick(true)
    , m_lowRoundsPerMinute(false)
    , m_continuousFirePlaying(false)
    , m_hadFourSampleStart(false)
    , m_enabled(false)
    , m_lastEnableTick(0)
{
    // WeaponComp has no emitted constructor. Retail seeds its base reference count here and leaves
    // both attachment pointers untouched until Object::setComponent drives setObject/init.
    WeaponComp::m_refCount = 1;

    for (int i = 0; i < SoundCount; ++i)
        m_sounds[i] = 0;
}

DefaultSoundComp::~DefaultSoundComp()
{
    for (int i = 0; i < SoundCount; ++i)
    {
        if (m_sounds[i])
            m_sounds[i]->release();
    }
    delete m_soundNode;
}

void DefaultSoundComp::addRef() { ++m_refCount; }
int DefaultSoundComp::getRef() { return m_refCount; }

int DefaultSoundComp::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        delete this;
    return remaining;
}

void* DefaultSoundComp::queryInterface(unsigned int iid) const
{
    DefaultSoundComp* const self = const_cast<DefaultSoundComp*>(this);
    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return static_cast<IObjectComponent*>(self);
    if (iid == IID_IWeaponComp || iid == IID_IWeaponSoundComp)
        return static_cast<IWeaponSoundComp*>(self);
    return 0;
}

unsigned int DefaultSoundComp::getInterfaceID() const
{
    return IID_IWeaponSoundComp;
}

void DefaultSoundComp::initComponent(WeaponInitEvent& event)
{
    delete m_soundNode;
    m_soundNode = new ObjectSoundNode(m_initObject);
    m_lowRoundsPerMinute = event.roundsPerMinute <= 300;

    const SimpleObjectTemplate* const objectTemplate = event.firearmTemplate;
    for (int i = 0; i < SoundCount; ++i)
    {
        m_sounds[i] = static_cast<ISound*>(objectTemplate->createSoundFromTemplate(
            std::string(kSoundNames[i]), m_initObject));
    }

    // A missing indoor 1P sample promotes the outdoor sample into its slot. Retail clears the
    // donor so destruction still releases the sound exactly once.
    if (!m_sounds[Fire1P])
    {
        m_sounds[Fire1P] = m_sounds[Fire1POutdoor];
        m_sounds[Fire1POutdoor] = 0;
    }

    for (int i = 0; i < SoundCount; ++i)
    {
        if (m_sounds[i])
            m_sounds[i]->setParent(m_soundNode);
    }
}

bool DefaultSoundComp::preUpdate() { return false; }
void DefaultSoundComp::updateComponent(float, unsigned int) {}
bool DefaultSoundComp::isSelectable() const { return true; }

void DefaultSoundComp::fireStop(bool)
{
    if (!m_continuousFirePlaying)
        return;

    if (m_sounds[Fire1POutdoor])
        m_sounds[Fire1POutdoor]->stop();
    if (m_sounds[Fire1P])
        m_sounds[Fire1P]->stop();
    if (m_sounds[Fire3P])
        m_sounds[Fire3P]->stop();
    m_continuousFirePlaying = false;
}

bool DefaultSoundComp::requestFire(bool, bool alternate)
{
    if (alternate)
        fireStop(alternate);
    return true;
}

void DefaultSoundComp::postFire(WeaponFireEvent& event)
{
    if (m_soundsUntilNextPlay > 0)
    {
        --m_soundsUntilNextPlay;
        return;
    }
    if (m_continuousFirePlaying)
        return;

    Vec3 position = Vec3::getZero();
    IPlayer* const player = firearm()->getPlayer();
    if (player)
    {
        ICameraObject* const camera = player->getCamera();
        IObject* const cameraObject = camera
            ? static_cast<IObject*>(camera->queryInterface(IID_IObject))
            : 0;
        // Retail checks the camera before queryInterface but then relies on the returned IObject
        // unconditionally. A player with no object-backed camera violates this path's invariant.
        position = cameraObject->getAbsolutePosition();
    }

    ISound* sound = m_sounds[Fire3P];
    if ((m_sounds[Fire1P] || m_sounds[Fire1POutdoor]) && is1P())
    {
        sound = (!m_sounds[Fire1POutdoor] || !g_bfSound->isOutdoors(position))
            ? m_sounds[Fire1P]
            : m_sounds[Fire1POutdoor];
    }

    if (!sound)
        return;

    int sampleCount = 0;
    sound->getTemplate()->getFilenames(&sampleCount);
    if (sampleCount == 1)
    {
        const bool continuous = static_cast<int>(event.fireRate) == 2;
        m_continuousFirePlaying = continuous;

        int playCount = event.maxProjectilesToFire;
        if (continuous)
        {
            playCount = 0;
        }
        else if (playCount > 1)
        {
            if (m_lowRoundsPerMinute)
                playCount = 1;
            else
                m_soundsUntilNextPlay = playCount - 1;
        }
        sound->play(playCount);
    }
    else if (sampleCount == 4)
    {
        int sample = 0;
        if (event.componentState)
        {
            m_hadFourSampleStart = true;
            sample = 1;
        }
        else
        {
            if (m_hadFourSampleStart)
                sample = event.fireStrength >= 0.5f ? 3 : 2;
            m_hadFourSampleStart = false;
        }
        sound->playSample(sample, 1);
    }
}

void DefaultSoundComp::fireComplete(unsigned int) {}

void DefaultSoundComp::postSwitchFireRate(FireRate)
{
    if (is1P() && m_sounds[SwitchFireRate])
        m_sounds[SwitchFireRate]->play();
    m_soundsUntilNextPlay = 0;
}

bool DefaultSoundComp::requestReload(bool) { return true; }

void DefaultSoundComp::reload(ReloadState state)
{
    const int stateValue = static_cast<int>(state);
    if (stateValue == 2)
        return;

    ISound* const sound = m_sounds[is1P() ? Reload1P : Reload3P];
    if (sound)
    {
        int sampleCount = 0;
        sound->getTemplate()->getFilenames(&sampleCount);

        if (stateValue == 7)
        {
            if (sampleCount == 2)
                sound->playSample(1, 1);
        }
        else if (sampleCount == 1 || sampleCount == 2)
        {
            if (!sound->isPlaying())
                sound->playSample(0, 1);
        }
        else if (sampleCount == 3)
        {
            int sample = -1;
            switch (stateValue)
            {
            case 3:
                break;
            case 4:
                m_reloadSampleState = 1;
                sample = 0;
                break;
            case 5:
                sample = 1;
                break;
            case 6:
                sample = m_reloadSampleState != 0 ? 1 : 0;
                m_reloadSampleState = 2;
                break;
            default:
                Debug(DEBUGTYPE_6,
                      "Game/Objects/Weapons/SoundComponents/DefaultSoundComp.cpp", 265, "")
                    << "Unhandled reload state";
                break;
            }

            if (sample >= 0)
                sound->playSample(sample, 1);
        }
    }

    m_playBoltClick = true;
    m_soundsUntilNextPlay = 0;
}

void DefaultSoundComp::postReload()
{
    if (m_reloadSampleState != 2)
        return;

    ISound* const sound = m_sounds[is1P() ? Reload1P : Reload3P];
    if (sound)
    {
        int sampleCount = 0;
        sound->getTemplate()->getFilenames(&sampleCount);
        if (sampleCount == 3)
            sound->playSample(2, 1);
    }
    m_reloadSampleState = 0;
}

void DefaultSoundComp::outOfAmmo(bool triggerClick)
{
    stop();
    if (is1P())
    {
        ISound* sound = 0;
        if (m_playBoltClick)
            sound = m_sounds[BoltClick];
        else if (triggerClick)
            sound = m_sounds[TriggerClick];

        if (sound)
            sound->play();
    }
    m_playBoltClick = false;
}

void DefaultSoundComp::stoppedTracking() {}
void DefaultSoundComp::postChangeTarget(IObject*) {}
void DefaultSoundComp::projectileWasReset(IObject*) {}

void DefaultSoundComp::enable()
{
    if (m_enabled)
        return;

    m_enabled = true;
    if (!isBackstepping())
    {
        if (WorldPref::getTickCount() - m_lastEnableTick > 9)
        {
            ISound* const deploy = m_sounds[is1P() ? Deploy1P : Deploy3P];
            if (deploy)
                deploy->play();
        }
        // The two reads are distinct in retail; preserve a tick boundary between them.
        m_lastEnableTick = WorldPref::getTickCount();
    }

    m_soundsUntilNextPlay = 0;
    m_hadFourSampleStart = false;
    m_reloadSampleState = 0;
    m_continuousFirePlaying = false;
    m_playBoltClick = false;
}

void DefaultSoundComp::disable()
{
    if (!m_enabled)
        return;

    m_enabled = false;
    stop();

    if (m_sounds[Reload1P])
        m_sounds[Reload1P]->forceStop();
    if (m_sounds[Reload3P])
        m_sounds[Reload3P]->forceStop();

    if (!isBackstepping())
    {
        if (m_sounds[Deploy1P])
            m_sounds[Deploy1P]->forceStop();
        if (m_sounds[Deploy3P])
            m_sounds[Deploy3P]->forceStop();
    }

    if (m_sounds[Zoom])
        m_sounds[Zoom]->forceStop();
}

void DefaultSoundComp::stop() { fireStop(false); }
bool DefaultSoundComp::busy() { return false; }
void DefaultSoundComp::exit() {}
void DefaultSoundComp::destroy() {}
void DefaultSoundComp::pickup() {}
void DefaultSoundComp::reset() {}

void DefaultSoundComp::toggleCamera()
{
    if (is1P() && m_sounds[Zoom])
        m_sounds[Zoom]->play();
}

void DefaultSoundComp::setAlternateParent(GenericFireArm* firearmObject)
{
    m_alternateParent = firearmObject;
}

bool DefaultSoundComp::getNoisy() const
{
    return m_template->getNoisy();
}

int DefaultSoundComp::getMaxSoundsPerBurst() const
{
    return m_template->getMaxSoundsPerBurst();
}

GenericFireArm* DefaultSoundComp::firearm() const
{
    return m_alternateParent
        ? m_alternateParent
        : static_cast<GenericFireArm*>(m_initObject);
}

bool DefaultSoundComp::is1P()
{
    IPlayer* const player = firearm()->getPlayerByForce();
    return player && player->hasFirstPersonView();
}

bool DefaultSoundComp::isBackstepping() const
{
    IPlayer* const player = firearm()->getPlayerByForce();
    return player && player->testFlags(g_playerFlagBit3);
}

DefaultSoundCompTemplate::DefaultSoundCompTemplate()
    : m_refCount(1)
    , m_noisy(true)
    , m_maxSoundsPerBurst(-1)
{
}

void DefaultSoundCompTemplate::addRef() { ++m_refCount; }
int DefaultSoundCompTemplate::getRef() { return m_refCount; }

int DefaultSoundCompTemplate::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        ::operator delete(this);
    return remaining;
}

void* DefaultSoundCompTemplate::queryInterface(unsigned int iid) const
{
    DefaultSoundCompTemplate* const self = const_cast<DefaultSoundCompTemplate*>(this);
    if (iid == IID_IBase
        || iid == IID_IObjectTemplateComponent
        || iid == IID_IWeaponCompTemplate)
    {
        return static_cast<IWeaponCompTemplate*>(self);
    }
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int DefaultSoundCompTemplate::getClassID() const
{
    return CID_DefaultSoundCompTemplate;
}

unsigned int DefaultSoundCompTemplate::getInterfaceID()
{
    return IID_IWeaponSoundCompTemplate;
}

IObjectComponent* DefaultSoundCompTemplate::createComponent()
{
    return new DefaultSoundComp(this);
}

void DefaultSoundCompTemplate::objectCreated(IObject*) {}
void DefaultSoundCompTemplate::objectDestroyed(IObject*) {}

void DefaultSoundCompTemplate::setTemplate(IObjectTemplate* objectTemplate)
{
    initTemplates(static_cast<SimpleObjectTemplate*>(objectTemplate), 0u);
}

void DefaultSoundCompTemplate::initTemplates(
    SimpleObjectTemplate* objectTemplate, unsigned int flags)
{
    IObjectTemplate* const active = objectTemplateManager->getActiveObjectTemplate();

    SoundObjectTemplate* sound = addSound(objectTemplate, kSoundNames[DefaultSoundComp::Fire1P]);
    sound->impl()->setIs3dSound(false);
    sound->impl()->setLoopCount(0);

    if ((flags & 2u) == 0)
    {
        sound = addSound(objectTemplate, kSoundNames[DefaultSoundComp::Fire1POutdoor]);
        sound->impl()->setIs3dSound(false);
        sound->impl()->setLoopCount(0);
    }

    sound = addSound(objectTemplate, kSoundNames[DefaultSoundComp::Fire3P]);
    sound->impl()->setIs3dSound(true);
    sound->impl()->setMinDistance(2.0f);
    sound->impl()->setLoopCount(0);

    if ((flags & 1u) == 0)
    {
        sound = addSound(objectTemplate, kSoundNames[DefaultSoundComp::BoltClick]);
        sound->impl()->setIs3dSound(false);
        sound->impl()->setLoopCount(1);
        sound->impl()->setReverbLevel(0.5f);

        sound = addSound(objectTemplate, kSoundNames[DefaultSoundComp::TriggerClick]);
        sound->impl()->setIs3dSound(false);
        sound->impl()->setLoopCount(1);
        sound->impl()->setReverbLevel(0.3f);

        sound = addSound(objectTemplate, kSoundNames[DefaultSoundComp::SwitchFireRate]);
        sound->impl()->setIs3dSound(false);
        sound->impl()->setLoopCount(1);
        sound->impl()->setReverbLevel(0.3f);
    }

    sound = addSound(objectTemplate, kSoundNames[DefaultSoundComp::Reload1P]);
    sound->impl()->setIs3dSound(false);
    sound->impl()->setLoopCount(1);

    sound = addSound(objectTemplate, kSoundNames[DefaultSoundComp::Reload3P]);
    sound->impl()->setIs3dSound(true);
    sound->impl()->setLoopCount(1);
    sound->impl()->setMinDistance(0.5f);

    sound = addSound(objectTemplate, kSoundNames[DefaultSoundComp::Deploy1P]);
    sound->impl()->setIs3dSound(false);
    sound->impl()->setLoopCount(1);
    sound->impl()->setStopType(0);

    sound = addSound(objectTemplate, kSoundNames[DefaultSoundComp::Deploy3P]);
    sound->impl()->setIs3dSound(true);
    sound->impl()->setLoopCount(1);
    sound->impl()->setMinDistance(0.5f);
    sound->impl()->setStopType(0);

    sound = addSound(objectTemplate, kSoundNames[DefaultSoundComp::Zoom]);
    sound->impl()->setIs3dSound(false);
    sound->impl()->setLoopCount(1);

    objectTemplateManager->setActiveObjectTemplate(active);
}

void* DefaultSoundCompTemplate::queryImplementation(unsigned int classId)
{
    return classId == CID_DefaultSoundCompTemplate ? this : 0;
}

bool DefaultSoundCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (!m_noisy)
        script << "ObjectTemplate.sound.noisy " << m_noisy << std::endl;
    if (m_maxSoundsPerBurst != -1)
        script << "ObjectTemplate.sound.maxSoundsPerBurst " << m_maxSoundsPerBurst << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
