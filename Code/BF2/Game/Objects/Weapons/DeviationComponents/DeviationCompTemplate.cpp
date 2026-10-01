// Path:     [INFERRED] beside DeviationCompTemplate.h.
// Contents: implementation of the four grouped setters. All four read end to end in bf2.i64.
// The store order below is the binary's, not declaration order, and that is the whole point of
// these four functions — see the header for why (first argument to the highest offset, confirmed
// independently by makeScript's emission order).

#include "DeviationCompTemplate.h"

#include <sstream>
#include <string>

#include "../../../../Core/Debug.h"
#include "../../../../Core/Ids.h"
#include "../../../../IO/Stream/IStream.h"

namespace dice {
namespace hfe {
namespace world {

DeviationCompTemplate::DeviationCompTemplate()
    : m_fireDevAdd(0.0f)
    , m_fireDevSub(0.0f)
    , m_fireDevMax(0.0f)
    , m_minDev(0.0f)
    , m_subProjectileDev(0.0f)
{
}

unsigned int DeviationCompTemplate::getClassID() const
{
    return CID_DeviationCompTemplate;
}

unsigned int DeviationCompTemplate::getInterfaceID()
{
    return IID_IWeaponDeviationCompTemplate;
}

IObjectComponent* DeviationCompTemplate::createComponent()
{
    Debug(DEBUGTYPE_6,
          "Game/Objects/Weapons/DeviationComponents/DeviationComp.h", 107, "")
        << "Virtual type";
    return 0;
}

void DeviationCompTemplate::objectCreated(IObject*) {}
void DeviationCompTemplate::objectDestroyed(IObject*) {}
void DeviationCompTemplate::setTemplate(IObjectTemplate*) {}

void* DeviationCompTemplate::queryImplementation(unsigned int classId)
{
    return classId == CID_DeviationCompTemplate ? this : 0;
}

DeviationCompTemplate::~DeviationCompTemplate()
{
}

// mov eax, [ebp+arg_4];  mov [edx+10h], eax      <- first argument, highest offset
// mov eax, [ebp+arg_8];  mov [edx+8],   eax
// mov eax, [ebp+arg_C];  mov [edx+0Ch], eax
void DeviationCompTemplate::setFireDev(float max, float add, float sub)
{
    m_fireDevMax   = max;      // +0x10 — written first
    m_fireDevAdd   = add;      // +0x08
    m_fireDevSub   = sub;      // +0x0C
}

// 32 bytes. arg1 -> +0x2C, then +0x20, +0x24, +0x28.
void SoldierDeviationCompTemplate::setTurnDev(float max, float addPitch, float addYaw, float sub)
{
    m_turnDevMax      = max;        // +0x2C
    m_turnDevAddPitch = addPitch;   // +0x20
    m_turnDevAddYaw   = addYaw;     // +0x24
    m_turnDevSub      = sub;        // +0x28
}

// 32 bytes. Identical shape one block up.
void SoldierDeviationCompTemplate::setSpeedDev(float max, float addForward,
                                                float addStrafe, float sub)
{
    m_speedDevMax        = max;          // +0x3C
    m_speedDevAddForward = addForward;   // +0x30
    m_speedDevAddStrafe  = addStrafe;    // +0x34
    m_speedDevSub        = sub;          // +0x38
}

// 26 bytes. Three arguments, same rule.
void SoldierDeviationCompTemplate::setMiscDev(float max, float addJump, float sub)
{
    m_miscDevMax     = max;       // +0x48
    m_miscDevAddJump = addJump;   // +0x40
    m_miscDevSub     = sub;       // +0x44
}

bool DeviationCompTemplate::makeScript(io::IStream* stream)
{
    std::ostringstream script;
    if (m_fireDevAdd != 0.0f || m_fireDevSub != 0.0f || m_fireDevMax != 0.0f)
    {
        script << "ObjectTemplate.deviation.setFireDev "
               << m_fireDevMax << ' ' << m_fireDevAdd << ' ' << m_fireDevSub << std::endl;
    }
    if (m_minDev != 0.0f)
        script << "ObjectTemplate.deviation.minDev " << m_minDev << std::endl;
    if (m_subProjectileDev != 0.0f)
    {
        script << "ObjectTemplate.deviation.subProjectileDev "
               << m_subProjectileDev << std::endl;
    }

    const std::string text = script.str();
    return stream->write(text.c_str(), static_cast<unsigned int>(text.length())) == text.length();
}

} // namespace world
} // namespace hfe
} // namespace dice
