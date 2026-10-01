// Replicates object or kit creation from the server. Object events carry template/network IDs,
// transform and default team; kit events carry creation index, unlock level and position.
// [UNRESOLVED] The general meaning of CreateObjectEvent::m_mode is not established.

#ifndef GAME_COMMON_CREATEOBJECTEVENT_H     // [INFERRED] guard style
#define GAME_COMMON_CREATEOBJECTEVENT_H

#include "GameEvent.h"
#include "../../Math/Vec3.h"

namespace dice {
namespace hfe {

class CreateObjectEvent : public GameEvent                        // 0x34
{
public:
    // GameEventMakerMaker<T>::createEvent() calls new T(), so registered events need this
    // default constructor. The ELF inlines it at the maker site and zeroes the members
    // deSerialize overwrites, as in StringBlockEvent.
    CreateObjectEvent();

    CreateObjectEvent(unsigned int templateId, unsigned short networkableId, int defaultTeam,
                      const Vec3& position, const Vec3& rotation);
    CreateObjectEvent(unsigned int templateId, unsigned short networkableId, unsigned int mode,
                      int defaultTeam);                              // zero vectors
    virtual ~CreateObjectEvent() {}                                  // vptr + GameEvent's

    virtual int  getType() const;                                    // GECreateObject (6)
    virtual void logInfo(bool, int);                                 // 5 B - shipped empty,
    virtual void eventReceivedByRemote(void*);                       // 50 B -
    virtual void executeClient();                                    // control-object tail fails loud
    virtual bool deSerialize(io::BitStream* stream);                 // 593 B -
    virtual bool serialize(io::BitStream* stream);                   // 872 B -

    unsigned int   m_templateId;                                     // +0x0C
    unsigned short m_networkableId;                                  // +0x10
    Vec3           m_position;                                       // +0x14
    Vec3           m_rotation;                                       // +0x20
    int            m_defaultTeam;                                    // +0x2C
    unsigned int   m_mode;                                           // +0x30 [UNRESOLVED name]
};

class CreateKitEvent : public GameEvent                           // 0x28
{
public:
    // GameEventMakerMaker<T>::createEvent() calls new T(), so registered events need this
    // default constructor. The ELF inlines it at the maker site and zeroes the members
    // deSerialize overwrites, as in StringBlockEvent.
    CreateKitEvent();

    CreateKitEvent(unsigned int kitTemplateId, unsigned short networkableId, unsigned int creationIndex,
                   int chosenUnlockLevel, const Vec3& position);
    virtual ~CreateKitEvent() {}

    virtual int  getType() const;                                    // GECreateKit (31)
    virtual void logInfo(bool, int);                                 // 5 B - shipped empty,
    virtual void eventReceivedByRemote(void*);                       // 50 B -
    virtual void executeClient();                                    // 422 B -  client-only
    virtual bool deSerialize(io::BitStream* stream);                 // 311 B -
    virtual bool serialize(io::BitStream* stream);                   // 305 B -

    unsigned int   m_kitTemplateId;                                  // +0x0C
    unsigned short m_networkableId;                                  // +0x10
    unsigned int   m_creationIndex;                                  // +0x14
    int            m_chosenUnlockLevel;                              // +0x18
    Vec3           m_position;                                       // +0x1C
};

} // namespace hfe
} // namespace dice

#endif // GAME_COMMON_CREATEOBJECTEVENT_H
