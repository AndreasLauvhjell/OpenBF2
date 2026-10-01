// Path:     attested - "C:\dice\Projects\BF2Branches\Patch_1_50\Code\BF2\Geom\ObjectDrawer.cpp",
//           read as a literal at RendDX9 0x1021AF98.
// Contents: recovered from RendDX9.dll (PE, symbols stripped).  The constructor, the IBase quartet
//           and the five leaf slots are written; the seven larger slots are named frontiers.

// See ObjectDrawer.h for the layout evidence and for why this class is NOT registered yet.

#ifdef BF2_IMAGE_RENDDX9

#include "ObjectDrawer.h"
#include "MeshTemplate.h"                     // meshBoundsFinalize (sub_101C5EB0) - slot +0x3C

#include "../../RendDX9Abi/BfMemory.h"
#include "../../RendDX9Abi/Frontier.h"
#include "../Core/EventManager.h"
#include "../Core/Ids.h"
#include "../Game/Objects/IObject.h"
#include "../Game/Objects/ObjectLayoutAccess.h"
#include "../Math/LocalBoundingBox.h"
#include "../Math/Mat4.h"
#include "../Math/Plane3.h"
#include "../Math/Vec3.h"
#include "../RendMisc/IRenderView.h"
#include "../Scene/Object/ObjectPredicator.h"

#include <algorithm>
#include <math.h>

#include <cstdio>
#include <new>

namespace dice {
namespace hfe {

// PluginEntry.cpp - the host ObjectManager singleton (dword_10244AD8).  Declared here rather
// than pulled through a header, exactly as BF2Render.cpp declares the same four singletons.
extern IBase* objectManager;

namespace geom {

namespace {


// Explicit primary and event-handler tables preserve the 16-slot and one-slot interfaces
// while the remaining method signatures are unknown.


#define OD_FRONTIER(fn, slotOffset, addr, what)                                       \
    void __fastcall fn(void* self, void*)                                             \
    {                                                                                 \
        RD9_FRONTIER_AT("dice::hfe::geom::ObjectDrawer::slot_" #slotOffset             \
                        " (" #addr " - " what ")", self, slotOffset);                  \
    }

OD_FRONTIER(od_slot1C, 0x1C, 0x100F8DE0, "324 B, unread")
// +0x3C, the box occlusion query; the body is ObjectDrawer::isBoundsOccluded below.
// `retn 4`, the answer in AL, like +0x38.
bool __fastcall od_slot3C(void* self, void*, void* bounds);
#undef OD_FRONTIER

// +0x38, the occlusion query; the body is ObjectDrawer::isGeometryOccluded below.
// `retn 4`, the answer in AL: __fastcall with the dummy edx gives the same shape.
bool __fastcall od_slot38(void* self, void*, void* geometry);


// Handles the drawer's event subobject at +0x04 and updates the outer frame gate.
// [UNRESOLVED] Event data at +0x2C must point to a word compared with 4; neither pointer is guarded.

char __fastcall od_event00(void* self, void*, int category, int id, void* data, void* arg4)
{
    (void)category;
    (void)arg4;

    if (id != 1)
        return static_cast<char>(id - 1);

    // &m_eventVptr sits at +0x04, so the object starts four bytes back.
    ObjectDrawer* const drawer =
        reinterpret_cast<ObjectDrawer*>(static_cast<unsigned char*>(self) - 4);

    const unsigned int* const probe =
        *reinterpret_cast<unsigned int* const*>(static_cast<unsigned char*>(data) + 0x2C);

    const char result = (*probe == 4u) ? 1 : 0;
    drawer->m_byte0C = static_cast<unsigned char>(result);
    return result;
}

// The two that take an argument, so they cannot use the macro above.
void __fastcall od_draw(void* self, void*, float time)
{
    static_cast<ObjectDrawer*>(self)->draw(time);
}

void __fastcall od_updateVisible(void* self, void*, float time)
{
    static_cast<ObjectDrawer*>(self)->updateVisible(time);
}

void __fastcall od_slot18(void* self, void*)
{
    static_cast<ObjectDrawer*>(self)->slot_0x18();
}

void __fastcall od_slot20(void* self, void*, void* object)
{
    (void)object;
    RD9_FRONTIER_AT("dice::hfe::geom::ObjectDrawer::slot_0x20 (0x100F8F30, 51 B - takes an object, "
                    "clears a flag through its +0xA4, then two helpers)", self, 0x20);
}

// the written leaf slots, as the thin __fastcall adapters the tables need

void __fastcall od_addRef(void* self, void*)
{
    static_cast<ObjectDrawer*>(self)->addRef();
}

int __fastcall od_getRef(void* self, void*)
{
    return static_cast<ObjectDrawer*>(self)->getRef();
}

int __fastcall od_release(void* self, void*)
{
    return static_cast<ObjectDrawer*>(self)->release();
}

void* __fastcall od_queryInterface(void* self, void*, unsigned int iid)
{
    return static_cast<const ObjectDrawer*>(self)->queryInterface(iid);
}

void* __fastcall od_getObjectVector(void* self, void*, int which)
{
    return static_cast<ObjectDrawer*>(self)->getObjectVector(which);
}

void __fastcall od_setRenderView(void* self, void*, void* view)
{
    static_cast<ObjectDrawer*>(self)->setRenderView(view);
}

void* __fastcall od_getRenderView(void* self, void*)
{
    return static_cast<const ObjectDrawer*>(self)->getRenderView();
}

void __fastcall od_setOccluder(void* self, void*, IBase* occluder)
{
    static_cast<ObjectDrawer*>(self)->setOccluder(occluder);
}

IBase* __fastcall od_getOccluder(void* self, void*)
{
    return static_cast<const ObjectDrawer*>(self)->getOccluder();
}


// Flag-based object-grid predicate with a compatibility-safe standalone virtual interface.
// mask also supplies query flags. Keep predicate/getFlags/performCulling/predicateSurface order,
// getFlags constness, and the absence of a virtual destructor.
class ObjectDrawerFlagPredicator
{
public:
    explicit ObjectDrawerFlagPredicator(unsigned int mask) : m_mask(mask) {}

    // every bit of the mask must be present, not merely one of them.
    virtual bool predicate(world::IObject* object)
    {
        return (m_mask & object->getFlags()) == m_mask;
    }
    virtual unsigned int getFlags() const       { return m_mask; }
    virtual bool performCulling(world::IObject*) { return true; }
    virtual bool predicateSurface(int)           { return true; }

private:
    unsigned int m_mask;
};


// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: RW-objectdrawer-census]
// Counts rejection and missing geometry on the first drawn frame. updateGeometry only
// updates transforms; submission also requires a geometry instance.
// Remove this census and its two call sites once the first-frame result is established.

struct ObjectDrawerCensus
{
    unsigned int seen;
    unsigned int rejectedFlags;
    unsigned int withGeometry;
    unsigned int withGeometryPart;
    unsigned int nothingToDraw;
};

ObjectDrawerCensus g_objectDrawerCensus = { 0, 0, 0, 0, 0 };
bool               g_objectDrawerCensusReported = false;

void objectDrawerCensusReport()
{
    if (g_objectDrawerCensusReported)
        return;
    g_objectDrawerCensusReported = true;

    const ObjectDrawerCensus& c = g_objectDrawerCensus;
    std::fprintf(stderr,
                 "[CENSUS] ObjectDrawer first frame: %u seen, %u rejected by flags, "
                 "%u WITH geometry, %u with getGeometryPart>0, %u with NOTHING to draw\n",
                 c.seen, c.rejectedFlags, c.withGeometry, c.withGeometryPart, c.nothingToDraw);
    std::fflush(stderr);
}

// Updates visual transforms and decals for an object; geometry submission runs in the deferred pass.
// Null objects are ignored. Typed calls preserve platform-specific virtual ordering.
void objectDrawerDrawObject(world::IObject* object)
{
    if (!object)
        return;

    ++g_objectDrawerCensus.seen;                               // [TEMP] census

    const unsigned int flags = object->getFlags();
    if ((flags & world::IObject::OFIsDisabled) != 0 || (flags & world::IObject::OFIsVisible) == 0)
    {
        ++g_objectDrawerCensus.rejectedFlags;                  // [TEMP] census
        return;
    }

    if (void* const decal = object->getDecalObject())            // PE +0xD8
    {
        typedef void (__fastcall* DecalFn)(void*, void*, int);
        void** const vtbl = *reinterpret_cast<void***>(decal);
        reinterpret_cast<DecalFn>(vtbl[0x0C / sizeof(void*)])(decal, 0, 1);
    }

    if (world::object_detail::LayoutAccess::geometry(object))
    {
        ++g_objectDrawerCensus.withGeometry;                      // [TEMP] census
        object->updateGeometry();                                 // PE +0xB4
    }
    else if (object->getGeometryPart() > 0)                       // PE +0xC4
    {
        ++g_objectDrawerCensus.withGeometryPart;                  // [TEMP] census
        // THE result IS discarded.  0x100F7C1E calls it and drops the matrix; the only
        // effect that can matter is whatever recomputing it does.  Shipped, reproduced.
        object->getAbsoluteTransformation();                      // PE +0x80
        object->updateGeometry();
    }
    else
    {
        ++g_objectDrawerCensus.nothingToDraw;                     // [TEMP] census
    }
}

// byte_10239638 - the alternating gate below.  [INFERRED] name.
unsigned char g_objectDrawerVisualTick = 0;

// sub_100F7E80 - the per-object visual update: hand each object the frame time and its distance
// from the camera.

// The +0x0C flag limits visual updates to alternating frames using the shared toggle.
class ObjectDrawerFlagPredicator;

// Defined below beside the other set accessors; declared here because the visual update is
// the first body that walks a set.
world::IObject** objectDrawerSetBegin(ObjectDrawerSet& set);
world::IObject** objectDrawerSetEnd(ObjectDrawerSet& set);

void objectDrawerVisualDistanceUpdate(ObjectDrawer* self, ObjectDrawerSet& set, float time)
{
    if (self->m_byte0C && g_objectDrawerVisualTick != 1)
        return;

    dx9rend::IRenderView* const view = static_cast<dx9rend::IRenderView*>(self->m_renderView);

    for (world::IObject** cursor = objectDrawerSetBegin(set);
         cursor != objectDrawerSetEnd(set); ++cursor)
    {
        world::IObject* const object = *cursor;

        // *(object + 5) & 0x10 - bit 12 of m_flags, read as a byte.  The enum does not name it.
        if ((object->getFlags() & 0x00001000u) == 0)
            continue;

        // The camera is the view matrix translation row; the object's is its visual transform.
        const Mat4* const viewMatrix = view->getViewMatrixPtr();          // +0x64
        const Mat4& objectMatrix = object->getVisualAbsoluteTransformation();  // PE +0x8C

        const float dx = viewMatrix->m[12] - objectMatrix.m[12];
        const float dy = viewMatrix->m[13] - objectMatrix.m[13];
        const float dz = viewMatrix->m[14] - objectMatrix.m[14];
        const float distance = static_cast<float>(::sqrt(static_cast<double>(dx * dx + dy * dy + dz * dz)));

        object->handleVisualUpdate(time, distance);                        // PE +0xA4
    }

    g_objectDrawerVisualTick = self->m_byte0C ? 0u : 1u;
}

// Compares geometry sort keys; missing geometry compares false against every object.
// [PLATFORM] This breaks strict weak ordering. Iterator debug checks must remain disabled.
struct ObjectDrawerGeometrySortKey
{
    bool operator()(world::IObject* lhs, world::IObject* rhs) const
    {
        void* const lg = world::object_detail::LayoutAccess::geometry(lhs);
        void* const rg = world::object_detail::LayoutAccess::geometry(rhs);
        if (!lg || !rg)
            return false;

        typedef unsigned int (__fastcall* KeyFn)(void*, void*);
        void** const lv = *reinterpret_cast<void***>(lg);
        void** const rv = *reinterpret_cast<void***>(rg);
        return reinterpret_cast<KeyFn>(lv[0x38 / sizeof(void*)])(lg, 0)
             < reinterpret_cast<KeyFn>(rv[0x38 / sizeof(void*)])(rg, 0);
    }
};

// the four set accessors, the only places that know which flavour is being built
// See the ABI note in the header: `native` uses the host's own std::vector, `compat` mirrors retail
// VC7.1's sixteen-byte shape.  Everything else below works in terms of IObject, so the two
// flavours share one body.

world::IObject** objectDrawerSetBegin(ObjectDrawerSet& set)
{
#if BF2_PLUGIN_ABI_NATIVE
    return set.empty() ? 0 : &set[0];
#else
    return reinterpret_cast<world::IObject**>(set.first);
#endif
}

world::IObject** objectDrawerSetEnd(ObjectDrawerSet& set)
{
#if BF2_PLUGIN_ABI_NATIVE
    return set.empty() ? 0 : &set[0] + set.size();
#else
    return reinterpret_cast<world::IObject**>(set.last);
#endif
}

void objectDrawerSort(ObjectDrawerSet& set)
{
    world::IObject** const first = objectDrawerSetBegin(set);
    world::IObject** const last  = objectDrawerSetEnd(set);
    if (first && first != last)
        std::sort(first, last, ObjectDrawerGeometrySortKey());
}

// sub_10032330(v, 0, 0) - resize(0) on a vector of pointers: the elements are trivially
// destructible, so retail's resize(0) is nothing but last = first, and clear() is the same thing
// on the native side.  Capacity is retained either way, which is what makes the per-frame reuse
// allocation-free.
void objectDrawerClear(ObjectDrawerSet& set)
{
#if BF2_PLUGIN_ABI_NATIVE
    set.clear();
#else
    set.last = set.first;
#endif
}

unsigned int objectDrawerSetSize(const ObjectDrawerSet& set)
{
#if BF2_PLUGIN_ABI_NATIVE
    return static_cast<unsigned int>(set.size());
#else
    return set.first ? static_cast<unsigned int>((set.last - set.first) >> 2) : 0u;
#endif
}

// Queries host collision/noncollision object grids through the selected compatibility slot.
void objectDrawerQueryGrid(unsigned int slot, const void* frustum, ObjectDrawerSet& out,
                           ObjectDrawerFlagPredicator* predicator, float time)
{
    if (!::dice::hfe::objectManager)
        return;

    // In the native build, out is the host's std::vector; the plugin ABI uses its own layout.
    typedef void (__fastcall* QueryFn)(void*, void*, const void*, void*,
                                       void*, int, float, float);
    void* const manager = ::dice::hfe::objectManager;
    void** const vtbl = *reinterpret_cast<void***>(manager);
    reinterpret_cast<QueryFn>(vtbl[slot / sizeof(void*)])(
        manager, 0, frustum, &out, predicator, 1, 0.0f, time);
}


// [TEMP] [CHOSEN, not recovered] [RETIRE-WHEN: RW-objectdrawer-occluders]
// Reports skipped occlusion work and leaves the candidate set unchanged, drawing too much.
// Reports include the caller, set size and a per-site hit count (first three, then every 256th).
// Retire when silhouette-plane construction and the enabled occluder-cull arm are implemented
// and validated against the geometry instance's face, edge and vertex tables.

void objectDrawerOccluderFrontier(unsigned int& hits, const char* qualified, const char* what,
                                  unsigned int setSize)
{
    ++hits;
    if (hits > 3u && (hits & 0xFFu) != 0u)
        return;
    std::fprintf(stderr,
                 "[FRONTIER] %s reached: %s - the set of %u object(s) is left UN-CULLED, "
                 "REPORTED AND CONTINUED, hit %u\n",
                 qualified, what, setSize, hits);
    std::fflush(stderr);
}

// Submits each geometry in the object's chain using the drawer's view and occluder.
// The unmodeled interface uses +0x18 for submission and +0xCC for the next geometry.
void objectDrawerDeferredGeometryPass(ObjectDrawer* self, world::IObject* object)
{
    if (!object)
        return;

    const unsigned int flags = object->getFlags();
    if ((flags & world::IObject::OFIsDisabled) != 0 ||
        (flags & world::IObject::OFIsVisible) == 0)
        return;

    void* geometry = world::object_detail::LayoutAccess::geometry(object);
    while (geometry)
    {
        typedef void  (__fastcall* PassFn)(void*, void*, void*, void*, int);
        typedef void* (__fastcall* NextFn)(void*, void*);
        void** const vtbl = *reinterpret_cast<void***>(geometry);
        reinterpret_cast<PassFn>(vtbl[0x18 / sizeof(void*)])(
            geometry, 0, self->m_renderView, self->m_occluder, 1);
        geometry = reinterpret_cast<NextFn>(vtbl[0xCC / sizeof(void*)])(geometry, 0);
    }
}

void* const kObjectDrawerPrimaryVtable[16] =
{
    reinterpret_cast<void*>(&od_addRef),           // +0x00
    reinterpret_cast<void*>(&od_getRef),           // +0x04
    reinterpret_cast<void*>(&od_release),          // +0x08
    reinterpret_cast<void*>(&od_queryInterface),   // +0x0C
    reinterpret_cast<void*>(&od_draw),             // +0x10
    reinterpret_cast<void*>(&od_updateVisible),    // +0x14
    reinterpret_cast<void*>(&od_slot18),           // +0x18
    reinterpret_cast<void*>(&od_slot1C),           // +0x1C
    reinterpret_cast<void*>(&od_slot20),           // +0x20
    reinterpret_cast<void*>(&od_getObjectVector),  // +0x24
    reinterpret_cast<void*>(&od_setRenderView),    // +0x28
    reinterpret_cast<void*>(&od_getRenderView),    // +0x2C
    reinterpret_cast<void*>(&od_setOccluder),      // +0x30
    reinterpret_cast<void*>(&od_getOccluder),      // +0x34
    reinterpret_cast<void*>(&od_slot38),           // +0x38
    reinterpret_cast<void*>(&od_slot3C)            // +0x3C
};

void* const kObjectDrawerEventVtable[1] =
{
    reinterpret_cast<void*>(&od_event00)           // +0x00
};

// sub_100F9A80 - the sentinel allocator both containers use.  The constructor then self-links all
// three of the node's pointers and sets the byte at +0x19, which is what identifies the container
// as a _Tree rather than a list; see the header.
void* objectDrawerTreeSentinel()
{
    unsigned char* const node = static_cast<unsigned char*>(::renddx9abi::engineAlloc(0x20));
    if (!node)
        return 0;

    node[0x19] = 1;
    *reinterpret_cast<void**>(node + 0x00) = node;
    *reinterpret_cast<void**>(node + 0x04) = node;
    *reinterpret_cast<void**>(node + 0x08) = node;
    return node;
}

} // anonymous namespace


// Initializes drawer state and registers its ECCore subobject at priority zero.

ObjectDrawer::ObjectDrawer()
    : m_vptr(kObjectDrawerPrimaryVtable)
    , m_eventVptr(kObjectDrawerEventVtable)
    , m_refCount(1)
    , m_byte0C(0)
    , m_updateVisibleHasRun(0)
    , m_byte0E(0)
    , m_renderView(0)
    , m_occluder(0)
    , m_byte60(1)
    , m_float64(60.0f)
    , m_byte68(1)
    , m_dword6C(0)
    , m_byte70(0)
    , m_culledCount(0)
    , m_dword78(0)
    , m_dword7C(0)
    , m_dword80(0)
    , m_dword84(0)
    , m_byte88(0)
{
    m_tree10.comp = 0;
    m_tree10.head = objectDrawerTreeSentinel();
    m_tree10.size = 0;

    m_tree1C.comp = 0;
    m_tree1C.head = objectDrawerTreeSentinel();
    m_tree1C.size = 0;

    // +0x30 IS NOT zeroed and that is the shipped behaviour - see the header's note on the
    // sixteen-byte vector shape.  The other three words of each are.
#if !BF2_PLUGIN_ABI_NATIVE
    m_visibleSet.first = 0;  m_visibleSet.last = 0;  m_visibleSet.end = 0;
    m_drawSet.first    = 0;  m_drawSet.last    = 0;  m_drawSet.end    = 0;
#endif
    // +0x50 is the retail shape in both flavours (see the header).
    m_vector50.first   = 0;  m_vector50.last   = 0;  m_vector50.end   = 0;

    // EventManager vslot +0x2C, registerEventHandler(category 0, &subobject, 0).
    if (::dice::hfe::eventManager)
    {
        ::dice::hfe::eventManager->registerEventHandler(
            static_cast<EventCategory>(0),
            reinterpret_cast<IEventListener*>(&m_eventVptr),
            0);
    }
}

// the IBase quartet

void ObjectDrawer::addRef()                                  // 0x1001A010, the shared body
{
    ++m_refCount;
}

int ObjectDrawer::getRef()                                   // 0x10051870, the shared body
{
    return m_refCount;
}

// Decrement, and on zero run the destructor and free through the engine allocator.
int ObjectDrawer::release()
{
    const int remaining = --m_refCount;
    if (remaining == 0)
    {
        // [FRONTIER] dice::hfe::geom::ObjectDrawer::~ObjectDrawer - the destructor at the address
        // ObjectDrawer_release calls is not read yet.  Nothing releases this object in this build
        // (it is never created), so the free below is the honest half of the pair rather than a
        // fabricated teardown.
        ::renddx9abi::engineFree(this);
    }
    return remaining;
}

// Two ids answer, everything else is null.  IT IS NOT const-correct in the image -
// the slot takes the object non-const - but every recovered reader treats it as a query.
void* ObjectDrawer::queryInterface(unsigned int iid) const
{
    if (iid == 1u)
        return const_cast<ObjectDrawer*>(this);
    if (iid == IID_IObjectDrawer)
        return const_cast<ObjectDrawer*>(this);
    return 0;
}

// the leaf slots

// 0x100FAFD0, 19 B.  note THE sense: zero selects the first vector (+0x30), anything else the
// second (+0x40).  The shipped body computes `this + 64` first and only then branches.
ObjectDrawerSet* ObjectDrawer::getObjectVector(int which)
{
    return which ? &m_drawSet : &m_visibleSet;
}

void ObjectDrawer::setRenderView(void* view)
{
    m_renderView = view;
}

void* ObjectDrawer::getRenderView() const
{
    return m_renderView;
}

// Releases the previous occluder before retaining the replacement, even when they are identical.
void ObjectDrawer::setOccluder(IBase* occluder)
{
    if (m_occluder)
    {
        m_occluder->release();
        m_occluder = 0;
    }

    m_occluder = occluder;

    if (occluder)
        occluder->addRef();
}

IBase* ObjectDrawer::getOccluder() const
{
    return m_occluder;
}

// Builds and sorts visible candidates, applies occlusion, then stores camera distances.
// Requires setRenderView before the call.

void ObjectDrawer::updateVisible(float time)
{
    objectDrawerClear(m_visibleSet);                       // sub_10032330(this+0x30, 0, 0)

    ObjectDrawerFlagPredicator predicator(world::IObject::OFIsVisible);   // sub_101C7E90(2)

    dx9rend::IRenderView* const view = static_cast<dx9rend::IRenderView*>(m_renderView);
    objectDrawerQueryGrid(0x40, view->getFrustumPtr(), m_visibleSet, &predicator, time);

    objectDrawerSort(m_visibleSet);

    if (m_byte88)
    {
        RD9_FRONTIER_AT("dice::hfe::geom::ObjectDrawer::updateVisible (sub_100F8750, the +0x88 "
                        "pass - unread; the constructor leaves +0x88 clear so nothing has reached "
                        "it yet)", this, 0x14);
    }

    m_dword7C = 0;
    m_dword80 = 0;
    m_dword84 = 0;

    m_culledCount = static_cast<int>(objectDrawerSetSize(m_visibleSet));

    if (m_byte60)
    {
        // [FRONTIER] the occluder pair - reports AND continues under the owner exception recorded
        // beside objectDrawerOccluderFrontier above.  sub_100F9300 builds silhouette planes out of
        // the visible set into the vector at +0x50; sub_100F8430 then tests every object against
        // them and erases the culled ones with a memmove.  Neither is transcribed.
        // [RETIRE-WHEN: RW-objectdrawer-occluders]
        static unsigned int hits = 0;
        objectDrawerOccluderFrontier(
            hits, "dice::hfe::geom::ObjectDrawer::updateVisible",
            "the occluder pair sub_100F9300 (silhouette planes into +0x50) + sub_100F8430 (the "
            "cull), gated on +0x60 which the constructor sets to 1",
            objectDrawerSetSize(m_visibleSet));
    }

    m_culledCount -= static_cast<int>(objectDrawerSetSize(m_visibleSet));

    objectDrawerVisualDistanceUpdate(this, m_visibleSet, time);

    m_updateVisibleHasRun = 1;                             // +0x0D
}


// Draws the visible collision set, then separately queries and draws noncollision objects.

void ObjectDrawer::draw(float time)
{
    if (!m_updateVisibleHasRun)
    {
        // The shipped diagnostic is sub_10074300(6, __FILE__, 598, "") streamed into - dice's own
        // file and line.  This image logs instead of asserting; the text is theirs.
        ::renddx9abi::logf("ObjectDrawer.cpp:598 updateVisible() must be drawn before this!");
        return;
    }

    for (world::IObject** cursor = objectDrawerSetBegin(m_visibleSet);
         cursor != objectDrawerSetEnd(m_visibleSet); ++cursor)
    {
        objectDrawerDrawObject(*cursor);
    }

    objectDrawerClear(m_drawSet);                          // sub_10032330(this+0x40, 0, 0)

    ObjectDrawerFlagPredicator predicator(world::IObject::OFIsVisible);

    dx9rend::IRenderView* const view = static_cast<dx9rend::IRenderView*>(m_renderView);
    objectDrawerQueryGrid(0x4C, view->getFrustumPtr(), m_drawSet, &predicator, time);

    objectDrawerSort(m_drawSet);

    m_dword78 = objectDrawerSetSize(m_drawSet);

    if (m_byte60)
    {
        // [FRONTIER] sub_100F8430, the occlusion cull - see updateVisible and the owner exception
        // recorded beside objectDrawerOccluderFrontier.
        // [RETIRE-WHEN: RW-objectdrawer-occluders]
        static unsigned int hits = 0;
        objectDrawerOccluderFrontier(
            hits, "dice::hfe::geom::ObjectDrawer::draw",
            "the occlusion cull sub_100F8430, gated on +0x60 which the constructor sets to 1",
            objectDrawerSetSize(m_drawSet));
    }

    m_dword78 -= objectDrawerSetSize(m_drawSet);

    objectDrawerVisualDistanceUpdate(this, m_drawSet, time);

    // The same per-object draw again, inlined in the shipped body.
    for (world::IObject** cursor = objectDrawerSetBegin(m_drawSet);
         cursor != objectDrawerSetEnd(m_drawSet); ++cursor)
    {
        objectDrawerDrawObject(*cursor);
    }

    m_byte0E = 1;                                          // +0x0E

    objectDrawerCensusReport();                            // [TEMP] census, once
}


// Submits the deferred geometry pass once per updateVisible/draw pair.
// Consumes both completion flags before callbacks, so reentrant calls see them cleared.

void ObjectDrawer::slot_0x18()
{
    ObjectDrawerFlagPredicator predicator(world::IObject::OFIsVisible);   // built, never read
    (void)predicator;

    if (!m_updateVisibleHasRun || !m_byte0E)
    {
        // sub_10074300(6, __FILE__, 638, "") streamed into - dice's own file and line.  This image
        // logs where retail asserts; the text is theirs.
        ::renddx9abi::logf("ObjectDrawer.cpp:638 updateVisible() must be drawn before this!");
        return;
    }

    m_updateVisibleHasRun = 0;
    m_byte0E = 0;

    for (world::IObject** cursor = objectDrawerSetBegin(m_visibleSet);
         cursor != objectDrawerSetEnd(m_visibleSet); ++cursor)
    {
        objectDrawerDeferredGeometryPass(this, *cursor);
    }

    // The same body again over the draw set, inlined in the shipped code.
    for (world::IObject** cursor = objectDrawerSetBegin(m_drawSet);
         cursor != objectDrawerSetEnd(m_drawSet); ++cursor)
    {
        objectDrawerDeferredGeometryPass(this, *cursor);
    }
}
void ObjectDrawer::slot_0x1C()           { od_slot1C(this, 0); }
void ObjectDrawer::slot_0x20(void* o)    { od_slot20(this, 0, o); }


// Tests geometry against the occluder set and counts hidden objects.
// Precise mode falls back from the sphere to box corners when a plane intersects it.
// An occluder with no planes hides every tested object; an empty set hides none.

namespace {

// The geometry instance slots, reached by offset through the instance's retail-ordered table as the
// deferred pass above does (StaticMesh / BundledMesh / SkinnedMesh install the same arity at each).
inline void* occlusionGeometrySlot(void* geometry, unsigned int offset)
{
    return (*reinterpret_cast<void***>(geometry))[offset / sizeof(void*)];
}

inline const LocalBoundingBox& occlusionGeometryLocalBounds(void* geometry)          // +0x40
{
    typedef const void* (__fastcall* BoundsFn)(void*, void*);
    return *static_cast<const LocalBoundingBox*>(
        reinterpret_cast<BoundsFn>(occlusionGeometrySlot(geometry, 0x40))(geometry, 0));
}

inline float occlusionGeometryBoundingRadius(void* geometry)                         // +0x44
{
    typedef float (__fastcall* RadiusFn)(void*, void*);
    return reinterpret_cast<RadiusFn>(occlusionGeometrySlot(geometry, 0x44))(geometry, 0);
}

inline const Mat4* occlusionGeometryTransformation(void* geometry)                  // +0x48
{
    typedef const Mat4* (__fastcall* TransformFn)(void*, void*);
    return reinterpret_cast<TransformFn>(occlusionGeometrySlot(geometry, 0x48))(geometry, 0);
}

// VC7.1 size(): a signed byte span divided by the element size, compared unsigned (`jnb`).
inline unsigned int occlusionVectorCount(const char* first, const char* last)
{
    return static_cast<unsigned int>((last - first) / 16);
}

// The eight-corner object box - 0x100AE110 (the 24 floats zeroed), 0x101C6750 (corner i takes x from
// max when bit 2, y when bit 1, z when bit 0) and 0x101C67F0 (all three rows formed from the untouched
// corner; corner 0's y and x rows associate differently from corners 1..7). The same bodies as
// XpLightManager.cpp's ShadowBoxCorners, re-read for this slot.
struct OcclusionBoxCorners
{
    Vec3 corner[8];

    OcclusionBoxCorners()
    {
        for (unsigned int i = 0; i < 8; ++i)
            corner[i].x = corner[i].y = corner[i].z = 0.0f;
    }

    void setBounds(const LocalBoundingBox& bounds)
    {
        for (unsigned int i = 0; i < 8; ++i)
        {
            corner[i].x = (i & 4u) ? bounds.m_max.x : bounds.m_min.x;
            corner[i].y = (i & 2u) ? bounds.m_max.y : bounds.m_min.y;
            corner[i].z = (i & 1u) ? bounds.m_max.z : bounds.m_min.z;
        }
    }

    void transform(const Mat4& world)
    {
        const float* const m = world.m;
        for (unsigned int i = 0; i < 8; ++i)
        {
            const float x = corner[i].x;
            const float y = corner[i].y;
            const float z = corner[i].z;
            const float tz = z * m[10] + x * m[2] + y * m[6] + m[14];
            const float ty = i == 0 ? x * m[1] + y * m[5] + z * m[9] + m[13]
                                    : x * m[1] + z * m[9] + y * m[5] + m[13];
            const float tx = i == 0 ? z * m[8] + x * m[0] + y * m[4] + m[12]
                                    : y * m[4] + z * m[8] + x * m[0] + m[12];
            corner[i].x = tx;
            corner[i].y = ty;
            corner[i].z = tz;
        }
    }
};

} // anonymous namespace

namespace {

// Table thunks use __fastcall with an unused EDX to match the __thiscall slots.
// Keep definitions in the anonymous namespace containing their declarations.
bool __fastcall od_slot38(void* self, void*, void* geometry)
{
    return static_cast<ObjectDrawer*>(self)->isGeometryOccluded(geometry);
}

bool __fastcall od_slot3C(void* self, void*, void* bounds)
{
    return static_cast<ObjectDrawer*>(self)->isBoundsOccluded(static_cast<LocalBoundingBox*>(bounds));
}

} // anonymous namespace

bool ObjectDrawer::isGeometryOccluded(void* geometry)
{
    if (!m_byte60)
        return false;
    const Mat4* const transform = occlusionGeometryTransformation(geometry);  // +0x48
    const Vec3& centre = *reinterpret_cast<const Vec3*>(&transform->m[12]);
    const float radius = occlusionGeometryBoundingRadius(geometry);         // +0x44

    if (m_byte68)
    {
        OcclusionBoxCorners box;
        bool boxBuilt = false;                                               // [ebp-1]
        for (unsigned int occluder = 0; ; ++occluder)
        {
            if (!m_vector50.first || occluder >= occlusionVectorCount(m_vector50.first, m_vector50.last))
                break;
            const char* const record = m_vector50.first + occluder * 16;
            bool hidden = true;                                              // [ebp-2]
            for (unsigned int index = 0; ; ++index)
            {
                const char* const planes = *reinterpret_cast<const char* const*>(record + 4);
                if (!planes ||
                    index >= occlusionVectorCount(planes, *reinterpret_cast<const char* const*>(record + 8)))
                    break;
                const Plane3& plane = *reinterpret_cast<const Plane3*>(planes + index * 16);
                const float distance = plane.normal.z * centre.z + plane.normal.y * centre.y
                                     + plane.normal.x * centre.x + plane.d;
                if (distance > 0.0f)
                {
                    hidden = false;                                          // straight to the
                    break;                                                   // next occluder
                }
                if (distance > -radius)
                {
                    if (!boxBuilt)
                    {
                        box.setBounds(occlusionGeometryLocalBounds(geometry));       // +0x40
                        box.transform(*occlusionGeometryTransformation(geometry));   // +0x48
                        boxBuilt = true;
                    }
                    for (unsigned int c = 0; c < 8; ++c)
                    {
                        const Vec3& k = box.corner[c];
                        if (k.x * plane.normal.x + k.z * plane.normal.z + plane.normal.y * k.y + plane.d > 0.0f)
                        {
                            hidden = false;                                  // 100F8373, next plane
                            break;
                        }
                    }
                }
            }
            if (hidden)
            {
                ++m_dword80;
                return true;
            }
        }
        return false;
    }

    for (unsigned int occluder = 0; ; ++occluder)
    {
        if (!m_vector50.first || occluder >= occlusionVectorCount(m_vector50.first, m_vector50.last))
            break;
        const char* const record = m_vector50.first + occluder * 16;
        const char* const planes = *reinterpret_cast<const char* const*>(record + 4);
        bool hidden = true;
        for (unsigned int index = 0;
             planes && index < occlusionVectorCount(planes, *reinterpret_cast<const char* const*>(record + 8));
             ++index)
        {
            const Plane3& plane = *reinterpret_cast<const Plane3*>(planes + index * 16);
            const float distance = plane.normal.z * centre.z + plane.normal.x * centre.x
                                 + centre.y * plane.normal.y + plane.d;
            if (-radius < distance)                                          // 100F8409 `jnp`
            {
                hidden = false;
                break;
            }
        }
        if (hidden)
        {
            ++m_dword80;
            return true;
        }
    }
    return false;
}

// Tests whether an axis-aligned world-space box is hidden by an occluder.
// bounds must be valid; enabled queries finalize its derived data in place.
// Uses the origin-based radius around the box centre. An occluder with no planes hides it.
// Without the silhouette builder's populated occluders, returns false (RW-objectdrawer-occluders).
bool ObjectDrawer::isBoundsOccluded(LocalBoundingBox* bounds)
{
    if (!m_byte60)
        return false;
    if (bounds->m_derivedDataDirty)
        meshBoundsFinalize(*reinterpret_cast<MeshBounds*>(bounds));
    if (bounds->m_derivedDataDirty)
        meshBoundsFinalize(*reinterpret_cast<MeshBounds*>(bounds));
    const float radius = bounds->m_radiusFromOrigin;                         // +0x18
    const Vec3& centre = bounds->m_centre;                                   // +0x30

    if (m_byte68)
    {
        for (unsigned int occluder = 0; ; ++occluder)
        {
            if (!m_vector50.first || occluder >= occlusionVectorCount(m_vector50.first, m_vector50.last))
                break;
            const char* const record = m_vector50.first + occluder * 16;
            bool hidden = true;                                              // [ebp-1]
            for (unsigned int index = 0; ; ++index)
            {
                const char* const planes = *reinterpret_cast<const char* const*>(record + 4);
                if (!planes ||
                    index >= occlusionVectorCount(planes, *reinterpret_cast<const char* const*>(record + 8)))
                    break;
                const Plane3& plane = *reinterpret_cast<const Plane3*>(planes + index * 16);
                const float distance = centre.y * plane.normal.y + plane.normal.x * centre.x
                                     + centre.z * plane.normal.z + plane.d;
                if (distance > 0.0f)
                {
                    hidden = false;                                          // straight to the
                    break;                                                   // next occluder
                }
                if (distance > -radius)
                {
                    for (unsigned int c = 0; c < 8; ++c)
                    {
                        const float kx = (c & 4u) ? bounds->m_max.x : bounds->m_min.x;
                        const float ky = (c & 2u) ? bounds->m_max.y : bounds->m_min.y;
                        const float kz = (c & 1u) ? bounds->m_max.z : bounds->m_min.z;
                        if (ky * plane.normal.y + kz * plane.normal.z + kx * plane.normal.x + plane.d > 0.0f)
                        {
                            hidden = false;                                  // 100F8138, next plane
                            break;
                        }
                    }
                }
            }
            if (hidden)
            {
                ++m_dword84;
                return true;
            }
        }
        return false;
    }

    for (unsigned int occluder = 0; ; ++occluder)
    {
        if (!m_vector50.first || occluder >= occlusionVectorCount(m_vector50.first, m_vector50.last))
            break;
        const char* const record = m_vector50.first + occluder * 16;
        const char* const planes = *reinterpret_cast<const char* const*>(record + 4);
        bool hidden = true;
        for (unsigned int index = 0;
             planes && index < occlusionVectorCount(planes, *reinterpret_cast<const char* const*>(record + 8));
             ++index)
        {
            const Plane3& plane = *reinterpret_cast<const Plane3*>(planes + index * 16);
            const float distance = plane.normal.z * centre.z + plane.normal.x * centre.x
                                 + centre.y * plane.normal.y + plane.d;
            if (-radius < distance)                                          // 100F81DA `jnp`
            {
                hidden = false;
                break;
            }
        }
        if (hidden)
        {
            ++m_dword84;
            return true;
        }
    }
    return false;
}


// RefFactory_geom_ObjectDrawer - 0x100B0ADE.  operator new(0x8C), the constructor, then
// queryInterface; the same shape as every other factory in this image.

// declared AND NOT registered.  ModuleGeom does not list this class.  See the header.

IBase* __fastcall createObjectDrawer(unsigned int iid, IBase* outer)
{
    if (outer)
        return 0;

    void* const storage = ::renddx9abi::engineAlloc(sizeof(ObjectDrawer));
    if (!storage)
        return 0;

    ObjectDrawer* const self = new (storage) ObjectDrawer();

    void* const itf = self->queryInterface(iid);
    if (!itf)
        self->release();

    return static_cast<IBase*>(itf);
}

} // namespace geom
} // namespace hfe
} // namespace dice

#endif // BF2_IMAGE_RENDDX9
