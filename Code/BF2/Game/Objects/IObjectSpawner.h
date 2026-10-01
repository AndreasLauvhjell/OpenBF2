// Team ownership and object-spawner control interfaces.
// Spawn offsets are passed and returned by value. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_IOBJECTSPAWNER_H     // [INFERRED] guard style
#define GAME_OBJECTS_IOBJECTSPAWNER_H

#include "../../Core/IBase.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {
namespace world {

// IID_ITeamObject = 50356 (Core/Ids.h). Two slots past the quartet.
class ITeamObject : public IBase
{
public:
    virtual void setTeam(int team) = 0;              // slot 4
    virtual int  getTeam() = 0;                      // slot 5
protected:
    ~ITeamObject() {}
};

// IID_IObjectSpawner = 50386 (Core/Ids.h). Twelve more, in the secondary table's order.
class IObjectSpawner : public ITeamObject
{
public:
    virtual void setActive(bool active) = 0;         // slot 6
    virtual bool getActive() = 0;                    // slot 7
    virtual void setControledById(int id) = 0;       // slot 8 dice's spelling, one 'l'
    virtual int  getControledById() = 0;             // slot 9
    virtual void setTeamOnStart(unsigned int team) = 0;   // slot 10
    virtual int  getTeamOnStart() = 0;               // slot 11
    virtual void setControlPointId(int id) = 0;      // slot 12
    virtual int  getControlPointId() = 0;            // slot 13
    virtual void setVisibleTeam(int team) = 0;       // slot 14
    virtual int  getVisibleTeam() = 0;               // slot 15
    virtual void setSpawnOffset(Vec3 offset) = 0;    // slot 16 by value
    virtual Vec3 getSpawnOffset() = 0;               // slot 17 by value
protected:
    ~IObjectSpawner() {}
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_IOBJECTSPAWNER_H
