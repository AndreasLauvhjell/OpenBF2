// [TEMP] [CHOSEN, not recovered] x86 component fixture. Allocation observers and geometry
// release probes verify recovered lifetime behavior; this is not a production map/server run.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <cstdint>
#include <string>
#include <vector>
#include <map>
#define private public
#include "../BF2/Physics/CollisionMeshTemplate.h"
#include "../BF2/Physics/CollisionManager.h"
#undef private
#include "../BF2/Core/SettingsRepostitory.h"
#include "../BF2/Core/System.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/IO/Stream/MemStream.h"

using namespace dice::hfe;
using namespace dice::hfe::world;

namespace {
struct Allocation { void* pointer; bool array; int hits, order; bool rightForm; };
Allocation watched[4096];
struct HeapBlock { void* pointer; std::size_t size; };
HeapBlock blocks[16384];
int watchCount = 0, sequence = 0, totalObserved = 0;
int checks = 0, failures = 0;
void* allocate(std::size_t count)
{
    const std::size_t size = count ? count : 1;
    void* pointer = std::malloc(size);
    if (!pointer) throw std::bad_alloc();
    for (HeapBlock& block : blocks)
        if (!block.pointer) { block = {pointer, size}; return pointer; }
    std::abort();
}
void check(bool ok, const char* message)
{
    ++checks; if (!ok) ++failures;
    std::printf("%s %s\n", ok ? "PASS" : "FAIL", message); std::fflush(stdout);
}
void observeFree(void* pointer, bool array)
{
    if (!pointer) return;
    for (int i = 0; i < watchCount; ++i)
        if (watched[i].pointer == pointer)
        {
            Allocation& a = watched[i];
            ++a.hits; a.order = ++sequence; a.rightForm = a.array == array;
        }
    for (HeapBlock& block : blocks)
        if (block.pointer == pointer) { block.pointer = 0; break; }
}
void beginWatch() { watchCount = sequence = 0; }
void track(void* pointer, bool array = false)
{
    if (!pointer) return;
    // Modern STL can align vector storage inside a larger allocation. Resolve the containing
    // new block from our allocator observations, without assuming any STL header or offset.
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(pointer);
    for (const HeapBlock& block : blocks)
    {
        const std::uintptr_t start = reinterpret_cast<std::uintptr_t>(block.pointer);
        if (block.pointer && address >= start && address - start < block.size)
        { pointer = block.pointer; break; }
    }
    if (watchCount == 4096) std::abort();
    watched[watchCount++] = {pointer, array, 0, 0, false};
}
template<class T> void trackVector(std::vector<T>& data)
{
    if (data.capacity()) track(data.data());
}
void verifyWatch(const char* message)
{
    bool ok = true;
    for (int i = 0; i < watchCount; ++i)
    {
        if (watched[i].hits != 1 || !watched[i].rightForm || watched[i].order != i + 1)
            std::printf("allocation %d/%d: %p, hits=%d, order=%d, form=%d, array=%d\n",
                i + 1, watchCount, watched[i].pointer, watched[i].hits, watched[i].order,
                watched[i].rightForm, watched[i].array);
        ok = ok && watched[i].hits == 1 && watched[i].rightForm && watched[i].order == i + 1;
    }
    totalObserved += watchCount;
    check(ok && watchCount != 0, message);
    beginWatch();
}
void trackBsp(Bsp& bsp)
{
    if (bsp.bspV8)
    {
        trackVector(bsp.bspV8->m_faceIndices);
        trackVector(bsp.bspV8->m_nodes);
        trackVector(bsp.bspV8->m_faces);
        track(bsp.bspV8);
    }
    // Synthetic legacy children are tracked explicitly in that case below.
    track(bsp.m_pool38, true);
    track(bsp.m_pool34, true);
}
void trackLod(CollisionLod* lod)
{
    if (!lod) return;
    track(lod->faceAdjacency, true);
    trackVector(lod->edgeIndices);
    trackVector(lod->materials);
    trackBsp(lod->bsp);
    trackVector(lod->vertices);
    trackVector(lod->faces);
    track(lod);
}
void trackTemplate(CollisionMeshTemplate* mesh)
{
    for (CollisionGeom& geom : mesh->m_lods)
        for (CollisionLod* lod : geom.lods) trackLod(lod);
    track(mesh->m_materialIndex); // Intentional retail scalar delete of its new[] allocation.
    if (mesh->m_name.size() > 15) track(const_cast<char*>(mesh->m_name.data()));
    if (mesh->m_directory.size() > 15) track(const_cast<char*>(mesh->m_directory.data()));
    for (CollisionGeom& geom : mesh->m_lods) trackVector(geom.lods);
    trackVector(mesh->m_lods);
    track(mesh);
}
void emptyGeom(CollisionGeom& geom, int count)
{
    geom.lods.resize(count, 0);
    for (int i = 0; i < 5; ++i) geom.colRemap[i] = -1;
}
int releases[24], releaseCount = 0;
struct GeometryProbe : IBase
{
    CollisionLod* owner;
    int slot, id, calls = 0;
    bool observeOrder = true;
    IBase* replaceSecondary = 0;
    GeometryProbe(CollisionLod* lod, int index, int value) : owner(lod), slot(index), id(value) {}
    void addRef() override {}
    int getRef() override { return 1; }
    int release() override
    {
        ++calls; releases[releaseCount++] = id;
        if (watchCount) check(sequence == 0, "debug releases precede all watched payload frees");
        if (observeOrder)
        {
            bool ownPointerPresent = slot == 0 ? owner->debugMesh == this :
                slot == 1 ? owner->dwordB0 == this : owner->dwordB4 == this;
            check(ownPointerPresent && (slot == 0 || !owner->debugMesh) &&
                (slot != 2 || !owner->dwordB0), "release happens before own clear and after earlier clears");
        }
        if (replaceSecondary) owner->dwordB0 = replaceSecondary;
        return 0;
    }
    void* queryInterface(unsigned int) const override { return 0; }
};
struct Events : EventManager
{
    bool registerEventHandler(EventCategory, IEventListener*, int) override { return true; }
    bool registerEvent(EventCategory, unsigned int, const std::string&) override { return true; }
    bool unregisterEventHandler(EventCategory, IEventListener*) override { return true; }
    bool unregisterEvent(EventCategory, unsigned int) override { return true; }
};

void debugCases()
{
    CollisionMeshTemplate mesh;
    mesh.m_lods.resize(3);
    emptyGeom(mesh.m_lods[0], 0);
    emptyGeom(mesh.m_lods[1], 4);
    emptyGeom(mesh.m_lods[2], 1);
    CollisionLod* lod = mesh.m_lods[1].lods[2] = new CollisionLod;
    mesh.m_lods[1].colRemap[0] = mesh.m_lods[1].colRemap[1] = 2;
    mesh.m_lods[1].colRemap[3] = 0; // Null raw slot is skipped by isLodValid.
    GeometryProbe first(lod, 0, 1), second(lod, 1, 2), third(lod, 2, 3);
    lod->debugMesh = &first; lod->dwordB0 = &second; lod->dwordB4 = &third;
    releaseCount = 0;
    mesh.destroyDebugMeshes(false);
    check(releaseCount == 3 && releases[0] == 1 && releases[1] == 2 && releases[2] == 3,
          "remapped duplicate LOD releases primary and two secondary references exactly once");
    check(!lod->debugMesh && !lod->dwordB0 && !lod->dwordB4, "all three geometry references cleared");
    mesh.destroyDebugMeshes(true);
    check(releaseCount == 3, "repeated cleanup is silent after primary clear; bool does not change traversal");

    lod->dwordB0 = &second; lod->dwordB4 = &third;
    mesh.destroyDebugMeshes(false);
    check(releaseCount == 3 && lod->dwordB0 == &second && lod->dwordB4 == &third,
          "retail primary-null gate leaves both secondary references untouched");
    lod->debugMesh = &first; lod->dwordB0 = 0;
    releaseCount = 0;
    mesh.destroyDebugMeshes(true);
    check(releaseCount == 2 && releases[0] == 1 && releases[1] == 3,
          "missing first secondary still permits third release");

    GeometryProbe replacement(lod, 1, 4);
    first.replaceSecondary = &replacement;
    lod->debugMesh = &first; lod->dwordB0 = &second; lod->dwordB4 = &third;
    releaseCount = 0;
    mesh.destroyDebugMeshes(false);
    check(releaseCount == 3 && releases[1] == 4 && second.calls == 1,
          "secondary reference is read after the primary release callback");
    first.replaceSecondary = 0;
}

void ownedPayloadCases()
{
    CollisionMeshTemplate* mesh = new CollisionMeshTemplate;
    mesh->m_lods.resize(2);
    emptyGeom(mesh->m_lods[0], 3);
    emptyGeom(mesh->m_lods[1], 0);
    CollisionLod* lod = mesh->m_lods[0].lods[0] = new CollisionLod;
    // The raw slot at 2 has no remap, but still belongs to template deletion.
    mesh->m_lods[0].lods[2] = new CollisionLod;
    mesh->m_lods[0].colRemap[0] = mesh->m_lods[0].colRemap[1] = 0;
    lod->faces.resize(2); lod->vertices.resize(4); lod->materials.resize(4);
    lod->edgeIndices.reserve(12); // Capacity with no elements must also be released.
    lod->faceAdjacency = new unsigned short[6];
    lod->bsp.bspV8 = new BspV8;
    lod->bsp.bspV8->m_faces.resize(2);
    lod->bsp.bspV8->m_nodes.resize(1);
    lod->bsp.bspV8->m_faceIndices.resize(2);
    lod->bsp.bspV8->m_vertices = &lod->vertices; // Borrowed, not separately deleted.
    lod->bsp.m_pool34 = new char[12]; lod->bsp.m_pool38 = new char[16];
    mesh->m_materialIndex = new unsigned short[5];
    mesh->m_name = "a long owned collision mesh name";
    mesh->m_directory = "a long owned collision mesh directory/";
    GeometryProbe first(lod, 0, 11), second(lod, 1, 12), third(lod, 2, 13);
    lod->debugMesh = &first; lod->dwordB0 = &second; lod->dwordB4 = &third;
    releaseCount = 0;
    beginWatch(); trackTemplate(mesh);
    delete mesh;
    check(releaseCount == 3, "template destructor invokes debug cleanup before deleting raw LODs");
    verifyWatch("template releases all raw LODs, nested arrays/vectors/Bsp, material map and strings in order");

    Bsp* bsp = new Bsp;
    bsp->bspNode.faces = new BspNode::Face*[2];
    bsp->bspNode.childNode1 = new BspNode;
    bsp->bspNode.childNode1->faces = new BspNode::Face*[1];
    bsp->bspNode.childNode2 = new BspNode;
    bsp->m_pool38 = new char[8]; bsp->m_pool34 = new char[8];
    beginWatch();
    track(bsp->bspNode.faces, true);
    track(bsp->bspNode.childNode1->faces, true); track(bsp->bspNode.childNode1);
    track(bsp->bspNode.childNode2);
    track(bsp->m_pool38, true); track(bsp->m_pool34, true); track(bsp);
    delete bsp;
    verifyWatch("Bsp destructor reaches legacy child teardown and both pool releases in retail order");

    Bsp gated;
    gated.m_flag0 = 1;
    BspNode* borrowed = gated.bspNode.childNode1 = new BspNode;
    gated.bspV8 = new BspV8;
    BspV8* oldTree = gated.bspV8;
    gated.m_totalFaceRefs = 7;
    gated.destroy();
    check(!gated.bspNode.childNode1 && gated.bspV8 == oldTree && gated.m_totalFaceRefs == 7,
          "destroy preserves retail legacy skip, dangling V8 pointer and unchanged reference counter");
    // [TEMP] Caller cleanup after inspecting retail's deliberately non-idempotent destroy.
    gated.bspV8 = 0; delete borrowed;
}

void stockCases(const char* path)
{
    FILE* file = std::fopen(path, "rb");
    if (!file) { check(false, "stock mesh fixture file opens"); return; }
    std::fseek(file, 0, SEEK_END); const long length = std::ftell(file); std::rewind(file);
    std::vector<unsigned char> bytes(static_cast<std::size_t>(length));
    const bool read = std::fread(bytes.data(), 1, bytes.size(), file) == bytes.size();
    std::fclose(file);
    check(read && bytes.size() >= 12, "stock mesh bytes read");
    if (!read || bytes.size() < 12) return;
    for (int pass = 0; pass < 3; ++pass)
    {
        io::MemStream stream;
        stream.setSize(static_cast<unsigned int>(bytes.size()));
        stream.write(bytes.data(), static_cast<unsigned int>(bytes.size()));
        stream.seek(io::SEEKORIGIN_BEGIN, 0);
        unsigned int major, minor, parts;
        stream.read(&major, 4); stream.read(&minor, 4); stream.read(&parts, 4);
        check(parts > 0 && parts < 32, "stock header has bounded fixture part count");
        if (!parts || parts >= 32) return;
        CollisionManager manager;
        std::vector<CollisionMeshTemplate*>& templates = manager.m_collisionMeshTemplates["StockFixture"];
        bool parsed = true;
        unsigned int vertexCount = 0, lodCount = 0, treeCount = 0;
        for (unsigned int part = 0; part < parts; ++part)
        {
            CollisionMeshTemplate* mesh = new CollisionMeshTemplate;
            templates.push_back(mesh); mesh->setVersionToLoad(major, minor);
            parsed = mesh->load(&stream, 0, true) && mesh->m_loaded && parsed;
            for (CollisionGeom& geom : mesh->m_lods)
                for (CollisionLod* lod : geom.lods)
                    if (lod) { ++lodCount; vertexCount += static_cast<unsigned int>(lod->vertices.size());
                               if (lod->bsp.bspV8) ++treeCount; }
        }
        check(parsed && lodCount && vertexCount && treeCount, "real loader populates stock LODs, vertices and BspV8 trees");
        check(stream.getPosition() == stream.getSize(), "stock mesh parser consumes the whole file");
        std::printf("stock pass %d: version %u/%u, %u parts, %u LODs, %u vertices, %u trees\n",
                    pass + 1, major, minor, parts, lodCount, vertexCount, treeCount);
        beginWatch();
        for (CollisionMeshTemplate* mesh : templates) trackTemplate(mesh);
        manager.deleteCollisionMesh("stockfixture");
        verifyWatch("real manager removal releases every watched stock payload once and in order");
        check(manager.m_collisionMeshTemplates.empty(), "stock entry removed before next fresh load");
    }
}
} // namespace

// [TEMP] The fixture uses the same CRT heap for new/delete while observing exact release form.
// No production allocation code is replaced in the source tree.
void* operator new(std::size_t count) { return allocate(count); }
void* operator new[](std::size_t count) { return allocate(count); }
void operator delete(void* p) noexcept { observeFree(p, false); std::free(p); }
void operator delete[](void* p) noexcept { observeFree(p, true); std::free(p); }
void operator delete(void* p, std::size_t) noexcept { observeFree(p, false); std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { observeFree(p, true); std::free(p); }

int main(int argc, char** argv)
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    g_system = new System; settings = new SettingsRepostitory;
    eventManager = new Events;
    debugCases(); ownedPayloadCases();
    if (argc == 2) stockCases(argv[1]); else check(false, "stock fixture path supplied");
    std::printf("%d checks, %d failures; %d payload allocations observed\n", checks, failures, totalObserved);
    return failures ? 1 : 0;
}
