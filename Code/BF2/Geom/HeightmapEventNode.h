// ECEditor heightmap-modified event carrying the affected rectangle.
// Coordinate units follow the producer: brushes use cluster/world coordinates, while
// setHeightData uses the tile's centre-origin grid extent. All rebuilt users share this layout.


#ifndef GEOM_HEIGHTMAPEVENTNODE_H
#define GEOM_HEIGHTMAPEVENTNODE_H

#include "../Core/EventManager.h"
#include "../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace geom {

class HeightmapEventNode : public SimpleEventNode
{
public:
    explicit HeightmapEventNode(unsigned int type);
    virtual ~HeightmapEventNode();

    void setAffectedArea(const Vec4& area);
    const Vec4& getAffectedArea() const;                 // 0x84A89A0, 11 B: `lea eax, [this+20h]`

private:
    Vec4 m_affectedArea;                                 // +0x20 (ELF): x0, z0, x1, z1
};

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // GEOM_HEIGHTMAPEVENTNODE_H
