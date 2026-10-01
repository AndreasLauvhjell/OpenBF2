// Commander vehicle-drop object.

#ifndef GAME_OBJECTS_GAME_DROPVEHICLE_H
#define GAME_OBJECTS_GAME_DROPVEHICLE_H

#include "../Base/SimpleObject.h"

namespace dice {
namespace hfe {
namespace world {

class DropVehicleTemplate;

// The controller object for a commander-dropped vehicle. It follows the vehicle while airborne,
// removes the parachute when landing is detected, broadcasts ECGameLogic/23, and eventually
// removes an unused landed PCO. The vehicle and stationary-time fields are deliberately not
// initialized by either shipped constructor; setVehicle() supplies the former before updates run.
class DropVehicle : public SimpleObject
{
public:
    explicit DropVehicle(const DropVehicleTemplate* objectTemplate);
    virtual ~DropVehicle();

    virtual void destroy(bool force);
    virtual void handleUpdate(float deltaTime, unsigned int updateFlags);

    void setVehicle(IObject* vehicle);

private:
    friend class CommanderToolState;

    /* ELF +0x138 / PE +0x150 */ const DropVehicleTemplate* m_dropVehicleTemplate;
    /* ELF +0x13C / PE +0x154 */ IObject* m_vehicle;       // ctor leaves uninitialized
    /* ELF +0x140 / PE +0x158 */ IObject* m_parachute;
    /* ELF +0x144 / PE +0x15C */ int m_onGroundCount;
    /* ELF +0x148 / PE +0x160 */ bool m_vehicleExpired;   // initialized but otherwise unused
    /* ELF +0x149 / PE +0x161 */ char m_padVehicleExpired[3];
    /* ELF +0x14C / PE +0x164 */ int m_unusedVehicleCount;
    /* ELF +0x150 / PE +0x168 */ int m_team;
    /* ELF +0x154 / PE +0x16C */ bool m_unusedVehicleExpired;
    /* ELF +0x155 / PE +0x16D */ bool m_landed;
    /* ELF +0x156 / PE +0x16E */ char m_padLanded[2];
    /* ELF +0x158 / PE +0x170 */ Vec3 m_previousPosition;
    /* ELF +0x164 / PE +0x17C */ float m_stationaryTime;   // ctor leaves uninitialized
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_DROPVEHICLE_H
