// Path:     attested - "Game/Objects/Visual/LightSourceTemplate.cpp" appears as __FILE__ in the
// binary (the four cap-warnings below pass it; their line numbers are preserved in the
// Debug calls, which is why the caps sit at odd-looking lines here rather than their
// original ones - the file and line arguments are the recovered constants, not ours).
// Contents: recovered from bf2.i64.

#include "LightSourceTemplate.h"
#include "../../../IO/Stream/IStream.h"
#include <sstream>
#include "LightSource.h"                        // createObject's product

#include <cmath>
#include "../../../Core/ClassManager.h"
#include "../../../Core/Debug.h"
#include "../../../Core/Ids.h"
#include "../../../Core/StringUtil.h"

namespace dice {
namespace hfe {

extern ClassManager* classManager;   // ds:0x8AC85A0 - defined in Core/Globals.cpp (no header
                                     // extern exists yet; same local declaration the
                                     // ObjectTemplateManager TU carries)

namespace world {

// the attested table. Index == the m_lightTypeEnum value.
const char* const LightSourceTypeNames[4] = { "Directional", "Point", "Spot", "SpotProjector" };

// ctor - 0x82B1CC0, 408 B, verified store-by-store in raw disasm (the decompile mixed byte and
// dword scalings; every value below is the disasm's). Defaults of note: type Point (enum 1, name
// string empty), direction (0,0,1), projector fov pi/3 (60 degrees), intensity 0.7. Tail:
// updateFlags(2, 0).
LightSourceTemplate::LightSourceTemplate()
{
    m_refCount = 1;                               // +0x30

    m_isDynamic = 0;
    m_castsStaticShadow = 0;
    m_castsDynamicShadow = 0;
    m_selfShadows = 0;
    m_selfLights = 1;
    m_supportsPerVertex = 1;
    m_supportsPerPixel = 0;
    m_supportsGlow = 1;
    m_supportsEmitter = 1;
    m_affectLightmappedObjects = 1;

    m_lightTypeEnum = 1;                          // Point - but m_lightType stays empty
    m_affectingType = 0;
    m_direction.x = 0.0f; m_direction.y = 0.0f; m_direction.z = 1.0f;
    m_attenuationRange1 = 4.0f;
    m_attenuationRange2 = 5.0f;
    m_color.x = 1.0f;    m_color.y = 1.0f;    m_color.z = 1.0f;
    m_specColor.x = 1.0f; m_specColor.y = 1.0f; m_specColor.z = 1.0f;
    m_scale.x = 1.0f;    m_scale.y = 1.0f;    m_scale.z = 1.0f;
    m_HDRIntensity = 1.0f;
    m_coneAngle1 = 20.0f;
    m_coneAngle2 = 22.5f;
    m_fov = 8.0f;
    m_flicker = 0.0f;
    m_enabled = 1;
    // m_projectorTexture: default-constructed empty.
    m_projectorFov = 1.0471976f;                  // = pi/3 - sixty degrees in radians
    m_projectorAspect = 1.0f;
    m_projectorNear = 0.001f;
    m_projectorFar = 5.0f;
    m_projectorApplyMask = 0;
    m_projectorExplicitMask.x = 1.0f; m_projectorExplicitMask.y = 1.0f;
    m_projectorExplicitMask.z = 1.0f;
    m_projectorGeometry = 0;
    m_objectShadows = 1;
    m_shadowIntensity = 0.85f;
    m_softness = 0.0f;
    m_intensity = 0.7f;
    // m_geometry: default-constructed empty.

    updateFlags(2, 0);
}

// (D1), 189 B: release the projector geometry if bound, then the three strings
// (compiler's job in our build), then ~ObjectTemplate.
LightSourceTemplate::~LightSourceTemplate()
{
    if (m_projectorGeometry)
        m_projectorGeometry->release();
}

// The per-class quartet - refcount +0x30, delete on the 1->0 edge via the D0 slot.
void LightSourceTemplate::addRef()  { ++m_refCount; }
int  LightSourceTemplate::getRef()  { return m_refCount; }

int LightSourceTemplate::release()
{
    const int r = --m_refCount;
    if (r == 0)
        delete this;
    return r;
}

// three interfaces only; no light-specific IID exists.
void* LightSourceTemplate::queryInterface(unsigned int iid) const
{
    LightSourceTemplate* self = const_cast<LightSourceTemplate*>(this);
    if (iid == IID_IBase || iid == IID_IObjectTemplate)
        return static_cast<IObjectTemplate*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    return 0;
}

unsigned int LightSourceTemplate::getClassID() const { return CID_LightSourceTemplate; }

// 7-byte stubs - return false, touch nothing.
bool LightSourceTemplate::writeBinary(io::IStream*) const { return false; }
bool LightSourceTemplate::readBinary(io::IStream*)        { return false; }

// flat two-CID check (cmovz in the ELF); does NOT chain to ObjectTemplate's.
void* LightSourceTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_LightSourceTemplate || classId == CID_ObjectTemplate)
        return this;
    return 0;
}

// Creates a light source from this template.
IObject* LightSourceTemplate::createObject() const { return new LightSource(this); }

// setLightType - 0x82B2D50, 213 B. The name->enum map over LightSourceTypeNames (case-insensitive;
// an unknown name is a total no-op - nothing stored, no warning). On a match: store the enum,
// store the name, drop any existing projector geometry, and for SpotProjector (3) create a fresh
// geom::LightGeometry through the ClassManager and hand it the projector texture. The concrete
// geometry class is unregistered in this tree -> createInstance returns null -> the ELF's own
// null guard skips the texture call, exactly as a headless server without the geom module behaves.
void LightSourceTemplate::setLightType(const std::string& name)
{
    int type = 0;
    for (;;)
    {
        if (compareIgnoreCase(name.c_str(), LightSourceTypeNames[type]) == 0)
            break;
        if (++type > 3)
            return;                               // unknown name: silent no-op
    }

    m_lightTypeEnum = type;
    m_lightType = name;

    if (m_projectorGeometry)
    {
        m_projectorGeometry->release();
        m_projectorGeometry = 0;
    }

    if (m_lightTypeEnum == 3)                     // SpotProjector
    {
        m_projectorGeometry = static_cast<geom::ILightGeometry*>(
            classManager->createInstance(geom::CID_LightGeometry, geom::IID_ILightGeometry, 0));
        if (m_projectorGeometry)
            m_projectorGeometry->setTexture(m_projectorTexture);
    }
}

const std::string& LightSourceTemplate::getLightType() const { return m_lightType; }
int  LightSourceTemplate::getLightTypeEnum() const           { return m_lightTypeEnum; }

// the eleven byte-flag pairs + affectingType: raw stores/reads, offsets per the header
void LightSourceTemplate::setIsDynamic(bool v)                { m_isDynamic = v; }
bool LightSourceTemplate::getIsDynamic() const                { return m_isDynamic != 0; }
void LightSourceTemplate::setCastsStaticShadow(bool v)        { m_castsStaticShadow = v; }
bool LightSourceTemplate::getCastsStaticShadow() const        { return m_castsStaticShadow != 0; }
void LightSourceTemplate::setCastsDynamicShadow(bool v)       { m_castsDynamicShadow = v; }
bool LightSourceTemplate::getCastsDynamicShadow() const       { return m_castsDynamicShadow != 0; }
void LightSourceTemplate::setSelfLights(bool v)               { m_selfLights = v; }
bool LightSourceTemplate::getSelfLights() const               { return m_selfLights != 0; }
void LightSourceTemplate::setSelfShadows(bool v)              { m_selfShadows = v; }
bool LightSourceTemplate::getSelfShadows() const              { return m_selfShadows != 0; }
void LightSourceTemplate::setSupportsPerVertex(bool v)        { m_supportsPerVertex = v; }
bool LightSourceTemplate::getSupportsPerVertex() const        { return m_supportsPerVertex != 0; }
void LightSourceTemplate::setSupportsPerPixel(bool v)         { m_supportsPerPixel = v; }
bool LightSourceTemplate::getSupportsPerPixel() const         { return m_supportsPerPixel != 0; }
void LightSourceTemplate::setSupportsGlow(bool v)             { m_supportsGlow = v; }
bool LightSourceTemplate::getSupportsGlow()                   { return m_supportsGlow != 0; }
void LightSourceTemplate::setSupportsEmitter(bool v)          { m_supportsEmitter = v; }
bool LightSourceTemplate::getSupportsEmitter()                { return m_supportsEmitter != 0; }
void LightSourceTemplate::setAffectLightmappedObjects(bool v) { m_affectLightmappedObjects = v; }
bool LightSourceTemplate::getAffectLightmappedObjects() const { return m_affectLightmappedObjects != 0; }
void LightSourceTemplate::setAffectingType(int v)             { m_affectingType = v; }
int  LightSourceTemplate::getAffectingType() const            { return m_affectingType; }

// setDirection - 0x82B2E50, 198 B: store, then normalize IN place unless the length is already 1
// (within FLT_EPSILON of the squared length). A (near-)zero vector is stored as exact zero. The
// stored field is therefore always unit or zero, whatever the .con said.
void LightSourceTemplate::setDirection(const Vec3& v)
{
    m_direction = v;
    const float lenSq = v.x * v.x + v.y * v.y + v.z * v.z;
    float dev = lenSq - 1.0f;
    if (dev < 0.0f) dev = -dev;                   // andnps sign-strip in the ELF
    if (dev >= 0.00000011920929f)
    {
        float alenSq = lenSq < 0.0f ? -lenSq : lenSq;
        if (alenSq >= 0.00000011920929f)
        {
            const float inv = 1.0f / std::sqrt(lenSq);
            m_direction.x *= inv;
            m_direction.y *= inv;
            m_direction.z *= inv;
        }
        else
        {
            m_direction.x = 0.0f;
            m_direction.y = 0.0f;
            m_direction.z = 0.0f;
        }
    }
}

const Vec3& LightSourceTemplate::getDirection() const { return m_direction; }

// Setters cap values after emitting a warning; DEBUGTYPE_6 currently aborts before that store.
void LightSourceTemplate::setAttenuationRange1(float v)
{
    if (v > 100.0f)
    {
        Debug(DEBUGTYPE_6, "Game/Objects/Visual/LightSourceTemplate.cpp", 198)
            << "Light " << getName() << " attenuation1 >100, capped to 100.";
        v = 100.0f;
    }
    m_attenuationRange1 = v;
}
float LightSourceTemplate::getAttenuationRange1() const { return m_attenuationRange1; }

void LightSourceTemplate::setAttenuationRange2(float v)
{
    if (v > 100.0f)
    {
        Debug(DEBUGTYPE_6, "Game/Objects/Visual/LightSourceTemplate.cpp", 215)
            << "Light " << getName() << " attenuation2 >100, capped to 100.";
        v = 100.0f;
    }
    m_attenuationRange2 = v;
}
float LightSourceTemplate::getAttenuationRange2() const { return m_attenuationRange2; }

void LightSourceTemplate::setConeAngle1(float v)
{
    if (v > 180.0f)
    {
        Debug(DEBUGTYPE_6, "Game/Objects/Visual/LightSourceTemplate.cpp", 261)
            << "Light " << getName() << " coneAngle1 >180, capped to 180.";
        v = 180.0f;
    }
    m_coneAngle1 = v;
}

// [DICE-BUG] - reads coneAngle2's field. Both getters are byte-identical `fld [this+8Ch]` in
// the ELF while setConeAngle1 stores +0x88. A console GET of coneAngle1 answers coneAngle2.
// Observable behaviour, preserved verbatim.
float LightSourceTemplate::getConeAngle1() const { return m_coneAngle2; }

void LightSourceTemplate::setConeAngle2(float v)
{
    if (v > 180.0f)
    {
        Debug(DEBUGTYPE_6, "Game/Objects/Visual/LightSourceTemplate.cpp", 276)
            << "Light " << getName() << " coneAngle2 >180, capped to 180.";
        v = 180.0f;
    }
    m_coneAngle2 = v;
}
float LightSourceTemplate::getConeAngle2() const { return m_coneAngle2; }

void LightSourceTemplate::setFov(float v)
{
    if (v > 180.0f)
    {
        Debug(DEBUGTYPE_6, "Game/Objects/Visual/LightSourceTemplate.cpp", 366)
            << "Light " << getName() << " fov >180, capped to 180.";
        v = 180.0f;
    }
    m_fov = v;
}
float LightSourceTemplate::getFov() const { return m_fov; }

// the plain Vec3 / float / bool stores
void LightSourceTemplate::setColor(const Vec3& v)                { m_color = v; }
const Vec3& LightSourceTemplate::getColor() const                { return m_color; }
void LightSourceTemplate::setSpecColor(const Vec3& v)            { m_specColor = v; }
const Vec3& LightSourceTemplate::getSpecColor() const            { return m_specColor; }
void LightSourceTemplate::setHDRIntensity(float v)               { m_HDRIntensity = v; }
float LightSourceTemplate::getHDRIntensity() const               { return m_HDRIntensity; }

// an empty name is ignored; an unchanged name is ignored; otherwise assign and, if a
// projector geometry exists, hand it the new texture.
void LightSourceTemplate::setProjectorTexture(const std::string& name)
{
    if (name.length() == 0)
        return;
    if (m_projectorTexture == name)
        return;
    m_projectorTexture = name;
    if (m_projectorGeometry)
        m_projectorGeometry->setTexture(m_projectorTexture);
}
const std::string& LightSourceTemplate::getProjectorTexture() const { return m_projectorTexture; }

void LightSourceTemplate::setProjectorFov(float v)               { m_projectorFov = v; }
float LightSourceTemplate::getProjectorFov() const               { return m_projectorFov; }
void LightSourceTemplate::setProjectorAspect(float v)            { m_projectorAspect = v; }
float LightSourceTemplate::getProjectorAspect() const            { return m_projectorAspect; }
void LightSourceTemplate::setProjectorNear(float v)              { m_projectorNear = v; }
float LightSourceTemplate::getProjectorNear() const              { return m_projectorNear; }
void LightSourceTemplate::setProjectorFar(float v)               { m_projectorFar = v; }
float LightSourceTemplate::getProjectorFar() const               { return m_projectorFar; }
void LightSourceTemplate::setProjectorApplyMask(bool v)          { m_projectorApplyMask = v; }
bool LightSourceTemplate::getProjectorApplyMask() const          { return m_projectorApplyMask != 0; }
void LightSourceTemplate::setProjectorExplicitMask(const Vec3& v){ m_projectorExplicitMask = v; }
const Vec3& LightSourceTemplate::getProjectorExplicitMask() const{ return m_projectorExplicitMask; }
void LightSourceTemplate::setObjectShadows(bool v)               { m_objectShadows = v; }
bool LightSourceTemplate::getObjectShadows() const               { return m_objectShadows != 0; }
void LightSourceTemplate::setShadowIntensity(float v)            { m_shadowIntensity = v; }
float LightSourceTemplate::getShadowIntensity() const            { return m_shadowIntensity; }
void LightSourceTemplate::setSoftness(float v)                   { m_softness = v; }
float LightSourceTemplate::getSoftness() const                   { return m_softness; }
void LightSourceTemplate::setIntensity(float v)                  { m_intensity = v; }
float LightSourceTemplate::getIntensity() const                  { return m_intensity; }
const Vec3& LightSourceTemplate::getScale() const                { return m_scale; }
void LightSourceTemplate::setScale(const Vec3& v)                { m_scale = v; }

// Clamps flicker to [0, 1].
// [PLATFORM] Windows maps NaN to zero; the ELF behavior preserves NaN.
void LightSourceTemplate::setFlicker(float v)
{
#if defined(_WIN32)
    float r = (v >= 1.0f) ? 1.0f : v;             // fcmovnb: an unordered compare keeps v
    r = (r >= 0.0f) ? r : 0.0f;                   // fcmovb:  an unordered compare takes 0
#else
    float r = v;
    if (v < 0.0f)                                 // fucomi / ja
        r = 0.0f;
    else if (v > 1.0f)                            // fucomi / fcmovnbe
        r = 1.0f;
#endif
    m_flicker = r;                                // +0x94 (ELF) / +0xDC (BF2.exe)
}

float LightSourceTemplate::getFlicker() const                    { return m_flicker; }
void LightSourceTemplate::setEnabled(bool v)                     { m_enabled = v; }
bool LightSourceTemplate::getEnabled() const                     { return m_enabled != 0; }

geom::ILightGeometry* LightSourceTemplate::getProjectorGeometry() const { return m_projectorGeometry; }

// ELF 82B2260 / dedicated PE 5F7470: every property is unconditional, in this order.
bool LightSourceTemplate::makeScript(io::IStream* stream)
{
    if (!ObjectTemplate::makeScript(stream)) return false;
    std::ostringstream out;
    out << "ObjectTemplate.lightType " << LightSourceTypeNames[m_lightTypeEnum] << std::endl;
    out << "ObjectTemplate.isDynamic " << (m_isDynamic != 0) << std::endl;
    out << "ObjectTemplate.castsStaticShadow " << (m_castsStaticShadow != 0) << std::endl;
    out << "ObjectTemplate.castsDynamicShadow " << (m_castsDynamicShadow != 0) << std::endl;
    out << "ObjectTemplate.selfLights " << (m_selfLights != 0) << std::endl;
    out << "ObjectTemplate.selfShadows " << (m_selfShadows != 0) << std::endl;
    out << "ObjectTemplate.supportsPerVertex " << (m_supportsPerVertex != 0) << std::endl;
    out << "ObjectTemplate.supportsPerPixel " << (m_supportsPerPixel != 0) << std::endl;
    out << "ObjectTemplate.supportsGlow " << (m_supportsGlow != 0) << std::endl;
    out << "ObjectTemplate.supportsEmitter " << (m_supportsEmitter != 0) << std::endl;
    out << "ObjectTemplate.affectLightmappedObjects " << (m_affectLightmappedObjects != 0) << std::endl;
    out << "ObjectTemplate.affectingType " << m_affectingType << std::endl;
    out << "ObjectTemplate.attenuationRange1 " << m_attenuationRange1 << std::endl;
    out << "ObjectTemplate.attenuationRange2 " << m_attenuationRange2 << std::endl;
    out << "ObjectTemplate.color " << m_color.x << '/' << m_color.y << '/' << m_color.z << std::endl;
    out << "ObjectTemplate.speccolor " << m_specColor.x << '/' << m_specColor.y << '/' << m_specColor.z << std::endl;
    out << "ObjectTemplate.HDRIntensity " << m_HDRIntensity << std::endl;
    out << "ObjectTemplate.coneAngle1 " << m_coneAngle1 << std::endl;
    out << "ObjectTemplate.coneAngle2 " << m_coneAngle2 << std::endl;
    out << "ObjectTemplate.projectorTexture " << m_projectorTexture << std::endl;
    out << "ObjectTemplate.projectorFov " << m_projectorFov << std::endl;
    out << "ObjectTemplate.projectorAspect " << m_projectorAspect << std::endl;
    out << "ObjectTemplate.projectorNear " << m_projectorNear << std::endl;
    out << "ObjectTemplate.projectorFar " << m_projectorFar << std::endl;
    out << "ObjectTemplate.enabled " << (m_enabled != 0) << std::endl;
    out << "ObjectTemplate.fov " << m_fov << std::endl;
    out << "ObjectTemplate.objectShadows " << (m_objectShadows != 0) << std::endl;
    out << "ObjectTemplate.shadowIntensity " << m_shadowIntensity << std::endl;
    out << "ObjectTemplate.softness " << m_softness << std::endl;
    out << "ObjectTemplate.intensity " << m_intensity << std::endl;
    out << "ObjectTemplate.flicker " << m_flicker << std::endl;
    out << "ObjectTemplate.scale " << m_scale.x << '/' << m_scale.y << '/' << m_scale.z << std::endl;
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

} // namespace world
} // namespace hfe
} // namespace dice
