// Rideable line with soldier attachment, collision and replicated traversal state.
// The length check and entry-distance check use different start points. [INFERRED] Source placement.

#ifndef GAME_OBJECTS_GAME_ZIPLINE_H     // [INFERRED] guard style
#define GAME_OBJECTS_GAME_ZIPLINE_H

#include <list>
#include <map>

#include "../Base/SimpleObject.h"

namespace dice {
namespace hfe {
class ServerConnection;
class ServerGameLogic;
namespace world {

class ISoldier;
class ZiplineTemplate;
class ZiplineRope;
class ZiplineContainer;

// The Special Forces placement, object, rope and rider hand-off spine is recovered. Registration
// requires the whole gadget boundary to be complete; the typed layout is
// shared by the object factory, SoldierNetworkable and ZiplineContainer.
class Zipline : public SimpleObject
{
public:
    explicit Zipline(const ZiplineTemplate* objectTemplate);
    virtual ~Zipline();

    virtual void  addRef();
    virtual int   getRef();
    virtual int   release();
    virtual void* queryInterface(unsigned int iid) const;

    virtual void init();
    virtual void destroy(bool force);                      // ; ignores force and destroys hard
    virtual void handleVisualUpdate(float deltaTime, float interpolation);
    virtual void handleUpdate(float deltaTime, unsigned int flags);
    virtual bool handleCollision(IObject*, const Vec3&, const Vec3&, const Vec3&,
                                 int, int, CollisionFaceInfo*, CollisionFaceInfo*);

    void  attachSoldier(ISoldier* soldier);
    bool  detachSoldier(ISoldier* soldier);
    float getCurrentState(ISoldier* soldier);
    void  setCurrentState(ISoldier* soldier, float state);

    // each, both `mov eax,[this+144h]; fld [eax+170h/174h]` - see the header block.
    float getEntryRadius() const;
    float getCollisionStartRadius() const;
    void  destroyZipline();                              // object-manager forward
    void  updateTimeOut(float deltaTime);
    float getZiplineHeight();
    float getAbsoluteTop();
    float getAbsoluteBottom();
    void  recalcBounds();
    float findStartPos(float lowerBound);

    // The return types are fixed by materialized callers: the first returns a Vec3 by value
    // (hidden sret in both x86 ports), while the second is tested as an exact 0/1 collision flag.
    Vec3 getNewSoldierPosition(const Vec3& start, const Vec3& fallback);
    bool wireBoxCheckForCollision(const Vec3& position, const float& radius,
                                  const float& up);

    // ZiplineTemplate::checkForEntry reads these two raw, exactly as the ELF does.
    friend class ZiplineTemplate;
    // ZiplineContainer::handleUpdate reads m_startPoint directly in both ports while refreshing
    // its cached soldier position (ELF 0x81D3430 / matching PE body).
    friend class ZiplineContainer;
    friend class ::dice::hfe::ServerGameLogic; // drop search: endpoint and deploying kit type
    // clientSendDatabase's zipline create arm passes m_endPoint in CreateObjectEvent's rotation
    // slot (ELF 0x808AADF reads +0x170; PE 0x54C7E0 reads +0x190 - the same member, both ports).
    friend class ::dice::hfe::ServerConnection;

protected:
    // layout +0x138..+0x18F, every value from the 429-byte constructor read in full. Total size 400
    // (0x190), attested by ZiplineTemplate::createObject's `operator new(0x190)`.
    // Names marked [UNRESOLVED] are ctor-attested slots with ctor-attested values and no
    // recovered meaning; they are placeholders that keep the two endpoint offsets correct, not
    // claims. Do not read behaviour into them.
    /* 0x138 */ int   m_refCount;                    // ctor 1
    // Source-level STL is intentional: retail ELF's list/iterator are 4+4 bytes, while the PE's
    // are 8+8.  That difference exactly accounts for the PE template pointer at +0x164 rather
    // than ELF +0x144; raw padding would make one port wrong.
    /* 0x13C */ std::list<ISoldier*> m_soldiers;
    /* 0x140 */ std::list<ISoldier*>::iterator m_soldierIterator;
    /* 0x144 */ const ZiplineTemplate* m_template;   // ctor = the constructor argument
    /* 0x148 */ ZiplineRope* m_rope;                 // owned span, ctor 0
    /* 0x14C */ int   m_networkableId;               // ctor 0; filled lazily in handleUpdate
    /* 0x150 */ bool  m_boundsDirty;                 // ctor true
    /* 0x151 */ char  _pad151[3];
    /* 0x154 */ float m_ziplineHeight;               // ctor 2.0f
    /* 0x158 */ int   _dword158;                     // [UNRESOLVED] NOT written by the ctor
    /* 0x15C */ float m_absoluteTop;                 // ctor 0
    /* 0x160 */ float m_absoluteBottom;              // ctor 0
    // THE line itself. Both zeroed by the ctor and filled by the recovered init() body.
    /* 0x164 */ Math::Vec3 m_startPoint;             // [CHOSEN name] the end checkForEntry measures
                                                     // the line's length from
    /* 0x170 */ Math::Vec3 m_endPoint;               // [CHOSEN name] the far end; checkForEntry's
                                                     // segment test runs getAbsolutePosition() -> here
    /* 0x17C */ bool  _byte17C;                      // [UNRESOLVED] ctor 0
    /* 0x17D */ bool  _byte17D;                      // [UNRESOLVED] ctor 0
    /* 0x17E */ char  _pad17E[2];
    /* 0x180 */ int   _dword180;                     // deploying kit type; NOT written by the ctor
    /* 0x184 */ float m_timeout;                     // ctor 0; compared with template +0x17C
    /* 0x188 */ int   m_riderCount;                  // ctor 0
    /* 0x18C */ bool  _byte18C;                      // [UNRESOLVED] ctor 0
    /* 0x18D */ char  _pad18D[3];
};

// ds:0x8AADDE0 - see the header block. Empty on a no-zipline map, so every SoldierNetworkable
// path that consults it is gated on a nonzero zipline id first.
extern std::map<int, Zipline*> ziplineMap;

} // namespace world
} // namespace hfe
} // namespace dice

#endif // GAME_OBJECTS_GAME_ZIPLINE_H
