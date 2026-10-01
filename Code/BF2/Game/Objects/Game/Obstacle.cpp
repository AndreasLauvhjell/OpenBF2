// Path: [INFERRED]. ELF 0x81931C0/3240/3390; PE 0x59BAA0/BA10/BAE0, read in full.
#include "Obstacle.h"
#include "../TemplateLeaves.h"

namespace dice { namespace hfe { namespace world {
Obstacle::Obstacle(const ObstacleTemplate* objectTemplate) : Bundle(objectTemplate) {}
Obstacle::~Obstacle() {}
bool Obstacle::handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                               int, int, CollisionFaceInfo*, CollisionFaceInfo*)
{
    // Dispatch on this object, not on the collider. PE +0x100 / ELF +0x104.
    handleMessage(static_cast<TemplateMessage>(0), 0, false);
    return false;
}
} } }
