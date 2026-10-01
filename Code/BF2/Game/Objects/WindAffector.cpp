// Path:     [INFERRED] - no __FILE__ string names the source. The symbolised ELF emits every
// WindAffector runtime body immediately before every WindAffectorTemplate body, which is
// strong evidence for one source translation unit beside the existing template header.
// Contents: recovered from bf2.i64 and cross-checked instruction-for-instruction against the BF2
// 1.50 Win32 dedicated server.

#include "WindAffector.h"
#include "WindAffectorTemplate.h"

#include <cmath>
#include <limits>
#include <sstream>

#include "../../Core/Ids.h"
#include "../../Core/StringUtil.h"
#include "../../IO/Stream/IStream.h"
#include "../../Scene/Object/WindManager.h"
#include "IObject.h"

namespace dice {
namespace hfe {
namespace world {

namespace {

// . The source symbol name is unavailable; the values and order are
// attested because setWindType scans this table and makeScript indexes it directly.
const char* const kWindTypeNames[4] = {
    "Directional", "Point", "RadialDir", "TurbulenceField"
};

enum WindType
{
    WindType_Directional    = 0,
    WindType_Point          = 1,
    WindType_RadialDir      = 2,
    WindType_TurbulenceField = 3
};

WindManager* activeWindManager()
{
    // The singleton registry stores the primary IWindManager face in the historical IBase* global.
    // RTTI proves that face is WindManager's sole base, so this is an ordinary typed downcast.
    return static_cast<WindManager*>(windManager);
}

Vec3 rotateDirection(const Mat4& transform, const Vec3& direction)
{
    Vec3 result;
    result.x = transform.m[0] * direction.x
             + transform.m[4] * direction.y
             + transform.m[8] * direction.z;
    result.y = transform.m[1] * direction.x
             + transform.m[5] * direction.y
             + transform.m[9] * direction.z;
    result.z = transform.m[2] * direction.x
             + transform.m[6] * direction.y
             + transform.m[10] * direction.z;
    return result;
}

float lengthSquared(const Vec3& value)
{
    return value.x * value.x + value.y * value.y + value.z * value.z;
}

float distanceSquared(const Vec3& a, const Vec3& b)
{
    const float x = a.x - b.x;
    const float y = a.y - b.y;
    const float z = a.z - b.z;
    return x * x + y * y + z * z;
}

float dot(const Vec3& a, const Vec3& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

// and the two inlined ELF copies. A nearly-unit vector is deliberately left bitwise
// untouched; a near-zero vector is zeroed; everything else is scaled by 1/sqrt(lengthSquared).
void normaliseSafe(Vec3& value)
{
    const float squared = lengthSquared(value);
    const float epsilon = std::numeric_limits<float>::epsilon();
    if (std::fabs(squared - 1.0f) >= epsilon)
    {
        if (std::fabs(squared) < epsilon)
        {
            value.x = 0.0f;
            value.y = 0.0f;
            value.z = 0.0f;
        }
        else
        {
            const float scale = 1.0f / std::sqrt(squared);
            value.x *= scale;
            value.y *= scale;
            value.z *= scale;
        }
    }
}

// The shipped PE cap maps equality, values above one, and unordered values to one. This spelling
// preserves that NaN behavior; std::min's argument order would make it easy to change silently.
float capAtOne(float value)
{
    return value < 1.0f ? value : 1.0f;
}

} // namespace

// WindAffector runtime - ELF 0x82B3DF0..0x82B471A / PE 0x628740..0x628BFF.

WindAffector::WindAffector(const WindAffectorTemplate* objectTemplate)
    : m_refCount(1)
    , m_template(objectTemplate)
    , m_object(0)
    , m_active(true)
{
    // m_remainingBlowTime is deliberately uninitialised; init() is its shipped first writer.
}

WindAffector::~WindAffector() {}

void WindAffector::addRef() { ++m_refCount; }
int WindAffector::getRef() { return m_refCount; }

int WindAffector::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        delete this;
    return remaining;
}

void* WindAffector::queryInterface(unsigned int iid) const
{
    if (iid == IID_IBase || iid == IID_IObjectComponent || iid == IID_IWindAffector)
        return const_cast<WindAffector*>(this);
    return 0;
}

unsigned int WindAffector::getInterfaceID() const { return IID_IWindAffector; }
unsigned int WindAffector::getClassID() const { return CID_WindAffector; }

void WindAffector::init(IObject* object)
{
    m_object = object;
    m_remainingBlowTime = m_template->getWindBlowTime();
    WindManager* const manager = activeWindManager();
    if (manager)
        manager->addWindAffector(this);
}

void WindAffector::destroy()
{
    WindManager* const manager = activeWindManager();
    if (manager)
        manager->removeWindAffector(this);
}

void WindAffector::setObject(IObject* object) { m_object = object; }
IObject* WindAffector::getObject() const { return m_object; }

void WindAffector::update(float deltaTime)
{
    if (m_active && m_remainingBlowTime > 0.0f)
    {
        m_remainingBlowTime -= deltaTime;
        if (m_remainingBlowTime <= 0.0f)
        {
            WindManager* const manager = activeWindManager();
            if (manager)
                manager->removeWindAffector(this);
            m_active = false;
        }
    }
}

bool WindAffector::getIsActive() const { return m_active; }
void WindAffector::setIsActive(bool active) { m_active = active; }

float WindAffector::getBoundingRadius() const
{
    return m_template->getWindFalloff();
}

const Vec3& WindAffector::getAbsolutePosition() const
{
    return m_object->getAbsolutePosition();
}

Vec3 WindAffector::getAbsoluteWind(const Vec3& position) const
{
    Vec3 result;
    if (!m_active)
    {
        result.x = 0.0f;
        result.y = 0.0f;
        result.z = 0.0f;
        return result;
    }

    Vec3 direction = { 0.0f, 0.0f, 0.0f };
    float speed; // Deliberately uninitialised: TurbulenceField's shipped body writes neither out.
    getAbsoluteWindDirectionAndSpeed(position, direction, speed);
    result.x = direction.x * speed;
    result.y = direction.y * speed;
    result.z = direction.z * speed;
    return result;
}

// . `windIsDynamic` is intentionally not read: both shipped runtimes
// always fetch the current object position/transformation. TurbulenceField is a recognised authoring
// token whose runtime case returns without touching either output; that retail defect is preserved.
void WindAffector::getAbsoluteWindDirectionAndSpeed(const Vec3& position,
                                                    Vec3& direction,
                                                    float& speed) const
{
    if (!m_active)
    {
        speed = 0.0f;
        return;                         // direction is deliberately left untouched
    }

    if (m_template->m_windTypeEnum == WindType_Directional)
    {
        direction = rotateDirection(m_object->getAbsoluteTransformation(),
                                    m_template->m_windDirection);
        const float falloffSquared = m_template->m_windFalloff * m_template->m_windFalloff;
        const float attenuation = capAtOne(
            distanceSquared(position, getAbsolutePosition()) / falloffSquared);
        speed = (1.0f - attenuation) * m_template->m_windSpeed;
        return;
    }

    if (m_template->m_windTypeEnum == WindType_Point)
    {
        const Vec3& centre = getAbsolutePosition();
        direction.x = position.x - centre.x;
        direction.y = position.y - centre.y;
        direction.z = position.z - centre.z;
        normaliseSafe(direction);

        const float falloff = m_template->m_windFalloff;
        // Retail refetches the virtual absolute position instead of retaining `centre`.
        const float attenuation = capAtOne(
            distanceSquared(position, getAbsolutePosition()) / (falloff * falloff));
        speed = (1.0f - attenuation) * m_template->m_windSpeed;
        return;
    }

    if (m_template->m_windTypeEnum == WindType_RadialDir)
    {
        const Vec3 axis = rotateDirection(m_object->getAbsoluteTransformation(),
                                          m_template->m_windDirection);
        const Vec3& centre = getAbsolutePosition();
        direction.x = position.x - centre.x;
        direction.y = position.y - centre.y;
        direction.z = position.z - centre.z;

        const float axialDistance = dot(direction, axis);
        const float axialAttenuation = capAtOne(axialDistance / m_template->m_windFalloff);
        speed = (1.0f - axialAttenuation) * m_template->m_windSpeed;

        Vec3 radial;
        radial.x = direction.x - axis.x * axialDistance;
        radial.y = direction.y - axis.y * axialDistance;
        radial.z = direction.z - axis.z * axialDistance;
        const float radialSquared = lengthSquared(radial);
        const float radialFalloffSquared =
            m_template->m_windRadialFalloff * m_template->m_windRadialFalloff;

        if (!(radialSquared < radialFalloffSquared))
        {
            speed = 0.0f;
            return;                     // direction remains the unnormalised centre-to-query delta
        }

        const float radialBlend = capAtOne(
            radialSquared / radialFalloffSquared + 0.2f);
        normaliseSafe(radial);
        direction.x = radial.x * radialBlend + axis.x * (1.0f - radialBlend);
        direction.y = radial.y * radialBlend + axis.y * (1.0f - radialBlend);
        direction.z = radial.z * radialBlend + axis.z * (1.0f - radialBlend);
        return;
    }

    // WindType_TurbulenceField and any out-of-range enum write neither output in retail.
}

// WindAffectorTemplate - ELF 0x82B4870..0x82B502F / PE 0x5F8210..0x5F865F.

WindAffectorTemplate::WindAffectorTemplate()
    : m_refCount(1)
    , m_windIsDynamic(true)
    , m_windType()
{
    // _byteD.._byteF and m_windTypeEnum have no direct constructor stores. setWindType below is
    // the enum's deterministic first writer.
    m_windDirection.x = 0.0f;
    m_windDirection.y = -1.0f;
    m_windDirection.z = 0.0f;
    m_windSpeed = 0.0f;
    m_windFalloff = 10.0f;
    m_windRadialFalloff = 1.0f;
    m_windBlowTime = -1.0f;
    setWindType(std::string("Directional"));
}

WindAffectorTemplate::~WindAffectorTemplate() {}

void WindAffectorTemplate::addRef() { ++m_refCount; }
int WindAffectorTemplate::getRef() { return m_refCount; }

int WindAffectorTemplate::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
        delete this;
    return remaining;
}

void* WindAffectorTemplate::queryInterface(unsigned int iid) const
{
    WindAffectorTemplate* self = const_cast<WindAffectorTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplateComponent
        || iid == IID_IWindAffectorTemplate)
    {
        return static_cast<IWindAffectorTemplate*>(self);
    }
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int WindAffectorTemplate::getClassID() const { return CID_WindAffectorTemplate; }
unsigned int IWindAffectorTemplate::getInterfaceID() { return IID_IWindAffectorTemplate; }

IObjectComponent* WindAffectorTemplate::createComponent()
{
    return new WindAffector(this);
}

void WindAffectorTemplate::objectCreated(IObject*) {}
void WindAffectorTemplate::objectDestroyed(IObject*) {}
void WindAffectorTemplate::setTemplate(IObjectTemplate*) {}

void* WindAffectorTemplate::queryImplementation(unsigned int classId)
{
    return classId == getClassID() ? this : 0;
}

void WindAffectorTemplate::setWindType(const std::string& name)
{
    for (int type = 0; type < 4; ++type)
    {
        if (compareIgnoreCase(name.c_str(), kWindTypeNames[type]) == 0)
        {
            m_windTypeEnum = type;
            m_windType = name;             // authored spelling/case is retained
            return;
        }
    }
}

const std::string& WindAffectorTemplate::getWindType() const { return m_windType; }
int WindAffectorTemplate::getWindTypeEnum() const { return m_windTypeEnum; }
void WindAffectorTemplate::setWindIsDynamic(bool value) { m_windIsDynamic = value; }
bool WindAffectorTemplate::getWindIsDynamic() const { return m_windIsDynamic; }

void WindAffectorTemplate::setWindDirection(const Math::Vec3& value)
{
    m_windDirection.x = value.x;
    m_windDirection.y = value.y;
    m_windDirection.z = value.z;
}

const Math::Vec3& WindAffectorTemplate::getWindDirection() const { return m_windDirection; }
void WindAffectorTemplate::setWindSpeed(float value) { m_windSpeed = value; }
float WindAffectorTemplate::getWindSpeed() const { return m_windSpeed; }
void WindAffectorTemplate::setWindFalloff(float value) { m_windFalloff = value; }
float WindAffectorTemplate::getWindFalloff() const { return m_windFalloff; }
void WindAffectorTemplate::setWindRadialFalloff(float value) { m_windRadialFalloff = value; }
float WindAffectorTemplate::getWindRadialFalloff() const { return m_windRadialFalloff; }
void WindAffectorTemplate::setWindBlowTime(float value) { m_windBlowTime = value; }
float WindAffectorTemplate::getWindBlowTime() const { return m_windBlowTime; }

bool WindAffectorTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    script << "ObjectTemplate.windType " << kWindTypeNames[m_windTypeEnum] << std::endl;
    script << "ObjectTemplate.windIsDynamic " << m_windIsDynamic << std::endl;
    script << "ObjectTemplate.windDirection "
           << m_windDirection.x << '/' << m_windDirection.y << '/' << m_windDirection.z
           << std::endl;
    script << "ObjectTemplate.windSpeed " << m_windSpeed << std::endl;
    script << "ObjectTemplate.windBlowTime " << m_windBlowTime << std::endl;
    script << "ObjectTemplate.windFalloff " << m_windFalloff << std::endl;
    script << "ObjectTemplate.windRadialFalloff " << m_windRadialFalloff << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
