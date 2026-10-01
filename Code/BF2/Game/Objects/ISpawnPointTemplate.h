// Authored spawn settings used by SpawnManager and SpawnPoint.
// Default rotation assignment does nothing; the default getter returns a zero vector. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_ISPAWNPOINTTEMPLATE_H     // [INFERRED] guard style
#define GAME_OBJECTS_ISPAWNPOINTTEMPLATE_H

#include "../../Core/IBase.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {

class ISpawnPointTemplate : public IBase
{
public:
    virtual void  setSpawnPositionOffset(Vec3&) = 0;            // 4  (+0x10)
    virtual Vec3& getSpawnPositionOffset() = 0;                 // 5  (+0x14)
    virtual void  setSpawnRotation(Vec3&) {}                    // 6  (+0x18) - the interface's own empty body
    virtual Vec3  getSpawnRotation()                            // 7  (+0x1C) - the interface's own body: (0,0,0)
    {
        Vec3 zero; zero.x = 0.0f; zero.y = 0.0f; zero.z = 0.0f;
        return zero;
    }
    virtual void  setSpawnPreventionDelay(float) = 0;           // 8  (+0x20)
    virtual float getSpawnPreventionDelay() = 0;                // 9  (+0x24)
    virtual void  setGroup(unsigned int) = 0;                   // 10 (+0x28)
    virtual unsigned int getGroup() = 0;                        // 11 (+0x2C)
    virtual void  setControlPointId(unsigned int) = 0;          // 12 (+0x30)
    virtual unsigned int getControlPointId() const = 0;         // 13 (+0x34) - SpawnManager::addSpawnPoint's read
    virtual void  setActive(bool) = 0;                          // 14 (+0x38)
    virtual bool  getActive() = 0;                              // 15 (+0x3C)
    virtual void  setEnterOnSpawn(bool) = 0;                    // 16 (+0x40)
    virtual bool  getEnterOnSpawn() = 0;                        // 17 (+0x44)
    virtual void  setAIEnterOnSpawn(bool) = 0;                  // 18 (+0x48)
    virtual bool  getAIEnterOnSpawn() = 0;                      // 19 (+0x4C)
    virtual void  setSpawnAsParaTroper(bool) = 0;               // 20 (+0x50)  [sic] dice's spelling
    virtual bool  getSpawnAsParaTroper() = 0;                   // 21 (+0x54)
    virtual void  setMinSpawnHeight(float) = 0;                 // 22 (+0x58)
    virtual float getMinSpawnHeight() const = 0;                // 23 (+0x5C)
    virtual void  setScatterSpawnPositions(bool) = 0;           // 24 (+0x60)
    virtual bool  getScatterSpawnPositions() = 0;               // 25 (+0x64)
    virtual void  setOnlyForAI(bool) = 0;                       // 26 (+0x68)
    virtual bool  getOnlyForAI() const = 0;                     // 27 (+0x6C)
    virtual void  setOnlyForHuman(bool) = 0;                    // 28 (+0x70)
    virtual bool  getOnlyForHuman() const = 0;                  // 29 (+0x74)
    virtual void  setAllowSpawnCloseToVehicle(bool) = 0;        // 30 (+0x78)
    virtual bool  getAllowSpawnCloseToVehicle() const = 0;      // 31 (+0x7C)
};

} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_ISPAWNPOINTTEMPLATE_H
