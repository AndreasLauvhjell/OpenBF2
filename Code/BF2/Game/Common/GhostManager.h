// Prioritized, acknowledged replication of objects on one connection.
// [INFERRED] Header placement beside attested Game/Common/GhostManager.cpp.
// The stream and event-listener bases have separate ABI tables.

#ifndef GAME_COMMON_GHOSTMANAGER_H            // [INFERRED] guard style
#define GAME_COMMON_GHOSTMANAGER_H

#include "IStreamManager.h"
#include "../IEventListener.h"      // the second base
#include "../../Math/Vec3.h"        // setCompressionVector
#include "../../Core/Hash.h"        // the two bucket tables

#include <deque>
#include <list>
#include <vector>

namespace dice {
namespace hfe {

// Match the struct declarations in NetworkManager.h; MSVC mangles class and struct differently.
namespace io { struct NetworkableDescriptor; }
namespace io { struct NetworkableStateMask; }   // attested name (`NS1_20NetworkableStateMaskE`); tag is struct
namespace world { class IPlayer; }
// GhostObject IS modelled - Game/Common/GhostObject.h, 84 bytes attested
// from `operator new(0x54)` at 0x8095669. Still forward-declared here: this header only
// needs the pointer, and GhostManager.cpp includes the definition.
class GhostObject;                            // `NS0_11GhostObjectE`

// Diagnostic ghost priority paired with its object-template ID.
struct PrioInfo
{
    int   m_templateId;                       // +0x00
    float m_priority;                         // +0x04
};
struct GhostLessComp;                         // make_heap over vector<GhostObject*> - GhostObject.h
// the same way GhostLessComp was - out of the heap instantiations that
// inline it (__push_heap 0x8099950 / __adjust_heap 0x809B1E0): the float at +0x04 compared with
// the max-heap `<`, same direction, same `fucomip`/`jbe` NaN-sorts-low shape.
struct GhostPrioLessComp
{
    bool operator()(const PrioInfo& a, const PrioInfo& b) const
    {
        return a.m_priority < b.m_priority;
    }
};

// [UNRESOLVED] Enumerator names. Mode values are 0 normal, 1 demo recording,
// and 2 playback.
enum GhostManagerMode : int;        // opaque enums, fixed to MSVC's default width so GCC accepts them
enum GhostAction : int;

class GhostManager : public IStreamManager, public IEventListener
{
public:
    // THE constructor, destructor, AND reset() ARE
    GhostManager(GhostManagerMode mode, int capacity);
    virtual ~GhostManager();

    // IBase + IStreamManager. The accessors and all three wire bodies are
    virtual void  addRef();                                      // +0x00
    virtual int   getRef();                                      // +0x04
    virtual int   release();                                     // +0x08
    virtual void* queryInterface(unsigned int iid) const;        // +0x0C
    virtual int   getStreamManagerId() const;                    // +0x10
    virtual void  setStreamManagerId(int id);                    // +0x14
    virtual bool  processReceivedPacket(io::BitStream* stream);  // +0x18
    virtual void  handlePacketStatus(PacketStatus& s, bool& d);  // +0x1C
    // return type see IStreamManager.h. A caller does
    // `transmitResult >> 3` on it; this is the BIT count written.
    virtual unsigned int transmit(io::BitStream* stream, PacketStatus& status,
                                  unsigned int budget);
                                                                 // +0x20
    virtual void  setConnectionId(int connectionId);             // +0x24
    virtual int   getConnectionId();                             // +0x28
    virtual int   getError();                                    // +0x2C

    // IEventListener, appended at +0x38 with a thunk in the secondary table
    virtual void  handleEvent(EventCategory category, unsigned int id, EventNode* node, void* data);
                                                                 // Undo record for one sent ghost update. Saves the pre-send dirty block so packet
                                                                 // loss can restore it after sendData clears the live block.
    struct GhostSaveBufferEntry
    {
        int            m_action;        // +0x00  the GhostAction that was written
        unsigned short m_ghostId;       // +0x04  the wire id
        unsigned short m_networkableId; // +0x06  the engine id
        int            m_state[12];     // +0x08; snapshot of the ghost state block.
    };
    // sixteen bytes, NOT eight - from transmit's push_back, which writes
    // four consecutive dwords and advances the deque cursor by 16 (`dword10B0 + 4` on an int*).
    // The first member is the save buffer, which is what makes sendData's fifth parameter -
    // `SaveListEntry&` by its mangled name - receive the address of transmit's buffer local.
    struct SaveListEntry
    {
        std::vector<GhostSaveBufferEntry>* m_buffer;         // +0x00
        unsigned short m_controlObjNetId;                    // +0x04  0xFFFF when none
        int            m_actionBufferEntries;                // +0x08
        // THE base-line index. handlePacketStatus's delivered arm is three lines
        // and this is what they are for: when the packet arrives and this is not -1 and the saved
        // control-object id still matches the live one, it becomes GhostManager::m_baseLineIndex -
        // "the client is now known to hold this baseline".
        int            m_baseLineIndex;                      // +0x0C, -1 when none
    };

    // contents from transmit's resend drain, which pops one and copies its
    // two dwords into a stack GhostObject's m_ghostableState and m_networkableId - so a lost
    // update is re-sent as "this object, in this state", nothing more.
    struct ResendQueueEntry
    {
        int            m_ghostableState;                     // +0x00
        unsigned short m_networkableId;                      // +0x04
    };

    // NON-virtual.
    void setPlayer(world::IPlayer* player);

    void reset();
    void resetGhostStates(bool full);
    void resetGhost(io::NetworkableDescriptor* descriptor);
    void removeOldGhosts(bool force);
    // `void` IS proven, not assumed: the ELF ends `return addGhostObjectToHash(...)` and the
    // decompiler shows a GhostObject* return, but all eight call sites discard eax untested.
    void ghostObject(io::NetworkableDescriptor* descriptor, float priority, bool keepHigher);
    void removeGhostObject(io::NetworkableDescriptor* descriptor);
    void removeGhostObject(GhostObject* ghost);
    // the GhostObject* return is ambiguous in the ELF and its one caller discards it -
    // see the body. Left as declared rather than churned on a single call site.
    void addGhostable(unsigned short netId);
    void removeGhostable(unsigned short netId);
    void addGhostObjectToHash(unsigned short netId, GhostObject* ghost);
    GhostObject* findGhostObjectInHash(unsigned short netId);
    void removeGhostObjectFromHash(unsigned short netId);
    // Merges fresh state masks into live ghosts once per frame for this connection.
    void updateStateMasks(const std::vector<std::pair<const io::NetworkableDescriptor*,
                                                      io::NetworkableStateMask> >& updated);
    void update(unsigned int tickCount);
    void updateActiveGhostVec();
    // Updates the control object and returns true.
    bool updatePlayerControlObject(world::IPlayer* player, int frameCount);
    void setCompressionVector(const Vec3& reference);
    void debugShowActiveGhosts();
    // Returns a 16-bit ghost ID.
    unsigned short assignGhostId();
    // Returns borrowed mutable priority-vector storage.
    std::vector<PrioInfo>* getGhostPrioVec();
    void renderPriorities();                                     // empty
    // The save buffer must remain the first member of SaveListEntry.
    bool sendData(io::BitStream& s, GhostAction a, GhostObject* g, bool b, SaveListEntry& e);
    std::vector<GhostSaveBufferEntry>* allocGhostSaveBuffer(int entries);
    void freeGhostSaveBuffer(std::vector<GhostSaveBufferEntry>* buffer);

    // Returns whether the entry can be committed; failure requires rolling the stream back.
    bool writeData(io::BitStream& s, GhostAction a, GhostObject* g, bool b);
    // Returns false for malformed packet data.
    bool readData(io::BitStream& s);
    // Returns whether control state was written, controlling the header flag and saved control fields.
    bool writeControlObjectState(io::BitStream* s);
    // Reports decode success; callers also inspect the stream error latch.
    bool readControlObjectState(io::BitStream* s);

public:
    // The signed +4/+8 counters belong to IStreamManager. This places the secondary IEventListener
    // at +12 (ELF 0x809283A / PE 0x558B43) and preserves the derived scalar prefix below.
    /* +0x0010 */ int m_refCount;
    /* +0x0014 */ int m_connectionId;
    /* +0x0018 */ int m_error;
    /* +0x001C */ Hash<unsigned int, GhostObject*, UIntHashFunc> m_ghostHash;   // 4,096 buckets
    /* +0x0028 */ Hash<unsigned int, bool, UIntHashFunc>         m_ghostables;  // 3,011 buckets
    // Member offsets describe the original image, not native STL layouts. Access fields
    // by name. [INFERRED] Scalar names. m_ghostsThisPacket resets at transmit, not
    // construction, and occupies eight bits on the wire.
    /* +0x0034 */ int    m_ghostsThisPacket;
    /* +0x0038 */ GhostObject* m_ghostObjects[1024];    // the id-indexed table, memset to zero
    /* +0x1038 */ bool   m_updateActiveGhostVec;        // = true
    /* +0x103C */ std::vector<GhostObject*> m_activeGhostVec;
    /* +0x1048 */ std::vector<GhostObject*> m_ghostPrioHeap;   // +4 B tail: see the header note
    /* +0x1058 */ std::deque<unsigned short> m_ghostIdPool;    // seeded lazily by assignGhostId
    /* +0x1080 */ bool   m_ghostIdPoolSeeded;           // = false
    /* +0x1081 */ bool   m_debugGhostManager;           // from GSDebugGhostManager
    // both by GhostManager::update, which stores `ticks` and `ticks / 30.0`.
    // m_tickCount is the 32-bit field in transmit's 42-bit header - the server tick every ghost
    // packet carries. The double is the same value in seconds at 30 ticks/s; no reader recovered.
    /* +0x1084 */ double m_timeSeconds;                 // = 0.0
    /* +0x108C */ unsigned int m_tickCount;             // = 0  goes out in every ghost header
    /* +0x1090 */ int    m_streamManagerId;             // = -1, write-once
    /* +0x1094 */ bool   m_updateControlObject;         // = false
    /* +0x1098 */ std::deque<SaveListEntry> m_saveList;
    /* +0x10C0 */ std::deque<std::vector<GhostSaveBufferEntry>*> m_saveBufferPool; // the free list
    /* +0x10E8 */ std::list<std::vector<GhostSaveBufferEntry>*> m_allSaveBuffers;  // owns them
    /* +0x10EC */ std::deque<ResendQueueEntry> m_resendQueue;
    // NOT initialised BY THE constructor - see the header note. Parallel to m_ghostObjects.
    /* +0x1114 */ io::NetworkableStateMask* m_networkableStateMasks[1024];
    /* +0x2114 */ float  m_priorityFloor;               // = -10000.0f
    /* +0x2118 */ float  m_priorityCeil;                // = +10000.0f
    // NOT initialised BY THE constructor either - eight readers. See the header note.
    /* +0x211C */ unsigned short m_controlObjectNetworkableId;
    // NOT padding after ALL - updatePlayerControlObject zeroes it every frame and
    // fills it with the default vehicle's (the soldier's) net id when the control object's template
    // is CID_ParachuteTemplate, so the client can keep predicting the body under the canopy.
    // Name [INFERRED] from exactly that. Like its neighbour, never touched by the constructor.
    /* +0x211E */ unsigned short m_parachuteSoldierNetId;
    /* +0x2120 */ int    m_frameCount;                  // = -1
    /* +0x2124 */ int    m_actionBufferEntries;         // = -1
    /* +0x2128 */ int    m_lastSentActionBufferEntries; // = -1
    // Baseline ledger: a packet's candidate becomes the acknowledged baseline only on
    // delivery. Eight 1,026-byte slots form a ring; a countdown requests a fresh baseline
    // after the configured number of deltas or when no baseline has been acknowledged.
    /* +0x212C */ int    m_baseLineSlotThisPacket;      // = -1
    /* +0x2130 */ int    m_baseLineIndex;               // = -1
    /* +0x2134 */ unsigned char* m_baseLineData;        // operator new[](0x2010) - 8 x 1,026
    /* +0x2138 */ int    m_currentBaseLineSlot;         // = 0
    /* +0x213C */ int    m_deltasUntilBaseLine;         // = 0
    /* +0x2140 */ world::IPlayer* m_player;             // = 0
    // dead - constructed and destroyed, never filled anywhere in the binary. See the note.
    /* +0x2144 */ std::vector<unsigned short> m_ushortVec2144;
    /* +0x2150 */ std::vector<PrioInfo> m_ghostPrioVec; // +4 B tail: see the header note
    // the delta-compression cadence - see the header note. .data 0x8986344.
    static int m_numDeltasPerBaseLine;                  // = 5

    /* +0x2160 */ int    m_mode;                        // stored, and read by nothing else
    /* +0x2164 */ bool   m_byte2164;                    // = false [UNRESOLVED]
    /* +0x2168 */ Vec3   m_compressionVector;
    /* +0x2174 */ int    m_capacity;                    // read only by transmit (0x809811E)
    /* +0x2178 */ int    m_updateLengthBits;            // 11, or 13 for a demo
};

// Only the scalar prefix is shared across retail and modern STL; no total GhostManager size claim.
#if defined(_WIN64) || defined(__LP64__)
// [CHOSEN, not recovered] The 16-byte native stream prefix precedes an 8-byte listener vptr.
// Pointer tables are allocated/cleared with typed sizeof; ghost IDs and masks do not widen.
static_assert(offsetof(GhostManager, m_refCount) == 24, "native x64 GhostManager prefix after listener");
static_assert(offsetof(GhostManager, m_connectionId) == 28, "native x64 GhostManager connection ID");
static_assert(offsetof(GhostManager, m_error) == 32, "native x64 GhostManager error");
#else
static_assert(offsetof(GhostManager, m_refCount) == 16, "retail GhostManager prefix after listener");
static_assert(offsetof(GhostManager, m_connectionId) == 20, "retail GhostManager connection ID");
static_assert(offsetof(GhostManager, m_error) == 24, "retail GhostManager error");
#endif

} // namespace hfe
} // namespace dice

#endif // GAME_COMMON_GHOSTMANAGER_H
