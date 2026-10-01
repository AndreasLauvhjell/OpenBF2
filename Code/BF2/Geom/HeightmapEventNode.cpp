// Path:     [INFERRED] - see Geom/HeightmapEventNode.h.
// Contents: the whole translation unit, (ELF 0x84A8900..0x84A8A5F, PE
//           bf2_w32ded 0x756520 / 0x756550). Engine code: the dedicated server links it too.

#include "HeightmapEventNode.h"

#include <iostream>     // the TU's static initialiser (ELF 0x84A89B0) is the <iostream> guard

namespace dice {
namespace hfe {
namespace geom {

// ELF 0x84A8930 (C1) / 0x84A8900 (C2), 47 B; PE 0x756520. EventNode's m_next = 0, the vptr, the
// type, then the four area floats zeroed.
HeightmapEventNode::HeightmapEventNode(unsigned int type)
    : SimpleEventNode(type)
{
    m_affectedArea.x = 0.0f;
    m_affectedArea.y = 0.0f;
    m_affectedArea.z = 0.0f;
    m_affectedArea.w = 0.0f;
}

// ELF 0x84A8A40 (D1) / 0x84A8A50 (D0): nothing of its own - the vptr goes back to EventNode's.
HeightmapEventNode::~HeightmapEventNode()
{
}

// ELF 0x84A8960, 64 B; PE 0x756550. A straight four-float copy.
void HeightmapEventNode::setAffectedArea(const Vec4& area)
{
    m_affectedArea = area;
}


const Vec4& HeightmapEventNode::getAffectedArea() const
{
    return m_affectedArea;
}

} // namespace geom
} // namespace hfe
} // namespace dice
