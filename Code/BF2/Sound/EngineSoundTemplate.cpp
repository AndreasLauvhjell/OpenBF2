// Path:     [INFERRED] - beside its header. Contents: recovered from bf2.i64.

#include "EngineSoundTemplate.h"

#include "../Game/Utility/Vars.h"

namespace dice {
namespace hfe {

// clear the two bools, then tail-call reset().
EngineSoundTemplate::EngineSoundTemplate()
{
    m_byte00 = 0;
    m_byte01 = 0;
    reset();
}


// Copies current Vars values into the template; does not retain pointers to them.

void EngineSoundTemplate::reset()
{
    m_swayFrequencyFirst = *Vars::getFloat("engine-audio-sway-frequency-first", 80.0f);
    m_swayFrequencyLast  = *Vars::getFloat("engine-audio-sway-frequency-last",  35.0f);
    m_swayAmplitudeFirst = *Vars::getFloat("engine-audio-sway-amplitude-first", 0.05f);
    m_swayAmplitudeLast  = *Vars::getFloat("engine-audio-sway-amplitude-last",  0.01f);
    // this one's var name is an unnamed rodata address in the IDB; the spelling is [CHOSEN] to
    // match its documented sibling, the default is read. See the header.
    m_swayTimeFirst      = *Vars::getFloat("engine-audio-sway-time-first",      0.7f);
    m_swayTimeLast       = *Vars::getFloat("engine-audio-sway-time-last",       0.7f);
    m_materialTweak      = *Vars::getFloat("engine-audio-material-tweak",       10.0f);
}

} // namespace hfe
} // namespace dice
