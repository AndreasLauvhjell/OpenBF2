// [TEMP] [CHOSEN, not recovered] Sentinel fixture for the actual BotInfo constructor.
// PE oracle: AIDLL_w32ded.dll 0x1018A590 and inlined 0x1018FB57..0x1018FBB8.
// Tests native MSVC x86 layout and every byte, including intentionally untouched fields/padding.
// This neither constructs a BotManager nor drives AI or a server.
#include "../../BF2/AI/AIDLL/AI/Main/BotManager.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <new>

typedef dice::hfe::ai::BotManager::BotInfo BotInfo;
typedef dice::hfe::ai::Bot Bot;

static_assert(sizeof(void*) == 4, "This fixture is the native x86 PE layout oracle");
static_assert(sizeof(BotInfo) == 96, "PE BotInfo allocation is 96 bytes");
static_assert(alignof(BotInfo) == 8, "PE BotInfo uses native double alignment");
static_assert(offsetof(BotInfo, m_bot) == 0, "bot pointer");
static_assert(offsetof(BotInfo, m_flag20) == 20, "leading flag");
static_assert(offsetof(BotInfo, m_double24) == 24, "first double");
static_assert(offsetof(BotInfo, m_behaviourUpdateCount) == 32, "uninitialized counter");
static_assert(offsetof(BotInfo, m_word36) == 36, "counter after untouched field");
static_assert(offsetof(BotInfo, m_double48) == 48, "timing fields");
static_assert(offsetof(BotInfo, m_flag72) == 72, "first trailing flag");
static_assert(offsetof(BotInfo, m_flag73) == 73, "second trailing flag");
static_assert(offsetof(BotInfo, m_double76) == 80, "native PE alignment differs from ELF +76");
static_assert(offsetof(BotInfo, m_damage) == 88, "native PE tail differs from ELF +84");
static_assert(offsetof(BotInfo, m_attackerIndex) == 92, "native PE untouched tail differs from ELF +88");

static int run(unsigned char sentinel, Bot* bot)
{
    alignas(BotInfo) unsigned char actual[sizeof(BotInfo)];
    unsigned char expected[sizeof(BotInfo)];
    std::memset(actual, sentinel, sizeof(actual));
    std::memset(expected, sentinel, sizeof(expected));

    // Independent raw-byte oracle from the PE stores, not from the C++ member names.
    std::memcpy(expected, &bot, sizeof(bot));
    std::memset(expected + 4, 0, 17);   // DWORDs +4,+8,+12,+16 and byte +20.
    std::memset(expected + 24, 0, 8);   // double +24; +32 stays untouched.
    std::memset(expected + 36, 0, 36);  // DWORDs +36,+40,+44 and doubles +48,+56,+64.
    expected[72] = 1;
    expected[73] = 0;
    std::memset(expected + 80, 0, 12);  // double +80, DWORD +88; +92 stays untouched.

    BotInfo* const info = new (actual) BotInfo(bot);
    int failures = 0;
    for (std::size_t i = 0; i < sizeof(actual); ++i)
    {
        if (actual[i] != expected[i])
        {
            std::fprintf(stderr, "BotInfo byte %u: got %02X expected %02X (sentinel %02X)\n",
                         static_cast<unsigned>(i), actual[i], expected[i], sentinel);
            ++failures;
        }
    }
    info->~BotInfo();
    return failures;
}

int main()
{
    // Pointer identity is the only bot interaction. Neither value is dereferenced.
    const int failures = run(0xA5, reinterpret_cast<Bot*>(0x12345678u)) + run(0x5A, nullptr);
    if (failures)
        return 1;
    std::puts("BotInfo actual-source PASS: 192 byte checks, native x86 size96, untouched fields and padding preserved");
    return 0;
}
