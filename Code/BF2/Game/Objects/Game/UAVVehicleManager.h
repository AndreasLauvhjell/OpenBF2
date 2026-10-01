// Owns each team's active UAV and handles its lifecycle events.

#ifndef GAME_OBJECTS_GAME_UAVVEHICLEMANAGER_H
#define GAME_OBJECTS_GAME_UAVVEHICLEMANAGER_H

#include "../../../Math/Vec3.h"
#include "../../IEventListener.h"

namespace dice {
namespace hfe {
namespace world {

class UAVVehicle;

class UAVVehicleManager : public IEventListener
{
public:
    UAVVehicleManager();

    // Preserve the recovered [handleEvent, D1, D0] table; IEventListener has no virtual dtor.
    virtual void handleEvent(EventCategory category, unsigned int id,
                             EventNode* node, void* userData);
    virtual ~UAVVehicleManager();

    void init();
    void reset();
    void removeAll();
    void removeUAVVehicle(int team);
    void update(float deltaTime);
    void createUAVVehicle(Vec3& position, int team);
    void initUAVVehicle(int team);

    // [CHOSEN] typed view replacing UAVVehicle.cpp's old TU-local layout declaration.
    UAVVehicle* getUAVVehicleForTeam(int team) const
    {
        return static_cast<unsigned int>(team - 1) <= 1u ? m_uavVehicles[team - 1] : 0;
    }

private:
    friend class UAVVehicle;

    /* +0x04 */ UAVVehicle* m_uavVehicles[2];
    /* +0x0C */ bool m_uavActive[2];
};

#if defined(_M_IX86) || defined(__i386__)
static_assert(sizeof(UAVVehicleManager) == 0x10,
              "UAVVehicleManager must retain its recovered x86 layout");
#endif

extern UAVVehicleManager* uavVehicleManager;                  // ds:0x8AADD40, Core/Globals.cpp

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_UAVVEHICLEMANAGER_H
