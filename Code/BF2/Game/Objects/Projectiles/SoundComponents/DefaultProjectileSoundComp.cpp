// Projectile flight and impact sound handling.

#include "DefaultProjectileSoundComp.h"

#include <sstream>
#include <string>

#include "../ProjectileCompTemplates.h"
#include "../../Base/SimpleObjectTemplate.h"
#include "../../IObject.h"
#include "../../IObjectTemplate.h"
#include "../../../../Core/Ids.h"
#include "../../../../IO/Stream/IStream.h"
#include "../../../../Scene/Object/ObjectTypes/Base/ObjectTemplateManager.h"
#include "../../../../Sound/ISoundNode.h"
#include "../../../../Sound/NullSound.h"
#include "../../../../Sound/SoundObjectTemplate.h"

namespace dice {
namespace hfe {
namespace world {

namespace {

// THE sound name, and it is the only string this class touches. A projectile template with no
// sound registered under "Looping" gets a null sound and every body below becomes a no-op.
const char* const kLoopingSoundName = "Looping";

} // namespace

// Construction / destruction - 0x82923D0 / 0x8292450, 62 B each.
// The destructor releases the sound but does not null it; the object is going away, so the
// shipped code does not bother. Reproduced.
DefaultProjectileSoundComp::DefaultProjectileSoundComp(
        DefaultProjectileSoundCompTemplate* objectTemplate)
    : m_refCount(1)
    , m_sound(0)
    , m_template(objectTemplate)
{
    ProjectileComp::m_refCount = 1;
    m_object = 0;
    m_initObject = 0;
}

DefaultProjectileSoundComp::~DefaultProjectileSoundComp()
{
    if (m_sound)
        m_sound->release();
}

void DefaultProjectileSoundComp::addRef() { ++m_refCount; }
int  DefaultProjectileSoundComp::getRef() { return m_refCount; }

int DefaultProjectileSoundComp::release()
{
    const int remaining = m_refCount - 1;
    const bool last = (m_refCount == 1);
    m_refCount = remaining;
    if (last && this)
        delete this;
    return remaining;
}

// . Two ids to the primary pointer, two to the +0x10 face.
void* DefaultProjectileSoundComp::queryInterface(unsigned int iid) const
{
    DefaultProjectileSoundComp* const self = const_cast<DefaultProjectileSoundComp*>(this);

    if (iid == IID_IBase || iid == IID_IObjectComponent)
        return static_cast<IObjectComponent*>(self);
    if (iid == IID_IProjectileComp || iid == IID_IProjectileSoundComp)
        return static_cast<IProjectileSoundComp*>(self);
    return 0;
}

unsigned int DefaultProjectileSoundComp::getInterfaceID() const
{
    return IID_IProjectileSoundComp;
}

// Both inert in the shipped image. The sound is driven by the engine through the ISoundNode this
// component bound to it, not by a per-tick call from here.
bool DefaultProjectileSoundComp::hasUpdate() const { return false; }
void DefaultProjectileSoundComp::updateComponent(float, unsigned int) {}

// initSoundComp - 0x8292520, 158 B. THE whole class, really: fetch the "Looping" sound off the
// projectile's own template and bind it to the projectile's ISoundNode face.
// THE owner passed TO createSoundFromTemplate IS m_initObject - the projectile - and so is the
// node. Two different uses of the same pointer, one as an owner and one as a motion source, and
// the ELF derives the second by adding the ISoundNode base offset to the first.
// A NULL projectile yields A NULL node, not a crash: the shipped code cmovs the offset away when
// the object pointer is zero. Reproduced by the conditional below.
void DefaultProjectileSoundComp::initSoundComp(const SimpleObjectTemplate* soundTemplate)
{
    const std::string name(kLoopingSoundName);
    m_sound = static_cast<ISound*>(
        soundTemplate->createSoundFromTemplate(name, m_initObject));

    if (m_sound)
    {
        ISoundNode* const node = m_initObject
            ? static_cast<ISoundNode*>(static_cast<GenericProjectile*>(m_initObject))
            : 0;
        m_sound->setParent(node);
    }
}

void DefaultProjectileSoundComp::activate()
{
    if (m_sound)
        m_sound->play();
}

void DefaultProjectileSoundComp::reset()
{
    if (m_sound)
        m_sound->stop();
}

// DefaultProjectileSoundCompTemplate - complete 12-slot template contract.
// ELF: ctor 0x8292600, createComponent 0x8292640, setTemplate 0x8292670, initTemplates
// makeScript 0x8292780, quartet/ids/hooks 0x8293030..0x8293140.
// PE cross-check: initTemplates 0x5B89D0. It reaches the same "Looping" child through the active
// SimpleObjectTemplate and applies the same three ISoundTemplate operations in the same order.

DefaultProjectileSoundCompTemplate::DefaultProjectileSoundCompTemplate()
    : m_refCount(1)
{
}

void DefaultProjectileSoundCompTemplate::addRef() { ++m_refCount; }
int DefaultProjectileSoundCompTemplate::getRef() { return m_refCount; }

int DefaultProjectileSoundCompTemplate::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        ::operator delete(this);
    return remaining;
}

void* DefaultProjectileSoundCompTemplate::queryInterface(unsigned int iid) const
{
    DefaultProjectileSoundCompTemplate* const self =
        const_cast<DefaultProjectileSoundCompTemplate*>(this);
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

unsigned int DefaultProjectileSoundCompTemplate::getClassID() const
{
    return CID_DefaultProjectileSoundCompTemplate;
}

unsigned int DefaultProjectileSoundCompTemplate::getInterfaceID()
{
    return IID_IProjectileSoundCompTemplate;
}

IObjectComponent* DefaultProjectileSoundCompTemplate::createComponent()
{
    return new DefaultProjectileSoundComp(this);
}

void DefaultProjectileSoundCompTemplate::objectCreated(IObject*) {}
void DefaultProjectileSoundCompTemplate::objectDestroyed(IObject*) {}

void DefaultProjectileSoundCompTemplate::setTemplate(IObjectTemplate* objectTemplate)
{
    initTemplates(static_cast<SimpleObjectTemplate*>(objectTemplate), 0u);
}

void DefaultProjectileSoundCompTemplate::initTemplates(
    SimpleObjectTemplate* objectTemplate, unsigned int)
{
    const IObjectTemplate* const active = objectTemplateManager->getActiveObjectTemplate();
    SoundObjectTemplate* const looping = static_cast<SoundObjectTemplate*>(
        objectTemplate->addSoundTemplate(std::string(kLoopingSoundName)));
    looping->impl()->setIs3dSound(true);
    looping->impl()->setLoopCount(0);
    looping->impl()->setMinDistance(0.5f);
    objectTemplateManager->setActiveObjectTemplate(active);
}

void* DefaultProjectileSoundCompTemplate::queryImplementation(unsigned int classId)
{
    return classId == getClassID() ? this : 0;
}

bool DefaultProjectileSoundCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
