// [TEMP] [CHOSEN, not recovered] isolated architecture-comparison fixture.
// Links the actual recovered BitStream and four event serializer TUs, never pasted codec bodies.
// These generated goldens compare rebuilt x86 against native x64; they are NOT retail captures.
#include "../BF2/IO/BitStream.h"
#include "../BF2/Core/Debug.h"
#include "../BF2/Core/EventManager.h"
#include "../BF2/Game/IGame.h"
#include "../BF2/Game/IGameLogic.h"
#include "../BF2/Game/GameEvents/BeginRoundEvent.h"
#include "../BF2/Game/GameEvents/DestroyObjectEvent.h"
#include "../BF2/Game/GameEvents/EnterVehicleEvent.h"
#include "../BF2/Game/GameEvents/ExitVehicleEvent.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

namespace dice { namespace hfe {
// Only serializers are exercised. These unpopulated execution dependencies must never be used.
IGame* game = nullptr;
IGameLogic* gameLogic = nullptr;
EventManager* eventManager = nullptr;
namespace world { class PlayerManager; PlayerManager* playerManager = nullptr; }
// The sole callable test collaborator is fail-loud, including its first executable statement.
Debug::Debug(DebugType, const std::string& file, int line, const std::string&)
{
    std::fprintf(stderr, "[FRONTIER] wire fixture dice::hfe::Debug reached at %s:%d\n", file.c_str(), line);
    std::fflush(stderr);
    std::abort();
}
Debug::~Debug()
{
    std::fprintf(stderr, "[FRONTIER] wire fixture dice::hfe::Debug::~Debug reached\n");
    std::fflush(stderr);
    std::abort();
}
} }

using namespace dice::hfe;
using dice::hfe::io::BitStream;
static_assert(sizeof(std::uint32_t) == 4 && sizeof(float) == 4, "fixed-width golden fields");

namespace {
FILE* wireFile;
FILE* decodedFile;
unsigned records;
std::uint32_t randomState = 0x42bfa19du;

void require(bool ok, const char* context)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL %s (record %u)\n", context, records);
        std::exit(1);
    }
}

std::uint32_t nextRandom()
{
    randomState = randomState * 1664525u + 1013904223u;
    return randomState;
}

void word(FILE* file, std::uint32_t value)
{
    // Explicit little endian, with no native object/container layout in the artifact.
    unsigned char bytes[4];
    for (unsigned i = 0; i != 4; ++i) bytes[i] = static_cast<unsigned char>(value >> (i * 8));
    require(std::fwrite(bytes, 1, 4, file) == 4, "write golden word");
}

void record(unsigned category, const BitStream& stream)
{
    require(!stream.hasError(), "encoder error");
    word(wireFile, category);
    word(wireFile, stream.getWriteBitPosition());
    const unsigned bytes = (stream.getWriteBitPosition() + 7) / 8;
    require(std::fwrite(stream.getBuffer(), 1, bytes, wireFile) == bytes, "write golden payload");
    ++records;
}

void decoded(float value)
{
    require(std::isfinite(value), "finite decoded float");
    std::uint32_t bits;
    std::memcpy(&bits, &value, 4);
    word(decodedFile, bits);
}

void checkReadEnd(BitStream& stream)
{
    require(!stream.hasError(), "decoder error");
    require(stream.getBitPosition() == stream.getWriteBitPosition(), "decoder consumed complete payload");
}

void bits()
{
    const unsigned widths[] = {1,2,3,7,8,9,15,16,17,31,32,33,63,64,65,127,128,129,255,256};
    for (unsigned offset = 0; offset != 8; ++offset)
        for (unsigned width : widths)
            for (unsigned trial = 0; trial != 16; ++trial) {
                unsigned char storage[80], expected[80], input[40], output[40] = {};
                std::memset(storage, 0xa5, sizeof(storage));
                std::memcpy(expected, storage, sizeof(storage));
                for (unsigned i = 0; i != sizeof(input); ++i) input[i] = static_cast<unsigned char>(nextRandom());
                BitStream stream(storage, sizeof(storage) - 2); // guard bytes for the retail byte lookahead
                const unsigned prefix = nextRandom();
                if (offset) require(stream.writeBits(&prefix, offset), "prefix write");
                require(stream.writeBits(input, width), "bit-field write");
                // Independent bit-by-bit oracle also proves surrounding bits were preserved.
                for (unsigned bit = 0; bit < offset + width; ++bit) {
                    const unsigned value = bit < offset ? ((prefix >> bit) & 1u) :
                        ((input[(bit - offset) / 8] >> ((bit - offset) % 8)) & 1u);
                    expected[bit / 8] = static_cast<unsigned char>((expected[bit / 8] & ~(1u << (bit % 8))) | (value << (bit % 8)));
                }
                require(std::memcmp(storage, expected, sizeof(storage)) == 0, "independent bit oracle");
                record(1, stream);
                stream.setReadMode();
                unsigned prefixRead = 0;
                if (offset) require(stream.readBits(&prefixRead, offset), "prefix read");
                require(prefixRead == (prefix & ((1u << offset) - 1u)), "prefix match");
                require(stream.readBits(output, width), "bit-field read");
                for (unsigned bit = 0; bit < width; ++bit)
                    require(((output[bit / 8] ^ input[bit / 8]) & (1u << (bit % 8))) == 0, "bit roundtrip");
                checkReadEnd(stream);
            }
    unsigned char storage[8] = {};
    BitStream small(storage, 1);
    unsigned value = 0xa5;
    require(!small.writeBits(&value, 0) && !small.hasError(), "zero-width write contract");
    require(small.writeBits(&value, 8), "exact capacity write");
    require(!small.writeBits(&value, 1) && small.hasError(), "write overflow latch");
    small.clearError();
    small.setReadMode();
    require(small.readBits(&value, 8), "exact capacity read");
    require(!small.readBits(&value, 1) && small.hasError(), "read overflow latch");
}

void vectors()
{
    const float limits[] = {0.0f, 0.00000001f, 0.125f, 1.0f, 127.999f, 128.0f, 511.999f,
        512.0f, 2047.999f, 2048.0f, 32767.99f, 32768.0f, 524287.0f, 524288.0f, 8388607.0f, 8388608.0f};
    const float scales[] = {1.0f, 0.01f, 1000.0f};
    for (unsigned enabled = 0; enabled != 2; ++enabled) {
        BitStream::m_relativeCompressionEnabled = enabled != 0;
        for (unsigned codec = 0; codec != 3; ++codec)
            for (float scale : scales)
                for (float limit : limits)
                    for (unsigned useReference = 0; useReference != 2; ++useReference) {
                        const Vec3 ref = useReference ? Vec3{123.5f, -45.25f, 7.0f} : Vec3{0,0,0};
                        const Vec3 value = {ref.x + limit, ref.y - limit * 0.5f, ref.z + limit * 0.25f};
                        unsigned char storage[80] = {};
                        BitStream stream(storage, sizeof(storage) - 2);
                        if (codec == 0) stream.writeCompressedVector(value, ref, scale);
                        if (codec == 1) stream.writeHighCompressedVector(value, ref, scale);
                        if (codec == 2) stream.writeCompressedVector2(value, ref, scale);
                        record(10 + codec, stream);
                        stream.setReadMode();
                        Vec3 result;
                        if (codec == 0) result = stream.readCompressedVector(ref, scale);
                        if (codec == 1) result = stream.readHighCompressedVector(ref, scale);
                        if (codec == 2) result = stream.readCompressedVector2(ref, scale);
                        checkReadEnd(stream);
                        decoded(result.x); decoded(result.y); decoded(result.z);
                        const float tolerance = scale * 1.1f + 2.0f;
                        require(std::fabs(result.x - value.x) <= tolerance && std::fabs(result.y - value.y) <= tolerance &&
                            std::fabs(result.z - value.z) <= tolerance, "position quantization error");
                    }
    }
    BitStream::m_relativeCompressionEnabled = true;
}

void rotations()
{
    const int widths[] = {4, 8, 10, 12, 16, 20, 24, 31, 32};
    const Vec3 normals[] = {{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1},
        {0.6f,0.8f,0},{-0.6f,0,0.8f},{0.33333334f,0.66666669f,0.66666669f},{0,0,1.000001f}};
    const Quat quaternions[] = {{0,0,0,1},{0,0,0,-1},{0.5f,0.5f,0.5f,0.5f},
        {-0.5f,0.5f,-0.5f,-0.5f},{0.6f,0.8f,0,0},{2,-3,4,-5},{0,0,0,0},
        {0.00001f,-0.00001f,0.00001f,1},{1,0,0,0}};
    for (int width : widths) {
        for (const Vec3& normal : normals) {
            unsigned char storage[80] = {};
            BitStream stream(storage, sizeof(storage) - 2);
            stream.writeNormalVector(normal, width);
            record(20, stream);
            stream.setReadMode();
            const Vec3 result = stream.readNormalVector(width);
            checkReadEnd(stream);
            decoded(result.x); decoded(result.y); decoded(result.z);
        }
        for (const Quat& quaternion : quaternions)
            for (unsigned three = 0; three != 2; ++three) {
                unsigned char storage[80] = {};
                BitStream stream(storage, sizeof(storage) - 2);
                if (three) stream.writeUnitQuaternion3Comp(quaternion, width);
                else stream.writeUnitQuaternion(quaternion, width);
                record(21 + three, stream);
                stream.setReadMode();
                const Quat result = three ? stream.readUnitQuaternion3Comp(width) : stream.readUnitQuaternion(width);
                checkReadEnd(stream);
                decoded(result.x); decoded(result.y); decoded(result.z); decoded(result.w);
            }
    }
}

void normalDecodeBoundaries()
{
    // Decoder-only payloads cover every quantized width, the dead zone, extremes,
    // and deterministic random azimuth/elevation pairs independently of the encoder.
    for (unsigned width = 1; width != 32; ++width) {
        const std::uint32_t maximum = (1u << width) - 1u;
        const std::uint32_t middle = maximum / 2;
        const std::uint32_t edges[] = {0, 1, maximum, maximum - 1,
            middle, middle + 1, middle ? middle - 1 : 0,
            maximum > 2 ? maximum - 2 : 0, maximum / 4};
        const unsigned exhaustive = width <= 6 ? (1u << (width * 2)) : 0;
        for (unsigned trial = 0; trial != 81 + 512 + exhaustive; ++trial) {
            // Take the high PRNG bits: low LCG bits repeat rapidly at narrow widths.
            const std::uint32_t azimuth = trial < 81 ? edges[trial / 9] : trial < 593 ?
                nextRandom() >> (32 - width) : (trial - 593) & maximum;
            const std::uint32_t elevation = trial < 81 ? edges[trial % 9] : trial < 593 ?
                nextRandom() >> (32 - width) : (trial - 593) >> width;
            unsigned char storage[16] = {};
            BitStream stream(storage, sizeof(storage) - 2);
            stream.writeBits(&azimuth, width);
            stream.writeBits(&elevation, width);
            record(23, stream);
            stream.setReadMode();
            const Vec3 result = stream.readNormalVector(width);
            checkReadEnd(stream);
            decoded(result.x); decoded(result.y); decoded(result.z);
        }
    }
    // Raw 32-bit normalized-angle fields: signed zero, values around the snap
    // threshold (2 / UINT_MAX rounds to 2^-31), unit limits, and interior values.
    const std::uint32_t floatBits[] = {0,0x80000000u,0x2fffffffu,0x30000000u,0x30000001u,
        0xafffffffu,0xb0000000u,0xb0000001u,0x3f7fffffu,0x3f800000u,0xbf7fffffu,0xbf800000u,
        0x3f000000u,0xbf000000u,0x3eaaaaabu,0xbeaaaaabu,0x00000001u,0x80000001u};
    for (std::uint32_t azimuth : floatBits)
        for (std::uint32_t elevation : floatBits) {
            unsigned char storage[16] = {};
            BitStream stream(storage, sizeof(storage) - 2);
            stream.writeBits(&azimuth, 32);
            stream.writeBits(&elevation, 32);
            record(23, stream);
            stream.setReadMode();
            const Vec3 result = stream.readNormalVector(32);
            checkReadEnd(stream);
            decoded(result.x); decoded(result.y); decoded(result.z);
        }
}

struct RoundInput : BeginRoundEvent {
    void set(std::uint32_t state, std::uint32_t tick) { m_beginRoundState = static_cast<int>(state); m_nextStateChangeAtTick = tick; }
};
struct WidthOnlyMaker : IGameEventMaker {
    GameEvent* createEvent() override { require(false, "width-only registry maker was executed"); std::abort(); }
};

template<class T> void eventRoundtrip(T& source)
{
    unsigned char encoded[128] = {}, reencoded[128] = {};
    BitStream stream(encoded, sizeof(encoded) - 2);
    require(source.serialize(&stream), "event serialize");
    record(100 + source.getType(), stream);
    stream.setReadMode();
    unsigned type = 0;
    require(stream.readBits(&type, gameEventTypeIdBitCount()) && type == static_cast<unsigned>(source.getType()), "event type prefix");
    T destination;
    require(destination.deSerialize(&stream), "event deserialize");
    checkReadEnd(stream);
    BitStream again(reencoded, sizeof(reencoded) - 2);
    require(destination.serialize(&again), "event reserialize");
    require(again.getWriteBitPosition() == stream.getWriteBitPosition() &&
        std::memcmp(encoded, reencoded, (stream.getWriteBitPosition() + 7) / 8) == 0, "event byte roundtrip");
}

void events()
{
    // Model only the registry's stock maximum-ID input; registration and bit counting are real.
    WidthOnlyMaker maker;
    require(gameEventRegistry->registerEventMaker(69, "wire-fixture-width-only", &maker), "register width fixture");
    require(gameEventTypeIdBitCount() == 7, "stock seven-bit event prefix");
    const std::uint32_t values[] = {0,1,127,128,255,256,32767,32768,65535,65536,0x7fffffff,0x80000000,0xffffffff};
    for (std::uint32_t value : values) {
        RoundInput round; round.set(value, ~value); eventRoundtrip(round);
        DestroyObjectEvent destroy(static_cast<unsigned short>(value)); eventRoundtrip(destroy);
        EnterVehicleEvent enter(static_cast<int>(value), static_cast<unsigned short>(~value), (value & 1) != 0); eventRoundtrip(enter);
        ExitVehicleEvent exit(static_cast<int>(value), (value & 1) == 0); eventRoundtrip(exit);
    }
}
}

int main(int argc, char** argv)
{
    require(argc == 3, "usage: w64_wire_golden wire.bin decoded.bin");
    wireFile = std::fopen(argv[1], "wb");
    decodedFile = std::fopen(argv[2], "wb");
    require(wireFile && decodedFile, "open golden files");
    bits(); vectors(); rotations(); events(); normalDecodeBoundaries();
    require(std::fclose(wireFile) == 0 && std::fclose(decodedFile) == 0, "close golden files");
    std::printf("PASS %u deterministic records; real BitStream + four event serializers; rebuilt comparison, not retail capture\n", records);
    return 0;
}
