// [TEMP] [CHOSEN, not recovered] Production-run observer, linked only by the soak fixture.
// No engine object or virtual method is replaced. Global new/delete use malloc/free while
// recording containing allocations, including modern MSVC's interior-aligned vector data.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <new>
#include <string>
#include <vector>
#include <map>
#define private public
#include "../BF2/Physics/CollisionManager.h"
#include "../BF2/Physics/CollisionMeshTemplate.h"
#undef private

using namespace dice::hfe::world;
namespace {
const unsigned blockCapacity = 1u << 21, watchCapacity = 262144, meshCapacity = 8192;
struct Block { void* p; size_t size; int watch; };
struct Watch { void* p; unsigned mesh, order; bool array; unsigned hits, actualOrder; bool form; };
struct Mesh { void* p; char name[160]; unsigned count, frees; void* stack[24]; USHORT depth; };
Block blocks[blockCapacity];
Watch watches[watchCapacity];
Mesh meshes[meshCapacity];
unsigned watchCount, meshCount, errors;
SRWLOCK lock = SRWLOCK_INIT;
void* const tombstone = reinterpret_cast<void*>(1);
unsigned hash(void* p) { return (unsigned(reinterpret_cast<uintptr_t>(p) >> 4) * 2654435761u) & (blockCapacity - 1); }
Block* findBlock(void* p)
{
    unsigned slot = hash(p);
    for (unsigned i = 0; i < blockCapacity; ++i, slot = (slot + 1) & (blockCapacity - 1))
    {
        if (!blocks[slot].p) return 0;
        if (blocks[slot].p == p) return &blocks[slot];
    }
    return 0;
}
void* allocate(size_t size)
{
    if (!size) size = 1;
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    AcquireSRWLockExclusive(&lock);
    unsigned slot = hash(p), i = 0;
    for (; i < blockCapacity; ++i, slot = (slot + 1) & (blockCapacity - 1))
        if (!blocks[slot].p || blocks[slot].p == tombstone) break;
    if (i == blockCapacity) std::abort();
    blocks[slot] = {p, size, -1};
    ReleaseSRWLockExclusive(&lock);
    return p;
}
void release(void* p, bool array)
{
    if (!p) return;
    AcquireSRWLockExclusive(&lock);
    Block* block = findBlock(p);
    if (block)
    {
        if (block->watch >= 0)
        {
            Watch& w = watches[block->watch];
            Mesh& m = meshes[w.mesh];
            ++w.hits;
            w.actualOrder = ++m.frees;
            w.form = array == w.array;
            if (!m.depth) m.depth = CaptureStackBackTrace(0, 24, m.stack, 0);
        }
        block->p = tombstone;
        block->watch = -1;
    }
    ReleaseSRWLockExclusive(&lock);
    std::free(p);
}
int compareBlocks(const void* a, const void* b)
{
    uintptr_t x = reinterpret_cast<uintptr_t>(static_cast<const Block*>(a)->p);
    uintptr_t y = reinterpret_cast<uintptr_t>(static_cast<const Block*>(b)->p);
    return x < y ? -1 : x != y;
}
Block* sorted;
unsigned sortedCount;
void track(void* p, unsigned mesh, bool array = false)
{
    if (!p) return;
    uintptr_t address = reinterpret_cast<uintptr_t>(p);
    unsigned lo = 0, hi = sortedCount;
    while (lo < hi)
    {
        unsigned mid = lo + (hi - lo) / 2;
        if (reinterpret_cast<uintptr_t>(sorted[mid].p) <= address) lo = mid + 1;
        else hi = mid;
    }
    if (!lo || address - reinterpret_cast<uintptr_t>(sorted[lo - 1].p) >= sorted[lo - 1].size)
    { ++errors; std::printf("[collision observer] allocation missing %p mesh=%u\n", p, mesh); return; }
    p = sorted[lo - 1].p;
    AcquireSRWLockExclusive(&lock);
    Block* block = findBlock(p);
    if (!block || watchCount == watchCapacity) std::abort();
    if (block->watch < 0)
    {
        unsigned order = ++meshes[mesh].count;
        block->watch = int(watchCount);
        watches[watchCount++] = {p, mesh, order, array, 0, 0, false};
    }
    else ++errors; // An unexpected shared owner invalidates this observer's ownership claim.
    ReleaseSRWLockExclusive(&lock);
}
template<class T> void trackVector(std::vector<T>& v, unsigned mesh)
{ if (v.capacity()) track(v.data(), mesh); }
void trackLegacy(BspNode& node, unsigned mesh)
{
    track(node.faces, mesh, true);
    if (node.childNode1) { trackLegacy(*node.childNode1, mesh); track(node.childNode1, mesh); }
    if (node.childNode2) { trackLegacy(*node.childNode2, mesh); track(node.childNode2, mesh); }
}
void trackMesh(CollisionMeshTemplate* mesh, unsigned id)
{
    for (auto& geom : mesh->m_lods)
        for (CollisionLod* lod : geom.lods)
        {
            if (!lod) continue;
            track(lod->faceAdjacency, id, true);
            trackVector(lod->edgeIndices, id);
            trackVector(lod->materials, id);
            Bsp& bsp = lod->bsp;
            if (bsp.bspV8)
            {
                trackVector(bsp.bspV8->m_faceIndices, id);
                trackVector(bsp.bspV8->m_nodes, id);
                trackVector(bsp.bspV8->m_faces, id);
                track(bsp.bspV8, id);
            }
            if (!bsp.m_flag0) trackLegacy(bsp.bspNode, id);
            track(bsp.m_pool38, id, true);
            track(bsp.m_pool34, id, true);
            trackVector(lod->vertices, id);
            trackVector(lod->faces, id);
            track(lod, id);
        }
    track(mesh->m_materialIndex, id); // Retail scalar delete is deliberately observed as scalar.
    if (mesh->m_name.capacity() > 15) track(const_cast<char*>(mesh->m_name.data()), id);
    if (mesh->m_directory.capacity() > 15) track(const_cast<char*>(mesh->m_directory.data()), id);
    for (auto& geom : mesh->m_lods) trackVector(geom.lods, id);
    trackVector(mesh->m_lods, id);
    track(mesh, id);
}
}
void* operator new(size_t n) { return allocate(n); }
void* operator new[](size_t n) { return allocate(n); }
void operator delete(void* p) noexcept { release(p, false); }
void operator delete[](void* p) noexcept { release(p, true); }
void operator delete(void* p, size_t) noexcept { release(p, false); }
void operator delete[](void* p, size_t) noexcept { release(p, true); }

void collisionProductionResetWatch()
{
    // Begin a distinct final-destruction observation after recording the reload interval.
    AcquireSRWLockExclusive(&lock);
    for (Block& b : blocks) b.watch = -1;
    std::memset(watches, 0, sizeof(watches));
    std::memset(meshes, 0, sizeof(meshes));
    watchCount = meshCount = 0;
    ReleaseSRWLockExclusive(&lock);
}

void collisionProductionSnapshot(const char* stage)
{
    // Snapshot on the game thread between frames; no concurrent template registry mutation.
    CollisionManager* manager = static_cast<CollisionManager*>(collisionManager);
    if (!manager) { ++errors; return; }
    sorted = static_cast<Block*>(std::malloc(sizeof(Block) * blockCapacity));
    if (!sorted) std::abort();
    sortedCount = 0;
    AcquireSRWLockExclusive(&lock);
    for (const Block& b : blocks) if (b.p && b.p != tombstone) sorted[sortedCount++] = b;
    ReleaseSRWLockExclusive(&lock);
    std::qsort(sorted, sortedCount, sizeof(Block), compareBlocks);
    unsigned loaded = 0, added = 0;
    for (auto& entry : manager->m_collisionMeshTemplates)
        for (CollisionMeshTemplate* mesh : entry.second)
        {
            if (!mesh || !mesh->m_loaded) continue;
            ++loaded;
            AcquireSRWLockExclusive(&lock);
            Block* b = findBlock(mesh);
            const bool alreadyWatched = b && b->watch >= 0;
            ReleaseSRWLockExclusive(&lock);
            if (alreadyWatched) continue;
            if (meshCount == meshCapacity) std::abort();
            unsigned id = meshCount++;
            meshes[id].p = mesh;
            strncpy_s(meshes[id].name, mesh->m_name.c_str(), _TRUNCATE);
            trackMesh(mesh, id);
            ++added;
        }
    std::free(sorted); sorted = 0;
    std::printf("[collision observer] %s: loaded=%u new=%u meshes=%u allocations=%u errors=%u\n",
        stage, loaded, added, meshCount, watchCount, errors);
    std::fflush(stdout);
}
bool collisionProductionReport(const char* stage, bool final)
{
    unsigned freed = 0, bad = 0, complete = 0;
    for (unsigned i = 0; i < watchCount; ++i)
    {
        const Watch& w = watches[i];
        if (w.hits) ++freed;
        if (w.hits && (w.hits != 1 || !w.form || (final && w.order != w.actualOrder)))
        {
            if (bad < 12) std::printf("[collision observer] mismatch mesh=%u ptr=%p hits=%u form=%d order=%u/%u\n",
                w.mesh, w.p, w.hits, w.form, w.actualOrder, w.order);
            ++bad;
        }
    }
    for (unsigned i = 0; i < meshCount; ++i)
    {
        const Mesh& m = meshes[i];
        if (m.count && m.frees == m.count) ++complete;
        if (final)
        {
            std::printf("[collision observer] mesh=%u %s %u/%u stack:", i, m.name, m.frees, m.count);
            for (USHORT j = 0; j < m.depth; ++j) std::printf(" %p", m.stack[j]);
            std::printf("\n");
        }
    }
    std::printf("[collision observer] %s: complete=%u/%u freed=%u/%u bad=%u errors=%u image=%p\n",
        stage, complete, meshCount, freed, watchCount, bad, errors, GetModuleHandleA(0));
    std::fflush(stdout);
    return !errors && !bad && meshCount && (!final || (complete == meshCount && freed == watchCount));
}
