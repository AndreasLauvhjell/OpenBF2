// Shared networkable behavior and prediction-mode conversion. [INFERRED] Source placement.

#include "INetworkable.h"
#include "NetworkableInfo.h"     // setNetworkableInfo reads the policy's base priority; PredictionMode

#include <istream>
#include <ostream>
#include <string>

namespace dice {
namespace hfe {
namespace io {

// Six stores, then return — no base-class call, no allocation.

// The order below is the constructor's own. It is preserved even though C++ would normally run the
// initialiser list in declaration order, because the observed sequence is the ground truth and a
// future reader comparing against the disassembly should see the same shape.
INetworkable::INetworkable()
{
    m_networkableId   = 0;        // +0x04, a word store: `mov word ptr [eax+4], 0`
    m_networkableInfo = 0;        // +0x08
    m_ghostActive     = true;     // +0x0C, `mov byte ptr [eax+0Ch], 1`
    m_basePriority    = 1.0f;     // +0x10, stored as the immediate 0x3F800000
    m_isInNetwork     = true;     // +0x14, `mov byte ptr [eax+14h], 1`
    m_networkableObject = 0;      // +0x18
}

// Sets the replication policy and copies its base priority. Null clears the policy but leaves priority unchanged.
void INetworkable::setNetworkableInfo(INetworkableInfo* info)
{
    m_networkableInfo = info;
    if (info)
        m_basePriority = info->getBasePriority();
}

// the Itanium D2/D1/D0 triple. The D0 form is 21 bytes and
// tail-jumps to operator delete; the body itself does nothing.
INetworkable::~INetworkable()
{
}

// Empty by design — a default for subclasses to override.
void INetworkable::addedToManager()
{
}

// `mov word ptr [eax+4], 0` and return.

// Clearing the id is what marks the object as no longer registered: `NetworkManager::addNetworkable`
// tests this field on entry and rejects a nonzero one with "Networkable already added to network
// manager". So 0 is the not-registered sentinel, and this is what restores it.
void INetworkable::removedFromManager()
{
    m_networkableId = 0;
}

// Empty — note this is INetworkable's reset, not NetworkableBase's, which does
// real work. See NetworkableBase.cpp.
void INetworkable::reset()
{
}

// Two instructions of work: `mov [eax+0Ch], dl`.
void INetworkable::setGhostActive(bool active)
{
    m_ghostActive = active;
}

// Returns zero by default. Components with timing histories override this method.
float INetworkable::getAverageUpdateDeltaTime() const
{
    return 0.0f;
}

// `xor eax,eax / retn`. Another override-me default.
unsigned int INetworkable::getMinimumBitSize() const
{
    return 0;
}

// Converts prediction mode names. Unknown input becomes PMNone without setting failbit; unknown values print PMNone.
std::istream& operator>>(std::istream& in, PredictionMode& mode)
{
    std::string token;
    in >> token;
    mode = PMNone;
    if (token.compare("PMNone") == 0)             mode = PMNone;
    else if (token.compare("PMLinear") == 0)      mode = PMLinear;
    else if (token.compare("PMCubic") == 0)       mode = PMCubic;
    else if (token.compare("PMUsePhysics") == 0)  mode = PMUsePhysics;
    return in;
}

std::ostream& operator<<(std::ostream& out, PredictionMode mode)
{
    if (mode == PMLinear)           out << "PMLinear";
    else if (mode == PMCubic)       out << "PMCubic";
    else if (mode == PMUsePhysics)  out << "PMUsePhysics";
    else                            out << "PMNone";
    return out;
}

} // namespace io
} // namespace hfe
} // namespace dice
