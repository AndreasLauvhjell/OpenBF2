// Registers network objects and their descriptors under 16-bit IDs. Header path [INFERRED].
// Descriptors select static-priority or distance-scoped ghost delivery; dynamic IDs start at 642.
// Construction subscribes to event category 5 and initializes interpolation/extrapolation settings.
// This is an IEventListener, not an IBase: handleEvent precedes the concrete virtual destructor.
// [FRONTIER] markers identify incomplete paths; registration alone does not prove replication.

#ifndef IO_NETWORK_NETWORKMANAGER_H     // [INFERRED] guard style
#define IO_NETWORK_NETWORKMANAGER_H

#include <list>
#include <map>
#include <utility>
#include <vector>

#include "../../Core/Hash.h"
#include "../../Game/IEventListener.h"

namespace dice {
namespace hfe {

class GameLogic;
class GameClient;   // predictRemoteObjects sweeps m_activeDescriptors - see the friend below
namespace world { class IObject; class IPlayerControlObject; }

namespace io {

class INetworkable;
class INetworkableObject;
class BitStream;
// Baseline payload with a cursor measured in two-byte units. Eight records form the ghost baseline ring.
struct BaseLineData
{
    unsigned char  m_data[1024];                 // +0x000 - the networkables' snapshot area
    unsigned short m_cursor;                     // +0x400 - two-byte units into m_data, max 0x200
};
typedef char BaseLineDataLayoutCheck[sizeof(BaseLineData) == 1026 ? 1 : -1];
// Twelve-word replication mask. Internal subdivisions and operation names remain [UNRESOLVED].
// Operations 0 through 3 assign, or, and-not and and; other values leave the mask unchanged.
enum StateMaskOp : int;             // opaque, fixed to MSVC's default width so GCC accepts the declaration

struct NetworkableStateMask                     // 48 B; see Game/Common/GhostObject.h
{
    int m_opaque[12];                           // [UNRESOLVED] internal division

    // Applies the selected mask operation. The implementation is grouped with GhostManager.
    void doOperation(const NetworkableStateMask& other, StateMaskOp op);
};

// Replication precision tiers. Ghost updates use a lower tier than full updates.
// [CHOSEN] Integer global declarations preserve their runtime loads.
extern int StatePrecisionFull;                   // = 3
extern int StatePrecisionGhost;                  // = 2
extern int StatePrecisionLow;                    // = 1
extern int StatePrecisionLowest;                 // = 0

struct UniqueNetProperty                         // 12 B - a std::vector<INetworkable*>
{
    std::vector<INetworkable*> networkables;
};

struct NetworkableDescriptor                     // 0x20
{
    /* +0x00 */ int            controlObjId;    // the PCO's objectCacheId, -1 for a root, unset for
                                                //   an object-less networkable (a spawn group)
    /* +0x04 */ unsigned short descId;          // the net id
    /* +0x08 */ int            dword8;          // [UNRESOLVED] - not written on the add path
    /* +0x0C */ std::vector<UniqueNetProperty> properties;   // +0x0C / +0x10 / +0x14
    // Marks the object's own descriptor rather than descriptors belonging to child PCOs.
    /* +0x18 */ bool           addedToNetwork; // [CHOSEN] Name.
    /* +0x19 */ bool           byte19;          // [UNRESOLVED]
    /* +0x1A */ bool           byte1A;          // [UNRESOLVED]
    /* +0x1B */ bool           byte1B;          // [UNRESOLVED]
    /* +0x1C */ int            minimumBitSize;  // IDA "someCount": the sum of the networkables'
                                                //   getMinimumBitSize()
};

class NetworkManager : public IEventListener
{
public:
    // Cached template plan grouping each player-control object's networkables into shared properties.
    struct OBJECTINFO
    {
        unsigned int    classId;                 // +0 template->getClassID()
        world::IObject* object;                  // +4
    };
    struct DescDescriptor
    {
        int                             dword0;  // +0x00 - never read (see above)
        std::vector<std::vector<int> >  groups;  // +0x04 - objectinfo indices, one vector per property
    };
    struct ObjectDescriptor
    {
        std::vector<DescDescriptor>     pcoDescriptors;   // one per PCO (or the single root entry)
    };

    NetworkManager();
    virtual ~NetworkManager();

    // IEventListener - 0x846C940, 233 B: category 5 id 1 with node[0] == 1005 -> GSInterpolationTime,
    // 1006 -> GSExtrapolationTime (settings->U32Get, defaults 100 / 1200).
    virtual void handleEvent(EventCategory category, unsigned int id, EventNode* node, void* userData);

    // The registry
    void clear();                                // abort()s if objects remain;
                                                 //   frees every descriptor, resets the counters
    unsigned short assignId(unsigned short requested);   // 0 = "next dynamic"
    void releaseId();                            // empty, as shipped
    unsigned short addObject(world::IObject* object);                        // 37 B -> (obj, 0)
    unsigned short addObject(world::IObject* object, unsigned short netId);  // 39 B -> analyseObject
    unsigned short addNetworkable(INetworkable* networkable);
    unsigned short addNetworkable(INetworkable* networkable, unsigned short netId);
    void addDescriptor(NetworkableDescriptor* desc, unsigned short netId);
    void removeNetworkable(unsigned short netId);
    unsigned short regressNetworkableIds();
    int  getHighestUsedNetID();
    void removeDescriptors(world::IObject* object);   // every PCO's (or the
                                                 //   root's) descriptor through deleteDescriptorForId
    void deleteDescriptorForId(unsigned short netId);
    void addActiveDescriptor(NetworkableDescriptor* desc);
    void removeActiveDescriptor(unsigned short netId);
    void removeActiveDescriptor(NetworkableDescriptor* desc);
    world::IObject* getObject(unsigned short netId);
    std::map<unsigned short, world::IObject*>& getObjectMap();
    std::map<unsigned short, NetworkableDescriptor*>& getDescriptorMap();
    std::list<std::list<std::pair<INetworkableObject*, NetworkableDescriptor*> > >&
         getScopableRootDescriptors();
    std::map<unsigned short, NetworkableDescriptor*>& getStaticDescriptors();
    NetworkableDescriptor* getDescriptor(unsigned short netId);
    void setGlobalPredictionMode(int mode);
    int  getGlobalPredictionMode() const;
    // [CHOSEN] Returns the interpolation interval in milliseconds.
    int  getInterpolationTime() const { return m_interpolationTime; }   // +0x50
    // [CHOSEN] Returns the extrapolation limit in milliseconds.
    int  getExtrapolationTime() const { return m_extrapolationTime; }   // +0x54
    // [CHOSEN] Stores interpolation and extrapolation intervals in milliseconds.
    void setInterpolationTime(int ms) { m_interpolationTime = ms; }       // +0x50
    void setExtrapolationTime(int ms) { m_extrapolationTime = ms; }       // +0x54
    // [CHOSEN] Returns the timing-debug flag controlling exact receive-time stamps.
    bool getByte5C() const { return m_byte5C; }                          // +0x5C
    void callAddedToNetworkManager(NetworkableDescriptor* desc);
    void callRemovedFromNetworkManager(NetworkableDescriptor* desc);

    // Groups an object hierarchy's networkables into properties and assigns sequential ids from netId.
    // Returns the object's networkable id.
    unsigned short analyseObject(world::IObject* object, unsigned short netId);
    void _analyseObject(world::IObject* object, std::vector<OBJECTINFO>& out);
    bool compareProperties(OBJECTINFO& a, OBJECTINFO& b);
    void getAllPlayerControlObjects(world::IObject* object,
                                    std::vector<world::IPlayerControlObject*>& out);

    // Returns a borrowed pointer to the collected descriptor/state-mask vector for connection updates.
    std::vector<std::pair<const NetworkableDescriptor*, NetworkableStateMask> >* updateStateMasks();
    // Writes replication data using the selected precision and ghost-mask policy.
    // Delta updates also use a baseline and may rewind its cursor. Implementations live with GhostManager.
    bool getNetUpdate(io::BitStream& stream, NetworkableDescriptor* descriptor,
                      NetworkableStateMask* mask, int precision, bool ghostTier);
    bool getNetUpdateDelta(io::BitStream& stream, NetworkableDescriptor* descriptor,
                           BaseLineData& baseLine, NetworkableStateMask* mask, int precision,
                           bool ghostTier, bool resetBaseLineCursor);
    // same TU (Game/Common/GhostManager.cpp). Snapshot every property's
    // representative networkable into the BaseLineData via INetworkable::getBaseLine (+0x38);
    // the bool rewinds the cursor first. False out on the first refusal.
    bool getBaseLine(BaseLineData& baseLine, NetworkableDescriptor* descriptor,
                     bool resetBaseLineCursor);
    // Applies each property payload to every networkable sharing it, rewinding the stream for each reader.
    // All entries must be nonnull; the delta form also passes the acknowledged baseline.
    bool setNetUpdateDelta(io::BitStream& stream, NetworkableDescriptor* descriptor,
                           BaseLineData& baseLine, float timeMs, bool ghostActive,
                           int precision, bool resetBaseLineCursor);

    // Applies property updates from stream with no baseline. timeMs is the packet timestamp in milliseconds.
    bool setNetUpdate(io::BitStream& stream, NetworkableDescriptor* descriptor,
                      float timeMs, bool ghostActive, int precision);

    // the wire quintet is complete: getNetUpdate / setNetUpdate / getBaseLine /
    // getNetUpdateDelta / setNetUpdateDelta, all defined in Game/Common/GhostManager.cpp.

private:
    friend class ::dice::hfe::GameLogic;          // unload clears only the per-template plan cache
    // GameClient traverses active descriptors during prediction.
    friend class ::dice::hfe::GameClient;
    /* +0x04 */ std::map<unsigned short, world::IObject*>          m_networkableObjects;
    /* +0x10 */ unsigned short m_idCounter;      // 642 - dynamic ids start above the fixed ones
    /* +0x12 */ unsigned short m_word12;         // 1 - [UNRESOLVED]
    /* +0x14 */ std::map<unsigned short, NetworkableDescriptor*>   m_descriptors;
    /* +0x20 */ Hash<unsigned int, NetworkableDescriptor*, UIntHashFunc> m_descriptorHash;   // 4096
    /* +0x2C */ std::map<unsigned short, NetworkableDescriptor*>   m_staticDescriptors;
    /* +0x38 */ std::map<unsigned short, NetworkableDescriptor*>   m_scopableDescriptors;
    /* +0x44 */ std::map<unsigned short, NetworkableDescriptor*>   m_activeDescriptors;
    /* +0x50 */ int  m_interpolationTime;        // 100
    /* +0x54 */ int  m_extrapolationTime;        // 1200
    /* +0x58 */ int  m_globalPredictionMode;     // 1
    /* +0x5C */ bool m_byte5C;                   // [UNRESOLVED] 0
    /* +0x60 */ std::map<int, ObjectDescriptor> m_networkedTemplates;   // template id -> the
                                                 //   analysed property plan (see the types above)
    /* +0x6C */ int  m_dword6C; // Update counter, initially zero.
    // Counts updates whose minimum size exceeds the stream budget; delta updates are excluded.
    /* +0x70 */ int  m_ghostNetworkableUpdatesMissing;   // 0
    // Owns one state mask per descriptor entry.
    /* +0x74 */ std::vector<std::pair<const NetworkableDescriptor*, NetworkableStateMask> >
                     m_networkableStateMasks;
    /* +0x80 */ std::list<std::list<std::pair<INetworkableObject*, NetworkableDescriptor*> > >
                     m_scopableRootDescriptors;
};

extern NetworkManager* networkManager;           // ds:0x8AC3D1C - Core/Globals.cpp

} // namespace io
} // namespace hfe
} // namespace dice

#endif // IO_NETWORK_NETWORKMANAGER_H
