// [INFERRED] header path beside Squad. Shared 48-byte order value measured in
// both PE/ELF Squad and IssueSquadOrderEvent, and the client HUD at+270.
// Preserve ushort/byte padding; construction does not initialize this aggregate.
#ifndef GAME_GAMELOGIC_SQUADMANAGER_SQUADORDER_H
#define GAME_GAMELOGIC_SQUADMANAGER_SQUADORDER_H
#include "../../../Math/Vec3.h"
namespace dice { namespace hfe {
struct SquadOrder {
    int orderId;                  // +00
    unsigned short targetNetId;   // +04
    Vec3 points[3];               // +08..2B
    bool flag2C;                  // +2C
};
} }
#endif
