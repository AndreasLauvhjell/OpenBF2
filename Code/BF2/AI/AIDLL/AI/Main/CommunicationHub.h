// Filters and distributes bot radio messages and enemy sightings.
// [FRONTIER] Remaining message behavior requires implementation.
// [RETIRE-WHEN: aidll-02-skeleton] Remove when the corresponding implementation is complete and validated.
// [INFERRED] Source placement.

#ifndef AI_AIDLL_AI_MAIN_COMMUNICATIONHUB_H
#define AI_AIDLL_AI_MAIN_COMMUNICATIONHUB_H

#include <utility>
#include <vector>

#include "AIDllTypes.h"

namespace dice {
namespace hfe {

// `filterAndSendAIRadioMessage`'s third argument. Engine-side enum; pointers and values only here.
enum ChannelMessageType : int;

namespace ai {

class BotMain;
class IAIObject;
class IAIPlayer;

// The three enums the radio path names. All [UNRESOLVED] as to enumerators - except that
// `CombatStrengthType` has at least seven, because `reset` fills with the literal 6.
enum AIRadioChannelType : int;
enum AIRadioMessageType : int;
enum CombatStrengthType : int;

class CommunicationHub
{
public:
    CommunicationHub();                     // 0x??,
    virtual ~CommunicationHub();            // 0x??, - (vtable slots 0 and 1)

    void reset();
    void enemySpottedByAI(IAIPlayer *, IAIObject *, int);
    void enemySpottedByHuman(IAIObject *, unsigned char) const;
    void filterAndSendAIRadioMessage(BotMain *, int, ChannelMessageType,
                                     AIRadioChannelType, AIRadioMessageType,
                                     unsigned char, int, unsigned char, unsigned char);
    void filterAndReceiveAIRadioMessage(const AIRadioMessageSettings&, int, int, bool);

    // Offsets describe the original ELF layout. STL sizes and double alignment depend on
    // the target compiler; do not treat 52 bytes as a portable object size.

    // These timestamps are uninitialized until reset is called.
    /*  +4 */ double m_time4;               // [UNRESOLVED] - reset puts -1000.0 here
    /* +12 */ double m_time12;              // [UNRESOLVED] - likewise
    // Two per-side combat-strength vectors. Each original ELF element occupies 12 bytes.
    /* +20 */ std::vector<std::pair<CombatStrengthType, double> > m_combatStrength[2];
    /* +44 */ double m_time44;              // [UNRESOLVED] - reset puts -1000.0 here too

    // first wins here (`if (!m_instance)`), and the destructor does test `== this` - the opposite
    // discipline from `AerialHeightMap`, one step earlier in the same init sequence.
    static CommunicationHub* m_instance;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_MAIN_COMMUNICATIONHUB_H
