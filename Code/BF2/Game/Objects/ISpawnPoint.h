// Spawn-system view of a spawn point. Its virtual order differs from the concrete class's primary table.
// Keep getObjectToEnter after the enter-on-spawn getters. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_ISPAWNPOINT_H     // [INFERRED] guard style
#define GAME_OBJECTS_ISPAWNPOINT_H

#include "../../Core/IBase.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {

namespace world { class IObject; }

class ISpawnPoint : public IBase
{
public:
    virtual bool  spawn(world::IObject* object, float) = 0;         // slot 4 (+0x10)
    virtual bool  isOccupied(float) const = 0;                       // slot 5 (+0x14)
    virtual void  setActive(bool active) = 0;                        // slot 6 (+0x18)
    virtual bool  getActive(bool isAI, int team) = 0;                // slot 7 (+0x1C)
    virtual bool  getEnterOnSpawn() = 0;                             // slot 8 (+0x20)
    virtual bool  getAIEnterOnSpawn() = 0;                           // slot 9 (+0x24)
    virtual world::IObject* getObjectToEnter(bool, int) = 0;         // slot 10 (+0x28)
    virtual void  setSpawnGroupId(int groupId) = 0;                  // slot 11 (+0x2C)
    virtual int   getSpawnGroupId() = 0;                             // slot 12 (+0x30)
    // Returns false after all attempts fail, leaving out at the failure sentinel.
    virtual bool  getSpawnPosition(Vec3& out, bool checkAI) = 0;     // slot 13 (+0x34)
    virtual const Vec3& getSpawnRotation() = 0;                      // slot 14 (+0x38)
    virtual void  setOverrideEnterOnSpawn(bool) = 0;                 // slot 15 (+0x3C)
    virtual bool  getOverrideEnterOnSpawn() = 0;                     // slot 16 (+0x40)
    virtual void  setProneOnSpawn(bool) = 0;                         // slot 17 (+0x44)
    virtual bool  getProneOnSpawn() = 0;                             // slot 18 (+0x48)
    virtual bool  getOnlyForAI() const = 0;                          // slot 19 (+0x4C)
    virtual bool  getOnlyForHuman() const = 0;                       // slot 20 (+0x50)
protected:
    ~ISpawnPoint() {}
};

} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_ISPAWNPOINT_H
