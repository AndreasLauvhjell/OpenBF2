// Bit packing and compressed values used by network events and demos.
// Companion paths IO/BitStream.cpp and IO/BitStream.inl are attested.
// The constructor sizes buffers in bytes; read/write positions and budgets use bits.
// Preserve the distinct reader/writer guards and quantization rules.

#ifndef IO_NETWORK_BITSTREAM_H     // [INFERRED] guard style
#define IO_NETWORK_BITSTREAM_H

// do not forward-declare Vec3. `dice::hfe::Vec3` is a typedef, not a class, so `class Vec3;`
// gives C2371 here and cascades into C2079 in every file that has a Vec3 member (SoldierTemplate.h
// hit it). This is the third time this trap has been sprung in this project — include the header.
#include "../Math/Vec3.h"
#include "../Math/Quat.h"

namespace dice {
namespace hfe {
namespace io {

// Saved stream position: 16 bytes, with the bit position at +0x04 and mode at +0x0C.
// setPosition (ELF 0x807E500) and setWritePosition (0x8431140) confirm the layout.
struct BSPosition
{
    // [UNRESOLVED] +0x00 and +0x08 are not accessed by the recovered bodies.
    int          m_unknown00;         // +0x00  [UNRESOLVED]
    unsigned int m_position;          // +0x04
    int          m_unknown08;         // +0x08  [UNRESOLVED]
    int          m_readWriteSwitch;   // +0x0C
};

// Bit-oriented stream over an owned or borrowed byte buffer.
// Read and write cursors are independent; the mode selects position/budget accounting.
class BitStream
{
public:
    // sizeBytes is in bytes. The size-only form allocates and owns the buffer;
    // the pointer form borrows the caller's storage. Both initialize the stream.
    explicit BitStream(unsigned int sizeBytes);
    BitStream(unsigned char* buf, unsigned int sizeBytes);
    ~BitStream();

    void init(unsigned char* buf, unsigned int sizeBytes);
    bool setBuffer(unsigned char* buf, unsigned int sizeBytes); // init + setBitSize*8
    void setBitSize(unsigned int nbits);
    bool setData(unsigned char* src, unsigned int n);        // written (ret bool)
    void clear();
    bool setPosition(BSPosition&);
    // Save the current position and mode, then seek to the caller's marker for backfilling.
    // ELF 0x8431140; the parameter is BSPosition&, including the mode.
    bool setWritePosition(BSPosition&);
    // Restore the marker saved by setWritePosition and clear its pending flag.
    // Without a saved marker, report the shipped diagnostic and leave the position unchanged.
    // [CHOSEN] void return type; attested inline path: IO/BitStream.inl:341.
    void resetWritePosition();
    // Advance the cursor; false reports one of the shipped guard failures (ELF 0x8431530).
    bool skipBits(unsigned int);

    bool writeBits(const void* src, unsigned int nbits);     // nbits is bits
    bool readBits(void* dst, unsigned int nbits);

    // [CHOSEN] accessor for MapInfo's direct +0x14 write-cursor read.
    // Buffer ownership at +0x0C is independent of the read/write mode at +0x34.
    unsigned int getWriteBitPosition() const { return m_bitPos; }

    // [CHOSEN] Returns borrowed buffer storage for packet transmission.
    unsigned char* getBuffer() const { return m_buffer; }

    // [CHOSEN] Returns allocated byte capacity for network budget checks.
    unsigned int getSizeBytes() const { return m_sizeBytes; }

    void setCompressionVector(const Vec3&);
    void resetCompressionVector();

    // Quantised forms — [UNRESOLVED] bit layouts, see the table above.
    void writeNormalVector(/*...*/);            void readNormalVector(/*...*/);
    // implemented in BitStream.cpp — both read end to end including every sign branch and both
    // uncompressed paths. Round-trip verified by Code/Tests/bitstream_roundtrip.cmd.
    void writeCompressedVector(const Vec3& v, const Vec3& ref, float scale);
    Vec3 readCompressedVector(const Vec3& ref, float scale);

private:
    // [CHOSEN, not recovered] shared helpers for instruction-equivalent codecs.
    // CompressedVector and HighCompressedVector differ only in their width tables.
    void writeCompressedVectorWithTable(const Vec3& v, const Vec3& ref, float scale,
                                        const unsigned int* table);
    Vec3 readCompressedVectorWithTable(const Vec3& ref, float scale,
                                       const unsigned int* table);
public:
    // Encodes or decodes a relative vector using the high-compression width table.
    void writeHighCompressedVector(const Vec3& v, const Vec3& ref, float scale);
    Vec3 readHighCompressedVector(const Vec3& ref, float scale);

    // Uses a 3-bit selector; 7 encodes a delta below FLT_EPSILON with no payload.
    // This codec ignores m_relativeCompressionEnabled and has distinct control flow.
    void writeCompressedVector2(const Vec3& v, const Vec3& ref, float scale);
    Vec3 readCompressedVector2(const Vec3& ref, float scale);
    // Encodes or decodes a normal direction. Decoding snaps near-zero components to zero.
    void  writeNormalVector(const Vec3& v, int nbits);
    Vec3  readNormalVector(int nbits);
private:
    // Per-angle helpers. The binary writes both angles out longhand; this factoring is a rebuild
    // convenience, and the two blocks were verified identical before splitting.
    void  writeNormalAngle(float angle, int nbits);
    float readNormalAngle(int nbits);
    void  writeQuatComponent(float v, int nbits);
    float readQuatComponent(int nbits);
public:

    // Serializes all four quaternion components in W, X, Y, Z order, normalizing when required.
    void writeUnitQuaternion(const Quat& q, int nbits);
    Quat readUnitQuaternion(int nbits);
    // Encodes or decodes three quaternion components, normalizing out-of-range input without setting an error.
    void writeUnitQuaternion3Comp(const Quat& q, int nbits);
    Quat readUnitQuaternion3Comp(int nbits);
    // Returns the normal-vector encoding round trip without reading or writing this stream.
    Vec3 shrinkNormalVector(const Vec3& v, int nbits);

    // process-wide, not per-stream.
    static bool m_relativeCompressionEnabled;

    // Compression bit-width tables. Selector zero always writes raw floats.
    // The first two formats use two-bit selectors; the third uses three bits.
    static unsigned int m_compressionVectorBitTable[4];
    static unsigned int m_highCompressionVectorBitTable[4];
    static unsigned int m_compressionVectorBitTable2[7];

private:
    /* +0x00 */ unsigned char* m_buffer;
    /* +0x04 */ unsigned int   m_sizeBytes;      // bytes
public:
    /* +0x08 */ unsigned int   m_bitLimit;       // bits; 0 means unlimited
    // True only for the allocating constructor; the destructor frees only owned storage.
    // This flag is independent of m_readWriteSwitch.
    /* +0x0C */ bool m_ownsBuffer;
    /* +0x10 */ unsigned int   m_readBitPos;     // bits. separate from m_bitPos — readBits
                                                 // advances this one and nothing else touches it.
    /* +0x14 */ unsigned int   m_bitPos;         // bits — the write cursor
    /* +0x18 */ unsigned int   m_highWaterBits;  // bits. resolved from writeBits' tail: raised to
                                                 // m_bitPos whenever m_bitPos exceeds it, and never
                                                 // lowered there. It records the furthest the stream
                                                 // has ever been written, independent of the current
                                                 // position — which is why seeking backwards to
                                                 // patch a length does not shrink the packet.
    /* +0x1C */ bool           m_error;       // IDA: "reachedEndOfStream" - the same latch the
                                              //   networkables test after their write/read runs
    /* +0x20 */ int            m_errorCode; // 0 cleared, 1 write overrun, 2 read overrun.
    // Save/restore fields are shared with GhostManager transmission. Some field names remain [UNRESOLVED].
public:
    /* +0x24 */ int            m_dword24;            // clear() zeroes
    /* +0x28 */ unsigned int   m_writePosition;      // IDA "writePosition"; clear() zeroes
    /* +0x2C */ bool           m_byte2C;             // clear() zeroes
    /* +0x30 */ int            m_readWriteSwitch2;   // IDA "readWriteSwitch2"
    /* +0x34 */ int            m_readWriteSwitch;    // read: 1 = "position is the READ cursor".
                                                     //   Every *Networkable::get/setNetUpdate opens
                                                     //   with `cmp [stream+34h], 1` to pick +0x10
                                                     //   over +0x14 for its bit accounting; clear()
                                                     //   zeroes it (write mode)
    /* +0x38 */ bool           m_byte38;             // clear() zeroes
    /* +0x39 */ bool           m_byte39;
    /* +0x3A */ bool           m_byte3A;
    /* +0x3B */ bool           m_byte3B;
    /* +0x3C */ int            m_dword3C;
    /* +0x40 */ unsigned int   m_positionCache;      // IDA "positionCache"
    /* +0x44 */ int            m_dword44;
    /* +0x48 */ int            m_rwSwitchCache;      // IDA "rwSwitchCache"
    /* +0x4C */ unsigned int   m_lengthCache;        // IDA "lengthCache"
    /* +0x50 */ bool           m_canResetWritePosition;   // IDA; clear() zeroes
    /* +0x54 */ float          m_compressionVector[3];
    /* +0x60 */ bool           m_hasCompressionVector;

public:
    // [CHOSEN] accessors for the pattern every networkable's wire pair inlines - the current cursor
    // (`m_readWriteSwitch == 1 ? +0x10 : +0x14`) taken before and after the run to count the bits
    // moved into its RegistredNetworkStats, and the error latch tested and cleared afterwards
    // (`m_errorCode = 0; m_error = false` - both, in that order, as shipped).
    unsigned int getBitPosition() const { return m_readWriteSwitch == 1 ? m_readBitPos : m_bitPos; }
    bool hasError() const { return m_error; }
    void clearError() { m_errorCode = 0; m_error = false; }

    // [CHOSEN] Returns capacity remaining after the active cursor, using the read high-water mark
    // or the configured write limit and buffer capacity.
    unsigned int getRemainingBitCapacity() const
    {
        if (m_readWriteSwitch == 1)
            return m_highWaterBits - m_readBitPos;
        const unsigned int limit = m_bitLimit ? m_bitLimit : m_sizeBytes * 8;
        return limit - m_bitPos;
    }

    // [CHOSEN] Switches the stream to read mode for packet-header decoding.
    void setReadMode() { m_readWriteSwitch = 1; }

    // [CHOSEN] accessor for ClientConnection::newPacket's direct mode reset
    // (ELF 0x80856F0). Mode 0 selects the write cursor.
    void setWriteMode() { m_readWriteSwitch = 0; }

    // [CHOSEN] accessor for NetServer::receive's two cursor resets
    // (ELF 0x84749DA/0x84749E1). setBuffer does not perform these resets.
    void resetPositions() { m_readBitPos = 0; m_bitPos = 0; }

    // [CHOSEN] accessors for event codecs that use the stream's compression reference.
    // KilledByEvent tests the flag before selecting compressed or raw vector encoding.
    bool        hasCompressionVector() const { return m_hasCompressionVector; }   // +0x60
    const Vec3& getCompressionVector() const
        { return *reinterpret_cast<const Vec3*>(m_compressionVector); }           // +0x54

    // [CHOSEN] accessor for the inlined range-error path (ELF 0x8430A86).
    // Preserve the store order: error flag first, error code second.
    void setError(int code) { m_error = true; m_errorCode = code; }
};

} // namespace io
} // namespace hfe
} // namespace dice

#endif
