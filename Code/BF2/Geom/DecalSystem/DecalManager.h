// Path: [INFERRED] header beside DecalManager.cpp, attested at RendDX9 0x1021CA40.
// Decal lifetime, template registry and rendering owner, recovered from RendDX9.dll 1.50.
// Retail layout: 0x2CC bytes in both plugin flavours; constructor 0x101251F0.
// VideoOptions writes enabled at +0x2A6 directly; preserve that offset.

#ifndef BF2_GEOM_DECALSYSTEM_DECALMANAGER_H
#define BF2_GEOM_DECALSYSTEM_DECALMANAGER_H

#ifdef BF2_IMAGE_RENDDX9

#include "IDecalManager.h"
#include "DecalContainers.h"
#include "DecalObject.h"
#include "DecalRenderer.h"
#include "../../RendDX9/RendDX9Base.h"                // dx9rend::RenderPathListener
#include "../../../RendDX9Abi/BfMemory.h"
#include "../../../RendDX9Abi/Vc71String.h"

#include <stddef.h>

namespace dice {
namespace hfe {

namespace dx9rend {
class IRenderView;
struct TextureAtlas;
struct TextureAtlasEntry;
}

namespace geom {

class DecalTemplate;

// The singleton global ModuleGeom's getSingletonList points at (dword_1024B908). ModuleGeom.cpp
// defines it.
extern IBase* g_DecalManager;

// The two ids (CID 821018, IID 820018) stay defined in DecalManager.cpp and declared where used
// (ModuleGeom.cpp) - not here: Core/Ids.h defines the same names as internal-linkage constants in
// this namespace, so an `extern` declaration in a header would collide in any TU including both.

// The two maps the image only constructs / clears / destroys: +0x78 has a 0x38-byte node (a 28-byte
// std::string key destroyed per node in sub_10126BD0, a 12-byte trivially destructible value);
// +0x84 a 0x18-byte node (4-byte key and value). [UNRESOLVED] types; see DecalContainers.h.
struct DecalUnreadValue12
{
    unsigned int words[3];
};

typedef decalvc71::Map<vc71::string, DecalTemplate*, decalvc71::NoCaseLess>  DecalTemplateMap;
typedef decalvc71::Map<unsigned short, DecalTemplate*,
                       decalvc71::UnsignedLess<unsigned short> >               DecalTemplateIndexMap;
typedef decalvc71::Map<vc71::string, DecalUnreadValue12, decalvc71::UnreadStringLess>
                                                                               DecalUnreadStringMap;
typedef decalvc71::Map<unsigned int, unsigned int, decalvc71::UnsignedLess<unsigned int> >
                                                                               DecalUnreadMap;

// Owns decal clients, templates, the pooled records and the embedded renderer.
// update() drains deferred removal/deletion queues; release() on a client is not immediate.
// Retail leaves renderPath unwritten; keep its listener subobject at +0x04.
class DecalManager : public IDecalManager, public dx9rend::RenderPathListener
{
public:
    DecalManager();
    ~DecalManager();

    // IBase, +0x00..+0x0C
    virtual void  addRef() override;                                         // (ICF)
    virtual int   getRef() override;                                         // (ICF)
    virtual int   release() override;
    virtual void* queryInterface(unsigned int iid) const override;

    // IDecalManager, +0x10..+0x54
    virtual void  reserved10() override;                                     // +0x10 nullsub_59
    virtual void* createClient(void* owner) override;                        // +0x14
    virtual void  enqueue(void* decalObject) override;                       // +0x18
    virtual IDecalTemplate* addTemplate(const DecalTemplateString& name) override; // +0x1C
    virtual void  reserved20(void* argument) override;                       // +0x20 nullsub_60
    virtual void  addInstance(void* instance) override;                      // +0x24
    virtual void  removeInstance(void* decalObject) override;                // +0x28
    virtual void  resetPool() override;                                      // +0x2C
    virtual void  update(float deltaTime) override;                          // +0x30
    virtual void  setTimeToLive(float distance) override;                  // +0x34 0x10124190 timeToLive
    virtual float getTimeToLive() const override;                          // +0x38 0x101241D0 timeToLive
    virtual void  setFadeDistance(float distance) override;                           // +0x3C 0x10124230 fadeDistance
    virtual float getFadeDistance() const override;                                   // +0x40 0x10124270 fadeDistance
    virtual void  setStartFadePercentageOfFadeDistance(float percentage) override; // +0x44
    virtual float getStartFadePercentageOfFadeDistance() const override;                              // +0x48 0x101242B0 startFade%
    virtual void  setFadeOutAtPercentageOfTimeToLive(float ratio) override;                            // +0x4C 0x101241E0 fadeOut%
    virtual float getFadeOutAtPercentageOfTimeToLive() const override;                                 // +0x50 0x10124220 fadeOut%
    virtual void  reserved54() override;                                     // +0x54 nullsub_62

    // dx9rend::RenderPathListener, table 0x1021CAB4
    virtual void onRenderPathActivated(int path) override;                   // +0x00 nullsub_20
    virtual void onRenderPathDeactivated() override;                         // +0x04 nullsub_63
    virtual void onRenderPathChanged(int path) override;                     // +0x08 nullsub_64
    // Draws the current frame's decals; unused is ignored.
    virtual void onRenderPathNotification(int unused) override;
    virtual int  getCurrentRenderPath() const override;                      // +0x10
    virtual vc71::string getRenderPathListenerName() const override;         // +0x14

    // non-virtual, in the image's order
    void renderDecals(dx9rend::IRenderView* view);                           // (unref)
    void unusedEmpty() {}                                                    // (unref)
    void setDrawStats(bool enable);                                          // empty
    bool getDrawStats() const { return m_drawStats; }
    void recomputeFades();                                                   // (unref;
                                                                             //   inlined in 5 slots)
    dx9rend::TextureAtlasEntry* getAtlasEntry(const vc71::string& textureName);
    bool loadAtlas(const vc71::string& fileName);
    void initPool();
    void collectDecals(dx9rend::IRenderView& view);                          // (unref)
    void flushDeletes();
    void flushRemovals();
    void clear();
    DecalTemplate* getTemplateByIndex(unsigned short index);                 // (unref)
    bool generateTexture(const vc71::string& atlasName);                     // (console)

    // the inline surface DecalObject.cpp (D1) and DecalRenderer.cpp use
    // The pool pop DecalObject::add inlines (0x101AE555..0x101AE56B): the head, and the head's link
    // as the new head - no null test before the caller writes the record (0x101AE571).
    DecalRecord* popFreeRecord()
    {
        DecalRecord* const record = m_freeList;
        m_freeList = record ? record->next : 0;
        return record;
    }
    // emitted out of line in DecalObject.cpp's range, so an inline of
    // this header: free the record's vertex block (sub_1005F360) and null it, then push the record.
    void freeRecord(DecalRecord* record)
    {
        if (record->vertices)
        {
            ::renddx9abi::engineFree(record->vertices);
            record->vertices = 0;
        }
        record->next = m_freeList;
        m_freeList = record;
    }
    bool  isEnabled() const          { return m_enabled; }                   // +0x2A6
    // VideoOptions::apply's store (sub_1000FCB0, 0x1001032A: `setnle cl; mov [eax+2A6h], cl` -
    // (videoOptions +0x3C) > 1). VideoOptions.cpp writes byte 678 raw today; this is the typed form
    // the integrator can switch it to - the offset is asserted either way.
    void  setEnabledField(bool enabled) { m_enabled = enabled; }
    float decalNormalOffset() const  { return m_decalNormalOffset; }         // +0x2C0
    float decalGeomClipAngle() const { return m_decalGeomClipAngle; }        // +0x2C4
    float fadeOutTime() const        { return m_fadeOutTime; }               // +0x14
    float fadeOutDuration() const    { return m_fadeOutDuration; }           // +0x18
    float fadeDistance() const       { return m_fadeDistance; }              // +0x2B0
    float startFadeDistance() const  { return m_startFadeDistance; }         // +0x2B4
    float minimumAlpha() const       { return m_minimumAlpha; }              // +0x2B8
    // The console's two direct fields (0x100BC266 / 0x100BC36A store and load them raw).
    void  setDecalNormalOffsetField(float value)  { m_decalNormalOffset = value; }
    void  setDecalGeomClipAngleField(float value) { m_decalGeomClipAngle = value; }

private:
    DecalManager(const DecalManager&);
    DecalManager& operator=(const DecalManager&);

    int                              m_renderPath;                          // +0x008
    int                              m_refCount;                            // +0x00C
    float                            m_timeToLive;                          // +0x010
    float                            m_fadeOutTime;                         // +0x014
    float                            m_fadeOutDuration;                     // +0x018
    float                            m_fadeOutAtPercentageOfTimeToLive;     // +0x01C
    float                            m_lastDeltaTime;                       // +0x020
    unsigned char                    m_byte24;                              // +0x024
    unsigned char                    m_pad25[3];
    DecalObjectMap                   m_clients;                             // +0x028
    DecalObjectMap                   m_activeInstances;                     // +0x034
    DecalTemplateMap                 m_templates;                           // +0x040
    decalvc71::Vector<DecalObject*>  m_deleteQueue;                         // +0x04C
    decalvc71::Vector<DecalObject*>  m_removeQueue;                         // +0x05C
    DecalTemplateIndexMap            m_templatesByIndex;                    // +0x06C
    DecalUnreadStringMap             m_map78;                               // +0x078
    DecalUnreadMap                   m_map84;                               // +0x084
    DecalRecord*                     m_pool;                                // +0x090
    DecalRenderer                    m_renderer;                            // +0x094
    DecalRecord*                     m_freeList;                            // +0x290
    unsigned int                     m_poolSize;                            // +0x294
    unsigned short                   m_templateCount;                       // +0x298
    unsigned char                    m_pad29A[2];
    unsigned int                     m_dword29C;                            // +0x29C
    unsigned int                     m_dword2A0;                            // +0x2A0
    bool                             m_clearing;                            // +0x2A4
    bool                             m_drawStats;                           // +0x2A5
    bool                             m_enabled;                             // +0x2A6
    unsigned char                    m_pad2A7[9];                           // +0x2A7..+0x2AF
    float                            m_fadeDistance;                        // +0x2B0
    float                            m_startFadeDistance;                   // +0x2B4
    float                            m_minimumAlpha;                        // +0x2B8
    float                            m_startFadePercentageOfFadeDistance;   // +0x2BC
    float                            m_decalNormalOffset;                   // +0x2C0
    float                            m_decalGeomClipAngle;                  // +0x2C4
    dx9rend::TextureAtlas*           m_atlas;                               // +0x2C8

    friend struct DecalManagerLayoutCheck;
};

// Asserted in both flavours (x86): the object is the renderer's own, and VideoOptions::apply's raw
// write at +0x2A6 has to land on m_enabled whichever flavour is built.
struct DecalManagerLayoutCheck
{
#if defined(_M_IX86)
    static_assert(sizeof(DecalManager) == 0x2CC, "RefFactory_geom_DecalManager allocates 0x2CC");
    static_assert(offsetof(DecalManager, m_renderPath) == 0x08, "listener +0x10 reads subobject+4");
    static_assert(offsetof(DecalManager, m_refCount) == 0x0C, "ICF_incrementDwordAt0xC");
    static_assert(offsetof(DecalManager, m_timeToLive) == 0x10, "+0x10");
    static_assert(offsetof(DecalManager, m_lastDeltaTime) == 0x20, "+0x20");
    static_assert(offsetof(DecalManager, m_clients) == 0x28, "+0x28");
    static_assert(offsetof(DecalManager, m_activeInstances) == 0x34, "+0x34");
    static_assert(offsetof(DecalManager, m_templates) == 0x40, "+0x40");
    static_assert(offsetof(DecalManager, m_deleteQueue) == 0x4C, "+0x4C");
    static_assert(offsetof(DecalManager, m_removeQueue) == 0x5C, "+0x5C");
    static_assert(offsetof(DecalManager, m_templatesByIndex) == 0x6C, "+0x6C");
    static_assert(offsetof(DecalManager, m_map78) == 0x78, "+0x78");
    static_assert(offsetof(DecalManager, m_map84) == 0x84, "+0x84");
    static_assert(offsetof(DecalManager, m_pool) == 0x90, "+0x90");
    static_assert(offsetof(DecalManager, m_renderer) == 0x94, "+0x94");
    static_assert(offsetof(DecalManager, m_freeList) == 0x290, "+0x290");
    static_assert(offsetof(DecalManager, m_poolSize) == 0x294, "+0x294");
    static_assert(offsetof(DecalManager, m_templateCount) == 0x298, "+0x298");
    static_assert(offsetof(DecalManager, m_dword29C) == 0x29C, "+0x29C");
    static_assert(offsetof(DecalManager, m_clearing) == 0x2A4, "+0x2A4");
    static_assert(offsetof(DecalManager, m_drawStats) == 0x2A5, "+0x2A5");
    static_assert(offsetof(DecalManager, m_enabled) == 0x2A6, "VideoOptions::apply writes +0x2A6");
    static_assert(offsetof(DecalManager, m_fadeDistance) == 0x2B0, "+0x2B0");
    static_assert(offsetof(DecalManager, m_minimumAlpha) == 0x2B8, "+0x2B8");
    static_assert(offsetof(DecalManager, m_decalNormalOffset) == 0x2C0, "+0x2C0");
    static_assert(offsetof(DecalManager, m_decalGeomClipAngle) == 0x2C4, "+0x2C4");
    static_assert(offsetof(DecalManager, m_atlas) == 0x2C8, "+0x2C8");
#endif
};

// RefFactory_geom_DecalManager, registered by ModuleGeom's getClassList.
IBase* __fastcall createDecalManager(unsigned int iid, IBase* outer);

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_RENDDX9

#endif // BF2_GEOM_DECALSYSTEM_DECALMANAGER_H
