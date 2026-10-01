// Vehicle entry marker linked to the nearest parent player-control object during init.
// enter accepts an unoccupied seat or missing link without using its arguments. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_ENTRYPOINT_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_ENTRYPOINT_H

#include "../Base/SimpleObject.h"
#include "../IEntryPointObject.h"

namespace dice {
namespace hfe {
namespace world {

class EntryPointTemplate;
class IPlayerControlObject;

class EntryPoint : public SimpleObject, public IEntryPointObject     // IEntryPointObject = the +0x138 sub-object
{
public:
    explicit EntryPoint(const EntryPointTemplate* objectTemplate);
    virtual ~EntryPoint();

    // IBase (both bases share the +0x13C count; the -312 thunks land here)
    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    virtual void init();
    virtual void handleUpdate(float, unsigned int);

    // IEntryPointObject
    virtual bool enter(IPlayer* player, int& seat);
    virtual bool exit(IPlayer* player);
    virtual IPlayer* getPlayer() const;                              // 0
    virtual IPlayerControlObject* getLinkedPlayerControlObject();

protected:
    /* 0x13C */ int                   m_refCount;
    /* 0x140 */ IPlayerControlObject* m_linkedPlayerControlObject;
};

// Finds an available vehicle seat, or an AI-occupied fallback when allowed.
// useEntryPointRadius selects the marker's own reach; radius still bounds the grid search.
// requireLineOfSight rejects blocked markers. AI players force a 15-unit search restricted to their courier's vehicle.
class IPlayer;

IPlayerControlObject* BFfindEntryPoint(const Vec3& position, float radius, int team,
                                       IPlayer* player, bool useEntryPointRadius,
                                       bool acceptAiOccupied, bool requireLineOfSight);

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_ENTRYPOINT_H
