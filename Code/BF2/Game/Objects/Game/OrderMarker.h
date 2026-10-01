// Path:     attested - both shipped binaries embed "Game/Objects/Game/OrderMarker.cpp".
// Contents: recovered from bf2.i64 and bf2_w32ded.exe.i64.
// OrderMarker is the short-lived client-side owner behind SquadManager's two order-marker slots.
// It is deliberately not an engine object: its first word is a WeakPtr<IObject> control block,
// followed by the non-owning SquadOrder pointer and marker kind.  The constructor either attaches
// a nametag directly to a network target or creates the corresponding world marker object; the
// destructor removes that nametag and destroys the created object, if one survived.

#ifndef GAME_OBJECTS_GAME_ORDERMARKER_H
#define GAME_OBJECTS_GAME_ORDERMARKER_H

#include "../../../Core/WeakPtr.h"

namespace dice {
namespace hfe {

struct SquadOrder;

namespace world {

class IObject;

class OrderMarker
{
public:
    OrderMarker(SquadOrder& order, int markerId);
    ~OrderMarker();

private:
    /* +0x00 */ WeakPtr<IObject> m_object;
    /* +0x04 */ SquadOrder*      m_order;
    /* +0x08 */ int              m_markerId;
};

#if defined(_WIN64) || defined(__LP64__)
// [CHOSEN, not recovered] WeakPtr and SquadOrder* are native addresses; marker ID stays int.
static_assert(sizeof(OrderMarker) == 24, "native x64 order-marker owner");
#else
static_assert(sizeof(OrderMarker) == 12,
              "OrderMarker is the three-word owner allocated with operator new(0xC)");
#endif

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_ORDERMARKER_H
