// Path:     [INFERRED] - beside its header. The ELF attests this TU as Game/Objects/Fx/Decal.cpp
// (_GLOBAL__N_Game_Objects_Fx_Decal.cpp - see DecalTemplate.h); not moved here.
// Contents: recovered from bf2.i64 and BF2.exe (client 1.50).

#include "DecalTemplate.h"
#include "../../../IO/Stream/IStream.h"
#include "../../../Geom/DecalSystem/IDecalManager.h"   // templateInitialize's addTemplate (+0x1C)
#include <sstream>
#include <string>

#include "../../../Core/ClassManager.h"
#include "../../../Core/Ids.h"

// attested name and scope: ELF 0x8AAE41C `_ZN42_GLOBAL__N_Game_Objects_Fx_Decal.cpptzEZKa13g_emptyStringE`
// - a file-scope, top-level anonymous-namespace std::string that this TU's static initialiser
// (0x81F8DD0) builds from "" and __tcf_1_65 destroys; getDecalTextureName (0x81F89FC) hands it out
// when there is no renderer template. The client keeps the same object at ds:0x9F6D3C (read by
// and by makeScript's inlined copy, 0x647662). [INFERRED] constness.
namespace {
const std::string g_emptyString("");
}

namespace dice {
namespace hfe {

extern ClassManager* classManager;   // ds:0x8AC85A0 - the same local declaration the
                                     // ObjectTemplateManager and LightSourceTemplate TUs carry

namespace world {

// ObjectTemplate base, refcount 1, null handle, updateFlags(0x40, 0).
// Client 0x6473F0: +0x60 = 1, +0x64 = 0, updateFlags(0x40, 0) - the same.
DecalTemplate::DecalTemplate()
{
    m_refCount = 1;                               // +0x30
    m_decal = 0;                                  // +0x34
    updateFlags(0x40, 0);
}

// (D1): vptr resets + ~ObjectTemplate. The renderer template is NOT released here - as read;
// RendDX9's DecalManager owns it.
DecalTemplate::~DecalTemplate()
{
}

// The per-class quartet - refcount +0x30, delete on the 1->0 edge through the D0 slot.
void DecalTemplate::addRef()  { ++m_refCount; }
int  DecalTemplate::getRef()  { return m_refCount; }

int DecalTemplate::release()
{
    const int r = --m_refCount;
    if (r == 0)
        delete this;
    return r;
}

// three interfaces only.
void* DecalTemplate::queryInterface(unsigned int iid) const
{
    DecalTemplate* self = const_cast<DecalTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplate)
        return static_cast<IObjectTemplate*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int DecalTemplate::getClassID() const { return CID_DecalTemplate; }

// Marks the template, then creates/finds and refreshes its named renderer decal when DecalManager exists.
// Dedicated builds register no manager, so only the flag changes. Preserve temporary-string lifetime before the test.
void DecalTemplate::templateInitialize()
{
    updateFlags(1, 0);

    geom::IDecalManager* const manager = static_cast<geom::IDecalManager*>(
        classManager->getSingleton(std::string("DecalManager")));
    if (manager)
        m_decal = manager->addTemplate(getName());
    if (m_decal)
        m_decal->refresh();
}

bool DecalTemplate::writeBinary(io::IStream*) const { return false; }
bool DecalTemplate::readBinary(io::IStream*)        { return false; }

// flat two-CID check, no chain.
void* DecalTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_DecalTemplate || classId == CID_ObjectTemplate)
        return this;
    return 0;
}

// mark-precached and return NULL - a decal template creates no world object.
IObject* DecalTemplate::createObject() const
{
    const_cast<DecalTemplate*>(this)->updateFlags(0x20, 0);
    return 0;
}

// Serializes decal properties in their authored order after the base template.
// Animation getters and relative-frame-size reads require a renderer template; they have no null guard.
// Relative frame-size statements retain setter names.
bool DecalTemplate::makeScript(io::IStream* stream)
{
    if (!ObjectTemplate::makeScript(stream)) return false;
    std::ostringstream out;
    const bool isStatic = getIsStatic();
    out << "ObjectTemplate.isStatic " << isStatic << std::endl;
    const float randomSize = getRandomSize();
    out << "ObjectTemplate.randomSize " << randomSize << std::endl;
    const float size = getSize();
    out << "ObjectTemplate.size " << size << std::endl;
    const Vec3 color = getColor();
    out << "ObjectTemplate.color " << color.x << '/' << color.y << '/' << color.z << std::endl;
    const std::string& decalTextureName = getDecalTextureName();
    out << "ObjectTemplate.decalTextureName " << decalTextureName << std::endl;
    const float randomRotation = getRandomRotation();
    out << "ObjectTemplate.randomRotation " << randomRotation << std::endl;
    const float rotation = getRotation();
    out << "ObjectTemplate.rotation " << rotation << std::endl;
    const float randomColorIntensity = getRandomColorIntensity();
    out << "ObjectTemplate.randomColorIntensity " << randomColorIntensity << std::endl;
    geom::IDecalTemplate* const animationDecal = m_decal;
    const bool animationEnable = getAnimationEnable();
    out << "ObjectTemplate.animationEnable " << animationEnable << std::endl;
    const float animationSpeed = getAnimationSpeed();
    out << "ObjectTemplate.animationSpeed " << animationSpeed << std::endl;
    const float frameWidthRelative = animationDecal->getAnimationFrameWidthRelative();    // +0x84
    out << "ObjectTemplate.setAnimationFrameWidthRelative " << frameWidthRelative << std::endl;
    const float frameHeightRelative = animationDecal->getAnimationFrameHeightRelative();  // +0x8C
    out << "ObjectTemplate.setAnimationFrameHeightRelative " << frameHeightRelative << std::endl;
    const bool animationRandomizedStartFrame = getAnimationRandomizedStartFrame();
    out << "ObjectTemplate.animationRandomizedStartFrame " << animationRandomizedStartFrame << std::endl;
    const unsigned int animationFrameCount = getAnimationFrameCount();
    out << "ObjectTemplate.animationFrameCount " << animationFrameCount << std::endl;
    const unsigned int animationFrameCountX = getAnimationFrameCountX();
    out << "ObjectTemplate.animationFrameCountX " << animationFrameCountX << std::endl;
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

// Setters recheck m_decal before refreshing; random color intensity does not refresh.
// Non-animation getters supply null defaults. Animation getters require a valid renderer template.
void DecalTemplate::setSize(float size)
{
    if (m_decal)
        m_decal->setSize(size);                                  // +0x08
    if (m_decal)
        m_decal->refresh();                                      // +0x04
}

float DecalTemplate::getSize()
{
    return m_decal ? m_decal->getSize() : 0.0f;                  // +0x0C
}

void DecalTemplate::setRotation(float rotation)
{
    if (m_decal)
        m_decal->setRotation(rotation);                          // +0x48
    if (m_decal)
        m_decal->refresh();
}

float DecalTemplate::getRotation() const
{
    return m_decal ? m_decal->getRotation() : 0.0f;              // +0x4C
}

void DecalTemplate::setRandomRotation(float randomRotation)
{
    if (m_decal)
        m_decal->setRandomRotation(randomRotation);              // +0x40
    if (m_decal)
        m_decal->refresh();
}

float DecalTemplate::getRandomRotation() const
{
    return m_decal ? m_decal->getRandomRotation() : 0.0f;        // +0x44
}

void DecalTemplate::setColor(const Vec3& color)
{
    if (m_decal)
        m_decal->setColor(color);                                // +0x20
    if (m_decal)
        m_decal->refresh();
}

Vec3 DecalTemplate::getColor() const
{
    if (m_decal)
        return m_decal->getColor();                              // +0x24 (hidden result pointer)
    const Vec3 zero = { 0.0f, 0.0f, 0.0f };
    return zero;
}

void DecalTemplate::setRandomSize(float randomSize)
{
    if (m_decal)
        m_decal->setRandomSize(randomSize);                      // +0x10
    if (m_decal)
        m_decal->refresh();
}

float DecalTemplate::getRandomSize() const
{
    return m_decal ? m_decal->getRandomSize() : 0.0f;            // +0x14
}

void DecalTemplate::setUsedOnTransparentMaterials(bool used)
{
    if (m_decal)
        m_decal->setUsedOnTransparentMaterials(used);            // +0x30
    if (m_decal)
        m_decal->refresh();
}

bool DecalTemplate::getUsedOnTransparentMaterials() const
{
    return m_decal ? m_decal->getUsedOnTransparentMaterials() : true;   // +0x34
}

void DecalTemplate::setIsStatic(bool isStatic)
{
    if (m_decal)
        m_decal->setIsStatic(isStatic);                          // +0x28
    if (m_decal)
        m_decal->refresh();
}

bool DecalTemplate::getIsStatic() const
{
    return m_decal ? m_decal->getIsStatic() : true;              // +0x2C
}

void DecalTemplate::setDecalTextureName(const std::string& name)
{
    if (m_decal)
        m_decal->setDecalTextureName(name);                      // +0x50
    if (m_decal)
        m_decal->refresh();
}

const std::string& DecalTemplate::getDecalTextureName() const
{
    return m_decal ? m_decal->getDecalTextureName() : g_emptyString;   // +0x54
}

// No refresh - both images (ELF 0x81F8A20 37 B, client 0x647170 a 17-byte tail jump).
void DecalTemplate::setRandomColorIntensity(float intensity)
{
    if (m_decal)
        m_decal->setRandomColorIntensity(intensity);             // +0x18
}

float DecalTemplate::getRandomColorIntensity() const
{
    return m_decal ? m_decal->getRandomColorIntensity() : 0.0f;  // +0x1C
}

void DecalTemplate::setAnimationEnable(bool enable)
{
    if (m_decal)
        m_decal->setAnimationEnable(enable);                     // +0x58
    if (m_decal)
        m_decal->refresh();
}

bool DecalTemplate::getAnimationEnable() const
{
    return m_decal->getAnimationEnable();                        // +0x5C unguarded, as shipped
}

void DecalTemplate::setAnimationRandomizedStartFrame(bool randomized)
{
    if (m_decal)
        m_decal->setAnimationRandomizedStartFrame(randomized);   // +0x60
    if (m_decal)
        m_decal->refresh();
}

bool DecalTemplate::getAnimationRandomizedStartFrame() const
{
    return m_decal->getAnimationRandomizedStartFrame();          // +0x64 unguarded, as shipped
}

void DecalTemplate::setAnimationSpeed(float speed)
{
    if (m_decal)
        m_decal->setAnimationSpeed(speed);                       // +0x68
    if (m_decal)
        m_decal->refresh();
}

float DecalTemplate::getAnimationSpeed() const
{
    return m_decal->getAnimationSpeed();                         // +0x6C unguarded, as shipped
}

void DecalTemplate::setAnimationFrameWidth(unsigned int width)
{
    if (m_decal)
        m_decal->setAnimationFrameWidth(width);                  // +0x70
    if (m_decal)
        m_decal->refresh();
}

unsigned int DecalTemplate::getAnimationFrameWidth() const
{
    return m_decal->getAnimationFrameWidth();                    // +0x74 unguarded, as shipped
}

void DecalTemplate::setAnimationFrameHeight(unsigned int height)
{
    if (m_decal)
        m_decal->setAnimationFrameHeight(height);                // +0x78
    if (m_decal)
        m_decal->refresh();
}

unsigned int DecalTemplate::getAnimationFrameHeight() const
{
    return m_decal->getAnimationFrameHeight();                   // +0x7C unguarded, as shipped
}

void DecalTemplate::setAnimationFrameWidthRelative(float width)
{
    if (m_decal)
        m_decal->setAnimationFrameWidthRelative(width);          // +0x80
    if (m_decal)
        m_decal->refresh();
}

void DecalTemplate::setAnimationFrameHeightRelative(float height)
{
    if (m_decal)
        m_decal->setAnimationFrameHeightRelative(height);        // +0x88
    if (m_decal)
        m_decal->refresh();
}

void DecalTemplate::setAnimationFrameCount(unsigned int count)
{
    if (m_decal)
        m_decal->setAnimationFrameCount(count);                  // +0x90
    if (m_decal)
        m_decal->refresh();
}

unsigned int DecalTemplate::getAnimationFrameCount() const
{
    return m_decal->getAnimationFrameCount();                    // +0x94 unguarded, as shipped
}

void DecalTemplate::setAnimationFrameCountX(unsigned int count)
{
    if (m_decal)
        m_decal->setAnimationFrameCountX(count);                 // +0x98
    if (m_decal)
        m_decal->refresh();
}

unsigned int DecalTemplate::getAnimationFrameCountX() const
{
    return m_decal->getAnimationFrameCountX();                   // +0x9C unguarded, as shipped
}

} // namespace world
} // namespace hfe
} // namespace dice
