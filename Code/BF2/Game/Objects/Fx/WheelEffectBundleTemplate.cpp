// Path:     [INFERRED] - beside its header. Contents: recovered from bf2.i64.

#include "WheelEffectBundleTemplate.h"
#include "WheelEffectBundle.h"                      // createObject's product (BF2.exe 0x6457F0)
#include "../../../IO/Stream/IStream.h"
#include "../../Main/Game.h"                        // isDedicated - BF2.exe 0x440510
#include <sstream>

#include "../../../Core/Ids.h"

namespace dice {
namespace hfe {
namespace world {

// SimpleObjectTemplate base, EffectProperties base at +0x164, then the one flag - true.
// NO updateFlags call, exactly like its AirDraft sibling.
WheelEffectBundleTemplate::WheelEffectBundleTemplate()
{
    m_emitDirectionZFromSpeedDof = 1;
}

// (D1): the two bases' teardown - the compiler's job in our build.
WheelEffectBundleTemplate::~WheelEffectBundleTemplate()
{
}

unsigned int WheelEffectBundleTemplate::getClassID() const
{
    return CID_WheelEffectBundleTemplate;
}

// 5 bytes - empty, as shipped.
void WheelEffectBundleTemplate::templateInitialize()
{
}

// three CIDs, flat; no chain to SimpleObjectTemplate's implementation.
void* WheelEffectBundleTemplate::queryImplementation(unsigned int classId)
{
    if (classId == CID_WheelEffectBundleTemplate ||
        classId == CID_SimpleObjectTemplate ||
        classId == CID_ObjectTemplate)
        return this;
    return 0;
}

// Creates a wheel-effect bundle unless running dedicated; dedicated images fold this to null.
// Keep the client's runtime isDedicated check.
IObject* WheelEffectBundleTemplate::createObject() const
{
    if (isDedicated())
        return 0;
    return new WheelEffectBundle(this);
}

void WheelEffectBundleTemplate::setEmitDirectionZFromSpeedDof(bool v)
{
    m_emitDirectionZFromSpeedDof = v;
}

bool WheelEffectBundleTemplate::getEmitDirectionZFromSpeedDof() const
{
    return m_emitDirectionZFromSpeedDof != 0;
}

// ELF 8215F90 / dedicated PE 5FC470.
bool WheelEffectBundleTemplate::makeScript(io::IStream* stream)
{
    if (!SimpleObjectTemplate::makeScript(stream) || !EffectProperties::makeScript(stream))
        return false;
    std::ostringstream out;
    out << "ObjectTemplate.emitDirectionZFromSpeedDof " << (m_emitDirectionZFromSpeedDof != 0) << std::endl;
    const std::string text = out.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.size())) == text.size();
}

} // namespace world
} // namespace hfe
} // namespace dice
