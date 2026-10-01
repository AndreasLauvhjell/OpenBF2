// Exposes commander actions to the AI.
// Preserve virtual declaration order. [INFERRED] Return types require validation where implementations remain incomplete.
// [INFERRED] Source placement.

#ifndef AI_AIINTERFACE_BFKNOWLEDGE_BFCOMMANDER_H
#define AI_AIINTERFACE_BFKNOWLEDGE_BFCOMMANDER_H

#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "AIAdapterTypes.h"

namespace dice {
namespace hfe {

class IBase;

namespace ai {

class IBFCommander
{
public:
    IBFCommander();
    virtual ~IBFCommander();

    virtual int applyAsCommander(IAIPlayer* player) = 0;
    virtual int retireAsCommander(IAIPlayer* player) = 0;
    virtual int giveAttackOrder(const Vec3& position, int a, int b, IAIObject* target) = 0;
    virtual int giveMoveOrder(const Vec3& position, int a, int b, IAIObject* target) = 0;
    virtual int giveDefendOrder(const Vec3& position, int a, int b, IAIObject* target) = 0;
    virtual void sendAttackMessage(int a, int b) = 0;
    virtual void sendDefendMessage(int a, int b) = 0;
    virtual int fireArtillery(int a, int b, Vec2 position) = 0;
    virtual int sendUav(int a, int b, Vec2 position) = 0;
    virtual int dropSupply(int a, int b, Vec2 position) = 0;
    virtual int dropVehicle(int a, int b, Vec2 position) = 0;
    virtual int denyArtilleryRequest(int a, int b) = 0;
    virtual bool gameHasArtillery(int team) = 0;
    virtual bool anyArtilleryPiecesLeft(int team) = 0;
    virtual int ArtilleryUnavailable(int a, int b) = 0;
    virtual int denySupplyRequest(int a, int b) = 0;
    virtual int denyVehicleDropRequest(int a, int b) = 0;
    virtual int denyUavRequest(int a, int b) = 0;
};

class BFCommander : public IBFCommander
{
public:
    BFCommander();
    virtual ~BFCommander();

    virtual int applyAsCommander(IAIPlayer* player);
    virtual int retireAsCommander(IAIPlayer* player);
    virtual int giveAttackOrder(const Vec3& position, int a, int b, IAIObject* target);
    virtual int giveMoveOrder(const Vec3& position, int a, int b, IAIObject* target);
    virtual int giveDefendOrder(const Vec3& position, int a, int b, IAIObject* target);
    virtual void sendAttackMessage(int a, int b);
    virtual void sendDefendMessage(int a, int b);
    virtual int fireArtillery(int a, int b, Vec2 position);
    virtual int sendUav(int a, int b, Vec2 position);
    virtual int dropSupply(int a, int b, Vec2 position);
    virtual int dropVehicle(int a, int b, Vec2 position);
    virtual int denyArtilleryRequest(int a, int b);
    virtual bool gameHasArtillery(int team);
    virtual bool anyArtilleryPiecesLeft(int team);
    virtual int ArtilleryUnavailable(int a, int b);
    virtual int denySupplyRequest(int a, int b);
    virtual int denyVehicleDropRequest(int a, int b);
    virtual int denyUavRequest(int a, int b);
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIINTERFACE_BFKNOWLEDGE_BFCOMMANDER_H
