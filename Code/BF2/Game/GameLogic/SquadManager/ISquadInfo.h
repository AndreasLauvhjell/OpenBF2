// Ten-slot squad interface with no IBase base. Protected nonvirtual destruction
// keeps derived destructor slots after this surface. [INFERRED] header path.

#ifndef GAME_GAMELOGIC_SQUADMANAGER_ISQUADINFO_H
#define GAME_GAMELOGIC_SQUADMANAGER_ISQUADINFO_H

#include <string>

namespace dice {
namespace hfe {

namespace world { class IPlayer; }

enum OrderType : int;       // opaque with a fixed underlying type: MSVC's default for a plain enum, and what
enum SquadOrderType : int;  // Explicit underlying type is required for an opaque enum on GCC.
struct SquadOrder;

class ISquadInfo
{
public:
    virtual const std::string& getSquadName() = 0;                       // +0x00
    virtual bool getIsPrivate() = 0;                                     // +0x04
    virtual bool getIsAccepted(SquadOrderType, OrderType) = 0;           // +0x08
    virtual int getSquadId() = 0;                                        // +0x0C
    virtual SquadOrder* getSquadOrders() = 0;                            // +0x10
    virtual SquadOrder* getCommanderOrders() = 0;                        // +0x14
    virtual SquadOrder* getRequest(SquadOrderType) = 0;                  // +0x18
    virtual int getSpawnGroupId() = 0;                                   // +0x1C
    virtual void addSquadLeaderSpawnPoint(world::IPlayer*) = 0;          // +0x20
    virtual void removeSquadLeaderSpawnPoint(world::IPlayer*) = 0;       // +0x24

protected:
    ~ISquadInfo() {}
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMELOGIC_SQUADMANAGER_ISQUADINFO_H
