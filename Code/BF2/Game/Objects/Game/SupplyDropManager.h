// Supply-drop singleton with per-team request throttling and active drop timers.
// Release clears the singleton pointer but leaves the startup service alias invalid after shutdown.
// [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_SUPPLYDROPMANAGER_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_SUPPLYDROPMANAGER_H

#include <map>

namespace dice {
namespace hfe {

namespace Math { struct Vec2; struct Vec3; }             // Math/Vec3.h - the hfe-level names are typedefs
typedef Math::Vec2 Vec2;
typedef Math::Vec3 Vec3;

namespace world {

class IObject;

class SupplyDropManager
{
public:
    SupplyDropManager();
    virtual ~SupplyDropManager();

    static SupplyDropManager* Instance();
    static void               Release();
    int  reset();                                            // returns 0

    bool requestSupplyDrop(unsigned char team, Vec2& where);
    bool spawnSupplyDrop(unsigned char team, const Vec2& where);
    bool spawnSupplyDrop(unsigned char team, const Vec3& where);
    void reportSupplyDrop(unsigned char team, const Vec3& where);
    bool handleSuppliesLanding(IObject* supplyObject, IObject* collidedWith);
    bool collidedWithSpawningObject(IObject* object);
    bool disableChute(IObject* supplyObject, IObject* chute);
    bool destroyCrate(IObject* crate);
    bool addCrate(int id);
    bool removeCrate(int id);
    void update();

    /* +0x04 */ std::map<int, float> m_dropTimers;           // [CHOSEN name]
    /* +0x10 */ float m_lastDropTimes[2];                    // teams 1 and 2

private:
    static SupplyDropManager* m_pInstance;                   // ds:0x8AADAA8
};

extern SupplyDropManager* supplyDropManager;                 // ds:0x8AADAAC (unnamed) - Core/Globals.cpp

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_SUPPLYDROPMANAGER_H
