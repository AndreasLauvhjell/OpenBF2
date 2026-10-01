// Path: [INFERRED] beside the other world runtime products; no __FILE__ string found.
#ifndef GAME_OBJECTS_GAME_OBSTACLE_H
#define GAME_OBJECTS_GAME_OBSTACLE_H

#include "../Base/Bundle.h"

namespace dice { namespace hfe { namespace world {
class ObstacleTemplate;
class Obstacle : public Bundle
{
public:
    explicit Obstacle(const ObstacleTemplate*);
    virtual ~Obstacle();
    virtual bool handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                                 int, int, CollisionFaceInfo*, CollisionFaceInfo*);
};
} } }
#endif
