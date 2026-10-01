// Reports a client's connection class so the server can recalculate bandwidth allocation.
// Values 0..6 use three wire bits; client execution uses the inherited no-op.
// [UNRESOLVED] The original connection-class enumerator names are unknown.

#ifndef GAME_GAMEEVENTS_CONNECTIONTYPEEVENT_H     // [INFERRED] guard style
#define GAME_GAMEEVENTS_CONNECTIONTYPEEVENT_H

#include "../Common/GameEvent.h"
#include "../IGameServer.h"                      // ConnectionType

namespace dice {
namespace hfe {

class ConnectionTypeEvent : public GameEvent                 // 0x10
{
public:
    // The number of valid ConnectionType values, read from the two width loops - see the header
    // block. Named here so the writer's bound and the reader's width cannot drift apart.
    enum { CONNECTION_TYPE_COUNT = 7, CONNECTION_TYPE_MAX = CONNECTION_TYPE_COUNT - 1 };

    explicit ConnectionTypeEvent(ConnectionType type);       // [INFERRED] - no ELF constructor
    ConnectionTypeEvent() : m_connectionType(0) {}           // the maker's form
    virtual ~ConnectionTypeEvent() {}

    virtual int  getType() const;                            // GEConnectionType (3)
    virtual void logInfo(bool, int);                         // empty
    virtual bool executeServer(int connectionId);
    virtual bool deSerialize(io::BitStream* stream);
    virtual bool serialize(io::BitStream* stream);           // mostly the assert

    // executeClient is deliberately NOT declared - the slot is inherited.

protected:
    /* +0x0C */ unsigned char m_connectionType;
};

} // namespace hfe
} // namespace dice

#endif // GAME_GAMEEVENTS_CONNECTIONTYPEEVENT_H
