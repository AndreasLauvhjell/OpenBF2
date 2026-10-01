// Test scaffolding: round-trips random bit fields across every offset and width.
// Uses copied BitStream bodies with minimal stand-in types; excluded from engine builds.

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>

struct Vec3 { float x, y, z; };
struct Quat { float x, y, z, w; };   // w last — proven by the decoder's identity pre-fill

// The width table, values read from .data at 0x898CB64.
static unsigned int m_compressionVectorBitTable[4]     = { 32, 20, 16, 12 };
static unsigned int m_highCompressionVectorBitTable[4] = { 32, 12, 10,  8 };
static unsigned int m_compressionVectorBitTable2[7]    = { 28, 24, 20, 16, 12, 10, 8 };
static bool         m_relativeCompressionEnabled   = true;

struct BitStream {
    unsigned char* m_buffer;
    unsigned int   m_sizeBytes;
    unsigned int   m_bitLimit;
    unsigned int   m_readBitPos;
    unsigned int   m_bitPos;
    unsigned int   m_highWaterBits;
    bool           m_error;
    int            m_errorCode;

    bool writeBits(const void* src, unsigned int nbits);
    bool readBits(void* dst, unsigned int nbits);
    void writeCompressedVector(const Vec3& v, const Vec3& ref, float scale, const unsigned int* table);
    Vec3 readCompressedVector(const Vec3& ref, float scale, const unsigned int* table);
    void writeCompressedVector2(const Vec3& v, const Vec3& ref, float scale);
    Vec3 readCompressedVector2(const Vec3& ref, float scale);
    void  writeNormalVector(const Vec3& v, int nbits);
    Vec3  readNormalVector(int nbits);
    void  writeNormalAngle(float angle, int nbits);
    float readNormalAngle(int nbits);
    void  writeUnitQuaternion3Comp(const Quat& q, int nbits);
    Quat  readUnitQuaternion3Comp(int nbits);
    void  writeQuatComponent(float v, int nbits);
    float readQuatComponent(int nbits);
    void  writeUnitQuaternion(const Quat& q, int nbits);
    Quat  readUnitQuaternion(int nbits);
    Vec3  shrinkNormalVector(const Vec3& v, int nbits);
};

// verbatim from Code/BF2/IO/BitStream.cpp
bool BitStream::writeBits(const void* src, unsigned int nbits)
{
    if (nbits == 0)  return false;
    if (m_error)     return false;

    const unsigned int end = m_bitPos + nbits;
    if (end > (m_sizeBytes << 3) || (m_bitLimit != 0 && end > m_bitLimit))
    {
        m_error = true; m_errorCode = 1; return false;
    }

    const unsigned char* in = static_cast<const unsigned char*>(src);
    unsigned char*       firstPtr = m_buffer + (m_bitPos >> 3);
    unsigned char* const lastPtr  = m_buffer + ((end - 1) >> 3);
    const unsigned int   startBit = m_bitPos & 7;

    const unsigned char origLast  = *lastPtr;
    const unsigned char firstMask = (unsigned char)(0xFF >> (8 - startBit));
    const unsigned char lastMask  = (unsigned char)~(0xFF >> ((8 - end) & 7));

    unsigned char prev = *in++;
    *firstPtr = (unsigned char)((firstMask & *firstPtr) | (prev << startBit));
    ++firstPtr;

    while (firstPtr <= lastPtr)
    {
        const unsigned char cur = *in++;
        *firstPtr = (unsigned char)((cur << startBit) | (prev >> (8 - startBit)));
        ++firstPtr;
        prev = cur;
    }

    *lastPtr = (unsigned char)((~lastMask & *lastPtr) | (lastMask & origLast));

    m_bitPos = end;
    if (end > m_highWaterBits) m_highWaterBits = end;
    return true;
}

bool BitStream::readBits(void* dst, unsigned int nbits)
{
    if (dst == 0)   return false;
    if (nbits == 0) return false;

    const unsigned int end = m_readBitPos + nbits;
    if (end > m_highWaterBits) { m_error = true; m_errorCode = 2; return false; }

    const unsigned char* in  = m_buffer + (m_readBitPos >> 3);
    unsigned char*       out = static_cast<unsigned char*>(dst);

    const unsigned int bitOffset = m_readBitPos & 7;
    const unsigned int upShift   = 8 - bitOffset;

    unsigned int  remaining = nbits;
    unsigned char prev      = *in++;

    while (remaining > 7)
    {
        const unsigned char cur = *in++;
        *out++ = (unsigned char)((cur << upShift) | (prev >> bitOffset));
        remaining -= 8;
        prev = cur;
    }
    if (remaining > 0)
    {
        const unsigned char cur = *in;
        const unsigned char v   = (unsigned char)((cur << upShift) | (prev >> bitOffset));
        *out = (unsigned char)(v & ((1u << remaining) - 1u));
    }

    m_readBitPos = end;
    return true;
}


// verbatim from Code/BF2/IO/BitStream.cpp
void BitStream::writeCompressedVector(const Vec3& v, const Vec3& ref, float scale, const unsigned int* table)
{
    const float invScale = 1.0f / scale;
    const float dx = v.x - ref.x, dy = v.y - ref.y, dz = v.z - ref.z;
    const float scaledLen = sqrtf(dx*dx + dy*dy + dz*dz) * invScale;

    unsigned int selector = 0;
    if (m_relativeCompressionEnabled)
    {
        if      ((float)(1 << (table[3] - 1)) > scaledLen) selector = 3;
        else if ((float)(1 << (table[2] - 1)) > scaledLen) selector = 2;
        else if ((float)(1 << (table[1] - 1)) > scaledLen) selector = 1;
        else                                                                     selector = 0;
    }
    writeBits(&selector, 2);

    if (selector == 0)
    {
        writeBits(&v.x, 32); writeBits(&v.y, 32); writeBits(&v.z, 32);
        return;
    }
    const unsigned int width = table[selector];
    const float d[3] = { dx, dy, dz };
    for (int i = 0; i < 3; ++i)
    {
        int q = (int)(d[i] * invScale);
        unsigned int sign = 0;
        if (q < 0) { sign = 1; q = -q; }
        writeBits(&sign, 1);
        writeBits(&q, width - 1);
    }
}

Vec3 BitStream::readCompressedVector(const Vec3& ref, float scale, const unsigned int* table)
{
    Vec3 out; out.x = 0.0f; out.y = 0.0f; out.z = 0.0f;
    unsigned int selector = 0;
    readBits(&selector, 2);
    if (selector == 0)
    {
        readBits(&out.x, 32); readBits(&out.y, 32); readBits(&out.z, 32);
        return out;
    }
    const unsigned int width = table[selector];
    float q[3];
    for (int i = 0; i < 3; ++i)
    {
        unsigned int sign = 0; readBits(&sign, 1);
        int mag = 0;           readBits(&mag, width - 1);
        q[i] = (float)(sign ? -mag : mag);
    }
    out.x = q[0]*scale + ref.x; out.y = q[1]*scale + ref.y; out.z = q[2]*scale + ref.z;
    return out;
}


// verbatim from Code/BF2/IO/BitStream.cpp
void BitStream::writeCompressedVector2(const Vec3& v, const Vec3& ref, float scale)
{
    const float invScale = 1.0f / scale;
    const float dx = v.x - ref.x, dy = v.y - ref.y, dz = v.z - ref.z;
    const float len = sqrtf(dx*dx + dy*dy + dz*dz);

    if (1.1920928955078125e-07f > len)          // FLT_EPSILON -> the zero sentinel
    {
        unsigned int zeroSel = 7;
        writeBits(&zeroSel, 3);
        return;
    }
    const float scaledLen = len * invScale;

    unsigned int selector = 0;
    for (int i = 6; i >= 1; --i)
        if ((float)(1 << (m_compressionVectorBitTable2[i] - 1)) > scaledLen) { selector = (unsigned int)i; break; }

    writeBits(&selector, 3);
    if (selector == 0)
    {
        writeBits(&v.x, 32); writeBits(&v.y, 32); writeBits(&v.z, 32);
        return;
    }
    const unsigned int width = m_compressionVectorBitTable2[selector];
    const float d[3] = { dx, dy, dz };
    for (int i = 0; i < 3; ++i)
    {
        int q = (int)(d[i] * invScale);
        unsigned int sign = 0;
        if (q < 0) { sign = 1; q = -q; }
        writeBits(&sign, 1);
        writeBits(&q, width - 1);
    }
}

Vec3 BitStream::readCompressedVector2(const Vec3& ref, float scale)
{
    Vec3 out; out.x = 0.0f; out.y = 0.0f; out.z = 0.0f;
    unsigned int selector = 0;
    readBits(&selector, 3);
    if (selector == 7) { out = ref; return out; }
    if (selector == 0)
    {
        readBits(&out.x, 32); readBits(&out.y, 32); readBits(&out.z, 32);
        return out;
    }
    const unsigned int width = m_compressionVectorBitTable2[selector];
    float q[3];
    for (int i = 0; i < 3; ++i)
    {
        unsigned int sign = 0; readBits(&sign, 1);
        int mag = 0;           readBits(&mag, width - 1);
        q[i] = (float)(sign ? -mag : mag);
    }
    out.x = q[0]*scale + ref.x; out.y = q[1]*scale + ref.y; out.z = q[2]*scale + ref.z;
    return out;
}


// verbatim from Code/BF2/IO/BitStream.cpp
void BitStream::writeNormalVector(const Vec3& v, int nbits)
{
    const float azimuth = atan2f(v.y, v.x);
    float z = v.z;
    if (z >  1.0f) z =  1.0f;
    if (z < -1.0f) z = -1.0f;
    const float elevation = acosf(z);
    if (fabsf(azimuth) > 3.1415927410125732f || fabsf(elevation) > 3.1415927410125732f)
    { m_error = true; m_errorCode = 4; return; }
    writeNormalAngle(azimuth,   nbits);
    writeNormalAngle(elevation, nbits);
}

void BitStream::writeNormalAngle(float angle, int nbits)
{
    const float normalised = angle / 3.1415927410125732f;
    if (nbits == 32) { writeBits(&normalised, 32); return; }
    float unit = (normalised + 1.0f) * 0.5f;
    if (unit < 0.0f) unit = 0.0f;
    const unsigned int max = (1u << nbits) - 1u;
    unsigned int q = (unsigned int)(unit * (float)max);
    writeBits(&q, nbits);
}

Vec3 BitStream::readNormalVector(int nbits)
{
    const float azimuth   = readNormalAngle(nbits);
    const float elevation = readNormalAngle(nbits);
    Vec3 out;
    out.x = cosf(azimuth) * sinf(elevation);
    out.y = sinf(azimuth) * sinf(elevation);
    out.z = cosf(elevation);
    return out;
}

float BitStream::readNormalAngle(int nbits)
{
    float value;
    if (nbits == 32) { value = 0.0f; readBits(&value, 32); }
    else
    {
        const unsigned int max = (1u << nbits) - 1u;
        unsigned int q = 0;
        readBits(&q, nbits);
        value = ((float)q / (float)max) * 2.0f - 1.0f;
    }
    const unsigned int maxForQuantum = (nbits == 32) ? 0xFFFFFFFFu : ((1u << nbits) - 1u);
    const float quantum = 2.0f / (float)maxForQuantum;
    if (quantum >= fabsf(value)) value = 0.0f;
    return value * 3.1415927410125732f;
}


// verbatim from Code/BF2/IO/BitStream.cpp
void BitStream::writeUnitQuaternion3Comp(const Quat& q, int nbits)
{
    Quat v = q;
    if (fabsf(v.x) > 1.0f || fabsf(v.y) > 1.0f || fabsf(v.z) > 1.0f)
    {
        const float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
        if (len < 1.1920928955078125e-07f) { v.x=0.0f; v.y=0.0f; v.z=0.0f; v.w=1.0f; }
        else { v.x/=len; v.y/=len; v.z/=len; v.w/=len; }
    }
    const unsigned int sign = (v.w < 0.0f) ? 1u : 0u;
    writeBits(&sign, 1);
    writeQuatComponent(v.x, nbits);
    writeQuatComponent(v.y, nbits);
    writeQuatComponent(v.z, nbits);
}

void BitStream::writeQuatComponent(float v, int nbits)
{
    if (nbits == 32) { writeBits(&v, 32); return; }
    float unit = (v + 1.0f) * 0.5f;
    if (unit < 0.0f) unit = 0.0f;
    const unsigned int max = (1u << nbits) - 1u;
    unsigned int qv = (unsigned int)(unit * (float)max);
    writeBits(&qv, nbits);
}

Quat BitStream::readUnitQuaternion3Comp(int nbits)
{
    Quat out; out.x=0.0f; out.y=0.0f; out.z=0.0f; out.w=1.0f;
    unsigned int sign = 0;
    readBits(&sign, 1);
    out.x = readQuatComponent(nbits);
    out.y = readQuatComponent(nbits);
    out.z = readQuatComponent(nbits);
    float s = out.x*out.x + out.y*out.y + out.z*out.z;
    if (1.0f <= s) s = 1.0f;
    out.w = sqrtf(1.0f - s);
    if (sign) out.w = -out.w;
    return out;
}

float BitStream::readQuatComponent(int nbits)
{
    float value;
    if (nbits == 32) { value = 0.0f; readBits(&value, 32); }
    else
    {
        const unsigned int max = (1u << nbits) - 1u;
        unsigned int qv = 0;
        readBits(&qv, nbits);
        value = ((float)qv / (float)max) * 2.0f - 1.0f;
    }
    const unsigned int maxForQuantum = (nbits == 32) ? 0xFFFFFFFFu : ((1u << nbits) - 1u);
    const float quantum = 2.0f / (float)maxForQuantum;
    if (quantum >= fabsf(value)) value = 0.0f;
    return value;
}


// verbatim from Code/BF2/IO/BitStream.cpp (4-component quaternion + shrinkNormalVector)
static void writeUnitQuatComponent(BitStream& s, float v, int nbits)
{
    if (nbits == 32) { s.writeBits(&v, 32); return; }
    float unit = (v + 1.0f) * 0.5f;
    if (unit < 0.0f)      unit = 0.0f;
    else if (unit > 1.0f) unit = 1.0f;
    const unsigned int max = (1u << nbits) - 1u;
    const unsigned int qv = (unsigned int)(unit * (float)max);
    s.writeBits(&qv, nbits);
}
static float readUnitQuatComponent(BitStream& s, int nbits)
{
    if (nbits == 32) { float f = 0.0f; s.readBits(&f, 32); return f; }
    const unsigned int max = (1u << nbits) - 1u;
    unsigned int qv = 0;
    s.readBits(&qv, nbits);
    return ((float)qv / (float)max) * 2.0f - 1.0f;
}
void BitStream::writeUnitQuaternion(const Quat& q, int nbits)
{
    if (fabsf(q.x) > 1.0f || fabsf(q.y) > 1.0f || fabsf(q.z) > 1.0f)
    {
        Quat v = q;
        const float len = sqrtf(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
        if (len >= 1.1920928955078125e-07f) { v.w/=len; v.x/=len; v.y/=len; v.z/=len; }
        else { v.x=0.0f; v.y=0.0f; v.z=0.0f; v.w=1.0f; }
        writeUnitQuatComponent(*this, v.w, nbits);
        writeUnitQuatComponent(*this, v.x, nbits);
        writeUnitQuatComponent(*this, v.y, nbits);
        writeUnitQuatComponent(*this, v.z, nbits);
        return;
    }
    writeUnitQuatComponent(*this, q.w, nbits);
    writeUnitQuatComponent(*this, q.x, nbits);
    writeUnitQuatComponent(*this, q.y, nbits);
    writeUnitQuatComponent(*this, q.z, nbits);
}
Quat BitStream::readUnitQuaternion(int nbits)
{
    Quat out; out.x=0.0f; out.y=0.0f; out.z=0.0f; out.w=1.0f;
    out.w = readUnitQuatComponent(*this, nbits);
    out.x = readUnitQuatComponent(*this, nbits);
    out.y = readUnitQuatComponent(*this, nbits);
    out.z = readUnitQuatComponent(*this, nbits);
    const float len = sqrtf(out.w*out.w + out.x*out.x + out.y*out.y + out.z*out.z);
    if (len >= 1.1920928955078125e-07f) { out.w/=len; out.x/=len; out.y/=len; out.z/=len; }
    else { out.x=0.0f; out.y=0.0f; out.z=0.0f; out.w=1.0f; }
    return out;
}
Vec3 BitStream::shrinkNormalVector(const Vec3& v, int nbits)
{
    const float PI = 3.1415927410125732f;
    const int   max = (nbits != 32) ? ((1 << nbits) - 1) : -1;
    auto shrinkAngle = [&](float normalised) -> float
    {
        if (normalised < -1.0f)      normalised = -1.0f;
        else if (normalised > 1.0f)  normalised =  1.0f;
        float unit = (normalised + 1.0f) * 0.5f;
        if (unit < 0.0f)      unit = 0.0f;
        else if (unit > 1.0f) unit = 1.0f;
        const float q = (float)((int)(unit * (float)max)) / (float)max;
        return PI * (q + q - 1.0f);
    };
    const float azimuth = shrinkAngle(atan2f(v.y, v.x) / PI);
    float z = v.z;
    if (z < -1.0f)      z = -1.0f;
    else if (z > 1.0f)  z =  1.0f;
    const float elevation = shrinkAngle(acosf(z) / PI);
    Vec3 out;
    out.x = cosf(azimuth) * sinf(elevation);
    out.y = sinf(azimuth) * sinf(elevation);
    out.z = cosf(elevation);
    return out;
}


static unsigned int rnd()
{
    static unsigned int s = 0x13579BDFu;
    s ^= s << 13; s ^= s >> 17; s ^= s << 5;
    return s;
}

int main()
{
    unsigned char buf[512];
    int fails = 0, cases = 0, guardFails = 0;

    // Sweep: every leading pad 0..31 bits, every field width 1..64 bits.
    for (unsigned pad = 0; pad < 32; ++pad)
    {
        for (unsigned width = 1; width <= 64; ++width)
        {
            memset(buf, 0xA5, sizeof buf);          // poison, so gaps are visible

            BitStream bs; memset(&bs, 0, sizeof bs);
            bs.m_buffer = buf; bs.m_sizeBytes = sizeof buf;

            // pad, then the payload, then a sentinel field that must survive intact
            unsigned char padbytes[4] = {0,0,0,0};
            if (pad) bs.writeBits(padbytes, pad);

            unsigned char src[16], got[16];
            const unsigned nbytes = (width + 7) / 8;
            for (unsigned i = 0; i < nbytes; ++i) src[i] = (unsigned char)rnd();
            // clear bits above the field so the comparison is well defined
            if (width & 7) src[nbytes-1] &= (unsigned char)((1u << (width & 7)) - 1u);

            if (!bs.writeBits(src, width)) { ++fails; continue; }

            const unsigned char sentinel[2] = { 0x5C, 0x3A };
            const unsigned sentinelBits = 13;
            if (!bs.writeBits(sentinel, sentinelBits)) { ++fails; continue; }

            // read back
            bs.m_readBitPos = pad;
            memset(got, 0, sizeof got);
            if (!bs.readBits(got, width)) { ++fails; continue; }

            ++cases;
            if (memcmp(src, got, nbytes) != 0)
            {
                ++fails;
                if (fails <= 5)
                    printf("MISMATCH pad=%u width=%u\n", pad, width);
                continue;
            }

            // the sentinel written after must be intact — this is what catches a missing
            // last-byte restore in writeBits
            unsigned char sgot[2] = {0,0};
            if (!bs.readBits(sgot, sentinelBits)) { ++fails; continue; }
            unsigned char sexp[2] = { sentinel[0], (unsigned char)(sentinel[1] & 0x1F) };
            if (memcmp(sgot, sexp, 2) != 0)
            {
                ++fails;
                if (fails <= 5)
                    printf("SENTINEL CLOBBERED pad=%u width=%u got %02X %02X want %02X %02X\n",
                           pad, width, sgot[0], sgot[1], sexp[0], sexp[1]);
            }
        }
    }

    // Guard behaviour asserted from the disassembly.
    {
        BitStream bs; memset(&bs, 0, sizeof bs);
        bs.m_buffer = buf; bs.m_sizeBytes = sizeof buf;
        unsigned char x = 1;
        if (bs.writeBits(&x, 0) != false) { ++guardFails; printf("writeBits(0) should FAIL\n"); }
        if (bs.readBits(&x, 0)  != false) { ++guardFails; printf("readBits(0) should FAIL\n"); }
        if (bs.readBits(0, 4)   != false) { ++guardFails; printf("readBits(NULL) should FAIL\n"); }

        // read past the high-water mark -> errorCode 2
        BitStream b2; memset(&b2, 0, sizeof b2);
        b2.m_buffer = buf; b2.m_sizeBytes = sizeof buf;
        b2.writeBits(&x, 4);
        unsigned char o = 0;
        if (b2.readBits(&o, 8) != false || b2.m_errorCode != 2)
            { ++guardFails; printf("read past high-water should fail with code 2\n"); }

        // write past capacity -> errorCode 1
        BitStream b3; memset(&b3, 0, sizeof b3);
        b3.m_buffer = buf; b3.m_sizeBytes = 1;
        unsigned char big[4] = {1,2,3,4};
        if (b3.writeBits(big, 9) != false || b3.m_errorCode != 1)
            { ++guardFails; printf("write past capacity should fail with code 1\n"); }

        // bitLimit == 0 means unlimited, not zero capacity
        BitStream b4; memset(&b4, 0, sizeof b4);
        b4.m_buffer = buf; b4.m_sizeBytes = sizeof buf; b4.m_bitLimit = 0;
        if (b4.writeBits(&x, 8) != true) { ++guardFails; printf("bitLimit 0 must mean unlimited\n"); }
    }

    // compressed-vector round trip
    // Lossy by design: the check is "within one quantum", not bit-exact.
    int vecCases = 0, vecFails = 0, selHist[4] = {0,0,0,0};
    {
        const float scales[] = { 0.001f, 0.01f, 0.1f, 1.0f, 10.0f };
        const unsigned int* tables[2] = { m_compressionVectorBitTable, m_highCompressionVectorBitTable };
        for (int ti = 0; ti < 2; ++ti)
        for (int si = 0; si < 5; ++si)
        {
            const unsigned int* table = tables[ti];
            const float scale = scales[si];
            for (int trial = 0; trial < 400; ++trial)
            {
                unsigned char vbuf[64];
                memset(vbuf, 0, sizeof vbuf);
                BitStream bs; memset(&bs, 0, sizeof bs);
                bs.m_buffer = vbuf; bs.m_sizeBytes = sizeof vbuf;

                Vec3 ref, v;
                ref.x = (float)((int)(rnd() % 2001) - 1000);
                ref.y = (float)((int)(rnd() % 2001) - 1000);
                ref.z = (float)((int)(rnd() % 2001) - 1000);
                // deltas spanning well past the finest tier so every selector gets exercised
                const float span = scale * (float)(1 << (rnd() % 18));
                v.x = ref.x + span * (((float)(rnd() % 2001) / 1000.0f) - 1.0f);
                v.y = ref.y + span * (((float)(rnd() % 2001) / 1000.0f) - 1.0f);
                v.z = ref.z + span * (((float)(rnd() % 2001) / 1000.0f) - 1.0f);

                bs.writeCompressedVector(v, ref, scale, table);
                bs.m_readBitPos = 0;
                const Vec3 got = bs.readCompressedVector(ref, scale, table);

                // recover which tier was used, for coverage reporting
                unsigned int sel = (unsigned int)(vbuf[0] & 3);
                if (sel < 4) ++selHist[sel];

                ++vecCases;
                const float tol = (sel == 0) ? 0.0f : scale * 1.0001f;
                if (fabsf(got.x - v.x) > tol + 1e-4f ||
                    fabsf(got.y - v.y) > tol + 1e-4f ||
                    fabsf(got.z - v.z) > tol + 1e-4f)
                {
                    ++vecFails;
                    if (vecFails <= 5)
                        printf("VEC MISMATCH sel=%u scale=%g d=(%g %g %g) err=(%g %g %g)%s",
                               sel, scale, v.x-ref.x, v.y-ref.y, v.z-ref.z,
                               got.x-v.x, got.y-v.y, got.z-v.z, "\n");
                }
            }
        }
    }
    // CompressedVector2: 3-bit selector, and the zero sentinel
    int v2Cases = 0, v2Fails = 0, sel2Hist[8] = {0,0,0,0,0,0,0,0};
    {
        const float scales[] = { 0.001f, 0.01f, 0.1f, 1.0f, 10.0f };
        for (int si = 0; si < 5; ++si)
        {
            const float scale = scales[si];
            for (int trial = 0; trial < 400; ++trial)
            {
                unsigned char vbuf[64]; memset(vbuf, 0, sizeof vbuf);
                BitStream bs; memset(&bs, 0, sizeof bs);
                bs.m_buffer = vbuf; bs.m_sizeBytes = sizeof vbuf;

                Vec3 ref, v;
                ref.x = (float)((int)(rnd() % 2001) - 1000);
                ref.y = (float)((int)(rnd() % 2001) - 1000);
                ref.z = (float)((int)(rnd() % 2001) - 1000);
                if ((trial % 10) == 0) { v = ref; }      // exercise the zero sentinel
                else {
                    // the exponent range must exceed table2[1]=24 bits for selectors
                    // 1 and 0 to be reachable at all — an earlier 1<<16 left both at zero
                    // coverage, which the histogram caught.
                    const float span = scale * (float)(1u << (rnd() % 30));
                    v.x = ref.x + span * (((float)(rnd() % 2001)/1000.0f) - 1.0f);
                    v.y = ref.y + span * (((float)(rnd() % 2001)/1000.0f) - 1.0f);
                    v.z = ref.z + span * (((float)(rnd() % 2001)/1000.0f) - 1.0f);
                }

                bs.writeCompressedVector2(v, ref, scale);
                const unsigned int bitsWritten = bs.m_bitPos;
                bs.m_readBitPos = 0;
                const Vec3 got = bs.readCompressedVector2(ref, scale);

                unsigned int sel = (unsigned int)(vbuf[0] & 7);
                if (sel < 8) ++sel2Hist[sel];
                ++v2Cases;

                // the sentinel must cost exactly 3 bits and reproduce ref exactly
                if (sel == 7 && bitsWritten != 3)
                { ++v2Fails; if (v2Fails<=5) printf("SENTINEL SIZE %u bits, expected 3%c", bitsWritten, 10); continue; }

                const float tol = (sel == 0 || sel == 7) ? 0.0f : scale * 1.0001f;
                if (fabsf(got.x-v.x) > tol+1e-4f || fabsf(got.y-v.y) > tol+1e-4f || fabsf(got.z-v.z) > tol+1e-4f)
                { ++v2Fails; if (v2Fails<=5) printf("V2 MISMATCH sel=%u scale=%g%c", sel, scale, 10); }
            }
        }
    }
    // normal vectors: two spherical angles, quantised
    int nCases = 0, nFails = 0; float worstDot = 1.0f;
    {
        const int widths[] = { 8, 10, 12, 16, 20, 32 };
        for (int wi = 0; wi < 6; ++wi)
        {
            const int nbits = widths[wi];
            for (int trial = 0; trial < 300; ++trial)
            {
                unsigned char nbuf[64]; memset(nbuf, 0, sizeof nbuf);
                BitStream bs; memset(&bs, 0, sizeof bs);
                bs.m_buffer = nbuf; bs.m_sizeBytes = sizeof nbuf;

                Vec3 n;
                if ((trial % 12) == 0) { n.x=0.0f; n.y=0.0f; n.z=1.0f; }        // axis-aligned
                else if ((trial % 12) == 1) { n.x=1.0f; n.y=0.0f; n.z=0.0f; }
                else {
                    float x = ((float)(rnd()%2001)/1000.0f)-1.0f;
                    float y = ((float)(rnd()%2001)/1000.0f)-1.0f;
                    float z = ((float)(rnd()%2001)/1000.0f)-1.0f;
                    float L = sqrtf(x*x+y*y+z*z);
                    if (L < 1e-4f) { x=0.0f; y=0.0f; z=1.0f; L=1.0f; }
                    n.x=x/L; n.y=y/L; n.z=z/L;
                }

                bs.writeNormalVector(n, nbits);
                if (bs.m_error) { ++nFails; if (nFails<=5) printf("NORMAL write error code=%d w=%d%c", bs.m_errorCode, nbits, 10); continue; }
                bs.m_readBitPos = 0;
                const Vec3 got = bs.readNormalVector(nbits);

                ++nCases;
                // the decoded normal must still be unit length and point the same way
                const float len = sqrtf(got.x*got.x + got.y*got.y + got.z*got.z);
                const float dot = got.x*n.x + got.y*n.y + got.z*n.z;
                if (dot < worstDot) worstDot = dot;
                // one quantum of angle ~= PI * 2/(1<<nbits); allow a couple of quanta
                const float tolDot = (nbits >= 20) ? 0.99999f : (1.0f - 40.0f/(float)(1u<<nbits));
                if (fabsf(len - 1.0f) > 1e-3f || dot < tolDot)
                {
                    ++nFails;
                    if (nFails <= 6)
                        printf("NORMAL w=%d len=%g dot=%g n=(%g %g %g)%c", nbits, len, dot, n.x,n.y,n.z, 10);
                }
            }
        }
    }
    // unit quaternions, 3-component form
    int qCases = 0, qFails = 0; float worstQDot = 1.0f;
    {
        const int widths[] = { 8, 10, 12, 16, 20, 32 };
        for (int wi = 0; wi < 6; ++wi)
        {
            const int nbits = widths[wi];
            for (int trial = 0; trial < 300; ++trial)
            {
                unsigned char qbuf[64]; memset(qbuf, 0, sizeof qbuf);
                BitStream bs; memset(&bs, 0, sizeof bs);
                bs.m_buffer = qbuf; bs.m_sizeBytes = sizeof qbuf;

                Quat q;
                if ((trial % 15) == 0) { q.x=0.0f; q.y=0.0f; q.z=0.0f; q.w=1.0f; }        // identity
                else if ((trial % 15) == 1) { q.x=0.0f; q.y=0.0f; q.z=0.0f; q.w=-1.0f; }  // -identity
                else {
                    float a=((float)(rnd()%2001)/1000.0f)-1.0f, b=((float)(rnd()%2001)/1000.0f)-1.0f;
                    float c=((float)(rnd()%2001)/1000.0f)-1.0f, d=((float)(rnd()%2001)/1000.0f)-1.0f;
                    float L=sqrtf(a*a+b*b+c*c+d*d);
                    if (L<1e-4f) { a=0.0f;b=0.0f;c=0.0f;d=1.0f;L=1.0f; }
                    q.x=a/L; q.y=b/L; q.z=c/L; q.w=d/L;
                }

                bs.writeUnitQuaternion3Comp(q, nbits);
                bs.m_readBitPos = 0;
                const Quat got = bs.readUnitQuaternion3Comp(nbits);

                ++qCases;
                const float L2 = sqrtf(got.x*got.x+got.y*got.y+got.z*got.z+got.w*got.w);
                // a quaternion and its negation are the same rotation, so compare |dot|
                float dot = got.x*q.x+got.y*q.y+got.z*q.z+got.w*q.w;
                if (dot < 0.0f) dot = -dot;
                if (dot < worstQDot) worstQDot = dot;
                // Quantized x/y/z may have squared length above one; the decoder clamps only the square-root input.
                // Allow the resulting length error, especially near w = 0; do not normalize the decoded quaternion.
                const float q1     = 1.0f / (float)(1u << (nbits > 24 ? 24 : nbits));
                const float tolLen = 8.0f * q1 + 1e-4f;      // scales with the quantisation step
                const float tolDot = (nbits >= 20) ? 0.9999f : (1.0f - 60.0f/(float)(1u<<nbits));
                if (fabsf(L2 - 1.0f) > tolLen || dot < tolDot)
                {
                    ++qFails;
                    if (qFails <= 6)
                        printf("QUAT w=%d len=%g dot=%g q=(%g %g %g %g)%c",
                               nbits, L2, dot, q.x,q.y,q.z,q.w, 10);
                }
            }
        }
    }
    printf("quat cases: %d   failures: %d   worst |dot|: %g%c", qCases, qFails, worstQDot, 10);

    // unit quaternions, full 4-component form (writeUnitQuaternion / readUnitQuaternion)
    // Unlike the 3Comp pair, the reader renormalises, so the decoded quaternion is unit length.
    int q4Cases = 0, q4Fails = 0; float worstQ4Dot = 1.0f;
    {
        const int widths[] = { 8, 10, 12, 16, 20, 32 };
        for (int wi = 0; wi < 6; ++wi)
        {
            const int nbits = widths[wi];
            for (int trial = 0; trial < 300; ++trial)
            {
                unsigned char qbuf[64]; memset(qbuf, 0, sizeof qbuf);
                BitStream bs; memset(&bs, 0, sizeof bs);
                bs.m_buffer = qbuf; bs.m_sizeBytes = sizeof qbuf;

                Quat q;
                if ((trial % 15) == 0) { q.x=0.0f; q.y=0.0f; q.z=0.0f; q.w=1.0f; }        // identity
                else if ((trial % 15) == 1) { q.x=0.0f; q.y=0.0f; q.z=0.0f; q.w=-1.0f; }  // -identity
                else {
                    float a=((float)(rnd()%2001)/1000.0f)-1.0f, b=((float)(rnd()%2001)/1000.0f)-1.0f;
                    float c=((float)(rnd()%2001)/1000.0f)-1.0f, d=((float)(rnd()%2001)/1000.0f)-1.0f;
                    float L=sqrtf(a*a+b*b+c*c+d*d);
                    if (L<1e-4f) { a=0.0f;b=0.0f;c=0.0f;d=1.0f;L=1.0f; }
                    q.x=a/L; q.y=b/L; q.z=c/L; q.w=d/L;
                }

                bs.writeUnitQuaternion(q, nbits);
                bs.m_readBitPos = 0;
                const Quat got = bs.readUnitQuaternion(nbits);

                ++q4Cases;
                const float L2 = sqrtf(got.x*got.x+got.y*got.y+got.z*got.z+got.w*got.w);
                float dot = got.x*q.x+got.y*q.y+got.z*q.z+got.w*q.w;
                if (dot < 0.0f) dot = -dot;      // q and -q are the same rotation
                if (dot < worstQ4Dot) worstQ4Dot = dot;
                const float tolDot = (nbits >= 20) ? 0.9999f : (1.0f - 60.0f/(float)(1u<<nbits));
                if (fabsf(L2 - 1.0f) > 1e-3f || dot < tolDot)
                {
                    ++q4Fails;
                    if (q4Fails <= 6)
                        printf("QUAT4 w=%d len=%g dot=%g q=(%g %g %g %g)%c",
                               nbits, L2, dot, q.x,q.y,q.z,q.w, 10);
                }
            }
        }
    }
    printf("quat4 cases: %d   failures: %d   worst |dot|: %g%c", q4Cases, q4Fails, worstQ4Dot, 10);

    // shrinkNormalVector: pure-math preview, no callers in the tree
    // Verified independently: it must return a unit vector (reconstructed from cos/sin) pointing at
    // the quantised direction. It ignores the stream, so a zeroed BitStream is a fine receiver.
    int shrCases = 0, shrFails = 0; float worstShrDot = 1.0f;
    {
        const int widths[] = { 8, 10, 12, 16, 20 };
        for (int wi = 0; wi < 5; ++wi)
        {
            const int nbits = widths[wi];
            for (int trial = 0; trial < 300; ++trial)
            {
                float x=((float)(rnd()%2001)/1000.0f)-1.0f;
                float y=((float)(rnd()%2001)/1000.0f)-1.0f;
                float z=((float)(rnd()%2001)/1000.0f)-1.0f;
                float L=sqrtf(x*x+y*y+z*z);
                if (L<1e-4f) { x=0.0f; y=0.0f; z=1.0f; L=1.0f; }
                Vec3 n; n.x=x/L; n.y=y/L; n.z=z/L;

                BitStream bs; memset(&bs, 0, sizeof bs);        // shrink ignores `this`/the stream
                const Vec3 shr = bs.shrinkNormalVector(n, nbits);
                ++shrCases;
                const float slen = sqrtf(shr.x*shr.x + shr.y*shr.y + shr.z*shr.z);
                const float dotN = shr.x*n.x + shr.y*n.y + shr.z*n.z;
                if (dotN < worstShrDot) worstShrDot = dotN;
                const float tolDot = (nbits >= 20) ? 0.9999f : (1.0f - 60.0f/(float)(1u<<nbits));
                if (fabsf(slen - 1.0f) > 1e-4f || dotN < tolDot)
                {
                    ++shrFails;
                    if (shrFails <= 6)
                        printf("SHRINK w=%d len=%g dot=%g n=(%g %g %g)%c", nbits, slen, dotN, n.x,n.y,n.z, 10);
                }
            }
        }
    }
    printf("shrink cases: %d   failures: %d   worst dot: %g%c", shrCases, shrFails, worstShrDot, 10);

    printf("normal cases: %d   failures: %d   worst dot: %g%c", nCases, nFails, worstDot, 10);

    printf("v2 cases: %d   failures: %d   selector coverage: 0=%d 1=%d 2=%d 3=%d 4=%d 5=%d 6=%d 7=%d%c",
           v2Cases, v2Fails, sel2Hist[0], sel2Hist[1], sel2Hist[2], sel2Hist[3],
           sel2Hist[4], sel2Hist[5], sel2Hist[6], sel2Hist[7], 10);

    printf("vector cases: %d   failures: %d   selector coverage: 0=%d 1=%d 2=%d 3=%d%s",
           vecCases, vecFails, selHist[0], selHist[1], selHist[2], selHist[3], "\n");

    printf("round-trip cases: %d   failures: %d   guard failures: %d\n", cases, fails, guardFails);
    return (fails || guardFails || vecFails || v2Fails || nFails || qFails || q4Fails || shrFails) ? 1 : 0;
}
