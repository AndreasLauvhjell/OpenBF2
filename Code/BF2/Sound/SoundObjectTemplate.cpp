// Sound template properties and serialization. [INFERRED] Source placement.

#include "SoundObjectTemplate.h"
#include "BfSound.h"

#include <cmath>
#include <sstream>
#include <vector>

#include "../Core/Ids.h"
#include "../Core/StringUtil.h"
#include "../IO/Stream/IStream.h"
#include "../Math/Mat4.h"
#include "../Math/Vec3.h"

namespace dice {
namespace hfe {

// Separators are consumed without validation; only stream failure rejects a nonempty point list.
struct SoundEnvelopeHeader
{
    float attack;
    float sustain;
    float release;
    float scale;
    bool loop;
    unsigned int pointCount;
};

struct SoundEnvelopePoint
{
    float time;
    float value;
    float tangent;
};

std::istream& operator>>(std::istream& stream, SoundEnvelopeHeader& value)
{
    char separator;
    stream >> value.attack >> separator
           >> value.sustain >> separator
           >> value.release >> separator
           >> value.scale >> separator
           >> value.loop >> separator
           >> value.pointCount >> separator;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const SoundEnvelopeHeader& value)
{
    stream << value.attack << '/'
           << value.sustain << '/'
           << value.release << '/'
           << value.scale << '/'
           << value.loop << '/'
           << value.pointCount << '/';
    return stream;
}

std::istream& operator>>(std::istream& stream, SoundEnvelopePoint& value)
{
    char separator;
    stream >> value.time >> separator
           >> value.value >> separator
           >> value.tangent >> separator;
    return stream;
}

std::ostream& operator<<(std::ostream& stream, const SoundEnvelopePoint& value)
{
    stream << value.time << '/' << value.value << '/' << value.tangent << '/';
    return stream;
}


// ctor - ObjectTemplate base (+ the ISoundObjectTemplate base vptr at
// +0x30 the compiler's store), then the impl binding: g_bfSound->engine()->createTemplate() - a
// raw, unguarded deref of g_bfSound, exactly as shipped (the module pass has always created the
// BfSoundNull by the time templates exist). The six defaults are seeded from the impl's getters -
// all 0.0f through the null engine - and the six multipliers are 1.0f. Tail: updateFlags(0x2000) -
// the sound-id-space flag createTemplate tests when assigning ids.

SoundObjectTemplate::SoundObjectTemplate()
{
    m_refCount = 1;                             // +0x34 the per-class quartet's count
    m_impl = 0;
    m_isAmbientAnimSound = 0;                   // +0x6C

    ISoundEngine* engine = g_bfSound->engine(); // +0x28 unguarded, as shipped
    m_impl = engine->createTemplate();          // +0x10 the shared null template on the server

    m_volume             = m_impl->getVolume();             // 0.0f
    m_pitch              = m_impl->getPitch();              // 0.0f
    m_minDistance        = m_impl->getMinDistance();        // 0.0f
    m_halfVolumeDistance = m_impl->getHalfVolumeDistance(); // 0.0f
    m_reverbLevel        = m_impl->getReverbLevel();        // 0.0f
    m_soundRadius        = m_impl->getRadius();             // 0.0f

    m_volumeMultiplier             = 1.0f;
    m_pitchMultiplier              = 1.0f;
    m_minDistanceMultiplier        = 1.0f;
    m_halfVolumeDistanceMultiplier = 1.0f;
    m_reverbLevelMultiplier        = 1.0f;
    m_soundRadiusMultiplier        = 1.0f;

    updateFlags(0x2000, 0);
}


// Releases the bound engine template; the shared null template is not deleted.

SoundObjectTemplate::~SoundObjectTemplate()
{
    if (m_impl)
        m_impl->release();
}


// The per-class IBase quartet - refcount at +0x34; release deletes on the 1->0 edge through the
// D0 slot (a plain `delete this` here). queryInterface: the four-way hop, including the second
// interface base the IID 9986002 selects.

void SoundObjectTemplate::addRef()  { ++m_refCount; }
int  SoundObjectTemplate::getRef()  { return m_refCount; }

int SoundObjectTemplate::release()
{
    const int r = --m_refCount;
    if (r == 0)
        delete this;                             // the ELF dispatches its own D0
    return r;
}

void* SoundObjectTemplate::queryInterface(unsigned int iid) const
{
    SoundObjectTemplate* self = const_cast<SoundObjectTemplate*>(this);
    if (iid == IID_IBase || iid == world::IID_IObjectTemplate)
        return static_cast<world::IObjectTemplate*>(self);
    if (iid == io::IID_IConsoleSaveable)
        return static_cast<io::IConsoleSaveable*>(self);
    if (iid == 9986002u)                         // no IID_ global names it - see the header note
        return static_cast<ISoundObjectTemplate*>(self);
    return 0;
}

unsigned int SoundObjectTemplate::getClassID() const { return 9986001u; }   // immediate


// At quality 2, marks reload/deploy sounds as ambient animation sounds and outdoor fire sounds with flag 2.
// Suffix matching requires a name longer than the suffix.

static bool endsWithIgnoreCase(const std::string& name, const std::string& suffix)
{
    // The ELF guard is strict >: a name equal to the suffix does not match.
    if (name.length() <= suffix.length())
        return false;
    return compareIgnoreCase(name.c_str() + (name.length() - suffix.length()),
                             suffix.c_str()) == 0;
}

void SoundObjectTemplate::templateInitialize()
{
    ISoundEngine* engine = g_bfSound->engine();
    if (engine->getQualityLevel() != 2)
        return;

    static const std::string s_animSuffixes[2] = { "_Reload3P", "_Deploy3P" };
    const std::string& name = getName();
    for (int i = 0; i < 2; ++i)
    {
        if (endsWithIgnoreCase(name, s_animSuffixes[i]))
        {
            m_isAmbientAnimSound = 1;
            break;
        }
    }

    static const std::string s_fire1pOutdoor = "_Fire1P_Outdoor";
    if (endsWithIgnoreCase(name, s_fire1pOutdoor))
        m_impl->setFlags(m_impl->getFlags() | 2);
}

// 15-byte stubs: xor eax + a dead 0x148-byte frame. Both return false and touch nothing.
bool SoundObjectTemplate::writeBinary(io::IStream*) const { return false; }
bool SoundObjectTemplate::readBinary(io::IStream*)        { return false; }

// a flat two-CID check; does not chain to ObjectTemplate::queryImplementation.
void* SoundObjectTemplate::queryImplementation(unsigned int classId)
{
    if (classId == 9986001u || classId == world::CID_ObjectTemplate)
        return this;
    return 0;
}

// Marks the sound template used and returns null; sound templates do not create world objects.
world::IObject* SoundObjectTemplate::createObject() const
{
    const_cast<SoundObjectTemplate*>(this)->setIsUsed();
    return 0;
}

void SoundObjectTemplate::preCache() const
{
    const_cast<SoundObjectTemplate*>(this)->setIsUsed();
}


// Serializes the base first, then engine-template properties.
// Suppresses sound properties for an empty filename, but emits them for a null filename.

namespace {

bool soundVecIsZero(const Vec3& value)
{
    return std::fabs(value.x) <= 0.00000011920929f
        && std::fabs(value.y) <= 0.00000011920929f
        && std::fabs(value.z) <= 0.00000011920929f;
}

void writeSoundVec(std::ostream& stream, const Vec3& value)
{
    stream << value.x << '/' << value.y << '/' << value.z;
}

} // anonymous namespace

bool SoundObjectTemplate::makeScript(io::IStream* stream)
{
    if (!m_impl)
        return false;

    if (!world::ObjectTemplate::makeScript(stream))
        return false;

    const char* const firstFilename = impl()->getFilename();
    if (firstFilename && *impl()->getFilename() == '\0')
        return true;

    std::ostringstream script;
    const std::string prefix("ObjectTemplate");

    if (getLowSamples() >= 0)
        script << prefix << ".lowSamples " << getLowSamples() << std::endl;
    if (getMediumSamples() >= 0)
        script << prefix << ".mediumSamples " << getMediumSamples() << std::endl;

    script << prefix << ".soundFilename \"" << getFilename() << "\"" << std::endl;
    script << prefix << ".loopCount " << impl()->getLoopCount() << std::endl;
    script << prefix << ".is3dSound " << impl()->getIs3dSound() << std::endl;
    script << prefix << ".stopType " << impl()->getStopType() << std::endl;
    script << prefix << ".volume " << impl()->getVolume() << std::endl;
    script << prefix << ".pitch " << impl()->getPitch() << std::endl;

    if (!impl()->getIs3dSound())
        script << prefix << ".pan " << impl()->getPan() << std::endl;

    script << prefix << ".reverbLevel " << impl()->getReverbLevel() << std::endl;

    if (impl()->getPitchEnvelope(0, 0, 0, 0, 0, 0))
        script << prefix << ".pitchEnvelope " << getPitchEnvelope() << std::endl;
    if (impl()->getVolumeEnvelope(0, 0, 0, 0, 0, 0))
        script << prefix << ".volumeEnvelope " << getVolumeEnvelope() << std::endl;

    const Vec3& position = impl()->getPosition();
    if (!soundVecIsZero(position))
    {
        script << prefix << ".position ";
        writeSoundVec(script, impl()->getPosition());
        script << std::endl;
    }

    if (impl()->getMinDistance() != 1.0f)
        script << prefix << ".minDistance " << impl()->getMinDistance() << std::endl;

    if (impl()->getIs3dSound())
    {
        script << prefix << ".halfVolumeDistance "
               << impl()->getHalfVolumeDistance() << std::endl;

        if (impl()->getConeInnerAngle() < 360.0f)
        {
            const Vec3& direction = impl()->getDirection();
            if (!soundVecIsZero(direction))
            {
                script << prefix << ".coneDirection ";
                writeSoundVec(script, impl()->getDirection());
                script << std::endl;
            }

            script << prefix << ".coneInnerAngle "
                   << impl()->getConeInnerAngle() << std::endl;

            if (impl()->getConeOuterAngle() != 360.0f)
                script << prefix << ".coneOuterAngle "
                       << impl()->getConeOuterAngle() << std::endl;

            if (impl()->getConeOuterVolume() != 1.0f)
                script << prefix << ".coneOuterVolume "
                       << impl()->getConeOuterVolume() << std::endl;
        }
    }

    if (impl()->getRadius() > 0.0f)
        script << prefix << ".soundRadius " << impl()->getRadius() << std::endl;

    if (impl()->getGlobalEffectLevel() != 1.0f)
        script << prefix << ".globalEffectLevel "
               << impl()->getGlobalEffectLevel() << std::endl;

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}


// The 30-slot sound surface. Shape per setter, read from the bodies: store the raw value in the
// mirror field (where one exists), forward (value x its multiplier) to the engine impl. On the
// server every forward lands in a 5-byte null. setIsUsed: 68 B - skipped entirely for
// "_Reload3P"/"_Deploy3P"-suffixed templates (m_isAmbientAnimSound), else impl->setIsUsed() (only
// if an impl is bound - the one guarded impl access in the class) + updateFlags(0x20, 0), the
// "already precached" bit preCache tests.

void SoundObjectTemplate::setIsUsed()
{
    if (m_isAmbientAnimSound)
        return;
    if (m_impl)
        m_impl->setIsUsed();
    updateFlags(0x20, 0);
}

// the comma-list route through the free helpers below.
void SoundObjectTemplate::setFilename(const std::string& filenames)
{
    setTemplateFilenames(m_impl, filenames);
}

// The engine owns an array of C-string filenames; the wrapper joins it with commas.
// Null entries contribute an empty field, and delimiters are based on the reported count rather
// than on whether a pointer was present.
std::string SoundObjectTemplate::getFilename() const
{
    std::ostringstream value;
    int count = 0;
    const char** const filenames = m_impl->getFilenames(&count);
    for (int index = 0; index < count; ++index)
    {
        if (filenames[index])
            value << filenames[index];
        if (index != count - 1)
            value << ',';
    }
    return value.str();
}

// The two setters are instruction-identical apart from the ISoundTemplate slot. A zero-point
// header is forwarded even when parsing failed; the non-empty arm suppresses the forward when the
// stream is bad after reading its point list. This asymmetry is shipped behavior.
void SoundObjectTemplate::setVolumeEnvelope(const std::string& text)
{
    std::istringstream input(text);
    SoundEnvelopeHeader header = {};
    input >> header;

    if (header.pointCount)
    {
        std::vector<SoundEnvelopePoint> points(header.pointCount);
        for (unsigned int index = 0; index < points.size(); ++index)
            input >> points[index];

        if (!input.fail())
        {
            m_impl->setVolumeEnvelope(header.attack, header.sustain, header.release, header.scale,
                                      header.loop, header.pointCount, &points[0]);
        }
        return;
    }

    m_impl->setVolumeEnvelope(header.attack, header.sustain, header.release, header.scale,
                              header.loop, 0, 0);
}

std::string SoundObjectTemplate::getVolumeEnvelope() const
{
    SoundEnvelopeHeader header;
    const SoundEnvelopePoint* const points = m_impl->getVolumeEnvelope(
        &header.attack, &header.sustain, &header.release, &header.scale,
        &header.loop, &header.pointCount);

    if (!points)
        return "0/1/0/1/0/0/";

    std::ostringstream text;
    text << header;
    for (unsigned int index = 0; index < header.pointCount; ++index)
        text << points[index];
    return text.str();
}

void SoundObjectTemplate::setPitchEnvelope(const std::string& text)
{
    std::istringstream input(text);
    SoundEnvelopeHeader header = {};
    input >> header;

    if (header.pointCount)
    {
        std::vector<SoundEnvelopePoint> points(header.pointCount);
        for (unsigned int index = 0; index < points.size(); ++index)
            input >> points[index];

        if (!input.fail())
        {
            m_impl->setPitchEnvelope(header.attack, header.sustain, header.release, header.scale,
                                     header.loop, header.pointCount, &points[0]);
        }
        return;
    }

    m_impl->setPitchEnvelope(header.attack, header.sustain, header.release, header.scale,
                             header.loop, 0, 0);
}

std::string SoundObjectTemplate::getPitchEnvelope() const
{
    SoundEnvelopeHeader header;
    const SoundEnvelopePoint* const points = m_impl->getPitchEnvelope(
        &header.attack, &header.sustain, &header.release, &header.scale,
        &header.loop, &header.pointCount);

    if (!points)
        return "0/1/0.1/3.0/0/0/";

    std::ostringstream text;
    text << header;
    for (unsigned int index = 0; index < header.pointCount; ++index)
        text << points[index];
    return text.str();
}

void SoundObjectTemplate::setLoopCount(int count) { m_impl->setLoopCount(count); }  // no store
int  SoundObjectTemplate::getLoopCount() const    { return m_impl->getLoopCount(); }

void SoundObjectTemplate::setVolume(float v)
{
    m_volume = v;
    m_impl->setVolume(v * m_volumeMultiplier);
}
float SoundObjectTemplate::getVolume() const { return m_volume; }

void SoundObjectTemplate::setPitch(float v)
{
    m_pitch = v;
    m_impl->setPitch(v * m_pitchMultiplier);
}
float SoundObjectTemplate::getPitch() const { return m_pitch; }

void SoundObjectTemplate::setMinDistance(float v)
{
    m_minDistance = v;
    m_impl->setMinDistance(v * m_minDistanceMultiplier);
}
float SoundObjectTemplate::getMinDistance() const { return m_minDistance; }

// the one setter with a computed default: a (near-)zero half-volume distance is replaced
// by 2 x getMinDistance() (virtual - through +0x120) x the minDistance multiplier before the
// forward. The stored mirror keeps the RAW argument either way.
void SoundObjectTemplate::setHalfVolumeDistance(float v)
{
    m_halfVolumeDistance = v;
    float forwarded = v;
    float av = v < 0.0f ? -v : v;                // andnps sign-strip in the ELF
    if (av <= 0.00000011920929f)                 // FLT_EPSILON as the ELF spells it
        forwarded = (getMinDistance() + getMinDistance()) * m_minDistanceMultiplier;
    m_impl->setHalfVolumeDistance(forwarded * m_halfVolumeDistanceMultiplier);
}
float SoundObjectTemplate::getHalfVolumeDistance() const { return m_halfVolumeDistance; }

void SoundObjectTemplate::setReverbLevel(float v)
{
    m_reverbLevel = v;
    m_impl->setReverbLevel(v * m_reverbLevelMultiplier);
}
float SoundObjectTemplate::getReverbLevel() const { return m_reverbLevel; }

void SoundObjectTemplate::setSoundRadius(float v)
{
    m_soundRadius = v;
    m_impl->setRadius(v * m_soundRadiusMultiplier);
}
float SoundObjectTemplate::getSoundRadius() const { return m_soundRadius; }

// The sample pairs address impl->set/getMaxSamples with a hard-coded quality: 2 = low, 1 = medium.
void SoundObjectTemplate::setLowSamples(int n)    { m_impl->setMaxSamples(SoundQuality(2), n); }
int  SoundObjectTemplate::getLowSamples() const   { return m_impl->getMaxSamples(SoundQuality(2)); }
void SoundObjectTemplate::setMediumSamples(int n) { m_impl->setMaxSamples(SoundQuality(1), n); }
int  SoundObjectTemplate::getMediumSamples() const{ return m_impl->getMaxSamples(SoundQuality(1)); }

// a null matrix is ignored; otherwise the impl gets the translation column (m[12..14]) as
// position and the Z-axis column (m[8..10]) as direction.
void SoundObjectTemplate::setRelativeTransformation(const Mat4* m)
{
    if (!m)
        return;
    m_impl->setPosition(*reinterpret_cast<const Vec3*>(&m->m[12]));
    m_impl->setDirection(*reinterpret_cast<const Vec3*>(&m->m[8]));
}

ISoundTemplate* SoundObjectTemplate::impl() const { return m_impl; }        // the +0x150 accessor

// Stores all six multipliers, then reapplies base values through their virtual setters in parameter order.
void SoundObjectTemplate::tweak(float volume, float pitch, float minDistance,
                                float halfVolumeDistance, float reverbLevel, float soundRadius)
{
    m_pitchMultiplier              = pitch;
    m_volumeMultiplier             = volume;
    m_reverbLevelMultiplier        = reverbLevel;
    m_minDistanceMultiplier        = minDistance;
    m_halfVolumeDistanceMultiplier = halfVolumeDistance;
    m_soundRadiusMultiplier        = soundRadius;
    setVolume(m_volume);
    setPitch(m_pitch);
    setMinDistance(m_minDistance);
    setHalfVolumeDistance(m_halfVolumeDistance);
    setReverbLevel(m_reverbLevel);
    setSoundRadius(m_soundRadius);
}


// Splits a comma-separated filename list into pointers backed by the caller's scratch string.
// The scratch string must remain valid while the pointers are used.

void parseTemplateFilenames(std::vector<const char*>* out, std::string* scratch,
                            const std::string& filenames)
{
    out->clear();                                // vector::erase(begin, end) in the ELF
    scratch->assign(filenames);

    std::string::size_type pos = 0;
    for (;;)
    {
        const std::string::size_type comma = scratch->find(',', pos);
        out->push_back(scratch->data() + pos);   // the piece's start, inside the scratch buffer
        if (comma == std::string::npos)
            break;
        (*scratch)[comma] = '\0';                // NUL-punch the comma in place
        pos = comma + 1;
        if (pos == scratch->length())            // comma was the last character: exit without an
            break;                               // empty final piece. As the ELF.
    }
}

void setTemplateFilenames(ISoundTemplate* impl, const std::string& filenames)
{
    if (!impl)
        return;
    static std::string s_scratch;
    // Shared scratch storage; not reentrant.
    std::vector<const char*> pieces;
    parseTemplateFilenames(&pieces, &s_scratch, filenames);
    impl->setFilenames(pieces.empty() ? 0 : &pieces[0], int(pieces.size()));
}

} // namespace hfe
} // namespace dice
