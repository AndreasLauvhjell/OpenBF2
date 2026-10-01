// [TEMP] Actual-source MemoryPool/Malloc native-port fixture. No production server boot.
#include "Core/MemoryPool.h"
#include "Core/Malloc.h"
#include "Core/Ids.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <vector>

// [TEMP] Link-only collaborator for the unexercised createMemoryPool factory.
// This fixture constructs actual MemoryPools directly and never calls the factory.
namespace dice { namespace hfe { class ClassManager; ClassManager* classManager = 0; } }

using namespace dice::hfe;
// Keep the deleted object's storage alive and overwrite it. This distinguishes
// the retail saved reference-count return from reading the member after delete.
struct PoisonedPool : MemoryPool
{
    static void* quarantined;
    static int destructed, deallocated;
    ~PoisonedPool() override { ++destructed; }
    static void operator delete(void* storage) noexcept
    {
        ++deallocated;
        std::memset(storage, 0xA5, sizeof(PoisonedPool));
        quarantined = storage;
    }
};
void* PoisonedPool::quarantined = 0;
int PoisonedPool::destructed = 0;
int PoisonedPool::deallocated = 0;

static int checks;
static void check(bool condition, const char* message)
{
    ++checks;
    if (!condition) { std::fprintf(stderr, "FAILED: %s\n", message); std::exit(1); }
}

int main()
{
#ifndef _WIN64
    static_assert(sizeof(MemoryPool) == 48, "Retail x86 MemoryPool layout changed");
#endif
    const int geometries[] = {1, 3, 4, 5, 8, 12, 16};
    const int width = static_cast<int>(sizeof(void*));
    bool highAddress = false;
    for (int requested : geometries)
    {
        const int stride = ((requested + width - 1) / width) * width;
        const int capacity = 128 / stride;
        MemoryPool* pool = new MemoryPool;
        check(pool->getRef() == 1, "default constructor reference");
        pool->init(requested, 128, false);
        check(pool->queryInterface(IID_IMemoryPool) == pool, "typed pool interface");
        check(pool->queryInterface(0) == 0, "unknown interface");
        std::vector<void*> blocks;
        std::set<void*> unique;
        for (int i = 0; i < capacity * 2 + 3; ++i)
        {
            // The size argument is intentionally ignored by retail's fixed-slot path.
            void* block = pool->alloc(0, "fixture", 123);
            const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(block);
            highAddress = highAddress || address > UINT32_MAX;
            check(address % sizeof(void*) == 0, "native free-list alignment");
            check(unique.insert(block).second, "overflow chain returns unique live slots");
            if (i > 0 && i % capacity != 0)
                check(static_cast<char*>(block) - static_cast<char*>(blocks.back()) == stride,
                      "native slot stride");
            std::memset(block, 0x40 + i % 32, static_cast<std::size_t>(requested));
            blocks.push_back(block);
        }
        for (std::size_t i = 0; i < blocks.size(); ++i)
        {
            const unsigned char* bytes = static_cast<const unsigned char*>(blocks[i]);
            bool intact = true;
            for (int n = 0; n < requested; ++n)
                intact = intact && bytes[n] == static_cast<unsigned char>(0x40 + i % 32);
            check(intact, "adjacent allocated payloads survive");
        }
        std::set<void*> freed;
        for (std::size_t i = 0; i < blocks.size(); i += 2)
        {
            freed.insert(blocks[i]);
            pool->free(blocks[i], "fixture", 456);
        }
        const std::size_t freedCount = freed.size();
        for (std::size_t i = 0; i < freedCount; ++i)
            check(freed.erase(pool->alloc(1)) == 1, "freed slots reused across overflow pools");
        void* foreign = Malloc::malloc(48);
        check(foreign != 0, "shared CRT allocation");
        check(!pool->tryFree(foreign), "foreign pointer is outside all pools");
        pool->free(foreign); // The real fallback releases through Malloc::free.
        pool->addRef();
        check(pool->getRef() == 2 && pool->release() == 1, "reference increment and release");
        check(pool->release() == 0, "destroy complete overflow chain on last release");
    }
#ifdef _WIN64
    check(highAddress, "exercise real heap pointers above 4 GiB");
#endif
    Malloc::free(0);
    PoisonedPool* poisoned = new PoisonedPool;
    check(poisoned->release() == 0, "last release returns saved count after destructor frees storage");
    check(PoisonedPool::destructed == 1 && PoisonedPool::deallocated == 1,
          "one virtual destruction and deallocation");
    bool poisonedBytes = true;
    for (std::size_t i = 0; i < sizeof(PoisonedPool); ++i)
        poisonedBytes = poisonedBytes && static_cast<unsigned char*>(PoisonedPool::quarantined)[i] == 0xA5;
    check(poisonedBytes, "quarantined storage was overwritten");
    ::operator delete(PoisonedPool::quarantined);
    std::printf("MemoryPool: %d checks passed (%u-bit), 7 geometries, real overflow chains\n",
                checks, static_cast<unsigned int>(sizeof(void*) * 8));
    return 0;
}
