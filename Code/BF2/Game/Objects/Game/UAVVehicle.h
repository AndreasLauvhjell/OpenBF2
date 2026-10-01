// Commander UAV drone managed by UAVVehicleManager and CommanderToolsManager.
// [INFERRED] source placement.

#ifndef GAME_OBJECTS_GAME_UAVVEHICLE_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_UAVVEHICLE_H

#include "../Base/Bundle.h"

namespace dice {
namespace hfe {
namespace world {

class UAVVehicleTemplate;
class UAVVehicleManager;

// Flying UAV bundle. The spawning manager must initialize team and turn rate before updates.
class UAVVehicle : public Bundle
{
public:
    explicit UAVVehicle(const UAVVehicleTemplate* objectTemplate);   // read in full
    virtual ~UAVVehicle();                           // (D1) / 51 B (D0) - vptr restore only

    virtual void disableCachedObject();              // slot 9  -
    virtual void enableCachedObject();               // slot 10 -
    virtual void destroy(bool force);                // slot 13 -
    virtual bool handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                                 int, int, CollisionFaceInfo*, CollisionFaceInfo*);
                                                     // slot 45 -
    virtual void handleMessage(TemplateMessage, IPlayer*, bool);
                                                     // slot 65 - (24 = die)

    // Non-virtual, written: pull this drone out of whichever manager slot holds it.
    void destroyUAVVehicle();

    // Updates UAV angular and forward velocity while its commander tool is active and has armor.
    // Otherwise destroys the UAV.
    void update(float dt);                           // recovered above

private:
    friend class UAVVehicleManager;
    /* 0x13C */ const UAVVehicleTemplate* m_template;    // the only ctor-written field
    /* 0x140 */ float m_turnRate;                    // initUAVVehicle writes the bank-derived yaw
                                                     // rate; update feeds it to rotational speed Y
    /* 0x144 */ int   m_team;                        // [CHOSEN] - NOT written by the ctor;
                                                     // destroy/update key on it
};

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_UAVVEHICLE_H
