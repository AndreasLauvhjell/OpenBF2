// Per-bot sight, hearing and communication state. Owns a view frustum and sensed-object records.

#ifndef AI_AIDLL_AI_AIBOT_BOTSENSES_H       // [INFERRED] guard style
#define AI_AIDLL_AI_AIBOT_BOTSENSES_H

#include <list>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include "../../../../Math/Frustum.h"

namespace dice {
namespace hfe {
namespace ai {

class BotMain;
// The two things a bot's perception owns and deletes. Both are [FRONTIER] classes - `reset` and
// the destructor delete them through an inlined, non-virtual destructor, so neither can be written
// until the class itself is recovered. See the reset block in the .cpp.
class FireObject;
class SensedObject;

class BotSenses
{
public:
    // (`operator new(0x94)` in SensingAgent's constructor).
    explicit BotSenses(BotMain* owner);
    ~BotSenses();
    void reset();                            // - (two arms frontier)

    // attested: an 11-byte body that returns `this + 112`.
    const std::map<unsigned int, SensedObject*>& getSensedObjects() const { return m_sensedObjects; }

private:
    typedef std::pair<double, SensedObject*>      TimedObject;   // a time and a thing
    typedef std::list<TimedObject>                TimedList;
    typedef std::pair<FireObject*, unsigned int>  FireEntry;
    typedef std::map<unsigned int, SensedObject*> SensedObjectMap;

    /*   0 */ BotMain* m_bot;                // the ctor argument, and the first store
    /*   4 */ Frustum* m_frustum;            // new(0x1E8) - . See the header block.
    /*   8 */ char m_gap8[4];                // [UNRESOLVED] - never written, by anything read so far
    /*  12 */ double m_double12;             // = -1000000.0 a fifth kind of sentinel
    // the distance sort buffer - what `InfoDistPred` and `partial_sort` operate on. Held as a
    // member rather than a local, so the seeing path does not reallocate every tick.
    /*  20 */ std::vector<std::pair<float, unsigned int> > m_sortBuffer;
    /*  32 */ int m_int32;                   // ctor: 0, reset: 0
    /*  36 */ int m_int36;                   // ctor: 0, reset: 0
    /*  40 */ int m_int40;                   // ctor: 0, reset: 0
    // handle -> position in the list at +56. See the header block.
    /*  44 */ std::map<unsigned int, TimedList::iterator> m_map44;
    /*  56 */ TimedList m_list56;
    // A cursor into the list above, seeded to end by the constructor's last statement.
    /*  60 */ TimedList::iterator m_cursor60;
    /*  64 */ std::set<unsigned int> m_set64;
    /*  76 */ TimedList m_list76;
    // A cursor into the list above - and unlike +60 the constructor leaves it default-constructed.
    // Only `reset` makes it valid. See the header block.
    /*  80 */ TimedList::iterator m_cursor80;
    /*  84 */ std::list<FireEntry> m_list84;
    /*  88 */ int m_int88;                   // ctor: 0, reset: 0
    /*  92 */ int m_int92;                   // ctor: 0, reset: 0
    /*  96 */ char m_gap96[4];               // [UNRESOLVED] - never written
    /* 100 */ std::map<unsigned int, FireObject*> m_map100;
    /* 112 */ SensedObjectMap m_sensedObjects;
    // the only two containers `reset` does not empty. Element width 4 and trivially destructible
    // (the destructor deallocates without a per-element loop); the element type is [INFERRED].
    /* 124 */ std::vector<unsigned int> m_vector124;
    /* 136 */ std::vector<unsigned int> m_vector136;
};

} // namespace ai
} // namespace hfe
} // namespace dice

#endif // AI_AIDLL_AI_AIBOT_BOTSENSES_H
