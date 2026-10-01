// Spawn-group creation/removal events with wire types 57 and 58. Creation carries
// client group id, team/eligibility flags, position, and network id; removal carries
// group and network ids. [INFERRED] Header placement.

#ifndef GAME_COMMON_SPAWNGROUPEVENTS_H     // [INFERRED] guard style
#define GAME_COMMON_SPAWNGROUPEVENTS_H

#include "GameEvent.h"

namespace dice {
namespace hfe {

namespace io { class BitStream; }

class CreateSpawnGroupEvent : public GameEvent               // 0x1C
{
public:
    // GameEventMakerMaker<T>::createEvent() calls new T(), so registered events need this
    // default constructor. The ELF inlines it at the maker site and zeroes the members
    // deSerialize overwrites, as in StringBlockEvent.
    CreateSpawnGroupEvent();

    CreateSpawnGroupEvent(unsigned char spawnGroupId, unsigned short netId, int team, bool active,
                          bool aiOnly, bool hasSpawnPoints, unsigned char x, unsigned char z);   // 122 B
    virtual ~CreateSpawnGroupEvent() {}                       // 27 / 40 B - vptr restore + GameEvent's

    virtual int  getType() const;                             // 10 B - GECreateSpawnGroupEvent (57)
    virtual void logInfo(bool, int);                          // 5 B - empty
    virtual void eventReceivedByRemote(void*);                // 5 B - empty
    virtual void executeClient();                             // 142 B - the client arm (IID_IGameClient)
    virtual bool serialize(io::BitStream* stream);            // +0x30, 460 B - the wire
    virtual bool deSerialize(io::BitStream* stream);          // +0x2C, 350 B -

    /* +0x0C */ unsigned char  m_spawnGroupId;
    /* +0x10 */ int            m_team;
    /* +0x14 */ bool           m_active;
    /* +0x15 */ bool           m_aiOnly;                      // IDA "byte15" - SpawnGroup's +0x6E
    /* +0x16 */ bool           m_hasSpawnPoints;
    /* +0x17 */ unsigned char  m_x;                           // getUnsignedWorldPosition's pair
    /* +0x18 */ unsigned char  m_z;
    /* +0x1A */ unsigned short m_netId;                       // IDA "objectId" - the group's networkable id
};

class RemoveSpawnGroupEvent : public GameEvent               // 0x10
{
public:
    // GameEventMakerMaker<T>::createEvent() calls new T(), so registered events need this
    // default constructor. The ELF inlines it at the maker site and zeroes the members
    // deSerialize overwrites, as in StringBlockEvent.
    RemoveSpawnGroupEvent();

    RemoveSpawnGroupEvent(unsigned char spawnGroupId, unsigned short netId);   // 61 B
    virtual ~RemoveSpawnGroupEvent() {}                       // 27 / 40 B

    virtual int  getType() const;                             // 10 B - GERemoveSpawnGroupEvent (58)
    virtual void logInfo(bool, int);                          // 5 B - empty
    virtual void eventReceivedByRemote(void*);                // 5 B - empty
    virtual void executeClient();                             // 77 B - the client arm
    virtual bool serialize(io::BitStream* stream);            // +0x30, 173 B -
    virtual bool deSerialize(io::BitStream* stream);          // +0x2C, 135 B -

    /* +0x0C */ int            m_spawnGroupId;                // stored as a dword from the u8 arg
    /* +0x0E */ unsigned short m_netId;                       // overlaps the dword's top half in the
                                                              //   ELF ((this+7) as a word) - the two
                                                              //   fields share bytes; modelled apart
                                                              //   (internal; the recovered wire form
                                                              //   preserves the overlap)
};

} // namespace hfe
} // namespace dice

#endif // GAME_COMMON_SPAWNGROUPEVENTS_H
