// [TEMP] [CHOSEN, not recovered] Actual BufferManager source and VC7 set shim, with fake COM
// resources. Exercises collection and inline wrapper release, not device reset or real D3D.
#include "../BF2/RendDX9/BufferManager.cpp"
#include <cstdio>
#include <vector>
#include <map>

using namespace dice::hfe;
using namespace dice::hfe::dx9rend;
namespace {
int checks=0, failures=0;
#define CHECK(e) do { ++checks; if(!(e)) { ++failures; std::printf("FAIL %d: %s\n",__LINE__,#e); } } while(0)
struct Resource {
    void** table;
    void* wrapper;
    unsigned int list;
    int id;
    Resource* change;
    bool freed;
};
BufferManager* manager=0;
std::vector<Resource*> resources;
std::map<void*,Resource*> byWrapper;
std::vector<int> releases, wrapperFrees;
std::vector<void*> backingBlocks;
bool collecting=false;
int scriptedReads=0;
void* comTable[3]={};

int& refs(void* wrapper) { return static_cast<int*>(wrapper)[1]; }
bool contains(unsigned int list,void* wrapper)
{
    vc71::set_layout& values=pointerSet(manager,list);
    return values.find(reinterpret_cast<unsigned int>(wrapper))!=values.end();
}
ULONG __stdcall resourceRelease(void* raw)
{
    Resource* resource=static_cast<Resource*>(raw);
    CHECK(refs(resource->wrapper)==0);
    CHECK(contains(resource->list,resource->wrapper)); // Release must precede erase.
    CHECK(!resource->freed);
    releases.push_back(resource->id);
    if(resource->change) refs(resource->change->wrapper)=1;
    return 0;
}
Resource* makeResource(void* storage,unsigned int list,int id,int references)
{
    Resource* resource=new Resource{comTable,storage,list,id,0,false};
    resources.push_back(resource); byWrapper[storage]=resource;
    if(list<2) {
        VertexBuffer* wrapper=new(storage) VertexBuffer{};
        wrapper->vptr=const_cast<void**>(kVertexBufferVtable); wrapper->refCount=references;
        wrapper->buffer=reinterpret_cast<IDirect3DVertexBuffer9*>(resource);
    } else {
        IndexBuffer* wrapper=new(storage) IndexBuffer{};
        wrapper->vptr=const_cast<void**>(kVertexBufferVtable); wrapper->refCount=references;
        wrapper->buffer=reinterpret_cast<IDirect3DIndexBuffer9*>(resource);
    }
    CHECK(pointerSet(manager,list).insert(reinterpret_cast<unsigned int>(storage))!=0);
    return resource;
}
Resource* addResource(unsigned int list,int id,int references)
{
    void* storage=std::malloc(list<2?sizeof(VertexBuffer):sizeof(IndexBuffer));
    backingBlocks.push_back(storage);
    return makeResource(storage,list,id,references);
}
int __fastcall scriptedGetRef(void* wrapper,void*)
{
    // Model a reference disappearing between default-list and managed-list visits. Both
    // observations still use the actual getter, whose exact int return is tested separately.
    if(++scriptedReads==2) refs(wrapper)=1;
    return vb_getRef(wrapper,0);
}
void cleanupResources()
{
    collecting=false;
    for(unsigned int list=0;list<4;++list) pointerSet(manager,list).clear();
    for(Resource* resource:resources) delete resource;
    resources.clear(); byWrapper.clear();
    for(void* block:backingBlocks) std::free(block);
    backingBlocks.clear(); releases.clear(); wrapperFrees.clear();
}
}

namespace renddx9abi {
void* engineAlloc(unsigned int size) { return std::malloc(size); }
void engineFree(void* p)
{
    auto found=byWrapper.find(p);
    if(found!=byWrapper.end()) {
        Resource* resource=found->second;
        CHECK(collecting && !resource->freed);
        CHECK(contains(resource->list,p)); // engineFree also precedes tree erase.
        CHECK(!releases.empty() && releases.back()==resource->id);
        resource->freed=true; wrapperFrees.push_back(resource->id);
        // Retain test storage until cleanup so any stale-set dereference is deterministic.
    } else std::free(p);
}
void* crtMalloc(size_t size) { return std::malloc(size); }
void crtFree(void* p) { std::free(p); }
void logf(const char*,...) {}
unsigned int addr(const void* p) { return reinterpret_cast<unsigned int>(p); }
void frontier(const char*,const void*,unsigned int) { std::abort(); }
void trace(const char*,const void*,unsigned int) { std::abort(); }
}
namespace dice { namespace hfe { namespace dx9rend {
IBase* g_Renderer=0;
// Unreached bodies retained by the complete production TU must fail if accidentally called.
int bitsPerPixel(unsigned int) { std::abort(); }
void registerDeviceLostResetListener(DeviceLostResetListener*,int) { std::abort(); }
void stateManagerSetStreamSource(unsigned int,IDirect3DVertexBuffer9*,unsigned int,unsigned int) { std::abort(); }
void stateManagerSetIndices(IDirect3DIndexBuffer9*) { std::abort(); }
void stateManagerSetVertexDeclaration(IDirect3DVertexDeclaration9*) { std::abort(); }
} } }

int main()
{
    comTable[2]=reinterpret_cast<void*>(&resourceRelease);
    BufferManager actual(0); manager=&actual; g_bufferManager=manager;
    unsigned char renderer[320]={}; g_Renderer=reinterpret_cast<IBase*>(renderer);
    int* counters=reinterpret_cast<int*>(renderer+264); counters[7]=10; counters[9]=20; counters[10]=30;
    VertexBuffer getterProbe={}; getterProbe.vptr=const_cast<void**>(kVertexBufferVtable);
    typedef int(__thiscall* Getter)(void*);
    const Getter getter=reinterpret_cast<Getter>((*reinterpret_cast<void***>(&getterProbe))[0]);
    const int refValues[]={0,1,2,-1,0x12345678};
    for(int value:refValues) { getterProbe.refCount=value; CHECK(getter(&getterProbe)==value); }

    // Guaranteed address order: the first entry is kept initially, the second release changes
    // the first to ref==1. Only restarting from begin collects it in the same sweep.
    unsigned char* pair=static_cast<unsigned char*>(std::malloc(2*sizeof(VertexBuffer)));
    backingBlocks.push_back(pair);
    Resource* first=makeResource(pair,1,10,2);
    Resource* second=makeResource(pair+sizeof(VertexBuffer),1,11,1); second->change=first;
    Resource* keep=addResource(1,12,3);
    Resource* zero=addResource(1,13,0);
    Resource* negative=addResource(1,14,-1);
    addResource(0,20,1); Resource* managedKeep=addResource(0,21,4);
    addResource(3,30,1); addResource(2,40,1); Resource* indexKeep=addResource(2,41,2);
    collecting=true; bufferManagerCollectUnused();
    const std::vector<int> expected={11,10,20,30,40};
    CHECK(releases==expected && wrapperFrees==expected);
    CHECK(pointerSet(manager,1).size==3 && pointerSet(manager,0).size==1);
    CHECK(pointerSet(manager,3).size==0 && pointerSet(manager,2).size==1);
    CHECK(contains(1,keep->wrapper) && refs(keep->wrapper)==3);
    CHECK(contains(1,zero->wrapper) && refs(zero->wrapper)==0);
    CHECK(contains(1,negative->wrapper) && refs(negative->wrapper)==-1);
    CHECK(contains(0,managedKeep->wrapper) && refs(managedKeep->wrapper)==4);
    CHECK(contains(2,indexKeep->wrapper) && refs(indexKeep->wrapper)==2);
    CHECK(counters[7]==7 && counters[9]==18 && counters[10]==28);
    bufferManagerCollectUnused(); CHECK(releases==expected);
    cleanupResources();

    // Both helpers search default before managed. This deliberately inconsistent duplicate
    // registration records that shipped behavior without dereferencing actually freed memory.
    for(unsigned int kind=0;kind<2;++kind) {
        const unsigned int defaults=kind?3u:1u, managed=defaults-1u;
        Resource* duplicate=addResource(defaults,50+kind,2);
        CHECK(pointerSet(manager,managed).insert(reinterpret_cast<unsigned int>(duplicate->wrapper))!=0);
        void* scriptedTable[3]={reinterpret_cast<void*>(&scriptedGetRef),0,0};
        *reinterpret_cast<void***>(duplicate->wrapper)=scriptedTable;
        scriptedReads=0; collecting=true; bufferManagerCollectUnused();
        CHECK(duplicate->freed && pointerSet(manager,defaults).size==0);
        CHECK(contains(managed,duplicate->wrapper) && refs(duplicate->wrapper)==0);
        CHECK(releases.size()==1 && releases[0]==50+static_cast<int>(kind));
        cleanupResources();
    }
    // Constructor sentinels are test-owned; BufferManager has no recovered deleting destructor.
    for(unsigned int i=0;i<7;++i) { std::free(actual.list(i).head); actual.list(i).head=0; }
    std::printf("BufferManager collect: %d/%d checks passed\n",checks-failures,checks);
    return failures?1:0;
}
